package ani.saikou.parsers

import ani.saikou.connections.loadData
import ani.saikou.connections.saveData

data class ShowResponse(val name: String, val link: String, val coverUrl: String)
data class Episode(val number: String, val link: String, val title: String? = null, val thumbnail: String? = null, val description: String? = null, val isFiller: Boolean = false, val extra: Map<String,String>? = null)
data class MangaChapter(val name: String, val link: String)
data class MangaImage(val url: String)

fun updateSources() {}
fun getMangaParsers() = emptyList<MangaParser>()
fun getNovelParsers() = emptyList<NovelParser>()
fun getAnimeParsers() = emptyList<AnimeParser>()
