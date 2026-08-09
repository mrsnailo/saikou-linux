package ani.saikou.connections.discord.rpc.serializers

import android.annotation.SuppressLint
import kotlinx.serialization.SerialName
import kotlinx.serialization.Serializable

@SuppressLint("UnsafeOptInUsageError")
@Serializable
data class IdentifyProperties(
    @SerialName("\$os") val os: String = "windows",
    @SerialName("\$browser") val browser: String ="chrome",
    @SerialName("\$device") val device: String= "disco"
)