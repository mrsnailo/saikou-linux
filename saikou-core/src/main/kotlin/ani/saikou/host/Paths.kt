package ani.saikou.host

import java.nio.file.Files
import java.nio.file.Path
import kotlin.io.path.createDirectories

/** XDG base directory locations. Replaces Android's `Context` file APIs. */
object Paths {
    private val home: Path = Path.of(System.getProperty("user.home"))

    private fun env(name: String, fallback: Path): Path =
        System.getenv(name)?.takeIf { it.isNotBlank() }?.let { Path.of(it) } ?: fallback

    val config: Path = env("XDG_CONFIG_HOME", home.resolve(".config")).resolve("saikou")
    val data: Path = env("XDG_DATA_HOME", home.resolve(".local/share")).resolve("saikou")
    val cache: Path = env("XDG_CACHE_HOME", home.resolve(".cache")).resolve("saikou")

    /**
     * Runtime dir for the control socket. Falls back to a private dir under temp dir when
     * XDG_RUNTIME_DIR is absent (ssh sessions, containers, Windows).
     */
    val runtime: Path = run {
        val xdg = System.getenv("XDG_RUNTIME_DIR")
        if (xdg.isNullOrBlank()) {
            val tmp = System.getProperty("java.io.tmpdir")
            val user = System.getProperty("user.name", "0")
            Path.of(tmp, "saikou-$user")
        } else {
            Path.of(xdg, "saikou")
        }
    }

    val socket: Path = runtime.resolve("core.sock")

    fun ensureAll() {
        listOf(config, data, cache).forEach { it.createDirectories() }
        runtime.createDirectories()
        // The socket carries no auth of its own; the directory mode is the access control.
        if (runtime.fileSystem.supportedFileAttributeViews().contains("posix")) {
            runCatching {
                Files.setPosixFilePermissions(
                    runtime,
                    java.nio.file.attribute.PosixFilePermissions.fromString("rwx------")
                )
            }
        }
    }
}
