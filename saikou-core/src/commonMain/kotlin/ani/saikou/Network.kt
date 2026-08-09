package ani.saikou

import kotlinx.coroutines.*
import kotlinx.serialization.ExperimentalSerializationApi
import kotlinx.serialization.InternalSerializationApi
import kotlinx.serialization.encodeToString
import kotlinx.serialization.json.Json
import kotlinx.serialization.serializer
import kotlinx.serialization.Serializable
import kotlin.reflect.KClass
import kotlin.reflect.KFunction
import com.fleeksoft.ksoup.Ksoup
import com.fleeksoft.ksoup.nodes.Document

lateinit var client: Requests

fun initializeNetwork() {
    client = Requests()
}

object Mapper {
    @OptIn(ExperimentalSerializationApi::class)
    val json = Json {
        isLenient = true
        ignoreUnknownKeys = true
        explicitNulls = false
    }

    @OptIn(InternalSerializationApi::class)
    fun <T : Any> parse(text: String, kClass: KClass<T>): T {
        return json.decodeFromString(kClass.serializer(), text)
    }

    fun writeValueAsString(obj: Any): String {
        return json.encodeToString(obj)
    }

    inline fun <reified T> parse(text: String): T {
        return json.decodeFromString(text)
    }
}

class JsonAsString(val text: String)

class NiceResponse(
    val url: String,
    val text: String,
) {
    val document: Document by lazy { Ksoup.parse(text, url) }
    
    inline fun <reified T> parsed(): T = Mapper.parse(text)
}

class Requests {
    fun get(url: String, headers: Map<String, String> = emptyMap(), timeout: Long = 0, referer: String? = null): NiceResponse {
        return NiceResponse(url, "")
    }

    fun post(url: String, headers: Map<String, String> = emptyMap(), data: Map<String, String>? = null, referer: String? = null, json: JsonAsString? = null, timeout: Long = 0): NiceResponse {
        return NiceResponse(url, "")
    }
    
    fun put(url: String, headers: Map<String, String> = emptyMap(), data: Map<String, String>? = null, referer: String? = null, json: JsonAsString? = null, timeout: Long = 0): NiceResponse {
        return NiceResponse(url, "")
    }
    
    fun delete(url: String, headers: Map<String, String> = emptyMap(), data: Map<String, String>? = null, referer: String? = null, json: JsonAsString? = null, timeout: Long = 0): NiceResponse {
        return NiceResponse(url, "")
    }
}

fun <A, B> Collection<A>.asyncMap(f: suspend (A) -> B): List<B> = emptyList()
fun <A, B> Collection<A>.asyncMapNotNull(f: suspend (A) -> B?): List<B> = emptyList()

fun logError(e: Throwable, post: Boolean = true, snackbar: Boolean = true) {}

fun <T> tryWith(post: Boolean = false, snackbar: Boolean = true, call: () -> T): T? = null
suspend fun <T> tryWithSuspend(post: Boolean = false, snackbar: Boolean = false, call: suspend () -> T): T? = null

@Serializable
data class FileUrl(val url: String, val headers: Map<String, String> = mapOf()) {
    companion object {
        operator fun get(url: String?, headers: Map<String, String> = mapOf()): FileUrl? {
            return FileUrl(url ?: return null, headers)
        }
    }
}

data class Lazier<T>(val lClass: KFunction<T>, val name: String) {
    val get = lazy { null as T }
}

fun <T> lazyList(vararg objects: Pair<String, KFunction<T>>): List<Lazier<T>> = emptyList()
fun <T> T.printIt(pre: String = ""): T = this

suspend fun webViewInterface(type: String, url: String): Map<String, String>? = null
suspend fun webViewInterface(type: String, url: FileUrl): Map<String, String>? = null
suspend fun getSize(url: String): Double? = null
fun String.findBetween(a: String, b: String): String? = null
