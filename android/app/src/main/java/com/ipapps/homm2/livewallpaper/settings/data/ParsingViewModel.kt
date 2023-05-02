package com.ipapps.homm2.livewallpaper.settings.data;

import android.app.Application
import android.net.Uri
import android.provider.SyncStateContract
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.lifecycle.ViewModel
import java.io.File
import java.io.InputStream
import kotlin.concurrent.thread

sealed class ParsingState {
    object Initial : ParsingState()
    object InProgress : ParsingState()
    object Done : ParsingState()
    object Error : ParsingState()
}

class ParsingViewModel(
    private val application: Application
) : ViewModel() {
    var parsingStateUiModel by mutableStateOf<ParsingState>(ParsingState.Initial)
    var parsingError by mutableStateOf<Exception?>(null)

    fun clearParsingError() {
        parsingStateUiModel = ParsingState.Initial
    }

    fun isGameAssetsAvailable(): Boolean {
        return true;
    }

    private fun prepareInputStream(uri: Uri): InputStream {
        return application
            .applicationContext
            .contentResolver
            .openInputStream(uri) ?: throw Exception("Can't open selected file")
    }

    private fun prepareOutputFile(): File {
        return application
                .applicationContext
                .filesDir
                .resolve("SyncStateContract.Constants.Assets.ATLAS_FOLDER")
    }

    fun copyDefaultMap() {
        val userMapsFolder = application
            .applicationContext
            .filesDir
            .resolve("USER_MAPS_FOLDER")

        userMapsFolder.mkdir()

        application
            .assets
            .list("maps")
            ?.forEach {
                application
                    .assets
                    .open("maps/${it}")
                    .copyTo(userMapsFolder.resolve(it).outputStream())
            }
    }

    fun parseFile(file: Uri) {

    }
}
