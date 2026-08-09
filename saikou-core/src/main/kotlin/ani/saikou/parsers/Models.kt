package ani.saikou.parsers

import kotlinx.serialization.Serializable

/** A url that may need headers to be fetched (referer-locked CDNs). */
@Serializable
data class FileUrl(
    val url: String,
    val headers: Map<String, String> = emptyMap(),
) {
    companion object {
        operator fun get(url: String?, headers: Map<String, String> = emptyMap()): FileUrl? {
            if (url == null) return null
            return FileUrl(url, headers)
        }
    }
}

/** One show as returned by a source's search. */
@Serializable
data class ShowResponse(
    val name: String,
    val link: String,
    val coverUrl: FileUrl,
    val episodes: List<Episode>? = null,
    val otherNames: List<String> = emptyList(),
    val total: Int? = null,
    val extra: Map<String, String>? = null,
) {
    constructor(name: String, link: String, coverUrl: String) :
        this(name, link, FileUrl(coverUrl), null, emptyList(), null, null)
}

@Serializable
data class Episode(
    val number: String,
    val link: String,
    val title: String? = null,
    val thumbnail: FileUrl? = null,
    val description: String? = null,
    val isFiller: Boolean = false,
    val extra: Map<String, String>? = null,
)

/** A host serving an episode. `embed` is the page/endpoint an extractor consumes. */
@Serializable
data class VideoServer(
    val name: String,
    val embed: FileUrl,
    val extraData: Map<String, String>? = null,
) {
    constructor(name: String, embedUrl: String, extraData: Map<String, String>? = null) :
        this(name, FileUrl(embedUrl), extraData)
}

enum class VideoType { CONTAINER, M3U8, DASH }

enum class SubtitleType { VTT, ASS, SRT }

@Serializable
data class Video(
    val quality: Int? = null,
    val format: VideoType,
    val file: FileUrl,
    val size: Double? = null,
    val extraNote: String? = null,
)

@Serializable
data class Subtitle(
    val language: String,
    val file: FileUrl,
    val type: SubtitleType = SubtitleType.VTT,
    val isDefault: Boolean = false,
)

/** Video-only + audio-only sources that the player has to mux itself. */
@Serializable
data class AudioTrack(
    val url: String,
    val bitrate: String? = null,
    val language: String? = null,
    val note: String? = null,
    val headers: Map<String, String>? = null,
)

@Serializable
data class VideoContainer(
    val videos: List<Video>,
    val subtitles: List<Subtitle> = emptyList(),
    val audioTracks: List<AudioTrack> = emptyList(),
)
