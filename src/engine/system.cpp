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
#include <initializer_list>
#include <system_error>
#include <utility>

#if defined( _WIN32 )
#include <clocale>
#include <map>

#include <direct.h>
#include <io.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dirent.h>
#include <unistd.h>

#if defined( TARGET_PS_VITA )
#include <algorithm>

#include <psp2/io/stat.h>
#else
#include <sys/stat.h>
#endif
#endif

#if defined( _WIN32 ) || defined( ANDROID )
#include "logging.h"
#else
#include <strings.h>
#endif

#include <SDL_version.h>

#if defined( ANDROID )
#include <SDL_error.h>
#include <SDL_system.h>
#endif

#if SDL_VERSION_ATLEAST( 2, 0, 1 ) && ( !defined( __linux__ ) || defined( ANDROID ) )
#include <SDL_filesystem.h>
#include <SDL_stdinc.h>
#endif

#if defined( _WIN32 )
#define SEPARATOR '\\'
#else
#define SEPARATOR '/'
#endif

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

#if !defined( _WIN32 ) && !defined( ANDROID )
    std::vector<std::string> splitUnixPath( const std::string & path, const std::string_view delimiter )
    {
        std::vector<std::string> result;

        if ( path.empty() ) {
            return result;
        }

        size_t pos = 0;

        while ( pos < path.size() ) {
            const size_t nextPos = path.find( delimiter, pos );

            if ( nextPos == std::string::npos ) {
                result.push_back( path.substr( pos ) );

                break;
            }
            if ( pos < nextPos ) {
                result.push_back( path.substr( pos, nextPos - pos ) );
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

bool System::MakeDirectory( const std::string & path )
{
#if defined( _WIN32 )
    return _mkdir( path.c_str() ) == 0;
#elif defined( TARGET_PS_VITA )
    return sceIoMkdir( path.c_str(), 0777 ) == 0;
#else
    return mkdir( path.c_str(), S_IRWXU ) == 0;
#endif
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

std::string System::truncateFileExtensionAndPath( std::string_view path )
{
    std::string res = GetBasename( path );

    const size_t pos = res.rfind( '.' );

    if ( pos != std::string::npos ) {
        res.resize( pos );
    }

    return res;
}

bool System::IsFile( const std::string & path, bool writable )
{
    if ( path.empty() ) {
        // An empty path cannot be a file.
        return false;
    }

#if defined( _WIN32 )
    const DWORD fileAttributes = GetFileAttributes( path.c_str() );
    if ( fileAttributes == INVALID_FILE_ATTRIBUTES ) {
        // This path doesn't exist.
        return false;
    }

    if ( ( fileAttributes & FILE_ATTRIBUTE_DIRECTORY ) != 0 ) {
        // This is a directory.
        return false;
    }

    return writable ? ( 0 == _access( path.c_str(), 06 ) ) : ( 0 == _access( path.c_str(), 04 ) );
#elif defined( TARGET_PS_VITA ) || defined( ANDROID )
    // TODO: check if it is really a file.
    return writable ? 0 == access( path.c_str(), W_OK ) : 0 == access( path.c_str(), R_OK );
#else
    std::string correctedPath;
    if ( !GetCaseInsensitivePath( path, correctedPath ) )
        return false;

    struct stat fs;

    if ( stat( correctedPath.c_str(), &fs ) || !S_ISREG( fs.st_mode ) )
        return false;

    return writable ? 0 == access( correctedPath.c_str(), W_OK ) : S_IRUSR & fs.st_mode;
#endif
}

bool System::IsDirectory( const std::string & path, bool writable )
{
    if ( path.empty() ) {
        // An empty path cannot be a directory.
        return false;
    }

#if defined( _WIN32 )
    const DWORD fileAttributes = GetFileAttributes( path.c_str() );
    if ( fileAttributes == INVALID_FILE_ATTRIBUTES ) {
        // This path doesn't exist.
        return false;
    }

    if ( ( fileAttributes & FILE_ATTRIBUTE_DIRECTORY ) == 0 ) {
        // Not a directory.
        return false;
    }

    return writable ? ( 0 == _access( path.c_str(), 06 ) ) : ( 0 == _access( path.c_str(), 00 ) );
#elif defined( TARGET_PS_VITA ) || defined( ANDROID )
    // TODO: check if it is really a directory.
    return writable ? 0 == access( path.c_str(), W_OK ) : 0 == access( path.c_str(), R_OK );
#else
    std::string correctedPath;
    if ( !GetCaseInsensitivePath( path, correctedPath ) )
        return false;

    struct stat fs;

    if ( stat( correctedPath.c_str(), &fs ) || !S_ISDIR( fs.st_mode ) )
        return false;

    return writable ? 0 == access( correctedPath.c_str(), W_OK ) : S_IRUSR & fs.st_mode;
#endif
}

bool System::Unlink( const std::string & path )
{
#if defined( _WIN32 )
    return _unlink( path.c_str() ) == 0;
#else
    return unlink( path.c_str() ) == 0;
#endif
}

#if !defined( _WIN32 ) && !defined( ANDROID )
// based on: https://github.com/OneSadCookie/fcaseopen
bool System::GetCaseInsensitivePath( const std::string & path, std::string & correctedPath )
{
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
            if ( strcasecmp( ( *subPathIter ).c_str(), e->d_name ) == 0 ) {
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
}
#else
bool System::GetCaseInsensitivePath( const std::string & path, std::string & correctedPath )
{
    correctedPath = path;
    return true;
}
#endif

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

    const auto getLastErrorStr = []() {
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
