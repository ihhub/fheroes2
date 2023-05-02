package com.ipapps.homm2.livewallpaper.settings.ui.components;

import androidx.compose.foundation.layout.*
import androidx.compose.material.AlertDialog
import androidx.compose.material.Button
import androidx.compose.material.CircularProgressIndicator
import androidx.compose.material.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.AnnotatedString
import androidx.compose.ui.text.SpanStyle
import androidx.compose.ui.text.buildAnnotatedString
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import com.ipapps.homm2.livewallpaper.settings.ui.theme.Theme
import com.ipapps.homm2.livewallpaper.settings.data.ParsingState
import com.ipapps.homm2.livewallpaper.settings.data.ParsingViewModel
import com.ipapps.homm2.livewallpaper.R

@Composable
inline fun <R : Any> AnnotatedString.Builder.boldText(
    crossinline block: AnnotatedString.Builder.() -> R
): R {
    val index = pushStyle(SpanStyle(fontWeight = FontWeight.Bold))
    return try {
        block(this)
    } finally {
        pop(index)
    }
}

@Composable
fun ParsingScreen(viewModel: ParsingViewModel, actions: NavigationActions) {
    val openFileSelector = createFileSelector { viewModel.parseFile(it) }
    val parseStatus = viewModel.parsingStateUiModel

    Theme {
        Column(
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.Center,
            modifier = Modifier
                .padding(24.dp)
                .fillMaxHeight()
                .fillMaxWidth()
        ) {
            Text(
                textAlign = TextAlign.Center,
                modifier = Modifier.padding(vertical = 8.dp),
                text = stringResource(R.string.parsing_upload)
            )

            Text(
                textAlign = TextAlign.Center,
                modifier = Modifier.padding(vertical = 8.dp),
                text = stringResource(R.string.parsing_restictions)
            )

            Text(
                textAlign = TextAlign.Center,
                modifier = Modifier.padding(vertical = 8.dp),
                text = buildAnnotatedString {
                    append(stringResource(R.string.parsing_how_to_upload_1))
                    append(" ")
                    append(stringResource(R.string.parsing_how_to_upload_2))
                    append(" ")
                    append(stringResource(R.string.parsing_how_to_upload_3))
                    append(" ")
                    append(stringResource(R.string.parsing_how_to_upload_4))
                    append("\n")
                    append(stringResource(R.string.parsing_how_to_upload_5))
                }
            )

            Text(
                textAlign = TextAlign.Center,
                modifier = Modifier
                    .padding(vertical = 8.dp)
                    .alpha(0.6f),
                text = stringResource(R.string.parsing_note)
            )

            when (parseStatus) {
                ParsingState.Done -> {
                    LaunchedEffect(key1 = "done", block = { actions.settings() })
                }
                ParsingState.Error -> {
                    AlertDialog(
                        title = { stringResource(R.string.parsing_error_cant_parse_header) },
                        text = {
                            Text(stringResource(R.string.parsing_error_cant_parse_text))
                        },
                        confirmButton = {
                            Button(onClick = { viewModel.clearParsingError() }) {
                                Text(stringResource(R.string.parsing_error_cant_parse_button))
                            }
                        },
                        onDismissRequest = { viewModel.clearParsingError() }
                    )
                }
                else -> {}
            }

            when (parseStatus) {
                ParsingState.Initial -> {
                    Button(
                        modifier = Modifier
                            .padding(vertical = 8.dp)
                            .defaultMinSize(minWidth = 40.dp),
                        onClick = { openFileSelector() }) {
                        Text(
                            textAlign = TextAlign.Center,
                            text = stringResource(R.string.parsing_button)
                        )
                    }
                }
                ParsingState.InProgress -> {
                    CircularProgressIndicator(
                        modifier = Modifier
                            .padding(vertical = 8.dp)
                            .size(32.dp),
                        strokeWidth = 4.dp
                    )
                }
                else -> {}
            }
        }
    }
}