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
 * The Android app uses the implicit grant, which returns the token in the URL *fragment*.
 * A fragment never reaches a server, which is why this used to fall back to the code grant
 * and make every user register their own API client just to log in. Instead the loopback
 * listener now answers the redirect with a page whose only job is to hand `location.hash`
 * straight back to `/callback/token`, so the implicit grant works on the desktop too — no
 * client secret, and nothing for the user to create or paste.
 *
 * [DEFAULT_CLIENT_ID] is the application's own registered client. A user can still point
 * Saikou at a client of their own (see [configure]); supplying a secret alongside it
 * switches back to the code grant, which is the stricter of the two.
 */
object Auth {
    private const val TAG = "AniListAuth"

    /**
     * The AniList API client this build signs in with. It has to be registered at
     * https://anilist.co/settings/developer with the redirect url `http://localhost:8998/callback`.
     * A client id is public by design — the implicit grant carries no secret — so shipping
     * it is how the one-click flow is possible at all.
     *
     * Overridable at runtime with SAIKOU_ANILIST_CLIENT_ID, which is also how a fork avoids
     * having to patch this constant.
     */
    const val DEFAULT_CLIENT_ID = "48249"

    const val DEFAULT_PORT = 8998
    const val CALLBACK_PATH = "/callback"
    private const val TOKEN_PATH = "/callback/token"

    private const val CLIENT_ID_KEY = "anilist.clientId"
    private const val CLIENT_SECRET_KEY = "anilist.clientSecret"
    private const val PORT_KEY = "anilist.port"

    private val tokenFile = Paths.data.resolve("anilist.token")

    @Volatile
    var token: String? = null
        private set

    /** The client the user supplied, if any. */
    private val ownClientId: String?
        get() = Preferences.get(CLIENT_ID_KEY)?.jsonPrimitive?.contentOrNull()

    val clientId: String?
        get() = ownClientId
            ?: System.getenv("SAIKOU_ANILIST_CLIENT_ID")?.takeIf { it.isNotBlank() }
            ?: DEFAULT_CLIENT_ID.takeIf { it.isNotBlank() }

    val clientSecret: String? get() = Preferences.get(CLIENT_SECRET_KEY)?.jsonPrimitive?.contentOrNull()

    val port: Int get() = Preferences.get(PORT_KEY)?.jsonPrimitive?.contentOrNull()?.toIntOrNull() ?: DEFAULT_PORT

    val redirectUri: String get() = "http://localhost:$port$CALLBACK_PATH"

    /** True when a sign-in can be started at all. */
    val isConfigured: Boolean get() = !clientId.isNullOrBlank()

    /** True when the user pointed Saikou at their own AniList client instead of this build's. */
    val usesOwnClient: Boolean get() = !ownClientId.isNullOrBlank()

    val isLoggedIn: Boolean get() = token != null

    /**
     * Points sign-in at a client of the user's own. A blank id clears it and returns to the
     * built-in client. The secret is optional: with one, the stricter code grant is used;
     * without, the same implicit grant as the built-in client.
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
        val id = clientId ?: throw noClientError()
        val responseType = if (clientSecret.isNullOrBlank()) "token" else "code"
        return "https://anilist.co/api/v2/oauth/authorize" +
            "?client_id=$id&redirect_uri=$redirectUri&response_type=$responseType"
    }

    /**
     * Runs the loopback listener until the browser comes back with a token, then stores it.
     * Blocks the calling coroutine, so callers dispatch it off the RPC read loop.
     */
    suspend fun awaitLogin(timeoutSeconds: Long = 300): String {
        if (!isConfigured) throw noClientError()

        val implicit = clientSecret.isNullOrBlank()
        val result = CompletableFuture<String>()

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

        // The implicit grant lands here with the token in the fragment, which the browser
        // keeps to itself — so this reply is a page that reads the fragment and calls back.
        server.createContext(CALLBACK_PATH) { exchange ->
            val params = parseQuery(exchange.requestURI.rawQuery)
            when {
                params["code"] != null -> {
                    result.complete(params.getValue("code"))
                    exchange.reply(page("Signed in", "You can close this tab and go back to Saikou."))
                }

                params["error"] != null -> {
                    result.completeExceptionally(IllegalStateException(params.getValue("error")))
                    exchange.reply(page("Sign-in failed", params.getValue("error")))
                }

                implicit -> exchange.reply(fragmentRelayPage())

                else -> {
                    result.completeExceptionally(IllegalStateException("no authorization code returned"))
                    exchange.reply(page("Sign-in failed", "AniList did not return an authorization code."))
                }
            }
        }

        server.createContext(TOKEN_PATH) { exchange ->
            val params = parseQuery(exchange.requestURI.rawQuery)
            val accessToken = params["access_token"]
            if (accessToken != null) {
                result.complete(accessToken)
                exchange.reply(page("Signed in", "You can close this tab and go back to Saikou."))
            } else {
                val error = params["error_description"] ?: params["error"] ?: "no token returned"
                result.completeExceptionally(IllegalStateException(error))
                exchange.reply(page("Sign-in failed", error))
            }
        }

        server.start()
        Log.i(TAG, "waiting for AniList redirect on $redirectUri (${if (implicit) "implicit" else "code"} grant)")

        val value = try {
            result.get(timeoutSeconds, TimeUnit.SECONDS)
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

        return if (implicit) {
            store(value)
            Log.i(TAG, "AniList sign-in complete")
            value
        } else {
            exchangeCode(value)
        }
    }

    private fun noClientError() = RpcException(
        ErrorCodes.NOT_AUTHENTICATED,
        "This build has no AniList client id compiled in, so one-click sign-in is unavailable. " +
            "Set SAIKOU_ANILIST_CLIENT_ID, or add your own client under Settings → Account.",
    )

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

    /**
     * The whole reason the implicit grant works here: this page runs in the browser, where
     * the fragment *is* readable, and immediately re-requests the same loopback server with
     * those values as an ordinary query string.
     */
    private fun fragmentRelayPage() = """
        <!doctype html>
        <html><head><meta charset="utf-8"><title>Signing in…</title>
        $STYLE
        <script>
          (function () {
            var hash = window.location.hash.replace(/^#/, "");
            window.location.replace("$TOKEN_PATH?" + (hash || "error=no_fragment_returned"));
          })();
        </script>
        </head>
        <body><div><h1>Signing in…</h1><p>Handing your AniList token back to Saikou.</p></div></body></html>
    """.trimIndent()

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
