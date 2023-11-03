package com.ipapps.homm2.livewallpaper

import androidx.compose.runtime.collectAsState
import com.ipapps.homm2.livewallpaper.settings.data.Scale
import com.ipapps.homm2.livewallpaper.settings.data.WallpaperPreferencesRepository
import de.andycandy.android.bridge.CallType
import de.andycandy.android.bridge.DefaultJSInterface
import de.andycandy.android.bridge.NativeCall
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.collect
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking
import org.json.JSONObject

class AndroidNativeInterface(val prefs: WallpaperPreferencesRepository) :
    DefaultJSInterface("Android") {
    @NativeCall(CallType.FULL_SYNC)
    fun helloFullSync(name: String): String {
        return "helloFullSync return: $name"
    }

    @NativeCall(CallType.WEB_PROMISE)
    fun helloWebPromise(name: String): String {
        return "helloWebPromise return: $name"
    }

    @NativeCall(CallType.FULL_PROMISE)
    fun helloFullPromise(name: String) = doInBackground { promise ->
        runBlocking {
            launch {
                delay(5000L)
                promise.resolve("helloFullPromise return: $name")
            }
        }
    }

    @NativeCall(CallType.FULL_SYNC)
    fun setBrightness(value: Int) {
        prefs.setBrightness(value)
    }

    @NativeCall(CallType.FULL_SYNC)
    fun setScale(value: Int) {
        prefs.setScale(Scale.fromInt(value))
    }

    @NativeCall(CallType.FULL_SYNC)
    fun setScaleType(value: String) {
        prefs.setScaleTypeString(value)
    }

    @NativeCall(CallType.FULL_SYNC)
    fun toggleUseScroll() {
        prefs.toggleUseScroll()
    }
}