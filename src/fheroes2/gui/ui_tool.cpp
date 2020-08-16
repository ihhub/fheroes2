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

#include "ui_tool.h"
#include "screen.h"

namespace fheroes2
{
    MovableSprite::MovableSprite( uint32_t width_, uint32_t height_, int32_t x_, int32_t y_ )
        : Sprite( width_, height_, x_, y_ )
        , _restorer( Display::instance() )
        , _isHidden( false )
    {}

    MovableSprite::MovableSprite( const Sprite & sprite )
        : Sprite( sprite )
        , _restorer( Display::instance() )
        , _isHidden( false )
    {}

    MovableSprite::~MovableSprite() {}

    void MovableSprite::setPosition( int32_t x_, int32_t y_ )
    {
        if ( x_ == x() && y_ == y() )
            return;

        _restorer.restore();
        Sprite::setPosition( x_, y_ );
        _restorer.update( x(), y(), width(), height() );
    }

    void MovableSprite::show()
    {
        if ( _isHidden ) {
            _restorer.update( x(), y(), width(), height() );
            fheroes2::Blit( *this, Display::instance(), x(), y() );
            _isHidden = false;
        }
    }

    void MovableSprite::hide()
    {
        if ( !_isHidden ) {
            _restorer.restore();
            _isHidden = true;
        }
    }

    void MovableSprite::redraw()
    {
        hide();
        show();
    }

    bool MovableSprite::isHidden() const
    {
        return _isHidden;
    }

    void FadeDisplay() {}

    void RiseDisplay() {}
}
