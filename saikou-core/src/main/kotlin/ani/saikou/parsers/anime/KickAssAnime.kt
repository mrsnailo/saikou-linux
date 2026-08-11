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

/**
 * KickAssAnime, through the JSON API its own web player uses.
 *
 * No credentials, no browser engine, and episodes resolve to a plain HLS master playlist
 * with real subtitle tracks — which makes it the second general source alongside
 * [AnimeHeaven], and the better of the two for anything but raw availability.
 *
 * The awkward part is the address. `kaa.to` is only a signpost: every request 301s to
 * whichever mirror is live that week (`kickass-anime.ru`, `.ro`, `kaa.lt`, …). A GET can
 * ride the redirect, but OkHttp downgrades a redirected POST to a GET, so search would
 * silently break. [origin] therefore resolves the live mirror once with a cheap GET and
 * every later request is aimed straight at it.
 */
class KickAssAnime : AnimeParser() {

    override val name = "KickAssAnime"
    override val saveName = "KickAssAnime"
    override val hostUrl = SIGNPOST

    private fun headers(referer: String = "$SIGNPOST/") = mapOf(
        "Referer" to referer,
        "Accept" to "application/json, text/html;q=0.9",
        "Accept-Language" to "en-US,en;q=0.9",
    )

    override suspend fun search(query: String): List<ShowResponse> {
        if (query.isBlank()) return emptyList()

        val base = origin()
        val response = Http.postJson(
            "$base/api/search",
            headers = headers() + ("Content-Type" to "application/json"),
            body = Http.json.encodeToString(SearchRequest.serializer(), SearchRequest(query)),
        )
        if (!response.isSuccess) {
            throw IllegalStateException("KickAssAnime returned HTTP ${response.code} for that search")
        }

        return response.parsed<List<SearchResult>>().map { show ->
            ShowResponse(
                name = show.title_en?.takeIf { it.isNotBlank() } ?: show.title,
                link = show.slug,
                coverUrl = FileUrl(
                    show.poster?.hq?.let { "$base/image/poster/$it.webp" }.orEmpty(),
                    mapOf("Referer" to "$base/"),
                ),
                otherNames = listOfNotNull(show.title.takeIf { it != show.title_en }),
            )
        }
    }

    override suspend fun loadEpisodes(animeLink: String, extra: Map<String, String>?): List<Episode> {
        if (animeLink.isBlank()) return emptyList()

        val base = origin()
        // Japanese audio is the subbed release; a show that only has a dub falls back to
        // whatever single locale it does carry.
        val locales = runCatching {
            Http.get("$base/api/show/$animeLink/language", headers()).parsed<Languages>().result
        }.getOrDefault(emptyList())
        val language = locales.firstOrNull { it == SUBBED } ?: locales.firstOrNull() ?: SUBBED

        val first = episodePage(base, animeLink, language, "1")
        val episodes = first.result.toMutableList()

        // The API answers with one page of episodes plus a table of contents; a page is
        // requested by naming any episode inside it, so the first episode of each page is
        // the cursor. Page one is already in hand.
        for (page in first.pages.drop(1)) {
            val cursor = page.eps.firstOrNull()?.toString() ?: page.from ?: continue
            episodes += runCatching { episodePage(base, animeLink, language, cursor).result }
                .getOrDefault(emptyList())
        }

        return episodes
            .distinctBy { it.slug }
            .sortedBy { it.episode_number ?: Double.MAX_VALUE }
            .map { episode ->
                Episode(
                    number = episode.episode_string ?: episode.episode_number?.clean() ?: "?",
                    // Everything the episode endpoint needs, and nothing the RPC layer has
                    // to carry separately: `extra` is always null by the time it gets back.
                    link = "$animeLink$LINK_SEPARATOR${episode.slug}$LINK_SEPARATOR${episode.episode_string.orEmpty()}",
                    title = episode.title,
                    thumbnail = episode.thumbnail?.hq?.let {
                        FileUrl("$base/image/thumbnail/$it.webp", mapOf("Referer" to "$base/"))
                    },
                )
            }
    }

    override suspend fun loadVideoServers(episodeLink: String, extra: Map<String, String>?): List<VideoServer> {
        val parts = episodeLink.split(LINK_SEPARATOR)
        if (parts.size < 3) return emptyList()
        val (show, slug, number) = parts

        val base = origin()
        val response = Http.get("$base/api/show/$show/episode/ep-$number-$slug", headers())
        if (!response.isSuccess) {
            throw IllegalStateException("KickAssAnime returned HTTP ${response.code} for that episode")
        }

        return response.parsed<EpisodeDetail>().servers.map { server ->
            VideoServer(
                name = server.name,
                embed = FileUrl(server.src, mapOf("Referer" to "$base/")),
            )
        }
    }

    override suspend fun getVideoExtractor(server: VideoServer): VideoExtractor = Extractor(server)

    /**
     * The servers are all the same player page, which embeds its own configuration as
     * escaped JSON rather than fetching it. So the streams are already in the HTML and no
     * second request or signature dance is needed.
     */
    private class Extractor(override val server: VideoServer) : VideoExtractor() {

        override suspend fun extract(): VideoContainer {
            val response = Http.get(server.embed.url, server.embed.headers)
            if (!response.isSuccess) {
                throw IllegalStateException("KickAssAnime returned HTTP ${response.code} for that server")
            }

            val config = response.text.replace("&quot;", "\"").replace("&amp;", "&")
            // The segments live on a separate CDN that answers 403 to a referer alone; it
            // wants the cross-origin pair the browser would have sent, Origin included.
            val referer = mapOf("Referer" to "$PLAYER/", "Origin" to PLAYER)

            val videos = STREAM.findAll(config)
                .map { normalize(it.value) }
                .distinct()
                .map { url ->
                    Video(
                        format = if (url.endsWith(".mpd")) VideoType.DASH else VideoType.M3U8,
                        file = FileUrl(url, referer),
                        extraNote = server.name,
                    )
                }
                // HLS first: the DASH mirror is video and audio in separate streams, so it
                // is the fallback rather than the pick.
                .sortedBy { it.format == VideoType.DASH }
                .toList()

            if (videos.isEmpty()) {
                throw IllegalStateException(
                    "KickAssAnime served the player without a stream. The episode may have been removed."
                )
            }

            val subtitles = SUBTITLE.findAll(config).map { match ->
                Subtitle(
                    language = match.groupValues[1],
                    file = FileUrl(normalize(match.groupValues[2]), referer),
                    type = SubtitleType.VTT,
                    isDefault = match.groupValues[1].startsWith("English"),
                )
            }.distinctBy { it.file.url }.toList()

            return VideoContainer(videos, subtitles)
        }

        /** The player writes some urls as `https:////host/…`; browsers shrug, OkHttp does not. */
        private fun normalize(url: String) = url.replace(Regex("^https:/+"), "https://")

        private companion object {
            const val PLAYER = "https://krussdomi.com"
            val STREAM = Regex("""https:/{2,}[^"'\s\\]+?\.(?:m3u8|mpd)""")
            val SUBTITLE = Regex(""""name":\[0,"([^"]+)"],"src":\[0,"(https:/{2,}[^"]+?\.vtt)"]""")
        }
    }

    private suspend fun episodePage(base: String, show: String, language: String, cursor: String): EpisodePage {
        val response = Http.get("$base/api/show/$show/episodes?ep=$cursor&lang=$language", headers())
        if (!response.isSuccess) {
            throw IllegalStateException("KickAssAnime returned HTTP ${response.code} for that title")
        }
        return response.parsed()
    }

    /**
     * The live mirror, resolved once per run. Held for the process rather than looked up
     * per call: the redirect costs a round trip, and the mirror only rotates between weeks.
     */
    private suspend fun origin(): String {
        resolved?.let { return it }
        val response = Http.get("$SIGNPOST/api/show/recent?type=all&page=1", headers())
        val base = runCatching {
            java.net.URI(response.url).let { "${it.scheme}://${it.host}" }
        }.getOrNull()?.takeIf { it.startsWith("http") } ?: SIGNPOST
        resolved = base
        return base
    }

    @Serializable
    private data class SearchRequest(val query: String)

    @Serializable
    private data class SearchResult(
        val slug: String,
        val title: String,
        val title_en: String? = null,
        val poster: Image? = null,
    )

    @Serializable
    private data class Image(val hq: String? = null, val sm: String? = null)

    @Serializable
    private data class Languages(val result: List<String> = emptyList())

    @Serializable
    private data class EpisodePage(
        val result: List<EpisodeEntry> = emptyList(),
        val pages: List<PageRef> = emptyList(),
    )

    @Serializable
    private data class PageRef(
        val number: Int? = null,
        val from: String? = null,
        val eps: List<Double> = emptyList(),
    )

    @Serializable
    private data class EpisodeEntry(
        val slug: String,
        val title: String? = null,
        val episode_number: Double? = null,
        val episode_string: String? = null,
        val thumbnail: Image? = null,
    )

    @Serializable
    private data class EpisodeDetail(val servers: List<Server> = emptyList())

    @Serializable
    private data class Server(val name: String, val src: String)

    private companion object {
        /** Only ever redirects; see the class comment. */
        const val SIGNPOST = "https://kaa.to"

        const val SUBBED = "ja-JP"

        /** Not a url character, so it cannot appear in a slug or an episode number. */
        const val LINK_SEPARATOR = "\u001F"

        @Volatile
        var resolved: String? = null

        fun Double.clean(): String =
            if (this % 1.0 == 0.0) toInt().toString() else toString()
    }
}
