/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2024                                                    *
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
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Locale;
import java.util.function.Predicate;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;
import java.util.zip.ZipOutputStream;

import org.apache.commons.io.IOUtils;

final class FileManagement
{
    private FileManagement()
    {
        throw new IllegalStateException( "Instantiation is not allowed" );
    }

    static List<String> getFileList( final File fileDir, final List<String> allowedFileExtensions )
    {
        final List<String> result = new ArrayList<>();

        final File[] fileList = fileDir.listFiles( ( dir, name ) -> {
            if ( !dir.equals( fileDir ) ) {
                return false;
            }

            final String lowercaseName = name.toLowerCase( Locale.ROOT );

            for ( final String extension : allowedFileExtensions ) {
                if ( lowercaseName.endsWith( extension ) ) {
                    return true;
                }
            }

            return false;
        } );

        if ( fileList != null ) {
            for ( final File file : fileList ) {
                if ( file.isFile() ) {
                    result.add( file.getName() );
                }
            }

            Collections.sort( result );
        }

        return result;
    }

    /**
     * The directory structure will not be preserved when importing. All matching files will be imported directly into the target directory.
     *
     * @return true if at least one matching file was found and imported, otherwise returns false
     */
    static boolean importFilesFromZip( final File fileDir, final List<String> allowedFileExtensions, final InputStream zipStream ) throws IOException
    {
        final Predicate<String> checkExtension = ( String name ) ->
        {
            final String lowercaseName = name.toLowerCase( Locale.ROOT );

            for ( final String extension : allowedFileExtensions ) {
                if ( lowercaseName.endsWith( extension ) ) {
                    return true;
                }
            }

            return false;
        };

        boolean result = false;

        try ( final ZipInputStream zin = new ZipInputStream( zipStream ) ) {
            Files.createDirectories( fileDir.toPath() );

            for ( ZipEntry zEntry = zin.getNextEntry(); zEntry != null; zEntry = zin.getNextEntry() ) {
                if ( zEntry.isDirectory() ) {
                    continue;
                }

                final String zEntryFileName = new File( zEntry.getName() ).getName();
                if ( !checkExtension.test( zEntryFileName ) ) {
                    continue;
                }

                try ( final OutputStream out = Files.newOutputStream( ( new File( fileDir, zEntryFileName ) ).toPath() ) ) {
                    IOUtils.copy( zin, out );
                }

                result = true;
            }
        }

        return result;
    }

    static void exportFilesToZip( final File fileDir, final List<String> fileNames, final OutputStream zipStream ) throws IOException
    {
        try ( final ZipOutputStream zout = new ZipOutputStream( zipStream ) ) {
            for ( final String fileName : fileNames ) {
                zout.putNextEntry( new ZipEntry( fileName ) );

                try ( final InputStream in = Files.newInputStream( ( new File( fileDir, fileName ) ).toPath() ) ) {
                    IOUtils.copy( in, zout );
                }
            }
        }
    }

    static void deleteFiles( final File fileDir, final List<String> fileNames ) throws IOException
    {
        for ( final String fileName : fileNames ) {
            final File file = new File( fileDir, fileName );

            Files.deleteIfExists( file.toPath() );
        }
    }
}
