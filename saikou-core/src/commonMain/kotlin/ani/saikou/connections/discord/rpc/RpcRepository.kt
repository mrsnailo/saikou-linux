package ani.saikou.connections.discord.rpc

import android.content.Context
import androidx.core.content.edit
import ani.saikou.R

class RpcRepository(private val context: Context) {

    private val discordRpcKey = "discord_rpc"

    fun saveRpcPreference(enabled: Boolean) {
        val sharedPreferences = context.getSharedPreferences(
            context.getString(R.string.preference_file_key), Context.MODE_PRIVATE
        )
        sharedPreferences.edit {
            putBoolean(discordRpcKey, enabled)
            apply()
        }
    }


    fun loadRpcPreference(): Boolean {
        val sharedPreferences = context.getSharedPreferences(
            context.getString(R.string.preference_file_key), Context.MODE_PRIVATE
        )

        return sharedPreferences.getBoolean(discordRpcKey, false)
    }
}