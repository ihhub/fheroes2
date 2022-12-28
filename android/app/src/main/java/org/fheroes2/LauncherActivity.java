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

import android.app.Activity;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;
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

public final class LauncherActivity extends Activity
{
    private static final int REQUEST_CODE_OPEN_HOMM2_RESOURCES_ZIP = 1001;

    private Button runGameButton = null;
    private Button extractHoMM2ResourcesButton = null;
    private Button downloadHoMM2DemoButton = null;

    private TextView gameStatusTextView = null;
    private TextView lastTaskStatusTextView = null;

    private ProgressBar progressBar = null;

    private Thread activeBackgroundTask = null;

    @Override
    protected void onCreate( final Bundle savedInstanceState )
    {
        super.onCreate( savedInstanceState );

        setContentView( R.layout.activity_launcher );

        runGameButton = findViewById( R.id.activity_launcher_run_game_btn );
        extractHoMM2ResourcesButton = findViewById( R.id.activity_launcher_extract_homm2_resources_btn );
        downloadHoMM2DemoButton = findViewById( R.id.activity_launcher_download_homm2_demo_btn );

        gameStatusTextView = findViewById( R.id.activity_launcher_game_status_lbl );
        lastTaskStatusTextView = findViewById( R.id.activity_launcher_last_task_status_lbl );

        progressBar = findViewById( R.id.activity_launcher_pb );
    }

    @Override
    protected void onResume()
    {
        super.onResume();

        updateUI();
    }

    @Override
    protected void onActivityResult( final int requestCode, final int resultCode, final Intent data )
    {
        super.onActivityResult( requestCode, resultCode, data );

        switch ( requestCode ) {
        case REQUEST_CODE_OPEN_HOMM2_RESOURCES_ZIP:
            if ( resultCode == RESULT_OK && data != null ) {
                final Uri zipFileUri = data.getData();

                if ( activeBackgroundTask == null ) {
                    activeBackgroundTask = new Thread( () -> {
                        try ( final InputStream iStream = getContentResolver().openInputStream( zipFileUri ) ) {
                            extractHoMM2ResourcesFromZip( iStream );

                            runOnUiThread( () -> updateLastTaskStatus( getString( R.string.activity_launcher_last_task_status_lbl_text_completed_successfully ) ) );
                        }
                        catch ( final Exception ex ) {
                            Log.e( "fheroes2", "Failed to extract the ZIP file.", ex );

                            runOnUiThread( () -> updateLastTaskStatus( String.format( getString( R.string.activity_launcher_last_task_status_lbl_text_failed ), ex ) ) );
                        }
                        finally {
                            runOnUiThread( () -> {
                                activeBackgroundTask = null;

                                updateUI();
                            } );
                        }
                    } );

                    updateUI();

                    activeBackgroundTask.start();
                }
            }
            break;
        default:
            break;
        }
    }

    public void runGameButtonClicked( final View view )
    {
        startActivity( new Intent( this, GameActivity.class ) );
    }

    public void extractHoMM2ResourcesButtonClicked( final View view )
    {
        final Intent intent = new Intent( Intent.ACTION_OPEN_DOCUMENT );
        intent.setType( "application/zip" );

        startActivityForResult( Intent.createChooser( intent, getString( R.string.activity_launcher_extract_homm2_resources_chooser_title ) ),
                                REQUEST_CODE_OPEN_HOMM2_RESOURCES_ZIP );
    }

    public void downloadHoMM2DemoButtonClicked( final View view )
    {
        startActivity( new Intent( Intent.ACTION_VIEW, Uri.parse( getString( R.string.activity_launcher_homm2_demo_url ) ) ) );
    }

    private void updateLastTaskStatus( final String status )
    {
        lastTaskStatusTextView.setText( status );
    }

    private void updateUI()
    {
        // A quick and dirty way to avoid the re-creation of this activity due to the screen orientation change while running a background task
        setRequestedOrientation( activeBackgroundTask == null ? ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED : ActivityInfo.SCREEN_ORIENTATION_LOCKED );

        runGameButton.setEnabled( activeBackgroundTask == null && isHoMM2ResourcesPresent() );
        extractHoMM2ResourcesButton.setEnabled( activeBackgroundTask == null );
        downloadHoMM2DemoButton.setEnabled( activeBackgroundTask == null && !isHoMM2ResourcesPresent() );

        gameStatusTextView.setVisibility( isHoMM2ResourcesPresent() ? View.GONE : View.VISIBLE );
        runGameButton.setVisibility( !isHoMM2ResourcesPresent() ? View.GONE : View.VISIBLE );
        downloadHoMM2DemoButton.setVisibility( isHoMM2ResourcesPresent() ? View.GONE : View.VISIBLE );
        progressBar.setVisibility( activeBackgroundTask == null ? View.GONE : View.VISIBLE );
        lastTaskStatusTextView.setVisibility( activeBackgroundTask != null ? View.GONE : View.VISIBLE );
    }

    private boolean isValidHoMM2ResourcePath( final File path, final Set<File> allowedSubdirs ) throws IOException
    {
        for ( File dir = path.getCanonicalFile().getParentFile(); dir != null; dir = dir.getParentFile() ) {
            if ( allowedSubdirs.contains( dir ) ) {
                return true;
            }
        }

        return false;
    }

    private void extractHoMM2ResourcesFromZip( final InputStream iStream ) throws IOException
    {
        final File externalFilesDir = getExternalFilesDir( null );

        // It is allowed to extract only files located in these subdirectories
        final String[] allowedSubdirNames = { "anim", "data", "maps", "music" };

        final Set<File> allowedSubdirs = new HashSet<>();
        for ( String name : allowedSubdirNames ) {
            allowedSubdirs.add( new File( externalFilesDir, name ).getCanonicalFile() );
        }

        final ZipInputStream zStream = new ZipInputStream( iStream );
        for ( ZipEntry zEntry = zStream.getNextEntry(); zEntry != null; zEntry = zStream.getNextEntry() ) {
            // No need to extract empty directories
            if ( zEntry.isDirectory() ) {
                continue;
            }

            // Convert file paths in the ZIP archive to lowercase to properly validate them
            final File outFile = new File( externalFilesDir, zEntry.getName().toLowerCase( Locale.ROOT ) );
            if ( isValidHoMM2ResourcePath( outFile, allowedSubdirs ) ) {
                final File outFileDir = outFile.getParentFile();
                if ( outFileDir != null ) {
                    outFileDir.mkdirs();
                }

                try ( final OutputStream out = new FileOutputStream( outFile ) ) {
                    IOUtils.copy( zStream, out );
                }
            }
        }
    }

    private boolean isHoMM2ResourcesPresent()
    {
        return ( new File( getExternalFilesDir( null ), "data" + File.separator + "heroes2.agg" ) ).exists();
    }
}
