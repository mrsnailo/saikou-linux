package ani.saikou.connections.discord.rpc.serializers


import android.annotation.SuppressLint
import kotlinx.serialization.Serializable

@SuppressLint("UnsafeOptInUsageError")
@Serializable
data class Identify(
    val token: String,
    val properties: IdentifyProperties
)

