/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
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
#if defined( _MSC_VER ) || defined( __MINGW32__ )
#include <windows.h>
#elif defined( FHEROES2_VITA )
#include <psp2/io/dirent.h>
#else
#include <dirent.h>
#endif

#include "dir.h"
#include "system.h"
#include "tools.h"
#include <cstring>
#if defined( __SWITCH__ )
#include <strings.h> // for strcasecmp
#endif

namespace fheroes2
{
    void AddOSSpecificDirectories( ListDirs & dirs )
    {
#if defined( FHEROES2_VITA )
        dirs.emplace_back( "ux0:app/FHOMM0002" );
        dirs.emplace_back( "ux0:data/fheroes2" );
#else
        (void)dirs;
#endif
    }
}

void ListFiles::Append( const ListFiles & files )
{
    insert( end(), files.begin(), files.end() );
}

void ListFiles::ReadDir( const std::string & path, const std::string & filter, bool sensitive )
{
#if defined( _MSC_VER ) || defined( __MINGW32__ )
    (void)sensitive;

    std::string pattern( path + "\\*" + filter );
    WIN32_FIND_DATA data;
    HANDLE hFind;
    if ( ( hFind = FindFirstFile( pattern.c_str(), &data ) ) != INVALID_HANDLE_VALUE ) {
        do {
            push_back( path + "\\" + data.cFileName );
        } while ( FindNextFile( hFind, &data ) != 0 );
        FindClose( hFind );
    }
#elif defined( FHEROES2_VITA )
    // open the directory
    const int uid = sceIoDopen( path.c_str() );
    if ( uid <= 0 )
        return;

    // iterate over the directory for files, print name and size of array (always 256)
    // this means you use strlen() to get length of file name
    SceIoDirent dir;

    while ( sceIoDread( uid, &dir ) > 0 ) {
        std::string fullname = System::ConcatePath( path, dir.d_name );

        // if not regular file
        if ( !SCE_S_ISREG( dir.d_stat.st_mode ) )
            continue;

        if ( !filter.empty() ) {
            const std::string filename( dir.d_name );

            if ( sensitive ) {
                if ( std::string::npos == filename.find( filter ) )
                    continue;
            }
            else if ( std::string::npos == StringLower( filename ).find( StringLower( filter ) ) )
                continue;
        }

        emplace_back( std::move( fullname ) );
    }

    // clean up
    sceIoDclose( uid );
#else
    std::string correctedPath;
    if ( !System::GetCaseInsensitivePath( path, correctedPath ) )
        return;

    // read directory
    DIR * dp = opendir( correctedPath.c_str() );

    if ( dp ) {
        struct dirent * ep;
        while ( NULL != ( ep = readdir( dp ) ) ) {
            const std::string fullname = System::ConcatePath( correctedPath, ep->d_name );

            // if not regular file
            if ( !System::IsFile( fullname ) )
                continue;

            if ( filter.size() ) {
                const size_t filenameLength = strlen( ep->d_name );
                if ( filenameLength < filter.length() )
                    continue;

                const char * filenamePtr = ep->d_name + filenameLength - filter.length();

                if ( sensitive ) {
                    if ( strcmp( filenamePtr, filter.c_str() ) != 0 )
                        continue;
                }
                else {
                    if ( strcasecmp( filenamePtr, filter.c_str() ) != 0 )
                        continue;
                }
            }

            push_back( fullname );
        }
        closedir( dp );
    }
#endif
}

bool ListFiles::IsEmpty( const std::string & path, const std::string & filter, bool sensitive )
{
    ListFiles list;
    list.ReadDir( path, filter, sensitive );
    return list.empty();
}

void ListDirs::Append( const std::list<std::string> & dirs )
{
    insert( end(), dirs.begin(), dirs.end() );
}
