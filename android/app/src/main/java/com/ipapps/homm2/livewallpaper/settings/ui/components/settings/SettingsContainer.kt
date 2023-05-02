package com.ipapps.homm2.livewallpaper.settings.ui.components.settings;
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.LazyListScope
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.material.Scaffold
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier

@Composable
fun SettingsContainer(
    content: LazyListScope.() -> Unit
) {
    val columnState = rememberLazyListState()

    Scaffold { innerPadding ->
        Box {
            LazyColumn(
                Modifier.fillMaxSize(),
                state = columnState,
                contentPadding = innerPadding,
                content = content
            )
        }
    }
}