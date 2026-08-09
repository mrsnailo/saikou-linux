package ani.saikou.parsers.anime.extractors

import ani.saikou.net.Http
import ani.saikou.parsers.FileUrl
import ani.saikou.parsers.Subtitle
import ani.saikou.parsers.SubtitleType
import ani.saikou.parsers.Video
import ani.saikou.parsers.VideoContainer
import ani.saikou.parsers.VideoExtractor
import ani.saikou.parsers.VideoServer
import ani.saikou.parsers.VideoType
import ani.saikou.parsers.anime.ApiBackend
import kotlinx.serialization.SerialName
import kotlinx.serialization.Serializable

/**
 * Reads the backend's `/sources` endpoint. The CDN behind these streams is referer-
 * locked, so the headers travel with each url — mpv has to send them or every segment
 * request comes back 403.
 */
class MegaPlay(override val server: VideoServer) : VideoExtractor() {

    override suspend fun extract(): VideoContainer {
        val response = Http.get(server.embed.url, ApiBackend.headers()).parsed<SourceResponse>()

        val referer = response.headers.referer
        if (referer.isNullOrEmpty() || response.data.sources.isEmpty()) {
            return VideoContainer(emptyList())
        }

        val streamHeaders = mapOf(
            "User-Agent" to Http.USER_AGENT,
            "Accept" to "*/*",
            "Origin" to referer.removeSuffix("/"),
            "Referer" to referer,
        )

        val videos = response.data.sources.map {
            Video(
                quality = null,
                format = if (it.isM3u8) VideoType.M3U8 else VideoType.CONTAINER,
                file = FileUrl(it.url, streamHeaders),
                extraNote = it.type,
            )
        }

        val subtitles = response.data.subtitles
            .filter { it.lang != "thumbnails" }
            .map {
                Subtitle(
                    language = it.lang,
                    file = FileUrl(it.url, streamHeaders),
                    type = subtitleTypeOf(it.url),
                    isDefault = it.default,
                )
            }

        return VideoContainer(videos, subtitles)
    }

    private fun subtitleTypeOf(url: String): SubtitleType = when {
        url.endsWith(".ass", ignoreCase = true) -> SubtitleType.ASS
        url.endsWith(".srt", ignoreCase = true) -> SubtitleType.SRT
        else -> SubtitleType.VTT
    }

    @Serializable
    private data class SourceResponse(
        val headers: Headers = Headers(),
        val data: SourceData = SourceData(),
    )

    @Serializable
    private data class Headers(@SerialName("Referer") val referer: String? = null)

    @Serializable
    private data class SourceData(
        val sources: List<SourceItem> = emptyList(),
        val subtitles: List<SubtitleItem> = emptyList(),
    )

    @Serializable
    private data class SourceItem(
        val url: String,
        val isM3u8: Boolean = true,
        val type: String? = null,
    )

    @Serializable
    private data class SubtitleItem(
        val url: String,
        val lang: String = "Unknown",
        val default: Boolean = false,
    )
}
