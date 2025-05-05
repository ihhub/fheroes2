/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2022 - 2025                                             *
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
import java.util.Arrays;
import java.util.List;

import android.content.Intent;
import android.os.Bundle;
import android.util.Log;

import org.apache.commons.io.IOUtils;

import org.libsdl.app.SDLActivity;

public final class GameActivity extends SDLActivity
{
    @Override
    protected void onCreate( final Bundle savedInstanceState )
    {
        final File filesDir = getFilesDir();
        final File externalFilesDir = getExternalFilesDir( null );

        if ( isAssetsDigestChanged( "assets.digest", new File( filesDir, "assets.digest" ) ) ) {
            try {
                extractAssets( "files", externalFilesDir );
                // Digest should be updated only after successful extraction of all assets
                extractAssets( "assets.digest", filesDir );
            }
            catch ( final Exception ex ) {
                Log.e( "fheroes2", "Failed to extract assets.", ex );
            }
        }

        super.onCreate( savedInstanceState );

        // If the minimum set of game assets has not been found, run the toolset activity instead
        if ( !HoMM2AssetManagement.isHoMM2AssetsPresent( externalFilesDir ) ) {
            startActivity( new Intent( this, ToolsetActivity.class ) );

            // Replace this activity with the newly launched activity
            finish();
        }
    }

    @Override
    protected void onDestroy()
    {
        super.onDestroy();

        // When SDL_main() exits, the app process can still remain in memory, and restarting it
        // (for example, using Android Launcher) may result in various errors when SDL attempts
        // to "reinitialize" already initialized things. This workaround terminates the whole
        // process when this activity is destroyed, allowing SDL to initialize normally on the
        // next startup.
        System.exit( 0 );
    }

    @SuppressWarnings( "SameParameterValue" )
    private boolean isAssetsDigestChanged( final String assetsDigestPath, final File localDigestFile )
    {
        try ( final InputStream assetsDigestStream = getAssets().open( assetsDigestPath ) ) {
            try ( final InputStream localDigestStream = Files.newInputStream( localDigestFile.toPath() ) ) {
                if ( Arrays.equals( IOUtils.toByteArray( assetsDigestStream ), IOUtils.toByteArray( localDigestStream ) ) ) {
                    return false;
                }

                Log.i( "fheroes2", "Digest of assets has been changed." );
            }
            catch ( final Exception ex ) {
                Log.i( "fheroes2", "Failed to access the local digest. Considering the digest of assets as changed.", ex );
            }
        }
        catch ( final Exception ex ) {
            Log.e( "fheroes2", "Failed to access the digest of assets. Considering the digest of assets as changed.", ex );
        }

        return true;
    }

    private void extractAssets( final String srcPath, final File dstDir ) throws IOException
    {
        for ( final String path : getAssetsPaths( srcPath ) ) {
            try ( final InputStream in = getAssets().open( path ) ) {
                final File outFile = new File( dstDir, path );

                final File outFileDir = outFile.getParentFile();
                if ( outFileDir != null ) {
                    Files.createDirectories( outFileDir.toPath() );
                }

                try ( final OutputStream out = Files.newOutputStream( outFile.toPath() ) ) {
                    IOUtils.copy( in, out );
                }
            }
        }
    }

    private List<String> getAssetsPaths( final String path ) throws IOException
    {
        final List<String> result = new ArrayList<>();

        final String[] assets = getAssets().list( path );

        // There is no such path at all
        if ( assets == null ) {
            return result;
        }

        // Leaf node
        if ( assets.length == 0 ) {
            result.add( path );

            return result;
        }

        // Regular node
        for ( final String asset : assets ) {
            result.addAll( getAssetsPaths( path + File.separator + asset ) );
        }

        return result;
    }
}
