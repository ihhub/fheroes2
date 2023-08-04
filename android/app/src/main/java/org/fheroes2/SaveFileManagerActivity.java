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
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.file.Files;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Objects;
import java.util.Queue;
import java.util.function.Function;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;
import java.util.zip.ZipOutputStream;

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
import android.widget.ToggleButton;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.appcompat.app.AppCompatActivity;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModel;
import androidx.lifecycle.ViewModelProvider;

import org.apache.commons.io.IOUtils;

public final class SaveFileManagerActivity extends AppCompatActivity
{
    public static final class SaveFileManagerActivityViewModel extends ViewModel
    {
        private static final class Status
        {
            private boolean isBackgroundTaskExecuting;
            private final List<String> saveFileNames;

            private Status( final boolean isBackgroundTaskExecuting, final List<String> saveFileNames )
            {
                this.isBackgroundTaskExecuting = isBackgroundTaskExecuting;
                this.saveFileNames = saveFileNames;
            }

            @SuppressWarnings( "SameParameterValue" )
            private Status setIsBackgroundTaskExecuting( final boolean isBackgroundTaskExecuting )
            {
                this.isBackgroundTaskExecuting = isBackgroundTaskExecuting;

                return this;
            }
        }

        private final MutableLiveData<Status> liveStatus = new MutableLiveData<>( new Status( false, new ArrayList<>() ) );

        private void updateSaveFileList( final File saveFileDir, final List<String> allowedSaveFileExtensions )
        {
            final Status status = Objects.requireNonNull( liveStatus.getValue() );
            assert !status.isBackgroundTaskExecuting;

            liveStatus.setValue( status.setIsBackgroundTaskExecuting( true ) );

            new Thread( () -> {
                try {
                    liveStatus.postValue( new Status( false, getSaveFileList( saveFileDir, allowedSaveFileExtensions ) ) );
                }
                catch ( final Exception ex ) {
                    Log.e( "fheroes2", "Failed to get a list of save files.", ex );

                    liveStatus.postValue( new Status( false, new ArrayList<>() ) );
                }
            } ).start();
        }

        private void importSaveFiles( final File saveFileDir, final List<String> allowedSaveFileExtensions, final Uri zipFileUri, final ContentResolver contentResolver )
        {
            final Status status = Objects.requireNonNull( liveStatus.getValue() );
            assert !status.isBackgroundTaskExecuting;

            liveStatus.setValue( status.setIsBackgroundTaskExecuting( true ) );

            new Thread( () -> {
                try {
                    final Function<String, Boolean> checkExtension = ( String name ) ->
                    {
                        for ( final String extension : allowedSaveFileExtensions ) {
                            if ( name.endsWith( extension ) ) {
                                return true;
                            }
                        }

                        return false;
                    };

                    try ( final InputStream in = contentResolver.openInputStream( zipFileUri ); final ZipInputStream zin = new ZipInputStream( in ) ) {
                        for ( ZipEntry zEntry = zin.getNextEntry(); zEntry != null; zEntry = zin.getNextEntry() ) {
                            if ( zEntry.isDirectory() ) {
                                continue;
                            }

                            final String zEntryFileName = new File( zEntry.getName() ).getName();
                            if ( !checkExtension.apply( zEntryFileName ) ) {
                                continue;
                            }

                            try ( final OutputStream out = Files.newOutputStream( ( new File( saveFileDir, zEntryFileName ) ).toPath() ) ) {
                                IOUtils.copy( zin, out );
                            }
                        }
                    }
                }
                catch ( final Exception ex ) {
                    Log.e( "fheroes2", "Failed to import save files.", ex );
                }
                finally {
                    try {
                        liveStatus.postValue( new Status( false, getSaveFileList( saveFileDir, allowedSaveFileExtensions ) ) );
                    }
                    catch ( final Exception ex ) {
                        Log.e( "fheroes2", "Failed to get a list of save files.", ex );

                        liveStatus.postValue( new Status( false, new ArrayList<>() ) );
                    }
                }
            } ).start();
        }

        private void exportSaveFiles( final File saveFileDir, final List<String> saveFileNames, final Uri zipFileUri, final ContentResolver contentResolver )
        {
            final Status status = Objects.requireNonNull( liveStatus.getValue() );
            assert !status.isBackgroundTaskExecuting;

            liveStatus.setValue( status.setIsBackgroundTaskExecuting( true ) );

            new Thread( () -> {
                try {
                    try ( final OutputStream out = contentResolver.openOutputStream( zipFileUri ); final ZipOutputStream zout = new ZipOutputStream( out ) ) {
                        for ( final String saveFileName : saveFileNames ) {
                            zout.putNextEntry( new ZipEntry( saveFileName ) );

                            try ( final InputStream in = Files.newInputStream( ( new File( saveFileDir, saveFileName ) ).toPath() ) ) {
                                IOUtils.copy( in, zout );
                            }
                        }
                    }
                }
                catch ( final Exception ex ) {
                    Log.e( "fheroes2", "Failed to export save files.", ex );
                }
                finally {
                    liveStatus.postValue( status.setIsBackgroundTaskExecuting( false ) );
                }
            } ).start();
        }

        private void deleteSaveFiles( final File saveFileDir, final List<String> allowedSaveFileExtensions, final List<String> saveFileNames )
        {
            final Status status = Objects.requireNonNull( liveStatus.getValue() );
            assert !status.isBackgroundTaskExecuting;

            liveStatus.setValue( status.setIsBackgroundTaskExecuting( true ) );

            new Thread( () -> {
                try {
                    for ( final String saveFileName : saveFileNames ) {
                        final File saveFile = new File( saveFileDir, saveFileName );

                        Files.deleteIfExists( saveFile.toPath() );
                    }
                }
                catch ( final Exception ex ) {
                    Log.e( "fheroes2", "Failed to delete save files.", ex );
                }
                finally {
                    try {
                        liveStatus.postValue( new Status( false, getSaveFileList( saveFileDir, allowedSaveFileExtensions ) ) );
                    }
                    catch ( final Exception ex ) {
                        Log.e( "fheroes2", "Failed to get a list of save files.", ex );

                        liveStatus.postValue( new Status( false, new ArrayList<>() ) );
                    }
                }
            } ).start();
        }

        private List<String> getSaveFileList( final File saveFileDir, final List<String> allowedSaveFileExtensions )
        {
            final List<String> saveFileNames = new ArrayList<>();

            final File[] saveFilesList = saveFileDir.listFiles( ( dir, name ) -> {
                if ( !dir.equals( saveFileDir ) ) {
                    return false;
                }

                for ( final String extension : allowedSaveFileExtensions ) {
                    if ( name.endsWith( extension ) ) {
                        return true;
                    }
                }

                return false;
            } );

            if ( saveFilesList != null ) {
                for ( final File saveFile : saveFilesList ) {
                    if ( saveFile.isFile() ) {
                        saveFileNames.add( saveFile.getName() );
                    }
                }

                Collections.sort( saveFileNames );
            }

            return saveFileNames;
        }
    }

    private File saveFileDir = null;

    private ToggleButton filterStandardToggleButton = null;
    private ToggleButton filterCampaignToggleButton = null;
    private ToggleButton filterMultiplayerToggleButton = null;

    private ListView saveFileListView = null;
    private ArrayAdapter<String> saveFileListViewAdapter = null;

    private SaveFileManagerActivityViewModel viewModel = null;

    private final ActivityResultLauncher<String> zipFileChooserLauncher = registerForActivityResult( new ActivityResultContracts.GetContent(), result -> {
        // No ZIP file was selected
        if ( result == null ) {
            return;
        }

        for ( int i = 0; i < saveFileListView.getCount(); ++i ) {
            saveFileListView.setItemChecked( i, false );
        }

        enqueueAsyncTask( () -> viewModel.importSaveFiles( saveFileDir, getAllowedSaveFileExtensions(), result, getContentResolver() ) );
    } );

    private final ActivityResultLauncher<String> zipFileLocationChooserLauncher
        = registerForActivityResult( new ActivityResultContracts.CreateDocument( "application/zip" ), result -> {
              // No location was selected
              if ( result == null ) {
                  return;
              }

              final List<String> saveFileNames = new ArrayList<>();

              for ( int i = 0; i < saveFileListView.getCount(); ++i ) {
                  if ( saveFileListView.isItemChecked( i ) ) {
                      saveFileNames.add( saveFileListViewAdapter.getItem( i ) );
                  }
              }

              enqueueAsyncTask( () -> viewModel.exportSaveFiles( saveFileDir, saveFileNames, result, getContentResolver() ) );
          } );

    private final Queue<Runnable> asyncTaskQueue = new ArrayDeque<>();

    @Override
    protected void onCreate( final Bundle savedInstanceState )
    {
        super.onCreate( savedInstanceState );

        setContentView( R.layout.activity_save_file_manager );

        saveFileDir = new File( getExternalFilesDir( null ), "files" + File.separator + "save" );

        filterStandardToggleButton = findViewById( R.id.activity_save_file_manager_filter_standard_btn );
        filterCampaignToggleButton = findViewById( R.id.activity_save_file_manager_filter_campaign_btn );
        filterMultiplayerToggleButton = findViewById( R.id.activity_save_file_manager_filter_multiplayer_btn );

        saveFileListView = findViewById( R.id.activity_save_file_manager_save_file_list );
        saveFileListViewAdapter = new ArrayAdapter<>( this, android.R.layout.simple_list_item_multiple_choice, new ArrayList<>() );

        saveFileListView.setAdapter( saveFileListViewAdapter );
        saveFileListView.setEmptyView( findViewById( R.id.activity_save_file_manager_save_file_list_empty_lbl ) );

        viewModel = new ViewModelProvider( this ).get( SaveFileManagerActivityViewModel.class );
        viewModel.liveStatus.observe( this, this::runNextAsyncTask );
        viewModel.liveStatus.observe( this, this::updateUI );
    }

    @Override
    protected void onResume()
    {
        super.onResume();

        enqueueAsyncTask( () -> viewModel.updateSaveFileList( saveFileDir, getAllowedSaveFileExtensions() ) );
    }

    public void filterButtonClicked( final View view )
    {
        final ToggleButton filterToggleButton = (ToggleButton)view;

        int activeFiltersCount = 0;

        activeFiltersCount += filterStandardToggleButton.isChecked() ? 1 : 0;
        activeFiltersCount += filterCampaignToggleButton.isChecked() ? 1 : 0;
        activeFiltersCount += filterMultiplayerToggleButton.isChecked() ? 1 : 0;

        // Do not allow all filters to be turned off at the same time.
        // TODO: Try disabling the button instead and changing its style so that it doesn't look disabled.
        if ( activeFiltersCount < 1 && !filterToggleButton.isChecked() ) {
            filterToggleButton.setChecked( true );
        }

        for ( int i = 0; i < saveFileListView.getCount(); ++i ) {
            saveFileListView.setItemChecked( i, false );
        }

        enqueueAsyncTask( () -> viewModel.updateSaveFileList( saveFileDir, getAllowedSaveFileExtensions() ) );
    }

    @SuppressWarnings( "java:S1172" ) // SonarQube warning "Remove unused method parameter"
    public void selectAllButtonClicked( final View view )
    {
        for ( int i = 0; i < saveFileListView.getCount(); ++i ) {
            saveFileListView.setItemChecked( i, true );
        }
    }

    @SuppressWarnings( "java:S1172" ) // SonarQube warning "Remove unused method parameter"
    public void unselectAllButtonClicked( final View view )
    {
        for ( int i = 0; i < saveFileListView.getCount(); ++i ) {
            saveFileListView.setItemChecked( i, false );
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
        if ( saveFileListView.getCheckedItemCount() == 0 ) {
            return;
        }

        zipFileLocationChooserLauncher.launch( getString( R.string.activity_save_file_manager_suggested_zip_file_name ) );
    }

    @SuppressWarnings( "java:S1172" ) // SonarQube warning "Remove unused method parameter"
    public void deleteButtonClicked( final View view )
    {
        final int selectedSaveFilesCount = saveFileListView.getCheckedItemCount();

        if ( selectedSaveFilesCount == 0 ) {
            return;
        }

        final Resources res = getResources();

        ( new AlertDialog.Builder( this ) )
            .setTitle( res.getQuantityString( R.plurals.activity_save_file_manager_delete_confirmation_title, selectedSaveFilesCount ) )
            .setMessage( res.getQuantityString( R.plurals.activity_save_file_manager_delete_confirmation_message, selectedSaveFilesCount, selectedSaveFilesCount ) )
            .setPositiveButton( R.string.activity_save_file_manager_delete_confirmation_positive_btn_text,
                                ( dialog, which ) -> {
                                    final List<String> saveFileNames = new ArrayList<>();

                                    for ( int i = 0; i < saveFileListView.getCount(); ++i ) {
                                        if ( saveFileListView.isItemChecked( i ) ) {
                                            saveFileListView.setItemChecked( i, false );

                                            saveFileNames.add( saveFileListViewAdapter.getItem( i ) );
                                        }
                                    }

                                    enqueueAsyncTask( () -> viewModel.deleteSaveFiles( saveFileDir, getAllowedSaveFileExtensions(), saveFileNames ) );
                                } )
            .setNegativeButton( R.string.activity_save_file_manager_delete_confirmation_negative_btn_text, ( dialog, which ) -> {} )
            .create()
            .show();
    }

    private List<String> getAllowedSaveFileExtensions()
    {
        final List<String> allowedSaveFileExtensions = new ArrayList<>();

        if ( filterStandardToggleButton.isChecked() ) {
            allowedSaveFileExtensions.add( ".sav" );
        }
        if ( filterCampaignToggleButton.isChecked() ) {
            allowedSaveFileExtensions.add( ".savc" );
        }
        if ( filterMultiplayerToggleButton.isChecked() ) {
            allowedSaveFileExtensions.add( ".savh" );
        }

        return allowedSaveFileExtensions;
    }

    private void enqueueAsyncTask( final Runnable task )
    {
        final SaveFileManagerActivityViewModel.Status modelStatus = Objects.requireNonNull( viewModel.liveStatus.getValue() );

        if ( modelStatus.isBackgroundTaskExecuting ) {
            asyncTaskQueue.add( task );
        }
        else {
            task.run();
        }
    }

    private void runNextAsyncTask( final SaveFileManagerActivityViewModel.Status modelStatus )
    {
        if ( modelStatus.isBackgroundTaskExecuting ) {
            return;
        }

        final Runnable task = asyncTaskQueue.poll();

        if ( task != null ) {
            task.run();
        }
    }

    private void updateUI( final SaveFileManagerActivityViewModel.Status modelStatus )
    {
        final ImageButton selectAllButton = findViewById( R.id.activity_save_file_manager_select_all_btn );
        final ImageButton unselectAllButton = findViewById( R.id.activity_save_file_manager_unselect_all_btn );
        final ImageButton importButton = findViewById( R.id.activity_save_file_manager_import_btn );
        final ImageButton exportButton = findViewById( R.id.activity_save_file_manager_export_btn );
        final ImageButton deleteButton = findViewById( R.id.activity_save_file_manager_delete_btn );

        final ProgressBar backgroundTaskProgressBar = findViewById( R.id.activity_save_file_manager_background_task_pb );

        filterStandardToggleButton.setEnabled( !modelStatus.isBackgroundTaskExecuting );
        filterCampaignToggleButton.setEnabled( !modelStatus.isBackgroundTaskExecuting );
        filterMultiplayerToggleButton.setEnabled( !modelStatus.isBackgroundTaskExecuting );
        saveFileListView.setEnabled( !modelStatus.isBackgroundTaskExecuting );
        selectAllButton.setEnabled( !modelStatus.isBackgroundTaskExecuting );
        unselectAllButton.setEnabled( !modelStatus.isBackgroundTaskExecuting );
        importButton.setEnabled( !modelStatus.isBackgroundTaskExecuting );
        exportButton.setEnabled( !modelStatus.isBackgroundTaskExecuting );
        deleteButton.setEnabled( !modelStatus.isBackgroundTaskExecuting );

        backgroundTaskProgressBar.setVisibility( modelStatus.isBackgroundTaskExecuting ? View.VISIBLE : View.GONE );

        saveFileListViewAdapter.clear();
        saveFileListViewAdapter.addAll( modelStatus.saveFileNames );
        saveFileListViewAdapter.notifyDataSetChanged();
    }
}
