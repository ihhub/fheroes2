/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
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

#ifndef H2TRANSLATIONS_H
#define H2TRANSLATIONS_H

#include <cstddef>
#include <string>

namespace Translation
{
    bool bindDomain( const char * domain, const char * file );

    // Reset any translation to the default language - English.
    void reset();

    const char * gettext( const char * str );
    const char * gettext( const std::string & str );
    const char * ngettext( const char * str, const char * plural, size_t num );

    std::string StringLower( std::string str );
}

#define _( s ) Translation::gettext( s )
#define _n( a, b, c ) Translation::ngettext( a, b, c )
constexpr const char * gettext_noop( const char * s )
{
    return s;
}

#endif
