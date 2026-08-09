package ani.saikou.connections.discord.rpc.serializers

import android.annotation.SuppressLint
import kotlinx.serialization.SerialName
import kotlinx.serialization.Serializable

@SuppressLint("UnsafeOptInUsageError")
@Serializable
data class Metadata(
    @SerialName("button_urls")
    val buttonUrls: List<String?>?,
)