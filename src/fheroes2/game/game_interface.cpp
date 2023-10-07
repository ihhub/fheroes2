/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "game_interface.h"

#include <algorithm>

#include "agg_image.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "game_delays.h"
#include "game_hotkeys.h"
#include "gamedefs.h"
#include "icn.h"
#include "image.h"
#include "interface_border.h"
#include "interface_gamearea.h"
#include "interface_radar.h"
#include "interface_status.h"
#include "localevent.h"
#include "maps.h"
#include "math_base.h"
#include "screen.h"
#include "settings.h"
#include "ui_button.h"
#include "ui_tool.h"
#include "world.h"

Interface::AdventureMap::AdventureMap()
    : BaseInterface( false )
    , iconsPanel( *this )
    , buttonsArea( *this )
    , controlPanel( *this )
    , _lockRedraw( false )
{
    AdventureMap::reset();
}

void Interface::AdventureMap::reset()
{
    const fheroes2::Display & display = fheroes2::Display::instance();

    Settings & conf = Settings::Get();
    const bool isHideInterface = conf.isHideInterfaceEnabled();

    if ( isHideInterface ) {
        conf.SetShowControlPanel( true );

        controlPanel.SetPos( display.width() - controlPanel.GetArea().width - BORDERWIDTH, 0 );

        fheroes2::Point radrPos = conf.PosRadar();
        fheroes2::Point bttnPos = conf.PosButtons();
        fheroes2::Point iconPos = conf.PosIcons();
        fheroes2::Point statPos = conf.PosStatus();

        const auto isPosValid = []( const fheroes2::Point & pos ) { return pos.x >= 0 && pos.y >= 0; };

        if ( isPosValid( radrPos ) && isPosValid( bttnPos ) && isPosValid( iconPos ) && isPosValid( statPos ) ) {
            _radar.SetPos( radrPos.x, radrPos.y );
            iconsPanel.SetPos( iconPos.x, iconPos.y );
            buttonsArea.SetPos( bttnPos.x, bttnPos.y );
            _statusWindow.SetPos( statPos.x, statPos.y );
        }
        else {
            _radar.SetPos( 0, 0 );
            // It's OK to use display.width() for the X coordinate here, panel will be docked to the right edge
            iconsPanel.SetPos( display.width(), _radar.GetRect().y + _radar.GetRect().height );
            buttonsArea.SetPos( display.width(), iconsPanel.GetRect().y + iconsPanel.GetRect().height );
            _statusWindow.SetPos( display.width(), buttonsArea.GetRect().y + buttonsArea.GetRect().height );
        }
    }
    else {
        const int32_t px = display.width() - BORDERWIDTH - RADARWIDTH;

        _radar.SetPos( px, BORDERWIDTH );
        iconsPanel.SetPos( px, _radar.GetArea().y + _radar.GetArea().height + BORDERWIDTH );
        buttonsArea.SetPos( px, iconsPanel.GetArea().y + iconsPanel.GetArea().height + BORDERWIDTH );
        _statusWindow.SetPos( px, buttonsArea.GetArea().y + buttonsArea.GetArea().height );
    }

    const fheroes2::Point prevCenter = _gameArea.getCurrentCenterInPixels();
    const fheroes2::Rect prevRoi = _gameArea.GetROI();

    _gameArea.generate( { display.width(), display.height() }, isHideInterface );

    const fheroes2::Rect newRoi = _gameArea.GetROI();

    _gameArea.SetCenterInPixels( prevCenter + fheroes2::Point( newRoi.x + newRoi.width / 2, newRoi.y + newRoi.height / 2 )
                                 - fheroes2::Point( prevRoi.x + prevRoi.width / 2, prevRoi.y + prevRoi.height / 2 ) );
}

Interface::AdventureMap & Interface::AdventureMap::Get()
{
    static AdventureMap basic;
    return basic;
}

void Interface::AdventureMap::redraw( const uint32_t force )
{
    if ( _lockRedraw ) {
        setRedraw( force );
        return;
    }

    const Settings & conf = Settings::Get();

    const uint32_t combinedRedraw = _redraw | force;
    const bool hideInterface = conf.isHideInterfaceEnabled();

    if ( combinedRedraw & REDRAW_GAMEAREA ) {
        _gameArea.Redraw( fheroes2::Display::instance(), LEVEL_ALL );

        if ( hideInterface && conf.ShowControlPanel() ) {
            controlPanel._redraw();
        }
    }

    if ( ( hideInterface && conf.ShowRadar() ) || ( combinedRedraw & ( REDRAW_RADAR_CURSOR | REDRAW_RADAR ) ) ) {
        // Redraw radar map only if `REDRAW_RADAR` is set.
        _radar._redraw( combinedRedraw & REDRAW_RADAR );
    }

    if ( ( hideInterface && conf.ShowIcons() ) || ( combinedRedraw & REDRAW_ICONS ) ) {
        iconsPanel._redraw();
    }
    else if ( combinedRedraw & REDRAW_HEROES ) {
        iconsPanel._redrawIcons( ICON_HEROES );
    }
    else if ( combinedRedraw & REDRAW_CASTLES ) {
        iconsPanel._redrawIcons( ICON_CASTLES );
    }

    if ( ( hideInterface && conf.ShowButtons() ) || ( combinedRedraw & REDRAW_BUTTONS ) ) {
        buttonsArea._redraw();
    }

    if ( ( hideInterface && conf.ShowStatus() ) || ( combinedRedraw & REDRAW_STATUS ) ) {
        _statusWindow._redraw();
    }

    if ( combinedRedraw & REDRAW_BORDER ) {
        GameBorderRedraw( false );
    }

    _redraw = 0;
}

int32_t Interface::AdventureMap::GetDimensionDoorDestination( const int32_t from, const int32_t distance, const bool water )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    Settings & conf = Settings::Get();
    const bool isEvilInterface = conf.isEvilInterfaceEnabled();
    const bool isHideInterface = conf.isHideInterfaceEnabled();
    const bool isIconsVisible = conf.ShowIcons();
    const bool isButtonsVisible = conf.ShowButtons();
    const bool isStatusVisible = conf.ShowStatus();
    const bool isControlPanelVisible = conf.ShowControlPanel();

    const fheroes2::Rect & radarRect = _radar.GetRect();
    const fheroes2::Rect & radarArea = _radar.GetArea();

    fheroes2::Button buttonExit( radarArea.x + 32, radarArea.y + radarArea.height - 37,
                                 ( isEvilInterface ? ICN::BUTTON_EXIT_PUZZLE_DDOOR_EVIL : ICN::BUTTON_EXIT_PUZZLE_DDOOR_GOOD ), 0, 1 );

    const auto drawControlPanel = [&display, isEvilInterface, isHideInterface, &radarRect, &radarArea, &buttonExit]() {
        if ( isHideInterface ) {
            Dialog::FrameBorder::RenderRegular( radarRect );
        }

        fheroes2::Blit( fheroes2::AGG::GetICN( ( isEvilInterface ? ICN::EVIWDDOR : ICN::VIEWDDOR ), 0 ), display, radarArea.x, radarArea.y );

        buttonExit.draw();
    };

    const fheroes2::Rect & gameAreaROI = _gameArea.GetROI();
    const bool isFadingEnabled = ( gameAreaROI.width > TILEWIDTH * distance ) || ( gameAreaROI.height > TILEWIDTH * distance );

    const fheroes2::Rect spellROI = [this, from, distance, isHideInterface, &gameAreaROI]() -> fheroes2::Rect {
        const fheroes2::Point heroPos = _gameArea.GetRelativeTilePosition( Maps::GetPoint( from ) );

        const int32_t x = heroPos.x - TILEWIDTH * ( distance / 2 );
        const int32_t y = heroPos.y - TILEWIDTH * ( distance / 2 );

        // We need to add an extra cell since the hero stands exactly in the middle of a cell
        const int32_t w = std::min( TILEWIDTH * ( distance + 1 ), gameAreaROI.width );
        const int32_t h = std::min( TILEWIDTH * ( distance + 1 ), gameAreaROI.height );

        return { isHideInterface ? x : std::max( x, BORDERWIDTH ), isHideInterface ? y : std::max( y, BORDERWIDTH ), w, h };
    }();

    if ( isHideInterface ) {
        // There is no need to hide the radar because it will be replaced by the Dimension Door control panel
        conf.SetShowIcons( false );
        conf.SetShowButtons( false );
        conf.SetShowStatus( false );
        conf.SetShowControlPanel( false );

        redraw( REDRAW_GAMEAREA );
    }

    if ( isFadingEnabled ) {
        if ( isHideInterface ) {
            InvertedShadow( display, gameAreaROI, spellROI, 5, 9 );

            drawControlPanel();
        }
        else {
            drawControlPanel();

            fheroes2::InvertedFadeWithPalette( display, gameAreaROI, spellROI, 5, 300, 9 );
        }
    }
    else {
        drawControlPanel();
    }

    display.render();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );
    Cursor & cursor = Cursor::Get();

    LocalEvent & le = LocalEvent::Get();
    int32_t returnValue = -1;

    while ( le.HandleEvents( Game::isDelayNeeded( { Game::MAPS_DELAY } ) ) ) {
        const fheroes2::Point & mp = le.GetMouseCursor();

        if ( radarRect & mp ) {
            cursor.SetThemes( Cursor::POINTER );

            le.MousePressLeft( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();
            if ( le.MouseClickLeft( buttonExit.area() ) || Game::HotKeyCloseWindow() ) {
                break;
            }
        }
        else if ( gameAreaROI & mp ) {
            const int32_t dst = _gameArea.GetValidTileIdFromPoint( mp );

            bool valid = ( dst >= 0 );

            if ( valid ) {
                valid = ( spellROI & mp ) && Maps::isValidForDimensionDoor( dst, water );
            }

            cursor.SetThemes( valid ? ( water ? static_cast<int>( Cursor::CURSOR_HERO_BOAT ) : static_cast<int>( Cursor::CURSOR_HERO_MOVE ) )
                                    : static_cast<int>( Cursor::WAR_NONE ) );

            if ( dst >= 0 && le.MousePressRight() ) {
                Dialog::QuickInfo( world.GetTiles( dst ) );
            }
            else if ( le.MouseClickLeft() && valid ) {
                returnValue = dst;
                break;
            }
        }
        else {
            cursor.SetThemes( Cursor::POINTER );
        }

        if ( Game::validateAnimationDelay( Game::MAPS_DELAY ) ) {
            Game::updateAdventureMapAnimationIndex();

            redraw( REDRAW_GAMEAREA );

            if ( isFadingEnabled ) {
                InvertedShadow( display, gameAreaROI, spellROI, 5, 9 );

                if ( isHideInterface ) {
                    drawControlPanel();
                }
            }

            display.render();
        }
    }

    if ( isFadingEnabled ) {
        setRedraw( REDRAW_GAMEAREA );
    }

    if ( isHideInterface ) {
        conf.SetShowIcons( isIconsVisible );
        conf.SetShowButtons( isButtonsVisible );
        conf.SetShowStatus( isStatusVisible );
        conf.SetShowControlPanel( isControlPanelVisible );

        setRedraw( REDRAW_ICONS | REDRAW_BUTTONS | REDRAW_STATUS | REDRAW_GAMEAREA );
    }

    redraw( REDRAW_RADAR_CURSOR );
    display.render();

    return returnValue;
}
