package ani.saikou.net

import ani.saikou.host.Paths
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.suspendCancellableCoroutine
import kotlinx.coroutines.withContext
import kotlinx.serialization.json.Json
import okhttp3.Cache
import okhttp3.Call
import okhttp3.Callback
import okhttp3.FormBody
import okhttp3.Headers.Companion.toHeaders
import okhttp3.MediaType.Companion.toMediaType
import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.RequestBody.Companion.toRequestBody
import okhttp3.Response
import org.jsoup.Jsoup
import org.jsoup.nodes.Document
import java.io.IOException
import java.util.concurrent.TimeUnit
import kotlin.coroutines.resume
import kotlin.coroutines.resumeWithException

/**
 * The replacement for NiceHttp. Keeps the call surface the parsers already use
 * (`client.get(url).document`, `.parsed<T>()`, `.text`) so porting a parser is a matter
 * of deleting Android imports, not rewriting its request code.
 */
object Http {
    /**
     * A desktop UA. The Android app claimed to be Chrome on Android; several sites vary
     * their markup by platform, so claiming the platform we actually are keeps the HTML
     * matching what a developer sees in their own browser.
     */
    const val USER_AGENT =
        "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36"

    val defaultHeaders = mapOf("User-Agent" to USER_AGENT)

    val json = Json {
        isLenient = true
        ignoreUnknownKeys = true
        explicitNulls = false
        coerceInputValues = true
    }

    private val jsonMediaType = "application/json; charset=utf-8".toMediaType()

    val client: OkHttpClient by lazy {
        OkHttpClient.Builder()
            .followRedirects(true)
            .followSslRedirects(true)
            .cache(Cache(Paths.cache.resolve("http").toFile(), 20L * 1024 * 1024))
            .connectTimeout(15, TimeUnit.SECONDS)
            .readTimeout(30, TimeUnit.SECONDS)
            .callTimeout(60, TimeUnit.SECONDS)
            .retryOnConnectionFailure(true)
            .build()
    }

    suspend fun get(
        url: String,
        headers: Map<String, String> = emptyMap(),
        timeoutSeconds: Long? = null,
    ): HttpResponse = execute(
        Request.Builder().url(url).get().headers((defaultHeaders + headers).toHeaders()).build(),
        timeoutSeconds
    )

    suspend fun head(
        url: String,
        headers: Map<String, String> = emptyMap(),
    ): HttpResponse = execute(
        Request.Builder().url(url).head().headers((defaultHeaders + headers).toHeaders()).build(),
        null
    )

    /** Form-encoded POST. */
    suspend fun post(
        url: String,
        headers: Map<String, String> = emptyMap(),
        data: Map<String, String> = emptyMap(),
        timeoutSeconds: Long? = null,
    ): HttpResponse {
        val body = FormBody.Builder().apply { data.forEach { (k, v) -> add(k, v) } }.build()
        return execute(
            Request.Builder().url(url).post(body).headers((defaultHeaders + headers).toHeaders()).build(),
            timeoutSeconds
        )
    }

    /** JSON-body POST, used by the GraphQL endpoints. */
    suspend fun postJson(
        url: String,
        headers: Map<String, String> = emptyMap(),
        body: String,
        timeoutSeconds: Long? = null,
    ): HttpResponse = execute(
        Request.Builder()
            .url(url)
            .post(body.toRequestBody(jsonMediaType))
            .headers((defaultHeaders + headers).toHeaders())
            .build(),
        timeoutSeconds
    )

    private suspend fun execute(request: Request, timeoutSeconds: Long?): HttpResponse =
        withContext(Dispatchers.IO) {
            val call = if (timeoutSeconds == null) {
                client.newCall(request)
            } else {
                client.newBuilder()
                    .callTimeout(timeoutSeconds, TimeUnit.SECONDS)
                    .build()
                    .newCall(request)
            }

            suspendCancellableCoroutine { continuation ->
                continuation.invokeOnCancellation { call.cancel() }
                call.enqueue(object : Callback {
                    override fun onFailure(call: Call, e: IOException) =
                        continuation.resumeWithException(e)

                    override fun onResponse(call: Call, response: Response) {
                        // Read the body here: it must be consumed on this thread, and
                        // callers treat the response as a plain value afterwards.
                        val text = response.body?.string().orEmpty()
                        val result = HttpResponse(
                            code = response.code,
                            text = text,
                            url = response.request.url.toString(),
                            headers = response.headers.toMultimap(),
                        )
                        response.close()
                        continuation.resume(result)
                    }
                })
            }
        }
}

/** A fully-read response. Deliberately a value type — nothing here holds a socket open. */
class HttpResponse(
    val code: Int,
    val text: String,
    val url: String,
    val headers: Map<String, List<String>>,
) {
    val isSuccess: Boolean get() = code in 200..299

    val document: Document by lazy { Jsoup.parse(text, url) }

    inline fun <reified T> parsed(): T = Http.json.decodeFromString(text)

    fun cookies(): List<String> = headers["set-cookie"].orEmpty()
}
