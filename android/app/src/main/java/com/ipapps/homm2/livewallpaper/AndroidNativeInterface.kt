package com.ipapps.homm2.livewallpaper

import com.ipapps.homm2.livewallpaper.data.MapUpdateInterval
import com.ipapps.homm2.livewallpaper.data.Scale
import com.ipapps.homm2.livewallpaper.data.ScaleType
import com.ipapps.homm2.livewallpaper.data.SettingsViewModel
import de.andycandy.android.bridge.CallType
import de.andycandy.android.bridge.DefaultJSInterface
import de.andycandy.android.bridge.NativeCall
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking
import org.json.JSONArray
import java.io.File

class AndroidNativeInterface(
    private val viewModel: SettingsViewModel, private val mapsFolder: File?
) : DefaultJSInterface("Android") {

    @NativeCall(CallType.FULL_PROMISE)
    fun helloFullPromise(name: String) = doInBackground { promise ->
        runBlocking {
            launch {
                delay(5000L)
                promise.resolve("helloFullPromise return: $name")
            }
        }
    }

    @NativeCall(CallType.FULL_PROMISE)
    fun getMapsList() = doInBackground { promise ->
        runBlocking {
            launch {
                if (mapsFolder == null) {
                    promise.resolve("[]")
                } else {
                    val list = mapsFolder.list { _, filename -> filename.endsWith(".mp2") }

                    promise.resolve(JSONArray(list).toString(2))
                }
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