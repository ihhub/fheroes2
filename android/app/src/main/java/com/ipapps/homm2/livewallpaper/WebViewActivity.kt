package com.ipapps.homm2.livewallpaper

import android.app.WallpaperManager
import android.content.ComponentName
import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.webkit.ValueCallback
import android.webkit.WebChromeClient
import android.webkit.WebView
import androidx.activity.result.contract.ActivityResultContracts
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

    private fun copyFile(from: String, to: String, path: String?) {
        val input = assets.open(from)
        val output = getExternalFilesDir(path)?.resolve(to);

        if (output != null && !output.exists() && output.createNewFile()) {
            input.copyTo(output.outputStream())
        }
    }

    private var filePathCallback: ValueCallback<Array<Uri>>? = null

    private val fileChooserLauncher =
        registerForActivityResult(ActivityResultContracts.GetContent()) { uri ->
            val urisList = mutableListOf<Uri>()
            if (uri != null) {
                println("selected url $uri")
                urisList.add(uri)
            } else {
                println("No uri")
            }

            filePathCallback?.onReceiveValue(urisList.toTypedArray())
            filePathCallback = null
        }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_web_view)

        copyFile("files/data/resurrection.h2d", "resurrection.h2d", "files/data")
        copyFile("fheroes2.cfg", "fheroes2.cfg", null)

        val config = getExternalFilesDir(null)?.resolve("fheroes2.cfg")
        val prefsRepository = WallpaperPreferencesRepository(config)
        val settingsViewModel =
            SettingsViewModel(prefsRepository, ::setWallpaper);

        val webView = findViewById<WebView>(R.id.activity_web_view)
        val bridge = Bridge(applicationContext, webView)
        bridge.addJSInterface(
            AndroidNativeInterface(
                settingsViewModel,
                getExternalFilesDir("maps")
            )
        )
        bridge.addAfterInitializeListener {
            settingsViewModel.subscribeToPreferences {
                sendWebViewEvent(WebViewSettingsEvent(it), webView)
            }
        }

        webView.webChromeClient = object : WebChromeClient() {
            override fun onShowFileChooser(
                webView: WebView?,
                filePathCallback: ValueCallback<Array<Uri>>?,
                fileChooserParams: FileChooserParams?
            ): Boolean {
                fileChooserLauncher.launch("*/*")
                this@WebViewActivity.filePathCallback = filePathCallback

                return true;
            }
        }

        webView.loadUrl("file:///android_asset/www/index.html")
    }
}
