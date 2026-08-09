package ani.saikou.parsers

import kotlinx.serialization.Serializable

@Serializable
open class BaseParser {
    open val name: String = ""
    open val saveName: String = ""
    open val hostUrl: String = ""
    val isNSFW: Boolean = false

    open suspend fun search(query: String): List<ShowResponse> = emptyList()
}
