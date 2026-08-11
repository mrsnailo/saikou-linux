package ani.saikou.parsers.anime

import ani.saikou.net.Http
import ani.saikou.parsers.AnimeParser
import ani.saikou.parsers.Episode
import ani.saikou.parsers.FileUrl
import ani.saikou.parsers.ShowResponse
import ani.saikou.parsers.Video
import ani.saikou.parsers.VideoContainer
import ani.saikou.parsers.VideoExtractor
import ani.saikou.parsers.VideoServer
import ani.saikou.parsers.VideoType

/**
 * AnimeHeaven, scraped straight off the site.
 *
 * The one general source that currently works with no credentials and no browser engine:
 * plain HTML, no Cloudflare challenge, and episodes that resolve to direct MP4 files
 * rather than to an obfuscated embed.
 *
 * Its one oddity is how an episode is opened. The site does not link to an episode page;
 * every episode anchor runs `gatea("<key>")`, which writes a `key` cookie and then loads
 * `/gate.php`, and that page serves whichever episode the cookie names. So the "server"
 * this parser returns is that one url plus the cookie that selects the episode.
 */
class AnimeHeaven : AnimeParser() {

    override val name = "AnimeHeaven"
    override val saveName = "AnimeHeaven"
    override val hostUrl = SITE

    private fun headers(referer: String = "$SITE/") = mapOf(
        "Referer" to referer,
        "Accept" to "text/html,application/xhtml+xml",
        "Accept-Language" to "en-US,en;q=0.9",
    )

    override suspend fun search(query: String): List<ShowResponse> {
        if (query.isBlank()) return emptyList()

        val response = Http.get("$SITE/search.php?s=${encode(query)}", headers())
        if (!response.isSuccess) {
            throw IllegalStateException("AnimeHeaven returned HTTP ${response.code} for that search")
        }

        return response.document.select("div.similarimg div.p1").mapNotNull { block ->
            val anchor = block.selectFirst("a[href^=anime.php]") ?: return@mapNotNull null
            val id = anchor.attr("href").substringAfter('?').ifBlank { return@mapNotNull null }
            val cover = block.selectFirst("img.coverimg")

            val title = block.selectFirst("div.similarname a")?.text()?.trim()
                ?: cover?.attr("alt")?.trim().orEmpty()

            ShowResponse(
                name = title.ifBlank { id },
                link = id,
                coverUrl = FileUrl(
                    cover?.attr("src")?.let { if (it.startsWith("http")) it else "$SITE/$it" }.orEmpty(),
                    mapOf("Referer" to "$SITE/"),
                ),
            )
        }
    }

    override suspend fun loadEpisodes(animeLink: String, extra: Map<String, String>?): List<Episode> {
        if (animeLink.isBlank()) return emptyList()

        val page = "$SITE/anime.php?$animeLink"
        val response = Http.get(page, headers())
        if (!response.isSuccess) {
            throw IllegalStateException("AnimeHeaven returned HTTP ${response.code} for that title")
        }

        val episodes = response.document.select("a[onclick^=gatea]").mapNotNull { anchor ->
            // onclick='gatea("092e021097c57370b8aa764fd5920ecd")'
            val key = anchor.attr("onclick").substringAfter("(\"", "").substringBefore("\")", "")
            if (key.isBlank()) return@mapNotNull null

            val number = anchor.selectFirst("div.watch2")?.text()?.trim().orEmpty()
            Episode(
                number = number.ifBlank { "?" },
                link = key,
                title = if (number.isBlank()) "Episode" else "Episode $number",
            )
        }

        // The page lists newest first, and the numbers are strings, so "10" would land
        // between "1" and "2" without a numeric sort.
        return episodes.sortedBy { it.number.toDoubleOrNull() ?: Double.MAX_VALUE }
    }

    override suspend fun loadVideoServers(episodeLink: String, extra: Map<String, String>?): List<VideoServer> {
        if (episodeLink.isBlank()) return emptyList()
        return listOf(
            VideoServer(
                name = "AnimeHeaven",
                // The cookie is the whole request: it is what tells gate.php which episode
                // to serve, so it travels with the server rather than being set globally.
                embed = FileUrl("$SITE/$GATE", mapOf("Cookie" to "key=$episodeLink") + headers()),
            )
        )
    }

    override suspend fun getVideoExtractor(server: VideoServer): VideoExtractor = Extractor(server)

    private class Extractor(override val server: VideoServer) : VideoExtractor() {

        override suspend fun extract(): VideoContainer {
            val response = Http.get(server.embed.url, server.embed.headers)
            if (!response.isSuccess) {
                throw IllegalStateException("AnimeHeaven returned HTTP ${response.code} for that episode")
            }

            // Several <source> elements, one per CDN mirror, all of the same file. The
            // later ones carry `&error` markers the site's own player uses as fallbacks;
            // as distinct urls they are exactly the retry order we want.
            val videos = response.document.select("video source[src]")
                .map { it.attr("src") }
                .filter { it.startsWith("http") }
                .distinct()
                .map { url ->
                    Video(
                        format = if (url.substringBefore('?').endsWith(".m3u8")) {
                            VideoType.M3U8
                        } else {
                            VideoType.CONTAINER
                        },
                        file = FileUrl(url, mapOf("Referer" to "$SITE/")),
                        extraNote = url.substringAfter("//").substringBefore('.'),
                    )
                }

            if (videos.isEmpty()) {
                throw IllegalStateException(
                    "AnimeHeaven served the episode page without a video source. The episode " +
                        "may have been removed."
                )
            }

            return VideoContainer(videos)
        }
    }

    private companion object {
        const val SITE = "https://animeheaven.me"

        /** Serves whichever episode the `key` cookie names. */
        const val GATE = "gate.php"
    }
}
