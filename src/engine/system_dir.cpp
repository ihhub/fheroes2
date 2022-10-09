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

#include <string>

#include "system_dir.h"

#if defined( _WIN32 )
#include <io.h>
#elseif defined( TARGET_PS_VITA )
#include <psp2/io/stat.h>
#else
#include <sys/stat.h>
#endif

#if defined( _WIN32 )
#define SEPARATOR '\\'
#else
#define SEPARATOR '/'
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
