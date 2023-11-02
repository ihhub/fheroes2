package com.ipapps.homm2.livewallpaper

import android.content.Context
import android.os.Bundle
import android.view.View
import android.webkit.JavascriptInterface
import android.webkit.WebView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity

class WebViewActivity : AppCompatActivity() {
    class WebViewJavaScriptInterface(context: Context) {
        private val context: Context

        init {
            this.context = context
        }

        @JavascriptInterface
        fun makeToast(message: String?, lengthLong: Boolean) {
            Toast.makeText(
                context, message, if (lengthLong) Toast.LENGTH_LONG else Toast.LENGTH_SHORT
            ).show()
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_web_view)

        val webView = findViewById<View>(R.id.activity_web_view) as WebView
        webView.loadUrl("file:///android_asset/www/index.html")
        webView.settings.javaScriptEnabled = true
        webView.addJavascriptInterface(WebViewJavaScriptInterface(this), "app")
    }
}