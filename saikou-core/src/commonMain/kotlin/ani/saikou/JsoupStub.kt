package ani.saikou.parsers.novel
import io.ktor.http.*


import com.fleeksoft.ksoup.Ksoup
import com.fleeksoft.ksoup.nodes.Document

object Jsoup {
    fun parse(html: String): Document = Ksoup.parse(html)
}
