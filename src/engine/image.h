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

#include <cstdint>
#include <memory>
#include <vector>

#include "math_base.h"

namespace fheroes2
{
    // Image contains image layer and transform layer.
    // - image layer contains visible pixels which are copy to a destination image
    // - transform layer is used to apply some transformation to an image on which we draw the current one. For example, shadowing
    class Image
    {
    public:
        Image( int32_t width_ = 0, int32_t height_ = 0 );
        Image( const Image & image_ );
        Image( Image && image_ ) noexcept;

        virtual ~Image() = default;

        Image & operator=( const Image & image_ );
        Image & operator=( Image && image_ ) noexcept;

        virtual void resize( int32_t width_, int32_t height_ );

        // It's safe to cast to uint32_t as width and height are always >= 0
        int32_t width() const
        {
            return _width;
        }

        int32_t height() const
        {
            return _height;
        }

        virtual uint8_t * image();
        virtual const uint8_t * image() const;

        uint8_t * transform();
        const uint8_t * transform() const;

        bool empty() const
        {
            return !_data;
        }

        void reset(); // makes image fully transparent (transform layer is set to 1)
        void clear(); // makes the image empty

        void fill( uint8_t value ); // fill 'image' layer with given value, setting 'transform' layer set to 0

        // This is an optional indicator for image processing functions.
        // The whole image still consists of 2 layers but transform layer might be ignored in computations.
        bool singleLayer() const
        {
            return _singleLayer;
        }

        // BE CAREFUL! This method disables transform layer usage. Use only for display / video related images which are for end rendering purposes!
        // The name of this method starts from _ on purpose to do not mix with other public methods.
        void _disableTransformLayer()
        {
            _singleLayer = true;
        }

    private:
        void copy( const Image & image );

        int32_t _width;
        int32_t _height;
        std::unique_ptr<uint8_t[]> _data; // holds 2 image layers

        bool _singleLayer; // only for images which are not used for any other operations except displaying on screen. Non-copyable member.
    };

    class Sprite : public Image
    {
    public:
        Sprite( int32_t width_ = 0, int32_t height_ = 0, int32_t x_ = 0, int32_t y_ = 0 );
        Sprite( const Image & image, int32_t x_ = 0, int32_t y_ = 0 );
        Sprite( const Sprite & sprite );
        Sprite( Sprite && sprite ) noexcept;

        ~Sprite() override = default;

        Sprite & operator=( const Sprite & sprite );
        Sprite & operator=( Sprite && sprite ) noexcept;

        int32_t x() const
        {
            return _x;
        }

        int32_t y() const
        {
            return _y;
        }

        virtual void setPosition( int32_t x_, int32_t y_ );

    private:
        int32_t _x;
        int32_t _y;
    };

    // This class is used in situations when we draw a window within another window
    class ImageRestorer
    {
    public:
        explicit ImageRestorer( Image & image );
        ImageRestorer( Image & image, int32_t x_, int32_t y_, int32_t width, int32_t height );
        ~ImageRestorer(); // restore method will be call upon object's destruction

        ImageRestorer( const ImageRestorer & ) = delete;

        void update( int32_t x_, int32_t y_, int32_t width, int32_t height );

        int32_t x() const
        {
            return _x;
        }

        int32_t y() const
        {
            return _y;
        }

        int32_t width() const
        {
            return _width;
        }

        int32_t height() const
        {
            return _height;
        }

        void restore();
        void reset();

    private:
        Image & _image;
        Image _copy;

        int32_t _x;
        int32_t _y;
        int32_t _width;
        int32_t _height;

        void _updateRoi();

        bool _isRestored;
    };

    // Replace a particular pixel value by transparency value (transform layer value will be 1)
    void AddTransparency( Image & image, uint8_t valueToReplace );

    // make sure that output image's transform layer doesn't have skipping values (transform == 1)
    void AlphaBlit( const Image & in, Image & out, uint8_t alphaValue, bool flip = false );
    void AlphaBlit( const Image & in, Image & out, int32_t outX, int32_t outY, uint8_t alphaValue, bool flip = false );
    void AlphaBlit( const Image & in, int32_t inX, int32_t inY, Image & out, int32_t outX, int32_t outY, int32_t width, int32_t height, uint8_t alphaValue,
                    bool flip = false );

    // inPos must contain non-negative values
    void AlphaBlit( const Image & in, const Point & inPos, Image & out, const Point & outPos, const Size & size, bool flip = false );

    // apply palette only for image layer, it doesn't affect transform part
    void ApplyPalette( Image & image, const std::vector<uint8_t> & palette );
    void ApplyPalette( const Image & in, Image & out, const std::vector<uint8_t> & palette );
    void ApplyPalette( Image & image, uint8_t paletteId );
    void ApplyPalette( const Image & in, Image & out, uint8_t paletteId );
    void ApplyPalette( const Image & in, int32_t inX, int32_t inY, Image & out, int32_t outX, int32_t outY, int32_t width, int32_t height, uint8_t paletteId );
    void ApplyPalette( const Image & in, int32_t inX, int32_t inY, Image & out, int32_t outX, int32_t outY, int32_t width, int32_t height,
                       const std::vector<uint8_t> & palette );

    void ApplyAlpha( const Image & in, Image & out, uint8_t alpha );
    void ApplyAlpha( const Image & in, int32_t inX, int32_t inY, Image & out, int32_t outX, int32_t outY, int32_t width, int32_t height, uint8_t alpha );

    void ApplyTransform( Image & image, int32_t x, int32_t y, int32_t width, int32_t height, uint8_t transformId );

    // draw one image onto another
    void Blit( const Image & in, Image & out, bool flip = false );
    void Blit( const Image & in, Image & out, int32_t outX, int32_t outY, bool flip = false );
    void Blit( const Image & in, int32_t inX, int32_t inY, Image & out, int32_t outX, int32_t outY, int32_t width, int32_t height, bool flip = false );

    // inPos must contain non-negative values
    void Blit( const Image & in, const Point & inPos, Image & out, const Point & outPos, const Size & size, bool flip = false );

    void Copy( const Image & in, Image & out );
    void Copy( const Image & in, int32_t inX, int32_t inY, Image & out, int32_t outX, int32_t outY, int32_t width, int32_t height );

    Image CreateBlurredImage( const Image & in, int32_t blurRadius );

    Sprite CreateContour( const Image & image, uint8_t value );

    Sprite Crop( const Image & image, int32_t x, int32_t y, int32_t width, int32_t height );

    // skipFactor is responsible for non-solid line. You can interpret it as skip every N pixel
    void DrawBorder( Image & image, uint8_t value, uint32_t skipFactor = 0 );

    // roi is an optional parameter when you need to draw in a small than image area
    void DrawLine( Image & image, const Point & start, const Point & end, uint8_t value, const Rect & roi = Rect() );

    void DrawRect( Image & image, const Rect & roi, uint8_t value );

    // Every image in the array must be the same size.
    Image ExtractCommonPattern( const std::vector<Image> & input );

    // Please use GetColorId function if you want to use an RGB value
    void Fill( Image & image, int32_t x, int32_t y, int32_t width, int32_t height, uint8_t colorId );

    Image FilterOnePixelNoise( const Image & input );

    bool FitToRoi( const Image & in, Point & inPos, const Image & out, Point & outPos, Size & outputSize, const Rect & outputRoi );

    Image Flip( const Image & in, bool horizontally, bool vertically );

    // Return ROI with pixels which are not skipped and not used for shadow creation. 1 is to skip, 2 - 5 types of shadows
    Rect GetActiveROI( const Image & image, const uint8_t minTransformValue = 6 );

    // Returns a closest color ID from the original game's palette
    uint8_t GetColorId( uint8_t red, uint8_t green, uint8_t blue );

    // This function does NOT check transform layer. If you intent to replace few colors at the same image please use ApplyPalette to be more efficient.
    void ReplaceColorId( Image & image, uint8_t oldColorId, uint8_t newColorId );

    // Use this function only when you need to convert pixel value into transform layer
    void ReplaceColorIdByTransformId( Image & image, uint8_t colorId, uint8_t transformId );

    // Please remember that subpixel accuracy resizing is extremely slow!
    void Resize( const Image & in, Image & out, const bool isSubpixelAccuracy = false );

    void Resize( const Image & in, const int32_t inX, const int32_t inY, const int32_t widthRoiIn, const int32_t heightRoiIn, Image & out, const int32_t outX,
                 const int32_t outY, const int32_t widthRoiOut, const int32_t heightRoiOut, const bool isSubpixelAccuracy = false );

    // Please use value from the main palette only
    void SetPixel( Image & image, int32_t x, int32_t y, uint8_t value );

    void SetPixel( Image & image, const std::vector<Point> & points, uint8_t value );

    // Please set value not bigger than 13!
    void SetTransformPixel( Image & image, int32_t x, int32_t y, uint8_t value );

    Image Stretch( const Image & in, int32_t inX, int32_t inY, int32_t widthIn, int32_t heightIn, int32_t widthOut, int32_t heightOut );

    void Transpose( const Image & in, Image & out );
}
