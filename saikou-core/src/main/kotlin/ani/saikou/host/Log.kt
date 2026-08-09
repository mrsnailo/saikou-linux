package ani.saikou.host

import java.time.LocalTime
import java.time.format.DateTimeFormatter

/**
 * Replaces `android.util.Log`. Writes to stderr so the UI can capture the daemon's
 * output without it being confused for RPC traffic on stdout.
 */
object Log {
    enum class Level { DEBUG, INFO, WARN, ERROR }

    var minLevel: Level = if (System.getenv("SAIKOU_DEBUG") != null) Level.DEBUG else Level.INFO

    private val clock = DateTimeFormatter.ofPattern("HH:mm:ss.SSS")

    fun d(tag: String, msg: String) = log(Level.DEBUG, tag, msg, null)
    fun i(tag: String, msg: String) = log(Level.INFO, tag, msg, null)
    fun w(tag: String, msg: String, e: Throwable? = null) = log(Level.WARN, tag, msg, e)
    fun e(tag: String, msg: String, e: Throwable? = null) = log(Level.ERROR, tag, msg, e)

    private fun log(level: Level, tag: String, msg: String, e: Throwable?) {
        if (level.ordinal < minLevel.ordinal) return
        synchronized(this) {
            System.err.println("${LocalTime.now().format(clock)} ${level.name.first()} $tag: $msg")
            e?.printStackTrace(System.err)
        }
    }
}
