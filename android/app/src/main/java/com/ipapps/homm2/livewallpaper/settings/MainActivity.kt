package com.ipapps.homm2.livewallpaper.settings;

import android.app.WallpaperManager
import android.content.ComponentName
import android.content.Intent
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.viewModels
import androidx.compose.runtime.LaunchedEffect
import com.ipapps.homm2.livewallpaper.settings.data.MapsViewModel
import com.ipapps.homm2.livewallpaper.settings.data.MapsViewModelFactory
import com.ipapps.homm2.livewallpaper.settings.data.ParsingViewModel
import com.ipapps.homm2.livewallpaper.settings.data.SettingsViewModel
import com.ipapps.homm2.livewallpaper.settings.data.WallpaperPreferencesRepository
import com.ipapps.homm2.livewallpaper.settings.ui.components.NavigationHost
import kotlinx.coroutines.flow.first
import org.libsdl.app.SDLActivity

class MainActivity() : ComponentActivity() {
    private fun setWallpaper() {
        startActivity(
            Intent()
                .setAction(WallpaperManager.ACTION_CHANGE_LIVE_WALLPAPER)
                .putExtra(
                    WallpaperManager.EXTRA_LIVE_WALLPAPER_COMPONENT,
                    ComponentName(
                        applicationContext,
                        SDLActivity::class.java
                    )
                )
        )
    }

    private fun openIconAuthorUrl() {
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val mapsViewModel: MapsViewModel by viewModels {
            MapsViewModelFactory(contentResolver, filesDir)
        }

        val config =
            getExternalFilesDir(null)?.resolve("fheroes2.cfg")

        val settingsViewModel = SettingsViewModel(
            WallpaperPreferencesRepository(config),
            setWallpaper = ::setWallpaper,
            openIconAuthorUrl = ::openIconAuthorUrl
        )
        val parsingViewModel = ParsingViewModel(application)

        setContent {
            LaunchedEffect(true) {
                if (mapsViewModel.mapsList.first().isEmpty()) {
                    parsingViewModel.copyDefaultMap()
                }
            }

            NavigationHost(
                mapViewModel = mapsViewModel,
                settingsViewModel = settingsViewModel,
                parsingViewModel = parsingViewModel,
            )
        }
    }
}

