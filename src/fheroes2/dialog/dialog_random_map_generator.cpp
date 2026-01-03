/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2025 - 2026                                             *
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

#include "dialog_random_map_generator.h"

#include <algorithm>
#include <cassert>
#include <functional>
#include <string>
#include <utility>

#include "agg_image.h"
#include "dialog.h"
#include "game_hotkeys.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "map_random_generator.h"
#include "math_base.h"
#include "pal.h"
#include "screen.h"
#include "settings.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_scrollbar.h"
#include "ui_text.h"
#include "ui_tool.h"
#include "ui_window.h"

namespace
{
    class HorizontalSlider final
    {
    public:
        ~HorizontalSlider() = default;
        HorizontalSlider( const HorizontalSlider & ) = delete;
        HorizontalSlider & operator=( const HorizontalSlider & ) = delete;

        HorizontalSlider( const fheroes2::Point position, const int minIndex, const int maxIndex, const int currentIndex )
            : _timedButtonLeft( [this]() { return _buttonLeft.isPressed(); } )
            , _timedButtonRight( [this]() { return _buttonRight.isPressed(); } )
        {
            assert( minIndex <= maxIndex );
            assert( currentIndex >= minIndex && currentIndex <= maxIndex );

            fheroes2::Display & display = fheroes2::Display::instance();
            const int tradpostIcnId = Settings::Get().isEvilInterfaceEnabled() ? ICN::TRADPOSE : ICN::TRADPOST;
            const fheroes2::Sprite & bar = fheroes2::AGG::GetICN( tradpostIcnId, 1 );
            fheroes2::Blit( bar, display, position.x, position.y );

            constexpr int32_t buttonWidth{ 15 };
            _buttonLeft.setPosition( position.x + 6, position.y + 1 );
            _buttonLeft.setICNInfo( tradpostIcnId, 3, 4 );
            _buttonLeft.subscribe( &_timedButtonLeft );
            _buttonLeft.draw( display );
            _buttonRight.setPosition( position.x + bar.width() - buttonWidth, position.y + 1 );
            _buttonRight.setICNInfo( tradpostIcnId, 5, 6 );
            _buttonRight.subscribe( &_timedButtonRight );
            _buttonRight.draw( display );

            constexpr int32_t sliderLength{ 187 };
            const fheroes2::Sprite & originalSlider = fheroes2::AGG::GetICN( tradpostIcnId, 2 );
            const fheroes2::Image scrollbarSlider = fheroes2::generateScrollbarSlider( originalSlider, true, sliderLength, 1, static_cast<int32_t>( maxIndex + 1 ),
                                                                                       { 0, 0, 2, originalSlider.height() }, { 2, 0, 8, originalSlider.height() } );
            _scrollbar.setImage( scrollbarSlider );
            _scrollbar.setArea( { position.x + buttonWidth + 9, position.y + 3, sliderLength, 11 } );
            _scrollbar.setRange( minIndex, maxIndex );
            _scrollbar.moveToIndex( currentIndex );

            _scrollbar.show();
        }

        int getCurrentValue() const
        {
            return _scrollbar.currentIndex();
        }

        void setRange( const int minIndex, const int maxIndex )
        {
            _scrollbar.setImage( fheroes2::generateScrollbarSlider( _scrollbar, true, _scrollbar.getArea().width, 1, maxIndex + 1, { 0, 0, 2, _scrollbar.height() },
                                                                    { 2, 0, 8, _scrollbar.height() } ) );

            const int currentIndex = std::min( _scrollbar.currentIndex(), maxIndex );
            _scrollbar.setRange( minIndex, maxIndex );
            _scrollbar.moveToIndex( currentIndex );
        }

        bool processEvents( LocalEvent & le )
        {
            _buttonLeft.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( _buttonLeft.area() ) );
            _buttonRight.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( _buttonRight.area() ) );

            if ( le.isMouseLeftButtonPressedInArea( _scrollbar.getArea() ) ) {
                const int prevPosX = _scrollbar.x();
                _scrollbar.moveToPos( le.getMouseCursorPos() );

                // Return true only if the slider position has changed.
                return ( prevPosX != _scrollbar.x() );
            }

            if ( _scrollbar.updatePosition() ) {
                return true;
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

    private:
        fheroes2::Scrollbar _scrollbar;
        fheroes2::Button _buttonLeft;
        fheroes2::Button _buttonRight;

        fheroes2::TimedEventValidator _timedButtonLeft;
        fheroes2::TimedEventValidator _timedButtonRight;
    };

    class TextRestorer final
    {
    public:
        TextRestorer( fheroes2::Image & output, const int32_t positionX, const int32_t positionY )
            : _restorer( output, positionX, positionY, 150, 40 )
        {
            // Do nothing.
        }

        void render( std::string content, fheroes2::Image & output )
        {
            const fheroes2::Rect & roi = _restorer.rect();
            _restorer.restore();
            const fheroes2::Text text{ std::move( content ), fheroes2::FontType::normalYellow() };
            text.drawInRoi( roi.x, roi.y + 2, roi.width, output, roi );
        }

    private:
        fheroes2::ImageRestorer _restorer;
    };
}

bool fheroes2::randomMapGeneratorDialog( Maps::Random_Generator::Configuration & configuration, const int32_t mapWidth )
{
    if ( mapWidth < 1 ) {
        // What are you trying to achieve?!
        assert( 0 );
        return false;
    }

    // Verify the configuration parameters.
    configuration.playerCount = std::max<int32_t>( 2, configuration.playerCount );
    configuration.playerCount = std::min<int32_t>( 6, configuration.playerCount );

    const int32_t originalWaterPercentageLimit{ Maps::Random_Generator::calculateMaximumWaterPercentage( configuration.playerCount, mapWidth ) };
    configuration.waterPercentage = std::min( configuration.waterPercentage, originalWaterPercentageLimit );

    Display & display = Display::instance();
    const bool isDefaultScreenSize = display.isDefaultSize();

    StandardWindow window( Display::DEFAULT_WIDTH, Display::DEFAULT_HEIGHT, !isDefaultScreenSize, display );
    const Rect activeArea( window.activeArea() );

    const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

    if ( isDefaultScreenSize ) {
        const Sprite & backgroundImage = AGG::GetICN( isEvilInterface ? ICN::STONEBAK_EVIL : ICN::STONEBAK, 0 );
        Copy( backgroundImage, 0, 0, display, activeArea );
    }

    // Dialog title.
    const Sprite & titleBox = AGG::GetICN( isEvilInterface ? ICN::METALLIC_BORDERED_TEXTBOX_EVIL : ICN::METALLIC_BORDERED_TEXTBOX_GOOD, 0 );
    const Rect titleBoxRoi( activeArea.x + ( activeArea.width - titleBox.width() ) / 2, activeArea.y + 10, titleBox.width(), titleBox.height() );
    const Rect titleTextRoi( titleBoxRoi.x + 6, titleBoxRoi.y + 5, titleBoxRoi.width - 12, titleBoxRoi.height - 11 );

    Copy( titleBox, 0, 0, display, titleBoxRoi );
    addGradientShadow( titleBox, display, titleBoxRoi.getPosition(), { -5, 5 } );

    Text text( _( "Random Map Generator" ), FontType::normalWhite() );
    text.fitToOneRow( titleTextRoi.width );
    text.drawInRoi( titleTextRoi.x, titleTextRoi.y + 3, titleTextRoi.width, display, titleTextRoi );

    const int32_t positionX = activeArea.x + 12;
    const int32_t settingDescriptionWidth = activeArea.width / 3;
    const int32_t inputPositionX = positionX + settingDescriptionWidth;
    const int32_t valuePositionX = inputPositionX + 230;
    const int32_t ySpacing = 45;
    int32_t positionY = activeArea.y + 70;

    // Map configuration options.
    text.set( _( "rmg|Player count" ), FontType::normalWhite() );
    text.draw( positionX + ( settingDescriptionWidth - text.width() ) / 2, positionY, display );
    HorizontalSlider playerCountSlider{ { inputPositionX, positionY }, 2, 6, configuration.playerCount };
    TextRestorer playerCountValue{ display, valuePositionX, positionY };
    playerCountValue.render( std::to_string( configuration.playerCount ), display );

    positionY += ySpacing;

    text.set( _( "rmg|Map layout" ), FontType::normalWhite() );
    text.draw( positionX + ( settingDescriptionWidth - text.width() ) / 2, positionY, display );

    // Dropdown with map layout selection.
    const int dropListIcn = isEvilInterface ? ICN::DROPLISL_EVIL : ICN::DROPLISL;
    const Sprite & itemBackground = AGG::GetICN( dropListIcn, 0 );
    const int32_t layoutBackgroundWidth = 200;
    const int32_t layoutBackgroundHeight = itemBackground.height();

    Copy( itemBackground, 0, 0, display, inputPositionX + 6, positionY - 5, layoutBackgroundWidth, layoutBackgroundHeight );
    text.set( Maps::Random_Generator::layoutToString( configuration.mapLayout ), FontType::normalWhite() );
    text.draw( inputPositionX + 12, positionY, display );

    // TODO: remove the next line when the dropdown is operational.
    ApplyPalette( display, inputPositionX + 6, positionY - 5, display, inputPositionX + 6, positionY - 5, layoutBackgroundWidth, layoutBackgroundHeight,
                  PAL::GetPalette( PAL::PaletteType::DARKENING ) );

    const Sprite & dropListButtonSprite = AGG::GetICN( dropListIcn, 1 );
    const Sprite & dropListButtonPressedSprite = AGG::GetICN( dropListIcn, 2 );

    ButtonSprite layoutDroplistButton( inputPositionX + layoutBackgroundWidth + 6, positionY - 5, dropListButtonSprite, dropListButtonPressedSprite );
    // TODO: remove the next line when the dropdown is operational.
    layoutDroplistButton.disable();
    layoutDroplistButton.draw();

    positionY += ySpacing;

    text.set( _( "rmg|Water percentage" ), FontType::normalWhite() );
    text.draw( positionX + ( settingDescriptionWidth - text.width() ) / 2, positionY, display );

    HorizontalSlider waterSlider{ { inputPositionX, positionY }, 0, originalWaterPercentageLimit, configuration.waterPercentage };
    TextRestorer waterValue{ display, valuePositionX, positionY };
    waterValue.render( std::to_string( configuration.waterPercentage ), display );

    positionY += ySpacing;

    text.set( _( "rmg|Monster strength" ), FontType::normalWhite() );
    text.draw( positionX + ( settingDescriptionWidth - text.width() ) / 2, positionY, display );
    HorizontalSlider monsterSlider{ { inputPositionX, positionY }, 0, 3, static_cast<int>( configuration.monsterStrength ) };
    TextRestorer monsterValue{ display, valuePositionX, positionY };
    monsterValue.render( Maps::Random_Generator::monsterStrengthToString( configuration.monsterStrength ), display );

    positionY += ySpacing;

    text.set( _( "rmg|Resource availability" ), FontType::normalWhite() );
    text.draw( positionX + ( settingDescriptionWidth - text.width() ) / 2, positionY, display );
    HorizontalSlider resourceSlider{ { inputPositionX, positionY }, 0, 2, static_cast<int>( configuration.resourceDensity ) };
    TextRestorer resourceValue{ display, valuePositionX, positionY };
    resourceValue.render( Maps::Random_Generator::resourceDensityToString( configuration.resourceDensity ), display );

    positionY += ySpacing + 10;

    text.set( _( "rmg|Map seed" ), FontType::normalWhite() );
    text.draw( positionX + ( settingDescriptionWidth - text.width() ) / 2, positionY, display );

    ValueSelectionDialogElement mapSeedSelection{ 0, 999999, configuration.seed, 1, { positionX + settingDescriptionWidth + 4, positionY - 5 } };
    mapSeedSelection.draw( display );

    Button buttonCancel;
    const int buttonCancelIcn = isEvilInterface ? ICN::BUTTON_SMALL_CANCEL_EVIL : ICN::BUTTON_SMALL_CANCEL_GOOD;
    window.renderButton( buttonCancel, buttonCancelIcn, 0, 1, { 30, 10 }, StandardWindow::Padding::BOTTOM_RIGHT );

    Button buttonOk;
    const int buttonOkIcn = isEvilInterface ? ICN::BUTTON_SMALL_OKAY_EVIL : ICN::BUTTON_SMALL_OKAY_GOOD;
    window.renderButton( buttonOk, buttonOkIcn, 0, 1, { 30, 10 }, StandardWindow::Padding::BOTTOM_LEFT );

    display.render( window.totalArea() );

    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents() ) {
        buttonOk.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonOk.area() ) );
        buttonCancel.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonCancel.area() ) );

        if ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) || le.MouseClickLeft( buttonCancel.area() ) ) {
            return false;
        }

        if ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) || le.MouseClickLeft( buttonOk.area() ) ) {
            break;
        }

        if ( le.isMouseRightButtonPressedInArea( buttonOk.area() ) ) {
            showStandardTextMessage( _( "Okay" ), _( "Click to generate a new map." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( buttonCancel.area() ) ) {
            showStandardTextMessage( _( "Cancel" ), _( "Return to the previous menu." ), Dialog::ZERO );
        }
        else if ( playerCountSlider.processEvents( le ) ) {
            configuration.playerCount = playerCountSlider.getCurrentValue();

            const int32_t newLimit = Maps::Random_Generator::calculateMaximumWaterPercentage( configuration.playerCount, mapWidth );
            configuration.waterPercentage = std::min( configuration.waterPercentage, newLimit );
            waterSlider.setRange( 0, newLimit );

            playerCountValue.render( std::to_string( configuration.playerCount ), display );
            waterValue.render( std::to_string( configuration.waterPercentage ), display );
            display.render( window.activeArea() );
        }
        else if ( waterSlider.processEvents( le ) ) {
            configuration.waterPercentage = waterSlider.getCurrentValue();
            waterValue.render( std::to_string( configuration.waterPercentage ), display );
            display.render( window.activeArea() );
        }
        else if ( monsterSlider.processEvents( le ) ) {
            configuration.monsterStrength = static_cast<Maps::Random_Generator::MonsterStrength>( monsterSlider.getCurrentValue() );
            monsterValue.render( Maps::Random_Generator::monsterStrengthToString( configuration.monsterStrength ), display );
            display.render( window.activeArea() );
        }
        else if ( resourceSlider.processEvents( le ) ) {
            configuration.resourceDensity = static_cast<Maps::Random_Generator::ResourceDensity>( resourceSlider.getCurrentValue() );
            resourceValue.render( Maps::Random_Generator::resourceDensityToString( configuration.resourceDensity ), display );
            display.render( window.activeArea() );
        }
        else if ( mapSeedSelection.processEvents() ) {
            mapSeedSelection.draw( display );

            configuration.seed = mapSeedSelection.getValue();
            display.render( window.activeArea() );
        }
    }

    return true;
}
