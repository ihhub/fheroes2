/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#ifndef TINYCONFIG_H
#define TINYCONFIG_H

#include <map>
#include <string>

#include "math_base.h"

class TinyConfig : protected std::multimap<std::string, std::string>
{
public:
    TinyConfig( char sep = '=', char com = ';' );

    bool Load( const std::string & cfile );

    bool Exists( const std::string & key ) const;

    int IntParams( const std::string & key ) const;
    std::string StrParams( const std::string & key ) const;
    // Tries to find and return a Point-type struct stored as a string "[ x, y ]" for a given key.
    // In case of any error, the fallback value is returned.
    fheroes2::Point PointParams( const std::string & key, const fheroes2::Point & fallbackValue ) const;

protected:
    char separator;
    char comment;
};

#endif
