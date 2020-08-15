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

#include <stdint.h>
#include <string>
#include <vector>

#include "math_base.h"

namespace fheroes2
{
    // Image contains image layer and transform layer.
    // - transform layer is used to apply some changes to an image on which we draw the current one. For example, shadowing
    // - image layer is used to copy pixel value to destination image
    class Image
    {
    public:
        Image( uint32_t width_ = 0, uint32_t height_ = 0 );
        virtual ~Image();

        virtual void resize( uint32_t width_, uint32_t height_ );
        uint32_t width() const;
        uint32_t height() const;

        virtual uint8_t * image();
        virtual const uint8_t * image() const;

        uint8_t * transform();
        const uint8_t * transform() const;

        bool empty() const;

        void reset(); // sets everything to 0: black image
        void clear(); // makes the image empty

        void fill( uint8_t value ); // fill only 'image' layer

    private:
        uint32_t _width;
        uint32_t _height;
        std::vector<uint8_t> _image;
        std::vector<uint8_t> _transform;
    };

    class Sprite : public Image
    {
    public:
        Sprite( uint32_t width_ = 0, uint32_t height_ = 0, int32_t x_ = 0, int32_t y_ = 0 );
        virtual ~Sprite();

        int32_t x() const;
        int32_t y() const;

        void setPosition( int32_t x_, int32_t y_ );

    private:
        int32_t _x;
        int32_t _y;
    };

    // This class is used in situations when we draw a window within another window
    class ImageRestorer
    {
    public:
        explicit ImageRestorer( Image & image );
        ImageRestorer( Image & image, uint32_t x, uint32_t y, uint32_t width, uint32_t height );
        ~ImageRestorer(); // restore method will be call upon object's destruction

        void restore();

    private:
        Image & _image;
        Image _copy;

        uint32_t _x;
        uint32_t _y;
        uint32_t _width;
        uint32_t _height;

        bool _isRestored;
    };

    void AlphaBlit( const Image & in, Image & out, uint8_t alphaValue, bool flip = false );
    void AlphaBlit( const Image & in, Image & out, int32_t outX, int32_t outY, uint8_t alphaValue, bool flip = false );
    void AlphaBlit( const Image & in, uint32_t inX, uint32_t inY, Image & out, int32_t outX, int32_t outY, uint32_t width, uint32_t height, uint8_t alphaValue,
                    bool flip = false );

    // inPos must contain non-negative values
    void AlphaBlit( const Image & in, const Point & inPos, Image & out, const Point & outPos, const Size & size, bool flip = false );

    // apply palette only for image layer, it doesn't affect transform part
    void ApplyPallete( Image & image, const std::vector<uint8_t> & palette );
    void ApplyPallete( const Image & in, Image & out, const std::vector<uint8_t> & palette );

    // draw one image onto another
    void Blit( const Image & in, Image & out, bool flip = false );
    void Blit( const Image & in, Image & out, int32_t outX, int32_t outY, bool flip = false );
    void Blit( const Image & in, uint32_t inX, uint32_t inY, Image & out, int32_t outX, int32_t outY, uint32_t width, uint32_t height, bool flip = false );

    // inPos must contain non-negative values
    void Blit( const Image & in, const Point & inPos, Image & out, const Point & outPos, const Size & size, bool flip = false );

    void Copy( const Image & in, Image & out );
    void Copy( const Image & in, uint32_t inX, uint32_t inY, Image & out, uint32_t outX, uint32_t outY, uint32_t width, uint32_t height );

    Image CreateContour( const Image & image );

    Sprite Crop( const Image & image, int32_t x, int32_t y, uint32_t width, uint32_t height );

    void DrawBorder( Image & image, uint8_t value );

    void DrawLine( Image & image, const Point & start, const Point & end, const Rect & roi );

    // Returns a closest color ID from the original game's palette
    uint8_t GetColorId( uint8_t red, uint8_t green, uint8_t blue );

    // Please remember that subpixel accuracy resizing is extremely slow!
    void Resize( const Image & in, Image & out, bool isSubpixelAccuracy = false );

    // Please use value from the main palette only
    void SetPixel( Image & image, uint32_t x, uint32_t y, uint8_t value );
}
