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
    int MakeDirectory( const std::string & );
    std::string ConcatePath( const std::string &, const std::string & );

    void appendOSSpecificDirectories( std::vector<std::string> & directories );
    std::string GetConfigDirectory( const std::string & prog );
    std::string GetDataDirectory( const std::string & prog );

    std::string GetDirname( const std::string & );
    std::string GetBasename( const std::string & );

    int GetCommandOptions( int argc, char * const argv[], const char * optstring );
    char * GetOptionsArgument();

    bool IsFile( const std::string & name, bool writable = false );
    bool IsDirectory( const std::string & name, bool writable = false );
    int Unlink( const std::string & );

    bool GetCaseInsensitivePath( const std::string & path, std::string & correctedPath );

    std::string FileNameToUTF8( const std::string & str );

    tm GetTM( const time_t time );
}

#endif
