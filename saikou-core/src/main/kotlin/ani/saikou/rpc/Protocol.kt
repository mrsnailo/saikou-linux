package ani.saikou.rpc

import kotlinx.serialization.SerialName
import kotlinx.serialization.Serializable
import kotlinx.serialization.json.Json
import kotlinx.serialization.json.JsonElement
import kotlinx.serialization.json.JsonNull

/**
 * JSON-RPC 2.0, line-delimited, over a Unix domain socket.
 *
 * Every method is async on the UI side: the UI writes a request and keeps servicing its
 * event loop until the matching id comes back. Long operations also emit notifications
 * (no id) so the UI can show progress instead of a spinner.
 */
object Protocol {
    const val VERSION = "2.0"

    val json = Json {
        ignoreUnknownKeys = true
        isLenient = true
        explicitNulls = false
        encodeDefaults = true
    }
}

@Serializable
data class Request(
    val jsonrpc: String = Protocol.VERSION,
    val id: JsonElement? = null,
    val method: String,
    val params: JsonElement? = null,
)

@Serializable
data class Response(
    val jsonrpc: String = Protocol.VERSION,
    val id: JsonElement? = null,
    val result: JsonElement? = null,
    val error: RpcErrorBody? = null,
)

@Serializable
data class Notification(
    val jsonrpc: String = Protocol.VERSION,
    val method: String,
    val params: JsonElement? = null,
)

@Serializable
data class RpcErrorBody(
    val code: Int,
    val message: String,
    val data: ErrorData? = null,
)

/**
 * `retryable` is load-bearing for UX: it is how the UI decides between a quiet inline
 * retry, a "try another source" prompt, and a hard error. `source` names the parser at
 * fault so the source-health view can flag it.
 */
@Serializable
data class ErrorData(
    val source: String? = null,
    val retryable: Boolean = false,
    @SerialName("stack") val stackTrace: String? = null,
)

/** Standard JSON-RPC codes plus Saikou's application range. */
object ErrorCodes {
    const val PARSE_ERROR = -32700
    const val INVALID_REQUEST = -32600
    const val METHOD_NOT_FOUND = -32601
    const val INVALID_PARAMS = -32602
    const val INTERNAL_ERROR = -32603

    const val NETWORK = -32000
    const val SOURCE_FAILED = -32001
    const val NOT_AUTHENTICATED = -32002
    const val CAPABILITY_MISSING = -32003
}

/** Thrown by handlers to produce a structured error instead of a bare stack trace. */
class RpcException(
    val code: Int,
    message: String,
    val source: String? = null,
    val retryable: Boolean = false,
    cause: Throwable? = null,
) : Exception(message, cause)

internal fun JsonElement?.orNull(): JsonElement? = if (this == null || this is JsonNull) null else this
