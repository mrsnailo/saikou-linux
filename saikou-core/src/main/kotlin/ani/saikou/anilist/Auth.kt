package ani.saikou.anilist

import ani.saikou.host.Log
import ani.saikou.host.Paths
import ani.saikou.host.Preferences
import ani.saikou.net.Http
import ani.saikou.rpc.ErrorCodes
import ani.saikou.rpc.RpcException
import com.sun.net.httpserver.HttpExchange
import com.sun.net.httpserver.HttpServer
import kotlinx.serialization.Serializable
import kotlinx.serialization.json.JsonPrimitive
import kotlinx.serialization.json.jsonPrimitive
import java.net.InetSocketAddress
import java.net.URLDecoder
import java.nio.file.Files
import java.nio.file.attribute.PosixFilePermissions
import java.util.concurrent.CompletableFuture
import java.util.concurrent.TimeUnit
import kotlin.io.path.deleteIfExists
import kotlin.io.path.exists
import kotlin.io.path.readText

/**
 * AniList sign-in: press one button, approve in the browser, done.
 *
 * Only the authorization-code grant is available. AniList answers `response_type=token`
 * with `unsupported_grant_type`, and its token endpoint answers `invalid_client` when the
 * request carries no secret — so the implicit grant and PKCE are both off the table, and
 * signing in means holding a client id *and* secret.
 *
 * Those come from [BundledClient], written at build time from the environment and never
 * committed. A build without them still signs in, but the user has to supply a client of
 * their own first (see [configure]).
 */
object Auth {
    private const val TAG = "AniListAuth"

    const val DEFAULT_PORT = 8998
    const val CALLBACK_PATH = "/callback"

    private const val CLIENT_ID_KEY = "anilist.clientId"
    private const val CLIENT_SECRET_KEY = "anilist.clientSecret"
    private const val PORT_KEY = "anilist.port"

    private val tokenFile = Paths.data.resolve("anilist.token")

    @Volatile
    var token: String? = null
        private set

    /** An id and its matching secret. The two are never mixed across sources. */
    private data class Client(val id: String, val secret: String)

    private val ownClient: Client?
        get() {
            val id = Preferences.get(CLIENT_ID_KEY)?.jsonPrimitive?.contentOrNull() ?: return null
            val secret = Preferences.get(CLIENT_SECRET_KEY)?.jsonPrimitive?.contentOrNull() ?: return null
            return Client(id, secret)
        }

    /**
     * The user's client wins over the built-in one, and the pair is taken whole: pairing a
     * user's id with the bundled secret would authenticate as neither client.
     */
    private val activeClient: Client?
        get() = ownClient
            ?: environmentClient()
            ?: Client(BundledClient.ID, BundledClient.SECRET).takeIf {
                it.id.isNotBlank() && it.secret.isNotBlank()
            }

    private fun environmentClient(): Client? {
        val id = System.getenv("SAIKOU_ANILIST_CLIENT_ID")?.takeIf { it.isNotBlank() } ?: return null
        val secret = System.getenv("SAIKOU_ANILIST_CLIENT_SECRET")?.takeIf { it.isNotBlank() } ?: return null
        return Client(id, secret)
    }

    val clientId: String? get() = activeClient?.id

    val port: Int get() = Preferences.get(PORT_KEY)?.jsonPrimitive?.contentOrNull()?.toIntOrNull() ?: DEFAULT_PORT

    val redirectUri: String get() = "http://localhost:$port$CALLBACK_PATH"

    /** True when a sign-in can be started at all. */
    val isConfigured: Boolean get() = activeClient != null

    /** True when the user pointed Saikou at their own AniList client instead of this build's. */
    val usesOwnClient: Boolean get() = ownClient != null

    /** True when this build carries a client of its own, so the user needs to supply nothing. */
    val hasBundledClient: Boolean get() = BundledClient.ID.isNotBlank() && BundledClient.SECRET.isNotBlank()

    val isLoggedIn: Boolean get() = token != null

    /**
     * Points sign-in at a client of the user's own. Both values are required — AniList
     * rejects a token request without a secret — and blanking either returns to the
     * built-in client.
     */
    fun configure(clientId: String, clientSecret: String, port: Int?) {
        Preferences.set(CLIENT_ID_KEY, clientId.trim().takeIf { it.isNotEmpty() }?.let { JsonPrimitive(it) })
        Preferences.set(CLIENT_SECRET_KEY, clientSecret.trim().takeIf { it.isNotEmpty() }?.let { JsonPrimitive(it) })
        port?.let { Preferences.set(PORT_KEY, JsonPrimitive(it)) }
    }

    fun loadSavedToken() {
        token = if (tokenFile.exists()) tokenFile.readText().trim().takeIf { it.isNotEmpty() } else null
        if (token != null) Log.i(TAG, "restored saved AniList login")
    }

    fun logout() {
        token = null
        tokenFile.deleteIfExists()
    }

    /** The url the UI opens in the user's browser. */
    fun authorizeUrl(): String {
        val client = activeClient ?: throw noClientError()
        return "https://anilist.co/api/v2/oauth/authorize" +
            "?client_id=${client.id}&redirect_uri=$redirectUri&response_type=code"
    }

    /**
     * Runs the loopback listener until the browser comes back with a token, then stores it.
     * Blocks the calling coroutine, so callers dispatch it off the RPC read loop.
     */
    suspend fun awaitLogin(timeoutSeconds: Long = 300): String {
        if (!isConfigured) throw noClientError()

        val client = activeClient ?: throw noClientError()
        val code = CompletableFuture<String>()

        val server = try {
            HttpServer.create(InetSocketAddress("127.0.0.1", port), 0)
        } catch (e: Exception) {
            throw RpcException(
                ErrorCodes.INTERNAL_ERROR,
                "Could not listen on port $port for the AniList redirect: ${e.message}",
                retryable = true,
                cause = e,
            )
        }

        server.createContext(CALLBACK_PATH) { exchange ->
            val params = parseQuery(exchange.requestURI.rawQuery)
            when {
                params["code"] != null -> {
                    code.complete(params.getValue("code"))
                    exchange.reply(page("Signed in", "You can close this tab and go back to Saikou."))
                }

                params["error"] != null -> {
                    val message = params["error_description"] ?: params.getValue("error")
                    code.completeExceptionally(IllegalStateException(message))
                    exchange.reply(page("Sign-in failed", message))
                }

                else -> {
                    code.completeExceptionally(IllegalStateException("no authorization code returned"))
                    exchange.reply(page("Sign-in failed", "AniList did not return an authorization code."))
                }
            }
        }

        server.start()
        Log.i(TAG, "waiting for AniList redirect on $redirectUri")

        val authorizationCode = try {
            code.get(timeoutSeconds, TimeUnit.SECONDS)
        } catch (e: Exception) {
            throw RpcException(
                ErrorCodes.NOT_AUTHENTICATED,
                "AniList sign-in did not complete: ${e.cause?.message ?: e.message}",
                retryable = true,
            )
        } finally {
            // Give the browser a moment to finish reading the "signed in" page before the
            // socket goes away, or the tab shows a connection error on success.
            server.stop(1)
        }

        return exchangeCode(client, authorizationCode)
    }

    private fun noClientError() = RpcException(
        ErrorCodes.NOT_AUTHENTICATED,
        "This build carries no AniList client, so one-click sign-in is unavailable. Add your " +
            "own client id and secret under Settings → Account, or set " +
            "SAIKOU_ANILIST_CLIENT_ID and SAIKOU_ANILIST_CLIENT_SECRET.",
    )

    private suspend fun exchangeCode(client: Client, code: String): String {
        val response = Http.post(
            "https://anilist.co/api/v2/oauth/token",
            headers = mapOf("Accept" to "application/json"),
            data = mapOf(
                "grant_type" to "authorization_code",
                "client_id" to client.id,
                "client_secret" to client.secret,
                "redirect_uri" to redirectUri,
                "code" to code,
            ),
        )

        if (!response.isSuccess) {
            throw RpcException(
                ErrorCodes.NOT_AUTHENTICATED,
                "AniList refused the token exchange (HTTP ${response.code}). " +
                    "Check that the client's redirect url is exactly $redirectUri.",
            )
        }

        val parsed = runCatching { response.parsed<TokenResponse>() }.getOrNull()
            ?: throw RpcException(ErrorCodes.NOT_AUTHENTICATED, "AniList returned an unreadable token response")

        store(parsed.accessToken)
        Log.i(TAG, "AniList sign-in complete")
        return parsed.accessToken
    }

    private fun store(value: String) {
        Files.createDirectories(Paths.data)
        Files.writeString(tokenFile, value)
        // The token grants full access to the user's list; keep it off other accounts.
        runCatching {
            Files.setPosixFilePermissions(tokenFile, PosixFilePermissions.fromString("rw-------"))
        }.onFailure { Log.w(TAG, "could not restrict permissions on the token file", it) }
        token = value
    }

    private fun HttpExchange.reply(body: String) {
        val bytes = body.toByteArray()
        responseHeaders.add("Content-Type", "text/html; charset=utf-8")
        sendResponseHeaders(200, bytes.size.toLong())
        responseBody.use { it.write(bytes) }
    }

    private fun parseQuery(raw: String?): Map<String, String> =
        raw.orEmpty().split('&').mapNotNull {
            val parts = it.split('=', limit = 2)
            if (parts.size == 2) {
                URLDecoder.decode(parts[0], "utf-8") to URLDecoder.decode(parts[1], "utf-8")
            } else null
        }.toMap()

    private fun page(title: String, message: String) = """
        <!doctype html>
        <html><head><meta charset="utf-8"><title>$title</title>
        $STYLE
        </head>
        <body><div><h1>$title</h1><p>$message</p></div></body></html>
    """.trimIndent()

    private val STYLE = """
        <style>
          body { font-family: system-ui, sans-serif; display: grid; place-content: center;
                 height: 100vh; margin: 0; background: #161516; color: #ffffff; text-align: center; }
          h1 { font-weight: 600; font-size: 1.4rem; margin-bottom: .5rem; color: #FB5DAC; }
          p { color: #9d9d9b; }
        </style>
    """.trimIndent()

    @Serializable
    private data class TokenResponse(
        val access_token: String,
        val token_type: String? = null,
        val expires_in: Long? = null,
    ) {
        val accessToken: String get() = access_token
    }
}

private fun kotlinx.serialization.json.JsonPrimitive.contentOrNull(): String? =
    runCatching { content }.getOrNull()?.takeIf { it.isNotBlank() && it != "null" }
