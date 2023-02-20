/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2023                                                    *
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

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.HashSet;
import java.util.Locale;
import java.util.Set;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;
import org.apache.commons.io.IOUtils;

final class HoMM2AssetManagement
{
    static boolean isHoMM2AssetsPresent( final File externalFilesDir )
    {
        return ( new File( externalFilesDir, "data" + File.separator + "heroes2.agg" ) ).exists();
    }

    // Returns true if at least one asset was found and extracted, otherwise returns false
    static boolean extractHoMM2AssetsFromZip( final File externalFilesDir, final InputStream iStream ) throws IOException
    {
        // It is allowed to extract only files located in these subdirectories
        final Set<String> allowedSubdirNames = new HashSet<>();
        allowedSubdirNames.add( "anim" );
        // ANIM2 directory is used in the Russian HoMM2 localization made by Buka
        allowedSubdirNames.add( "anim2" );
        allowedSubdirNames.add( "data" );
        allowedSubdirNames.add( "maps" );
        allowedSubdirNames.add( "music" );

        final Set<File> allowedSubdirs = new HashSet<>();
        for ( String name : allowedSubdirNames ) {
            allowedSubdirs.add( new File( externalFilesDir, name ).getCanonicalFile() );
        }

        boolean result = false;

        final ZipInputStream zStream = new ZipInputStream( iStream );
        for ( ZipEntry zEntry = zStream.getNextEntry(); zEntry != null; zEntry = zStream.getNextEntry() ) {
            // No need to extract empty directories
            if ( zEntry.isDirectory() ) {
                continue;
            }

            final String assetSubpath = getHoMM2AssetSubpath( new File( zEntry.getName() ), allowedSubdirNames );
            // No need to extract the file if its path does not contain any of the allowed subdirectories
            if ( assetSubpath.isEmpty() ) {
                continue;
            }

            final File outFile = new File( externalFilesDir, assetSubpath );
            // Check the path for various trickery, such as 'data/../../../bin/file'
            if ( !isValidHoMM2AssetPath( outFile, allowedSubdirs ) ) {
                continue;
            }

            final File outFileDir = outFile.getParentFile();
            if ( outFileDir != null ) {
                outFileDir.mkdirs();
            }

            try ( final OutputStream out = new FileOutputStream( outFile ) ) {
                IOUtils.copy( zStream, out );
            }

            result = true;
        }

        return result;
    }

    // Tries to truncate the given path to the shortest path starting from one of the allowed subdirectories,
    // for example 'foo/bar/data/zoo/file' -> 'data/zoo/file'. Returns an empty string if the given path does
    // not contain any of the allowed subdirectories.
    private static String getHoMM2AssetSubpath( final File path, final Set<String> allowedSubdirNames )
    {
        StringBuilder assetSubpath = new StringBuilder();

        for ( File pathItem = path; pathItem != null; pathItem = pathItem.getParentFile() ) {
            final String pathItemName = pathItem.getName().toLowerCase( Locale.ROOT );

            if ( pathItem != path ) {
                assetSubpath.insert( 0, File.separator );
            }
            assetSubpath.insert( 0, pathItemName );

            if ( allowedSubdirNames.contains( pathItemName ) ) {
                return assetSubpath.toString();
            }
        }

        return "";
    }

    private static boolean isValidHoMM2AssetPath( final File path, final Set<File> allowedSubdirs ) throws IOException
    {
        for ( File dir = path.getCanonicalFile().getParentFile(); dir != null; dir = dir.getParentFile() ) {
            if ( allowedSubdirs.contains( dir ) ) {
                return true;
            }
        }

        return false;
    }
}
