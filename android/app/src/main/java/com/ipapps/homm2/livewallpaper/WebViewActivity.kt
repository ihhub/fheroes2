package com.ipapps.homm2.livewallpaper

import android.app.WallpaperManager
import android.content.ComponentName
import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.provider.OpenableColumns
import android.util.Log
import android.webkit.ValueCallback
import android.webkit.WebChromeClient
import android.webkit.WebView
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import com.ipapps.homm2.livewallpaper.data.MapHeaderReader
import com.ipapps.homm2.livewallpaper.data.SettingsViewModel
import com.ipapps.homm2.livewallpaper.data.WallpaperPreferencesRepository
import com.ipapps.homm2.livewallpaper.data.WebViewSettingsEvent
import com.ipapps.homm2.livewallpaper.data.sendWebViewEvent
import de.andycandy.android.bridge.Bridge
import org.libsdl.app.SDLActivity
import java.io.InputStream
import java.nio.ByteBuffer
import java.nio.ByteOrder

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

    private fun copyFile(input: InputStream, to: String, path: String?) {
        val output = getExternalFilesDir(path)?.resolve(to);

        if (output != null && !output.exists() && output.createNewFile()) {
            val outputStream = output.outputStream()
            input.copyTo(outputStream)
            outputStream.close()
        }
    }

    private var filePathCallback: ValueCallback<Array<Uri>>? = null

    private fun getFileName(uri: Uri): String {
        val cursor = contentResolver.query(
            uri, null, null, null, null, null
        )

        if (cursor != null && cursor.moveToFirst()) {
            val columnIndex = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME)
            if (columnIndex == -1) {
                return "Unknown.mp2"
            }

            // Note it's called "Display Name". This is
            // provider-specific, and might not necessarily be the file name.
            return cursor.getString(columnIndex)
        }

        return "Not-found.mp2"
    }

    private val fileChooserLauncher =
        registerForActivityResult(ActivityResultContracts.GetContent()) { uri ->
            val urisList = mutableListOf<Uri>()
            if (uri != null) {
                contentResolver.openInputStream(uri).use {
                    if (it == null) {
                        return@use
                    }

                    val mh = kotlin.runCatching {
                        MapHeaderReader(it).readMapHeader()
                    }.getOrNull()
                    Log.v("MAP", "Map: ${mh?.title} width: ${mh?.width} height: ${mh?.height}")

                    it.reset()

                    copyFile(it, getFileName(uri), "maps")
                }

                urisList.add(uri)
            }

            filePathCallback?.onReceiveValue(urisList.toTypedArray())
            filePathCallback = null
        }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_web_view)

        assets.open("files/data/resurrection.h2d").use {
            copyFile(it, "resurrection.h2d", "files/data")
            it.close()
        }
        assets.open("fheroes2.cfg").use {
            copyFile(it, "fheroes2.cfg", null)
            it.close()
        }

        val config = getExternalFilesDir(null)?.resolve("fheroes2.cfg")
        val prefsRepository = WallpaperPreferencesRepository(config)
        val settingsViewModel = SettingsViewModel(prefsRepository, ::setWallpaper);

        val webView = findViewById<WebView>(R.id.activity_web_view)
        val bridge = Bridge(applicationContext, webView)
        bridge.addJSInterface(
            AndroidNativeInterface(
                settingsViewModel, getExternalFilesDir("maps")
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
