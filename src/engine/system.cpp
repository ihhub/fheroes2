/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
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

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <initializer_list>
#include <map>
#include <system_error>
#include <utility>

#if defined( _WIN32 )
#include <tuple>

#include "tools.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined( TARGET_PS_VITA )
#include <algorithm>
#endif

#if !defined( _WIN32 ) && !defined( ANDROID )
#include <dirent.h>
#include <strings.h>
#endif

// Managing compiler warnings for SDL headers
#if defined( __GNUC__ )
#pragma GCC diagnostic push

#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wswitch-default"
#endif

#include <SDL_version.h>

#if defined( ANDROID )
#include <SDL_error.h>
#include <SDL_system.h>
#endif

#if SDL_VERSION_ATLEAST( 2, 0, 1 ) && ( !defined( __linux__ ) || defined( ANDROID ) )
#include <memory>

#include <SDL_filesystem.h>
#include <SDL_stdinc.h>
#endif

// Managing compiler warnings for SDL headers
#if defined( __GNUC__ )
#pragma GCC diagnostic pop
#endif

#if defined( _WIN32 )
#define SEPARATOR '\\'
#else
#define SEPARATOR '/'
#endif

namespace
{
#if !defined( __linux__ ) || defined( ANDROID )
    std::string GetHomeDirectory( const std::string_view appName )
    {
#if defined( TARGET_PS_VITA )
        return System::concatPath( "ux0:data", appName );
#elif defined( TARGET_NINTENDO_SWITCH )
        return System::concatPath( "/switch", appName );
#elif defined( ANDROID )
        (void)appName;

        if ( const char * storagePath = SDL_AndroidGetExternalStoragePath(); storagePath != nullptr ) {
            return storagePath;
        }

        return { "." };
#endif
        {
            const char * homeEnvPath = getenv( "HOME" );

#if defined( MACOS_APP_BUNDLE )
            if ( homeEnvPath != nullptr ) {
                return System::concatPath( System::concatPath( homeEnvPath, "Library/Preferences" ), appName );
            }

            return { "." };
#endif

            if ( homeEnvPath != nullptr ) {
                return System::concatPath( homeEnvPath, std::string( "." ).append( appName ) );
            }
        }

        if ( const char * dataEnvPath = getenv( "APPDATA" ); dataEnvPath != nullptr ) {
            return System::concatPath( dataEnvPath, appName );
        }

#if SDL_VERSION_ATLEAST( 2, 0, 1 )
        if ( const std::unique_ptr<char, void ( * )( void * )> path( SDL_GetPrefPath( "", System::encLocalToSDL( std::string{ appName } ).c_str() ), SDL_free ); path ) {
            return System::encSDLToLocal( path.get() );
        }
#endif

        return { "." };
    }
#endif

#if !defined( _WIN32 ) && !defined( ANDROID )
    std::vector<std::string> splitUnixPath( const std::string_view path, const std::string_view delimiter )
    {
        std::vector<std::string> result;

        if ( path.empty() ) {
            return result;
        }

        size_t pos = 0;

        while ( pos < path.size() ) {
            const size_t nextPos = path.find( delimiter, pos );

            if ( nextPos == std::string::npos ) {
                result.emplace_back( path.substr( pos ) );

                break;
            }
            if ( pos < nextPos ) {
                result.emplace_back( path.substr( pos, nextPos - pos ) );
            }

            pos = nextPos + delimiter.size();
        }

        return result;
    }
#endif

    std::string_view trimTrailingSeparators( std::string_view path )
    {
        while ( path.size() > 1 && path.back() == SEPARATOR ) {
            path.remove_suffix( 1 );
        }

        return path;
    }

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

#if defined( _WIN32 )
    enum class EncodingConversionDirection
    {
        ACPToUTF8,
        UTF8ToACP
    };

    std::string convertBetweenACPAndUTF8( const std::string_view str, const EncodingConversionDirection dir )
    {
        if ( str.empty() ) {
            return {};
        }

        thread_local std::map<EncodingConversionDirection, std::map<std::string, std::string, std::less<>>> resultsCache;

        if ( const auto dirIter = resultsCache.find( dir ); dirIter != resultsCache.end() ) {
            const auto & strMap = dirIter->second;

            if ( const auto strIter = strMap.find( str ); strIter != strMap.end() ) {
                return strIter->second;
            }
        }

        // In case of any issues, the original string will be returned, so let's put it to the cache right away
        const auto [resultIter, inserted] = resultsCache[dir].emplace( str, str );
        if ( !inserted ) {
            assert( 0 );
        }

        const auto strLen = fheroes2::checkedCast<int>( str.size() );
        if ( !strLen ) {
            // The size of this string does not fit into an int, so this string cannot be safely converted
#ifdef WITH_DEBUG
            assert( 0 );
#endif
            return std::string{ str };
        }

        const auto [mbCodePage, mbFlags, wcCodePage, wcFlags] = [dir]() -> std::tuple<UINT, DWORD, UINT, DWORD> {
            switch ( dir ) {
            case EncodingConversionDirection::ACPToUTF8:
                return { CP_ACP, MB_ERR_INVALID_CHARS, CP_UTF8, WC_ERR_INVALID_CHARS };
            case EncodingConversionDirection::UTF8ToACP:
                return { CP_UTF8, MB_ERR_INVALID_CHARS, CP_ACP, WC_NO_BEST_FIT_CHARS };
            default:
                assert( 0 );
                break;
            }

            return { CP_ACP, MB_ERR_INVALID_CHARS, CP_ACP, WC_NO_BEST_FIT_CHARS };
        }();

        const int wcLen = MultiByteToWideChar( mbCodePage, mbFlags, str.data(), *strLen, nullptr, 0 );
        if ( wcLen <= 0 ) {
#ifdef WITH_DEBUG
            assert( 0 );
#endif
            return std::string{ str };
        }

        // The contents of this buffer will not be zero-terminated
        const std::unique_ptr<wchar_t[]> wcStr( new wchar_t[wcLen] );

        if ( MultiByteToWideChar( mbCodePage, mbFlags, str.data(), *strLen, wcStr.get(), wcLen ) != wcLen ) {
#ifdef WITH_DEBUG
            assert( 0 );
#endif
            return std::string{ str };
        }

        const int mbLen = WideCharToMultiByte( wcCodePage, wcFlags, wcStr.get(), wcLen, nullptr, 0, nullptr, nullptr );
        if ( mbLen <= 0 ) {
#ifdef WITH_DEBUG
            assert( 0 );
#endif
            return std::string{ str };
        }

        // The contents of this buffer will not be zero-terminated
        const std::unique_ptr<char[]> mbStr( new char[mbLen] );

        if ( WideCharToMultiByte( wcCodePage, wcFlags, wcStr.get(), wcLen, mbStr.get(), mbLen, nullptr, nullptr ) != mbLen ) {
#ifdef WITH_DEBUG
            assert( 0 );
#endif
            return std::string{ str };
        }

        // The contents of this buffer is not zero-terminated
        std::string result( mbStr.get(), mbLen );

        // Put the final result to the cache
        resultIter->second = result;

        return result;
    }
#endif
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

bool System::MakeDirectory( const std::string_view path )
{
    std::error_code ec;

    // Using the non-throwing overload
    return std::filesystem::create_directories( path, ec );
}

bool System::Unlink( const std::string_view path )
{
    std::error_code ec;

    // Using the non-throwing overload
    return std::filesystem::remove( path, ec );
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

std::string System::GetConfigDirectory( const std::string_view appName )
{
    // Location of the app config directory, in theory, should not change while the app is running and can be cached
    thread_local std::map<std::string, std::string, std::less<>> resultsCache;

    if ( const auto iter = resultsCache.find( appName ); iter != resultsCache.end() ) {
        return iter->second;
    }

    std::string result = [&appName]() -> std::string {
#if defined( __linux__ ) && !defined( ANDROID )
        if ( const char * configEnv = getenv( "XDG_CONFIG_HOME" ); configEnv != nullptr ) {
            return System::concatPath( configEnv, appName );
        }

        if ( const char * homeEnv = getenv( "HOME" ); homeEnv != nullptr ) {
            return System::concatPath( System::concatPath( homeEnv, ".config" ), appName );
        }

        return { "." };
#else
        return GetHomeDirectory( appName );
#endif
    }();

    const auto [dummy, inserted] = resultsCache.emplace( appName, result );
    if ( !inserted ) {
        assert( 0 );
    }

    return result;
}

std::string System::GetDataDirectory( const std::string_view appName )
{
    // Location of the app data directory, in theory, should not change while the app is running and can be cached
    thread_local std::map<std::string, std::string, std::less<>> resultsCache;

    if ( const auto iter = resultsCache.find( appName ); iter != resultsCache.end() ) {
        return iter->second;
    }

    std::string result = [&appName]() -> std::string {
#if defined( __linux__ ) && !defined( ANDROID )
        if ( const char * dataEnv = getenv( "XDG_DATA_HOME" ); dataEnv != nullptr ) {
            return System::concatPath( dataEnv, appName );
        }

        if ( const char * homeEnv = getenv( "HOME" ); homeEnv != nullptr ) {
            return System::concatPath( System::concatPath( homeEnv, ".local/share" ), appName );
        }

        return { "." };
#elif defined( MACOS_APP_BUNDLE )
        if ( const char * homeEnv = getenv( "HOME" ); homeEnv != nullptr ) {
            return System::concatPath( System::concatPath( homeEnv, "Library/Application Support" ), appName );
        }

        return { "." };
#else
        return GetHomeDirectory( appName );
#endif
    }();

    const auto [dummy, inserted] = resultsCache.emplace( appName, result );
    if ( !inserted ) {
        assert( 0 );
    }

    return result;
}

std::string System::GetDirname( std::string_view path )
{
    if ( path.empty() ) {
        return { "." };
    }

    path = trimTrailingSeparators( path );

    const size_t pos = path.rfind( SEPARATOR );

    if ( pos == std::string::npos ) {
        return { "." };
    }
    if ( pos == 0 ) {
        return { std::initializer_list<char>{ SEPARATOR } };
    }

    // Trailing separators should already be trimmed
    assert( pos != path.size() - 1 );

    return std::string{ trimTrailingSeparators( path.substr( 0, pos ) ) };
}

std::string System::GetBasename( std::string_view path )
{
    if ( path.empty() ) {
        return { "." };
    }

    path = trimTrailingSeparators( path );

    const size_t pos = path.rfind( SEPARATOR );

    if ( pos == std::string::npos || ( pos == 0 && path.size() == 1 ) ) {
        return std::string{ path };
    }

    // Trailing separators should already be trimmed
    assert( pos != path.size() - 1 );

    return std::string{ path.substr( pos + 1 ) };
}

std::string System::GetStem( const std::string_view path )
{
    std::string res = GetBasename( path );

    const size_t pos = res.rfind( '.' );

    if ( pos != 0 && pos != std::string::npos ) {
        res.resize( pos );
    }

    return res;
}

bool System::IsFile( const std::string_view path )
{
    if ( path.empty() ) {
        // An empty path cannot be a file.
        return false;
    }

    std::string correctedPath;
    if ( !GetCaseInsensitivePath( path, correctedPath ) ) {
        return false;
    }

    std::error_code ec;

    // Using the non-throwing overload
    return std::filesystem::is_regular_file( correctedPath, ec );
}

bool System::IsDirectory( const std::string_view path )
{
    if ( path.empty() ) {
        // An empty path cannot be a directory.
        return false;
    }

    std::string correctedPath;
    if ( !GetCaseInsensitivePath( path, correctedPath ) ) {
        return false;
    }

    std::error_code ec;

    // Using the non-throwing overload
    return std::filesystem::is_directory( correctedPath, ec );
}

bool System::GetCaseInsensitivePath( const std::string_view path, std::string & correctedPath )
{
#if !defined( _WIN32 ) && !defined( ANDROID )
    // based on: https://github.com/OneSadCookie/fcaseopen
    correctedPath.clear();

    if ( path.empty() ) {
        return false;
    }

    DIR * d;
    bool last = false;
    const char * curDir = ".";
    const char * delimiter = "/";

    if ( path[0] == delimiter[0] ) {
        correctedPath.append( delimiter );

        d = opendir( delimiter );
    }
    else {
        d = opendir( curDir );
    }

    const std::vector<std::string> splittedPath = splitUnixPath( path, delimiter );
    for ( std::vector<std::string>::const_iterator subPathIter = splittedPath.begin(); subPathIter != splittedPath.end(); ++subPathIter ) {
        if ( !d ) {
            return false;
        }

        if ( last ) {
            closedir( d );
            return false;
        }

        if ( subPathIter != splittedPath.begin() ) {
            correctedPath.append( delimiter );
        }

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

        std::string absSubpath = correctedPath + *subPathIter;
        DIR * de = opendir( absSubpath.c_str() );
        if ( de ) {
            correctedPath = std::move( absSubpath );

            closedir( d );
            d = opendir( correctedPath.c_str() );

            closedir( de );
            continue;
        }

        const struct dirent * e = readdir( d );
        while ( e ) {
            if ( strcasecmp( subPathIter->c_str(), e->d_name ) == 0 ) {
                correctedPath += e->d_name;

                closedir( d );
                d = opendir( correctedPath.c_str() );

                break;
            }

            e = readdir( d );
        }

        if ( !e ) {
            correctedPath += *subPathIter;
            last = true;
        }
    }

    if ( d ) {
        closedir( d );
    }

    return !last;
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

std::string System::encLocalToSDL( const std::string_view str )
{
#if defined( _WIN32 )
    return convertBetweenACPAndUTF8( str, EncodingConversionDirection::ACPToUTF8 );
#else
    return std::string{ str };
#endif
}

std::string System::encSDLToLocal( const std::string_view str )
{
#if defined( _WIN32 )
    return convertBetweenACPAndUTF8( str, EncodingConversionDirection::UTF8ToACP );
#else
    return std::string{ str };
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
