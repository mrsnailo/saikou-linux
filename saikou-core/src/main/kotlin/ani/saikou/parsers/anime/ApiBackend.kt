package ani.saikou.parsers.anime

import ani.saikou.host.Preferences
import kotlinx.serialization.json.JsonPrimitive
import kotlinx.serialization.json.jsonPrimitive

/**
 * Every general anime source in the upstream Android app talks to one private backend,
 * whose base url and api key were injected at build time as `BuildConfig.SERVER_URL` and
 * `BuildConfig.MY_CUSTOM_API_KEY`. Those values are not in this repository.
 *
 * Rather than hard-code them, they are runtime settings. Supplying them switches every
 * API-backed source on at once; without them the sources report themselves unavailable
 * and the UI explains why instead of failing at click time.
 */
object ApiBackend {
    private const val HOST_KEY = "api.host"
    private const val API_KEY = "api.key"

    val host: String?
        get() = System.getenv("SAIKOU_API_HOST")?.takeIf { it.isNotBlank() }
            ?: Preferences.get(HOST_KEY)?.jsonPrimitive?.contentOrNullSafe()

    val key: String?
        get() = System.getenv("SAIKOU_API_KEY")?.takeIf { it.isNotBlank() }
            ?: Preferences.get(API_KEY)?.jsonPrimitive?.contentOrNullSafe()

    val isConfigured: Boolean get() = !host.isNullOrBlank() && !key.isNullOrBlank()

    fun configure(host: String, key: String) {
        Preferences.set(HOST_KEY, JsonPrimitive(host.trimEnd('/')))
        Preferences.set(API_KEY, JsonPrimitive(key))
    }

    fun clear() {
        Preferences.set(HOST_KEY, null)
        Preferences.set(API_KEY, null)
    }

    fun headers(): Map<String, String> = mapOf("x-api-key" to (key ?: ""))

    fun requireHost(): String = host?.trimEnd('/')
        ?: error("No anime backend is configured. Set api.host and api.key in settings.")
}

private fun kotlinx.serialization.json.JsonPrimitive.contentOrNullSafe(): String? =
    runCatching { content }.getOrNull()?.takeIf { it.isNotBlank() && it != "null" }
