package ani.saikou.parsers.anime

import ani.saikou.host.Log
import ani.saikou.net.Http
import ani.saikou.parsers.AnimeParser
import ani.saikou.parsers.Episode
import ani.saikou.parsers.FileUrl
import ani.saikou.parsers.ShowResponse
import ani.saikou.parsers.VideoExtractor
import ani.saikou.parsers.VideoServer
import ani.saikou.parsers.anime.extractors.MegaPlay
import kotlinx.serialization.Serializable

/**
 * Shared implementation for the sources that proxy through the backend API. They differ
 * only in a path segment and in which of three response shapes the backend returns, so
 * one class covers all of them.
 */
abstract class ApiParser : AnimeParser() {

    /** Path segment identifying this provider to the backend, e.g. "anikoto". */
    abstract val providerName: String

    /** How this provider's endpoints are laid out. */
    abstract val shape: Shape

    enum class Shape {
        /** `/anime/{id}` → providerEpisodes; `/episode/{id}/servers` → grouped sub/dub/raw. */
        GROUPED_SERVERS,

        /** `/anime/{id}` → providerEpisodes; a single implicit server. */
        SINGLE_SERVER,

        /** `/anime/{id}/episodes` → data; `/episode/{id}/servers` → flat list. */
        FLAT_SERVERS,
    }

    override val hostUrl: String get() = ApiBackend.requireHost()
    override val saveName: String get() = name

    private fun api(path: String) = "$hostUrl/api/$providerName$path"

    override suspend fun search(query: String): List<ShowResponse> {
        if (query.isBlank()) return emptyList()
        val response = Http.get(api("/anime/search?q=${encode(query)}"), ApiBackend.headers())
            .parsed<SearchResponse>()
        return response.data.map {
            ShowResponse(
                name = it.name ?: it.romaji ?: "Unknown",
                link = it.id,
                coverUrl = FileUrl(it.posterImage.orEmpty()),
            )
        }
    }

    override suspend fun loadEpisodes(animeLink: String, extra: Map<String, String>?): List<Episode> {
        if (animeLink.isBlank()) return emptyList()

        return if (shape == Shape.FLAT_SERVERS) {
            Http.get(api("/anime/$animeLink/episodes"), ApiBackend.headers())
                .parsed<FlatEpisodesResponse>()
                .data.map {
                    Episode(
                        number = it.episodeNumber?.toString() ?: "?",
                        link = it.episodeId,
                        title = it.title ?: "Episode ${it.episodeNumber}",
                    )
                }
        } else {
            Http.get(api("/anime/$animeLink"), ApiBackend.headers())
                .parsed<ProviderEpisodesResponse>()
                .providerEpisodes.map {
                    Episode(
                        number = it.episodeNumber?.toString() ?: "?",
                        link = it.episodeId,
                        title = it.title ?: "Episode ${it.episodeNumber}",
                        description = it.overview,
                        thumbnail = it.thumbnail?.let { url -> FileUrl(url) },
                    )
                }
        }
    }

    override suspend fun loadVideoServers(episodeLink: String, extra: Map<String, String>?): List<VideoServer> {
        if (episodeLink.isBlank()) return emptyList()

        return when (shape) {
            Shape.SINGLE_SERVER -> listOf(
                VideoServer("SUB", FileUrl(api("/sources/$episodeLink")))
            )

            Shape.GROUPED_SERVERS -> {
                val grouped = Http.get(api("/episode/$episodeLink/servers"), ApiBackend.headers())
                    .parsed<GroupedServersResponse>().data

                fun group(version: String, items: List<GroupedServer>) = items.map {
                    VideoServer(
                        name = "${version.uppercase()} - ${it.serverName}",
                        embed = FileUrl(api("/sources/$episodeLink?version=$version&server=${it.serverName}")),
                    )
                }

                // Sub first unless the user asked for dub; the preferred track should be
                // the default pick, with the other still reachable.
                val sub = group("sub", grouped.sub)
                val dub = group("dub", grouped.dub)
                val raw = group("raw", grouped.raw)
                (if (selectDub) dub + sub else sub + dub) + raw
            }

            Shape.FLAT_SERVERS -> {
                val versions = listOf("sub", "dub")
                versions.flatMap { version ->
                    runCatching {
                        Http.get(api("/episode/$episodeLink/servers?version=$version"), ApiBackend.headers())
                            .parsed<FlatServersResponse>().data.map {
                                VideoServer(
                                    name = "${version.uppercase()} - ${it.serverId}",
                                    embed = FileUrl(api("/sources/$episodeLink?version=$version&server=${it.serverId}")),
                                )
                            }
                    }.onFailure { Log.d(name, "no $version servers for $episodeLink: ${it.message}") }
                        .getOrDefault(emptyList())
                }
            }
        }
    }

    override suspend fun getVideoExtractor(server: VideoServer): VideoExtractor = MegaPlay(server)

    @Serializable
    private data class SearchResponse(val data: List<SearchItem> = emptyList())

    @Serializable
    private data class SearchItem(
        val id: String,
        val name: String? = null,
        val romaji: String? = null,
        val posterImage: String? = null,
    )

    @Serializable
    private data class ProviderEpisodesResponse(val providerEpisodes: List<ProviderEpisode> = emptyList())

    @Serializable
    private data class ProviderEpisode(
        val episodeId: String,
        val title: String? = null,
        val episodeNumber: Int? = null,
        val overview: String? = null,
        val thumbnail: String? = null,
    )

    @Serializable
    private data class FlatEpisodesResponse(val data: List<ProviderEpisode> = emptyList())

    @Serializable
    private data class GroupedServersResponse(val data: GroupedServers = GroupedServers())

    @Serializable
    private data class GroupedServers(
        val sub: List<GroupedServer> = emptyList(),
        val dub: List<GroupedServer> = emptyList(),
        val raw: List<GroupedServer> = emptyList(),
    )

    @Serializable
    private data class GroupedServer(val serverName: String, val serverId: String? = null)

    @Serializable
    private data class FlatServersResponse(val data: List<FlatServer> = emptyList())

    @Serializable
    private data class FlatServer(val serverId: String, val serverName: String? = null)
}

class Anikoto : ApiParser() {
    override val name = "Anikoto"
    override val providerName = "anikoto"
    override val shape = Shape.GROUPED_SERVERS
}

// AniBD used to live here. Its site turned out to expose the same API the backend was
// proxying, so it is now a standalone parser in AniBD.kt.

class Anizone : ApiParser() {
    override val name = "Anizone"
    override val providerName = "anizone"
    override val shape = Shape.SINGLE_SERVER
}

class AniDB : ApiParser() {
    override val name = "AniDB"
    override val providerName = "anidb"
    override val shape = Shape.FLAT_SERVERS
}

class AnimePahe : ApiParser() {
    override val name = "AnimePahe"
    override val providerName = "animepahe"
    override val shape = Shape.FLAT_SERVERS
}
