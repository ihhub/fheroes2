package org.fheroes2;

import android.Manifest;
import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.provider.Settings;

public class LauncherActivity extends Activity
{
    private static final int REQUEST_CODE_EXTERNAL_STORAGE_ACCESS = 1001;

    @Override
    protected void onResume()
    {
        super.onResume();

        // On Android 11+ we need to request MANAGE_EXTERNAL_STORAGE permission
        if ( Build.VERSION.SDK_INT >= Build.VERSION_CODES.R ) {
            if ( !Environment.isExternalStorageManager() ) {
                Intent intent = new Intent( Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION );
                intent.setData( Uri.fromParts( "package", getPackageName(), null ) );

                startActivity( intent );

                return;
            }
        }
        // On Android 6 to 10 we need to request READ_EXTERNAL_STORAGE and WRITE_EXTERNAL_STORAGE permissions
        else if ( Build.VERSION.SDK_INT >= Build.VERSION_CODES.M ) {
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
