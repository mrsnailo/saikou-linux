package ani.saikou.parsers.anime

import ani.saikou.net.Http
import ani.saikou.parsers.AnimeParser
import ani.saikou.parsers.Episode
import ani.saikou.parsers.FileUrl
import ani.saikou.parsers.ShowResponse
import ani.saikou.parsers.Subtitle
import ani.saikou.parsers.SubtitleType
import ani.saikou.parsers.Video
import ani.saikou.parsers.VideoContainer
import ani.saikou.parsers.VideoExtractor
import ani.saikou.parsers.VideoServer
import ani.saikou.parsers.VideoType
import kotlinx.serialization.Serializable
import java.net.URLEncoder

/**
 * AniBD, through the same public JSON API its own site calls.
 *
 * The Android app reached this source by asking the maintainer's private backend for
 * `/api/anibd/…`, which is why it looked like it needed one. It does not: `anibd.app` is a
 * thin front end over three open endpoints on `animeapps.top`, none of which want a key.
 *
 * The useful part is that episodes are keyed by **AniList id**, not by a slug of its own.
 * Every other source here has to guess which of its titles matches the one being tracked;
 * this one is handed the answer, so [search] returns the AniList id as the link and
 * [loadEpisodes] looks it up directly.
 *
 * Streams are Blu-ray rips — 1080p where the release has it — and the playlist host serves
 * a redirect to an unrelated site unless the request carries a referer.
 */
class AniBD : AnimeParser() {

    override val name = "AniBD"
    override val saveName = "AniBD"
    override val hostUrl = SITE

    /** Every link this source hands out is an AniList id; see the class comment. */
    override val anilistKeyed = true

    private fun headers(referer: String = "$SITE/") = mapOf(
        "Referer" to referer,
        "Accept" to "application/json, text/html;q=0.9",
        "Accept-Language" to "en-US,en;q=0.9",
    )

    override suspend fun search(query: String): List<ShowResponse> {
        if (query.isBlank()) return emptyList()

        val encoded = URLEncoder.encode(query, "utf-8")
        val response = Http.get("$API/search3.php?keyword=$encoded", headers())
        if (!response.isSuccess) {
            throw IllegalStateException("AniBD returned HTTP ${response.code} for that search")
        }

        return response.parsed<SearchResponse>().data
            // An entry without an AniList id has no episodes to look up, so it would only
            // ever be a dead end in the picker.
            .filter { !it.anilist.isNullOrBlank() }
            .map { show ->
                ShowResponse(
                    name = show.postname,
                    link = show.anilist!!,
                    coverUrl = FileUrl(
                        show.ani_cover_large ?: show.ani_cover_medium.orEmpty(),
                        mapOf("Referer" to "$SITE/"),
                    ),
                    otherNames = listOfNotNull(
                        show.postyear?.takeIf { it.isNotBlank() }?.let { "($it)" },
                    ),
                )
            }
    }

    override suspend fun loadEpisodes(animeLink: String, extra: Map<String, String>?): List<Episode> {
        if (animeLink.isBlank()) return emptyList()

        val response = Http.get("$EPISODES/api2.php?epid=$animeLink", headers())
        if (!response.isSuccess) {
            throw IllegalStateException("AniBD returned HTTP ${response.code} for that title")
        }

        val servers = response.parsed<List<EpisodeServer>>()
        // The site shows one tab per server and they carry the same episodes; the subbed
        // tab is the one to list, falling back to whatever single tab exists.
        val chosen = servers.firstOrNull { it.server_name.contains("sub", ignoreCase = true) }
            ?: servers.firstOrNull()
            ?: return emptyList()

        return chosen.server_data.map { episode ->
            Episode(
                // "01", "02"… — trimmed so the UI sorts and labels them as numbers.
                number = episode.name.trimStart('0').ifBlank { episode.name },
                link = episode.link,
            )
        }
    }

    override suspend fun loadVideoServers(episodeLink: String, extra: Map<String, String>?): List<VideoServer> {
        if (episodeLink.isBlank()) return emptyList()

        val response = Http.get("$EPISODES/apilink.php?data=$episodeLink", headers())
        if (!response.isSuccess) {
            throw IllegalStateException("AniBD returned HTTP ${response.code} for that episode")
        }

        return response.parsed<List<SourceLink>>().map { source ->
            VideoServer(name = source.server, embed = FileUrl(source.link, headers()))
        }
    }

    override suspend fun getVideoExtractor(server: VideoServer): VideoExtractor = Extractor(server)

    /**
     * The server url is an ArtPlayer page that declares its own config inline, so the
     * playlist is in the HTML. Its `videoUrl` is site-relative and does not always match
     * the id in the server url, which is why it is read rather than constructed.
     */
    private class Extractor(override val server: VideoServer) : VideoExtractor() {

        override suspend fun extract(): VideoContainer {
            val response = Http.get(server.embed.url, server.embed.headers)
            if (!response.isSuccess) {
                throw IllegalStateException("AniBD returned HTTP ${response.code} for that server")
            }

            val page = response.text
            val path = VIDEO_URL.find(page)?.groupValues?.get(1)
                ?: throw IllegalStateException(
                    "AniBD served the player without a stream. The episode may not be uploaded yet."
                )

            // Without a referer the playlist host answers 301 to an unrelated site rather
            // than 403, so a missing header looks like a working request that plays nothing.
            val referer = mapOf("Referer" to "$PLAYER/")

            val videos = listOf(
                Video(
                    format = VideoType.M3U8,
                    file = FileUrl(absolute(path), referer),
                    extraNote = server.name,
                )
            )

            val subtitles = TRACK.findAll(page).map { match ->
                val url = match.groupValues[1]
                val label = match.groupValues[2].ifBlank { "Subtitle" }
                Subtitle(
                    language = label,
                    file = FileUrl(absolute(url), referer),
                    type = if (url.endsWith(".ass")) SubtitleType.ASS else SubtitleType.VTT,
                    isDefault = label.startsWith("English", ignoreCase = true),
                )
            }.distinctBy { it.file.url }.toList()

            return VideoContainer(videos, subtitles)
        }

        private fun absolute(url: String) =
            if (url.startsWith("http")) url else "$PLAYER${if (url.startsWith("/")) "" else "/"}$url"

        private companion object {
            const val PLAYER = "https://playeng.animeapps.top"
            val VIDEO_URL = Regex("""videoUrl\s*:\s*"([^"]+)"""")

            /** ArtPlayer's `tracks` entries, whichever order the two keys appear in. */
            val TRACK = Regex(
                """\{[^{}]*?url\s*:\s*"([^"]+)"[^{}]*?(?:html|label|name)\s*:\s*"([^"]*)"[^{}]*?\}"""
            )
        }
    }

    @Serializable
    private data class SearchResponse(val data: List<SearchItem> = emptyList())

    @Serializable
    private data class SearchItem(
        val postname: String,
        val anilist: String? = null,
        val postyear: String? = null,
        val ani_cover_large: String? = null,
        val ani_cover_medium: String? = null,
    )

    @Serializable
    private data class EpisodeServer(
        val server_name: String = "",
        val server_data: List<EpisodeEntry> = emptyList(),
    )

    @Serializable
    private data class EpisodeEntry(val name: String, val slug: String = "", val link: String)

    @Serializable
    private data class SourceLink(val server: String, val link: String)

    private companion object {
        const val SITE = "https://anibd.app"
        const val API = "https://eng.animeapps.top/api"
        const val EPISODES = "https://epeng.animeapps.top"
    }
}
