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
import kotlinx.serialization.Serializable
import kotlinx.serialization.builtins.ListSerializer
import kotlinx.serialization.json.JsonArray
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonPrimitive

/**
 * AllAnime, straight against its public GraphQL API.
 *
 * This is the one source that needs no private backend: allanime.day answers unauthenticated
 * GraphQL over GET, which is what makes a fresh install able to play something at all. Every
 * other source in the registry proxies through the API described in [ApiBackend], and stays
 * unavailable until the user supplies its address and key.
 *
 * The endpoint sits behind Cloudflare. A normal residential connection passes without a
 * challenge; datacentre and VPN addresses frequently do not, and there is nothing this
 * parser can do about that beyond reporting it clearly.
 */
class AllAnime : AnimeParser() {

    override val name = "AllAnime"
    override val saveName = "AllAnime"
    override val hostUrl = SITE
    override val isDubAvailableSeparately = false

    private fun headers() = mapOf(
        "Referer" to "$REFERER/",
        "Origin" to REFERER,
        "Accept" to "application/json, text/plain, */*",
        "Accept-Language" to "en-US,en;q=0.9",
    )

    /**
     * The API takes the document and its variables as query parameters, not as a JSON body;
     * a POST is rejected.
     */
    private suspend fun graphql(document: String, variables: String): JsonObject {
        val url = "$API?variables=${encode(variables)}&query=${encode(document)}"
        val response = Http.get(url, headers())

        if (!response.isSuccess || response.text.trimStart().startsWith("<")) {
            throw IllegalStateException(
                if (response.text.contains("Just a moment", ignoreCase = true)) {
                    "AllAnime returned a Cloudflare challenge instead of data. That usually means " +
                        "the request came from a datacentre or VPN address."
                } else {
                    "AllAnime returned HTTP ${response.code}"
                }
            )
        }

        val parsed = Http.json.parseToJsonElement(response.text).jsonObject
        return parsed["data"]?.jsonObject
            ?: throw IllegalStateException("AllAnime returned no data for this request")
    }

    override suspend fun search(query: String): List<ShowResponse> {
        if (query.isBlank()) return emptyList()

        val variables = """
            {"search":{"allowAdult":false,"allowUnknown":false,"query":"${query.jsonEscaped()}"},
             "limit":40,"page":1,"translationType":"sub","countryOrigin":"ALL"}
        """.compact()

        val data = graphql(SEARCH, variables)
        val edges = data["shows"]?.jsonObject?.get("edges")?.let {
            Http.json.decodeFromJsonElement(ListSerializer(SearchEdge.serializer()), it)
        }.orEmpty()

        return edges.map { edge ->
            ShowResponse(
                name = edge.name,
                link = edge._id,
                coverUrl = FileUrl(edge.thumbnail.orEmpty(), mapOf("Referer" to "$REFERER/")),
                total = edge.availableEpisodes?.sub,
            )
        }
    }

    override suspend fun loadEpisodes(animeLink: String, extra: Map<String, String>?): List<Episode> {
        if (animeLink.isBlank()) return emptyList()

        val data = graphql(EPISODES, """{"showId":"${animeLink.jsonEscaped()}"}""")
        val detail = data["show"]?.jsonObject?.get("availableEpisodesDetail")?.jsonObject
            ?: return emptyList()

        // Dub is a separate track of the same show here, not a separate show, so the
        // preference decides which list to read and sub is the fallback when there is no dub.
        val preferred = if (selectDub) "dub" else "sub"
        val numbers = detail.stringList(preferred).ifEmpty { detail.stringList("sub") }
        val track = if (detail.stringList(preferred).isNotEmpty()) preferred else "sub"

        // The API hands them back newest-first as strings; episodes read oldest-first and
        // "10" must sort after "9".
        return numbers
            .sortedBy { it.toDoubleOrNull() ?: Double.MAX_VALUE }
            .map { number ->
                Episode(
                    number = number,
                    link = "$animeLink$LINK_SEPARATOR$track$LINK_SEPARATOR$number",
                    title = "Episode $number",
                )
            }
    }

    override suspend fun loadVideoServers(episodeLink: String, extra: Map<String, String>?): List<VideoServer> {
        val parts = episodeLink.split(LINK_SEPARATOR)
        if (parts.size != 3) return emptyList()
        val (showId, track, number) = parts

        val variables = """
            {"showId":"${showId.jsonEscaped()}","translationType":"$track","episodeString":"${number.jsonEscaped()}"}
        """.compact()

        val data = graphql(SOURCES, variables)
        val sources = data["episode"]?.jsonObject?.get("sourceUrls")?.let {
            Http.json.decodeFromJsonElement(ListSerializer(SourceUrl.serializer()), it)
        }.orEmpty()

        return sources
            .mapNotNull { source ->
                val url = deobfuscate(source.sourceUrl) ?: return@mapNotNull null
                val label = source.sourceName ?: "AllAnime"
                VideoServer(
                    name = "${track.uppercase()} - $label",
                    embed = FileUrl(url, mapOf("Referer" to "$REFERER/")),
                )
            }
            // Ranked the way ani-cli ranks them: the mp4 hosts are the ones that reliably
            // resolve, so they are tried first and everything unrecognised goes last.
            .sortedBy { server ->
                PREFERRED.indexOfFirst { server.name.contains(it) }.takeIf { it >= 0 } ?: PREFERRED.size
            }
    }

    override suspend fun getVideoExtractor(server: VideoServer): VideoExtractor = Extractor(server)

    /**
     * AllAnime hides its provider urls behind a hex string: strip the `--` marker, read the
     * remainder as byte pairs, and XOR each with 0x38.
     */
    private fun deobfuscate(raw: String): String? {
        val value = raw.trim()
        if (value.isEmpty()) return null
        if (!value.startsWith("--")) {
            return value.takeIf { it.startsWith("http") }
        }

        val hex = value.removePrefix("--")
        if (hex.length % 2 != 0) return null

        val decoded = buildString(hex.length / 2) {
            for (index in hex.indices step 2) {
                val byte = hex.substring(index, index + 2).toIntOrNull(16) ?: return null
                append(((byte xor 0x38) and 0xFF).toChar())
            }
        }

        return when {
            decoded.startsWith("http") -> decoded
            decoded.startsWith("/") -> SITE + decoded
            else -> null
        }
    }

    /**
     * Turns one provider url into playable files. The `clock` endpoints answer with JSON
     * when asked for `clock.json`; anything else is already a direct file.
     */
    private class Extractor(override val server: VideoServer) : VideoExtractor() {

        override suspend fun extract(): VideoContainer {
            val url = server.embed.url

            if (!url.contains("/clock")) {
                return VideoContainer(listOf(video(url, null)))
            }

            // The provider endpoint answers with JSON only under `clock.json`; the id and
            // everything after it stays untouched.
            val endpoint = url.replaceFirst("/clock", "/clock.json")
            val response = Http.get(endpoint, server.embed.headers)
            if (!response.isSuccess) {
                throw IllegalStateException("AllAnime provider returned HTTP ${response.code}")
            }

            val links = runCatching { response.parsed<ClockResponse>().links }.getOrNull().orEmpty()
            if (links.isEmpty()) {
                throw IllegalStateException("AllAnime provider returned no links")
            }

            return VideoContainer(
                links.mapNotNull { entry ->
                    val file = entry.link ?: entry.src ?: return@mapNotNull null
                    video(file, entry.resolutionStr)
                }.sortedByDescending { it.quality ?: 0 }
            )
        }

        private fun video(url: String, resolution: String?): Video {
            val quality = resolution?.filter { it.isDigit() }?.toIntOrNull()
            return Video(
                quality = quality,
                format = if (url.substringBefore('?').endsWith(".m3u8")) VideoType.M3U8 else VideoType.CONTAINER,
                file = FileUrl(url, server.embed.headers),
                extraNote = resolution,
            )
        }
    }

    // ------------------------------------------------------------------ wire types

    @Serializable
    private data class SearchEdge(
        val _id: String,
        val name: String = "Unknown",
        val thumbnail: String? = null,
        val availableEpisodes: AvailableEpisodes? = null,
    )

    @Serializable
    private data class AvailableEpisodes(val sub: Int? = null, val dub: Int? = null)

    @Serializable
    private data class SourceUrl(
        val sourceUrl: String,
        val sourceName: String? = null,
        val type: String? = null,
        val priority: Double? = null,
    )

    @Serializable
    private data class ClockResponse(val links: List<ClockLink> = emptyList())

    @Serializable
    private data class ClockLink(
        val link: String? = null,
        val src: String? = null,
        val resolutionStr: String? = null,
        val hls: Boolean? = null,
        val mp4: Boolean? = null,
    )

    private companion object {
        const val SITE = "https://allanime.day"
        const val API = "https://api.allanime.day/api"
        const val REFERER = "https://allanime.to"

        /** ASCII unit separator: it cannot occur in a show id or an episode string. */
        const val LINK_SEPARATOR = "\u001F"

        val PREFERRED = listOf("Luf-mp4", "S-mp4", "Sak", "Default", "Yt-mp4")

        val SEARCH = """
            query( ${'$'}search: SearchInput, ${'$'}limit: Int, ${'$'}page: Int,
                   ${'$'}translationType: VaildTranslationTypeEnumType,
                   ${'$'}countryOrigin: VaildCountryOriginEnumType ) {
                shows( search: ${'$'}search, limit: ${'$'}limit, page: ${'$'}page,
                       translationType: ${'$'}translationType, countryOrigin: ${'$'}countryOrigin ) {
                    edges { _id name thumbnail availableEpisodes }
                }
            }
        """.compact()

        val EPISODES = """
            query( ${'$'}showId: String! ) {
                show( _id: ${'$'}showId ) { _id availableEpisodesDetail }
            }
        """.compact()

        val SOURCES = """
            query( ${'$'}showId: String!, ${'$'}translationType: VaildTranslationTypeEnumType!,
                   ${'$'}episodeString: String! ) {
                episode( showId: ${'$'}showId, translationType: ${'$'}translationType,
                         episodeString: ${'$'}episodeString ) {
                    episodeString sourceUrls
                }
            }
        """.compact()

        fun String.compact(): String = trimIndent().replace(Regex("\\s+"), " ").trim()

        fun String.jsonEscaped(): String = replace("\\", "\\\\").replace("\"", "\\\"")

        fun JsonObject.stringList(key: String): List<String> =
            (this[key] as? JsonArray)
                ?.mapNotNull { runCatching { it.jsonPrimitive.content }.getOrNull() }
                .orEmpty()
    }
}
