/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
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

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <map>
#include <memory>

#if defined( _WIN32 )
#include <clocale>
#endif

#include "logging.h"
#include "system.h"
#include "tools.h"

#include <SDL.h>

#if defined( _WIN32 )
#define WIN32_LEAN_AND_MEAN
// clang-format off
// shellapi.h must be included after windows.h
#include <windows.h>
#include <shellapi.h>
// clang-format on
#else
#include <dirent.h>
#endif

#if defined( _WIN32 )
#include <io.h>
#else

#if defined( TARGET_PS_VITA )
#include <psp2/io/stat.h>
#else
#include <sys/stat.h>
#endif

#include <unistd.h>
#endif

#if defined( _WIN32 )
#define SEPARATOR '\\'
#else
#define SEPARATOR '/'
#endif

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

        const char * homeEnvPath = getenv( "HOME" );

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

int System::MakeDirectory( const std::string & path )
{
#if defined( _WIN32 )
    return CreateDirectoryA( path.c_str(), nullptr );
#elif defined( TARGET_PS_VITA )
    return sceIoMkdir( path.c_str(), 0777 );
#else
    return mkdir( path.c_str(), S_IRWXU );
#endif
}

std::string System::ConcatePath( const std::string & str1, const std::string & str2 )
{
    // Avoid memory allocation while concatenating string. Allocate needed size at once.
    std::string temp;
    temp.reserve( str1.size() + 1 + str2.size() );

    temp += str1;
    temp += SEPARATOR;
    temp += str2;

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
#if defined( __LINUX__ )
    const char * configEnv = getenv( "XDG_CONFIG_HOME" );
    if ( configEnv ) {
        return System::ConcatePath( configEnv, prog );
    }

    const char * homeEnv = getenv( "HOME" );
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
    const char * dataEnv = getenv( "XDG_DATA_HOME" );
    if ( dataEnv ) {
        return System::ConcatePath( dataEnv, prog );
    }

    const char * homeEnv = getenv( "HOME" );
    if ( homeEnv ) {
        return System::ConcatePath( System::ConcatePath( homeEnv, ".local/share" ), prog );
    }

    return {};
#elif defined( MACOS_APP_BUNDLE )
    const char * homeEnv = getenv( "HOME" );
    if ( homeEnv ) {
        return System::ConcatePath( System::ConcatePath( homeEnv, "Library/Application Support" ), prog );
    }

    return {};
#else
    return GetHomeDirectory( prog );
#endif
}

std::string System::GetDirname( const std::string & str )
{
    if ( !str.empty() ) {
        size_t pos = str.rfind( SEPARATOR );

        if ( std::string::npos == pos )
            return std::string( "." );
        else if ( pos == 0 )
            return std::string( "./" );
        else if ( pos == str.size() - 1 )
            return GetDirname( str.substr( 0, str.size() - 1 ) );
        else
            return str.substr( 0, pos );
    }

    return str;
}

std::string System::GetBasename( const std::string & str )
{
    if ( !str.empty() ) {
        size_t pos = str.rfind( SEPARATOR );

        if ( std::string::npos == pos || pos == 0 )
            return str;
        else if ( pos == str.size() - 1 )
            return GetBasename( str.substr( 0, str.size() - 1 ) );
        else
            return str.substr( pos + 1 );
    }

    return str;
}

int System::GetCommandOptions( int argc, char * const argv[], const char * optstring )
{
#if defined( _WIN32 )
    (void)argc;
    (void)argv;
    (void)optstring;
    return -1;
#else
    return getopt( argc, argv, optstring );
#endif
}

char * System::GetOptionsArgument()
{
#if defined( _WIN32 )
    return nullptr;
#else
    return optarg;
#endif
}

bool System::IsFile( const std::string & name, bool writable )
{
    if ( name.empty() ) {
        // An empty path cannot be a file.
        return false;
    }

#if defined( _WIN32 )
    const DWORD fileAttributes = GetFileAttributes( name.c_str() );
    if ( fileAttributes == INVALID_FILE_ATTRIBUTES ) {
        // This path doesn't exist.
        return false;
    }

    if ( ( fileAttributes & FILE_ATTRIBUTE_DIRECTORY ) != 0 ) {
        // This is a directory.
        return false;
    }

    return writable ? ( 0 == _access( name.c_str(), 06 ) ) : ( 0 == _access( name.c_str(), 04 ) );
#elif defined( TARGET_PS_VITA )
    // TODO: check if it is really a file.
    return writable ? 0 == access( name.c_str(), W_OK ) : 0 == access( name.c_str(), R_OK );
#else
    std::string correctedPath;
    if ( !GetCaseInsensitivePath( name, correctedPath ) )
        return false;

    struct stat fs;

    if ( stat( correctedPath.c_str(), &fs ) || !S_ISREG( fs.st_mode ) )
        return false;

    return writable ? 0 == access( correctedPath.c_str(), W_OK ) : S_IRUSR & fs.st_mode;
#endif
}

bool System::IsDirectory( const std::string & name, bool writable )
{
    if ( name.empty() ) {
        // An empty path cannot be a directory.
        return false;
    }

#if defined( _WIN32 )
    const DWORD fileAttributes = GetFileAttributes( name.c_str() );
    if ( fileAttributes == INVALID_FILE_ATTRIBUTES ) {
        // This path doesn't exist.
        return false;
    }

    if ( ( fileAttributes & FILE_ATTRIBUTE_DIRECTORY ) == 0 ) {
        // Not a directory.
        return false;
    }

    return writable ? ( 0 == _access( name.c_str(), 06 ) ) : ( 0 == _access( name.c_str(), 00 ) );
#elif defined( TARGET_PS_VITA )
    // TODO: check if it is really a directory.
    return writable ? 0 == access( name.c_str(), W_OK ) : 0 == access( name.c_str(), R_OK );
#else
    std::string correctedPath;
    if ( !GetCaseInsensitivePath( name, correctedPath ) )
        return false;

    struct stat fs;

    if ( stat( correctedPath.c_str(), &fs ) || !S_ISDIR( fs.st_mode ) )
        return false;

    return writable ? 0 == access( correctedPath.c_str(), W_OK ) : S_IRUSR & fs.st_mode;
#endif
}

int System::Unlink( const std::string & file )
{
#if defined( _WIN32 )
    return _unlink( file.c_str() );
#else
    return unlink( file.c_str() );
#endif
}

#if !( defined( _WIN32 ) )
// splitUnixPath - function for splitting strings by delimiter
std::vector<std::string> splitUnixPath( const std::string & path, const std::string & delimiter )
{
    std::vector<std::string> result;

    if ( path.empty() ) {
        return result;
    }

    size_t pos = path.find( delimiter, 0 );
    while ( pos != std::string::npos ) { // while found delimiter
        const size_t nextPos = path.find( delimiter, pos + 1 );
        if ( nextPos != std::string::npos ) { // if found next delimiter
            if ( pos + 1 < nextPos ) { // have what to append
                result.push_back( path.substr( pos + 1, nextPos - pos - 1 ) );
            }
        }
        else { // if no more delimiter present
            if ( pos + 1 < path.length() ) { // if not a postfix delimiter
                result.push_back( path.substr( pos + 1 ) );
            }
            break;
        }

        pos = path.find( delimiter, pos + 1 );
    }

    if ( result.empty() ) { // if delimiter not present
        result.push_back( path );
    }

    return result;
}

// based on: https://github.com/OneSadCookie/fcaseopen
bool System::GetCaseInsensitivePath( const std::string & path, std::string & correctedPath )
{
    correctedPath.clear();

    if ( path.empty() )
        return false;

    DIR * d;
    bool last = false;
    const char * curDir = ".";
    const char * delimiter = "/";

    if ( path[0] == delimiter[0] ) {
        d = opendir( delimiter );
    }
    else {
        correctedPath = curDir[0];
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

        correctedPath.append( delimiter );

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

    if ( d )
        closedir( d );

    return !last;
}
#else
bool System::GetCaseInsensitivePath( const std::string & path, std::string & correctedPath )
{
    correctedPath = path;
    return true;
}
#endif

std::string System::FileNameToUTF8( const std::string & str )
{
#if defined( _WIN32 )
    if ( str.empty() ) {
        return str;
    }

    static std::map<std::string, std::string> acpToUtf8;

    const auto iter = acpToUtf8.find( str );
    if ( iter != acpToUtf8.end() ) {
        return iter->second;
    }

    // In case of any issues, the original string will be returned, so let's put it to the cache right away
    acpToUtf8[str] = str;

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

    const int wLen = MultiByteToWideChar( CP_ACP, MB_ERR_INVALID_CHARS, str.c_str(), -1, nullptr, 0 );
    if ( wLen <= 0 ) {
        ERROR_LOG( getLastErrorStr() )

        return str;
    }

    const std::unique_ptr<wchar_t[]> wStr( new wchar_t[wLen] );

    if ( MultiByteToWideChar( CP_ACP, MB_ERR_INVALID_CHARS, str.c_str(), -1, wStr.get(), wLen ) != wLen ) {
        ERROR_LOG( getLastErrorStr() )

        return str;
    }

    const int uLen = WideCharToMultiByte( CP_UTF8, WC_ERR_INVALID_CHARS | WC_NO_BEST_FIT_CHARS, wStr.get(), -1, nullptr, 0, nullptr, nullptr );
    if ( uLen <= 0 ) {
        ERROR_LOG( getLastErrorStr() )

        return str;
    }

    const std::unique_ptr<char[]> uStr( new char[uLen] );

    if ( WideCharToMultiByte( CP_UTF8, WC_ERR_INVALID_CHARS | WC_NO_BEST_FIT_CHARS, wStr.get(), -1, uStr.get(), uLen, nullptr, nullptr ) != uLen ) {
        ERROR_LOG( getLastErrorStr() )

        return str;
    }

    const std::string result( uStr.get() );

    // Put the final result to the cache
    acpToUtf8[str] = result;

    return result;
#else
    return str;
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
