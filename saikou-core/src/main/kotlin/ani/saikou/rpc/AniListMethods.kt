package ani.saikou.rpc

import ani.saikou.anilist.AniList
import ani.saikou.anilist.Auth
import ani.saikou.anilist.Queries
import kotlinx.serialization.json.JsonArray
import kotlinx.serialization.json.JsonElement
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.JsonPrimitive
import kotlinx.serialization.json.buildJsonArray
import kotlinx.serialization.json.buildJsonObject
import kotlinx.serialization.json.int
import kotlinx.serialization.json.jsonArray
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonPrimitive
import kotlinx.serialization.json.put
import java.time.LocalDate

fun Registry.registerAniListMethods() {
    register("anilist.status") { _, _ ->
        buildJsonObject {
            put("configured", Auth.isConfigured)
            put("loggedIn", Auth.isLoggedIn)
            put("usesOwnClient", Auth.usesOwnClient)
            put("clientId", Auth.clientId ?: "")
            put("redirectUri", Auth.redirectUri)
            put("developerUrl", "https://anilist.co/settings/developer")
        }
    }

    /**
     * Points sign-in at a client of the user's own. Optional — the build ships its own
     * client id and the one-click flow needs nothing from the user. Passing blanks clears
     * the override and returns to the built-in client.
     */
    register("anilist.configure") { params, _ ->
        val obj = params.obj()
        Auth.configure(
            clientId = obj["clientId"]?.jsonPrimitive?.content.orEmpty(),
            clientSecret = obj["clientSecret"]?.jsonPrimitive?.content.orEmpty(),
            port = obj["port"]?.jsonPrimitive?.content?.toIntOrNull(),
        )
        buildJsonObject {
            put("redirectUri", Auth.redirectUri)
            put("configured", Auth.isConfigured)
            put("usesOwnClient", Auth.usesOwnClient)
        }
    }

    /** The url the UI opens in a browser. Paired with `anilist.awaitLogin`. */
    register("anilist.authorizeUrl") { _, _ -> JsonPrimitive(Auth.authorizeUrl()) }

    /**
     * Blocks until the browser redirect lands. The UI calls this right after opening the
     * authorize url and shows a "waiting for AniList…" state; the RPC layer runs each
     * request on its own coroutine, so nothing else stalls meanwhile.
     */
    register("anilist.awaitLogin") { _, session ->
        Auth.awaitLogin()
        val viewer = AniList.query(Queries.VIEWER)["Viewer"]?.jsonObject
        session.notify("anilist.loggedIn", viewer)
        viewer ?: JsonObject(emptyMap())
    }

    register("anilist.logout") { _, _ ->
        Auth.logout()
        JsonPrimitive(true)
    }

    register("anilist.viewer") { _, _ ->
        if (!Auth.isLoggedIn) throw RpcException(ErrorCodes.NOT_AUTHENTICATED, "Not signed in to AniList")
        AniList.query(Queries.VIEWER)["Viewer"] ?: JsonObject(emptyMap())
    }

    register("anilist.trending") { params, _ ->
        val page = params.intOr("page", 1)
        AniList.query(
            Queries.TRENDING,
            buildJsonObject { put("page", page); put("perPage", params.intOr("perPage", 30)) },
        ).page()
    }

    register("anilist.thisSeason") { params, _ ->
        val (season, year) = currentSeason()
        AniList.query(
            Queries.POPULAR_THIS_SEASON,
            buildJsonObject {
                put("page", params.intOr("page", 1))
                put("perPage", params.intOr("perPage", 30))
                put("season", season)
                put("seasonYear", year)
            },
        ).page()
    }

    register("anilist.search") { params, _ ->
        val query = params.obj().string("query")
        if (query.isBlank()) return@register buildJsonObject { put("media", buildJsonArray { }) }
        AniList.query(
            Queries.SEARCH,
            buildJsonObject {
                put("search", query)
                put("page", params.intOr("page", 1))
                put("perPage", params.intOr("perPage", 30))
            },
        ).page()
    }

    /**
     * The filtered browse query behind the Browse screen. Absent filters are left out of
     * the variables map rather than sent as null, so AniList treats them as unset.
     */
    register("anilist.browse") { params, _ ->
        val obj = (params as? JsonObject) ?: JsonObject(emptyMap())
        AniList.query(
            Queries.BROWSE,
            buildJsonObject {
                put("page", params.intOr("page", 1))
                put("perPage", params.intOr("perPage", 40))
                obj["search"]?.jsonPrimitive?.content?.takeIf { it.isNotBlank() }?.let { put("search", it) }
                obj["genres"]?.let { genres ->
                    val list = genres as? JsonArray ?: buildJsonArray { add(genres) }
                    if (list.isNotEmpty()) put("genres", list)
                }
                obj["sort"]?.jsonPrimitive?.content?.takeIf { it.isNotBlank() }?.let {
                    put("sort", buildJsonArray { add(JsonPrimitive(it)) })
                }
                obj["format"]?.jsonPrimitive?.content?.takeIf { it.isNotBlank() }?.let { put("format", it) }
                obj["status"]?.jsonPrimitive?.content?.takeIf { it.isNotBlank() }?.let { put("status", it) }
                obj["season"]?.jsonPrimitive?.content?.takeIf { it.isNotBlank() }?.let { season ->
                    put("season", season)
                    put("seasonYear", params.intOr("seasonYear", LocalDate.now().year))
                }
            },
        ).page()
    }

    /** AniList's canonical genre list, so the Genres screen is never out of date. */
    register("anilist.genres") { _, _ ->
        AniList.query(Queries.GENRES)["GenreCollection"] ?: JsonArray(emptyList())
    }

    /**
     * Airing schedule for a window of days. Defaults to the seven days starting today,
     * which is exactly what the Calendar screen shows.
     */
    register("anilist.airing") { params, _ ->
        val days = params.intOr("days", 7).coerceIn(1, 14)
        val start = params.intOr("start", (System.currentTimeMillis() / 1000).toInt())
        AniList.query(
            Queries.AIRING,
            buildJsonObject {
                put("start", start - 1)
                put("end", start + days * 86_400)
                put("page", params.intOr("page", 1))
                put("perPage", params.intOr("perPage", 50))
            },
        ).page()
    }

    register("anilist.media") { params, _ ->
        AniList.query(
            Queries.MEDIA,
            buildJsonObject { put("id", params.obj().int("id")) },
        )["Media"] ?: throw RpcException(ErrorCodes.NETWORK, "AniList returned no media")
    }

    /**
     * The watch list, flattened to entries. Defaults to what the user is actively
     * watching, which is what the home screen leads with.
     */
    register("anilist.userList") { params, _ ->
        if (!Auth.isLoggedIn) throw RpcException(ErrorCodes.NOT_AUTHENTICATED, "Not signed in to AniList")

        val viewerId = AniList.query(Queries.VIEWER)["Viewer"]?.jsonObject?.get("id")?.jsonPrimitive?.int
            ?: throw RpcException(ErrorCodes.NOT_AUTHENTICATED, "Could not read the AniList profile")

        val statuses = (params as? JsonObject)?.get("status")?.let { value ->
            when (value) {
                is JsonArray -> value
                is JsonPrimitive -> buildJsonArray { add(value) }
                else -> null
            }
        } ?: buildJsonArray {
            add(JsonPrimitive("CURRENT"))
            add(JsonPrimitive("REPEATING"))
        }

        val lists = AniList.query(
            Queries.USER_LIST,
            buildJsonObject { put("userId", viewerId); put("status", statuses) },
        )["MediaListCollection"]?.jsonObject?.get("lists")?.jsonArray ?: JsonArray(emptyList())

        buildJsonObject {
            put("entries", buildJsonArray {
                lists.forEach { list ->
                    list.jsonObject["entries"]?.jsonArray?.forEach { add(it) }
                }
            })
        }
    }

    register("anilist.setProgress") { params, _ ->
        if (!Auth.isLoggedIn) throw RpcException(ErrorCodes.NOT_AUTHENTICATED, "Not signed in to AniList")
        val obj = params.obj()
        AniList.query(
            Queries.SAVE_PROGRESS,
            buildJsonObject {
                put("mediaId", obj.int("mediaId"))
                put("progress", obj.int("progress"))
                obj["status"]?.let { put("status", it) }
            },
        )["SaveMediaListEntry"] ?: JsonObject(emptyMap())
    }

    register("anilist.setScore") { params, _ ->
        if (!Auth.isLoggedIn) throw RpcException(ErrorCodes.NOT_AUTHENTICATED, "Not signed in to AniList")
        val obj = params.obj()
        AniList.query(
            Queries.SAVE_SCORE,
            buildJsonObject {
                put("mediaId", obj.int("mediaId"))
                put("score", obj["score"]?.jsonPrimitive?.content?.toDoubleOrNull() ?: 0.0)
            },
        )["SaveMediaListEntry"] ?: JsonObject(emptyMap())
    }
}

/** AniList wraps list results in `Page`; the UI only ever wants what is inside. */
private fun JsonObject.page(): JsonElement = this["Page"] ?: JsonObject(emptyMap())

private fun currentSeason(): Pair<String, Int> {
    val now = LocalDate.now()
    val season = when (now.monthValue) {
        1, 2, 3 -> "WINTER"
        4, 5, 6 -> "SPRING"
        7, 8, 9 -> "SUMMER"
        else -> "FALL"
    }
    return season to now.year
}

internal fun JsonElement?.obj(): JsonObject =
    this as? JsonObject ?: throw RpcException(ErrorCodes.INVALID_PARAMS, "expected an object")

internal fun JsonObject.string(key: String): String =
    this[key]?.jsonPrimitive?.content
        ?: throw RpcException(ErrorCodes.INVALID_PARAMS, "missing '$key'")

internal fun JsonObject.int(key: String): Int =
    this[key]?.jsonPrimitive?.content?.toIntOrNull()
        ?: throw RpcException(ErrorCodes.INVALID_PARAMS, "missing or non-numeric '$key'")

internal fun JsonElement?.intOr(key: String, fallback: Int): Int =
    (this as? JsonObject)?.get(key)?.jsonPrimitive?.content?.toIntOrNull() ?: fallback
