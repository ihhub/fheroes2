package com.ipapps.homm2.livewallpaper.settings.ui.components;

import android.widget.Toast
import androidx.compose.foundation.lazy.items
import androidx.compose.material.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Delete
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import com.ipapps.homm2.livewallpaper.settings.ui.theme.Theme
import com.ipapps.homm2.livewallpaper.settings.data.MapReadingException
import com.ipapps.homm2.livewallpaper.settings.data.MapsViewModel
import com.ipapps.homm2.livewallpaper.settings.ui.components.settings.SettingsCategory
import com.ipapps.homm2.livewallpaper.settings.ui.components.settings.SettingsContainer
import com.ipapps.homm2.livewallpaper.R
@Composable
fun MapLoadingErrorAlert(error: MapReadingException?, onClose: () -> Unit) {
    if (error !== null) {
        AlertDialog(
            title = {
                Text(
                    when (error) {
                        MapReadingException.CantParseMap -> stringResource(R.string.maps_error_parse_title)
                        MapReadingException.CantOpenStream -> stringResource(R.string.maps_error_open_title)
                        MapReadingException.CantCopyMap -> stringResource(R.string.maps_error_copy_title)
                    }
                )
            },
            text = {
                Text(
                    when (error) {
                        MapReadingException.CantParseMap -> stringResource(R.string.maps_error_parse_text)
                        MapReadingException.CantOpenStream -> stringResource(R.string.maps_error_open_title)
                        MapReadingException.CantCopyMap -> stringResource(R.string.maps_error_copy_title)
                    }
                )
            },
            confirmButton = {
                Button(onClick = { onClose() }) {
                    Text(
                        "Error"
//                        stringResource(R.string.maps_error_button)
                    )
                }
            },
            onDismissRequest = { onClose() }
        )
    }
}

@OptIn(ExperimentalMaterialApi::class)
@Composable
fun MapsScreen(viewModel: MapsViewModel) {
    val context = LocalContext.current
    val filesSelector = createFileSelector { file -> viewModel.copyMap(file) }
    val files by viewModel.mapsList.collectAsState()
    val readingError by viewModel.mapReadingError.collectAsState()
    val isAddEnabled = files.size < 5
    val isRemoveEnabled = files.size > 1

    LaunchedEffect(true) {
        viewModel.updateFilesList()
    }

    Theme {
        Scaffold(
            floatingActionButton = {
                Fab(
                    disabled = !isAddEnabled,
                    onClick = {
                        if (isAddEnabled) {
                            filesSelector()
                        } else {
                            Toast
                                .makeText(
                                    context,
                                    "context.getText(R.string.maps_toast_too_much_maps)",
                                    Toast.LENGTH_SHORT
                                )
                                .show()
                        }
                    })
            },
        ) {
            MapLoadingErrorAlert(error = readingError, onClose = { viewModel.resetCopyMapError() })

            SettingsContainer {
                item { SettingsCategory(text = stringResource(R.string.maps_title)) }

                items(files, key = { it.name }) {
                    ListItem(
                        text = { Text(it.name) },
                        trailing = {
                            IconButton(
                                enabled = isRemoveEnabled,
                                onClick = { viewModel.removeMap(it.name) }) {
                                Icon(
                                    Icons.Default.Delete,
                                    contentDescription = "Delete map",
                                )
                            }

                        }
                    )
                }
            }
        }
    }
}