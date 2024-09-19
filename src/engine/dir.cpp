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

#include "dir.h"

#include <filesystem>
#include <system_error>
#include <utility>

#if defined( _WIN32 )
#include <cstring>
#else
#include <strings.h>
#endif

#include "system.h"

namespace
{
    template <typename F>
    bool nameFilter( const std::string & filename, const bool needExactMatch, const std::string & filter, const F & strCmp )
    {
        if ( !needExactMatch && filter.empty() ) {
            return true;
        }

        if ( filename.length() < filter.length() ) {
            return false;
        }

        if ( needExactMatch && filename.length() != filter.length() ) {
            return false;
        }

        const char * filenamePtr = filename.c_str() + filename.length() - filter.length();

        return ( strCmp( filenamePtr, filter.c_str() ) == 0 );
    }

    void getFilesFromDirectory( const std::string_view path, const std::string & filter, const bool needExactMatch, ListFiles & files )
    {
        std::string correctedPath;
        if ( !System::GetCaseInsensitivePath( path, correctedPath ) ) {
            return;
        }

#if defined( _WIN32 )
        auto * const strCmp = _stricmp;
#else
        auto * const strCmp = strcasecmp;
#endif

        std::error_code ec;

        // Using the non-throwing overload
        for ( const std::filesystem::directory_entry & entry : std::filesystem::directory_iterator( correctedPath, ec ) ) {
            // Using the non-throwing overload
            if ( !entry.is_regular_file( ec ) ) {
                continue;
            }

            const std::string fileName = entry.path().filename().string();
            if ( !nameFilter( fileName, needExactMatch, filter, strCmp ) ) {
                continue;
            }

            files.emplace_back( System::concatPath( correctedPath, fileName ) );
        }
    }
}

void ListFiles::Append( ListFiles && files )
{
    for ( std::string & file : files ) {
        emplace_back( std::move( file ) );
    }
}

void ListFiles::ReadDir( const std::string_view path, const std::string & filter )
{
    getFilesFromDirectory( path, filter, false, *this );
}

void ListFiles::FindFileInDir( const std::string_view path, const std::string & fileName )
{
    getFilesFromDirectory( path, fileName, true, *this );
}

bool ListFiles::IsEmpty( const std::string_view path, const std::string & filter )
{
    ListFiles list;
    list.ReadDir( path, filter );
    return list.empty();
}
