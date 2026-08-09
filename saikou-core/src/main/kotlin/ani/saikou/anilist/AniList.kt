package ani.saikou.anilist

import ani.saikou.host.Log
import ani.saikou.net.Http
import ani.saikou.rpc.ErrorCodes
import ani.saikou.rpc.RpcException
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.buildJsonObject
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.put

/**
 * AniList GraphQL client. Public queries work signed out; anything touching a user's
 * library needs the token from [Auth].
 */
object AniList {
    const val ENDPOINT = "https://graphql.anilist.co"
    private const val TAG = "AniList"

    suspend fun query(query: String, variables: JsonObject = JsonObject(emptyMap())): JsonObject {
        val body = buildJsonObject {
            put("query", query)
            put("variables", variables)
        }

        val headers = buildMap {
            put("Content-Type", "application/json")
            put("Accept", "application/json")
            Auth.token?.let { put("Authorization", "Bearer $it") }
        }

        val response = Http.postJson(ENDPOINT, headers, Http.json.encodeToString(JsonObject.serializer(), body))

        if (response.code == 429) {
            throw RpcException(
                ErrorCodes.NETWORK,
                "AniList is rate limiting us. Try again in a minute.",
                source = "AniList",
                retryable = true,
            )
        }

        if (!response.text.startsWith("{")) {
            throw RpcException(
                ErrorCodes.NETWORK,
                "AniList returned a non-JSON response (HTTP ${response.code}). It may be down.",
                source = "AniList",
                retryable = true,
            )
        }

        val parsed = Http.json.parseToJsonElement(response.text).jsonObject

        parsed["errors"]?.let { errors ->
            val message = runCatching {
                errors.jsonObject.toString()
            }.getOrDefault(errors.toString())

            // 401 with a token means it expired or was revoked; the UI should prompt a
            // fresh sign-in rather than silently showing an empty library.
            if (response.code == 401 || response.code == 400 && Auth.token != null) {
                Log.w(TAG, "auth rejected: $message")
                throw RpcException(
                    ErrorCodes.NOT_AUTHENTICATED,
                    "AniList rejected the saved login. Sign in again.",
                    source = "AniList",
                    retryable = false,
                )
            }

            throw RpcException(ErrorCodes.NETWORK, "AniList error: $message", source = "AniList", retryable = true)
        }

        return parsed["data"]?.jsonObject
            ?: throw RpcException(ErrorCodes.NETWORK, "AniList returned no data", source = "AniList", retryable = true)
    }
}
