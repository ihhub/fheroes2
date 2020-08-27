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

#include <cmath>
#include <stdint.h>

// Below source code was partially taken from https://github.com/ihhub/penguinv open source project
namespace fheroes2
{
    // overload this function for floating types
    template <typename _Type>
    bool isEqual( const _Type & value1, const _Type & value2 )
    {
        return ( value1 == value2 );
    }

    template <typename _Type>
    struct PointBase2D
    {
        PointBase2D( _Type _x = 0, _Type _y = 0 )
            : x( _x )
            , y( _y )
        {}

        bool operator==( const PointBase2D & point ) const
        {
            return isEqual( x, point.x ) && isEqual( y, point.y );
        }

        bool operator!=( const PointBase2D & point ) const
        {
            return !( *this == point );
        }

        PointBase2D & operator+=( const PointBase2D & point )
        {
            x += point.x;
            y += point.y;
            return *this;
        }

        PointBase2D & operator-=( const PointBase2D & point )
        {
            x -= point.x;
            y -= point.y;
            return *this;
        }

        PointBase2D operator+( const PointBase2D & point ) const
        {
            return PointBase2D( x + point.x, y + point.y );
        }

        PointBase2D operator-( const PointBase2D & point ) const
        {
            return PointBase2D( x - point.x, y - point.y );
        }

        PointBase2D operator*( const _Type & value ) const
        {
            return PointBase2D( value * x, value * y );
        }

        _Type x;
        _Type y;
    };

    template <typename _Type, typename T>
    PointBase2D<_Type> operator*( const T & value, const PointBase2D<_Type> & point )
    {
        return PointBase2D<_Type>( static_cast<_Type>( value ) * point.x, static_cast<_Type>( value ) * point.y );
    }

    template <typename _Type>
    struct SizeBase2D
    {
        SizeBase2D( _Type _width = 0, _Type _height = 0 )
            : width( _width )
            , height( _height )
        {}

        bool operator==( const SizeBase2D & size ) const
        {
            return isEqual( width, size.width ) && isEqual( height, size.height );
        }

        bool operator!=( const SizeBase2D & size ) const
        {
            return !( *this == size );
        }

        SizeBase2D & operator+=( const SizeBase2D & size )
        {
            width += size.width;
            height += size.height;
            return *this;
        }

        SizeBase2D & operator-=( const SizeBase2D & size )
        {
            width -= size.width;
            height -= size.height;
            return *this;
        }

        SizeBase2D operator+( const SizeBase2D & size ) const
        {
            return SizeBase2D( width + size.width, height + size.height );
        }

        SizeBase2D operator-( const SizeBase2D & size ) const
        {
            return SizeBase2D( width - size.width, height - size.height );
        }

        SizeBase2D operator*( const _Type & value ) const
        {
            return SizeBase2D( value * width, value * height );
        }

        _Type width;
        _Type height;
    };

    template <typename _TypePoint, typename _TypeSize>
    struct RectBase2D
    {
        RectBase2D( _TypePoint _x = 0, _TypePoint _y = 0, _TypeSize _width = 0, _TypeSize _height = 0 )
            : x( _x )
            , y( _y )
            , width( _width )
            , height( _height )
        {}

        RectBase2D( const PointBase2D<_TypePoint> & point, const SizeBase2D<_TypeSize> & size )
            : x( point.x )
            , y( point.y )
            , width( size.width )
            , height( size.height )
        {}

        bool operator==( const RectBase2D & rect ) const
        {
            return isEqual( x, rect.x ) && isEqual( y, rect.y ) && isEqual( width, rect.width ) && isEqual( height, rect.height );
        }

        bool operator!=( const RectBase2D & rect ) const
        {
            return !( *this == rect );
        }

        RectBase2D & operator+=( const PointBase2D<_TypePoint> & point )
        {
            x += point.x;
            y += point.y;
            return *this;
        }

        RectBase2D & operator-=( const PointBase2D<_TypePoint> & point )
        {
            x -= point.x;
            y -= point.y;
            return *this;
        }

        RectBase2D operator+( const PointBase2D<_TypePoint> & point ) const
        {
            return RectBase2D( x + point.x, y + point.y, width, height );
        }

        RectBase2D operator-( const PointBase2D<_TypePoint> & point ) const
        {
            return RectBase2D( x - point.x, y - point.y, width, height );
        }

        _TypePoint x;
        _TypePoint y;
        _TypeSize width;
        _TypeSize height;
    };

    typedef PointBase2D<int32_t> Point;
    typedef SizeBase2D<uint32_t> Size;
    typedef RectBase2D<int32_t, uint32_t> Rect;
}
