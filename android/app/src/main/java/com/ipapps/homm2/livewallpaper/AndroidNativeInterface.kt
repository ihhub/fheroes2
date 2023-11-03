package com.ipapps.homm2.livewallpaper

import de.andycandy.android.bridge.CallType
import de.andycandy.android.bridge.DefaultJSInterface
import de.andycandy.android.bridge.NativeCall
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking

class AndroidNativeInterface : DefaultJSInterface("Android") {
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
}