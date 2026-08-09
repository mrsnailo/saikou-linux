package ani.saikou.host

import kotlinx.serialization.json.Json
import kotlinx.serialization.json.JsonElement
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.jsonObject
import java.nio.file.Files
import java.nio.file.StandardCopyOption
import kotlin.io.path.exists
import kotlin.io.path.readText

/**
 * Replaces `SharedPreferences` / `loadData` / `saveData`. A single JSON file under
 * $XDG_CONFIG_HOME/saikou. Writes are atomic so a crash mid-save cannot truncate it.
 *
 * Secrets (OAuth tokens) do NOT belong here — those go to the Secret Service via the UI.
 */
object Preferences {
    private const val TAG = "Preferences"
    private val file = Paths.config.resolve("settings.json")
    private val json = Json { prettyPrint = true; ignoreUnknownKeys = true; isLenient = true }

    private var values: MutableMap<String, JsonElement> = mutableMapOf()

    @Synchronized
    fun load() {
        values = if (file.exists()) {
            runCatching { json.parseToJsonElement(file.readText()).jsonObject.toMutableMap() }
                .onFailure { Log.w(TAG, "settings.json unreadable, starting fresh", it) }
                .getOrDefault(mutableMapOf())
        } else mutableMapOf()
    }

    @Synchronized
    fun get(key: String): JsonElement? = values[key]

    @Synchronized
    fun set(key: String, value: JsonElement?) {
        if (value == null) values.remove(key) else values[key] = value
        persist()
    }

    @Synchronized
    fun all(): JsonObject = JsonObject(values.toMap())

    private fun persist() {
        Paths.config.let { Files.createDirectories(it) }
        val tmp = Files.createTempFile(Paths.config, "settings", ".tmp")
        Files.writeString(tmp, json.encodeToString(JsonObject.serializer(), JsonObject(values.toMap())))
        Files.move(tmp, file, StandardCopyOption.REPLACE_EXISTING, StandardCopyOption.ATOMIC_MOVE)
    }
}
