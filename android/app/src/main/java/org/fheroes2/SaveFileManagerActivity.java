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

import android.app.Activity;
import android.app.AlertDialog;
import android.content.pm.ActivityInfo;
import android.content.res.Resources;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.ImageButton;
import android.widget.ListView;
import android.widget.ToggleButton;
import java.io.File;
import java.util.ArrayList;
import java.util.Collections;

public final class SaveFileManagerActivity extends Activity
{
    private File saveFileDir = null;

    private ToggleButton filterStandardToggleButton = null;
    private ToggleButton filterCampaignToggleButton = null;
    private ToggleButton filterMultiplayerToggleButton = null;

    private ListView saveFileListView = null;

    private ImageButton selectAllButton = null;
    private ImageButton unselectAllButton = null;
    private ImageButton deleteButton = null;

    private ArrayAdapter<String> saveFileListViewAdapter = null;

    private Thread backgroundTask = null;

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

        selectAllButton = findViewById( R.id.activity_save_file_manager_select_all_btn );
        unselectAllButton = findViewById( R.id.activity_save_file_manager_unselect_all_btn );
        deleteButton = findViewById( R.id.activity_save_file_manager_delete_btn );

        saveFileListViewAdapter = new ArrayAdapter<>( this, android.R.layout.simple_list_item_multiple_choice, new ArrayList<>() );

        saveFileListView.setAdapter( saveFileListViewAdapter );
        saveFileListView.setEmptyView( findViewById( R.id.activity_save_file_manager_save_file_list_empty_lbl ) );
    }

    @Override
    protected void onResume()
    {
        super.onResume();

        updateSaveFileList();
    }

    public void filterButtonClicked( final View view )
    {
        final ToggleButton filterToggleButton = (ToggleButton)view;

        int activeFiltersCount = 0;

        activeFiltersCount += filterStandardToggleButton.isChecked() ? 1 : 0;
        activeFiltersCount += filterCampaignToggleButton.isChecked() ? 1 : 0;
        activeFiltersCount += filterMultiplayerToggleButton.isChecked() ? 1 : 0;

        // Do not allow all filters to be turned off at the same time.
        // TODO: Try disabling the button instead by changing its style so that it doesn't look disabled.
        if ( activeFiltersCount < 1 && !filterToggleButton.isChecked() ) {
            filterToggleButton.setChecked( true );
        }

        for ( int i = 0; i < saveFileListView.getCount(); ++i ) {
            saveFileListView.setItemChecked( i, false );
        }

        updateSaveFileList();
    }

    public void selectAllButtonClicked( final View view )
    {
        for ( int i = 0; i < saveFileListView.getCount(); ++i ) {
            saveFileListView.setItemChecked( i, true );
        }
    }

    public void unselectAllButtonClicked( final View view )
    {
        for ( int i = 0; i < saveFileListView.getCount(); ++i ) {
            saveFileListView.setItemChecked( i, false );
        }
    }

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
            .setPositiveButton( R.string.activity_save_file_manager_delete_confirmation_positive_button_text,
                                ( dialog, which ) -> {
                                    final ArrayList<String> saveFileNames = new ArrayList<>();

                                    for ( int i = 0; i < saveFileListView.getCount(); ++i ) {
                                        if ( saveFileListView.isItemChecked( i ) ) {
                                            saveFileListView.setItemChecked( i, false );

                                            saveFileNames.add( saveFileListViewAdapter.getItem( i ) );
                                        }
                                    }

                                    deleteSaveFiles( saveFileNames );
                                } )
            .setNegativeButton( R.string.activity_save_file_manager_delete_confirmation_negative_button_text, ( dialog, which ) -> {} )
            .create()
            .show();
    }

    private void updateSaveFileList()
    {
        if ( backgroundTask != null ) {
            return;
        }

        final ArrayList<String> allowedSaveFileExtensions = new ArrayList<>();

        if ( filterStandardToggleButton.isChecked() ) {
            allowedSaveFileExtensions.add( ".sav" );
        }
        if ( filterCampaignToggleButton.isChecked() ) {
            allowedSaveFileExtensions.add( ".savc" );
        }
        if ( filterMultiplayerToggleButton.isChecked() ) {
            allowedSaveFileExtensions.add( ".savh" );
        }

        backgroundTask = new Thread( () -> {
            try {
                final ArrayList<String> saveFileNames = new ArrayList<>();

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

                runOnUiThread( () -> {
                    saveFileListViewAdapter.clear();
                    saveFileListViewAdapter.addAll( saveFileNames );
                    saveFileListViewAdapter.notifyDataSetChanged();
                } );
            }
            catch ( final Exception ex ) {
                Log.e( "fheroes2", "Failed to get a list of save files.", ex );
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

    private void deleteSaveFiles( final ArrayList<String> saveFileNames )
    {
        if ( saveFileNames.isEmpty() ) {
            return;
        }

        if ( backgroundTask != null ) {
            return;
        }

        backgroundTask = new Thread( () -> {
            try {
                for ( final String saveFileName : saveFileNames ) {
                    final File saveFile = new File( saveFileDir, saveFileName );

                    if ( !saveFile.isFile() ) {
                        continue;
                    }

                    if ( !saveFile.delete() ) {
                        Log.e( "fheroes2", "Unable to delete save file " + saveFile.getCanonicalPath() );
                    }
                }
            }
            catch ( final Exception ex ) {
                Log.e( "fheroes2", "Failed to delete save files.", ex );
            }
            finally {
                runOnUiThread( () -> {
                    backgroundTask = null;

                    updateSaveFileList();
                } );
            }
        } );

        updateUI();

        backgroundTask.start();
    }

    private void updateUI()
    {
        // A quick and dirty way to avoid the re-creation of this activity due to the screen orientation change while running a background task
        setRequestedOrientation( backgroundTask == null ? ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED : ActivityInfo.SCREEN_ORIENTATION_LOCKED );

        filterStandardToggleButton.setEnabled( backgroundTask == null );
        filterCampaignToggleButton.setEnabled( backgroundTask == null );
        filterMultiplayerToggleButton.setEnabled( backgroundTask == null );
        saveFileListView.setEnabled( backgroundTask == null );
        selectAllButton.setEnabled( backgroundTask == null );
        unselectAllButton.setEnabled( backgroundTask == null );
        deleteButton.setEnabled( backgroundTask == null );
    }
}
