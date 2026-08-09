package ani.saikou.parsers

import java.net.URLDecoder
import java.net.URLEncoder

/** Base of every source. Ported from the Android `BaseParser` minus the Android APIs. */
abstract class BaseParser {
    abstract val name: String
    abstract val saveName: String
    abstract val hostUrl: String

    open val isNSFW = false
    open val language = "English"

    /**
     * Whether this source needs a browser engine (Cloudflare, JS challenges). The UI
     * greys these out unless the optional QtWebEngine package is present, rather than
     * letting the user click into a guaranteed failure.
     */
    open val needsWebEngine = false

    abstract suspend fun search(query: String): List<ShowResponse>

    fun encode(input: String): String = URLEncoder.encode(input, "utf-8").replace("+", "%20")
    fun decode(input: String): String = URLDecoder.decode(input, "utf-8")
}

abstract class AnimeParser : BaseParser() {
    abstract suspend fun loadEpisodes(animeLink: String, extra: Map<String, String>?): List<Episode>

    abstract suspend fun loadVideoServers(episodeLink: String, extra: Map<String, String>?): List<VideoServer>

    /** Dub and sub as separate shows on the site, rather than two tracks of one show. */
    open val isDubAvailableSeparately = false

    open var selectDub = false

    abstract suspend fun getVideoExtractor(server: VideoServer): VideoExtractor?
}

/** Turns one server's embed into playable urls. */
abstract class VideoExtractor {
    abstract val server: VideoServer

    abstract suspend fun extract(): VideoContainer
}
