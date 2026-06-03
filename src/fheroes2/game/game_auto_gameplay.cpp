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
#include <map>
#include <ostream>
#include <string>

#include "agg_image.h"
#include "color.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "game_hotkeys.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "logging.h"
#include "maps_fileinfo.h"
#include "math_base.h"
#include "players.h"
#include "screen.h"
#include "settings.h"
#include "statusbar.h"
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

    class TextRestorer final : public fheroes2::MovableText
    {
    public:
        TextRestorer( fheroes2::Image & output, const fheroes2::Point offset )
            : fheroes2::MovableText( output )
            , _offset( offset )
        {
            // Do nothing.
        }

        void render( std::string content )
        {
            update( std::make_unique<fheroes2::Text>( std::move( content ), fheroes2::FontType::normalYellow() ) );
            draw( _offset.x, _offset.y );
        }

    private:
        const fheroes2::Point _offset;
    };

    bool prepareMap()
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

    void displayResults( const fheroes2::AutoGameplay & gameplay )
    {
        if ( gameplay.getResults().empty() ) {
            // Nothing to display.
            return;
        }

        // Process only rounds that ended before time limit.
        int32_t roundByTimeLimit{ 0 };
        std::map<PlayerColor, int32_t> wins;

        for ( const auto & result : gameplay.getResults() ) {
            // Check that everyone either lost of won.
            bool isTimeLimitRound{ false };
            for ( const auto & info : result ) {
                if ( info.state == fheroes2::AutoGameplay::PlayerState::TIME_LIMIT ) {
                    isTimeLimitRound = true;
                    break;
                }
            }

            if ( isTimeLimitRound ) {
                ++roundByTimeLimit;
                continue;
            }

            for ( const auto & info : result ) {
                if ( info.state == fheroes2::AutoGameplay::PlayerState::WINNER ) {
                    ++wins[info.color];
                }
                else {
                    // If this assertion blows up then our logic is not valid.
                    assert( info.state == fheroes2::AutoGameplay::PlayerState::LOSER );
                }
            }
        }

        // Display the results.
        constexpr int32_t playerStepX{ 80 };
        const int32_t roundCount{ static_cast<int32_t>( gameplay.getResults().size() ) };
        const int32_t playerCount{ static_cast<int32_t>( gameplay.getResults().front().size() ) };

        const CursorRestorer cursorRestorer( true, Cursor::POINTER );
        fheroes2::Display & display = fheroes2::Display::instance();

        fheroes2::StandardWindow window( 500, 210, true, display );
        const fheroes2::Rect activeArea( window.activeArea() );

        const Settings & conf = Settings::Get();
        const bool isEvilInterface = conf.isEvilInterfaceEnabled();

        const fheroes2::Sprite & titleBox = fheroes2::AGG::GetICN( isEvilInterface ? ICN::METALLIC_BORDERED_TEXTBOX_EVIL : ICN::METALLIC_BORDERED_TEXTBOX_GOOD, 0 );
        const fheroes2::Rect titleBoxRoi( activeArea.x + ( activeArea.width - titleBox.width() ) / 2, activeArea.y + 10, titleBox.width(), titleBox.height() );
        const fheroes2::Rect titleTextRoi( titleBoxRoi.x + 6, titleBoxRoi.y + 5, titleBoxRoi.width - 12, titleBoxRoi.height - 11 );

        Copy( titleBox, 0, 0, display, titleBoxRoi );
        addGradientShadow( titleBox, display, titleBoxRoi.getPosition(), { -5, 5 } );

        fheroes2::Text text( _( "Results" ), fheroes2::FontType::normalWhite() );
        text.fitToOneRow( titleTextRoi.width );
        text.drawInRoi( titleTextRoi.x, titleTextRoi.y + 3, titleTextRoi.width, display, titleTextRoi );

        // Render players.
        const int32_t offsetX = activeArea.x + ( activeArea.width - playerCount * playerStepX ) / 2;
        int32_t offsetY = titleBoxRoi.y + titleBoxRoi.height + 10;

        std::vector<fheroes2::Rect> playerRects( playerCount );
        PlayerColorsSet playerColorSet{ 0 };
        for ( const auto & info : gameplay.getResults().front() ) {
            playerColorSet |= info.color;
        }

        const PlayerColorsVector availableColors{ playerColorSet };

        const fheroes2::Sprite & playerIconShadow = fheroes2::AGG::GetICN( ICN::NGEXTRA, 61 );
        for ( int32_t i = 0; i < playerCount; ++i ) {
            playerRects[i].x = offsetX + i * playerStepX;
            playerRects[i].y = offsetY;

            fheroes2::Blit( playerIconShadow, display, playerRects[i].x - 5, playerRects[i].y + 3 );

            const uint32_t icnIndex = Color::GetIndex( availableColors[i] ) + 3;

            const fheroes2::Sprite & playerIcon = fheroes2::AGG::GetICN( ICN::NGEXTRA, icnIndex );
            playerRects[i].width = playerIcon.width();
            playerRects[i].height = playerIcon.height();
            fheroes2::Copy( playerIcon, 0, 0, display, playerRects[i].x, playerRects[i].y, playerRects[i].width, playerRects[i].height );
        }

        offsetY += playerRects[0].height + 10;

        for ( int32_t i = 0; i < playerCount; ++i ) {
            text.set( std::to_string( wins[availableColors[i]] * 100 / roundCount ) + "%", fheroes2::FontType::normalYellow() );
            text.fitToOneRow( playerRects[i].width );
            text.draw( playerRects[i].x, offsetY, playerRects[i].width, display );
        }

        offsetY += 20;
        text.set( std::to_string( roundCount ) + _( " rounds" ), fheroes2::FontType::normalWhite() );
        text.fitToOneRow( titleTextRoi.width );
        text.draw( titleTextRoi.x, offsetY, titleTextRoi.width, display );

        offsetY += 20;
        text.set( std::to_string( roundByTimeLimit ) + _( " rounds reached time limit." ), fheroes2::FontType::normalWhite() );
        text.fitToOneRow( titleTextRoi.width );
        text.draw( titleTextRoi.x, offsetY, titleTextRoi.width, display );

        fheroes2::Button buttonOk;
        const int buttonOkIcn = isEvilInterface ? ICN::BUTTON_SMALL_OKAY_EVIL : ICN::BUTTON_SMALL_OKAY_GOOD;
        window.renderButton( buttonOk, buttonOkIcn, 0, 1, { 6, 6 }, fheroes2::StandardWindow::Padding::BOTTOM_CENTER );

        LocalEvent & le = LocalEvent::Get();

        display.render( window.totalArea() );

        while ( le.HandleEvents() ) {
            buttonOk.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonOk.area() ) );

            if ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) || le.MouseClickLeft( buttonOk.area() ) ) {
                break;
            }
        }
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
            if ( !prepareMap() ) {
                fheroes2::showStandardTextMessage( _( "Warning" ), _( "Failed to prepare the map for auto gameplay." ), Dialog::ZERO );
                return;
            }

            conf.SetGameType( Game::TYPE_AUTO_GAMEPLAY );

            Game::StartGame();

            for ( const auto & infos : autoGameplay.getResults() ) {
                VERBOSE_LOG( "----- Round " << roundId + 1 << " -----" )
                for ( const auto & info : infos ) {
                    switch ( info.state ) {
                    case fheroes2::AutoGameplay::PlayerState::WINNER:
                        VERBOSE_LOG( "Player " << Color::String( info.color ) << " won" )
                        break;
                    case fheroes2::AutoGameplay::PlayerState::LOSER:
                        VERBOSE_LOG( "Player " << Color::String( info.color ) << " lost on " << info.dayOfState << " day" )
                        break;
                    case fheroes2::AutoGameplay::PlayerState::TIME_LIMIT:
                        VERBOSE_LOG( "Player " << Color::String( info.color ) << " hit time limit on " << info.dayOfState << " day" )
                        break;
                    case fheroes2::AutoGameplay::PlayerState::INTERRUPTED:
                        VERBOSE_LOG( "Player " << Color::String( info.color ) << " interrupted on " << info.dayOfState << " day" )
                        break;
                    default:
                        assert( 0 );
                        break;
                    }
                }
            }

            // Verify whether the current round ended properly or was interrupted by a user.
            bool isInterrupted{ false };
            for ( const auto & info : autoGameplay.getResults().back() ) {
                if ( info.state == fheroes2::AutoGameplay::PlayerState::INTERRUPTED ) {
                    isInterrupted = true;
                    break;
                }
            }

            if ( isInterrupted ) {
                break;
            }

            autoGameplay.nextRound();
        }

        autoGameplay.popLastResults();

        displayResults( autoGameplay );

        // Restore the original AI speed.
        conf.SetAIMoveSpeed( currentAISpeed );
    }

    std::string getValueString( const int32_t value, const int32_t limit )
    {
        return std::to_string( value ) + '/' + std::to_string( limit );
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

        text.set( _( "auto|Number of rounds:" ), FontType::normalWhite() );
        text.draw( positionX + ( optionTextMaxWidth - text.width() ) / 2, positionY, display );
        fheroes2::HorizontalSlider roundCountSlider{ sliderWidth, { inputPositionX, positionY }, 1, roundLimit, autoGameplay.getMaxRounds() };
        TextRestorer roundCountValue{ display, { valuePositionX, positionY } };
        roundCountValue.render( getValueString( autoGameplay.getMaxRounds(), roundLimit ) );

        positionY += ySpacing;

        text.set( _( "auto|Max days per game:" ), FontType::normalWhite() );
        text.draw( positionX + ( optionTextMaxWidth - text.width() ) / 2, positionY, display );
        fheroes2::HorizontalSlider dayCountSlider{ sliderWidth, { inputPositionX, positionY }, 1, dayLimit, autoGameplay.getMaxDaysInGameplay() };
        TextRestorer dayCountValue{ display, { valuePositionX, positionY } };
        dayCountValue.render( getValueString( autoGameplay.getMaxDaysInGameplay(), dayLimit ) );

        positionY += ySpacing;

        text.set( _( "auto|Animation speed:" ), FontType::normalWhite() );
        text.draw( positionX + ( optionTextMaxWidth - text.width() ) / 2, positionY, display );
        fheroes2::HorizontalSlider speedCountSlider{ sliderWidth, { inputPositionX, positionY }, 1, speedLimit, autoGameplay.getMovementSpeed() };
        TextRestorer speedCountValue{ display, { valuePositionX, positionY } };
        speedCountValue.render( getValueString( autoGameplay.getMovementSpeed(), speedLimit ) );

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
                roundCountValue.render( getValueString( roundCountSlider.getCurrentValue(), roundLimit ) );
                display.render( window.activeArea() );
            }
            else if ( dayCountSlider.processEvents( eventHandler ) ) {
                autoGameplay.setMaxDaysInGameplay( dayCountSlider.getCurrentValue() );
                dayCountValue.render( getValueString( dayCountSlider.getCurrentValue(), dayLimit ) );
                display.render( window.activeArea() );
            }
            else if ( speedCountSlider.processEvents( eventHandler ) ) {
                autoGameplay.setMovementSpeed( speedCountSlider.getCurrentValue() );
                speedCountValue.render( getValueString( speedCountSlider.getCurrentValue(), speedLimit ) );
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

        const auto & autoGameplay = AutoGameplay::instance();

        std::string title{ _( "Auto gameplay\n(round %{currentRound} of %{totalRounds})" ) };
        StringReplace( title, "%{currentRound}", autoGameplay.getResults().size() );
        StringReplace( title, "%{totalRounds}", autoGameplay.getMaxRounds() );

        // We need to reset left mouse button state to avoid mis-clicking on the dialog's buttons.
        LocalEvent & eventHandler = LocalEvent::Get();
        while ( eventHandler.HandleEvents() ) {
            if ( !eventHandler.isMouseLeftButtonPressed() ) {
                break;
            }
        }

        if ( fheroes2::showStandardTextMessage( std::move( title ), _( "Do you want to interrupt auto gameplay? The effect will take place only on the next turn." ),
                                                Dialog::YES | Dialog::NO )
             == Dialog::NO ) {
            return;
        }

        // Switch off AI auto control mode for all players.
        currentPlayer->setAIAutoControlMode( false );
    }
}
