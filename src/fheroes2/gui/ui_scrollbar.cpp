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

#include "ui_scrollbar.h"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace
{
    const int32_t minimumSliderLength = 15;
}

namespace fheroes2
{
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
            if ( isVertical() ) {
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

    bool Scrollbar::moveToIndex( const int indexId )
    {
        // TODO: Make a function to update the Scrollbar position after its image is changed
        // and call it in the code instead of this one with 'indexId' that is equal to '_currentId'.

        const int32_t roiWidth = _area.width - width();
        const int32_t roiHeight = _area.height - height();

        if ( _maxIndex == _minIndex ) {
            // There are less elements than the list height. We set scrollbar slider to the center of scrollbar.
            setPosition( _area.x + roiWidth / 2, _area.y + roiHeight / 2 );

            return false;
        }

        _currentIndex = std::clamp( indexId, _minIndex, _maxIndex );

        Point newPosition;

        if ( isVertical() ) {
            newPosition.x = _area.x + roiWidth / 2;
            newPosition.y = _area.y + ( _currentIndex - _minIndex ) * roiHeight / ( _maxIndex - _minIndex );
        }
        else {
            newPosition.x = _area.x + ( _currentIndex - _minIndex ) * roiWidth / ( _maxIndex - _minIndex );
            newPosition.y = _area.y + roiHeight / 2;
        }

        if ( newPosition.x != x() || newPosition.y != y() ) {
            // Update only on the change.
            setPosition( newPosition.x, newPosition.y );
            return true;
        }

        return false;
    }

    void Scrollbar::moveToPos( const Point & position )
    {
        if ( _maxIndex == _minIndex ) {
            return;
        }

        const int32_t roiWidth = _area.width - width();
        const int32_t roiHeight = _area.height - height();

        Point newPosition;

        if ( isVertical() ) {
            newPosition.y = std ::clamp( position.y - height() / 2, _area.y, _area.y + roiHeight );
            newPosition.x = _area.x + roiWidth / 2;
        }
        else {
            newPosition.x = std ::clamp( position.x - width() / 2, _area.x, _area.x + roiWidth );
            newPosition.y = _area.y + roiHeight / 2;
        }

        if ( newPosition.x != x() || newPosition.y != y() ) {
            // Update only on the change.

            const double indexPos = isVertical() ? static_cast<double>( newPosition.y - _area.y ) / roiHeight : static_cast<double>( newPosition.x - _area.x ) / roiWidth;
            _currentIndex = static_cast<int>( std::lround( indexPos * ( _maxIndex - _minIndex ) ) ) + _minIndex;

            setPosition( newPosition.x, newPosition.y );
        }
    }

    Image generateScrollbarSlider( const Image & originalSlider, const bool horizontalSlider, const int32_t sliderAreaLength, const int32_t elementCountPerView,
                                   const int32_t totalElementCount, const Rect & startSliderArea, const Rect & middleSliderArea )
    {
        if ( originalSlider.empty() ) {
            // Why do you pass an empty image?
            assert( 0 );
            return originalSlider;
        }

        assert( sliderAreaLength > 0 && elementCountPerView > 0 );

        if ( horizontalSlider ) {
            if ( middleSliderArea.width < 1 ) {
                // Middle area cannot be empty!
                assert( 0 );
                return originalSlider;
            }

            if ( sliderAreaLength < originalSlider.width() ) {
                // The slider is bigger than the area!
                assert( 0 );
                return originalSlider;
            }
        }
        else {
            if ( middleSliderArea.height < 1 ) {
                // Middle area cannot be empty!
                assert( 0 );
                return originalSlider;
            }

            if ( sliderAreaLength < originalSlider.height() ) {
                // The slider is bigger than the area!
                assert( 0 );
                return originalSlider;
            }
        }

        const int32_t currentSliderLength = horizontalSlider ? originalSlider.width() : originalSlider.height();

        if ( sliderAreaLength * elementCountPerView == currentSliderLength * totalElementCount ) {
            // There is no need change the slider image.
            return originalSlider;
        }

        const int32_t step = horizontalSlider ? middleSliderArea.width : middleSliderArea.height;
        const int32_t middleLength = ( sliderAreaLength * elementCountPerView / std::max( elementCountPerView, totalElementCount ) ) - currentSliderLength;

        int32_t width = originalSlider.width();
        int32_t height = originalSlider.height();

        if ( horizontalSlider ) {
            width = std::max( minimumSliderLength, std::max( width + middleLength, startSliderArea.width * 2 ) );
        }
        else {
            height = std::max( minimumSliderLength, std::max( height + middleLength, startSliderArea.height * 2 ) );
        }

        Image output( width, height );

        if ( originalSlider.singleLayer() ) {
            output._disableTransformLayer();
        }

        output.reset();

        // Copy the start slider part.
        Copy( originalSlider, startSliderArea.x, startSliderArea.y, output, startSliderArea.x, startSliderArea.y, startSliderArea.width, startSliderArea.height );

        if ( middleLength < 0 ) {
            // The slider is shortened. Copy the rest slider part from the end.
            if ( horizontalSlider ) {
                const int32_t copyWidth = width - startSliderArea.width;
                Copy( originalSlider, startSliderArea.x + originalSlider.width() - copyWidth, startSliderArea.y, output, startSliderArea.width, startSliderArea.y,
                      copyWidth, startSliderArea.height );
            }
            else {
                const int32_t copyHeight = height - startSliderArea.height;
                Copy( originalSlider, startSliderArea.x, startSliderArea.y + originalSlider.height() - copyHeight, output, startSliderArea.x, startSliderArea.height,
                      startSliderArea.width, copyHeight );
            }

            return output;
        }

        // The slider should be extended.

        int32_t offset = 0;
        if ( horizontalSlider ) {
            offset = startSliderArea.x + startSliderArea.width;
        }
        else {
            offset = startSliderArea.y + startSliderArea.height;
        }

        // Draw the middle slider part.
        const int32_t middleChunkCount = middleLength / step;
        for ( int32_t i = 0; i < middleChunkCount; ++i ) {
            if ( horizontalSlider ) {
                Copy( originalSlider, middleSliderArea.x, middleSliderArea.y, output, offset, startSliderArea.y, middleSliderArea.width, middleSliderArea.height );

                offset += middleSliderArea.width;
            }
            else {
                Copy( originalSlider, middleSliderArea.x, middleSliderArea.y, output, startSliderArea.x, offset, middleSliderArea.width, middleSliderArea.height );

                offset += middleSliderArea.height;
            }
        }

        // Draw leftovers of the middle part.
        const int32_t leftover = middleLength - middleChunkCount * step;
        if ( leftover > 0 ) {
            if ( horizontalSlider ) {
                Copy( originalSlider, middleSliderArea.x, middleSliderArea.y, output, offset, startSliderArea.y, leftover, middleSliderArea.height );

                offset += leftover;
            }
            else {
                Copy( originalSlider, middleSliderArea.x, middleSliderArea.y, output, startSliderArea.x, offset, middleSliderArea.width, leftover );

                offset += leftover;
            }
        }

        // Copy the end part.
        if ( horizontalSlider ) {
            Copy( originalSlider, startSliderArea.x + startSliderArea.width, startSliderArea.y, output, offset, startSliderArea.y,
                  originalSlider.width() - startSliderArea.width, startSliderArea.height );
        }
        else {
            Copy( originalSlider, startSliderArea.x, startSliderArea.y + startSliderArea.height, output, startSliderArea.x, offset, startSliderArea.width,
                  originalSlider.height() - startSliderArea.height );
        }

        return output;
    }
}
