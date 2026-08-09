package ani.saikou.rpc

import ani.saikou.BuildInfo
import ani.saikou.host.Preferences
import kotlinx.serialization.json.JsonElement
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.JsonPrimitive
import kotlinx.serialization.json.buildJsonArray
import kotlinx.serialization.json.buildJsonObject
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonPrimitive
import kotlinx.serialization.json.put

/**
 * Methods available from Phase 0. The `anime.*` namespace is declared here but returns an
 * empty source list until Phase 1 ports the first parser.
 */
fun Registry.registerCoreMethods() {
    register("core.ping") { _, _ -> JsonPrimitive("pong") }

    register("core.version") { _, _ ->
        buildJsonObject {
            put("core", BuildInfo.VERSION)
            put("protocol", Protocol.VERSION)
        }
    }

    register("core.methods") { _, _ ->
        buildJsonArray { methods().forEach { add(JsonPrimitive(it)) } }
    }

    /**
     * Capabilities the UI must branch on. `webengine` stays false until the optional
     * QtWebEngine package is installed; sources needing it are greyed out rather than
     * failing at click time.
     */
    register("core.capabilities") { _, _ ->
        buildJsonObject {
            put("webengine", false)
            put("anime", true)
            put("manga", false)
            put("novel", false)
        }
    }

    register("settings.getAll") { _, _ -> Preferences.all() }

    register("settings.get") { params, _ ->
        Preferences.get(params.requireString("key")) ?: JsonPrimitive(null as String?)
    }

    register("settings.set") { params, _ ->
        val obj = params as? JsonObject ?: throw RpcException(ErrorCodes.INVALID_PARAMS, "expected an object")
        Preferences.set(obj.requireString("key"), obj["value"])
        JsonPrimitive(true)
    }

    // Phase 1 replaces this with the real source registry.
    register("anime.sources") { _, _ -> buildJsonArray { } }
}

private fun JsonElement?.requireString(key: String): String {
    val obj = this as? JsonObject ?: throw RpcException(ErrorCodes.INVALID_PARAMS, "expected an object")
    return obj[key]?.jsonPrimitive?.content
        ?: throw RpcException(ErrorCodes.INVALID_PARAMS, "missing '$key'")
}

private fun JsonObject.requireString(key: String): String =
    this[key]?.jsonPrimitive?.content
        ?: throw RpcException(ErrorCodes.INVALID_PARAMS, "missing '$key'")

@Suppress("unused")
private fun JsonElement.asObject(): JsonObject = jsonObject
