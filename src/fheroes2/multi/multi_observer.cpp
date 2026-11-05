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

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "castle.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "game_delays.h"
#include "game_exit.h"
#include "game_hotkeys.h"
#include "game_interface.h"
#include "game_mode.h"
#include "heroes.h"
#include "interface_buttons.h"
#include "interface_gamearea.h"
#include "interface_radar.h"
#include "interface_status.h"
#include "localevent.h"
#include "maps.h"
#include "maps_tiles.h"
#include "math_base.h"
#include "multi_notice.h"
#include "multi_simultaneous.h"
#include "multi_spectator.h"
#include "screen.h"
#include "settings.h"
#include "system.h"
#include "translations.h"
#include "ui_dialog.h"
#include "world.h"

fheroes2::GameMode Interface::AdventureMap::observeOtherPlayers( const std::function<bool()> & keepObserving )
{
    const Settings & conf = Settings::Get();
    LocalEvent & le = LocalEvent::Get();

    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    reset();

    _radar.Build();
    _radar.SetHide( false );
    _iconsPanel.resetIcons( ICON_ANY );
    _iconsPanel.showIcons( ICON_ANY );

    // Hourglass in the status panel, as during the turn of an AI player.
    class StatusPanelRestorer
    {
    public:
        explicit StatusPanelRestorer( StatusPanel & panel )
            : _panel( &panel )
        {
            // Do nothing.
        }

        StatusPanelRestorer( const StatusPanelRestorer & ) = delete;
        StatusPanelRestorer & operator=( const StatusPanelRestorer & ) = delete;

        ~StatusPanelRestorer()
        {
            _panel->Reset();
            _panel->setRedraw();
        }

    private:
        StatusPanel * _panel;
    };

    const StatusPanelRestorer statusPanelRestorer( _statusPanel );
    _statusPanel.Reset();
    _statusPanel.SetState( StatusType::STATUS_AITURN );

    // Show the view of the current player with their fog of war, heroes and castles.
    Interface::GameArea::updateMapFogDirections();
    updateFocus();

    // Nothnig is selected yet before the first turn of the player.
    if ( GetFocusType() == GameFocus::UNSEL ) {
        ResetFocus( GameFocus::FIRSTHERO, false );
    }

    redraw( REDRAW_ALL );
    std::string bannerText = Multi::getObserverBannerText();
    validateFadeInAndRender();

    const std::vector<Game::DelayType> delayTypes = { Game::DelayType::MAPS_DELAY };

    while ( true ) {
        if ( !le.HandleEvents( Game::isDelayNeeded( delayTypes ), true ) ) {
            if ( Game::processExitEvent() == fheroes2::GameMode::QUIT_GAME ) {
                return fheroes2::GameMode::QUIT_GAME;
            }

            continue;
        }

        const auto isLeaveConfirmed = []() {
            return ( Dialog::YES
                     & fheroes2::showStandardTextMessage( _( "Network game" ), _( "Are you sure you want to leave the network game?" ), Dialog::YES | Dialog::NO ) )
                   != 0;
        };

        // The banner is shown only while a battle of an ally can be watched.
        if ( Multi::isSpectatedBattleAvailable() && ( le.isKeyPressed( fheroes2::Key::KEY_B ) || le.MouseClickLeft( Multi::getMapBannerArea() ) ) ) {
            Multi::watchSpectatedBattle();
            setRedraw( REDRAW_ALL );
        }
        else if ( Multi::canResumeDay() && le.isKeyPressed( fheroes2::Key::KEY_R ) ) {
            Multi::askDayResume();
        }

        Multi::spectateForTest();

        if ( !keepObserving() ) {
            return fheroes2::GameMode::CANCEL;
        }

        if ( Multi::getObserverBannerText() != bannerText ) {
            bannerText = Multi::getObserverBannerText();
            setRedraw( REDRAW_GAMEAREA );
        }

        if ( le.isAnyKeyPressed() ) {
            if ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
                if ( isLeaveConfirmed() ) {
                    return fheroes2::GameMode::MAIN_MENU;
                }
            }
            else if ( conf.ScrollSpeed() != SCROLL_SPEED_NONE ) {
                if ( Game::HotKeyPressEvent( Game::HotKeyEvent::WORLD_SCROLL_LEFT ) ) {
                    _gameArea.SetScroll( SCROLL_LEFT );
                }
                else if ( Game::HotKeyPressEvent( Game::HotKeyEvent::WORLD_SCROLL_RIGHT ) ) {
                    _gameArea.SetScroll( SCROLL_RIGHT );
                }
                else if ( Game::HotKeyPressEvent( Game::HotKeyEvent::WORLD_SCROLL_UP ) ) {
                    _gameArea.SetScroll( SCROLL_TOP );
                }
                else if ( Game::HotKeyPressEvent( Game::HotKeyEvent::WORLD_SCROLL_DOWN ) ) {
                    _gameArea.SetScroll( SCROLL_BOTTOM );
                }
            }
        }

        const fheroes2::Point & mousePosition = le.getMouseCursorPos();

        // Only the file and system menus, the kingdom overview and the end of turn button (to resume the day) are active.
        fheroes2::GameMode buttonsMode = fheroes2::GameMode::CANCEL;

        // A panel which captured the mouse keeps it, otherwise the panel under the cursor gets the events. isMouseCaptured()
        // releases the capture once the button is up, and returns true one last time for the release: only one call each.
        const bool isIconsCaptured = _iconsPanel.isMouseCaptured();
        const bool isButtonsCaptured = !isIconsCaptured && _buttonsPanel.isMouseCaptured();
        const bool isRadarCaptured = !isIconsCaptured && !isButtonsCaptured && _radar.isMouseCaptured();
        const bool isCaptured = isIconsCaptured || isButtonsCaptured || isRadarCaptured;

        const bool isForIcons = isCaptured ? isIconsCaptured : le.isMouseCursorPosInArea( _iconsPanel.GetRect() );
        const bool isForButtons = isCaptured ? isButtonsCaptured : le.isMouseCursorPosInArea( _buttonsPanel.GetRect() );
        const bool isForRadar = isCaptured ? isRadarCaptured : le.isMouseCursorPosInArea( _radar.GetRect() );

        if ( isForIcons ) {
            // Heroes open in read-only mode, castles do not open.
            _iconsPanel.queueEventProcessing();
        }
        else if ( isForButtons ) {
            buttonsMode = _buttonsPanel.queueEventProcessing();
        }
        else if ( isForRadar ) {
            _radar.QueueEventProcessing();
        }
        else if ( le.isMouseCursorPosInArea( _gameArea.GetROI() ) ) {
            if ( fheroes2::Cursor::isFocusActive() && conf.ScrollSpeed() != SCROLL_SPEED_NONE ) {
                int scrollDirection = SCROLL_NONE;

                if ( isScrollLeft( mousePosition ) ) {
                    scrollDirection |= SCROLL_LEFT;
                }
                else if ( isScrollRight( mousePosition ) ) {
                    scrollDirection |= SCROLL_RIGHT;
                }
                if ( isScrollTop( mousePosition ) ) {
                    scrollDirection |= SCROLL_TOP;
                }
                else if ( isScrollBottom( mousePosition ) ) {
                    scrollDirection |= SCROLL_BOTTOM;
                }

                if ( scrollDirection != SCROLL_NONE ) {
                    _gameArea.SetScroll( scrollDirection );
                }
            }

            const int32_t tileIndex = _gameArea.GetValidTileIdFromPoint( mousePosition );

            if ( Maps::isValidAbsIndex( tileIndex ) && !_gameArea.NeedScroll() ) {
                if ( le.isMouseRightButtonPressedInArea( _gameArea.GetROI() ) ) {
                    mouseCursorAreaPressRight( tileIndex );
                }
                else if ( le.MouseClickLeft( _gameArea.GetROI() ) ) {
                    const Maps::Tile & tile = world.getTile( tileIndex );
                    Heroes * hero = tile.getHero();
                    Castle * castle = world.getCastle( tile.GetCenter() );

                    if ( hero != nullptr && hero->isFriends( conf.CurrentColor() ) ) {
                        Game::OpenHeroesDialog( *hero, false, true );
                        setRedraw( REDRAW_ALL );
                    }
                    else if ( castle != nullptr && castle->isFriends( conf.CurrentColor() ) ) {
                        Game::OpenCastleDialog( *castle, false );
                        setRedraw( REDRAW_ALL );
                    }
                }
            }
        }

        if ( buttonsMode != fheroes2::GameMode::CANCEL ) {
            return buttonsMode;
        }

        if ( _gameArea.NeedScroll() && Game::validateAnimationDelay( Game::DelayType::SCROLL_DELAY ) ) {
            _gameArea.Scroll();
            setRedraw( REDRAW_GAMEAREA | REDRAW_RADAR_CURSOR );
        }

        if ( Game::validateAnimationDelay( Game::DelayType::MAPS_DELAY ) ) {
            Game::updateAdventureMapAnimationIndex();
            _gameArea.SetRedraw();
            setRedraw( REDRAW_STATUS | REDRAW_BUTTONS );
        }

        // Selecting a hero or a castle changes the status panel.
        _statusPanel.SetState( StatusType::STATUS_AITURN );

        if ( needRedraw() ) {
            redraw( 0 );
            validateFadeInAndRender();
        }
    }
}
