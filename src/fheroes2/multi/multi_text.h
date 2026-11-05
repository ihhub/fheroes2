/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2026                                                    *
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

#pragma once

#include <cstddef>
#include <string>

#include "game_language.h"

// Lobby messages carry UTF-8, so that players with other languages see the same names. The game writes and shows texts
// in the code page of its language.
namespace Multi
{
    // From the code page of the current language.
    std::string toUtf8( const std::string & text );

    // To the code page of the current language, with '?' for a character its fonts cannot draw.
    std::string fromUtf8( const std::string & text );

    std::string toUtf8( const std::string & text, const fheroes2::CodePage codePage );
    std::string fromUtf8( const std::string & text, const fheroes2::CodePage codePage );

    // Without the spaces around the text.
    std::string trimText( const std::string & text );

    // The first characters of a UTF-8 text.
    std::string cutUtf8( const std::string & text, const size_t maxCharacters );
}
