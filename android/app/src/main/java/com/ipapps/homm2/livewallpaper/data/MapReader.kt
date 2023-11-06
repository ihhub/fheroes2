package com.ipapps.homm2.livewallpaper.data

import java.io.InputStream
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.charset.Charset

class MapHeader(val title: String, val width: Int, val height: Int, val isPoL: Boolean)

class MapReader(val input: InputStream?) {
    companion object {
        private const val titleOffset = 0x3a
        private const val titleLength = 16
        private const val headerSize = 428
        private const val tileSize = 20

        private fun isCorrectMagicNumber(headerBuffer: ByteArray): Boolean {
            val magic = ByteArray(4);
            headerBuffer.copyInto(magic, 0, 0, 4)
            val magicInt = ByteBuffer.wrap(magic).order(ByteOrder.BIG_ENDIAN).int

            return magicInt == 0x5C000000
        }

        private fun readMapName(headerBuffer: ByteArray): String {
            val mapName = ByteArray(titleLength)
            headerBuffer.copyInto(mapName, 0, titleOffset, titleOffset + titleLength)

            // FIXME some international codepage
            return mapName.toString(Charset.forName("windows-1251")).split((0x00).toChar())[0]
        }

        fun readMap(input: InputStream?): MapHeader {
            val headerBuffer = ByteArray(titleOffset + titleLength)
            input?.read(headerBuffer)

            if (!isCorrectMagicNumber(headerBuffer)) {
                throw Error("Unknown magic number")
            }

            val width = headerBuffer[4 + 2].toUByte().toInt()
            val height = headerBuffer[4 + 3].toUByte().toInt()

            val offset = headerSize - headerBuffer.size
            input?.skip(offset.toLong())
            val tilesBuffer = ByteArray(width * height * tileSize)
            input?.read(tilesBuffer)
            val isPoL = isPriceOfLoyalty(tilesBuffer)

            return MapHeader(readMapName(headerBuffer), width, height, isPoL)
        }

        private val polMapObjectIndexes = listOf(
            247, // MP2::OBJ_BARRIER
            249, // MP2::OBJ_EXPANSION_DWELLING
            250, // MP2::OBJ_EXPANSION_OBJECT
            251, // MP2::OBJ_JAIL
            248, // MP2::OBJ_TRAVELLER_TENT
        )

        private fun isPoLTile(tilesBuffer: ByteArray, tileIndex: Int): Boolean {
            val offset = (tileIndex * tileSize) + 9
            val mapObjectType = tilesBuffer[offset].toInt() and 0xFF

            return polMapObjectIndexes.contains(mapObjectType)
        }

        private fun isPriceOfLoyalty(tilesBuffer: ByteArray): Boolean {
            var tileIndex = 0;

            while (tileIndex < tilesBuffer.size / tileSize) {
                if (isPoLTile(tilesBuffer, tileIndex)) {
                    return true
                }

                tileIndex += 1
            }

            return false
        }
    }
}