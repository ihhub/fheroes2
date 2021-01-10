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

#include "ui_scrollbar.h"

#include <assert.h>

namespace fheroes2
{
    Scrollbar::Scrollbar()
        : _minIndex( 0 )
        , _maxIndex( 0 )
        , _currentIndex( 0 )
    {}

    Scrollbar::Scrollbar( const Image & image, const Rect & area )
        : fheroes2::MovableSprite( image )
        , _area( area )
        , _minIndex( 0 )
        , _maxIndex( 0 )
        , _currentIndex( 0 )
    {}

    Scrollbar::~Scrollbar() {}

    void Scrollbar::setImage( const Image & image )
    {
        fheroes2::Copy( image, *this );
    }

    void Scrollbar::setArea( const Rect & area )
    {
        _area = area;
    }

    void Scrollbar::setRange( const int minIndex, const int maxIndex )
    {
        assert( maxIndex >= minIndex );

        _minIndex = minIndex;
        _maxIndex = maxIndex;
        _currentIndex = _minIndex;

        if ( _maxIndex == _minIndex ) {
            setPosition( _area.x + ( _area.width - width() ) / 2, _area.y + ( _area.height - height() ) / 2 );
        }
        else {
            if ( _isVertical() ) {
                setPosition( _area.x + ( _area.width - width() ) / 2, _area.y );
            }
            else {
                setPosition( _area.x, _area.y + ( _area.height - height() ) / 2 );
            }
        }
    }

    void Scrollbar::forward()
    {
        if ( _currentIndex < _maxIndex ) {
            moveToIndex( _currentIndex + 1 );
        }
    }

    void Scrollbar::backward()
    {
        if ( _currentIndex > _minIndex ) {
            moveToIndex( _currentIndex - 1 );
        }
    }

    void Scrollbar::moveToIndex( const int indexId )
    {
        if ( _maxIndex == _minIndex )
            return;

        if ( indexId < _minIndex )
            _currentIndex = _minIndex;
        else if ( indexId > _maxIndex )
            _currentIndex = _maxIndex;
        else
            _currentIndex = indexId;

        const int roiWidth = _area.width - width();
        const int roiHeight = _area.height - height();

        if ( _isVertical() ) {
            setPosition( _area.x + roiWidth / 2, _area.y + ( _currentIndex - _minIndex ) * roiHeight / ( _maxIndex - _minIndex ) );
        }
        else {
            setPosition( _area.x + ( _currentIndex - _minIndex ) * roiWidth / ( _maxIndex - _minIndex ), _area.y + roiHeight / 2 );
        }
    }

    void Scrollbar::moveToPos( const Point & position )
    {
        if ( _maxIndex == _minIndex )
            return;

        const int roiWidth = _area.width - width();
        const int roiHeight = _area.height - height();

        if ( _isVertical() ) {
            int32_t posY = position.y;
            const int32_t maxYPos = _area.y + roiHeight;

            if ( posY < _area.y )
                posY = _area.y;
            else if ( posY > maxYPos )
                posY = maxYPos;

            _currentIndex = ( posY - _area.y ) * ( _maxIndex - _minIndex ) / roiHeight;

            const int32_t posX = _area.x + roiWidth / 2;
            setPosition( posX, posY );
        }
        else {
            int32_t posX = position.x;
            const int32_t maxXPos = _area.x + roiWidth;

            if ( posX < _area.x )
                posX = _area.x;
            else if ( posX > maxXPos )
                posX = maxXPos;

            _currentIndex = ( posX - _area.x ) * ( _maxIndex - _minIndex ) / roiWidth;
            const int32_t posY = _area.y + roiHeight / 2;
            setPosition( posX, posY );
        }
    }

    bool Scrollbar::_isVertical() const
    {
        return _area.width < _area.height;
    }
}
