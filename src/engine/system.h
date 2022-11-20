/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2013 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#ifndef H2SYSTEM_H
#define H2SYSTEM_H

#include <ctime>
#include <string>
#include <vector>

namespace System
{
    bool isHandheldDevice();

    bool MakeDirectory( const std::string & path );
    std::string concatPath( const std::string & left, const std::string & right );

    void appendOSSpecificDirectories( std::vector<std::string> & directories );
    std::string GetConfigDirectory( const std::string & prog );
    std::string GetDataDirectory( const std::string & prog );

    std::string GetDirname( std::string_view path );
    std::string GetBasename( std::string_view path );

    int GetCommandOptions( int argc, char * const argv[], const char * optstring );
    char * GetOptionsArgument();

    bool IsFile( const std::string & path, bool writable = false );
    bool IsDirectory( const std::string & path, bool writable = false );
    bool Unlink( const std::string & path );

    bool GetCaseInsensitivePath( const std::string & path, std::string & correctedPath );

    std::string FileNameToUTF8( const std::string & name );

    tm GetTM( const time_t time );
}

#endif
