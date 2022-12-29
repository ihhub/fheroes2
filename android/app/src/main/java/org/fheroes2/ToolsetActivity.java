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
import java.io.InputStream;

public final class ToolsetActivity extends Activity
{
    private static final int REQUEST_CODE_OPEN_HOMM2_ASSETS_ZIP = 1001;

    private Button runGameButton = null;
    private Button extractHoMM2AssetsButton = null;
    private Button downloadHoMM2DemoButton = null;

    private TextView gameStatusTextView = null;
    private TextView lastTaskStatusTextView = null;

    private ProgressBar backgroundTaskProgressBar = null;

    private Thread backgroundTask = null;

    @Override
    protected void onCreate( final Bundle savedInstanceState )
    {
        super.onCreate( savedInstanceState );

        setContentView( R.layout.activity_toolset );

        runGameButton = findViewById( R.id.activity_toolset_run_game_btn );
        extractHoMM2AssetsButton = findViewById( R.id.activity_toolset_extract_homm2_assets_btn );
        downloadHoMM2DemoButton = findViewById( R.id.activity_toolset_download_homm2_demo_btn );

        gameStatusTextView = findViewById( R.id.activity_toolset_game_status_lbl );
        lastTaskStatusTextView = findViewById( R.id.activity_toolset_last_task_status_lbl );

        backgroundTaskProgressBar = findViewById( R.id.activity_toolset_background_task_pb );
    }

    @Override
    protected void onResume()
    {
        super.onResume();

        updateUI();
    }

    @Override
    protected void onPause()
    {
        super.onPause();
    }

    @Override
    protected void onActivityResult( final int requestCode, final int resultCode, final Intent data )
    {
        super.onActivityResult( requestCode, resultCode, data );

        switch ( requestCode ) {
        case REQUEST_CODE_OPEN_HOMM2_ASSETS_ZIP:
            if ( resultCode == RESULT_OK && data != null ) {
                final Uri zipFileUri = data.getData();

                if ( backgroundTask == null ) {
                    backgroundTask = new Thread( () -> {
                        try ( final InputStream iStream = getContentResolver().openInputStream( zipFileUri ) ) {
                            HoMM2AssetManagement.extractHoMM2AssetsFromZip( getExternalFilesDir( null ), iStream );

                            runOnUiThread( () -> updateLastTaskStatus( getString( R.string.activity_toolset_last_task_status_lbl_text_completed_successfully ) ) );
                        }
                        catch ( final Exception ex ) {
                            Log.e( "fheroes2", "Failed to extract the ZIP file.", ex );

                            runOnUiThread( () -> updateLastTaskStatus( String.format( getString( R.string.activity_toolset_last_task_status_lbl_text_failed ), ex ) ) );
                        }
                        finally {
                            runOnUiThread( () -> {
                                backgroundTask = null;

                                updateUI();
                            } );
                        }
                    } );

                    updateUI();

                    backgroundTask.start();
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

        // Replace this activity with the newly launched activity
        finish();
    }

    public void extractHoMM2AssetsButtonClicked( final View view )
    {
        final Intent intent = new Intent( Intent.ACTION_OPEN_DOCUMENT );
        intent.setType( "application/zip" );

        startActivityForResult( Intent.createChooser( intent, getString( R.string.activity_toolset_extract_homm2_assets_chooser_title ) ),
                                REQUEST_CODE_OPEN_HOMM2_ASSETS_ZIP );
    }

    public void downloadHoMM2DemoButtonClicked( final View view )
    {
        startActivity( new Intent( Intent.ACTION_VIEW, Uri.parse( getString( R.string.activity_toolset_homm2_demo_url ) ) ) );
    }

    private void updateLastTaskStatus( final String status )
    {
        lastTaskStatusTextView.setText( status );
    }

    private void updateUI()
    {
        // A quick and dirty way to avoid the re-creation of this activity due to the screen orientation change while running a background task
        setRequestedOrientation( backgroundTask == null ? ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED : ActivityInfo.SCREEN_ORIENTATION_LOCKED );

        final boolean isHoMM2AssetsPresent = HoMM2AssetManagement.isHoMM2AssetsPresent( getExternalFilesDir( null ) );

        runGameButton.setEnabled( backgroundTask == null && isHoMM2AssetsPresent );
        extractHoMM2AssetsButton.setEnabled( backgroundTask == null );
        downloadHoMM2DemoButton.setEnabled( backgroundTask == null && !isHoMM2AssetsPresent );

        gameStatusTextView.setVisibility( isHoMM2AssetsPresent ? View.GONE : View.VISIBLE );
        runGameButton.setVisibility( !isHoMM2AssetsPresent ? View.GONE : View.VISIBLE );
        downloadHoMM2DemoButton.setVisibility( isHoMM2AssetsPresent ? View.GONE : View.VISIBLE );
        backgroundTaskProgressBar.setVisibility( backgroundTask == null ? View.GONE : View.VISIBLE );
        lastTaskStatusTextView.setVisibility( backgroundTask != null ? View.GONE : View.VISIBLE );
    }
}
