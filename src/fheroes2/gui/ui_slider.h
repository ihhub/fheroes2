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

#include <cstdint>

#include "math_base.h"
#include "ui_button.h"
#include "ui_scrollbar.h"
#include "ui_tool.h"

class LocalEvent;

namespace fheroes2
{
    class HorizontalSlider final
    {
    public:
        ~HorizontalSlider() = default;
        HorizontalSlider( const HorizontalSlider & ) = delete;
        HorizontalSlider & operator=( const HorizontalSlider & ) = delete;

        HorizontalSlider( const int32_t width, const Point position, const int minIndex, const int maxIndex, const int currentIndex );

        int getCurrentValue() const
        {
            return _scrollbar.currentIndex();
        }

        void setRange( const int minIndex, const int maxIndex );

        bool processEvents( LocalEvent & le );

        void disable();

        void enable();

    private:
        Scrollbar _scrollbar;
        Button _buttonLeft;
        Button _buttonRight;

        Image _scrollbarBackup;

        TimedEventValidator _timedButtonLeft;
        TimedEventValidator _timedButtonRight;
    };
}
