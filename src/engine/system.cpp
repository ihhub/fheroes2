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
#include <functional>
#include <map>
#include <memory>
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

#if !defined( _WIN32 ) && !defined( ANDROID ) && !defined( TARGET_PS_VITA )
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

#include <SDL_touch.h>
#include <SDL_version.h>

#if defined( ANDROID )
#include <SDL_error.h>
#include <SDL_system.h>
#endif

#if SDL_VERSION_ATLEAST( 2, 0, 1 ) && ( !defined( __linux__ ) || defined( ANDROID ) )
#include <SDL_filesystem.h>
#include <SDL_stdinc.h>
#endif

// Managing compiler warnings for SDL headers
#if defined( __GNUC__ )
#pragma GCC diagnostic pop
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

        return {};
#endif
        {
            const char * homeEnvPath = getenv( "HOME" );

#if defined( MACOS_APP_BUNDLE )
            if ( homeEnvPath != nullptr ) {
                return System::concatPath( System::concatPath( homeEnvPath, "Library/Preferences" ), appName );
            }

            return {};
#endif

            if ( homeEnvPath != nullptr ) {
                return System::concatPath( homeEnvPath, std::string( "." ).append( appName ) );
            }
        }

        if ( const char * dataEnvPath = getenv( "APPDATA" ); dataEnvPath != nullptr ) {
            return System::concatPath( dataEnvPath, appName );
        }

#if SDL_VERSION_ATLEAST( 2, 0, 1 )
        if ( const std::unique_ptr<char, void ( * )( void * )> path( SDL_GetPrefPath( "", System::encLocalToUTF8( std::string{ appName } ).c_str() ), SDL_free ); path ) {
            return System::encUTF8ToLocal( path.get() );
        }
#endif

        return {};
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
        const auto [resultIter, inserted] = resultsCache[dir].try_emplace( std::string{ str }, str );
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

bool System::isTouchInputAvailable()
{
    return SDL_GetNumTouchDevices() > 0;
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
    return fsPathToString( std::filesystem::path{ left }.append( right ) );
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
            return concatPath( configEnv, appName );
        }

        if ( const char * homeEnv = getenv( "HOME" ); homeEnv != nullptr ) {
            return concatPath( concatPath( homeEnv, ".config" ), appName );
        }

        return {};
#else
        return GetHomeDirectory( appName );
#endif
    }();

    if ( const auto [dummy, inserted] = resultsCache.try_emplace( std::string{ appName }, result ); !inserted ) {
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
            return concatPath( dataEnv, appName );
        }

        if ( const char * homeEnv = getenv( "HOME" ); homeEnv != nullptr ) {
            return concatPath( concatPath( homeEnv, ".local/share" ), appName );
        }

        return {};
#elif defined( MACOS_APP_BUNDLE )
        if ( const char * homeEnv = getenv( "HOME" ); homeEnv != nullptr ) {
            return concatPath( concatPath( homeEnv, "Library/Application Support" ), appName );
        }

        return {};
#else
        return GetHomeDirectory( appName );
#endif
    }();

    if ( const auto [dummy, inserted] = resultsCache.try_emplace( std::string{ appName }, result ); !inserted ) {
        assert( 0 );
    }

    return result;
}

std::string System::GetParentDirectory( std::string_view path )
{
    return fsPathToString( std::filesystem::path{ path }.parent_path() );
}

std::string System::GetFileName( std::string_view path )
{
    return fsPathToString( std::filesystem::path{ path }.filename() );
}

std::string System::GetStem( const std::string_view path )
{
    return fsPathToString( std::filesystem::path{ path }.stem() );
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
#if !defined( _WIN32 ) && !defined( ANDROID ) && !defined( TARGET_PS_VITA )
    // The following code is based on https://github.com/OneSadCookie/fcaseopen and assumes the use of POSIX IEEE Std 1003.1-2001 pathnames
    correctedPath.clear();

    if ( path.empty() ) {
        return false;
    }

    constexpr char dirSep{ '/' };

    std::unique_ptr<DIR, int ( * )( DIR * )> dir( path.front() == dirSep ? opendir( "/" ) : opendir( "." ), closedir );

    for ( const std::filesystem::path & pathItem : std::filesystem::path{ path } ) {
        if ( !dir ) {
            return false;
        }

        if ( !correctedPath.empty() && correctedPath.back() != dirSep ) {
            correctedPath += dirSep;
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
        // The idea is to try to open the current path item as a directory and
        // avoid directory traversal altogether. Otherwise fall back to linear
        // case-insensitive search.
        {
            std::string tmpPath = correctedPath + pathItem.string();

            if ( std::unique_ptr<DIR, int ( * )( DIR * )> tmpDir( opendir( tmpPath.c_str() ), closedir ); tmpDir ) {
                correctedPath = std::move( tmpPath );
                dir = std::move( tmpDir );

                continue;
            }
        }

        const struct dirent * entry = readdir( dir.get() );
        while ( entry != nullptr ) {
            if ( strcasecmp( pathItem.c_str(), entry->d_name ) == 0 ) {
                correctedPath += entry->d_name;

                dir.reset( opendir( correctedPath.c_str() ) );

                break;
            }

            entry = readdir( dir.get() );
        }

        if ( entry == nullptr ) {
            return false;
        }
    }
#else
    correctedPath = path;
#endif

    return !correctedPath.empty();
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

        if ( !globMatch( fsPathToString( entryPath.filename() ), pattern ) ) {
            continue;
        }

        fileNames.emplace_back( fsPathToString( entryPath ) );

        isNoMatches = false;
    }

    if ( isNoMatches ) {
        fileNames.emplace_back( glob );
    }
}

std::string System::encLocalToUTF8( const std::string_view str )
{
#if defined( _WIN32 )
    return convertBetweenACPAndUTF8( str, EncodingConversionDirection::ACPToUTF8 );
#else
    return std::string{ str };
#endif
}

std::string System::encUTF8ToLocal( const std::string_view str )
{
#if defined( _WIN32 )
    return convertBetweenACPAndUTF8( str, EncodingConversionDirection::UTF8ToACP );
#else
    return std::string{ str };
#endif
}

std::string System::fsPathToString( const std::filesystem::path & path )
{
#if defined( _WIN32 )
    // On Windows, std::filesystem::path::string() can throw an exception if path contains UTF-16 characters that
    // are non-representable in CP_ACP. However, converting a well-formed UTF-16 string to UTF-8 is always safe,
    // so we perform this conversion first, and then convert the resulting UTF-8 to CP_ACP using our conversion
    // function, which can never throw an exception.
    return encUTF8ToLocal( path.u8string() );
#else
    return path.string();
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
