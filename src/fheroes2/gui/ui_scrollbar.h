/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2023                                             *
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

#include "image.h"
#include "math_base.h"
#include "ui_tool.h"

namespace fheroes2
{
    class Scrollbar : public fheroes2::MovableSprite
    {
    public:
        Scrollbar();

        ~Scrollbar() override = default;

        // The original resources do not support proper scrollbar slider scaling. Use generateScrollbarSlider() function to generate needed image.
        void setImage( const Image & image );

        void setArea( const Rect & area );
        void setRange( const int minIndex, const int maxIndex );

        void forward();
        void backward();

        // Returns true if the position and/or index is updated.
        bool moveToIndex( const int indexId );

        void moveToPos( const Point & position );

        // Update position of the scrollbar based on the index. Useful for mouse movement and release.
        bool updatePosition()
        {
            return moveToIndex( _currentIndex );
        }

        int currentIndex() const
        {
            return _currentIndex;
        }

        int minIndex() const
        {
            return _minIndex;
        }

        int maxIndex() const
        {
            return _maxIndex;
        }

        const Rect & getArea() const
        {
            return _area;
        }

    private:
        Rect _area;
        int _minIndex;
        int _maxIndex;
        int _currentIndex;

        bool _isVertical() const
        {
            return _area.width < _area.height;
        }
    };

    // The original scrollbar slider has fixed size. This is a not user-friendly solution as on big screens it might look extremely tiny.
    // In the most modern applications the slider size depends on the number of elements. The lesser the number the bigger the slider.
    Image generateScrollbarSlider( const Image & originalSlider, const bool horizontalSlider, const int32_t sliderAreaLength, const int32_t elementCountPerView,
                                   const int32_t totalElementCount, const Rect & startSliderArea, const Rect & middleSliderArea );
}
