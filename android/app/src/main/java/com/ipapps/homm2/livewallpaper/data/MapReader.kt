package com.ipapps.homm2.livewallpaper.data

import java.io.ByteArrayInputStream
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

            val uTilesBuffer = tilesBuffer.asUByteArray()
            println("First bytes: ${uTilesBuffer[0].toString(16)} ${uTilesBuffer[1].toString(16)}")
            val firstInt = ByteArray(2)
            firstInt[0] = uTilesBuffer[0].toByte()
            firstInt[1] = uTilesBuffer[1].toByte()
            val fInt = ByteBuffer.wrap(firstInt.reversedArray()).short
            println("First int $fInt")

            val isPoL = isPriceOfLoyalty(tilesBuffer)


            return MapHeader(readMapName(headerBuffer), width, height, isPoL)
        }

        private val polMapObjectIndexes = listOf(
            "247", // MP2::OBJ_BARRIER
            "249", // MP2::OBJ_EXPANSION_DWELLING
            "250", // MP2::OBJ_EXPANSION_OBJECT
            "251", // MP2::OBJ_JAIL
            "248", // MP2::OBJ_TRAVELLER_TENT
        )

        class MapTile {
            var terrainImageIndex = ByteArray(2) // stream.getLE16()
            var objectName1 = ByteArray(1) // stream.get()
            var bottomIcnImageIndex = ByteArray(1) // stream.get()
            var quantity1 = ByteArray(1) // stream.get()
            var quantity2 = ByteArray(1) // stream.get()
            var objectName2 = ByteArray(1) // stream.get()
            var topIcnImageIndex = ByteArray(1) // stream.get()
            var terrainFlags = ByteArray(1) // stream.get()
            var mapObjectType = ByteArray(1) // stream.get()
            var nextAddonIndex = ByteArray(2) // stream.getLE16()
            var level1ObjectUID = ByteArray(4) // stream.getLE32()
            var level2ObjectUID = ByteArray(4) // stream.getLE32()
        }

        private fun isPoLTile(input: InputStream, tileIndex: Int): Boolean {
//            val offset = (tileIndex * tileSize) + 10
//            val mapObjectType: UByte = tilesBuffer[offset]

            val tile = MapTile()
            input.read(tile.terrainImageIndex);
            input.read(tile.objectName1);
            input.read(tile.bottomIcnImageIndex);
            input.read(tile.quantity1);
            input.read(tile.quantity2);
            input.read(tile.objectName2);
            input.read(tile.topIcnImageIndex);
            input.read(tile.terrainFlags);
            input.read(tile.mapObjectType);
            input.read(tile.nextAddonIndex);
            input.read(tile.level1ObjectUID);
            input.read(tile.level2ObjectUID);

            val mapObjectType = ByteBuffer.wrap(tile.mapObjectType.reversedArray())[0].toUByte()

            return polMapObjectIndexes.contains(mapObjectType.toString())
        }

        private fun isPriceOfLoyalty(tilesBuffer: ByteArray): Boolean {
            var tileIndex = 0;

            val input = ByteArrayInputStream(tilesBuffer)

            while (tileIndex < tilesBuffer.size / tileSize) {
                if (isPoLTile(input, tileIndex)) {
                    return true
                }

                tileIndex += 1
            }

            return false
        }
    }
}