package ani.saikou.parsers

import ani.saikou.parsers.anime.AllAnime
import ani.saikou.parsers.anime.AniBD
import ani.saikou.parsers.anime.AniDB
import ani.saikou.parsers.anime.AnimeHeaven
import ani.saikou.parsers.anime.AnimePahe
import ani.saikou.parsers.anime.Anikoto
import ani.saikou.parsers.anime.ApiBackend
import ani.saikou.parsers.anime.KickAssAnime
import ani.saikou.parsers.anime.Anizone

/**
 * The anime source registry.
 *
 * The standalone sources scrape their site directly and are always available, so a fresh
 * install can play something without any configuration. The rest proxy through the private
 * backend described in [ApiBackend] and stay unavailable until it is set up; [availability]
 * is what the UI shows, because a source the user cannot use should say why rather than
 * fail when clicked.
 *
 * Order matters: [default] takes the first usable entry, so the standalone sources that
 * actually resolve come first — KickAssAnime ahead of AnimeHeaven because it serves HLS
 * with subtitle tracks rather than a bare MP4. AllAnime is kept below both because its API
 * sits behind a Cloudflare challenge that many connections do not pass.
 */
object AnimeSources {
    private data class Entry(
        val name: String,
        val factory: () -> AnimeParser,
        /** False for the sources that only work through the private backend API. */
        val standalone: Boolean,
    )

    private val entries: List<Entry> = listOf(
        Entry("KickAssAnime", ::KickAssAnime, standalone = true),
        Entry("AnimeHeaven", ::AnimeHeaven, standalone = true),
        Entry("AllAnime", ::AllAnime, standalone = true),
        Entry("Anikoto", ::Anikoto, standalone = false),
        Entry("AniBD", ::AniBD, standalone = false),
        Entry("Anizone", ::Anizone, standalone = false),
        Entry("AniDB", ::AniDB, standalone = false),
        Entry("AnimePahe", ::AnimePahe, standalone = false),
    )

    private val instances = mutableMapOf<String, AnimeParser>()

    data class Availability(val name: String, val enabled: Boolean, val reason: String?)

    @Synchronized
    fun get(name: String): AnimeParser? {
        instances[name]?.let { return it }
        val entry = entries.firstOrNull { it.name.equals(name, ignoreCase = true) } ?: return null
        return entry.factory().also { instances[entry.name] = it }
    }

    fun names(): List<String> = entries.map { it.name }

    /** True when this source works without the private backend API. */
    fun isStandalone(name: String): Boolean =
        entries.firstOrNull { it.name.equals(name, ignoreCase = true) }?.standalone == true

    fun availability(): List<Availability> {
        val configured = ApiBackend.isConfigured
        return entries.map { entry ->
            val enabled = entry.standalone || configured
            Availability(
                name = entry.name,
                enabled = enabled,
                reason = if (enabled) {
                    null
                } else {
                    "Needs the private backend API. Set it in Settings → Sources, or with " +
                        "SAIKOU_API_HOST and SAIKOU_API_KEY."
                },
            )
        }
    }

    /** The source to use when the user has not picked one: the first usable entry. */
    fun default(): AnimeParser? =
        entries.firstOrNull { it.standalone || ApiBackend.isConfigured }?.let { get(it.name) }
}
