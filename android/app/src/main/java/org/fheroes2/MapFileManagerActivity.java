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
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Objects;
import java.util.Queue;

import android.app.AlertDialog;
import android.content.ContentResolver;
import android.content.res.Resources;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.ImageButton;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.TextView;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.appcompat.app.AppCompatActivity;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModel;
import androidx.lifecycle.ViewModelProvider;

public final class MapFileManagerActivity extends AppCompatActivity
{
    public static final class MapFileManagerActivityViewModel extends ViewModel
    {
        private enum BackgroundTaskResult
        {
            RESULT_NONE,
            RESULT_SUCCESS,
            RESULT_NO_MAP_FILES,
            RESULT_ERROR
        }

        private static final class Status
        {
            private boolean isBackgroundTaskExecuting;
            private final BackgroundTaskResult backgroundTaskResult;
            private final String backgroundTaskError;
            private final List<String> mapFileNames;

            private Status( final boolean isBackgroundTaskExecuting, final BackgroundTaskResult backgroundTaskResult, final String backgroundTaskError,
                            final List<String> mapFileNames )
            {
                this.isBackgroundTaskExecuting = isBackgroundTaskExecuting;
                this.backgroundTaskResult = backgroundTaskResult;
                this.backgroundTaskError = backgroundTaskError;
                this.mapFileNames = mapFileNames;
            }

            @SuppressWarnings( "SameParameterValue" )
            private Status setIsBackgroundTaskExecuting( final boolean isBackgroundTaskExecuting )
            {
                this.isBackgroundTaskExecuting = isBackgroundTaskExecuting;

                return this;
            }
        }

        private final MutableLiveData<Status> liveStatus = new MutableLiveData<>( new Status( false, BackgroundTaskResult.RESULT_NONE, "", new ArrayList<>() ) );

        /**
         * This method should never be called directly. Call it only using the enqueueBackgroundTask() method.
         */
        @SuppressWarnings( "SameParameterValue" )
        private void updateMapFileList( final File mapFileDir, final List<String> allowedMapFileExtensions )
        {
            final Status status = Objects.requireNonNull( liveStatus.getValue() );
            assert !status.isBackgroundTaskExecuting;

            liveStatus.setValue( status.setIsBackgroundTaskExecuting( true ) );

            new Thread( () -> {
                try {
                    // Reading the list of map files should not by itself change the visible status of the last background task, unless an error occurred while reading
                    liveStatus.postValue( new Status( false, BackgroundTaskResult.RESULT_NONE, "", FileManagement.getFileList( mapFileDir, allowedMapFileExtensions ) ) );
                }
                catch ( final Exception ex ) {
                    Log.e( "fheroes2", "Failed to get a list of map files.", ex );

                    liveStatus.postValue( new Status( false, BackgroundTaskResult.RESULT_ERROR, String.format( "%s", ex ), new ArrayList<>() ) );
                }
            } ).start();
        }

        /**
         * This method should never be called directly. Call it only using the enqueueBackgroundTask() method.
         */
        @SuppressWarnings( "SameParameterValue" )
        private void importMapFiles( final File mapFileDir, final List<String> allowedMapFileExtensions, final Uri zipFileUri, final ContentResolver contentResolver )
        {
            final Status status = Objects.requireNonNull( liveStatus.getValue() );
            assert !status.isBackgroundTaskExecuting;

            liveStatus.setValue( status.setIsBackgroundTaskExecuting( true ) );

            new Thread( () -> {
                boolean atLeastOneMapFileImported = false;
                Exception caughtException = null;

                try ( final InputStream in = contentResolver.openInputStream( zipFileUri ) ) {
                    atLeastOneMapFileImported = FileManagement.importFilesFromZip( mapFileDir, allowedMapFileExtensions, in );
                }
                catch ( final Exception ex ) {
                    Log.e( "fheroes2", "Failed to import map files.", ex );

                    caughtException = ex;
                }
                finally {
                    try {
                        if ( caughtException != null ) {
                            liveStatus.postValue( new Status( false, BackgroundTaskResult.RESULT_ERROR, String.format( "%s", caughtException ),
                                                              FileManagement.getFileList( mapFileDir, allowedMapFileExtensions ) ) );
                        }
                        else {
                            liveStatus.postValue( new Status( false,
                                                              atLeastOneMapFileImported ? BackgroundTaskResult.RESULT_SUCCESS : BackgroundTaskResult.RESULT_NO_MAP_FILES,
                                                              "", FileManagement.getFileList( mapFileDir, allowedMapFileExtensions ) ) );
                        }
                    }
                    catch ( final Exception ex ) {
                        Log.e( "fheroes2", "Failed to get a list of map files.", ex );

                        liveStatus.postValue( new Status( false, BackgroundTaskResult.RESULT_ERROR, String.format( "%s", ex ), new ArrayList<>() ) );
                    }
                }
            } ).start();
        }

        /**
         * This method should never be called directly. Call it only using the enqueueBackgroundTask() method.
         */
        @SuppressWarnings( "SameParameterValue" )
        private void exportMapFiles( final File mapFileDir, final List<String> allowedMapFileExtensions, final List<String> mapFileNames, final Uri zipFileUri,
                                     final ContentResolver contentResolver )
        {
            final Status status = Objects.requireNonNull( liveStatus.getValue() );
            assert !status.isBackgroundTaskExecuting;

            liveStatus.setValue( status.setIsBackgroundTaskExecuting( true ) );

            new Thread( () -> {
                Exception caughtException = null;

                try ( final OutputStream out = contentResolver.openOutputStream( zipFileUri ) ) {
                    FileManagement.exportFilesToZip( mapFileDir, mapFileNames, out );
                }
                catch ( final Exception ex ) {
                    Log.e( "fheroes2", "Failed to export map files.", ex );

                    caughtException = ex;
                }
                finally {
                    try {
                        if ( caughtException != null ) {
                            liveStatus.postValue( new Status( false, BackgroundTaskResult.RESULT_ERROR, String.format( "%s", caughtException ),
                                                              FileManagement.getFileList( mapFileDir, allowedMapFileExtensions ) ) );
                        }
                        else {
                            liveStatus.postValue(
                                new Status( false, BackgroundTaskResult.RESULT_SUCCESS, "", FileManagement.getFileList( mapFileDir, allowedMapFileExtensions ) ) );
                        }
                    }
                    catch ( final Exception ex ) {
                        Log.e( "fheroes2", "Failed to get a list of map files.", ex );

                        liveStatus.postValue( new Status( false, BackgroundTaskResult.RESULT_ERROR, String.format( "%s", ex ), new ArrayList<>() ) );
                    }
                }
            } ).start();
        }

        /**
         * This method should never be called directly. Call it only using the enqueueBackgroundTask() method.
         */
        @SuppressWarnings( "SameParameterValue" )
        private void deleteMapFiles( final File mapFileDir, final List<String> allowedMapFileExtensions, final List<String> mapFileNames )
        {
            final Status status = Objects.requireNonNull( liveStatus.getValue() );
            assert !status.isBackgroundTaskExecuting;

            liveStatus.setValue( status.setIsBackgroundTaskExecuting( true ) );

            new Thread( () -> {
                Exception caughtException = null;

                try {
                    FileManagement.deleteFiles( mapFileDir, mapFileNames );
                }
                catch ( final Exception ex ) {
                    Log.e( "fheroes2", "Failed to delete map files.", ex );

                    caughtException = ex;
                }
                finally {
                    try {
                        if ( caughtException != null ) {
                            liveStatus.postValue( new Status( false, BackgroundTaskResult.RESULT_ERROR, String.format( "%s", caughtException ),
                                                              FileManagement.getFileList( mapFileDir, allowedMapFileExtensions ) ) );
                        }
                        else {
                            liveStatus.postValue(
                                new Status( false, BackgroundTaskResult.RESULT_SUCCESS, "", FileManagement.getFileList( mapFileDir, allowedMapFileExtensions ) ) );
                        }
                    }
                    catch ( final Exception ex ) {
                        Log.e( "fheroes2", "Failed to get a list of map files.", ex );

                        liveStatus.postValue( new Status( false, BackgroundTaskResult.RESULT_ERROR, String.format( "%s", ex ), new ArrayList<>() ) );
                    }
                }
            } ).start();
        }
    }

    private static final List<String> allowedMapFileExtensions = new ArrayList<>( Collections.singletonList( ".fh2m" ) );

    private File mapFileDir = null;

    private ListView mapFileListView = null;
    private ArrayAdapter<String> mapFileListViewAdapter = null;

    private MapFileManagerActivityViewModel viewModel = null;

    private final ActivityResultLauncher<String> zipFileChooserLauncher = registerForActivityResult( new ActivityResultContracts.GetContent(), result -> {
        // No ZIP file was selected
        if ( result == null ) {
            return;
        }

        for ( int i = 0; i < mapFileListView.getCount(); ++i ) {
            mapFileListView.setItemChecked( i, false );
        }

        enqueueBackgroundTask( () -> viewModel.importMapFiles( mapFileDir, allowedMapFileExtensions, result, getContentResolver() ) );
    } );

    private final ActivityResultLauncher<String> zipFileLocationChooserLauncher
        = registerForActivityResult( new ActivityResultContracts.CreateDocument( "application/zip" ), result -> {
              // No location was selected
              if ( result == null ) {
                  return;
              }

              final List<String> mapFileNames = new ArrayList<>();

              for ( int i = 0; i < mapFileListView.getCount(); ++i ) {
                  if ( mapFileListView.isItemChecked( i ) ) {
                      mapFileNames.add( mapFileListViewAdapter.getItem( i ) );
                  }
              }

              enqueueBackgroundTask( () -> viewModel.exportMapFiles( mapFileDir, allowedMapFileExtensions, mapFileNames, result, getContentResolver() ) );
          } );

    private final Queue<Runnable> backgroundTaskQueue = new ArrayDeque<>();

    @Override
    protected void onCreate( final Bundle savedInstanceState )
    {
        super.onCreate( savedInstanceState );

        setContentView( R.layout.activity_map_file_manager );

        mapFileDir = new File( getExternalFilesDir( null ), "maps" );

        mapFileListView = findViewById( R.id.activity_map_file_manager_map_file_list );
        mapFileListViewAdapter = new ArrayAdapter<>( this, android.R.layout.simple_list_item_multiple_choice, new ArrayList<>() );

        mapFileListView.setAdapter( mapFileListViewAdapter );
        mapFileListView.setEmptyView( findViewById( R.id.activity_map_file_manager_map_file_list_empty_lbl ) );

        viewModel = new ViewModelProvider( this ).get( MapFileManagerActivityViewModel.class );
        viewModel.liveStatus.observe( this, this::runNextBackgroundTask );
        viewModel.liveStatus.observe( this, this::updateUI );
    }

    @Override
    protected void onResume()
    {
        super.onResume();

        enqueueBackgroundTask( () -> viewModel.updateMapFileList( mapFileDir, allowedMapFileExtensions ) );
    }

    @SuppressWarnings( "java:S1172" ) // SonarQube warning "Remove unused method parameter"
    public void selectAllButtonClicked( final View view )
    {
        for ( int i = 0; i < mapFileListView.getCount(); ++i ) {
            mapFileListView.setItemChecked( i, true );
        }
    }

    @SuppressWarnings( "java:S1172" ) // SonarQube warning "Remove unused method parameter"
    public void unselectAllButtonClicked( final View view )
    {
        for ( int i = 0; i < mapFileListView.getCount(); ++i ) {
            mapFileListView.setItemChecked( i, false );
        }
    }

    @SuppressWarnings( "java:S1172" ) // SonarQube warning "Remove unused method parameter"
    public void importButtonClicked( final View view )
    {
        zipFileChooserLauncher.launch( "application/zip" );
    }

    @SuppressWarnings( "java:S1172" ) // SonarQube warning "Remove unused method parameter"
    public void exportButtonClicked( final View view )
    {
        if ( mapFileListView.getCheckedItemCount() == 0 ) {
            ( new AlertDialog.Builder( this ) )
                .setTitle( R.string.activity_map_file_manager_no_files_selected_for_export_title )
                .setMessage( R.string.activity_map_file_manager_no_files_selected_for_export_message )
                .setPositiveButton( R.string.activity_map_file_manager_no_files_selected_for_export_positive_btn_text, ( dialog, which ) -> {} )
                .create()
                .show();

            return;
        }

        zipFileLocationChooserLauncher.launch( getString( R.string.activity_map_file_manager_suggested_zip_file_name ) );
    }

    @SuppressWarnings( "java:S1172" ) // SonarQube warning "Remove unused method parameter"
    public void deleteButtonClicked( final View view )
    {
        final int selectedMapFilesCount = mapFileListView.getCheckedItemCount();

        if ( selectedMapFilesCount == 0 ) {
            ( new AlertDialog.Builder( this ) )
                .setTitle( R.string.activity_map_file_manager_no_files_selected_for_deletion_title )
                .setMessage( R.string.activity_map_file_manager_no_files_selected_for_deletion_message )
                .setPositiveButton( R.string.activity_map_file_manager_no_files_selected_for_deletion_positive_btn_text, ( dialog, which ) -> {} )
                .create()
                .show();

            return;
        }

        final Resources res = getResources();

        ( new AlertDialog.Builder( this ) )
            .setTitle( res.getQuantityString( R.plurals.activity_map_file_manager_delete_confirmation_title, selectedMapFilesCount ) )
            .setMessage( res.getQuantityString( R.plurals.activity_map_file_manager_delete_confirmation_message, selectedMapFilesCount, selectedMapFilesCount ) )
            .setPositiveButton( R.string.activity_map_file_manager_delete_confirmation_positive_btn_text,
                                ( dialog, which ) -> {
                                    final List<String> mapFileNames = new ArrayList<>();

                                    for ( int i = 0; i < mapFileListView.getCount(); ++i ) {
                                        if ( mapFileListView.isItemChecked( i ) ) {
                                            mapFileListView.setItemChecked( i, false );

                                            mapFileNames.add( mapFileListViewAdapter.getItem( i ) );
                                        }
                                    }

                                    enqueueBackgroundTask( () -> viewModel.deleteMapFiles( mapFileDir, allowedMapFileExtensions, mapFileNames ) );
                                } )
            .setNegativeButton( R.string.activity_map_file_manager_delete_confirmation_negative_btn_text, ( dialog, which ) -> {} )
            .create()
            .show();
    }

    private void enqueueBackgroundTask( final Runnable task )
    {
        final MapFileManagerActivityViewModel.Status modelStatus = Objects.requireNonNull( viewModel.liveStatus.getValue() );

        if ( modelStatus.isBackgroundTaskExecuting ) {
            backgroundTaskQueue.add( task );
        }
        else {
            task.run();
        }
    }

    private void runNextBackgroundTask( final MapFileManagerActivityViewModel.Status modelStatus )
    {
        if ( modelStatus.isBackgroundTaskExecuting ) {
            return;
        }

        final Runnable task = backgroundTaskQueue.poll();

        if ( task != null ) {
            task.run();
        }
    }

    private void updateUI( final MapFileManagerActivityViewModel.Status modelStatus )
    {
        final ImageButton selectAllButton = findViewById( R.id.activity_map_file_manager_select_all_btn );
        final ImageButton unselectAllButton = findViewById( R.id.activity_map_file_manager_unselect_all_btn );
        final ImageButton importButton = findViewById( R.id.activity_map_file_manager_import_btn );
        final ImageButton exportButton = findViewById( R.id.activity_map_file_manager_export_btn );
        final ImageButton deleteButton = findViewById( R.id.activity_map_file_manager_delete_btn );

        final TextView lastTaskStatusTextView = findViewById( R.id.activity_map_file_manager_last_task_status_lbl );

        final ProgressBar backgroundTaskProgressBar = findViewById( R.id.activity_map_file_manager_background_task_pb );

        mapFileListView.setEnabled( !modelStatus.isBackgroundTaskExecuting );
        selectAllButton.setEnabled( !modelStatus.isBackgroundTaskExecuting );
        unselectAllButton.setEnabled( !modelStatus.isBackgroundTaskExecuting );
        importButton.setEnabled( !modelStatus.isBackgroundTaskExecuting );
        exportButton.setEnabled( !modelStatus.isBackgroundTaskExecuting );
        deleteButton.setEnabled( !modelStatus.isBackgroundTaskExecuting );

        switch ( modelStatus.backgroundTaskResult ) {
        case RESULT_NONE:
            break;
        case RESULT_SUCCESS:
            lastTaskStatusTextView.setText( "" );
            break;
        case RESULT_NO_MAP_FILES:
            lastTaskStatusTextView.setText( getString( R.string.activity_map_file_manager_last_task_status_lbl_text_no_map_files_found ) );
            break;
        case RESULT_ERROR:
            lastTaskStatusTextView.setText(
                String.format( getString( R.string.activity_map_file_manager_last_task_status_lbl_text_failed ), modelStatus.backgroundTaskError ) );
            break;
        default:
            assert false;
            break;
        }

        lastTaskStatusTextView.setVisibility( lastTaskStatusTextView.getText().length() > 0 ? View.VISIBLE : View.GONE );
        backgroundTaskProgressBar.setVisibility( modelStatus.isBackgroundTaskExecuting ? View.VISIBLE : View.GONE );

        mapFileListViewAdapter.clear();
        mapFileListViewAdapter.addAll( modelStatus.mapFileNames );
        mapFileListViewAdapter.notifyDataSetChanged();
    }
}
