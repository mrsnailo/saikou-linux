package ani.saikou.rpc

import ani.saikou.host.Log
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.NonCancellable
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.coroutineScope
import kotlinx.coroutines.job
import kotlinx.coroutines.joinAll
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import kotlinx.serialization.json.JsonElement
import kotlinx.serialization.json.JsonPrimitive
import java.io.BufferedReader
import java.io.BufferedWriter
import java.io.PrintWriter
import java.io.StringWriter
import java.net.StandardProtocolFamily
import java.net.UnixDomainSocketAddress
import java.nio.channels.Channels
import java.nio.channels.ServerSocketChannel
import java.nio.channels.SocketChannel
import java.nio.file.Files
import java.nio.file.Path

/**
 * One connected UI client. Handlers use [notify] to push progress without waiting for
 * the request to finish.
 */
class Session internal constructor(private val writer: BufferedWriter) {
    private val lock = Any()

    fun notify(method: String, params: JsonElement? = null) =
        write(Protocol.json.encodeToString(Notification.serializer(), Notification(method = method, params = params)))

    internal fun respond(response: Response) =
        write(Protocol.json.encodeToString(Response.serializer(), response))

    private fun write(line: String) {
        synchronized(lock) {
            runCatching {
                writer.write(line)
                writer.write("\n")
                writer.flush()
            }.onFailure { Log.d(TAG, "write to closed session: ${it.message}") }
        }
    }

    private companion object { const val TAG = "Session" }
}

/**
 * Accepts UI connections on a Unix socket. Each connection gets its own coroutine, and
 * each request within a connection is dispatched concurrently — a slow scrape must never
 * head-of-line block a `core.ping` or a cancel.
 */
class Server(
    private val socketPath: Path,
    private val registry: Registry,
) {
    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.IO)
    private var channel: ServerSocketChannel? = null

    fun start() {
        // A stale socket from a killed daemon would block bind(); the lock below makes
        // removing it safe.
        Files.deleteIfExists(socketPath)
        val server = ServerSocketChannel.open(StandardProtocolFamily.UNIX)
        server.bind(UnixDomainSocketAddress.of(socketPath))
        channel = server
        Log.i(TAG, "listening on $socketPath")

        while (true) {
            val client = try {
                server.accept()
            } catch (e: Exception) {
                if (channel == null) break else throw e
            }
            scope.launch { serve(client) }
        }
    }

    fun stop() {
        val server = channel ?: return
        channel = null
        runCatching { server.close() }
        runCatching { Files.deleteIfExists(socketPath) }
        scope.cancel()
        Log.i(TAG, "stopped")
    }

    private suspend fun serve(client: SocketChannel) = coroutineScope {
        val peer = client.hashCode()
        Log.d(TAG, "client $peer connected")
        val reader = BufferedReader(Channels.newReader(client, Charsets.UTF_8))
        val writer = BufferedWriter(Channels.newWriter(client, Charsets.UTF_8))
        val session = Session(writer)

        // Requests are dispatched into a connection-scoped supervisor rather than the
        // server scope: reaching EOF must not close the socket while replies are still
        // in flight, which is exactly what a client that half-closes after writing does.
        val requests = CoroutineScope(coroutineContext + SupervisorJob(coroutineContext[Job]))
        try {
            while (true) {
                val line = withContext(Dispatchers.IO) { reader.readLine() } ?: break
                if (line.isBlank()) continue
                requests.launch { dispatch(line, session) }
            }
        } catch (e: CancellationException) {
            throw e
        } catch (e: Exception) {
            Log.d(TAG, "client $peer read ended: ${e.message}")
        } finally {
            withContext(NonCancellable) {
                requests.coroutineContext.job.children.toList().joinAll()
            }
            runCatching { client.close() }
            Log.d(TAG, "client $peer disconnected")
        }
    }

    private suspend fun dispatch(line: String, session: Session) {
        val request = try {
            Protocol.json.decodeFromString(Request.serializer(), line)
        } catch (e: Exception) {
            session.respond(
                Response(id = null, error = RpcErrorBody(ErrorCodes.PARSE_ERROR, "malformed request: ${e.message}"))
            )
            return
        }

        val handler = registry[request.method]
        if (handler == null) {
            session.respond(
                Response(
                    id = request.id,
                    error = RpcErrorBody(ErrorCodes.METHOD_NOT_FOUND, "unknown method '${request.method}'")
                )
            )
            return
        }

        try {
            val result = handler.handle(request.params.orNull(), session)
            // A request without an id is a notification: the caller wants no reply.
            if (request.id.orNull() != null) {
                session.respond(Response(id = request.id, result = result ?: JsonPrimitive(true)))
            }
        } catch (e: CancellationException) {
            throw e
        } catch (e: RpcException) {
            session.respond(
                Response(
                    id = request.id,
                    error = RpcErrorBody(
                        code = e.code,
                        message = e.message ?: "request failed",
                        data = ErrorData(source = e.source, retryable = e.retryable)
                    )
                )
            )
        } catch (e: Throwable) {
            Log.e(TAG, "handler '${request.method}' threw", e)
            session.respond(
                Response(
                    id = request.id,
                    error = RpcErrorBody(
                        code = ErrorCodes.INTERNAL_ERROR,
                        message = e.message ?: e::class.simpleName ?: "internal error",
                        data = ErrorData(retryable = false, stackTrace = e.stackTraceString())
                    )
                )
            )
        }
    }

    private fun Throwable.stackTraceString(): String =
        StringWriter().also { sw -> PrintWriter(sw).use { printStackTrace(it) } }.toString()

    private companion object { const val TAG = "Server" }
}
