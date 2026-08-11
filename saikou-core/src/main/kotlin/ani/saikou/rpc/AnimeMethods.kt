package ani.saikou.rpc

import ani.saikou.net.Http
import ani.saikou.parsers.AnimeParser
import ani.saikou.parsers.AnimeSources
import ani.saikou.parsers.VideoContainer
import ani.saikou.parsers.anime.ApiBackend
import kotlinx.serialization.json.JsonElement
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.JsonPrimitive
import kotlinx.serialization.json.buildJsonArray
import kotlinx.serialization.json.buildJsonObject
import kotlinx.serialization.json.encodeToJsonElement
import kotlinx.serialization.json.jsonPrimitive
import kotlinx.serialization.json.put

fun Registry.registerAnimeMethods() {
    register("anime.sources") { _, _ ->
        buildJsonArray {
            AnimeSources.availability().forEach { source ->
                add(buildJsonObject {
                    put("name", source.name)
                    put("enabled", source.enabled)
                    put("anilistKeyed", source.anilistKeyed)
                    source.reason?.let { put("reason", it) }
                })
            }
        }
    }

    register("anime.backend.status") { _, _ ->
        buildJsonObject {
            put("configured", ApiBackend.isConfigured)
            put("host", ApiBackend.host ?: "")
        }
    }

    register("anime.backend.configure") { params, _ ->
        val obj = params.obj()
        ApiBackend.configure(obj.string("host"), obj.string("key"))
        buildJsonObject { put("configured", ApiBackend.isConfigured) }
    }

    register("anime.search") { params, _ ->
        val parser = params.parser()
        val query = params.obj().string("query")
        Http.json.encodeToJsonElement(parser.search(query))
    }

    register("anime.episodes") { params, _ ->
        val parser = params.parser()
        val link = params.obj().string("link")
        Http.json.encodeToJsonElement(parser.loadEpisodes(link, null))
    }

    register("anime.servers") { params, _ ->
        val parser = params.parser()
        val link = params.obj().string("episodeLink")
        Http.json.encodeToJsonElement(parser.loadVideoServers(link, null))
    }

    /**
     * Resolves an episode all the way to playable urls. Servers are tried in order and
     * the first that yields a video wins, so the UI gets something playable from one
     * call instead of orchestrating the fallback itself.
     */
    register("anime.streams") { params, session ->
        val parser = params.parser()
        val obj = params.obj()
        val episodeLink = obj.string("episodeLink")
        val preferred = obj["server"]?.jsonPrimitive?.content

        val servers = parser.loadVideoServers(episodeLink, null)
        if (servers.isEmpty()) {
            throw RpcException(
                ErrorCodes.SOURCE_FAILED,
                "${parser.name} returned no servers for this episode.",
                source = parser.name,
                retryable = true,
            )
        }

        val ordered = preferred
            ?.let { name -> servers.sortedByDescending { it.name == name } }
            ?: servers

        val failures = mutableListOf<String>()
        for (server in ordered) {
            session.notify("anime.streamProgress", buildJsonObject {
                put("server", server.name)
                put("state", "trying")
            })

            val container = runCatching { parser.getVideoExtractor(server)?.extract() }
                .onFailure { failures += "${server.name}: ${it.message}" }
                .getOrNull()

            if (container != null && container.videos.isNotEmpty()) {
                return@register buildJsonObject {
                    put("server", server.name)
                    put("container", Http.json.encodeToJsonElement(VideoContainer.serializer(), container))
                }
            }
        }

        throw RpcException(
            ErrorCodes.SOURCE_FAILED,
            "No server produced a playable stream. Tried ${ordered.size}: ${failures.take(3).joinToString("; ")}",
            source = parser.name,
            retryable = true,
        )
    }
}

private fun JsonElement?.parser(): AnimeParser {
    val obj = this as? JsonObject ?: throw RpcException(ErrorCodes.INVALID_PARAMS, "expected an object")
    val requested = obj["source"]?.jsonPrimitive?.content

    val parser = if (requested.isNullOrBlank()) AnimeSources.default() else AnimeSources.get(requested)
        ?: throw RpcException(
            ErrorCodes.INVALID_PARAMS,
            "Unknown source '$requested'. Known: ${AnimeSources.names().joinToString()}",
        )

    if (parser == null) {
        throw RpcException(
            ErrorCodes.CAPABILITY_MISSING,
            "No anime source is usable. The standalone source could not be created and the " +
                "private backend API is not configured.",
            retryable = false,
        )
    }

    // Only the API-backed sources need the private backend. Refusing every call whenever it
    // is unset — which is the normal state — is what made a fresh install unable to play
    // anything at all, including through the sources that need no backend.
    if (!AnimeSources.isStandalone(parser.name) && !ApiBackend.isConfigured) {
        throw RpcException(
            ErrorCodes.CAPABILITY_MISSING,
            "${parser.name} needs the private backend API, which is not configured. " +
                "Set it in Settings → Sources, or pick KickAssAnime or AnimeHeaven, " +
                    "which need no backend.",
            source = parser.name,
            retryable = false,
        )
    }

    return parser
}

// `obj()` and `string()` are the shared helpers declared in AniListMethods.kt.
