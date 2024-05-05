/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
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
#include <string_view>
#include <vector>

namespace System
{
    bool isHandheldDevice();

    bool isVirtualKeyboardSupported();

    // Returns true if target platform supports shell-level globbing (Unix-like platforms with POSIX-compatible shells).
    // Otherwise returns false, which means that app need to resolve wildcard patterns itself (for example, on Windows).
    bool isShellLevelGlobbingSupported();

    bool MakeDirectory( const std::string & path );
    std::string concatPath( const std::string_view left, const std::string_view right );

    void appendOSSpecificDirectories( std::vector<std::string> & directories );
    std::string GetConfigDirectory( const std::string & prog );
    std::string GetDataDirectory( const std::string & prog );

    std::string GetDirname( std::string_view path );
    std::string GetBasename( std::string_view path );
    std::string truncateFileExtensionAndPath( std::string_view path );

    bool IsFile( const std::string & path, bool writable = false );
    bool IsDirectory( const std::string & path, bool writable = false );
    bool Unlink( const std::string & path );

    bool GetCaseInsensitivePath( const std::string & path, std::string & correctedPath );

    // Resolves the wildcard pattern 'glob' and appends matching paths to 'fileNames'. Supported wildcards are '?' and '*'.
    // These wildcards are resolved only if they are in the last element of the path. For example, they will be resolved
    // in the case of the 'foo/b*r?' pattern, but they will be ignored (used as is) in the case of '*/bar' pattern. If there
    // are no files matching the pattern, it will be appended to the 'fileNames' as is.
    void globFiles( const std::string_view glob, std::vector<std::string> & fileNames );

    std::string FileNameToUTF8( const std::string & name );

    tm GetTM( const time_t time );
}

#endif
