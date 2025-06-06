/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2024 - 2025                                             *
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

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "color.h"
#include "math_base.h"
#include "ui_tool.h"

struct Funds;

namespace fheroes2
{
    class Image;
}

namespace Editor
{
    class Checkbox
    {
    public:
        Checkbox( const int32_t x, const int32_t y, const PlayerColor boxColor, const bool checked, fheroes2::Image & output );

        Checkbox( Checkbox && other ) = delete;
        ~Checkbox() = default;
        Checkbox( Checkbox & ) = delete;
        Checkbox & operator=( const Checkbox & ) = delete;

        const fheroes2::Rect & getRect() const
        {
            return _area;
        }

        PlayerColor getColor() const
        {
            return _color;
        }

        bool toggle();

    private:
        const PlayerColor _color{ PlayerColor::NONE };
        fheroes2::Rect _area;
        fheroes2::MovableSprite _checkmark;
    };

    void createColorCheckboxes( std::vector<std::unique_ptr<Checkbox>> & list, const PlayerColorsSet availableColors, const PlayerColorsSet selectedColors,
                                const int32_t boxOffsetX, const int32_t boxOffsetY, fheroes2::Image & output );

    fheroes2::Rect drawCheckboxWithText( fheroes2::MovableSprite & checkSprite, std::string str, fheroes2::Image & output, const int32_t posX, const int32_t posY,
                                         const bool isEvil, const int32_t maxWidth );

    void renderResources( const Funds & resources, const fheroes2::Rect & roi, fheroes2::Image & output, std::array<fheroes2::Rect, 7> & resourceRoi );

    std::string getDateDescription( const int32_t day );
}
