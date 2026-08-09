package ani.saikou.connections.discord.rpc.serializers

import android.annotation.SuppressLint
import kotlinx.serialization.Serializable

@SuppressLint("UnsafeOptInUsageError")
@Serializable
data class GatewayPayload<T>(
    val op: Int,
    val d: T,
    val t: String? = null,
    val s: Int? = null
)