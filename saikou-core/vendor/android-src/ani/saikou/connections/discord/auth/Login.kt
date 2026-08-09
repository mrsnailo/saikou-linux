package ani.saikou.connections.discord.auth

import android.annotation.SuppressLint
import android.content.Context
import android.os.Bundle
import android.webkit.*
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import ani.saikou.R
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

class Login : AppCompatActivity() {
    private lateinit var repository: DiscordRepository


    private val USER_AGENT =
        "Mozilla/5.0 (Linux; Android 14; SM-S921U; Build/UP1A.231005.007) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/129.0.0.0 Mobile Safari/537.36"

    @SuppressLint("SetJavaScriptEnabled")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        repository = DiscordRepository(this)

        setContentView(R.layout.activity_discord)

        val webView = findViewById<WebView>(R.id.discordWebview)

        webView.apply {
            settings.javaScriptEnabled = true
            settings.domStorageEnabled = true
            settings.databaseEnabled = true
            settings.userAgentString = USER_AGENT

            CookieManager.getInstance().setAcceptCookie(true)
        }

        val jsSnippet = """
            javascript:(function(){
                var i = document.createElement('iframe');
                document.body.appendChild(i);
                var token = i.contentWindow.localStorage.token;
                if (token) {
                    alert(token.replace(/"/g, ""));
                }
            })()
        """.trimIndent()

        webView.webChromeClient = object : WebChromeClient() {
            override fun onJsAlert(
                view: WebView,
                url: String,
                message: String,
                result: JsResult
            ): Boolean {
                if (message.isNotBlank() && message != "null" && message != "undefined") {
                    handleToken(message)
                    result.confirm()
                    return true
                }
                return super.onJsAlert(view, url, message, result)
            }
        }

        webView.webViewClient = object : WebViewClient() {
            override fun onPageFinished(view: WebView?, url: String?) {
                // When we hit /app, the login is successful and the token is available
                if (url?.contains("/app") == true || url?.contains("/channels") == true) {
                    view?.loadUrl(jsSnippet)
                }
            }
        }

        webView.loadUrl("https://discord.com/login")
    }

    private fun handleToken(token: String) {

        lifecycleScope.launch(Dispatchers.IO) {
            try {
                repository.saveToken(token)
                val userData = repository.fetchUserData(token)
                val userName = userData?.globalName ?: userData?.username
                withContext(Dispatchers.Main) {
                    Toast.makeText(
                        this@Login,
                        "Welcome ${userName}",
                        Toast.LENGTH_SHORT
                    ).show()
                    setResult(RESULT_OK)
                    finish()
                }
            } catch (e: Exception) {
                withContext(Dispatchers.Main) {
                    Toast.makeText(this@Login, "Login failed: ${e.message}", Toast.LENGTH_LONG)
                        .show()
                }
            }
        }
    }
}