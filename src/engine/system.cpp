/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
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

#include "system.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <initializer_list>
#include <system_error>

#if defined( _WIN32 )
#include <clocale>
#include <map>

#include <direct.h>
#include <io.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#if defined( _WIN32 ) || defined( ANDROID )
#include "logging.h"
#else
#include <strings.h>
#endif

#include <SDL_version.h>

#if SDL_VERSION_ATLEAST( 2, 0, 0 ) && defined( ANDROID )
#include <SDL_error.h>
#include <SDL_system.h>
#endif

#if SDL_VERSION_ATLEAST( 2, 0, 1 ) && ( !defined( __linux__ ) || defined( ANDROID ) )
#include <SDL_filesystem.h>
#include <SDL_stdinc.h>
#endif

constexpr auto SEPARATOR = std::filesystem::path::preferred_separator;

namespace
{
#if !defined( __linux__ ) || defined( ANDROID )
    std::string GetHomeDirectory( const std::string & prog )
    {
#if defined( TARGET_PS_VITA )
        return System::concatPath( "ux0:data", prog );
#elif defined( TARGET_NINTENDO_SWITCH )
        return System::concatPath( "/switch", prog );
#elif defined( ANDROID )
        (void)prog;

        const char * storagePath = SDL_AndroidGetExternalStoragePath();
        if ( storagePath == nullptr ) {
            ERROR_LOG( "Failed to obtain the path to external storage. The error: " << SDL_GetError() )
            return { "." };
        }

        VERBOSE_LOG( "Application storage path is " << storagePath )
        return storagePath;
#endif

        const char * homeEnvPath = getenv( "HOME" );

#if defined( MACOS_APP_BUNDLE )
        if ( homeEnvPath != nullptr ) {
            return System::concatPath( System::concatPath( homeEnvPath, "Library/Preferences" ), prog );
        }

        return { "." };
#endif

        if ( homeEnvPath != nullptr ) {
            return System::concatPath( homeEnvPath, std::string( "." ).append( prog ) );
        }

        const char * dataEnvPath = getenv( "APPDATA" );
        if ( dataEnvPath != nullptr ) {
            return System::concatPath( dataEnvPath, prog );
        }

#if SDL_VERSION_ATLEAST( 2, 0, 1 )
        char * path = SDL_GetPrefPath( "", prog.c_str() );
        if ( path ) {
            const std::string result{ path };

            SDL_free( path );

            return result;
        }
#endif

        return { "." };
    }
#endif

    bool globMatch( const std::string_view string, const std::string_view wildcard )
    {
        size_t stringIdx = 0;
        size_t wildcardIdx = 0;

        size_t fallbackStringIdx = std::string_view::npos;
        size_t fallbackWildcardIdx = std::string_view::npos;

        while ( stringIdx < string.length() ) {
            const bool isWildcardNotEnded = ( wildcardIdx < wildcard.length() );

            if ( isWildcardNotEnded && wildcard[wildcardIdx] == '*' ) {
                ++wildcardIdx;

                fallbackStringIdx = stringIdx;
                fallbackWildcardIdx = wildcardIdx;
            }
            else if ( isWildcardNotEnded && ( wildcard[wildcardIdx] == '?' || wildcard[wildcardIdx] == string[stringIdx] ) ) {
                ++stringIdx;
                ++wildcardIdx;
            }
            else {
                if ( fallbackWildcardIdx == std::string_view::npos ) {
                    return false;
                }

                assert( fallbackStringIdx != std::string_view::npos );

                ++fallbackStringIdx;

                stringIdx = fallbackStringIdx;
                wildcardIdx = fallbackWildcardIdx;
            }
        }

        for ( ; wildcardIdx < wildcard.length(); ++wildcardIdx ) {
            if ( wildcard[wildcardIdx] != '*' ) {
                break;
            }
        }

        return wildcardIdx == wildcard.length();
    }

    bool checkFSObject( const std::filesystem::path & path, std::filesystem::file_type type, bool writable )
    {
        if ( path.empty() ) {
            return false;
        }

        std::filesystem::path correctedPath;
        if ( !System::GetCaseInsensitivePath( path, correctedPath ) )
            return false;

        std::filesystem::file_status st = status( correctedPath );
        return ( st.type() == type )
               && ( ( st.permissions() & ( writable ? std::filesystem::perms::owner_write : std::filesystem::perms::owner_read ) ) != std::filesystem::perms::none );
    }
}

bool System::isHandheldDevice()
{
#if defined( ANDROID )
    return true;
#else
    return false;
#endif
}

bool System::isVirtualKeyboardSupported()
{
#if defined( ANDROID ) || defined( TARGET_PS_VITA ) || defined( TARGET_NINTENDO_SWITCH )
    return true;
#else
    return false;
#endif
}

bool System::isShellLevelGlobbingSupported()
{
#if defined( _WIN32 )
    return false;
#else
    return true;
#endif
}

bool System::MakeDirectory( const std::filesystem::path & path )
{
    std::error_code ec;
    return std::filesystem::create_directory( path, ec );
}

std::string System::concatPath( const std::string_view left, const std::string_view right )
{
    // Avoid memory allocation while concatenating string. Allocate needed size at once.
    std::string temp;
    temp.reserve( left.size() + 1 + right.size() );

    temp += left;
    temp += SEPARATOR;
    temp += right;

    return temp;
}

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
#if defined( __linux__ ) && !defined( ANDROID )
    const char * configEnv = getenv( "XDG_CONFIG_HOME" );
    if ( configEnv ) {
        return System::concatPath( configEnv, prog );
    }

    const char * homeEnv = getenv( "HOME" );
    if ( homeEnv ) {
        return System::concatPath( System::concatPath( homeEnv, ".config" ), prog );
    }

    return { "." };
#else
    return GetHomeDirectory( prog );
#endif
}

std::string System::GetDataDirectory( const std::string & prog )
{
#if defined( __linux__ ) && !defined( ANDROID )
    const char * dataEnv = getenv( "XDG_DATA_HOME" );
    if ( dataEnv ) {
        return System::concatPath( dataEnv, prog );
    }

    const char * homeEnv = getenv( "HOME" );
    if ( homeEnv ) {
        return System::concatPath( System::concatPath( homeEnv, ".local/share" ), prog );
    }

    return { "." };
#elif defined( MACOS_APP_BUNDLE )
    const char * homeEnv = getenv( "HOME" );
    if ( homeEnv ) {
        return System::concatPath( System::concatPath( homeEnv, "Library/Application Support" ), prog );
    }

    return { "." };
#else
    return GetHomeDirectory( prog );
#endif
}

std::filesystem::path System::GetDirname( std::filesystem::path path )
{
    if ( path.empty() ) {
        return { "." };
    }

    std::filesystem::path basePath = ( path.has_filename() ? path : path.parent_path() ).parent_path();
    return basePath.empty() ? "." : basePath;
}

std::filesystem::path System::GetBasename( std::filesystem::path path )
{
    if ( path.empty() ) {
        return { "." };
    }

    return ( path.has_filename() ? path : path.parent_path() ).filename();
}

bool System::IsFile( const std::filesystem::path & path, bool writable )
{
    return checkFSObject( path, std::filesystem::file_type::regular, writable );
}

bool System::IsDirectory( const std::filesystem::path & path, bool writable )
{
    return checkFSObject( path, std::filesystem::file_type::directory, writable );
}

bool System::Remove( const std::filesystem::path & path )
{
    std::error_code ec;
    return std::filesystem::remove( path, ec );
}

bool System::GetCaseInsensitivePath( const std::filesystem::path & path, std::filesystem::path & correctedPath )
{
#if !defined( _WIN32 ) && !defined( ANDROID )
    // based on: https://github.com/OneSadCookie/fcaseopen
    correctedPath.clear();

    if ( path.empty() ) {
        return false;
    }

    bool isAbsolute = path.is_absolute();
    std::error_code ec;
    std::filesystem::directory_iterator di;
    if ( isAbsolute ) {
        correctedPath = path.root_path();
        di = std::filesystem::directory_iterator( path.root_path(), ec );
    }
    else {
        di = std::filesystem::directory_iterator( ".", ec );
    }
    if ( ec.value() )
        return false;

    for ( const auto & subPath : ( isAbsolute ? path.relative_path() : path ) ) {
        // Avoid directory traversal and try to probe directory name directly.
        // Speeds up file lookup when intermediate directories have a lot of
        // files. Example is NixOS where file layout is:
        //     /nix/store/...-fheroes2-${ver}/share/fheroes2/files/lang
        //     /nix/store/...-other-package-1/...
        //     /nix/store/...-other-package-2/...
        // It's not uncommon for /nix/store to have tens of thousands files.
        // GetCaseInsensitivePath() calls become very expensive on such systems.
        //
        // The idea is to try to open current subpath as a directory and avoid
        // directory traversal altogether. Otherwise fall back to linear
        // case-insensitive search.
        std::filesystem::path absSubpath = correctedPath / subPath;
        if ( std::filesystem::exists( absSubpath, ec ) && !ec.value() ) {
            correctedPath.swap( absSubpath );
            continue;
        }
        const auto & result = std::find_if( di, end( di ), [&subPath]( const auto & de ) { return strcasecmp( de.path().filename().c_str(), subPath.c_str() ) == 0; } );
        if ( result == end( di ) ) {
            correctedPath /= subPath;
            return false;
        }
        correctedPath /= result->path().filename();
        di = std::filesystem::directory_iterator( correctedPath, ec );
        if ( ec.value() )
            return false;
    }
    return true;
#else
    correctedPath = path;
    return true;
#endif
}

void System::globFiles( const std::string_view glob, std::vector<std::string> & fileNames )
{
    const std::filesystem::path globPath( glob );

    std::filesystem::path dirPath = globPath.parent_path();
    if ( dirPath.empty() ) {
        dirPath = std::filesystem::path{ "." };
    }

    std::error_code ec;

    // Using the non-throwing overload
    if ( !std::filesystem::is_directory( dirPath, ec ) ) {
        fileNames.emplace_back( glob );
        return;
    }

    const std::string pattern = globPath.filename().string();

    if ( pattern.find( '*' ) == std::string_view::npos && pattern.find( '?' ) == std::string_view::npos ) {
        fileNames.emplace_back( glob );
        return;
    }

    bool isNoMatches = true;

    // Using the non-throwing overload
    for ( const std::filesystem::directory_entry & entry : std::filesystem::directory_iterator( dirPath, ec ) ) {
        const std::filesystem::path & entryPath = entry.path();

        if ( globMatch( entryPath.filename().string(), pattern ) ) {
            fileNames.push_back( entryPath.string() );

            isNoMatches = false;
        }
    }

    if ( isNoMatches ) {
        fileNames.emplace_back( glob );
    }
}

std::string System::FileNameToUTF8( const std::string & name )
{
#if defined( _WIN32 )
    if ( name.empty() ) {
        return name;
    }

    thread_local std::map<std::string, std::string> acpToUtf8;

    const auto iter = acpToUtf8.find( name );
    if ( iter != acpToUtf8.end() ) {
        return iter->second;
    }

    // In case of any issues, the original string will be returned, so let's put it to the cache right away
    acpToUtf8[name] = name;

    auto getLastErrorStr = []() {
        LPTSTR msgBuf;

        if ( FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, GetLastError(), 0,
                            reinterpret_cast<LPTSTR>( &msgBuf ), 0, nullptr )
             > 0 ) {
            const std::string result( msgBuf );

            LocalFree( msgBuf );

            return result;
        }

        return std::string( "FormatMessage() failed: " ) + std::to_string( GetLastError() );
    };

    const int wLen = MultiByteToWideChar( CP_ACP, MB_ERR_INVALID_CHARS, name.c_str(), -1, nullptr, 0 );
    if ( wLen <= 0 ) {
        ERROR_LOG( getLastErrorStr() )

        return name;
    }

    const std::unique_ptr<wchar_t[]> wStr( new wchar_t[wLen] );

    if ( MultiByteToWideChar( CP_ACP, MB_ERR_INVALID_CHARS, name.c_str(), -1, wStr.get(), wLen ) != wLen ) {
        ERROR_LOG( getLastErrorStr() )

        return name;
    }

    const int uLen = WideCharToMultiByte( CP_UTF8, WC_ERR_INVALID_CHARS | WC_NO_BEST_FIT_CHARS, wStr.get(), -1, nullptr, 0, nullptr, nullptr );
    if ( uLen <= 0 ) {
        ERROR_LOG( getLastErrorStr() )

        return name;
    }

    const std::unique_ptr<char[]> uStr( new char[uLen] );

    if ( WideCharToMultiByte( CP_UTF8, WC_ERR_INVALID_CHARS | WC_NO_BEST_FIT_CHARS, wStr.get(), -1, uStr.get(), uLen, nullptr, nullptr ) != uLen ) {
        ERROR_LOG( getLastErrorStr() )

        return name;
    }

    const std::string result( uStr.get() );

    // Put the final result to the cache
    acpToUtf8[name] = result;

    return result;
#else
    return name;
#endif
}

tm System::GetTM( const time_t time )
{
    tm result = {};

#if defined( _WIN32 )
    errno_t res = localtime_s( &result, &time );

    if ( res != 0 ) {
        assert( 0 );
    }
#else
    const tm * res = localtime_r( &time, &result );

    if ( res == nullptr ) {
        assert( 0 );
    }
#endif

    return result;
}
