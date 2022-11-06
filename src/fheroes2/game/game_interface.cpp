/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
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
#include "agg_image.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "game_delays.h"
#include "game_hotkeys.h"
#include "icn.h"
#include "maps.h"
#include "settings.h"
#include "ui_tool.h"
#include "world.h"

Interface::Basic::Basic()
    : gameArea( *this )
    , radar( *this )
    , iconsPanel( *this )
    , buttonsArea( *this )
    , statusWindow( *this )
    , controlPanel( *this )
    , redraw( 0 )
    , scrollDelayType( Game::SCROLL_START_DELAY )
    , lastScrollPosition( SCROLL_NONE )
{
    Reset();
}

void Interface::Basic::Reset()
{
    const fheroes2::Display & display = fheroes2::Display::instance();

    Settings & conf = Settings::Get();
    const bool isHideInterface = conf.ExtGameHideInterface();

    if ( isHideInterface ) {
        conf.SetShowPanel( true );

        controlPanel.SetPos( display.width() - controlPanel.GetArea().width - BORDERWIDTH, 0 );

        fheroes2::Point radrPos = conf.PosRadar();
        fheroes2::Point bttnPos = conf.PosButtons();
        fheroes2::Point iconPos = conf.PosIcons();
        fheroes2::Point statPos = conf.PosStatus();

        auto isPosValid = []( const fheroes2::Point & pos ) { return pos.x >= 0 && pos.y >= 0; };

        if ( isPosValid( radrPos ) && isPosValid( bttnPos ) && isPosValid( iconPos ) && isPosValid( statPos ) ) {
            radar.SetPos( radrPos.x, radrPos.y );
            iconsPanel.SetPos( iconPos.x, iconPos.y );
            buttonsArea.SetPos( bttnPos.x, bttnPos.y );
            statusWindow.SetPos( statPos.x, statPos.y );
        }
        else {
            radar.SetPos( 0, 0 );
            // It's OK to use display.width() for the X coordinate here, panel will be docked to the right edge
            iconsPanel.SetPos( display.width(), radar.GetArea().y + radar.GetArea().height + BORDERWIDTH );
            buttonsArea.SetPos( display.width(), iconsPanel.GetArea().y + iconsPanel.GetArea().height + BORDERWIDTH );
            statusWindow.SetPos( display.width(), buttonsArea.GetArea().y + buttonsArea.GetArea().height );
        }
    }
    else {
        const int32_t px = display.width() - BORDERWIDTH - RADARWIDTH;

        radar.SetPos( px, BORDERWIDTH );
        iconsPanel.SetPos( px, radar.GetArea().y + radar.GetArea().height + BORDERWIDTH );
        buttonsArea.SetPos( px, iconsPanel.GetArea().y + iconsPanel.GetArea().height + BORDERWIDTH );
        statusWindow.SetPos( px, buttonsArea.GetArea().y + buttonsArea.GetArea().height );
    }

    const fheroes2::Point prevCenter = gameArea.getCurrentCenterInPixels();
    const fheroes2::Rect prevRoi = gameArea.GetROI();

    gameArea.generate( { display.width(), display.height() }, isHideInterface );

    const fheroes2::Rect newRoi = gameArea.GetROI();

    gameArea.SetCenterInPixels( prevCenter + fheroes2::Point( newRoi.x + newRoi.width / 2, newRoi.y + newRoi.height / 2 )
                                - fheroes2::Point( prevRoi.x + prevRoi.width / 2, prevRoi.y + prevRoi.height / 2 ) );
}

Interface::Basic & Interface::Basic::Get()
{
    static Basic basic;
    return basic;
}

void Interface::Basic::Redraw( const uint32_t force /* = 0 */ )
{
    const Settings & conf = Settings::Get();

    const uint32_t combinedRedraw = redraw | force;
    const bool hideInterface = conf.ExtGameHideInterface();

    if ( combinedRedraw & REDRAW_GAMEAREA ) {
        gameArea.Redraw( fheroes2::Display::instance(), LEVEL_ALL );

        if ( hideInterface && conf.ShowControlPanel() ) {
            controlPanel.Redraw();
        }
    }

    if ( ( hideInterface && conf.ShowRadar() ) || ( combinedRedraw & REDRAW_RADAR ) ) {
        radar.Redraw();
    }

    if ( ( hideInterface && conf.ShowIcons() ) || ( combinedRedraw & REDRAW_ICONS ) ) {
        iconsPanel.Redraw();
    }
    else if ( combinedRedraw & REDRAW_HEROES ) {
        iconsPanel.RedrawIcons( ICON_HEROES );
    }
    else if ( combinedRedraw & REDRAW_CASTLES ) {
        iconsPanel.RedrawIcons( ICON_CASTLES );
    }

    if ( ( hideInterface && conf.ShowButtons() ) || ( combinedRedraw & REDRAW_BUTTONS ) ) {
        buttonsArea.Redraw();
    }

    if ( ( hideInterface && conf.ShowStatus() ) || ( combinedRedraw & REDRAW_STATUS ) ) {
        statusWindow.Redraw();
    }

    if ( combinedRedraw & REDRAW_BORDER ) {
        GameBorderRedraw( false );
    }

    redraw = 0;
}

int32_t Interface::Basic::GetDimensionDoorDestination( const int32_t from, const int32_t distance, const bool water )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    const Settings & conf = Settings::Get();
    const bool isEvilInterface = conf.ExtGameEvilInterface();
    const bool isHideInterface = conf.ExtGameHideInterface();

    const fheroes2::Rect & radarRect = radar.GetRect();
    const fheroes2::Rect & radarArea = radar.GetArea();

    fheroes2::Button buttonExit( radarArea.x + 32, radarArea.y + radarArea.height - 37, ( isEvilInterface ? ICN::LGNDXTRE : ICN::LGNDXTRA ), 4, 5 );

    auto drawControlPanel = [&display, isEvilInterface, isHideInterface, &radarRect, &radarArea, &buttonExit]() {
        if ( isHideInterface ) {
            Dialog::FrameBorder::RenderRegular( radarRect );
        }

        fheroes2::Blit( fheroes2::AGG::GetICN( ( isEvilInterface ? ICN::EVIWDDOR : ICN::VIEWDDOR ), 0 ), display, radarArea.x, radarArea.y );

        buttonExit.draw();
    };

    const fheroes2::Rect & visibleArea = gameArea.GetROI();
    const bool isFadingEnabled = ( gameArea.GetROI().width > TILEWIDTH * distance ) || ( gameArea.GetROI().height > TILEWIDTH * distance );

    // We need to add an extra one cell as a hero stands exactly in the middle of a cell
    const fheroes2::Point heroPos( gameArea.GetRelativeTilePosition( Maps::GetPoint( from ) ) );
    const fheroes2::Point heroPosOffset( heroPos.x - TILEWIDTH * ( distance / 2 ), heroPos.y - TILEWIDTH * ( distance / 2 ) );
    const fheroes2::Rect spellROI( heroPosOffset.x, heroPosOffset.y, TILEWIDTH * ( distance + 1 ), TILEWIDTH * ( distance + 1 ) );

    if ( isFadingEnabled ) {
        if ( isHideInterface ) {
            InvertedShadow( display, visibleArea, spellROI, 5, 9 );

            drawControlPanel();
        }
        else {
            drawControlPanel();

            fheroes2::InvertedFadeWithPalette( display, visibleArea, spellROI, 5, 300, 9 );
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

    while ( le.HandleEvents() ) {
        const fheroes2::Point & mp = le.GetMouseCursor();

        if ( radarRect & mp ) {
            cursor.SetThemes( Cursor::POINTER );

            le.MousePressLeft( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();
            if ( le.MouseClickLeft( buttonExit.area() ) || Game::HotKeyCloseWindow() ) {
                break;
            }
        }
        else if ( visibleArea & mp ) {
            const int32_t dst = gameArea.GetValidTileIdFromPoint( mp );

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
            uint32_t & frame = Game::MapsAnimationFrame();
            ++frame;

            Redraw( REDRAW_GAMEAREA );

            if ( isFadingEnabled ) {
                InvertedShadow( display, visibleArea, spellROI, 5, 9 );

                if ( isHideInterface ) {
                    drawControlPanel();
                }
            }

            display.render();
        }
    }

    if ( isFadingEnabled ) {
        gameArea.SetRedraw();
    }

    Redraw( REDRAW_RADAR );
    display.render();

    return returnValue;
}
