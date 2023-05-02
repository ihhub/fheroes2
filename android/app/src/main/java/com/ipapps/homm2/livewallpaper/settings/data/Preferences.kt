package com.ipapps.homm2.livewallpaper.settings.data;

enum class MapUpdateInterval(val value: Int) {
    EVERY_SWITCH(0),
    MINUTES_10(1),
    MINUTES_30(2),
    HOURS_2(3),
    HOURS_24(4);

    companion object {
        fun fromInt(value: Int?): MapUpdateInterval {
            return values()
                .runCatching { first { it.value == value } }
                .getOrDefault(WallpaperPreferences.defaultMapUpdateInterval)
        }
    }
}

enum class ScaleType {
    NEAREST,
    LINEAR;
}

enum class Scale(val value: Int) {
    DPI(0),
    X1(1),
    X2(2),
    X3(3),
    X4(4),
    X5(5);

    companion object {
        fun fromInt(value: Int?): Scale {
            return values()
                .runCatching { first { it.value == value } }
                .getOrDefault(WallpaperPreferences.defaultScale)
        }
    }
}

data class WallpaperPreferences(
    val scale: Scale = defaultScale,
    val scaleType: ScaleType = defaultScaleType,
    val mapUpdateInterval: MapUpdateInterval = defaultMapUpdateInterval,
    val useScroll: Boolean = defaultUseScroll,
    val brightness: Int = defaultBrightness,
) {
    companion object Defaults {
        val defaultScale = Scale.DPI
        val defaultScaleType = ScaleType.NEAREST
        val defaultMapUpdateInterval = MapUpdateInterval.MINUTES_10
        const val defaultUseScroll = true
        const val defaultBrightness = 60
    }
}
