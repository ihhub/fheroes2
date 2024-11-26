/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2023 - 2024                                             *
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
#include <cstdint>
#include <string>

namespace fheroes2
{
    // fheroes2 does not support UTF-8 so on mobile devices with virtual keyboard it might be a big problem.
    // As a solution we should utilize an in-game virtual keyboard which supports all code pages available by the engine.
    // The default language in the keyboard is English.
    //
    // If length limit is set to 0 then no limit will be applied.
    void openVirtualKeyboard( std::string & output, size_t lengthLimit );

    void openVirtualNumpad( int32_t & output, const int32_t minValue = INT32_MIN, const int32_t maxValue = INT32_MAX );
}
