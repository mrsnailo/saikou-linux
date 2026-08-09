package ani.saikou.parsers.anime.extractors

import ani.saikou.FileUrl
import ani.saikou.client

import ani.saikou.parsers.Video
import ani.saikou.parsers.VideoContainer
import ani.saikou.parsers.VideoExtractor
import ani.saikou.parsers.VideoServer
import ani.saikou.parsers.VideoType
import ani.saikou.tryWithSuspend
import kotlinx.serialization.InternalSerializationApi
import kotlinx.serialization.Serializable


@OptIn(InternalSerializationApi::class)
class InternalSMP4(override val server: VideoServer) : VideoExtractor() {


    @Serializable
    data class SourceItem(
        val url: String,
        val isM3u8: Boolean,
        val type: String,

        )


    @Serializable
    data class SourceData(
        val sources: List<SourceItem> = emptyList(),
    )

    @Serializable
    data class SourceResponse(
        val data: SourceData
    )

    override suspend fun extract(): VideoContainer {
        return tryWithSuspend(post = false, snackbar = false) {
            val response = client.get(server.embed.url, headers = mapOf("x-api-key" to apiKey))
                .parsed<SourceResponse>()

            val videos = response.data.sources.map {
                Video(
                    quality = null,
                    format = VideoType.CONTAINER,
                    file = FileUrl(it.url),
                    extraNote = it.type
                )
            }
            VideoContainer(videos)

        } ?: VideoContainer(emptyList())

    }
}