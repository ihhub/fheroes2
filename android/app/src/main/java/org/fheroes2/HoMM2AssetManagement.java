/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2022                                                    *
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

public final class HoMM2AssetManagement
{
    static boolean isHoMM2AssetsPresent( final File externalFilesDir )
    {
        return ( new File( externalFilesDir, "data" + File.separator + "heroes2.agg" ) ).exists();
    }

    // Returns true if at least one asset was found and extracted, otherwise returns false
    static boolean extractHoMM2AssetsFromZip( final File externalFilesDir, final InputStream iStream ) throws IOException
    {
        // It is allowed to extract only files located in these subdirectories
        final String[] allowedSubdirNames = { "anim", "data", "maps", "music" };

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

            // Convert file paths in the ZIP archive to lowercase in order to properly validate them against
            // the list of allowed subdirectories that are specified in lowercase
            final File outFile = new File( externalFilesDir, zEntry.getName().toLowerCase( Locale.ROOT ) );
            if ( isValidHoMM2AssetPath( outFile, allowedSubdirs ) ) {
                final File outFileDir = outFile.getParentFile();
                if ( outFileDir != null ) {
                    outFileDir.mkdirs();
                }

                try ( final OutputStream out = new FileOutputStream( outFile ) ) {
                    IOUtils.copy( zStream, out );
                }

                result = true;
            }
        }

        return result;
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
