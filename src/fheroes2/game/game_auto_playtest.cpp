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

#include "game_auto_playtest.h"

#include <cassert>
#include <cstddef>
#include <map>
#include <memory>
#include <string>

#include "audio.h"
#include "audio_manager.h"
#include "color.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "game_assets.h"
#include "game_delays.h"
#include "game_hotkeys.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "maps_fileinfo.h"
#include "math_base.h"
#include "mus.h"
#include "pal.h"
#include "players.h"
#include "screen.h"
#include "settings.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_slider.h"
#include "ui_text.h"
#include "ui_tool.h"
#include "ui_window.h"
#include "world.h"

#if defined( WITH_DEBUG )
#include <ostream>

#include "logging.h"
#endif

namespace
{
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

    void displayResults( const fheroes2::AutoPlaytest & playtest )
    {
        if ( playtest.getResults().empty() ) {
            // Nothing to display.
            return;
        }

        // Process only playthroughs that ended before time limit.
        int32_t playthroughByTimeLimit{ 0 };
        std::map<PlayerColor, int32_t> wins;

        for ( const auto & result : playtest.getResults() ) {
            // Check that everyone either lost of won.
            bool isTimeLimitedPlaythrough{ false };
            for ( const auto & info : result ) {
                if ( info.state == fheroes2::AutoPlaytest::PlayerState::TIME_LIMIT ) {
                    isTimeLimitedPlaythrough = true;
                    break;
                }
            }

            if ( isTimeLimitedPlaythrough ) {
                ++playthroughByTimeLimit;
                continue;
            }

            for ( const auto & info : result ) {
                if ( info.state == fheroes2::AutoPlaytest::PlayerState::WINNER ) {
                    ++wins[info.color];
                }
                else {
                    // If this assertion blows up then our logic is not valid.
                    assert( info.state == fheroes2::AutoPlaytest::PlayerState::LOSER );
                }
            }
        }

        // Display the results.
        constexpr int32_t playerStepX{ 80 };
        const int32_t playthroughCount{ static_cast<int32_t>( playtest.getResults().size() ) };
        const int32_t playerCount{ static_cast<int32_t>( playtest.getResults().front().size() ) };

        const CursorRestorer cursorRestorer( true, Cursor::POINTER );
        fheroes2::Display & display = fheroes2::Display::instance();

        fheroes2::StandardWindow window( 500, 220, true, display );
        const fheroes2::Rect activeArea( window.activeArea() );

        const Settings & conf = Settings::Get();
        const bool isEvilInterface = conf.isEvilInterfaceEnabled();

        const fheroes2::Sprite & titleBox = Assets::getImage( isEvilInterface ? ICN::METALLIC_BORDERED_TEXTBOX_EVIL : ICN::METALLIC_BORDERED_TEXTBOX_GOOD, 0 );
        const fheroes2::Rect titleBoxRoi{ activeArea.x + ( activeArea.width - titleBox.width() ) / 2, activeArea.y + 10, titleBox.width(), titleBox.height() };
        const fheroes2::Rect titleTextRoi{ titleBoxRoi.x + 6, titleBoxRoi.y + 5, titleBoxRoi.width - 12, titleBoxRoi.height - 11 };

        Copy( titleBox, 0, 0, display, titleBoxRoi );
        addGradientShadow( titleBox, display, titleBoxRoi.getPosition(), { -5, 5 } );

        fheroes2::Text text( _( "Auto Playtest Results" ), fheroes2::FontType::normalWhite() );
        text.fitToOneRow( titleTextRoi.width );
        text.drawInRoi( titleTextRoi.x, titleTextRoi.y + 3, titleTextRoi.width, display, titleTextRoi );

        // Render players.
        const int32_t offsetX = activeArea.x + ( activeArea.width - playerCount * playerStepX + 18 ) / 2;
        int32_t offsetY = titleBoxRoi.y + titleBoxRoi.height + 10;

        std::vector<fheroes2::Rect> playerRects( playerCount );
        PlayerColorsSet playerColorSet{ 0 };
        for ( const auto & info : playtest.getResults().front() ) {
            playerColorSet |= info.color;
        }

        const PlayerColorsVector availableColors{ playerColorSet };

        const fheroes2::Sprite & playerIconShadow = Assets::getImage( ICN::NGEXTRA, 61 );
        for ( int32_t i = 0; i < playerCount; ++i ) {
            playerRects[i].x = offsetX + i * playerStepX;
            playerRects[i].y = offsetY;

            fheroes2::Blit( playerIconShadow, display, playerRects[i].x - 5, playerRects[i].y + 3 );

            const uint32_t icnIndex = Color::GetIndex( availableColors[i] ) + 3;

            const fheroes2::Sprite & playerIcon = Assets::getImage( ICN::NGEXTRA, icnIndex );
            playerRects[i].width = playerIcon.width();
            playerRects[i].height = playerIcon.height();
            fheroes2::Copy( playerIcon, 0, 0, display, playerRects[i].x, playerRects[i].y, playerRects[i].width, playerRects[i].height );
        }

        offsetY += playerRects[0].height + 10;

        for ( int32_t i = 0; i < playerCount; ++i ) {
            text.set( std::to_string( wins[availableColors[i]] * 100 / playthroughCount ) + "%", fheroes2::FontType::normalYellow() );
            text.fitToOneRow( playerRects[i].width );
            text.draw( playerRects[i].x, offsetY, playerRects[i].width, display );
        }

        offsetY += 20;

        std::string playthroughString = _n( "1 playthrough", "%{count} playthroughs", playthroughCount );
        StringReplace( playthroughString, "%{count}", playthroughCount );

        text.set( std::move( playthroughString ), fheroes2::FontType::normalWhite() );
        text.draw( activeArea.x, offsetY, activeArea.width, display );

        offsetY += 20;
        text.set( std::to_string( playthroughByTimeLimit ) + _( " playthrough(s) reached the specified time limit" ), fheroes2::FontType::normalWhite() );
        text.draw( activeArea.x, offsetY, activeArea.width, display );

        fheroes2::Button buttonOk;
        const int buttonOkIcn = isEvilInterface ? ICN::BUTTON_SMALL_OKAY_EVIL : ICN::BUTTON_SMALL_OKAY_GOOD;
        window.renderButton( buttonOk, buttonOkIcn, 0, 1, { 6, 6 }, fheroes2::StandardWindow::Padding::BOTTOM_CENTER );

        LocalEvent & le = LocalEvent::Get();

        display.render( window.totalArea() );

        AudioManager::PlayMusic( MUS::VICTORY, Music::PlaybackMode::REWIND_AND_PLAY_INFINITE );

        while ( le.HandleEvents() ) {
            buttonOk.drawOnState( le.isMouseLeftButtonPressedAndHeldInArea( buttonOk.area() ) );

            if ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) || le.MouseClickLeft( buttonOk.area() ) ) {
                break;
            }

            if ( le.isMouseRightButtonPressedInArea( buttonOk.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Okay" ), _( "Click to close the dialog." ), Dialog::ZERO );
            }

            for ( size_t i = 0; i < availableColors.size(); ++i ) {
                if ( le.isMouseRightButtonPressedInArea( playerRects[i] ) ) {
                    std::string playerString{ _( "Won %{percent}% of playthroughs." ) };
                    StringReplace( playerString, "%{percent}", wins[availableColors[i]] * 100 / playthroughCount );
                    fheroes2::showStandardTextMessage( Color::String( availableColors[i] ), std::move( playerString ), Dialog::ZERO );
                }
            }
        }

        AudioManager::ResetAudio();
    }

    void runPlayTest()
    {
        Settings & conf = Settings::Get();
        auto & autoPlaytest = fheroes2::AutoPlaytest::instance();
        autoPlaytest.reset( conf.GetPlayers().GetColors() );

        const int32_t currentAISpeed{ conf.AIMoveSpeed() };

        if ( autoPlaytest.isAnimationEnabled() ) {
            conf.SetAIMoveSpeed( autoPlaytest.getAnimationSpeed() );
        }
        else {
            conf.SetAIMoveSpeed( 0 );
        }

        Game::UpdateGameSpeed();

        const fheroes2::GameInterfaceTypeRestorer interfaceRestorer{ conf.isEvilInterfaceEnabled() ? InterfaceType::EVIL : InterfaceType::GOOD };

        for ( int32_t playthroughId = 0; playthroughId < autoPlaytest.getMaxPlaythroughs(); ++playthroughId ) {
            if ( !prepareMap() ) {
                fheroes2::showStandardTextMessage( _( "Warning" ), _( "Failed to prepare the map for auto playtest." ), Dialog::ZERO );
                return;
            }

            conf.SetGameType( Game::TYPE_AUTO_PLAYTEST );

            Game::StartGame();

#if defined( WITH_DEBUG )
            VERBOSE_LOG( "----- Playthrough " << autoPlaytest.getResults().size() << " -----" )
            for ( const auto & info : autoPlaytest.getResults().back() ) {
                switch ( info.state ) {
                case fheroes2::AutoPlaytest::PlayerState::WINNER:
                    VERBOSE_LOG( "Player " << Color::String( info.color ) << " won" )
                    break;
                case fheroes2::AutoPlaytest::PlayerState::LOSER:
                    VERBOSE_LOG( "Player " << Color::String( info.color ) << " lost on " << info.dayOfState << " day" )
                    break;
                case fheroes2::AutoPlaytest::PlayerState::TIME_LIMIT:
                    VERBOSE_LOG( "Player " << Color::String( info.color ) << " hit time limit on " << info.dayOfState << " day" )
                    break;
                case fheroes2::AutoPlaytest::PlayerState::INTERRUPTED:
                    VERBOSE_LOG( "Player " << Color::String( info.color ) << " interrupted on " << info.dayOfState << " day" )
                    break;
                default:
                    assert( 0 );
                    break;
                }
            }
#endif

            // Verify whether the current playthrough ended properly or was interrupted by a user.
            bool isInterrupted{ false };
            for ( const auto & info : autoPlaytest.getResults().back() ) {
                if ( info.state == fheroes2::AutoPlaytest::PlayerState::INTERRUPTED ) {
                    isInterrupted = true;
                    break;
                }
            }

            if ( isInterrupted ) {
                break;
            }

            autoPlaytest.nextPlaythrough();
        }

        autoPlaytest.popLastResults();

        // Make sure to reset music and audio as the playtest could be interrupted.
        AudioManager::ResetAudio();

        displayResults( autoPlaytest );

        // Restore the original AI speed.
        conf.SetAIMoveSpeed( currentAISpeed );
        Game::UpdateGameSpeed();
    }

    std::string getValueString( const int32_t value, const int32_t limit )
    {
        return std::to_string( value ) + '/' + std::to_string( limit );
    }

    fheroes2::Rect renderCheckbox( const int32_t offsetX, const int32_t offsetY, const bool isEnabled, fheroes2::Image & output, const bool isEvilInterface )
    {
        const fheroes2::Sprite & cell = Assets::getImage( ICN::CELLWIN, 1 );

        fheroes2::Blit( cell, output, offsetX, offsetY );
        if ( isEnabled ) {
            const fheroes2::Sprite & mark = Assets::getImage( ICN::CELLWIN, 2 );
            fheroes2::Blit( mark, output, offsetX + mark.x(), offsetY + mark.y() );
        }

        if ( isEvilInterface ) {
            fheroes2::ApplyPalette( output, offsetX, offsetY, output, offsetX, offsetY, cell.width(), cell.height(),
                                    PAL::GetPalette( PAL::PaletteType::GOOD_TO_EVIL_INTERFACE ) );
        }

        return { offsetX, offsetY, cell.width(), cell.height() };
    }
}

namespace fheroes2
{
    bool openMapAutoPlayTest()
    {
        Display & display = Display::instance();

        StandardWindow window( 550, 345, true, display );
        const Rect activeArea( window.activeArea() );

        const Settings & conf = Settings::Get();
        const bool isEvilInterface = conf.isEvilInterfaceEnabled();

        const Sprite & titleBox = Assets::getImage( isEvilInterface ? ICN::METALLIC_BORDERED_TEXTBOX_EVIL : ICN::METALLIC_BORDERED_TEXTBOX_GOOD, 0 );
        const Rect titleBoxRoi( activeArea.x + ( activeArea.width - titleBox.width() ) / 2, activeArea.y + 10, titleBox.width(), titleBox.height() );
        const Rect titleTextRoi( titleBoxRoi.x + 6, titleBoxRoi.y + 5, titleBoxRoi.width - 12, titleBoxRoi.height - 11 );

        Copy( titleBox, 0, 0, display, titleBoxRoi );
        addGradientShadow( titleBox, display, titleBoxRoi.getPosition(), { -5, 5 } );

        Text text( _( "Auto Playtest" ), FontType::normalWhite() );
        text.fitToOneRow( titleTextRoi.width );
        text.drawInRoi( titleTextRoi.x, titleTextRoi.y + 3, titleTextRoi.width, display, titleTextRoi );

        auto & autoPlaytest = AutoPlaytest::instance();

        constexpr int32_t optionTextMaxWidth{ 250 };
        constexpr int32_t optionTitleOffsetX{ 10 };
        const int32_t positionX = activeArea.x + 10;
        const int32_t inputPositionX = positionX + optionTextMaxWidth;
        const int32_t valuePositionX = inputPositionX + sliderWidth + 57;
        constexpr int32_t ySpacing{ 45 };
        int32_t positionY = activeArea.y + 70;

        text.set( _( "autoPlaytest|Number of playthroughs:" ), FontType::normalWhite() );
        text.fitToOneRow( optionTextMaxWidth );
        text.draw( positionX + optionTextMaxWidth - text.width() - optionTitleOffsetX, positionY + 1, display );
        HorizontalSlider playthroughCountSlider{ sliderWidth, { inputPositionX, positionY }, 1, AutoPlaytest::playthroughLimit, autoPlaytest.getMaxPlaythroughs() };
        TextRestorer playthroughCountValue{ display, { valuePositionX, positionY + 2 } };
        playthroughCountValue.render( getValueString( autoPlaytest.getMaxPlaythroughs(), AutoPlaytest::playthroughLimit ) );

        positionY += ySpacing;

        text.set( _( "autoPlaytest|Max days per playthrough:" ), FontType::normalWhite() );
        text.fitToOneRow( optionTextMaxWidth );
        text.draw( positionX + optionTextMaxWidth - text.width() - optionTitleOffsetX, positionY + 1, display );
        HorizontalSlider dayCountSlider{ sliderWidth, { inputPositionX, positionY }, 1, AutoPlaytest::dayLimit, autoPlaytest.getMaxDaysInPlaythrough() };
        TextRestorer dayCountValue{ display, { valuePositionX, positionY + 2 } };
        dayCountValue.render( getValueString( autoPlaytest.getMaxDaysInPlaythrough(), AutoPlaytest::dayLimit ) );

        positionY += ySpacing;

        const Rect animationCheckboxArea{ renderCheckbox( inputPositionX + 3, positionY, autoPlaytest.isAnimationEnabled(), display, isEvilInterface ) };

        text.set( _( "autoPlaytest|Animation" ), FontType::normalWhite() );
        text.draw( animationCheckboxArea.x + animationCheckboxArea.width + 5, animationCheckboxArea.y + 2, display );

        positionY += 30;

        text.set( _( "autoPlaytest|Animation speed:" ), autoPlaytest.isAnimationEnabled() ? FontType::normalWhite() : FontType{ FontSize::NORMAL, FontColor::GRAY } );
        text.fitToOneRow( optionTextMaxWidth );

        const Point animationTextOffset{ positionX + optionTextMaxWidth - text.width() - optionTitleOffsetX, positionY + 1 };

        auto animationTextAreaRestorer = std::make_unique<ImageRestorer>( display, animationTextOffset.x, animationTextOffset.y, text.width(), text.height() );
        text.draw( animationTextOffset.x, animationTextOffset.y, display );
        HorizontalSlider speedCountSlider{ sliderWidth, { inputPositionX, positionY }, 1, AutoPlaytest::animationLimit, autoPlaytest.getAnimationSpeed() };
        TextRestorer speedCountValue{ display, { valuePositionX, positionY + 2 } };
        speedCountValue.render( getValueString( autoPlaytest.getAnimationSpeed(), AutoPlaytest::animationLimit ) );
        if ( !autoPlaytest.isAnimationEnabled() ) {
            speedCountSlider.disable();
        }

        positionY += 30;

        const Rect soundsCheckboxArea{ renderCheckbox( inputPositionX + 3, positionY, autoPlaytest.areEnvironmentSoundsEnabled(), display, isEvilInterface ) };

        text.set( _( "autoPlaytest|Sound Effects" ), FontType::normalWhite() );
        text.draw( soundsCheckboxArea.x + soundsCheckboxArea.width + 5, soundsCheckboxArea.y + 2, display );

        positionY += ySpacing;
        text.set( _( "Left-clicking at any point will interrupt the playtest." ), FontType::normalYellow() );
        text.draw( positionX, positionY, activeArea.width, display );

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

            if ( playthroughCountSlider.processEvents( eventHandler ) ) {
                autoPlaytest.setMaxPlaythroughs( playthroughCountSlider.getCurrentValue() );
                playthroughCountValue.render( getValueString( playthroughCountSlider.getCurrentValue(), AutoPlaytest::playthroughLimit ) );
                display.render( window.activeArea() );
            }
            else if ( dayCountSlider.processEvents( eventHandler ) ) {
                autoPlaytest.setMaxDaysInPlaythrough( dayCountSlider.getCurrentValue() );
                dayCountValue.render( getValueString( dayCountSlider.getCurrentValue(), AutoPlaytest::dayLimit ) );
                display.render( window.activeArea() );
            }
            else if ( autoPlaytest.isAnimationEnabled() && speedCountSlider.processEvents( eventHandler ) ) {
                autoPlaytest.setAnimationSpeed( speedCountSlider.getCurrentValue() );
                speedCountValue.render( getValueString( speedCountSlider.getCurrentValue(), AutoPlaytest::animationLimit ) );
                display.render( window.activeArea() );
            }
            else if ( eventHandler.MouseClickLeft( animationCheckboxArea ) ) {
                if ( autoPlaytest.isAnimationEnabled() ) {
                    speedCountSlider.disable();
                }
                else {
                    speedCountSlider.enable();
                }

                autoPlaytest.enableAnimation( !autoPlaytest.isAnimationEnabled() );

                animationTextAreaRestorer->restore();
                text.set( _( "autoPlaytest|Animation speed:" ),
                          autoPlaytest.isAnimationEnabled() ? FontType::normalWhite() : FontType{ FontSize::NORMAL, FontColor::GRAY } );
                text.fitToOneRow( optionTextMaxWidth );
                text.draw( animationTextOffset.x, animationTextOffset.y, display );

                renderCheckbox( animationCheckboxArea.x, animationCheckboxArea.y, autoPlaytest.isAnimationEnabled(), display, isEvilInterface );
                display.render( window.activeArea() );
            }
            else if ( eventHandler.MouseClickLeft( soundsCheckboxArea ) ) {
                autoPlaytest.enableSounds( !autoPlaytest.areEnvironmentSoundsEnabled() );

                renderCheckbox( soundsCheckboxArea.x, soundsCheckboxArea.y, autoPlaytest.areEnvironmentSoundsEnabled(), display, isEvilInterface );
                display.render( soundsCheckboxArea );
            }

            if ( eventHandler.isMouseRightButtonPressedInArea( buttonOk.area() ) ) {
                showStandardTextMessage( _( "Okay" ), _( "Click to run an automated map playtest." ), Dialog::ZERO );
            }
            else if ( eventHandler.isMouseRightButtonPressedInArea( buttonCancel.area() ) ) {
                showStandardTextMessage( _( "Cancel" ), _( "Return to the previous menu." ), Dialog::ZERO );
            }
        }

        return false;
    }

    AutoPlaytest & AutoPlaytest::instance()
    {
        static AutoPlaytest playtest;
        return playtest;
    }

    void interruptAutoPlaytest()
    {
        Settings & conf = Settings::Get();
        assert( conf.IsGameType( Game::TYPE_AUTO_PLAYTEST ) );

        Player * currentPlayer = conf.GetPlayers().GetCurrent();
        if ( currentPlayer == nullptr ) {
            // Should we assert it?
            return;
        }

        if ( !currentPlayer->isAIAutoControlMode() ) {
            return;
        }

        auto & autoPlaytest = AutoPlaytest::instance();
        // Check if the game has been interrupted.
        if ( autoPlaytest.isInterrupted() ) {
            // Nothing we need to do. Just wait.
            return;
        }

        std::string title{ _( "Auto playtest\n(playthrough %{current} of %{total})" ) };
        StringReplace( title, "%{current}", autoPlaytest.getResults().size() );
        StringReplace( title, "%{total}", autoPlaytest.getMaxPlaythroughs() );

        // We need to reset left mouse button state to avoid mis-clicking on the dialog's buttons.
        LocalEvent & eventHandler = LocalEvent::Get();
        while ( eventHandler.HandleEvents() ) {
            if ( !eventHandler.isMouseLeftButtonPressed() ) {
                break;
            }
        }

        if ( showStandardTextMessage( std::move( title ), _( "Do you want to interrupt the automatic playtest? The effect will take place only on the next turn." ),
                                      Dialog::YES | Dialog::NO )
             == Dialog::NO ) {
            return;
        }

        // Switch off AI auto control mode for all players.
        currentPlayer->setAIAutoControlMode( false );

        autoPlaytest.interrupt( world.CountDay() );
    }
}
