/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2026                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

package org.fheroes2;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;
import java.util.zip.DataFormatException;
import java.util.zip.Inflater;

/**
 * Extracts HoMM2 game assets from GOG Inno Setup installers (v5.5.0+ / v5.6.2).
 * Pure Java implementation — no native dependencies required.
 *
 * References:
 *   https://github.com/dscharrer/innoextract (by Daniel Scharrer, zlib license)
 */
final class InnoExtract
{
    private static final byte[] LOADER_MAGIC = { 0x72, 0x44, 0x6C, 0x50, 0x74, 0x53, (byte) 0xCD, (byte) 0xE6, (byte) 0xD7, 0x7B, 0x0B, 0x2A };

    private static final Set<String> TARGET_DIRS = new HashSet<>();

    private static final String GOG_CD_IMAGE_NAME = "homm2.gog";

    static {
        TARGET_DIRS.add( "data" );
        TARGET_DIRS.add( "maps" );
        TARGET_DIRS.add( "music" );
        TARGET_DIRS.add( "anim" );
        TARGET_DIRS.add( "anim2" );
    }

    private InnoExtract()
    {
        throw new IllegalStateException( "Instantiation is not allowed" );
    }

    /**
     * Checks whether the given file looks like a valid Inno Setup installer
     * that contains HoMM2 data by scanning for the loader magic signature.
     */
    static boolean isInnoSetupInstaller( final File file )
    {
        try ( final RandomAccessFile raf = new RandomAccessFile( file, "r" ) ) {
            return findLoaderMagic( raf ) >= 0;
        }
        catch ( final IOException e ) {
            return false;
        }
    }

    /**
     * Checks whether the given InputStream starts with the PE/MZ header
     * typically found in Inno Setup EXE installers. Does not consume the stream
     * if the underlying implementation supports mark/reset.
     */
    static boolean isInnoSetupInstaller( final InputStream inputStream )
    {
        try {
            if ( inputStream.markSupported() ) {
                inputStream.mark( 2 );
            }

            final byte[] header = new byte[2];
            final int read = inputStream.read( header );

            if ( inputStream.markSupported() ) {
                inputStream.reset();
            }

            return read == 2 && header[0] == 'M' && header[1] == 'Z';
        }
        catch ( final IOException e ) {
            return false;
        }
    }

    /**
     * Extracts HoMM2 assets from a GOG Inno Setup EXE installer into the given output directory.
     *
     * @param installerFile   the Inno Setup EXE file
     * @param outputDir       the directory to extract assets into (e.g. externalFilesDir)
     * @return true if at least one asset was extracted
     * @throws IOException if an I/O error occurs
     */
    static boolean extractAssets( final File installerFile, final File cacheDir, final File outputDir ) throws IOException
    {
        try ( final RandomAccessFile raf = new RandomAccessFile( installerFile, "r" ) ) {
            return extractAssetsInternal( raf, cacheDir, outputDir );
        }
    }

    /**
     * Extracts HoMM2 assets from a GOG Inno Setup EXE provided as an InputStream.
     * The stream is first cached to a temporary file to allow random access.
     *
     * @param inputStream     the InputStream containing the EXE data
     * @param cacheDir        a directory for the temporary file
     * @param outputDir       the directory to extract assets into
     * @return true if at least one asset was extracted
     * @throws IOException if an I/O error occurs
     */
    static boolean extractAssets( final InputStream inputStream, final File cacheDir, final File outputDir ) throws IOException
    {
        final File tempFile = new File( cacheDir, "gog_installer.tmp" );

        try {
            Files.createDirectories( cacheDir.toPath() );

            try ( final OutputStream out = Files.newOutputStream( tempFile.toPath() ) ) {
                final byte[] buffer = new byte[65536];
                int bytesRead;

                while ( ( bytesRead = inputStream.read( buffer ) ) != -1 ) {
                    out.write( buffer, 0, bytesRead );
                }
            }

            return extractAssets( tempFile, cacheDir, outputDir );
        }
        finally {
            Files.deleteIfExists( tempFile.toPath() );
        }
    }

    // ---- Internal implementation ----

    private static boolean extractAssetsInternal( final RandomAccessFile raf, final File cacheDir, final File outputDir ) throws IOException
    {
        final long loaderOff = findLoaderMagic( raf );
        if ( loaderOff < 0 ) {
            throw new IOException( "Not an Inno Setup installer: loader magic not found" );
        }

        raf.seek( loaderOff + 32 );
        final long headerOffset = readUint32( raf );
        final long dataOffset = readUint32( raf );

        // Read version string (64 bytes at headerOffset)
        raf.seek( headerOffset );
        final byte[] versionBytes = new byte[64];
        raf.readFully( versionBytes );
        final String version = new String( versionBytes, 0, indexOf( versionBytes, (byte) 0 ), StandardCharsets.US_ASCII );

        if ( !version.contains( "Inno Setup" ) ) {
            throw new IOException( "Not an Inno Setup installer: unexpected version string: " + version );
        }

        // Block1 (file entries): CRC(4) + stored_size(4) + compressed(1) + data
        final long block1Start = headerOffset + 64;
        raf.seek( block1Start + 4 ); // skip CRC
        final long block1StoredSize = readUint32( raf );
        final int block1Compressed = raf.readUnsignedByte();
        final byte[] block1Raw = new byte[(int) block1StoredSize];
        raf.readFully( block1Raw );
        byte[] block1 = stripBlockCrc( block1Raw );
        if ( block1Compressed != 0 ) {
            block1 = lzmaDecompress( block1 );
        }

        // Block2 (data entries): CRC(4) + stored_size(4) + compressed(1) + data
        final long block2Start = block1Start + 9 + block1StoredSize;
        raf.seek( block2Start + 4 ); // skip CRC
        final long block2StoredSize = readUint32( raf );
        final int block2Compressed = raf.readUnsignedByte();
        final byte[] block2Raw = new byte[(int) block2StoredSize];
        raf.readFully( block2Raw );
        byte[] block2 = stripBlockCrc( block2Raw );
        if ( block2Compressed != 0 ) {
            block2 = lzmaDecompress( block2 );
        }

        // Parse data entries (74 bytes each)
        final int numEntries = block2.length / 74;
        final DataEntry[] dataEntries = new DataEntry[numEntries];
        for ( int i = 0; i < numEntries; i++ ) {
            dataEntries[i] = DataEntry.parse( block2, i * 74 );
        }

        // Find file mappings from block1
        Map<String, FileInfo> fileMap = scanBeforeInstallFiles( block1 );
        if ( fileMap.isEmpty() ) {
            fileMap = scanDestinationFiles( block1 );
        }

        if ( fileMap.isEmpty() ) {
            return false;
        }

        // Extract each target file
        boolean result = false;

        final Set<File> allowedSubdirs = new HashSet<>();
        for ( final String dirName : TARGET_DIRS ) {
            allowedSubdirs.add( new File( outputDir, dirName ).getCanonicalFile() );
        }

        FileInfo gogCdImageInfo = null;

        for ( final FileInfo fileInfo : fileMap.values() ) {
            final String path = fileInfo.path.replace( '\\', '/' );
            final String[] pathParts = path.split( "/" );

            // Check for GOG CD image (homm2.gog) which contains ANIM files
            final String fileName = pathParts[pathParts.length - 1].toLowerCase( Locale.ROOT );
            if ( fileName.equals( GOG_CD_IMAGE_NAME ) ) {
                gogCdImageInfo = fileInfo;
                continue;
            }

            if ( pathParts.length < 2 ) {
                continue;
            }

            if ( !TARGET_DIRS.contains( pathParts[0].toLowerCase( Locale.ROOT ) ) ) {
                continue;
            }

            // Construct output path with lowercase directory name
            final String assetSubpath = path.toLowerCase( Locale.ROOT );
            final File outFile = new File( outputDir, assetSubpath );

            // Security check: ensure the output path stays within allowed directories
            boolean valid = false;
            for ( File dir = outFile.getCanonicalFile().getParentFile(); dir != null; dir = dir.getParentFile() ) {
                if ( allowedSubdirs.contains( dir ) ) {
                    valid = true;
                    break;
                }
            }

            if ( !valid ) {
                continue;
            }

            final File outFileDir = outFile.getParentFile();
            if ( outFileDir != null ) {
                Files.createDirectories( outFileDir.toPath() );
            }

            try {
                extractFile( raf, dataOffset, dataEntries, fileInfo, outFile );
                result = true;
            }
            catch ( final Exception e ) {
                // Log and continue with remaining files
            }
        }

        // Extract ANIM files from GOG CD image if present
        if ( gogCdImageInfo != null ) {
            try {
                final boolean cdResult = extractAnimFromGogCdImage( raf, dataOffset, dataEntries, gogCdImageInfo, cacheDir, outputDir );
                result = result || cdResult;
            }
            catch ( final Exception e ) {
                // Log and continue — regular assets may still be usable
            }
        }

        return result;
    }

    /**
     * Extracts the GOG CD image (homm2.gog) from the installer, converts it to ISO,
     * and extracts ANIM files from it using the isotools library.
     */
    private static boolean extractAnimFromGogCdImage( final RandomAccessFile raf, final long dataOffset, final DataEntry[] dataEntries,
                                                      final FileInfo gogFileInfo, final File cacheDir, final File outputDir )
        throws IOException
    {
        Files.createDirectories( cacheDir.toPath() );

        final File gogTempFile = new File( cacheDir, "homm2_gog.tmp" );
        final File isoTempFile = new File( cacheDir, "homm2.iso" );

        try {
            // Extract the raw GOG CD image to a temp file
            extractFile( raf, dataOffset, dataEntries, gogFileInfo, gogTempFile );

            // Convert GOG BIN format to ISO
            try ( final InputStream gogIn = Files.newInputStream( gogTempFile.toPath() );
                  final OutputStream isoOut = Files.newOutputStream( isoTempFile.toPath() ) ) {
                HoMM2AssetManagement.gogToISO( gogIn, isoOut );
            }

            // Extract ANIM files from the ISO
            return HoMM2AssetManagement.extractAnimationsFromISO( outputDir, isoTempFile );
        }
        finally {
            Files.deleteIfExists( gogTempFile.toPath() );
            Files.deleteIfExists( isoTempFile.toPath() );
        }
    }

    private static void extractFile( final RandomAccessFile raf, final long dataOffset, final DataEntry[] dataEntries, final FileInfo fileInfo, final File outFile )
        throws IOException
    {
        try ( final OutputStream out = Files.newOutputStream( outFile.toPath() ) ) {
            if ( fileInfo.chunkCount > 1 ) {
                // Multi-chunk mode (GOG Galaxy v5.6.2)
                for ( int i = 0; i < fileInfo.chunkCount; i++ ) {
                    final DataEntry entry = dataEntries[fileInfo.location + i];
                    final byte[] decompressed = readAndDecompressChunk( raf, dataOffset + entry.chunkOffset, entry.chunkSize, entry.fileSize );
                    out.write( decompressed );
                }
            }
            else {
                // Single-chunk mode
                final DataEntry entry = dataEntries[fileInfo.location];
                final long chunkPos = dataOffset + entry.chunkOffset;
                final byte[] decompressed = readAndDecompressChunk( raf, chunkPos, entry.chunkSize, entry.fileSize );

                if ( entry.fileOffset > 0 && entry.fileOffset + entry.fileSize <= decompressed.length ) {
                    out.write( decompressed, (int) entry.fileOffset, (int) entry.fileSize );
                }
                else {
                    out.write( decompressed );
                }
            }
        }
    }

    private static byte[] readAndDecompressChunk( final RandomAccessFile raf, final long chunkPos, final long chunkSize, final long expectedSize ) throws IOException
    {
        raf.seek( chunkPos );

        final byte[] header = new byte[4];
        raf.readFully( header );

        // Expect "zlb\x1a" header
        if ( header[0] != 0x7A || header[1] != 0x6C || header[2] != 0x62 || header[3] != 0x1A ) {
            throw new IOException( "Invalid chunk data header at offset " + chunkPos );
        }

        final byte[] compData = new byte[(int) chunkSize];
        raf.readFully( compData );

        // Try zlib decompression first
        try {
            return zlibDecompress( compData );
        }
        catch ( final DataFormatException e ) {
            // Not zlib — check if it might be LZMA2 or uncompressed
        }

        // Check first byte for LZMA2 dictionary property (0x00-0x28)
        if ( compData[0] >= 0 && compData[0] <= 40 ) {
            // LZMA2
            try {
                return lzma2Decompress( compData, expectedSize );
            }
            catch ( final Exception e ) {
                // Fall through to uncompressed
            }
        }

        // Treat as uncompressed
        return compData;
    }

    // ---- Scanning methods for file entries in block1 ----

    /**
     * Scans for before_install('hash','path',chunkCount) scripts (v5.6.2 GOG Galaxy format)
     */
    private static Map<String, FileInfo> scanBeforeInstallFiles( final byte[] block1 )
    {
        final byte[] biMarker = encodeUTF16LE( "before_install('" );
        final Map<String, FileInfo> fileMap = new HashMap<>();
        int pos = 0;

        while ( pos < block1.length - biMarker.length ) {
            final int idx = findBytes( block1, biMarker, pos );
            if ( idx < 0 ) {
                break;
            }

            if ( idx < 4 ) {
                pos = idx + biMarker.length;
                continue;
            }

            final int strLen = readUint32LE( block1, idx - 4 );
            if ( strLen <= 0 || strLen > 10000 || idx + strLen > block1.length ) {
                pos = idx + biMarker.length;
                continue;
            }

            final String text = decodeUTF16LE( block1, idx, strLen );
            final String[] parts = text.split( "'" );

            if ( parts.length >= 4 ) {
                final String path = parts[3];

                int count = 1;
                final int lastComma = text.lastIndexOf( ',' );
                if ( lastComma >= 0 ) {
                    final String countStr = text.substring( lastComma + 1 ).trim().replace( ")", "" );
                    try {
                        count = Integer.parseInt( countStr );
                    }
                    catch ( final NumberFormatException e ) {
                        count = 1;
                    }
                }

                final int strEnd = idx + strLen;
                if ( strEnd + 24 <= block1.length ) {
                    final int location = readUint32LE( block1, strEnd + 20 );

                    if ( isTargetFile( path ) ) {
                        fileMap.put( path, new FileInfo( path, location, count ) );
                    }
                }
            }

            pos = idx + strLen;
        }

        return fileMap;
    }

    /**
     * Scans for destination strings containing target directories (v5.5.0 format).
     * File entry structure: 10 length-prefixed UTF-16LE strings, then 20 bytes version data, then location(u32).
     */
    private static Map<String, FileInfo> scanDestinationFiles( final byte[] block1 )
    {
        final Map<String, FileInfo> fileMap = new HashMap<>();

        for ( final String targetDir : TARGET_DIRS ) {
            final byte[][] patterns = {
                encodeUTF16LE( targetDir + "\\" ), encodeUTF16LE( targetDir.toUpperCase( Locale.ROOT ) + "\\" ),
                encodeUTF16LE( targetDir + "/" ), encodeUTF16LE( targetDir.toUpperCase( Locale.ROOT ) + "/" )
            };

            for ( final byte[] pattern : patterns ) {
                int searchPos = 0;

                while ( searchPos < block1.length ) {
                    final int idx = findBytes( block1, pattern, searchPos );
                    if ( idx < 0 ) {
                        break;
                    }

                    // Find the string that contains this match by backtracking to the length prefix
                    for ( int backtrack = idx - 2; backtrack >= Math.max( 4, idx - 4000 ); backtrack -= 2 ) {
                        final int maybeLen = readUint32LE( block1, backtrack - 4 );

                        if ( maybeLen > 0 && maybeLen < 4000 && maybeLen % 2 == 0 ) {
                            final int strStart = backtrack;
                            final int strEnd = strStart + maybeLen;

                            if ( idx >= strStart && idx + pattern.length <= strEnd && strEnd <= block1.length ) {
                                final String destStr = decodeUTF16LE( block1, strStart, maybeLen );

                                if ( isTargetFile( destStr ) ) {
                                    // Skip 8 more strings after destination, then 20 bytes version data
                                    int p = strEnd;
                                    for ( int s = 0; s < 8; s++ ) {
                                        p = skipString( block1, p );
                                    }
                                    p += 20;

                                    if ( p + 4 <= block1.length ) {
                                        final int location = readUint32LE( block1, p );

                                        if ( !fileMap.containsKey( destStr ) ) {
                                            fileMap.put( destStr, new FileInfo( destStr, location, 1 ) );
                                        }
                                    }
                                }

                                break;
                            }
                        }
                    }

                    searchPos = idx + pattern.length;
                }
            }
        }

        return fileMap;
    }

    // ---- Finding loader magic ----

    private static long findLoaderMagic( final RandomAccessFile raf ) throws IOException
    {
        final int bufSize = 262144;
        final byte[] buf = new byte[bufSize];
        final long fileLen = raf.length();

        for ( long offset = 0; offset < fileLen; offset += bufSize - LOADER_MAGIC.length ) {
            raf.seek( offset );
            final int read = raf.read( buf, 0, (int) Math.min( bufSize, fileLen - offset ) );

            if ( read < LOADER_MAGIC.length ) {
                break;
            }

            final int idx = findBytes( buf, LOADER_MAGIC, 0, read );
            if ( idx >= 0 ) {
                return offset + idx;
            }
        }

        return -1;
    }

    // ---- Helpers ----

    private static boolean isTargetFile( final String name )
    {
        final String lower = name.toLowerCase( Locale.ROOT ).replace( '\\', '/' );

        // Match the GOG CD image file
        if ( lower.equals( GOG_CD_IMAGE_NAME ) || lower.endsWith( "/" + GOG_CD_IMAGE_NAME ) ) {
            return true;
        }

        final String[] parts = lower.split( "/" );
        return parts.length >= 2 && TARGET_DIRS.contains( parts[0] );
    }

    private static int readUint32LE( final byte[] data, final int offset )
    {
        return ( data[offset] & 0xFF ) | ( ( data[offset + 1] & 0xFF ) << 8 ) | ( ( data[offset + 2] & 0xFF ) << 16 ) | ( ( data[offset + 3] & 0xFF ) << 24 );
    }

    private static long readUint64LE( final byte[] data, final int offset )
    {
        final long lo = readUint32LE( data, offset ) & 0xFFFFFFFFL;
        final long hi = readUint32LE( data, offset + 4 ) & 0xFFFFFFFFL;
        return lo | ( hi << 32 );
    }

    private static long readUint32( final RandomAccessFile raf ) throws IOException
    {
        final byte[] buf = new byte[4];
        raf.readFully( buf );
        return readUint32LE( buf, 0 ) & 0xFFFFFFFFL;
    }

    private static int indexOf( final byte[] data, final byte value )
    {
        for ( int i = 0; i < data.length; i++ ) {
            if ( data[i] == value ) {
                return i;
            }
        }
        return data.length;
    }

    private static int findBytes( final byte[] haystack, final byte[] needle, final int start )
    {
        return findBytes( haystack, needle, start, haystack.length );
    }

    private static int findBytes( final byte[] haystack, final byte[] needle, final int start, final int end )
    {
        final int limit = end - needle.length;

        outer:
        for ( int i = start; i <= limit; i++ ) {
            for ( int j = 0; j < needle.length; j++ ) {
                if ( haystack[i + j] != needle[j] ) {
                    continue outer;
                }
            }
            return i;
        }

        return -1;
    }

    private static byte[] encodeUTF16LE( final String str )
    {
        final byte[] buf = new byte[str.length() * 2];
        for ( int i = 0; i < str.length(); i++ ) {
            final char c = str.charAt( i );
            buf[i * 2] = (byte) ( c & 0xFF );
            buf[i * 2 + 1] = (byte) ( ( c >> 8 ) & 0xFF );
        }
        return buf;
    }

    private static String decodeUTF16LE( final byte[] data, final int offset, final int length )
    {
        return new String( data, offset, length, StandardCharsets.UTF_16LE );
    }

    private static int skipString( final byte[] data, final int pos )
    {
        if ( pos + 4 > data.length ) {
            return data.length;
        }
        final int len = readUint32LE( data, pos );
        return pos + 4 + len;
    }

    /**
     * Strips CRC32 prefixes from Inno Setup block sub-blocks.
     * Format: 4-byte CRC + up to 4096 bytes of data, repeating.
     */
    private static byte[] stripBlockCrc( final byte[] raw )
    {
        final ByteArrayOutputStream result = new ByteArrayOutputStream( raw.length );
        int pos = 0;

        while ( pos < raw.length ) {
            pos += 4; // skip CRC32
            final int chunkLen = Math.min( 4096, raw.length - pos );
            result.write( raw, pos, chunkLen );
            pos += chunkLen;
        }

        return result.toByteArray();
    }

    /**
     * LZMA1 decompression. The input starts with 5 bytes of LZMA properties
     * followed by the compressed stream (no size header).
     */
    private static byte[] lzmaDecompress( final byte[] stripped ) throws IOException
    {
        if ( stripped.length < 5 ) {
            throw new IOException( "LZMA data too short" );
        }

        // Build LZMA alone format: 5 bytes props + 8 bytes size (-1 = unknown)
        final byte[] lzmaAlone = new byte[stripped.length + 8];
        System.arraycopy( stripped, 0, lzmaAlone, 0, 5 );
        // Uncompressed size = -1 (unknown)
        for ( int i = 5; i < 13; i++ ) {
            lzmaAlone[i] = (byte) 0xFF;
        }
        System.arraycopy( stripped, 5, lzmaAlone, 13, stripped.length - 5 );

        try ( final InputStream lzmaStream = new java.io.ByteArrayInputStream( lzmaAlone ) ) {
            try ( final org.tukaani.xz.LZMAInputStream decoder = new org.tukaani.xz.LZMAInputStream( lzmaStream ) ) {
                return readAllBytes( decoder );
            }
        }
    }

    /**
     * LZMA2 decompression using XZ library.
     */
    private static byte[] lzma2Decompress( final byte[] data, final long expectedSize ) throws IOException
    {
        // First byte is the dict size property
        final int dictProp = data[0] & 0xFF;
        final int dictSize;

        if ( dictProp == 0 ) {
            dictSize = 1;
        }
        else {
            dictSize = ( 2 | ( dictProp & 1 ) ) << ( dictProp / 2 + 11 );
        }

        try ( final InputStream bis = new java.io.ByteArrayInputStream( data, 1, data.length - 1 ) ) {
            try ( final org.tukaani.xz.LZMA2InputStream decoder = new org.tukaani.xz.LZMA2InputStream( bis, dictSize ) ) {
                return readAllBytes( decoder );
            }
        }
    }

    /**
     * Zlib decompression.
     */
    private static byte[] zlibDecompress( final byte[] data ) throws DataFormatException
    {
        final Inflater inflater = new Inflater();

        try {
            inflater.setInput( data );

            final ByteArrayOutputStream result = new ByteArrayOutputStream( data.length * 2 );
            final byte[] buffer = new byte[65536];

            while ( !inflater.finished() ) {
                final int count = inflater.inflate( buffer );
                if ( count == 0 && !inflater.finished() ) {
                    throw new DataFormatException( "Inflater produced no output" );
                }
                result.write( buffer, 0, count );
            }

            return result.toByteArray();
        }
        finally {
            inflater.end();
        }
    }

    private static byte[] readAllBytes( final InputStream in ) throws IOException
    {
        final ByteArrayOutputStream result = new ByteArrayOutputStream( 65536 );
        final byte[] buffer = new byte[65536];
        int bytesRead;

        while ( ( bytesRead = in.read( buffer ) ) != -1 ) {
            result.write( buffer, 0, bytesRead );
        }

        return result.toByteArray();
    }

    // ---- Data structures ----

    private static final class DataEntry
    {
        final int firstSlice;
        final int lastSlice;
        final long chunkOffset;
        final long fileOffset;
        final long fileSize;
        final long chunkSize;
        final int flags;

        private DataEntry( final int firstSlice, final int lastSlice, final long chunkOffset, final long fileOffset, final long fileSize, final long chunkSize,
                           final int flags )
        {
            this.firstSlice = firstSlice;
            this.lastSlice = lastSlice;
            this.chunkOffset = chunkOffset;
            this.fileOffset = fileOffset;
            this.fileSize = fileSize;
            this.chunkSize = chunkSize;
            this.flags = flags;
        }

        static DataEntry parse( final byte[] data, final int offset )
        {
            return new DataEntry( (int) readUint32LE( data, offset ), (int) readUint32LE( data, offset + 4 ), readUint32LE( data, offset + 8 ) & 0xFFFFFFFFL,
                                  readUint64LE( data, offset + 12 ), readUint64LE( data, offset + 20 ), readUint64LE( data, offset + 28 ),
                                  ( data[offset + 72] & 0xFF ) | ( ( data[offset + 73] & 0xFF ) << 8 ) );
        }

        private static long readUint32LE( final byte[] data, final int offset )
        {
            return ( data[offset] & 0xFF ) | ( ( data[offset + 1] & 0xFF ) << 8 ) | ( ( data[offset + 2] & 0xFF ) << 16 ) | ( ( data[offset + 3] & 0xFF ) << 24 );
        }

        private static long readUint64LE( final byte[] data, final int offset )
        {
            final long lo = readUint32LE( data, offset ) & 0xFFFFFFFFL;
            final long hi = readUint32LE( data, offset + 4 ) & 0xFFFFFFFFL;
            return lo | ( hi << 32 );
        }
    }

    private static final class FileInfo
    {
        final String path;
        final int location;
        final int chunkCount;

        FileInfo( final String path, final int location, final int chunkCount )
        {
            this.path = path;
            this.location = location;
            this.chunkCount = chunkCount;
        }
    }
}
