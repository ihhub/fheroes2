/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2024                                             *
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
#include <utility>
#include <vector>

#include "math_base.h"

namespace fheroes2
{
    // Image always contains an image layer and if image is not a single-layer then also a transform layer.
    // - image layer contains visible pixels which are copy to a destination image
    // - transform layer is used to apply some transformation to an image on which we draw the current one. For example, shadowing
    class Image
    {
    public:
        Image() = default;
        Image( const int32_t width_, const int32_t height_ );

        Image( const Image & image_ );
        Image( Image && image_ ) noexcept;

        virtual ~Image() = default;

        Image & operator=( const Image & image_ );
        Image & operator=( Image && image_ ) noexcept;

        virtual void resize( const int32_t width_, const int32_t height_ );

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

        uint8_t * transform()
        {
            return _data.get() + width() * height();
        }

        const uint8_t * transform() const
        {
            return _data.get() + width() * height();
        }

        bool empty() const
        {
            return !_data;
        }

        void reset(); // makes image fully transparent (transform layer is set to 1)

        void clear(); // makes the image empty

        // Fill 'image' layer with given value, setting 'transform' layer to 0.
        void fill( const uint8_t value );

        // This is an optional indicator for image processing functions.
        // The whole image still consists of 2 layers but transform layer might be ignored in computations
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

        int32_t _width{ 0 };
        int32_t _height{ 0 };
        std::unique_ptr<uint8_t[]> _data; // holds 2 image layers

        // Only for images which are not used for any other operations except displaying on screen.
        bool _singleLayer{ false };
    };

    class Sprite : public Image
    {
    public:
        Sprite() = default;
        Sprite( const int32_t width_, const int32_t height_, const int32_t x_ = 0, const int32_t y_ = 0 );
        Sprite( const Image & image, const int32_t x_ = 0, const int32_t y_ = 0 );

        Sprite( const Sprite & sprite ) = default;
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

        virtual void setPosition( const int32_t x_, const int32_t y_ );

    private:
        int32_t _x{ 0 };
        int32_t _y{ 0 };
    };

    // This class is used in situations when we draw a window within another window
    class ImageRestorer
    {
    public:
        explicit ImageRestorer( Image & image );
        ImageRestorer( Image & image, const int32_t x_, const int32_t y_, const int32_t width, const int32_t height );

        ImageRestorer( const ImageRestorer & ) = delete;

        // Restores the original image if necessary, see the implementation for details
        ~ImageRestorer();

        void update( const int32_t x_, const int32_t y_, const int32_t width, const int32_t height );

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

        Rect rect() const
        {
            return { _x, _y, _width, _height };
        }

        void restore();

        void reset()
        {
            _isRestored = true;
        }

    private:
        Image & _image;
        Image _copy;

        int32_t _x{ 0 };
        int32_t _y{ 0 };
        int32_t _width{ 0 };
        int32_t _height{ 0 };

        void _updateRoi();

        bool _isRestored{ false };
    };

    // Apply shadow that gradually reduces strength using 'in' image shape. Shadow is applied to the 'out' image.
    void addGradientShadow( const Sprite & in, Image & out, const Point & outPos, const Point & shadowOffset );

    // Generates a new image with a shadow of the shape of existing image. Shadow must have only (-x, +y) offset.
    Sprite addShadow( const Sprite & in, const Point & shadowOffset, const uint8_t transformId );

    // Replace a particular pixel value by transparency value (transform layer value will be 1)
    void AddTransparency( Image & image, const uint8_t valueToReplace );

    // make sure that output image's transform layer doesn't have skipping values (transform == 1)
    void AlphaBlit( const Image & in, Image & out, const uint8_t alphaValue, const bool flip = false );
    void AlphaBlit( const Image & in, Image & out, int32_t outX, int32_t outY, const uint8_t alphaValue, const bool flip = false );
    void AlphaBlit( const Image & in, int32_t inX, int32_t inY, Image & out, int32_t outX, int32_t outY, int32_t width, int32_t height, const uint8_t alphaValue,
                    const bool flip = false );

    // apply palette only for image layer, it doesn't affect transform part
    void ApplyPalette( Image & image, const std::vector<uint8_t> & palette );
    void ApplyPalette( const Image & in, Image & out, const std::vector<uint8_t> & palette );
    void ApplyPalette( Image & image, const uint8_t paletteId );
    void ApplyPalette( const Image & in, Image & out, const uint8_t paletteId );
    void ApplyPalette( const Image & in, int32_t inX, int32_t inY, Image & out, int32_t outX, int32_t outY, int32_t width, int32_t height, uint8_t paletteId );
    void ApplyPalette( const Image & in, int32_t inX, int32_t inY, Image & out, int32_t outX, int32_t outY, int32_t width, int32_t height,
                       const std::vector<uint8_t> & palette );

    void ApplyAlpha( const Image & in, Image & out, const uint8_t alpha );
    void ApplyAlpha( const Image & in, int32_t inX, int32_t inY, Image & out, int32_t outX, int32_t outY, int32_t width, int32_t height, const uint8_t alpha );

    void ApplyTransform( Image & image, int32_t x, int32_t y, int32_t width, int32_t height, const uint8_t transformId );

    // draw one image onto another
    void Blit( const Image & in, Image & out, const bool flip = false );
    void Blit( const Image & in, Image & out, int32_t outX, int32_t outY, const bool flip = false );
    void Blit( const Image & in, int32_t inX, int32_t inY, Image & out, int32_t outX, int32_t outY, int32_t width, int32_t height, const bool flip = false );

    // inPos must contain non-negative values
    void Blit( const Image & in, const Point & inPos, Image & out, const Point & outPos, const Size & size, bool flip = false );

    void Copy( const Image & in, Image & out );
    void Copy( const Image & in, int32_t inX, int32_t inY, Image & out, const Rect & outRoi );
    void Copy( const Image & in, int32_t inX, int32_t inY, Image & out, int32_t outX, int32_t outY, int32_t width, int32_t height );

    // Copies transform the layer from in to out. Both images must be of the same size.
    void CopyTransformLayer( const Image & in, Image & out );

    Sprite CreateContour( const Image & image, const uint8_t value );

    // Make a transition to "in" image from left to right or vertically - from top to bottom using dithering (https://en.wikipedia.org/wiki/Dither).
    // The direction of transition can be reversed.
    void CreateDitheringTransition( const Image & in, int32_t inX, int32_t inY, Image & out, int32_t outX, int32_t outY, int32_t width, int32_t height,
                                    const bool isVertical, const bool isReverse );

    Sprite Crop( const Image & image, int32_t x, int32_t y, int32_t width, int32_t height );

    // skipFactor is responsible for non-solid line. You can interpret it as skip every N pixel
    void DrawBorder( Image & image, const uint8_t value, const uint32_t skipFactor = 0 );

    // roi is an optional parameter when you need to draw in a small than image area
    void DrawLine( Image & image, const Point & start, const Point & end, const uint8_t value, const Rect & roi = Rect() );

    void DrawRect( Image & image, const Rect & roi, const uint8_t value );

    void DivideImageBySquares( const Point & spriteOffset, const Image & original, const int32_t squareSize, std::vector<Point> & outputSquareId,
                               std::vector<std::pair<Point, Rect>> & outputImageInfo );

    // Every image in the array must be the same size. Make sure that pointers aren't nullptr!
    Image ExtractCommonPattern( const std::vector<const Image *> & input );

    // Please use GetColorId function if you want to use an RGB value
    void Fill( Image & image, int32_t x, int32_t y, int32_t width, int32_t height, const uint8_t colorId );

    void FillTransform( Image & image, int32_t x, int32_t y, int32_t width, int32_t height, const uint8_t transformId );

    Image FilterOnePixelNoise( const Image & input );

    bool FitToRoi( const Image & in, Point & inPos, const Image & out, Point & outPos, Size & outputSize, const Rect & outputRoi );

    Image Flip( const Image & in, const bool horizontally, const bool vertically );
    void Flip( const Image & in, int32_t inX, int32_t inY, Image & out, int32_t outX, int32_t outY, int32_t width, int32_t height, const bool horizontally,
               const bool vertically );

    // Return ROI with pixels which are not skipped and not used for shadow creation. 1 is to skip, 2 - 5 types of shadows
    Rect GetActiveROI( const Image & image, const uint8_t minTransformValue = 6 );

    // Returns a closest color ID from the original game's palette
    uint8_t GetColorId( const uint8_t red, const uint8_t green, const uint8_t blue );

    std::vector<uint8_t> getTransformTable( const Image & in, const Image & out, int32_t x, int32_t y, int32_t width, int32_t height );

    Sprite makeShadow( const Sprite & in, const Point & shadowOffset, const uint8_t transformId );

    void MaskTransformLayer( const Image & mask, int32_t maskX, int32_t maskY, Image & out, int32_t outX, int32_t outY, int32_t width, int32_t height );

    // This function does NOT check transform layer. If you intent to replace few colors at the same image please use ApplyPalette to be more efficient.
    void ReplaceColorId( Image & image, const uint8_t oldColorId, const uint8_t newColorId );

    // Use this function only when you need to convert pixel value into transform layer
    void ReplaceColorIdByTransformId( Image & image, const uint8_t colorId, const uint8_t transformId );

    // Use this function only when you need to convert transform value into non-transparent pixel with the given color.
    void ReplaceTransformIdByColorId( Image & image, const uint8_t transformId, const uint8_t colorId );

    void Resize( const Image & in, Image & out );

    void Resize( const Image & in, const int32_t inX, const int32_t inY, const int32_t widthRoiIn, const int32_t heightRoiIn, Image & out, const int32_t outX,
                 const int32_t outY, const int32_t widthRoiOut, const int32_t heightRoiOut );

    // Please use value from the main palette only
    void SetPixel( Image & image, const int32_t x, const int32_t y, const uint8_t value );

    void SetPixel( Image & image, const std::vector<Point> & points, const uint8_t value );

    // Please set value not bigger than 13!
    void SetTransformPixel( Image & image, const int32_t x, const int32_t y, const uint8_t value );

    Image Stretch( const Image & in, int32_t inX, int32_t inY, int32_t widthIn, int32_t heightIn, const int32_t widthOut, const int32_t heightOut );

    void SubpixelResize( const Image & in, Image & out );

    void SubpixelResize( const Image & in, const int32_t inX, const int32_t inY, const int32_t widthRoiIn, const int32_t heightRoiIn, Image & out, const int32_t outX,
                         const int32_t outY, const int32_t widthRoiOut, const int32_t heightRoiOut );

    void Transpose( const Image & in, Image & out );

    void updateShadow( Image & image, const Point & shadowOffset, const uint8_t transformId, const bool connectCorners );
}
