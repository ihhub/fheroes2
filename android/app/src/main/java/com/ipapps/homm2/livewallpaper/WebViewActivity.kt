package com.ipapps.homm2.livewallpaper

import android.app.WallpaperManager
import android.content.ComponentName
import android.content.Intent
import android.os.Bundle
import android.webkit.WebView
import androidx.appcompat.app.AppCompatActivity
import com.ipapps.homm2.livewallpaper.data.SettingsViewModel
import com.ipapps.homm2.livewallpaper.data.WallpaperPreferencesRepository
import com.ipapps.homm2.livewallpaper.data.WebViewSettingsEvent
import com.ipapps.homm2.livewallpaper.data.sendWebViewEvent
import de.andycandy.android.bridge.Bridge
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

    private fun copyResurrectionH2d() {
        val input = assets.open("files/data/resurrection.h2d")
        val output = getExternalFilesDir("files/data")?.resolve("resurrection.h2d");

        if (output != null && !output.exists() && output.createNewFile()) {
            input.copyTo(output.outputStream())
        }
    }

    private fun copyConfigFile() {
        val input = assets.open("fheroes2.cfg")
        val output = getExternalFilesDir(null)?.resolve("fheroes2.cfg");

        if (output != null && !output.exists() && output.createNewFile()) {
            input.copyTo(output.outputStream())
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_web_view)

        copyResurrectionH2d()
        copyConfigFile()

        val config = getExternalFilesDir(null)?.resolve("fheroes2.cfg")
        val prefsRepository = WallpaperPreferencesRepository(config)
        val settingsViewModel =
            SettingsViewModel(prefsRepository, ::setWallpaper, ::openIconAuthorUrl);

        val webView = findViewById<WebView>(R.id.activity_web_view)
        val bridge = Bridge(applicationContext, webView)
        bridge.addJSInterface(AndroidNativeInterface(settingsViewModel))
        bridge.addAfterInitializeListener {
            settingsViewModel.subscribeToPreferences {
                sendWebViewEvent(WebViewSettingsEvent(it), webView)
            }
        }

        webView.loadUrl("file:///android_asset/www/index.html")
    }
}
