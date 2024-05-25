/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2023 - 2024                                             *
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
import java.io.InputStream;
import java.util.Objects;

import android.app.AlertDialog;
import android.content.ContentResolver;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.appcompat.app.AppCompatActivity;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModel;
import androidx.lifecycle.ViewModelProvider;

public final class ToolsetActivity extends AppCompatActivity
{
    public static final class ToolsetActivityViewModel extends ViewModel
    {
        private enum BackgroundTaskResult
        {
            RESULT_NONE,
            RESULT_SUCCESS,
            RESULT_NO_ASSETS,
            RESULT_ERROR
        }

        private static final class Status
        {
            private boolean isHoMM2AssetsPresent;
            private boolean isBackgroundTaskExecuting;
            private final BackgroundTaskResult backgroundTaskResult;
            private final String backgroundTaskError;

            private Status( final boolean isHoMM2AssetsPresent, final boolean isBackgroundTaskExecuting, final BackgroundTaskResult backgroundTaskResult,
                            final String backgroundTaskError )
            {
                this.isHoMM2AssetsPresent = isHoMM2AssetsPresent;
                this.isBackgroundTaskExecuting = isBackgroundTaskExecuting;
                this.backgroundTaskResult = backgroundTaskResult;
                this.backgroundTaskError = backgroundTaskError;
            }

            private Status setIsHoMM2AssetsPresent( final boolean isHoMM2AssetsPresent )
            {
                this.isHoMM2AssetsPresent = isHoMM2AssetsPresent;

                return this;
            }

            @SuppressWarnings( "SameParameterValue" )
            private Status setIsBackgroundTaskExecuting( final boolean isBackgroundTaskExecuting )
            {
                this.isBackgroundTaskExecuting = isBackgroundTaskExecuting;

                return this;
            }
        }

        private final MutableLiveData<Status> liveStatus = new MutableLiveData<>( new Status( false, false, BackgroundTaskResult.RESULT_NONE, "" ) );

        private void validateAssets( final File externalFilesDir )
        {
            final Status status = Objects.requireNonNull( liveStatus.getValue() );

            liveStatus.setValue( status.setIsHoMM2AssetsPresent( HoMM2AssetManagement.isHoMM2AssetsPresent( externalFilesDir ) ) );
        }

        private void extractAssets( final File externalFilesDir, final File cacheDir, final Uri zipFileUri, final ContentResolver contentResolver )
        {
            final Status status = Objects.requireNonNull( liveStatus.getValue() );

            if ( status.isBackgroundTaskExecuting ) {
                return;
            }

            liveStatus.setValue( status.setIsBackgroundTaskExecuting( true ) );

            new Thread( () -> {
                try ( final InputStream in = contentResolver.openInputStream( zipFileUri ) ) {
                    if ( HoMM2AssetManagement.extractHoMM2AssetsFromZip( externalFilesDir, cacheDir, in ) ) {
                        liveStatus.postValue(
                            new Status( HoMM2AssetManagement.isHoMM2AssetsPresent( externalFilesDir ), false, BackgroundTaskResult.RESULT_SUCCESS, "" ) );
                    }
                    else {
                        liveStatus.postValue(
                            new Status( HoMM2AssetManagement.isHoMM2AssetsPresent( externalFilesDir ), false, BackgroundTaskResult.RESULT_NO_ASSETS, "" ) );
                    }
                }
                catch ( final Exception ex ) {
                    Log.e( "fheroes2", "Failed to extract the assets.", ex );

                    liveStatus.postValue( new Status( HoMM2AssetManagement.isHoMM2AssetsPresent( externalFilesDir ), false, BackgroundTaskResult.RESULT_ERROR,
                                                      String.format( "%s", ex ) ) );
                }
            } ).start();
        }
    }

    private ToolsetActivityViewModel viewModel = null;

    private final ActivityResultLauncher<String> zipFileChooserLauncher = registerForActivityResult( new ActivityResultContracts.GetContent(), result -> {
        // No ZIP file was selected
        if ( result == null ) {
            return;
        }

        viewModel.extractAssets( getExternalFilesDir( null ), getCacheDir(), result, getContentResolver() );
    } );

    @Override
    protected void onCreate( final Bundle savedInstanceState )
    {
        super.onCreate( savedInstanceState );

        setContentView( R.layout.activity_toolset );

        viewModel = new ViewModelProvider( this ).get( ToolsetActivityViewModel.class );
        viewModel.liveStatus.observe( this, this::updateUI );
    }

    @Override
    protected void onResume()
    {
        super.onResume();

        viewModel.validateAssets( getExternalFilesDir( null ) );
    }

    @SuppressWarnings( "java:S1172" ) // SonarQube warning "Remove unused method parameter"
    public void startGameButtonClicked( final View view )
    {
        startActivity( new Intent( this, GameActivity.class ) );

        // Replace this activity with the newly launched activity
        finish();
    }

    @SuppressWarnings( "java:S1172" ) // SonarQube warning "Remove unused method parameter"
    public void extractHoMM2AssetsButtonClicked( final View view )
    {
        zipFileChooserLauncher.launch( "application/zip" );
    }

    @SuppressWarnings( "java:S1172" ) // SonarQube warning "Remove unused method parameter"
    public void downloadHoMM2DemoButtonClicked( final View view )
    {
        try {
            startActivity( new Intent( Intent.ACTION_VIEW, Uri.parse( getString( R.string.activity_toolset_homm2_demo_url ) ) ) );
        }
        catch ( final Exception ex ) {
            Log.e( "fheroes2", "Failed to download the HoMM2 demo archive.", ex );

            ( new AlertDialog.Builder( this ) )
                .setTitle( R.string.activity_toolset_download_homm2_demo_error_title )
                .setMessage( R.string.activity_toolset_download_homm2_demo_error_message )
                .setPositiveButton( R.string.activity_toolset_download_homm2_demo_error_positive_btn_text, ( dialog, which ) -> {} )
                .create()
                .show();
        }
    }

    @SuppressWarnings( "java:S1172" ) // SonarQube warning "Remove unused method parameter"
    public void saveFileManagerButtonClicked( final View view )
    {
        startActivity( new Intent( this, SaveFileManagerActivity.class ) );
    }

    @SuppressWarnings( "java:S1172" ) // SonarQube warning "Remove unused method parameter"
    public void mapFileManagerButtonClicked( final View view )
    {
        startActivity( new Intent( this, MapFileManagerActivity.class ) );
    }

    private void updateUI( final ToolsetActivityViewModel.Status modelStatus )
    {
        final Button startGameButton = findViewById( R.id.activity_toolset_start_game_btn );
        final Button extractHoMM2AssetsButton = findViewById( R.id.activity_toolset_extract_homm2_assets_btn );
        final Button downloadHoMM2DemoButton = findViewById( R.id.activity_toolset_download_homm2_demo_btn );
        final Button saveFileManagerButton = findViewById( R.id.activity_toolset_save_file_manager_btn );
        final Button mapFileManagerButton = findViewById( R.id.activity_toolset_map_file_manager_btn );

        final TextView gameStatusTextView = findViewById( R.id.activity_toolset_game_status_lbl );
        final TextView lastTaskStatusTextView = findViewById( R.id.activity_toolset_last_task_status_lbl );

        final ProgressBar backgroundTaskProgressBar = findViewById( R.id.activity_toolset_background_task_pb );

        startGameButton.setEnabled( !modelStatus.isBackgroundTaskExecuting && modelStatus.isHoMM2AssetsPresent );
        extractHoMM2AssetsButton.setEnabled( !modelStatus.isBackgroundTaskExecuting );
        downloadHoMM2DemoButton.setEnabled( !modelStatus.isBackgroundTaskExecuting );
        saveFileManagerButton.setEnabled( !modelStatus.isBackgroundTaskExecuting );
        mapFileManagerButton.setEnabled( !modelStatus.isBackgroundTaskExecuting );

        switch ( modelStatus.backgroundTaskResult ) {
        case RESULT_NONE:
            break;
        case RESULT_SUCCESS:
            lastTaskStatusTextView.setText( getString( R.string.activity_toolset_last_task_status_lbl_text_completed_successfully ) );
            break;
        case RESULT_NO_ASSETS:
            lastTaskStatusTextView.setText( getString( R.string.activity_toolset_last_task_status_lbl_text_no_assets_found ) );
            break;
        case RESULT_ERROR:
            lastTaskStatusTextView.setText( String.format( getString( R.string.activity_toolset_last_task_status_lbl_text_failed ), modelStatus.backgroundTaskError ) );
            break;
        default:
            assert false;
            break;
        }

        gameStatusTextView.setVisibility( modelStatus.isHoMM2AssetsPresent ? View.GONE : View.VISIBLE );
        backgroundTaskProgressBar.setVisibility( !modelStatus.isBackgroundTaskExecuting ? View.GONE : View.VISIBLE );
        lastTaskStatusTextView.setVisibility( modelStatus.isBackgroundTaskExecuting ? View.GONE : View.VISIBLE );
    }
}
