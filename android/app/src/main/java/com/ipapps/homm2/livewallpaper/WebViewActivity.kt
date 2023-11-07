package com.ipapps.homm2.livewallpaper

import android.app.WallpaperManager
import android.content.ComponentName
import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.provider.OpenableColumns
import android.util.Log
import android.view.KeyEvent
import android.webkit.ValueCallback
import android.webkit.WebChromeClient
import android.webkit.WebView
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import com.ipapps.homm2.livewallpaper.data.MapHeader
import com.ipapps.homm2.livewallpaper.data.MapReader.Companion.readMap
import com.ipapps.homm2.livewallpaper.data.SettingsViewModel
import com.ipapps.homm2.livewallpaper.data.WallpaperPreferencesRepository
import com.ipapps.homm2.livewallpaper.data.WebViewSettingsEvent
import com.ipapps.homm2.livewallpaper.data.sendWebViewEvent
import de.andycandy.android.bridge.Bridge
import org.libsdl.app.SDLActivity
import java.io.InputStream

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

    private fun getMap(uri: Uri): MapHeader? {
        val input = contentResolver.openInputStream(uri)
        val map = kotlin.runCatching { readMap(input) }.getOrNull()
        input?.close()

        Log.v(
            "MAP",
            "Map: ${map?.title} width: ${map?.width} height: ${map?.height} pol: ${map?.isPoL}"
        )

        return map
    }

    private fun getFileName(uri: Uri): String? {
        val cursor = contentResolver.query(
            uri, null, null, null, null, null
        )

        if (cursor?.moveToFirst() == null) {
            cursor?.close()
            return null;
        }

        val columnIndex = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME)
        if (columnIndex == -1) {
            cursor.close()
            return null
        }

        // Note it's called "Display Name". This is
        // provider-specific, and might not necessarily be the file name.
        val filename = cursor.getString(columnIndex)
        cursor.close()
        if (filename.length > 30 || !filename.endsWith(".mp2")) {
            return null;
        }

        return filename
    }

    private var filePathCallback: ValueCallback<Array<Uri>>? = null
    private val fileChooserLauncher =
        registerForActivityResult(ActivityResultContracts.GetContent()) { uri ->
            val urisList = mutableListOf<Uri>()

            kotlin.runCatching {
                if (uri == null) {
                    return@runCatching
                }

                val map = getMap(uri) ?: return@runCatching
                val input = contentResolver.openInputStream(uri) ?: return@runCatching
                val fileName = getFileName(uri) ?: "${map.title}.mp2"
                if (map.isPoL) {
                    Log.v("MAP", "$fileName is pol")
                    return@runCatching
                }

                copyFile(input, fileName, "maps")
                input.close()

                urisList.add(uri)
            }

            if (urisList.isEmpty()) {
                Toast
                    .makeText(applicationContext, "Selected file is not correct map", Toast.LENGTH_SHORT)
                    .show()
                filePathCallback?.onReceiveValue(null)
                filePathCallback = null
                return@registerForActivityResult
            }

            filePathCallback?.onReceiveValue(urisList.toTypedArray())
            filePathCallback = null
        }

    override fun onKeyDown(keyCode: Int, event: KeyEvent?): Boolean {
        val webView = findViewById<WebView>(R.id.activity_web_view)

        if (keyCode == KeyEvent.KEYCODE_BACK && webView.canGoBack()) {
            webView.goBack()
            return true
        }

        return super.onKeyDown(keyCode, event)
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
