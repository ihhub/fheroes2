/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2022                                                    *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <cstdlib>
#include <string>
#include <vector>

#include "system.h"
#include "system_dirs.h"

#include <SDL.h>

#if !defined( __LINUX__ )
namespace
{
    std::string GetHomeDirectory( const std::string & prog )
    {
#if defined( TARGET_PS_VITA )
        return "ux0:data/fheroes2";
#elif defined( TARGET_NINTENDO_SWITCH )
        return "/switch/fheroes2";
#endif

        const char * homeEnvPath = std::getenv( "HOME" );

#if defined( MACOS_APP_BUNDLE )
        if ( homeEnvPath != nullptr ) {
            return System::ConcatePath( System::ConcatePath( homeEnvPath, "Library/Preferences" ), prog );
        }

        return {};
#endif

        if ( homeEnvPath != nullptr ) {
            return System::ConcatePath( homeEnvPath, std::string( "." ).append( prog ) );
        }

        const char * dataEnvPath = getenv( "APPDATA" );
        if ( dataEnvPath != nullptr ) {
            return System::ConcatePath( dataEnvPath, prog );
        }

        std::string res;
#if SDL_VERSION_ATLEAST( 2, 0, 0 )
        char * path = SDL_GetPrefPath( "", prog.c_str() );
        if ( path ) {
            res = path;
            SDL_free( path );
        }
#endif
        return res;
    }
}
#endif

void System::appendOSSpecificDirectories( std::vector<std::string> & directories )
{
#if defined( TARGET_PS_VITA )
    const char * path = "ux0:app/FHOMM0002";
    if ( std::find( directories.begin(), directories.end(), path ) == directories.end() ) {
        directories.emplace_back( path );
    }
#else
    (void)directories;
#endif
}

std::string System::GetConfigDirectory( const std::string & prog )
{
#if defined( __LINUX__ )
    const char * configEnv = std::getenv( "XDG_CONFIG_HOME" );
    if ( configEnv ) {
        return System::ConcatePath( configEnv, prog );
    }

    const char * homeEnv = std::getenv( "HOME" );
    if ( homeEnv ) {
        return System::ConcatePath( System::ConcatePath( homeEnv, ".config" ), prog );
    }

    return std::string();
#else
    return GetHomeDirectory( prog );
#endif
}

std::string System::GetDataDirectory( const std::string & prog )
{
#if defined( __LINUX__ )
    const char * dataEnv = std::getenv( "XDG_DATA_HOME" );
    if ( dataEnv ) {
        return System::ConcatePath( dataEnv, prog );
    }

    const char * homeEnv = std::getenv( "HOME" );
    if ( homeEnv ) {
        return System::ConcatePath( System::ConcatePath( homeEnv, ".local/share" ), prog );
    }

    return {};
#elif defined( MACOS_APP_BUNDLE )
    const char * homeEnv = std::getenv( "HOME" );
    if ( homeEnv ) {
        return System::ConcatePath( System::ConcatePath( homeEnv, "Library/Application Support" ), prog );
    }

    return {};
#else
    return GetHomeDirectory( prog );
#endif
}
