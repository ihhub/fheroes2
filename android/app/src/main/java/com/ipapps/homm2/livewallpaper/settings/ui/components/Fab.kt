package com.ipapps.homm2.livewallpaper.settings.ui.components;

import androidx.compose.material.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.ripple.LocalRippleTheme
import androidx.compose.material.ripple.RippleAlpha
import androidx.compose.material.ripple.RippleTheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.CompositionLocalProvider
import androidx.compose.ui.graphics.Color

private object NoRippleTheme : RippleTheme {
    @Composable
    override fun defaultColor() = Color.Unspecified

    @Composable
    override fun rippleAlpha(): RippleAlpha = RippleAlpha(0.0f, 0.0f, 0.0f, 0.0f)
}

@Composable
fun Fab(disabled: Boolean = true, onClick: () -> Unit) {
    CompositionLocalProvider(
        LocalRippleTheme provides
            if (disabled) NoRippleTheme else LocalRippleTheme.current
    ) {
        FloatingActionButton(
            backgroundColor = if (disabled) Color.Gray else MaterialTheme.colors.secondary,
            onClick = { onClick() },
        ) {
            Icon(
                tint = if (disabled) {
                    Color.DarkGray
                } else {
                    LocalContentColor.current.copy(alpha = LocalContentAlpha.current)
                },
                imageVector = Icons.Default.Add, contentDescription = "add map"
            )
        }
    }
}
