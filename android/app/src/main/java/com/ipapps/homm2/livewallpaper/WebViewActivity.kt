package com.ipapps.homm2.livewallpaper

import android.os.Bundle
import android.webkit.WebView
import androidx.appcompat.app.AppCompatActivity
import com.ipapps.homm2.livewallpaper.settings.data.MapUpdateInterval
import com.ipapps.homm2.livewallpaper.settings.data.Scale
import com.ipapps.homm2.livewallpaper.settings.data.ScaleType
import com.ipapps.homm2.livewallpaper.settings.data.WallpaperPreferences
import com.ipapps.homm2.livewallpaper.settings.data.WallpaperPreferencesRepository
import de.andycandy.android.bridge.Bridge
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.flow.collect
import kotlinx.coroutines.flow.launchIn
import kotlinx.coroutines.flow.onEach
import kotlinx.coroutines.flow.update
import org.json.JSONObject

class WebViewActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_web_view)

        val webView = findViewById<WebView>(R.id.activity_web_view)
        webView.loadUrl("file:///android_asset/www/index.html")

        val config =
            getExternalFilesDir(null)?.resolve("fheroes2.cfg")
        val prefs = WallpaperPreferencesRepository(config)

        prefs.preferencesFlow.onEach {
            val obj = JSONObject()
            obj.put("scale", it.scale)
            obj.put("scaleType", it.scaleType)
            obj.put("mapUpdateInterval", it.mapUpdateInterval)
            obj.put("useScroll", it.useScroll)
            obj.put("brightness", it.brightness)
            val jsonString = obj.toString(2)

            println("send settings")
            webView.evaluateJavascript(
                "typeof window.dispatch === 'function' ? " +
                    "window.dispatch(${jsonString}) : " +
                    "console.error('no dispatch', ${jsonString})"
            ) { }
        }.launchIn(
            scope = CoroutineScope(SupervisorJob() + Dispatchers.Main)
        )

        val bridge = Bridge(applicationContext, webView)
        bridge.addJSInterface(AndroidNativeInterface(prefs))
        bridge.addAfterInitializeListener {
            println("noop to trigger update")
            prefs.setBrightness(13)
        }
    }
}