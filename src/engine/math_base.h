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
#include <cstdint>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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
        PointBase2D()
            : x( 0 )
            , y( 0 )
        {}

        PointBase2D( _Type _x, _Type _y )
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

        bool operator<( const PointBase2D & point ) const
        {
            return x == point.x ? y < point.y : x < point.x;
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
        SizeBase2D()
            : width( 0 )
            , height( 0 )
        {}

        SizeBase2D( _Type _width, _Type _height )
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

        bool operator<( const SizeBase2D & size ) const
        {
            return width < size.width || ( width == size.width && height < size.height );
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
        RectBase2D()
            : x( 0 )
            , y( 0 )
            , width( 0 )
            , height( 0 )
        {}

        RectBase2D( _TypePoint _x, _TypePoint _y, _TypeSize _width, _TypeSize _height )
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

        // Check whether a point within the rectangle
        bool operator&( const PointBase2D<_TypePoint> & point ) const
        {
            return point.x >= x && point.y >= y && point.x < ( x + width ) && point.y < ( y + height );
        }

        // Check whether rectangles are intersecting each other
        bool operator&( const RectBase2D & rect ) const
        {
            return x <= rect.x + rect.width && rect.x <= x + width && y <= rect.y + rect.height && rect.y <= y + height;
        }

        // Find intersection rectangle
        RectBase2D operator^( const RectBase2D & rect ) const
        {
            RectBase2D output = rect;
            if ( output.x < x ) {
                const _TypePoint diff = x - output.x;
                output.x = x;
                output.width -= diff;
            }
            if ( output.y < y ) {
                const _TypePoint diff = y - output.y;
                output.y = y;
                output.height -= diff;
            }

            if ( output.x > x + width || output.y > y + height )
                return RectBase2D();

            if ( output.x + output.width > x + width ) {
                const _TypePoint diff = output.x + output.width - ( x + width );
                output.width -= diff;
            }

            if ( output.y + output.height > y + height ) {
                const _TypePoint diff = output.y + output.height - ( y + height );
                output.height -= diff;
            }

            return output;
        }

        PointBase2D<_TypePoint> getPosition() const
        {
            return PointBase2D<_TypePoint>( x, y );
        }

        _TypePoint x;
        _TypePoint y;
        _TypeSize width;
        _TypeSize height;
    };

    using Point = PointBase2D<int32_t>;
    using Size = SizeBase2D<int32_t>;
    using Rect = RectBase2D<int32_t, int32_t>;
}
