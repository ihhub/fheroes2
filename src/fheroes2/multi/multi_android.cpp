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

#include "multi_android.h"

#if defined( ANDROID )
#include <jni.h>

// Managing compiler warnings for SDL headers
#if defined( __GNUC__ )
#pragma GCC diagnostic push

#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wswitch-default"
#endif

#include <SDL_system.h>

// Managing compiler warnings for SDL headers
#if defined( __GNUC__ )
#pragma GCC diagnostic pop
#endif

#include "logging.h"
#endif

void Multi::setNetworkGameRunning( const bool isRunning )
{
#if defined( ANDROID )
    static bool isServiceRunning = false;
    if ( isRunning == isServiceRunning ) {
        return;
    }

    JNIEnv * env = static_cast<JNIEnv *>( SDL_AndroidGetJNIEnv() );
    jobject activity = static_cast<jobject>( SDL_AndroidGetActivity() );
    if ( env == nullptr || activity == nullptr ) {
        return;
    }

    // The class loader of this thread may not know GameActivity, take the class from the activity.
    jclass activityClass = env->GetObjectClass( activity );
    const jmethodID method = env->GetStaticMethodID( activityClass, "setNetworkGameRunning", "(Z)V" );

    if ( method != nullptr ) {
        env->CallStaticVoidMethod( activityClass, method, static_cast<jboolean>( isRunning ? JNI_TRUE : JNI_FALSE ) );
        isServiceRunning = isRunning;
    }

    if ( env->ExceptionCheck() ) {
        env->ExceptionClear();
        ERROR_LOG( "[MULTI] Unable to tell Android about the network game" )
    }

    env->DeleteLocalRef( activityClass );
    env->DeleteLocalRef( activity );
#else
    (void)isRunning;
#endif
}
