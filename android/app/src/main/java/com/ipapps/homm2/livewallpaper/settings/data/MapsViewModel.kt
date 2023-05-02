package com.ipapps.homm2.livewallpaper.settings.data;

import android.content.ContentResolver
import android.net.Uri
import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.viewModelScope
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.launch
import java.io.File
import java.io.FileOutputStream
import java.io.InputStream

sealed class MapReadingException : Throwable() {
    object CantOpenStream : MapReadingException()
    object CantParseMap : MapReadingException()
    object CantCopyMap : MapReadingException()
}

class MapsViewModel(private val contentResolver: ContentResolver, root: File) : ViewModel() {
    private val mapsFolder = root.resolve("user-maps")

    private val _mapsListState = MutableStateFlow(emptyList<File>())
    private val _mapReadingErrorState = MutableStateFlow<MapReadingException?>(null)
    val mapsList: StateFlow<List<File>> = _mapsListState
    val mapReadingError: StateFlow<MapReadingException?> = _mapReadingErrorState

    init {
        if (!mapsFolder.exists()) {
            mapsFolder.mkdir()
        }

        viewModelScope.launch { updateFilesList() }
    }

    suspend fun updateFilesList() {
        val files = mapsFolder.listFiles()

        if (files !== null) {
            _mapsListState.emit(files.toList())
        }
    }

    private fun copyMapToFile(uri: Uri, filename: String) {
        try {
            val stream = openStream(uri)
            stream.copyTo(FileOutputStream(mapsFolder.resolve(filename)))
            stream.close()
        } catch (ex: Exception) {
            throw MapReadingException.CantCopyMap
        }
    }

    private fun openStream(uri: Uri): InputStream {
        try {
            val stream = contentResolver.openInputStream(uri)

            if (stream === null) {
                throw MapReadingException.CantOpenStream
            }

            return stream
        } catch (ex: Exception) {
            throw MapReadingException.CantOpenStream
        }
    }

    private fun parseMap(uri: Uri): String {
//        try {
//            val stream = openStream(uri)
//            val h3m = H3mReader(stream).read()
//            stream.close()
//
//            val fileName = uri.normalizeScheme().path
//                ?.split("/")
//                ?.last()
//                ?.split(".")
//                ?.dropLast(1)
//                ?.joinToString(".")
//
//            return fileName ?: h3m.header.title
//        } catch (ex: Exception) {
//            throw MapReadingException.CantParseMap
//        }

        return "parseMap";
    }

    fun resetCopyMapError() {
        viewModelScope.launch {
            _mapReadingErrorState.emit(null)
        }
    }

    fun copyMap(uri: Uri) {
        viewModelScope.launch {
            try {
                val filename = parseMap(uri)
                copyMapToFile(uri, "${filename}.h3m")

                updateFilesList()
            } catch (ex: MapReadingException) {
                _mapReadingErrorState.emit(ex)
            }
        }
    }

    fun removeMap(name: String) {
        viewModelScope.launch {
            mapsFolder.resolve(name).delete()

            updateFilesList()
        }
    }
}


class MapsViewModelFactory(private val contentResolver: ContentResolver, private val root: File) :
    ViewModelProvider.Factory {
    override fun <T : ViewModel> create(modelClass: Class<T>): T {
        if (modelClass.isAssignableFrom(MapsViewModel::class.java)) {
            @Suppress("UNCHECKED_CAST")
            return MapsViewModel(contentResolver, root) as T
        }

        throw IllegalArgumentException("Unknown ViewModel class")
    }
}

