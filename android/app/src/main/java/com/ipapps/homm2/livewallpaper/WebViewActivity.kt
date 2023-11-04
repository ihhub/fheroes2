package com.ipapps.homm2.livewallpaper

import android.app.WallpaperManager
import android.content.ComponentName
import android.content.Intent
import android.os.Bundle
import android.webkit.WebView
import androidx.appcompat.app.AppCompatActivity
import com.ipapps.homm2.livewallpaper.settings.data.SettingsViewModel
import com.ipapps.homm2.livewallpaper.settings.data.WallpaperPreferencesRepository
import de.andycandy.android.bridge.Bridge
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.flow.launchIn
import kotlinx.coroutines.flow.onEach
import org.json.JSONObject
import org.libsdl.app.SDLActivity

class WebViewActivity : AppCompatActivity() {
    private fun setWallpaper() {
        startActivity(
            Intent().setAction(WallpaperManager.ACTION_CHANGE_LIVE_WALLPAPER).putExtra(
                WallpaperManager.EXTRA_LIVE_WALLPAPER_COMPONENT, ComponentName(
                    applicationContext, SDLActivity::class.java
                )
            )
        )
    }

    private fun openIconAuthorUrl() {
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_web_view)

        val webView = findViewById<WebView>(R.id.activity_web_view)
        webView.loadUrl("file:///android_asset/www/index.html")

        val config = getExternalFilesDir(null)?.resolve("fheroes2.cfg")
        val prefsRepository = WallpaperPreferencesRepository(config)
        val settingsViewModel =
            SettingsViewModel(prefsRepository, ::setWallpaper, ::openIconAuthorUrl);

        val bridge = Bridge(applicationContext, webView)
        bridge.addJSInterface(AndroidNativeInterface(settingsViewModel))

        bridge.addAfterInitializeListener {
            settingsViewModel.subscribeToPreferences {
                val obj = JSONObject()
                obj.put("scale", it.scale.value)
                obj.put("scaleType", it.scaleType.value)
                obj.put("mapUpdateInterval", it.mapUpdateInterval.value)
                obj.put("useScroll", it.useScroll)
                obj.put("brightness", it.brightness)
                val jsonString = obj.toString(2)

                webView.evaluateJavascript(
                    """
                if (typeof window.dispatchWebViewEvent === 'function') {
                    window.dispatchWebViewEvent(${jsonString});
                } else {
                    console.error('no dispatchWebViewEvent', ${jsonString})
                }
                """
                ) { }
            }
        }
    }
}