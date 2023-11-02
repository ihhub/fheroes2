package com.ipapps.homm2.livewallpaper

import android.os.Bundle
import android.webkit.WebView
import androidx.appcompat.app.AppCompatActivity
import de.andycandy.android.bridge.Bridge

class WebViewActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_web_view)

        val webView = findViewById<WebView>(R.id.activity_web_view)
        webView.loadUrl("file:///android_asset/www/index.html")

        val bridge = Bridge(applicationContext, webView)
        bridge.addJSInterface(AndroidNativeInterface())
    }
}