package ani.saikou.connections.anilist
import io.ktor.http.*


import kotlinx.serialization.Serializable

data class Genre(
    val name: String,
    var id: Int,
    var thumbnail: String,
    var time: Long,
) 