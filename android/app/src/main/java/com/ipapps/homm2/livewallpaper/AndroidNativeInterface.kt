package com.ipapps.homm2.livewallpaper

import com.ipapps.homm2.livewallpaper.data.MapReader
import com.ipapps.homm2.livewallpaper.data.MapUpdateInterval
import com.ipapps.homm2.livewallpaper.data.MapsViewModel
import com.ipapps.homm2.livewallpaper.data.Scale
import com.ipapps.homm2.livewallpaper.data.ScaleType
import com.ipapps.homm2.livewallpaper.data.SettingsViewModel
import de.andycandy.android.bridge.CallType
import de.andycandy.android.bridge.DefaultJSInterface
import de.andycandy.android.bridge.NativeCall
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking
import org.json.JSONArray
import org.json.JSONObject
import java.io.File

class AndroidNativeInterface(
    private val settingsViewModel: SettingsViewModel,
    private val mapsViewModel: MapsViewModel
) : DefaultJSInterface("Android") {

    @NativeCall(CallType.FULL_PROMISE)
    fun getMapsList() = doInBackground { promise ->
        runBlocking {
            launch {
                promise.resolve("[]")
            }
        }
    }

    @NativeCall(CallType.FULL_PROMISE)
    fun deleteMap(name: String) = doInBackground { promise ->
        runBlocking {
            launch {
                mapsViewModel.deleteMap(name)
                promise.resolve("")
            }
        }
    }

    @NativeCall(CallType.FULL_SYNC)
    fun setBrightness(value: Int) {
        settingsViewModel.setBrightness(value)
    }

    @NativeCall(CallType.FULL_SYNC)
    fun setScale(value: Int) {
        settingsViewModel.setScale(Scale.fromInt(value))
    }

    @NativeCall(CallType.FULL_SYNC)
    fun setScaleType(value: Int) {
        settingsViewModel.setScaleType(ScaleType.fromInt(value))
    }

    @NativeCall(CallType.FULL_SYNC)
    fun toggleUseScroll() {
        settingsViewModel.toggleUseScroll()
    }

    @NativeCall(CallType.FULL_SYNC)
    fun setMapUpdateInterval(value: Int) {
        settingsViewModel.setMapUpdateInterval(MapUpdateInterval.fromInt(value))
    }

    @NativeCall(CallType.FULL_SYNC)
    fun setWallpaper() {
        settingsViewModel.onSetWallpaper()
    }
}