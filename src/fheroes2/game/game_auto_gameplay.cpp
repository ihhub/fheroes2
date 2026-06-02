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

#include "game_auto_gameplay.h"

#include <cassert>

#include "agg_image.h"
#include "dialog.h"
#include "game.h"
#include "game_hotkeys.h"
#include "icn.h"
#include "logging.h"
#include "players.h"
#include "screen.h"
#include "settings.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_slider.h"
#include "ui_text.h"
#include "ui_window.h"
#include "world.h"

namespace
{
    constexpr int32_t roundLimit{ 100 };
    constexpr int32_t dayLimit{ 1000 };
    constexpr int32_t speedLimit{ 10 };

    constexpr int32_t sliderWidth{ 150 };

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
            text.draw( roi.x, roi.y + 2, output );
        }

    private:
        fheroes2::ImageRestorer _restorer;
    };

    bool loadMap()
    {
        auto & conf = Settings::Get();
        const Maps::FileInfo & mapInfo = conf.getCurrentMapInfo();
        if ( mapInfo.version != GameVersion::RESURRECTION ) {
            // How it is even possible?!
            assert( 0 );
            return false;
        }

        auto & players = conf.GetPlayers();
        players.Init( conf.getCurrentMapInfo() );
        players.SetStartGame();

        return world.loadResurrectionMap( mapInfo.filename );
    }

    void runPlayTest()
    {
        Settings & conf = Settings::Get();
        fheroes2::AutoGameplay & autoGameplay = fheroes2::AutoGameplay::instance();
        autoGameplay.reset( conf.GetPlayers().GetColors() );

        const int32_t currentAISpeed{ conf.AIMoveSpeed() };

        if ( autoGameplay.getMovementSpeed() == 10 ) {
            conf.SetAIMoveSpeed( 0 );
        }
        else {
            conf.SetAIMoveSpeed( autoGameplay.getMovementSpeed() - 1 );
        }

        for ( int32_t roundId = 0; roundId < autoGameplay.getMaxRounds(); ++roundId ) {
            if ( !loadMap() ) {
                fheroes2::showStandardTextMessage( _( "Warning" ), _( "Failed to load the map." ), Dialog::ZERO );
                return;
            }

            conf.SetGameType( Game::TYPE_AUTO_GAMEPLAY );

            autoGameplay.reset( conf.GetPlayers().GetColors() );

            Game::StartGame();

            for ( const auto & infos : autoGameplay.getResults() ) {
                VERBOSE_LOG( "----- Round " << roundId + 1 << " -----" )
                for ( const auto & info : infos ) {
                    switch ( info.state ) {
                    case fheroes2::AutoGameplay::PlayerInfo::State::WINNER:
                        VERBOSE_LOG( "Player " << Color::String( info.color ) << " won" )
                        break;
                    case fheroes2::AutoGameplay::PlayerInfo::State::LOSER:
                        VERBOSE_LOG( "Player " << Color::String( info.color ) << " lost on " << info.dayOfState << " day" )
                        break;
                    case fheroes2::AutoGameplay::PlayerInfo::State::TIME_LIMIT:
                        VERBOSE_LOG( "Player " << Color::String( info.color ) << " hit time limit on " << info.dayOfState << " day" )
                        break;
                    case fheroes2::AutoGameplay::PlayerInfo::State::INTERRUPTED:
                        VERBOSE_LOG( "Player " << Color::String( info.color ) << " interrupted on " << info.dayOfState << " day" )
                        break;
                    default:
                        assert( 0 );
                        break;
                    }
                }
            }
        }

        // Restore the original AI speed.
        conf.SetAIMoveSpeed( currentAISpeed );
    }
}

namespace fheroes2
{
    bool openMapAutoPlayTest()
    {
        Display & display = Display::instance();

        StandardWindow window( 500, 300, true, display );
        const Rect activeArea( window.activeArea() );

        const Settings & conf = Settings::Get();
        const bool isEvilInterface = conf.isEvilInterfaceEnabled();

        const Sprite & titleBox = AGG::GetICN( isEvilInterface ? ICN::METALLIC_BORDERED_TEXTBOX_EVIL : ICN::METALLIC_BORDERED_TEXTBOX_GOOD, 0 );
        const Rect titleBoxRoi( activeArea.x + ( activeArea.width - titleBox.width() ) / 2, activeArea.y + 10, titleBox.width(), titleBox.height() );
        const Rect titleTextRoi( titleBoxRoi.x + 6, titleBoxRoi.y + 5, titleBoxRoi.width - 12, titleBoxRoi.height - 11 );

        Copy( titleBox, 0, 0, display, titleBoxRoi );
        addGradientShadow( titleBox, display, titleBoxRoi.getPosition(), { -5, 5 } );

        Text text( _( "Auto map testing" ), FontType::normalWhite() );
        text.fitToOneRow( titleTextRoi.width );
        text.drawInRoi( titleTextRoi.x, titleTextRoi.y + 3, titleTextRoi.width, display, titleTextRoi );

        fheroes2::AutoGameplay & autoGameplay = fheroes2::AutoGameplay::instance();

        constexpr int32_t optionTextMaxWidth{ 200 };
        const int32_t positionX = activeArea.x + 10;
        const int32_t inputPositionX = positionX + optionTextMaxWidth;
        const int32_t valuePositionX = inputPositionX + sliderWidth + 55;
        const int32_t ySpacing = 45;
        int32_t positionY = activeArea.y + 70;

        text.set( _( "auto|Round count:" ), FontType::normalWhite() );
        text.draw( positionX + ( optionTextMaxWidth - text.width() ) / 2, positionY, display );
        fheroes2::HorizontalSlider roundCountSlider{ sliderWidth, { inputPositionX, positionY }, 1, roundLimit, autoGameplay.getMaxRounds() };
        TextRestorer roundCountValue{ display, valuePositionX, positionY };
        roundCountValue.render( std::to_string( autoGameplay.getMaxRounds() ) + '/' + std::to_string( roundLimit ), display );

        positionY += ySpacing;

        text.set( _( "auto|Max days per game:" ), FontType::normalWhite() );
        text.draw( positionX + ( optionTextMaxWidth - text.width() ) / 2, positionY, display );
        fheroes2::HorizontalSlider dayCountSlider{ sliderWidth, { inputPositionX, positionY }, 1, dayLimit, autoGameplay.getMaxDaysInGameplay() };
        TextRestorer dayCountValue{ display, valuePositionX, positionY };
        dayCountValue.render( std::to_string( autoGameplay.getMaxDaysInGameplay() ) + '/' + std::to_string( dayLimit ), display );

        positionY += ySpacing;

        text.set( _( "auto|Animation speed:" ), FontType::normalWhite() );
        text.draw( positionX + ( optionTextMaxWidth - text.width() ) / 2, positionY, display );
        fheroes2::HorizontalSlider speedCountSlider{ sliderWidth, { inputPositionX, positionY }, 1, speedLimit, autoGameplay.getMovementSpeed() };
        TextRestorer speedCountValue{ display, valuePositionX, positionY };
        speedCountValue.render( std::to_string( autoGameplay.getMovementSpeed() ) + '/' + std::to_string( speedLimit ), display );

        Button buttonCancel;
        const int buttonCancelIcn = isEvilInterface ? ICN::BUTTON_SMALL_CANCEL_EVIL : ICN::BUTTON_SMALL_CANCEL_GOOD;
        window.renderButton( buttonCancel, buttonCancelIcn, 0, 1, { 30, 10 }, StandardWindow::Padding::BOTTOM_RIGHT );

        Button buttonOk;
        const int buttonOkIcn = isEvilInterface ? ICN::BUTTON_SMALL_OKAY_EVIL : ICN::BUTTON_SMALL_OKAY_GOOD;
        window.renderButton( buttonOk, buttonOkIcn, 0, 1, { 30, 10 }, StandardWindow::Padding::BOTTOM_LEFT );

        display.render( window.totalArea() );

        LocalEvent & eventHandler = LocalEvent::Get();
        while ( eventHandler.HandleEvents() ) {
            buttonOk.drawOnState( eventHandler.isMouseLeftButtonPressedAndHeldInArea( buttonOk.area() ) );
            buttonCancel.drawOnState( eventHandler.isMouseLeftButtonPressedAndHeldInArea( buttonCancel.area() ) );

            if ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) || eventHandler.MouseClickLeft( buttonCancel.area() ) ) {
                return false;
            }

            if ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) || eventHandler.MouseClickLeft( buttonOk.area() ) ) {
                runPlayTest();
                return true;
            }

            if ( roundCountSlider.processEvents( eventHandler ) ) {
                autoGameplay.setMaxRounds( roundCountSlider.getCurrentValue() );
                roundCountValue.render( std::to_string( roundCountSlider.getCurrentValue() ) + '/' + std::to_string( roundLimit ), display );
                display.render( window.activeArea() );
            }
            else if ( dayCountSlider.processEvents( eventHandler ) ) {
                autoGameplay.setMaxDaysInGameplay( dayCountSlider.getCurrentValue() );
                dayCountValue.render( std::to_string( dayCountSlider.getCurrentValue() ) + '/' + std::to_string( dayLimit ), display );
                display.render( window.activeArea() );
            }
            else if ( speedCountSlider.processEvents( eventHandler ) ) {
                autoGameplay.setMovementSpeed( speedCountSlider.getCurrentValue() );
                speedCountValue.render( std::to_string( speedCountSlider.getCurrentValue() ) + '/' + std::to_string( speedLimit ), display );
                display.render( window.activeArea() );
            }

            if ( eventHandler.isMouseRightButtonPressedInArea( buttonOk.area() ) ) {
                showStandardTextMessage( _( "Okay" ), _( "Click to run an automated map testing." ), Dialog::ZERO );
            }
            else if ( eventHandler.isMouseRightButtonPressedInArea( buttonCancel.area() ) ) {
                showStandardTextMessage( _( "Cancel" ), _( "Return to the previous menu." ), Dialog::ZERO );
            }
        }

        return false;
    }

    AutoGameplay & AutoGameplay::instance()
    {
        static AutoGameplay gameplay;
        return gameplay;
    }

    void interruptAutoGameplay()
    {
        Settings & conf = Settings::Get();
        assert( conf.IsGameType( Game::TYPE_AUTO_GAMEPLAY ) );

        Player * currentPlayer = conf.GetPlayers().GetCurrent();
        if ( currentPlayer == nullptr ) {
            // Should we assert it?
            return;
        }

        if ( !currentPlayer->isAIAutoControlMode() ) {
            return;
        }

        if ( fheroes2::showStandardTextMessage( _( "Auto gameplay" ), _( "Do you want to interrupt auto gameplay? The effect will take place only on the next turn." ),
                                                Dialog::YES | Dialog::NO )
             == Dialog::NO ) {
            return;
        }

        // Switch off AI auto control mode for all players.
        currentPlayer->setAIAutoControlMode( false );
    }
}
