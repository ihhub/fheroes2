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

import android.Manifest;
import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;

public class LauncherActivity extends Activity
{
    private static final int REQUEST_CODE_EXTERNAL_STORAGE_ACCESS = 1001;

    @Override
    protected void onResume()
    {
        super.onResume();

        // On Android 6 to 9 we need to request READ_EXTERNAL_STORAGE and WRITE_EXTERNAL_STORAGE permissions
        if ( Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && Build.VERSION.SDK_INT < Build.VERSION_CODES.Q ) {
            if ( checkSelfPermission( Manifest.permission.READ_EXTERNAL_STORAGE ) != PackageManager.PERMISSION_GRANTED
                 || checkSelfPermission( Manifest.permission.WRITE_EXTERNAL_STORAGE ) != PackageManager.PERMISSION_GRANTED ) {
                requestPermissions( new String[] { Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.WRITE_EXTERNAL_STORAGE },
                                    REQUEST_CODE_EXTERNAL_STORAGE_ACCESS );

                return;
            }
        }

        startMainActivity();
    }

    @Override
    protected void onActivityResult( int requestCode, int resultCode, Intent data )
    {
        super.onActivityResult( requestCode, resultCode, data );

        if ( requestCode == REQUEST_CODE_EXTERNAL_STORAGE_ACCESS ) {
            if ( Build.VERSION.SDK_INT >= Build.VERSION_CODES.M ) {
                if ( checkSelfPermission( Manifest.permission.READ_EXTERNAL_STORAGE ) == PackageManager.PERMISSION_GRANTED
                     && checkSelfPermission( Manifest.permission.WRITE_EXTERNAL_STORAGE ) == PackageManager.PERMISSION_GRANTED ) {
                    startMainActivity();
                }
            }
        }
    }

    private void startMainActivity()
    {
        startActivity( new Intent( this, MainActivity.class ) );

        // Remove this Activity from history to avoid it being resumed when the MainActivity is finished
        finish();
    }
}
