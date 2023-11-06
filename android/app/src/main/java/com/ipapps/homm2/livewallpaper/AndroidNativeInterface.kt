package com.ipapps.homm2.livewallpaper

import com.ipapps.homm2.livewallpaper.data.MapReader
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
import org.json.JSONObject
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
                val filesList = mapsFolder?.listFiles() ?: emptyArray<File>()

                val objectsList = filesList
                    .filter { it.extension.endsWith("mp2") }
                    .map {
                        val input = it.inputStream()
                        val mapHeader = kotlin.runCatching {
                            MapReader.readMap(input)
                        }.getOrNull()
                        input.close()

                        Pair(it, mapHeader)
                    }
                    .filter { it.second != null }
                    .map {
                        val file = it.first
                        val header = it.second

                        JSONObject()
                            .put("name", file.name)
                            .put("title", header?.title)
                            .put("width", header?.width)
                            .put("height", header?.height)
                            .put("isPoL", header?.isPoL)
                    }

                promise.resolve(JSONArray(objectsList).toString(2))
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