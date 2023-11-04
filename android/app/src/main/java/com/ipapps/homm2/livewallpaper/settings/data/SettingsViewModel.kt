package com.ipapps.homm2.livewallpaper.settings.data;

import androidx.lifecycle.ViewModel
import androidx.lifecycle.asLiveData
import androidx.lifecycle.viewModelScope
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.flow.launchIn
import kotlinx.coroutines.flow.onEach
import kotlinx.coroutines.launch
import org.json.JSONObject

class SettingsViewModel(
    private val wallpaperPreferencesRepository: WallpaperPreferencesRepository,
    private val setWallpaper: () -> Unit,
    private val openIconAuthorUrl: () -> Unit
) : ViewModel() {
    val settingsUiModel = wallpaperPreferencesRepository.preferencesFlow.asLiveData()

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

    fun setScaleTypeString(value: String) {
        viewModelScope.launch {
            wallpaperPreferencesRepository.setScaleTypeString(value)
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

    fun onOpenIconAuthorUrl() {
        openIconAuthorUrl()
    }

    fun subscribeToPreferences(callback: (it: WallpaperPreferences) -> Unit) {
        wallpaperPreferencesRepository
            .preferencesFlow
            .onEach { callback(it) }
            .launchIn(scope = CoroutineScope(SupervisorJob() + Dispatchers.Main))
    }
}
