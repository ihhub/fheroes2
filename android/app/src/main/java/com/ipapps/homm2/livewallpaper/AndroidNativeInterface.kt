package com.ipapps.homm2.livewallpaper

import de.andycandy.android.bridge.CallType
import de.andycandy.android.bridge.DefaultJSInterface
import de.andycandy.android.bridge.NativeCall

class AndroidNativeInterface : DefaultJSInterface("Android") {
    @NativeCall(CallType.FULL_SYNC)
    fun helloFullSync(name: String): String {
        return "hello $name"
    }

    @NativeCall(CallType.WEB_PROMISE)
    fun helloWebPromise(name: String): String {
        return "hello $name"
    }

    @NativeCall(CallType.FULL_PROMISE)
    fun helloFullPromise(name: String) = doInBackground<String> { promise ->
        promise.resolve("hello $name")
    }
}