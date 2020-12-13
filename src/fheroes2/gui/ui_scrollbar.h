/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2020                                                    *
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

#include "ui_tool.h"

namespace fheroes2
{
    class Scrollbar : public fheroes2::MovableSprite
    {
    public:
        Scrollbar();
        Scrollbar( const Image & image, const Rect & area );

        virtual ~Scrollbar();

        void setImage( const Image & image );
        void setArea( const Rect & area );
        void setRange( const int minIndex, const int maxIndex );

        void forward();
        void backward();

        void moveToIndex( const int indexId );
        void moveToPos( const Point & position );

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
        fheroes2::Rect _area;
        int _minIndex;
        int _maxIndex;
        int _currentIndex;

        bool _isVertical() const;
    };
}
