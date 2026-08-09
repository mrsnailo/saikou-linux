package ani.saikou.anilist

import ani.saikou.host.Log
import ani.saikou.host.Paths
import ani.saikou.host.Preferences
import ani.saikou.net.Http
import ani.saikou.rpc.ErrorCodes
import ani.saikou.rpc.RpcException
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
 * AniList sign-in using the authorization code grant with a loopback redirect.
 *
 * The implicit grant the Android app uses returns the token in the URL *fragment*, which
 * a local listener never receives — so desktop uses the code grant instead. That needs a
 * client id and secret, which means each user registers their own AniList API client at
 * https://anilist.co/settings/developer with the redirect url below.
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

    val clientId: String? get() = Preferences.get(CLIENT_ID_KEY)?.jsonPrimitive?.contentOrNull()
    val clientSecret: String? get() = Preferences.get(CLIENT_SECRET_KEY)?.jsonPrimitive?.contentOrNull()
    val port: Int get() = Preferences.get(PORT_KEY)?.jsonPrimitive?.contentOrNull()?.toIntOrNull() ?: DEFAULT_PORT

    val redirectUri: String get() = "http://localhost:$port$CALLBACK_PATH"

    val isConfigured: Boolean get() = !clientId.isNullOrBlank() && !clientSecret.isNullOrBlank()
    val isLoggedIn: Boolean get() = token != null

    fun configure(clientId: String, clientSecret: String, port: Int?) {
        Preferences.set(CLIENT_ID_KEY, JsonPrimitive(clientId.trim()))
        Preferences.set(CLIENT_SECRET_KEY, JsonPrimitive(clientSecret.trim()))
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
        val id = clientId ?: throw RpcException(
            ErrorCodes.NOT_AUTHENTICATED,
            "No AniList client is configured. Create one at https://anilist.co/settings/developer.",
        )
        return "https://anilist.co/api/v2/oauth/authorize" +
            "?client_id=$id&redirect_uri=$redirectUri&response_type=code"
    }

    /**
     * Runs the loopback listener until AniList redirects back, then trades the code for a
     * token. Blocks the calling coroutine, so callers dispatch it off the RPC read loop.
     */
    suspend fun awaitLogin(timeoutSeconds: Long = 300): String {
        if (!isConfigured) {
            throw RpcException(
                ErrorCodes.NOT_AUTHENTICATED,
                "Set your AniList client id and secret first.",
            )
        }

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
            val received = params["code"]
            val error = params["error"]

            val body = if (received != null) {
                code.complete(received)
                page("Signed in", "You can close this tab and go back to Saikou.")
            } else {
                code.completeExceptionally(IllegalStateException(error ?: "no code returned"))
                page("Sign-in failed", error ?: "AniList did not return an authorization code.")
            }

            val bytes = body.toByteArray()
            exchange.responseHeaders.add("Content-Type", "text/html; charset=utf-8")
            exchange.sendResponseHeaders(200, bytes.size.toLong())
            exchange.responseBody.use { it.write(bytes) }
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
            server.stop(0)
        }

        return exchangeCode(authorizationCode)
    }

    private suspend fun exchangeCode(code: String): String {
        val response = Http.post(
            "https://anilist.co/api/v2/oauth/token",
            headers = mapOf("Accept" to "application/json"),
            data = mapOf(
                "grant_type" to "authorization_code",
                "client_id" to clientId.orEmpty(),
                "client_secret" to clientSecret.orEmpty(),
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
        <style>
          body { font-family: system-ui, sans-serif; display: grid; place-content: center;
                 height: 100vh; margin: 0; background: #16181d; color: #e8eaed; text-align: center; }
          h1 { font-weight: 600; font-size: 1.4rem; margin-bottom: .5rem; }
          p { color: #9aa0a6; }
        </style></head>
        <body><div><h1>$title</h1><p>$message</p></div></body></html>
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
