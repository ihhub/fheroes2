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

#include "image.h"

namespace fheroes2
{
    class MovableSprite : public Sprite
    {
    public:
        MovableSprite( uint32_t width_ = 0, uint32_t height_ = 0, int32_t x_ = 0, int32_t y_ = 0 );
        virtual ~MovableSprite();

        void show();
        void hide();
        void redraw(); // in case if Display has changed

        bool isHidden() const;

        virtual void setPosition( int32_t x_, int32_t y_ );

    private:
        ImageRestorer _restorer;
        bool _isHidden;
    };
}
