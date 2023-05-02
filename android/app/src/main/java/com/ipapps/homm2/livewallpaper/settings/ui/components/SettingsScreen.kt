package com.ipapps.homm2.livewallpaper.settings.ui.components;

import androidx.compose.foundation.layout.padding
import androidx.compose.material.*
import androidx.compose.runtime.*
import androidx.compose.runtime.livedata.observeAsState
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import com.ipapps.homm2.livewallpaper.settings.ui.theme.Theme
import com.ipapps.homm2.livewallpaper.R
import com.ipapps.homm2.livewallpaper.settings.data.MapUpdateInterval
import com.ipapps.homm2.livewallpaper.settings.data.Scale
import com.ipapps.homm2.livewallpaper.settings.data.ScaleType
import com.ipapps.homm2.livewallpaper.settings.data.SettingsViewModel
import com.ipapps.homm2.livewallpaper.settings.data.WallpaperPreferences
import com.ipapps.homm2.livewallpaper.settings.ui.components.settings.SettingsCategory
import com.ipapps.homm2.livewallpaper.settings.ui.components.settings.SettingsContainer
import com.ipapps.homm2.livewallpaper.settings.ui.components.settings.SettingsDropdown
import com.ipapps.homm2.livewallpaper.settings.ui.components.settings.SettingsDropdownItem
import com.ipapps.homm2.livewallpaper.settings.ui.components.settings.SettingsItem

@Composable
fun SetWallpaperFab(onClick: () -> Unit) {
    var isWallpaperLimitationVisible by remember { mutableStateOf(false) }

    if (isWallpaperLimitationVisible) {
        AlertDialog(
            title = { Text(stringResource(R.string.wallpaper_change_limitation_title)) },
            text = { Text(stringResource(R.string.wallpaper_change_limitation_text)) },
            confirmButton = {
                Button(onClick = {
                    isWallpaperLimitationVisible = false
                    onClick()
                }) { Text(stringResource(R.string.wallpaper_change_limitation_button)) }
            },
            onDismissRequest = { isWallpaperLimitationVisible = false }
        )
    }

    FloatingActionButton(
        backgroundColor = MaterialTheme.colors.primary,
        onClick = { isWallpaperLimitationVisible = true }
    ) {
        Text(
            modifier = Modifier.padding(horizontal = 24.dp),
            text = stringResource(R.string.wallpaper_change_button)
        )
    }
}

@Composable
fun SettingsScreen(
    viewModel: SettingsViewModel,
    actions: NavigationActions
) {
    val scaleOptions = listOf(
        SettingsDropdownItem(Scale.DPI, stringResource(R.string.scale_by_density)),
//        SettingsDropdownItem(Scale.X1, "x1"),
        SettingsDropdownItem(Scale.X2, "x2"),
        SettingsDropdownItem(Scale.X3, "x3"),
        SettingsDropdownItem(Scale.X4, "x4"),
        SettingsDropdownItem(Scale.X5, "x5"),
    )
    val mapUpdateIntervalOptions = listOf(
        SettingsDropdownItem(
            MapUpdateInterval.EVERY_SWITCH,
            stringResource(R.string.update_timeout_every_switch)
        ),
        SettingsDropdownItem(
            MapUpdateInterval.MINUTES_10,
            stringResource(R.string.update_timeout_10minutes)
        ),
        SettingsDropdownItem(
            MapUpdateInterval.MINUTES_30,
            stringResource(R.string.update_timeout_30minutes)
        ),
        SettingsDropdownItem(
            MapUpdateInterval.HOURS_2,
            stringResource(R.string.update_timeout_2hour)
        ),
        SettingsDropdownItem(
            MapUpdateInterval.HOURS_24,
            stringResource(R.string.update_timeout_24hours)
        ),
    )
    val scaleTypeOptions = listOf(
        SettingsDropdownItem(
            ScaleType.NEAREST,
            stringResource(R.string.scale_type_sharp_title)
        ),
        SettingsDropdownItem(
            ScaleType.LINEAR,
            stringResource(R.string.scale_type_smooth_title)
        )
    )

    val prefs by viewModel.settingsUiModel.observeAsState(WallpaperPreferences())
    var brightnessSliderValue by remember { mutableStateOf(prefs.brightness.toFloat()) }

    // FIXME rewrite setting of initial brightness value
    if (brightnessSliderValue == WallpaperPreferences.defaultBrightness.toFloat()
        && prefs.brightness != WallpaperPreferences.defaultBrightness
    ) {
        brightnessSliderValue = prefs.brightness.toFloat()
    }

    Theme {
        Scaffold(
            floatingActionButton = {
                SetWallpaperFab(onClick = { viewModel.onSetWallpaper() })
            },
        ) {
            SettingsContainer {
                item { SettingsCategory(text = stringResource(R.string.preferences_header)) }
                /*item {
                    SettingsItem(
                        title = stringResource(R.string.maps_item),
                        icon = { Icon(Icons.Filled.List, contentDescription = "Maps list") },
                        onClick = { actions.mapsList() }
                    )
                }*/
                item {
                    SettingsDropdown(
                        title = stringResource(R.string.scale_title),
                        subtitle = scaleOptions.find { it.value == prefs.scale }?.title.orEmpty(),
                        items = scaleOptions,
                        selectedItemValue = prefs.scale,
                        onItemSelected = { viewModel.setScale(it.value) },
                    )
                }
                item {
                    SettingsDropdown(
                        title = stringResource(R.string.scale_type_title),
                        subtitle = scaleTypeOptions.find { it.value == prefs.scaleType }?.title.orEmpty(),
                        items = scaleTypeOptions,
                        selectedItemValue = prefs.scaleType,
                        onItemSelected = { viewModel.setScaleType(it.value) },
                    )
                }
                item {
                    SettingsDropdown(
                        title = stringResource(R.string.update_time_title),
                        subtitle = mapUpdateIntervalOptions.find { it.value == prefs.mapUpdateInterval }?.title.orEmpty(),
                        items = mapUpdateIntervalOptions,
                        selectedItemValue = prefs.mapUpdateInterval,
                        onItemSelected = { viewModel.setMapUpdateInterval(it.value) },
                    )
                }
                /*item {
                     SettingsItem(
                         title = stringResource(R.string.use_scroll_title),
                         subtitle = stringResource(R.string.use_scroll_summary),
                         onClick = { viewModel.toggleUseScroll() },
                     ) { interactionSource ->
                         Switch(
                             checked = prefs.useScroll,
                             onCheckedChange = { viewModel.toggleUseScroll() },
                             interactionSource = interactionSource
                         )
                     }
                 }*/
                item {
                    SettingsItem(
                        title = stringResource(R.string.brightness_title),
                        nextLine = true,
                    ) {
                        Slider(
                            value = brightnessSliderValue,
                            valueRange = 0f..100f,
                            onValueChange = { brightnessSliderValue = it },
                            onValueChangeFinished = { viewModel.setBrightness(brightnessSliderValue.toInt()) }
                        )

                    }
                }
            }
        }
    }
}
