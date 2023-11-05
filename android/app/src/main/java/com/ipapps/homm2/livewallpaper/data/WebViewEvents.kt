package com.ipapps.homm2.livewallpaper.data

import android.webkit.WebView
import org.json.JSONArray
import org.json.JSONObject

open class WebViewEvent(type: String) {
    val obj: JSONObject = JSONObject().put("type", type)

    fun <T> setPayload(payload: T) {
        obj.put("payload", payload)
    }

    override fun toString(): String {
        return obj.toString(2);
    }
}

class WebViewSettingsEvent(preferences: WallpaperPreferences) : WebViewEvent("settings") {
    init {
        setPayload(
            JSONObject()
                .put("scale", preferences.scale.value)
                .put("scaleType", preferences.scaleType.value)
                .put("mapUpdateInterval", preferences.mapUpdateInterval.value)
                .put("useScroll", preferences.useScroll)
                .put("brightness", preferences.brightness)
        )
    }
}

class WebViewMapsListEvent() : WebViewEvent("maps-list") {
    init {
        setPayload(JSONArray())
    }
}

fun sendWebViewEvent(message: WebViewEvent, webView: WebView) {
    val jsonString = message.toString()

    webView.evaluateJavascript(
        """
                if (typeof window.dispatchWebViewEvent === 'function') {
                    window.dispatchWebViewEvent(${jsonString});
                } else {
                    console.error('no dispatchWebViewEvent', ${jsonString})
                }
        """
    ) { }
}