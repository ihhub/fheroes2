/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2025                                                    *
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

#include "dialog_random_map.h"

#include <algorithm>
#include <string>
#include <utility>

#include "agg_image.h"
#include "game_hotkeys.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "map_random_generator.h"
#include "math_base.h"
#include "screen.h"
#include "settings.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_scrollbar.h"
#include "ui_text.h"
#include "ui_window.h"

namespace
{
    class HorizontalSlider final
    {
    public:
        HorizontalSlider() = default;
        ~HorizontalSlider() override = default;
        HorizontalSlider( const HorizontalSlider & ) = delete;
        HorizontalSlider & operator=( const HorizontalSlider & ) = delete;

        HorizontalSlider( const fheroes2::Point position, const int minIndex, const int maxIndex, const int startIndex )
        {
            const int tradpostIcnId = Settings::Get().isEvilInterfaceEnabled() ? ICN::TRADPOSE : ICN::TRADPOST;
            const int32_t sliderLength = 187;
            const int32_t buttonWidth = 15;

            fheroes2::Display & display = fheroes2::Display::instance();
            const fheroes2::Sprite & bar = fheroes2::AGG::GetICN( tradpostIcnId, 1 );
            fheroes2::Blit( bar, display, position.x, position.y );

            _buttonLeft.setPosition( position.x + 6, position.y + 1 );
            _buttonLeft.setICNInfo( tradpostIcnId, 3, 4 );
            _buttonRight.setPosition( position.x + bar.width() - buttonWidth, position.y + 1 );
            _buttonRight.setICNInfo( tradpostIcnId, 5, 6 );

            const fheroes2::Sprite & originalSlider = fheroes2::AGG::GetICN( tradpostIcnId, 2 );
            const fheroes2::Image scrollbarSlider = fheroes2::generateScrollbarSlider( originalSlider, true, sliderLength, 1, static_cast<int32_t>( maxIndex + 1 ),
                                                                                       { 0, 0, 2, originalSlider.height() }, { 2, 0, 8, originalSlider.height() } );
            _scrollbar.setImage( scrollbarSlider );
            _scrollbar.setArea( { position.x + buttonWidth + 9, position.y + 3, sliderLength, 11 } );
            _scrollbar.setRange( minIndex, maxIndex );
            _scrollbar.moveToIndex( startIndex );

            _scrollbar.show();
            redraw( display );
        }

        int getCurrentValue() const
        {
            return _scrollbar.currentIndex();
        }

        void setRange( const int minIndex, const int maxIndex )
        {
            if ( _scrollbar.currentIndex() > maxIndex ) {
                _scrollbar.moveToIndex( maxIndex );
            }
            _scrollbar.setRange( minIndex, maxIndex );
        }

        void redraw( fheroes2::Image & output = fheroes2::Display::instance() ) const
        {
            _buttonLeft.draw( output );
            _buttonRight.draw( output );
        }

        bool processEvent( LocalEvent & le )
        {
            if ( le.isMouseLeftButtonPressedInArea( _scrollbar.getArea() ) ) {
                const fheroes2::Point & mousePos = le.getMouseCursorPos();
                _scrollbar.moveToPos( mousePos );
                return true;
            }
            if ( _scrollbar.updatePosition() ) {
                return true;
            }

            if ( le.MouseClickLeft( _buttonLeft.area() ) || le.isMouseWheelDownInArea( _scrollbar.getArea() ) ) {
                _scrollbar.backward();
                return true;
            }
            if ( le.MouseClickLeft( _buttonRight.area() ) || le.isMouseWheelUpInArea( _scrollbar.getArea() ) ) {
                _scrollbar.forward();
                return true;
            }
            return false;
        }

    private:
        fheroes2::Scrollbar _scrollbar;
        fheroes2::Button _buttonLeft;
        fheroes2::Button _buttonRight;
    };

    class ConfigValueText final
    {
    public:
        ConfigValueText( fheroes2::Image & output, const int32_t positionX, const int32_t positionY )
            : _restorer( output, positionX, positionY, 150, 40 )
        {
            // Do nothing.
        }

        void render( fheroes2::Text & text, std::string content, fheroes2::Image & output )
        {
            const fheroes2::Rect & roi = _restorer.rect();
            _restorer.restore();
            text.set( std::move( content ), fheroes2::FontType::normalYellow() );
            text.drawInRoi( roi.x, roi.y + 2, roi.width, output, roi );
        }

    private:
        fheroes2::ImageRestorer _restorer;
    };
}

bool fheroes2::randomMapDialog( Maps::Random_Generator::Configuration & configuration, const int32_t mapWidth )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    const bool isDefaultScreenSize = display.isDefaultSize();

    fheroes2::StandardWindow background( fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT, !isDefaultScreenSize );
    const fheroes2::Rect activeArea( background.activeArea() );

    const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

    if ( isDefaultScreenSize ) {
        const fheroes2::Sprite & backgroundImage = fheroes2::AGG::GetICN( isEvilInterface ? ICN::STONEBAK_EVIL : ICN::STONEBAK, 0 );
        fheroes2::Copy( backgroundImage, 0, 0, display, activeArea );
    }

    // Dialog title.
    const fheroes2::Sprite & titleBox = fheroes2::AGG::GetICN( isEvilInterface ? ICN::METALLIC_BORDERED_TEXTBOX_EVIL : ICN::METALLIC_BORDERED_TEXTBOX_GOOD, 0 );
    const fheroes2::Rect titleBoxRoi( activeArea.x + ( activeArea.width - titleBox.width() ) / 2, activeArea.y + 10, titleBox.width(), titleBox.height() );
    const fheroes2::Rect titleTextRoi( titleBoxRoi.x + 6, titleBoxRoi.y + 5, titleBoxRoi.width - 12, titleBoxRoi.height - 11 );

    fheroes2::Copy( titleBox, 0, 0, display, titleBoxRoi );
    fheroes2::addGradientShadow( titleBox, display, titleBoxRoi.getPosition(), { -5, 5 } );

    fheroes2::Text text( _( "Random Map Generator" ), fheroes2::FontType::normalWhite() );
    text.fitToOneRow( titleTextRoi.width );
    text.drawInRoi( titleTextRoi.x, titleTextRoi.y + 3, titleTextRoi.width, display, titleTextRoi );

    const int32_t positionX = activeArea.x + 12;
    const int32_t settingDescriptionWidth = activeArea.width / 3;
    const int32_t inputPositionX = positionX + settingDescriptionWidth;
    const int32_t valuePositionX = inputPositionX + 230;
    const int32_t ySpacing = 45;
    int32_t positionY = 140;

    // Map configuration options.
    text.set( _( "Player count" ), fheroes2::FontType::normalWhite() );
    text.draw( positionX + ( settingDescriptionWidth - text.width() ) / 2, positionY, display );
    HorizontalSlider playerCountSlider{ { inputPositionX, positionY }, 2, 6, configuration.playerCount };
    ConfigValueText playerCountValue{ display, valuePositionX, positionY };
    playerCountValue.render( text, std::to_string( configuration.playerCount ), display );

    positionY += ySpacing;

    text.set( _( "Map layout" ), fheroes2::FontType::normalWhite() );
    text.draw( positionX + ( settingDescriptionWidth - text.width() ) / 2, positionY, display );

    // Dropdown with map layout selection.
    const int dropListIcn = isEvilInterface ? ICN::DROPLISL_EVIL : ICN::DROPLISL;
    const fheroes2::Sprite & itemBackground = fheroes2::AGG::GetICN( dropListIcn, 0 );
    const int32_t layoutBackgroundWidth = 200;
    const int32_t layoutBackgroundHeight = itemBackground.height();

    fheroes2::Copy( itemBackground, 0, 0, display, inputPositionX + 6, positionY - 5, layoutBackgroundWidth, layoutBackgroundHeight );
    text.set( Maps::Random_Generator::layoutToString( configuration.mapLayout ), fheroes2::FontType::normalWhite() );
    text.draw( inputPositionX + 12, positionY, display );

    const fheroes2::Sprite & dropListButtonSprite = fheroes2::AGG::GetICN( dropListIcn, 1 );
    const fheroes2::Sprite & dropListButtonPressedSprite = fheroes2::AGG::GetICN( dropListIcn, 2 );

    fheroes2::ButtonSprite layoutDroplistButton( inputPositionX + layoutBackgroundWidth + 6, positionY - 5, dropListButtonSprite, dropListButtonPressedSprite );
    layoutDroplistButton.disable();
    layoutDroplistButton.draw();

    positionY += ySpacing;

    text.set( _( "Water percentage" ), fheroes2::FontType::normalWhite() );
    text.draw( positionX + ( settingDescriptionWidth - text.width() ) / 2, positionY, display );

    const int32_t waterLimit = Maps::Random_Generator::calculateMaximumWaterPercentage( configuration.playerCount, mapWidth );
    const int32_t waterPercentage = std::min( configuration.waterPercentage, waterLimit );

    HorizontalSlider waterSlider{ { inputPositionX, positionY }, 0, 100, waterPercentage };
    ConfigValueText waterValue{ display, valuePositionX, positionY };
    waterValue.render( text, std::to_string( configuration.waterPercentage ), display );

    positionY += ySpacing;

    text.set( _( "Monster strength" ), fheroes2::FontType::normalWhite() );
    text.draw( positionX + ( settingDescriptionWidth - text.width() ) / 2, positionY, display );
    HorizontalSlider monsterSlider{ { inputPositionX, positionY }, 0, 3, static_cast<int>( configuration.monsterStrength ) };
    ConfigValueText monsterValue{ display, valuePositionX, positionY };
    monsterValue.render( text, Maps::Random_Generator::monsterStrengthToString( configuration.monsterStrength ), display );

    positionY += ySpacing;

    text.set( _( "Resource availability" ), fheroes2::FontType::normalWhite() );
    text.draw( positionX + ( settingDescriptionWidth - text.width() ) / 2, positionY, display );
    HorizontalSlider resourceSlider{ { inputPositionX, positionY }, 0, 2, static_cast<int>( configuration.resourceDensity ) };
    ConfigValueText resourceValue{ display, valuePositionX, positionY };
    resourceValue.render( text, Maps::Random_Generator::resourceDensityToString( configuration.resourceDensity ), display );

    positionY += ySpacing + 10;

    text.set( _( "Map seed" ), fheroes2::FontType::normalWhite() );
    text.draw( positionX + ( settingDescriptionWidth - text.width() ) / 2, positionY, display );

    fheroes2::ValueSelectionDialogElement mapSeedSelection{ 0, 999999, configuration.seed, 1, { positionX + settingDescriptionWidth + 4, positionY - 5 } };
    mapSeedSelection.draw( display );

    fheroes2::Button buttonCancel;
    const int buttonCancelIcn = isEvilInterface ? ICN::BUTTON_SMALL_CANCEL_EVIL : ICN::BUTTON_SMALL_CANCEL_GOOD;
    background.renderButton( buttonCancel, buttonCancelIcn, 0, 1, { 30, 10 }, fheroes2::StandardWindow::Padding::BOTTOM_RIGHT );

    fheroes2::Button buttonOk;
    const int buttonOkIcn = isEvilInterface ? ICN::BUTTON_SMALL_OKAY_EVIL : ICN::BUTTON_SMALL_OKAY_GOOD;
    background.renderButton( buttonOk, buttonOkIcn, 0, 1, { 30, 10 }, fheroes2::StandardWindow::Padding::BOTTOM_LEFT );

    LocalEvent & le = LocalEvent::Get();

    display.render( background.totalArea() );

    while ( le.HandleEvents() ) {
        buttonOk.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonOk.area() ) );
        buttonCancel.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonCancel.area() ) );

        if ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) || le.MouseClickLeft( buttonCancel.area() ) ) {
            return false;
        }

        if ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) || le.MouseClickLeft( buttonOk.area() ) ) {
            break;
        }

        if ( playerCountSlider.processEvent( le ) ) {
            configuration.playerCount = playerCountSlider.getCurrentValue();

            const int32_t newLimit = Maps::Random_Generator::calculateMaximumWaterPercentage( configuration.playerCount, mapWidth );
            configuration.waterPercentage = std::min( configuration.waterPercentage, newLimit );
            waterSlider.setRange( 0, newLimit );

            playerCountSlider.redraw( display );
            playerCountValue.render( text, std::to_string( configuration.playerCount ), display );
            waterValue.render( text, std::to_string( configuration.waterPercentage ), display );
            display.render( background.totalArea() );
        }
        else if ( waterSlider.processEvent( le ) ) {
            configuration.waterPercentage = waterSlider.getCurrentValue();
            waterSlider.redraw( display );
            waterValue.render( text, std::to_string( configuration.waterPercentage ), display );
            display.render( background.totalArea() );
        }
        else if ( monsterSlider.processEvent( le ) ) {
            configuration.monsterStrength = static_cast<Maps::Random_Generator::MonsterStrength>( monsterSlider.getCurrentValue() );
            monsterSlider.redraw( display );
            monsterValue.render( text, Maps::Random_Generator::monsterStrengthToString( configuration.monsterStrength ), display );
            display.render( background.totalArea() );
        }
        else if ( resourceSlider.processEvent( le ) ) {
            configuration.resourceDensity = static_cast<Maps::Random_Generator::ResourceDensity>( resourceSlider.getCurrentValue() );
            resourceSlider.redraw( display );
            resourceValue.render( text, Maps::Random_Generator::resourceDensityToString( configuration.resourceDensity ), display );
            display.render( background.totalArea() );
        }
        else if ( mapSeedSelection.processEvents() ) {
            mapSeedSelection.draw( display );

            configuration.seed = mapSeedSelection.getValue();
            display.render( background.totalArea() );
        }
    }

    return true;
}
