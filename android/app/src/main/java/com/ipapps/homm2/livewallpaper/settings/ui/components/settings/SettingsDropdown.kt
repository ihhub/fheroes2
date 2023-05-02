package com.ipapps.homm2.livewallpaper.settings.ui.components.settings;
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.material.DropdownMenu
import androidx.compose.material.DropdownMenuItem
import androidx.compose.material.MaterialTheme
import androidx.compose.material.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.DpOffset
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.launch

data class SettingsDropdownItem<T>(
    val value: T,
    val title: String
)

@Composable
fun <T> SettingsDropdown(
    icon: @Composable () -> Unit = {},
    title: String,
    subtitle: String,
    items: List<SettingsDropdownItem<T>>,
    selectedItemValue: T,
    onItemSelected: (SettingsDropdownItem<T>) -> Unit,
    enabled: Boolean = true,
) {
    var dropDownExpanded by remember { mutableStateOf(false) }
    val scope = rememberCoroutineScope()

    Box {
        SettingsItem(
            icon = icon,
            title = title,
            subtitle = subtitle,
            enabled = enabled,
            onClick = { dropDownExpanded = !dropDownExpanded }
        )
        DropdownMenu(
            expanded = dropDownExpanded,
            offset = DpOffset(24.dp, 0.dp),
            onDismissRequest = { dropDownExpanded = false },

            ) {
            items.forEach { item ->
                DropdownMenuItem(
                    onClick = {
                        dropDownExpanded = false
                        scope.launch {
//                            Actions.delay(100F)
                            onItemSelected(item)
                        }
                    },
                    Modifier.background(
                        if (selectedItemValue == item.value) MaterialTheme.colors.primary.copy(
                            alpha = 0.3f
                        ) else Color.Unspecified
                    )
                ) {
                    Text(item.title)
                }
            }
        }
    }
}