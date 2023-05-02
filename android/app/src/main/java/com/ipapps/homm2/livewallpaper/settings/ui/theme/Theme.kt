package com.ipapps.homm2.livewallpaper.settings.ui.theme

import androidx.compose.material.MaterialTheme
import androidx.compose.runtime.Composable
import com.homm3.livewallpaper.android.ui.theme.Shapes
import com.homm3.livewallpaper.android.ui.theme.Typography

@Composable
fun Theme(content: @Composable () -> Unit) {
    MaterialTheme(
        typography = Typography,
        shapes = Shapes,
        content = content
    )
}