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

import android.os.Bundle;
import android.util.Log;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import org.apache.commons.io.IOUtils;
import org.libsdl.app.SDLActivity;

public class MainActivity extends SDLActivity
{
    @Override
    protected void onCreate( Bundle savedInstanceState )
    {
        // Extract H2D and translations to the external app-specific storage (sdcard)
        extractAssets( "files", getExternalFilesDir( null ) );

        // Extract TiMidity GUS patches and config file to the internal app-specific storage
        extractAssets( "instruments", getFilesDir() );
        extractAssets( "timidity.cfg", getFilesDir() );

        super.onCreate( savedInstanceState );
    }

    @Override
    protected void onDestroy()
    {
        super.onDestroy();

        // TODO: When SDL_main() exits, the Android app can still remain in memory, and restarting it using Launcher may result in
        // TODO: the following errors during SDL reinitialization:
        // TODO:
        // TODO: Fatal signal 11 (SIGSEGV), code 1 (SEGV_MAPERR), fault addr 0x38 in tid 4397 (SDLThread), pid 4295 (SDLActivity)
        // TODO:
        // TODO: This workaround terminates the whole app when this Activity exits, allowing SDL to initialize normally on startup
        System.exit( 0 );
    }

    private void extractAssets( String assetsDir, File dstDir )
    {
        ArrayList<String> assetsPaths;

        try {
            assetsPaths = getAssetsPaths( assetsDir );
        }
        catch ( Exception ex ) {
            Log.e( "fheroes2", "Failed to get a list of assets.", ex );

            return;
        }

        for ( String path : assetsPaths ) {
            try ( InputStream in = getAssets().open( path ) ) {
                File outFile = new File( dstDir, path );
                String outFileDir = outFile.getParent();

                if ( outFileDir != null ) {
                    ( new File( outFileDir ) ).mkdirs();
                }

                try ( OutputStream out = new FileOutputStream( outFile ) ) {
                    IOUtils.copy( in, out );
                }
            }
            catch ( Exception ex ) {
                Log.e( "fheroes2", "Failed to extract the asset.", ex );
            }
        }
    }

    private ArrayList<String> getAssetsPaths( String path ) throws IOException
    {
        ArrayList<String> result = new ArrayList<>();

        String[] assets = getAssets().list( path );

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
        for ( String asset : assets ) {
            result.addAll( getAssetsPaths( path + File.separator + asset ) );
        }

        return result;
    }
}
