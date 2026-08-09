package ani.saikou.connections
import io.ktor.http.*


class Context { fun getString(id: Int): String = "" }

inline fun <reified T> loadData(key: String): T? = null
fun saveData(key: String, value: Any) {}
