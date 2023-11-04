package com.ipapps.homm2.livewallpaper

import androidx.compose.runtime.collectAsState
import com.ipapps.homm2.livewallpaper.settings.data.MapUpdateInterval
import com.ipapps.homm2.livewallpaper.settings.data.Scale
import com.ipapps.homm2.livewallpaper.settings.data.ScaleType
import com.ipapps.homm2.livewallpaper.settings.data.SettingsViewModel
import com.ipapps.homm2.livewallpaper.settings.data.WallpaperPreferencesRepository
import de.andycandy.android.bridge.CallType
import de.andycandy.android.bridge.DefaultJSInterface
import de.andycandy.android.bridge.NativeCall
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.collect
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking
import org.json.JSONObject

class AndroidNativeInterface(
    private val viewModel: SettingsViewModel
) :
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
        viewModel.setBrightness(value)
    }

    @NativeCall(CallType.FULL_SYNC)
    fun setScale(value: Int) {
        viewModel.setScale(Scale.fromInt(value))
    }

    @NativeCall(CallType.FULL_SYNC)
    fun setScaleType(value: Int) {
        viewModel.setScaleType(ScaleType.fromInt(value))
    }

    @NativeCall(CallType.FULL_SYNC)
    fun toggleUseScroll() {
        viewModel.toggleUseScroll()
    }

    @NativeCall(CallType.FULL_SYNC)
    fun setMapUpdateInterval(value: Int) {
        viewModel.setMapUpdateInterval(MapUpdateInterval.fromInt(value))
    }

    @NativeCall(CallType.FULL_SYNC)
    fun setWallpaper() {
        viewModel.onSetWallpaper()
    }
}