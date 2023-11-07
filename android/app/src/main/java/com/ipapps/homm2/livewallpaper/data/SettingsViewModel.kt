package com.ipapps.homm2.livewallpaper.data

import androidx.lifecycle.ViewModel
import androidx.lifecycle.asLiveData
import androidx.lifecycle.viewModelScope
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.flow.launchIn
import kotlinx.coroutines.flow.onEach
import kotlinx.coroutines.launch

class SettingsViewModel(
    private val wallpaperPreferencesRepository: WallpaperPreferencesRepository,
    private val setWallpaper: () -> Unit,
) : ViewModel() {
    fun toggleUseScroll() {
        viewModelScope.launch {
            wallpaperPreferencesRepository.toggleUseScroll()
        }
    }

    fun setScale(value: Scale) {
        viewModelScope.launch {
            wallpaperPreferencesRepository.setScale(value)
        }
    }

    fun setScaleType(value: ScaleType) {
        viewModelScope.launch {
            wallpaperPreferencesRepository.setScaleType(value)
        }
    }

    fun setMapUpdateInterval(value: MapUpdateInterval) {
        viewModelScope.launch {
            wallpaperPreferencesRepository.setMapUpdateInterval(value)
        }
    }

    fun setBrightness(value: Int) {
        viewModelScope.launch {
            wallpaperPreferencesRepository.setBrightness(value)
        }
    }

    fun onSetWallpaper() {
        setWallpaper()
    }

    fun subscribeToPreferences(callback: (it: WallpaperPreferences) -> Unit) {
        wallpaperPreferencesRepository
            .preferencesFlow
            .onEach { callback(it) }
            .launchIn(viewModelScope)
    }
}
