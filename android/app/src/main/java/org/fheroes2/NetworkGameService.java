/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2026                                                    *
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

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ServiceInfo;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.IBinder;
import android.os.PowerManager;

// Runs in the game process during a network game. Android keeps the connection alive while the game is in the background.
public final class NetworkGameService extends Service
{
    private static final String CHANNEL_ID = "network_game";
    private static final int NOTIFICATION_ID = 1;

    private PowerManager.WakeLock wakeLock = null;
    private WifiManager.WifiLock wifiLock = null;

    @Override
    public int onStartCommand( final Intent intent, final int flags, final int startId )
    {
        final Notification notification = makeNotification();

        if ( Build.VERSION.SDK_INT >= Build.VERSION_CODES.UPSIDE_DOWN_CAKE ) {
            startForeground( NOTIFICATION_ID, notification, ServiceInfo.FOREGROUND_SERVICE_TYPE_SPECIAL_USE );
        }
        else {
            startForeground( NOTIFICATION_ID, notification );
        }

        acquireLocks();

        return START_NOT_STICKY;
    }

    @Override
    public void onDestroy()
    {
        releaseLocks();

        super.onDestroy();
    }

    @Override
    public IBinder onBind( final Intent intent )
    {
        return null;
    }

    @SuppressWarnings( "deprecation" )
    private Notification makeNotification()
    {
        // Touching the notification brings the game back.
        final Intent gameIntent = new Intent( this, GameActivity.class );
        final PendingIntent pendingIntent = PendingIntent.getActivity( this, 0, gameIntent, PendingIntent.FLAG_IMMUTABLE | PendingIntent.FLAG_UPDATE_CURRENT );

        final Notification.Builder builder;

        if ( Build.VERSION.SDK_INT >= Build.VERSION_CODES.O ) {
            final NotificationManager manager = (NotificationManager)getSystemService( Context.NOTIFICATION_SERVICE );
            if ( manager != null ) {
                manager.createNotificationChannel(
                    new NotificationChannel( CHANNEL_ID, getString( R.string.multi_network_game_channel ), NotificationManager.IMPORTANCE_LOW ) );
            }

            builder = new Notification.Builder( this, CHANNEL_ID );
        }
        else {
            builder = new Notification.Builder( this );
        }

        return builder.setSmallIcon( R.mipmap.ic_launcher )
            .setContentTitle( getString( R.string.multi_network_game_notification_title ) )
            .setContentText( getString( R.string.multi_network_game_notification_text ) )
            .setContentIntent( pendingIntent )
            .setOngoing( true )
            .build();
    }

    @SuppressWarnings( "deprecation" )
    private void acquireLocks()
    {
        // The system keeps answering the other computer in the background. The processor and the Wi-Fi must stay awake.
        final PowerManager powerManager = (PowerManager)getSystemService( Context.POWER_SERVICE );
        if ( powerManager != null && wakeLock == null ) {
            wakeLock = powerManager.newWakeLock( PowerManager.PARTIAL_WAKE_LOCK, "fheroes2:network_game" );
            wakeLock.acquire();
        }

        final WifiManager wifiManager = (WifiManager)getApplicationContext().getSystemService( Context.WIFI_SERVICE );
        if ( wifiManager != null && wifiLock == null ) {
            // The low latency mode works only while the screen is on.
            wifiLock = wifiManager.createWifiLock( WifiManager.WIFI_MODE_FULL_HIGH_PERF, "fheroes2:network_game" );
            wifiLock.acquire();
        }
    }

    private void releaseLocks()
    {
        if ( wakeLock != null && wakeLock.isHeld() ) {
            wakeLock.release();
        }
        wakeLock = null;

        if ( wifiLock != null && wifiLock.isHeld() ) {
            wifiLock.release();
        }
        wifiLock = null;
    }
}
