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
#if defined( _WIN32 )
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined( TARGET_PS_VITA )
#include <psp2/io/dirent.h>
#else
#include <dirent.h>
#endif

#include "dir.h"
#include "system.h"
#if defined( TARGET_PS_VITA )
#include "tools.h"
#endif

#include <cstring>
#if defined( TARGET_PS_VITA ) || defined( TARGET_NINTENDO_SWITCH )
#include <strings.h> // for strcasecmp
#endif

namespace
{
    using StrCmp = int ( * )( const char *, const char * );

    bool filterByName( const char * filename, const bool nameAsFilter, const std::string & name, const StrCmp strCmp )
    {
        if ( !nameAsFilter || !name.empty() ) {
            const size_t filenameLength = strlen( filename );
            if ( filenameLength < name.length() )
                return true;

            if ( !nameAsFilter && filenameLength != name.length() ) {
                return true;
            }

            const char * filenamePtr = filename + filenameLength - name.length();
            if ( strCmp( filenamePtr, name.c_str() ) != 0 )
                return true;
        }

        return false;
    }

    void getFilesFromDirectory( const std::string & path, const std::string & name, bool sensitive, bool nameAsFilter, ListFiles & files )
    {
#if defined( _WIN32 )
        (void)sensitive;

        const std::string pattern( nameAsFilter ? path + "\\*" + name : path + "\\" + name );
        WIN32_FIND_DATA data;
        HANDLE hFind = FindFirstFile( pattern.c_str(), &data );
        if ( hFind == INVALID_HANDLE_VALUE ) {
            return;
        }

        do {
            if ( data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
                // Ignore any internal directories.
                continue;
            }

            std::string fullname = System::ConcatePath( path, data.cFileName );

            // FindFirstFile() searches for both long and short variants of names, so we need additional filtering
            if ( filterByName( data.cFileName, nameAsFilter, name, _stricmp ) )
                continue;

            files.emplace_back( std::move( fullname ) );
        } while ( FindNextFile( hFind, &data ) != 0 );

        FindClose( hFind );
#elif defined( TARGET_PS_VITA )
        // open the directory
        const int uid = sceIoDopen( path.c_str() );
        if ( uid <= 0 )
            return;

        const StrCmp strCmp = sensitive ? strcmp : strcasecmp;

        // iterate over the directory for files, print name and size of array (always 256)
        // this means you use strlen() to get length of file name
        SceIoDirent dir;
        while ( sceIoDread( uid, &dir ) > 0 ) {
            std::string fullname = System::ConcatePath( path, dir.d_name );

            // if not regular file
            if ( !SCE_S_ISREG( dir.d_stat.st_mode ) )
                continue;

            if ( filterByName( dir.d_name, nameAsFilter, name, strCmp ) )
                continue;

            files.emplace_back( std::move( fullname ) );
        }

        // clean up
        sceIoDclose( uid );
#else
        std::string correctedPath;
        if ( !System::GetCaseInsensitivePath( path, correctedPath ) )
            return;

        // read directory
        DIR * dp = opendir( correctedPath.c_str() );
        if ( !dp ) {
            return;
        }

        const StrCmp strCmp = sensitive ? strcmp : strcasecmp;

        const struct dirent * ep;
        while ( nullptr != ( ep = readdir( dp ) ) ) {
            std::string fullname = System::ConcatePath( correctedPath, ep->d_name );

            // if not regular file
            if ( !System::IsFile( fullname ) )
                continue;

            if ( filterByName( ep->d_name, nameAsFilter, name, strCmp ) )
                continue;

            files.emplace_back( std::move( fullname ) );
        }
        closedir( dp );
#endif
    }
}

void ListFiles::Append( const ListFiles & files )
{
    insert( end(), files.begin(), files.end() );
}

void ListFiles::ReadDir( const std::string & path, const std::string & filter, bool sensitive )
{
    getFilesFromDirectory( path, filter, sensitive, true, *this );
}

void ListFiles::FindFileInDir( const std::string & path, const std::string & fileName, bool sensitive )
{
    getFilesFromDirectory( path, fileName, sensitive, false, *this );
}

bool ListFiles::IsEmpty( const std::string & path, const std::string & filter, bool sensitive )
{
    ListFiles list;
    list.ReadDir( path, filter, sensitive );
    return list.empty();
}
