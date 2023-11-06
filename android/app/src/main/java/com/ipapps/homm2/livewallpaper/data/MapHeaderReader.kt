package com.ipapps.homm2.livewallpaper.data

import java.io.InputStream
import java.nio.ByteBuffer
import java.nio.ByteOrder

class MapHeader(val title: String, val width: Int, val height: Int)

class MapHeaderReader(val input: InputStream?) {
    companion object {
        private const val titleOffset = 0x3a
        private const val titleLength = 16

        private fun isCorrectMagicNumber(headerBuffer: ByteArray): Boolean {
            val magic = ByteArray(4);
            headerBuffer.copyInto(magic, 0, 0, 4)
            val magicInt = ByteBuffer.wrap(magic).order(ByteOrder.BIG_ENDIAN).int

            return magicInt == 0x5C000000;
        }

        private fun readMapName(headerBuffer: ByteArray): String {
            val mapName = ByteArray(titleLength)
            headerBuffer.copyInto(mapName, 0, titleOffset, titleOffset + titleLength)

            return mapName.toString(Charsets.UTF_8).split((0x00).toChar())[0]
        }

        fun readMapHeader(input: InputStream?): MapHeader {
            val headerBuffer = ByteArray(titleOffset + titleLength)
            input?.read(headerBuffer)

            if (!isCorrectMagicNumber(headerBuffer)) {
                throw Error("Unknown magic number")
            }

            val width = headerBuffer[4 + 2].toInt()
            val height = headerBuffer[4 + 3].toInt()

            return MapHeader(readMapName(headerBuffer), width, height)
        }

    }
}