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
#include "ui_text.h"
#include "ui_window.h"
#include "world.h"

namespace
{
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

    void runPlayTest( const uint32_t roundCount, const uint32_t daysInGameLimit )
    {
        assert( roundCount > 0 );
        assert( daysInGameLimit > 0 );

        Settings & conf = Settings::Get();
        fheroes2::AutoGameplay & autoGameplay = fheroes2::AutoGameplay::instance();
        autoGameplay.setMaxDaysInGameplay( daysInGameLimit );
        autoGameplay.reset( conf.GetPlayers().GetColors() );

        for ( uint32_t roundId = 0; roundId < roundCount; ++roundId ) {
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
    }
}

namespace fheroes2
{
    bool openMapAutoPlayTest()
    {
        Display & display = Display::instance();

        StandardWindow window( 400, 200, true, display );
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
                runPlayTest( 1, 365 );
                return true;
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
        Player * currentPlayer = conf.GetPlayers().GetCurrent();
        if ( currentPlayer != nullptr && currentPlayer->isAIAutoControlMode() ) {
            if ( fheroes2::showStandardTextMessage( _( "Auto gameplay" ),
                                                    _( "Do you want to interrupt auto gameplay? The effect will take place only on the next turn." ),
                                                    Dialog::YES | Dialog::NO )
                 == Dialog::YES ) {
                for ( Player * player : conf.GetPlayers() ) {
                    if ( player != nullptr ) {
                        player->setAIAutoControlMode( false );
                    }
                }
            }
        }
    }
}
