/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2022                                                    *
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

#include "math_base.h"

namespace fheroes2
{
    struct ObjectRenderingInfo
    {
        Point tileOffset;

        Point imageOffset;

        Rect area;

        int icnId{ -1 };

        uint32_t icnIndex{ 0 };

        bool isFlipped{ false };

        uint8_t alphaValue{ 255 };

        ObjectRenderingInfo() = default;

        ObjectRenderingInfo( const Point & tileOffset_, const Point & imageOffset_, const Rect & area_, const int icnId_, const uint32_t icnIndex_, const bool isFlipped_,
                             const uint8_t alphaValue_ )
            : tileOffset( tileOffset_ )
            , imageOffset( imageOffset_ )
            , area( area_ )
            , icnId( icnId_ )
            , icnIndex( icnIndex_ )
            , isFlipped( isFlipped_ )
            , alphaValue( alphaValue_ )
        {
            // Do nothing.
        }

        ObjectRenderingInfo( const ObjectRenderingInfo & ) = default;

        ObjectRenderingInfo( ObjectRenderingInfo && ) = default;

        ObjectRenderingInfo & operator=( const ObjectRenderingInfo & ) = default;

        ObjectRenderingInfo & operator=( ObjectRenderingInfo && ) = default;
    };
}
