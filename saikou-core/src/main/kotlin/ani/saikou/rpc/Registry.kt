package ani.saikou.rpc

import kotlinx.serialization.json.JsonElement

/** A single RPC method. Receives the raw `params` and the calling session. */
fun interface Handler {
    suspend fun handle(params: JsonElement?, session: Session): JsonElement?
}

/**
 * Method table. Namespaced by dot: `core.*`, `anime.*`, `anilist.*`.
 * Manga and novel namespaces are intentionally absent for the MVP.
 */
class Registry {
    private val handlers = LinkedHashMap<String, Handler>()

    fun register(method: String, handler: Handler) {
        require(handlers.put(method, handler) == null) { "duplicate RPC method: $method" }
    }

    operator fun get(method: String): Handler? = handlers[method]

    fun methods(): List<String> = handlers.keys.toList()
}
