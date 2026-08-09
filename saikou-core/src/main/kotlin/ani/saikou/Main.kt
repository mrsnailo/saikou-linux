package ani.saikou

import ani.saikou.host.Log
import ani.saikou.host.Paths
import ani.saikou.host.Preferences
import ani.saikou.rpc.Registry
import ani.saikou.rpc.Server
import ani.saikou.rpc.registerCoreMethods
import java.nio.channels.FileChannel
import java.nio.file.Path
import java.nio.file.StandardOpenOption
import kotlin.system.exitProcess

private const val TAG = "Main"

/**
 * Headless scraping daemon. The Qt UI spawns and supervises this; it is not meant to be
 * launched by hand except for debugging.
 *
 *   saikou-core [--socket PATH] [--debug]
 */
fun main(args: Array<String>) {
    val options = parseArgs(args)
    if (options.debug) Log.minLevel = Log.Level.DEBUG

    Paths.ensureAll()
    Preferences.load()

    val socket = options.socket ?: Paths.socket

    // Single instance per socket. Without this, a second daemon would delete the live
    // socket during start() and silently steal the UI's connections.
    val lock = FileChannel.open(
        socket.resolveSibling("core.lock"),
        StandardOpenOption.CREATE, StandardOpenOption.WRITE
    ).tryLock()

    if (lock == null) {
        Log.e(TAG, "another saikou-core already owns $socket")
        exitProcess(2)
    }

    val registry = Registry().apply { registerCoreMethods() }
    val server = Server(socket, registry)

    Runtime.getRuntime().addShutdownHook(Thread {
        server.stop()
        runCatching { lock.release() }
    })

    Log.i(TAG, "saikou-core ${BuildInfo.VERSION} starting")
    server.start()
}

private class Options(val socket: Path?, val debug: Boolean)

private fun parseArgs(args: Array<String>): Options {
    var socket: Path? = null
    var debug = false
    var i = 0
    while (i < args.size) {
        when (val arg = args[i]) {
            "--socket" -> {
                socket = Path.of(args.getOrNull(++i) ?: fail("--socket needs a path"))
            }
            "--debug" -> debug = true
            "--help", "-h" -> {
                println("usage: saikou-core [--socket PATH] [--debug]")
                exitProcess(0)
            }
            else -> fail("unknown argument '$arg'")
        }
        i++
    }
    return Options(socket, debug)
}

private fun fail(message: String): Nothing {
    System.err.println("saikou-core: $message")
    exitProcess(1)
}
