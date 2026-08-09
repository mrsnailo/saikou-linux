package ani.saikou.parsers

import ani.saikou.parsers.anime.AllAnime
import ani.saikou.parsers.anime.AniBD
import ani.saikou.parsers.anime.AniDB
import ani.saikou.parsers.anime.AnimeHeaven
import ani.saikou.parsers.anime.AnimePahe
import ani.saikou.parsers.anime.Anikoto
import ani.saikou.parsers.anime.ApiBackend
import ani.saikou.parsers.anime.Anizone

/**
 * The anime source registry.
 *
 * Every general source proxies through the backend API, so all of them are unavailable
 * until it is configured. [availability] is what the UI shows: a source the user cannot
 * use should say why, not fail when clicked.
 */
object AnimeSources {
    private val factories: List<Pair<String, () -> AnimeParser>> = listOf(
        "Anikoto" to ::Anikoto,
        "AniBD" to ::AniBD,
        "Anizone" to ::Anizone,
        "AnimeHeaven" to ::AnimeHeaven,
        "AniDB" to ::AniDB,
        "AllAnime" to ::AllAnime,
        "AnimePahe" to ::AnimePahe,
    )

    private val instances = mutableMapOf<String, AnimeParser>()

    data class Availability(val name: String, val enabled: Boolean, val reason: String?)

    @Synchronized
    fun get(name: String): AnimeParser? {
        instances[name]?.let { return it }
        val factory = factories.firstOrNull { it.first.equals(name, ignoreCase = true) } ?: return null
        return factory.second().also { instances[factory.first] = it }
    }

    fun names(): List<String> = factories.map { it.first }

    fun availability(): List<Availability> {
        val configured = ApiBackend.isConfigured
        val reason = if (configured) {
            null
        } else {
            "Needs an anime backend. Set it in Settings, or with SAIKOU_API_HOST and SAIKOU_API_KEY."
        }
        return factories.map { Availability(it.first, configured, reason) }
    }

    /** The source to use when the user has not picked one. */
    fun default(): AnimeParser? = names().firstNotNullOfOrNull { get(it) }.takeIf { ApiBackend.isConfigured }
}
