package ani.saikou

import kotlinx.cinterop.*

import kotlinx.serialization.encodeToString

@OptIn(kotlinx.cinterop.ExperimentalForeignApi::class, kotlin.experimental.ExperimentalNativeApi::class)
@CName("saikouCoreInit")
fun saikouCoreInit() {
    initializeNetwork()
}

@OptIn(kotlinx.cinterop.ExperimentalForeignApi::class, kotlin.experimental.ExperimentalNativeApi::class)
@CName("searchAnime")
fun searchAnime(query: CPointer<ByteVar>): CPointer<ByteVar> {
    val q = query.toKString()
    val resultJson = Mapper.json.encodeToString(mapOf("query" to q, "results" to emptyList<String>()))
    
    // In KMP Native, we use MemScope or arena for returning strings, but for simplicity here we use a static or leaked buffer.
    // For a real C export, Kotlin/Native provides `kotlinx.cinterop.Arena().allocArray(...)`
    val arena = Arena()
    return resultJson.cstr.getPointer(arena)
}
