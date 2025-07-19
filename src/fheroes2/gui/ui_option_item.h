/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2022 - 2025                                             *
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

#include <cstdint>
#include <string>

#include "math_base.h"

enum class InterfaceType : uint8_t;

namespace fheroes2
{
    enum UiOptionTextWidth : int32_t
    {
        TWO_ELEMENTS_ROW = 113,
        THREE_ELEMENTS_ROW = 87
    };

    // Horizontal offset between option icons for 3 options in row for game settings dialogs.
    inline constexpr int32_t threeOptionsStepX{ 92 };

    // Horizontal offset between option icons for 2 options in row for game settings dialogs.
    inline constexpr int32_t twoOptionsStepX{ 118 };

    // Vertical offset between option icons for game settings dialogs.
    inline constexpr int32_t optionsStepY{ 110 };

    // Horizontal offset for option icon for first icon for 3 options in row for game settings dialogs.
    inline constexpr int32_t threeOptionsOffsetX{ 20 };

    // Horizontal offset for option icon for first icon for 2 options in row for game settings dialogs.
    inline constexpr int32_t twoOptionsOffsetX{ 53 };

    // Vertical offset for the first option icon in column for game settings dialogs.
    inline constexpr int32_t optionsOffsetY{ 31 };

    // Default option icon size for game settings dialogs.
    inline constexpr int32_t optionIconSize{ 65 };

    class Sprite;

    void drawOption( const Rect & optionRoi, const Sprite & icon, std::string titleText, std::string valueText, const int32_t textMaxWidth );

    void drawScrollSpeed( const fheroes2::Rect & optionRoi, const int speed );

    void drawInterfaceType( const fheroes2::Rect & optionRoi, const InterfaceType interfaceType, const int32_t textMaxWidth );

    void drawCursorType( const fheroes2::Rect & optionRoi, const bool isMonochromeCursor, const int32_t textMaxWidth );
}
