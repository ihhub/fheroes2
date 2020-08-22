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
    MovableSprite::MovableSprite( int32_t width_, int32_t height_, int32_t x_, int32_t y_ )
        : Sprite( width_, height_, x_, y_ )
        , _restorer( Display::instance(), x_, y_, width_, height_ )
        , _isHidden( height_ == 0 && width_ == 0 )
    {}

    MovableSprite::MovableSprite( const Sprite & sprite )
        : Sprite( sprite )
        , _restorer( Display::instance(), sprite.x(), sprite.y(), sprite.width(), sprite.height() )
        , _isHidden( false )
    {}

    MovableSprite::~MovableSprite() {}

    MovableSprite & MovableSprite::operator=( const Sprite & sprite )
    {
        Sprite::operator=( sprite );
        _restorer.update( x(), y(), width(), height() );
        return *this;
    }

    void MovableSprite::setPosition( int32_t x_, int32_t y_ )
    {
        hide();
        Sprite::setPosition( x_, y_ );
        show();
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

    Image CreateRippleEffect( const Image & in, int32_t frameId, double scaleX, double waveFrequency )
    {
        if ( in.empty() )
            return Image();

        const int32_t widthIn = in.width();
        const int32_t height = in.height();

        // convert frames to -10...10 range with a period of 40
        const int32_t linearWave = std::abs( 20 - ( frameId + 10 ) % 40 ) - 10;
        const int32_t progress = 7 - frameId / 10;

        const double rippleXModifier = ( progress * scaleX + 0.3 ) * linearWave;
        const int32_t offsetX = static_cast<int32_t>( std::abs( rippleXModifier ) );
        const double pi = std::acos( -1 );
        const int32_t limitY = static_cast<int32_t>( waveFrequency * pi );

        Image out( widthIn + offsetX * 2, height );
        out.reset();

        const int32_t widthOut = out.width();

        uint8_t * outImageY = out.image();
        uint8_t * outTransformY = out.transform();

        const uint8_t * inImageY = in.image();
        const uint8_t * inTransformY = in.transform();

        for ( int32_t y = 0; y < height; ++y, inImageY += widthIn, inTransformY += widthIn, outImageY += widthOut, outTransformY += widthOut ) {
            // Take top half the sin wave starting at 0 with period set by waveFrequency, result is -1...1
            const double sinYEffect = sin( ( y % limitY ) / waveFrequency ) * 2.0 - 1;
            const int32_t offset = static_cast<int32_t>( rippleXModifier * sinYEffect ) + offsetX;

            memcpy( outImageY + offset, inImageY, widthIn );
            memcpy( outTransformY + offset, inTransformY, widthIn );
        }

        return out;
    }

    void FadeDisplay() {}

    void RiseDisplay() {}

    void InvertedFade() {}
}
