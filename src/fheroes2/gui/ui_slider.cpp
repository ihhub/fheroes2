/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2026                                                    *
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

#include "ui_slider.h"

#include <algorithm>
#include <cassert>
#include <functional>
#include <utility>

#include "game_assets.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "pal.h"
#include "screen.h"
#include "settings.h"

namespace fheroes2
{
    HorizontalSlider::HorizontalSlider( const int32_t width, const Point position, const int minIndex, const int maxIndex, const int currentIndex )
        : _timedButtonLeft( [this]() { return _buttonLeft.isPressed(); } )
        , _timedButtonRight( [this]() { return _buttonRight.isPressed(); } )
    {
        assert( minIndex <= maxIndex );
        assert( currentIndex >= minIndex && currentIndex <= maxIndex );

        Display & display = Display::instance();
        const int tradpostIcnId = Settings::Get().isEvilInterfaceEnabled() ? ICN::TRADPOSE : ICN::TRADPOST;
        const Sprite & bar = Assets::getImage( tradpostIcnId, 1 );
        // The original slider is 230 pixels wide with the active slider area of 187.
        constexpr int32_t buttonWidth{ 15 };

        constexpr int32_t leftOffset{ 24 };
        constexpr int32_t rightOffset{ 19 };
        const int32_t leftSliderArea{ leftOffset + width / 2 };
        const int32_t rightSliderArea{ rightOffset + width - width / 2 };

        Blit( bar, 0, 0, display, position.x, position.y, leftSliderArea, bar.height() );
        Blit( bar, bar.width() - rightSliderArea, 0, display, position.x + leftSliderArea, position.y, rightSliderArea, bar.height() );

        _buttonLeft.setPosition( position.x + 6, position.y + 1 );
        _buttonLeft.setICNInfo( tradpostIcnId, 3, 4 );
        _buttonLeft.subscribe( &_timedButtonLeft );
        _buttonLeft.draw( display );
        _buttonRight.setPosition( position.x + leftOffset + width + rightOffset - buttonWidth, position.y + 1 );
        _buttonRight.setICNInfo( tradpostIcnId, 5, 6 );
        _buttonRight.subscribe( &_timedButtonRight );
        _buttonRight.draw( display );

        const Sprite & originalSlider = Assets::getImage( tradpostIcnId, 2 );
        Image scrollbarSlider = generateScrollbarSlider( originalSlider, true, width, 1, static_cast<int32_t>( maxIndex - minIndex + 1 ),
                                                         { 0, 0, 2, originalSlider.height() }, { 2, 0, 8, originalSlider.height() } );
        _scrollbar.setImage( std::move( scrollbarSlider ) );
        _scrollbar.setArea( { position.x + leftOffset, position.y + 3, width, 11 } );
        _scrollbar.setRange( minIndex, maxIndex );
        _scrollbar.moveToIndex( currentIndex );

        _scrollbar.show();
    }

    void HorizontalSlider::setRange( const int minIndex, const int maxIndex )
    {
        _scrollbar.setImage( generateScrollbarSlider( _scrollbar, true, _scrollbar.getArea().width, 1, static_cast<int32_t>( maxIndex - minIndex + 1 ),
                                                      { 0, 0, 2, _scrollbar.height() }, { 2, 0, 8, _scrollbar.height() } ) );

        const int currentIndex = std::min( _scrollbar.currentIndex(), maxIndex );
        _scrollbar.setRange( minIndex, maxIndex );
        _scrollbar.moveToIndex( currentIndex );
    }

    bool HorizontalSlider::processEvents( LocalEvent & le )
    {
        _buttonLeft.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( _buttonLeft.area() ) );
        _buttonRight.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( _buttonRight.area() ) );

        if ( le.isMouseLeftButtonPressedInArea( _scrollbar.getArea() ) ) {
            const int32_t prevValue{ _scrollbar.currentIndex() };
            const int prevPosX = _scrollbar.x();
            _scrollbar.moveToPos( le.getMouseCursorPos() );

            // Return true only if the slider position has changed.
            return ( prevPosX != _scrollbar.x() ) || ( prevValue != _scrollbar.currentIndex() );
        }

        if ( le.MouseClickLeft( _buttonLeft.area() ) || le.isMouseWheelDownInArea( _scrollbar.getArea() ) || _timedButtonLeft.isDelayPassed() ) {
            if ( _scrollbar.currentIndex() == _scrollbar.minIndex() ) {
                return false;
            }

            _scrollbar.backward();
            return true;
        }

        if ( le.MouseClickLeft( _buttonRight.area() ) || le.isMouseWheelUpInArea( _scrollbar.getArea() ) || _timedButtonRight.isDelayPassed() ) {
            if ( _scrollbar.currentIndex() == _scrollbar.maxIndex() ) {
                return false;
            }

            _scrollbar.forward();
            return true;
        }

        return false;
    }

    void HorizontalSlider::disable()
    {
        _buttonLeft.disable();
        _buttonRight.disable();

        _buttonLeft.draw();
        _buttonRight.draw();

        Copy( _scrollbar, _scrollbarBackup );
        ApplyPalette( _scrollbar, PAL::GetPalette( PAL::PaletteType::DARKENING ) );
        _scrollbar.redraw();
    }

    void HorizontalSlider::enable()
    {
        _buttonLeft.enable();
        _buttonRight.enable();

        _buttonLeft.draw();
        _buttonRight.draw();

        if ( !_scrollbarBackup.empty() ) {
            _scrollbar.setImage( _scrollbarBackup );
            _scrollbar.redraw();
        }
    }
}
