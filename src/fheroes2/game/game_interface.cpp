/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
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

#include <sstream>

#include "agg_image.h"
#include "cursor.h"
#include "dialog.h"
#include "direction.h"
#include "game.h"
#include "game_delays.h"
#include "game_interface.h"
#include "icn.h"
#include "maps.h"
#include "mp2.h"
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
{
    Reset();
}

void Interface::Basic::Reset()
{
    const fheroes2::Display & display = fheroes2::Display::instance();
    const Settings & conf = Settings::Get();

    SetHideInterface( conf.ExtGameHideInterface() );

    scrollLeft = fheroes2::Rect( 0, 0, BORDERWIDTH, display.height() );
    scrollRight = fheroes2::Rect( display.width() - BORDERWIDTH, 0, BORDERWIDTH, display.height() );
    scrollTop = fheroes2::Rect( 0, 0, display.width(), BORDERWIDTH );
    scrollBottom = fheroes2::Rect( 0, display.height() - BORDERWIDTH, display.width(), BORDERWIDTH );
}

Interface::GameArea & Interface::Basic::GetGameArea( void )
{
    return gameArea;
}

Interface::Radar & Interface::Basic::GetRadar( void )
{
    return radar;
}

Interface::IconsPanel & Interface::Basic::GetIconsPanel( void )
{
    return iconsPanel;
}

Interface::ButtonsArea & Interface::Basic::GetButtonsArea( void )
{
    return buttonsArea;
}

Interface::StatusWindow & Interface::Basic::GetStatusWindow( void )
{
    return statusWindow;
}

Interface::ControlPanel & Interface::Basic::GetControlPanel( void )
{
    return controlPanel;
}

void Interface::Basic::SetHideInterface( bool f )
{
    const fheroes2::Display & display = fheroes2::Display::instance();
    Settings & conf = Settings::Get();
    const u32 px = display.width() - BORDERWIDTH - RADARWIDTH;

    conf.SetHideInterface( f );

    if ( f ) {
        conf.SetShowPanel( true );

        fheroes2::Point pos_radr = conf.PosRadar();
        fheroes2::Point pos_bttn = conf.PosButtons();
        fheroes2::Point pos_icon = conf.PosIcons();
        fheroes2::Point pos_stat = conf.PosStatus();

        if ( 0 == pos_radr.x && 0 == pos_radr.y )
            pos_radr = fheroes2::Point( BORDERWIDTH, BORDERWIDTH );
        if ( 0 == pos_icon.x && 0 == pos_icon.y )
            pos_icon = fheroes2::Point( px - BORDERWIDTH, radar.GetArea().y + radar.GetArea().height );
        if ( 0 == pos_bttn.x && 0 == pos_bttn.y )
            pos_bttn = fheroes2::Point( px - BORDERWIDTH, iconsPanel.GetArea().y + iconsPanel.GetArea().height );
        if ( 0 == pos_stat.x && 0 == pos_stat.y )
            pos_stat = fheroes2::Point( px - BORDERWIDTH, buttonsArea.GetArea().y + buttonsArea.GetArea().height );

        controlPanel.SetPos( display.width() - controlPanel.GetArea().width - BORDERWIDTH, 0 );
        radar.SetPos( pos_radr.x, pos_radr.y );
        iconsPanel.SetPos( pos_icon.x, pos_icon.y );
        buttonsArea.SetPos( pos_bttn.x, pos_bttn.y );
        statusWindow.SetPos( pos_stat.x, pos_stat.y );
    }
    else {
        radar.SetPos( px, BORDERWIDTH );
        iconsPanel.SetPos( px, radar.GetArea().y + radar.GetArea().height + BORDERWIDTH );

        buttonsArea.SetPos( px, iconsPanel.GetArea().y + iconsPanel.GetArea().height + BORDERWIDTH );
        statusWindow.SetPos( px, buttonsArea.GetArea().y + buttonsArea.GetArea().height );
    }

    gameArea.Build();
}

Interface::Basic & Interface::Basic::Get( void )
{
    static Basic basic;
    return basic;
}

const fheroes2::Rect & Interface::Basic::GetScrollLeft( void ) const
{
    return scrollLeft;
}

const fheroes2::Rect & Interface::Basic::GetScrollRight( void ) const
{
    return scrollRight;
}

const fheroes2::Rect & Interface::Basic::GetScrollTop( void ) const
{
    return scrollTop;
}

const fheroes2::Rect & Interface::Basic::GetScrollBottom( void ) const
{
    return scrollBottom;
}

bool Interface::Basic::NeedRedraw( void ) const
{
    return redraw != 0;
}

void Interface::Basic::SetRedraw( int f )
{
    redraw |= f;
}

int Interface::Basic::GetRedrawMask() const
{
    return redraw;
}

void Interface::Basic::Redraw( int force )
{
    const Settings & conf = Settings::Get();

    const int combinedRedraw = redraw | force;
    const bool hideInterface = conf.ExtGameHideInterface();

    if ( combinedRedraw & REDRAW_GAMEAREA )
        gameArea.Redraw( fheroes2::Display::instance(), LEVEL_ALL );

    if ( ( hideInterface && conf.ShowRadar() ) || ( combinedRedraw & REDRAW_RADAR ) )
        radar.Redraw();

    if ( ( hideInterface && conf.ShowIcons() ) || ( combinedRedraw & REDRAW_ICONS ) )
        iconsPanel.Redraw();
    else if ( combinedRedraw & REDRAW_HEROES )
        iconsPanel.RedrawIcons( ICON_HEROES );
    else if ( combinedRedraw & REDRAW_CASTLES )
        iconsPanel.RedrawIcons( ICON_CASTLES );

    if ( ( hideInterface && conf.ShowButtons() ) || ( combinedRedraw & REDRAW_BUTTONS ) )
        buttonsArea.Redraw();

    if ( ( hideInterface && conf.ShowStatus() ) || ( combinedRedraw & REDRAW_STATUS ) )
        statusWindow.Redraw();

    if ( hideInterface && conf.ShowControlPanel() && ( redraw & REDRAW_GAMEAREA ) )
        controlPanel.Redraw();

    if ( combinedRedraw & REDRAW_BORDER )
        GameBorderRedraw( false );

    redraw = 0;
}

int32_t Interface::Basic::GetDimensionDoorDestination( const int32_t from, const int32_t distance, const bool water )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    const fheroes2::Rect & radarArea = Interface::Basic::Get().GetRadar().GetArea();
    const Settings & conf = Settings::Get();
    const bool isEvilInterface = conf.ExtGameEvilInterface();
    const bool isNoInterface = conf.ExtGameHideInterface();

    fheroes2::ImageRestorer back( display, radarArea.x, radarArea.y, radarArea.width, radarArea.height );

    const fheroes2::Sprite & viewDoor = fheroes2::AGG::GetICN( ( isEvilInterface ? ICN::EVIWDDOR : ICN::VIEWDDOR ), 0 );
    fheroes2::Blit( viewDoor, 0, 0, display, radarArea.x, radarArea.y, radarArea.width, radarArea.height );

    const fheroes2::Rect & visibleArea = gameArea.GetROI();
    const bool isFadingEnabled = ( gameArea.GetROI().width > TILEWIDTH * distance ) || ( gameArea.GetROI().height > TILEWIDTH * distance );

    // We need to add an extra one cell as a hero stands exactly in the middle of a cell
    const fheroes2::Point heroPos( gameArea.GetRelativeTilePosition( Maps::GetPoint( from ) ) );
    const fheroes2::Point heroPosOffset( heroPos.x - TILEWIDTH * ( distance / 2 ), heroPos.y - TILEWIDTH * ( distance / 2 ) );
    const fheroes2::Rect spellROI( heroPosOffset.x, heroPosOffset.y, TILEWIDTH * ( distance + 1 ), TILEWIDTH * ( distance + 1 ) );

    if ( isFadingEnabled ) {
        fheroes2::InvertedFadeWithPalette( display, visibleArea, spellROI, 5, 300, 9 );
    }

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );
    Cursor & cursor = Cursor::Get();

    LocalEvent & le = LocalEvent::Get();
    int32_t returnValue = -1;

    const fheroes2::Point exitButtonPos( radarArea.x + 32, radarArea.y + radarArea.height - 37 );
    fheroes2::Button buttonExit( exitButtonPos.x, exitButtonPos.y, ( isEvilInterface ? ICN::LGNDXTRE : ICN::LGNDXTRA ), 4, 5 );
    buttonExit.draw();

    while ( le.HandleEvents() ) {
        const fheroes2::Point & mp = le.GetMouseCursor();

        if ( radarArea & mp ) {
            cursor.SetThemes( Cursor::POINTER );

            le.MousePressLeft( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();
            if ( le.MouseClickLeft( buttonExit.area() ) || HotKeyCloseWindow )
                break;
        }
        else if ( visibleArea & mp ) {
            const int32_t dst = gameArea.GetValidTileIdFromPoint( mp );

            bool valid = ( dst >= 0 );

            if ( valid ) {
                const Maps::Tiles & tile = world.GetTiles( dst );

                valid = ( ( spellROI & mp ) && MP2::isClearGroundObject( tile.GetObject() ) && water == world.GetTiles( dst ).isWater() );
            }

            cursor.SetThemes( valid ? ( water ? static_cast<int>( Cursor::CURSOR_HERO_BOAT ) : static_cast<int>( Cursor::CURSOR_HERO_MOVE ) )
                                    : static_cast<int>( Cursor::WAR_NONE ) );

            if ( dst >= 0 && le.MousePressRight() ) {
                const Maps::Tiles & tile = world.GetTiles( dst );
                Dialog::QuickInfo( tile );
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
            gameArea.SetRedraw();
            Redraw();

            if ( isFadingEnabled ) {
                InvertedShadow( display, visibleArea, spellROI, 5, 9 );

                if ( isNoInterface ) {
                    fheroes2::Blit( viewDoor, 0, 0, display, radarArea.x, radarArea.y, radarArea.width, radarArea.height );
                    buttonExit.draw();
                }
            }

            display.render();
        }
    }

    if ( isFadingEnabled ) {
        gameArea.SetRedraw();
        Redraw();
        display.render();
    }

    back.restore();
    display.render();

    return returnValue;
}
