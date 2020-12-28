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

#include "agg.h"
#include "dialog.h"
#include "direction.h"
#include "game.h"
#include "game_interface.h"
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
    Settings & conf = Settings::Get().Get();

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
    Settings & conf = Settings::Get().Get();
    const u32 px = display.width() - BORDERWIDTH - RADARWIDTH;

    conf.SetHideInterface( f );

    if ( f ) {
        conf.SetShowPanel( true );

        Point pos_radr = conf.PosRadar();
        Point pos_bttn = conf.PosButtons();
        Point pos_icon = conf.PosIcons();
        Point pos_stat = conf.PosStatus();

        if ( 0 == pos_radr.x && 0 == pos_radr.y )
            pos_radr = Point( BORDERWIDTH, BORDERWIDTH );
        if ( 0 == pos_icon.x && 0 == pos_icon.y )
            pos_icon = Point( px - BORDERWIDTH, radar.GetArea().y + radar.GetArea().h );
        if ( 0 == pos_bttn.x && 0 == pos_bttn.y )
            pos_bttn = Point( px - BORDERWIDTH, iconsPanel.GetArea().y + iconsPanel.GetArea().h );
        if ( 0 == pos_stat.x && 0 == pos_stat.y )
            pos_stat = Point( px - BORDERWIDTH, buttonsArea.GetArea().y + buttonsArea.GetArea().h );

        controlPanel.SetPos( display.width() - controlPanel.GetArea().width - BORDERWIDTH, 0 );
        radar.SetPos( pos_radr.x, pos_radr.y );
        iconsPanel.SetPos( pos_icon.x, pos_icon.y );
        buttonsArea.SetPos( pos_bttn.x, pos_bttn.y );
        statusWindow.SetPos( pos_stat.x, pos_stat.y );
    }
    else {
        radar.SetPos( px, BORDERWIDTH );
        iconsPanel.SetPos( px, radar.GetArea().y + radar.GetArea().h + BORDERWIDTH );

        buttonsArea.SetPos( px, iconsPanel.GetArea().y + iconsPanel.GetArea().h + BORDERWIDTH );
        statusWindow.SetPos( px, buttonsArea.GetArea().y + buttonsArea.GetArea().h );
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

void Interface::Basic::Redraw( int force )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    Settings & conf = Settings::Get();

    const int combinedRedraw = redraw | force;
    const bool hideInterface = conf.ExtGameHideInterface();

    if ( combinedRedraw & REDRAW_GAMEAREA )
        gameArea.Redraw( display, LEVEL_ALL );

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
        GameBorderRedraw();

    redraw = 0;
}

s32 Interface::Basic::GetDimensionDoorDestination( s32 from, u32 distance, bool water ) const
{
    fheroes2::Display & display = fheroes2::Display::instance();

    const Rect & radarArea = Interface::Basic::Get().GetRadar().GetArea();
    Settings & conf = Settings::Get();
    const fheroes2::Sprite & viewDoor = fheroes2::AGG::GetICN( ( conf.ExtGameEvilInterface() ? ICN::EVIWDDOR : ICN::VIEWDDOR ), 0 );
    fheroes2::ImageRestorer back( display, radarArea.x, radarArea.y, radarArea.w, radarArea.h );

    fheroes2::Blit( viewDoor, 0, 0, display, radarArea.x, radarArea.y, radarArea.w, radarArea.h );

    const Rect & visibleArea = gameArea.GetROI();
    const bool isFadingEnabled = ( gameArea.GetROI().w > TILEWIDTH * distance ) || ( gameArea.GetROI().h > TILEWIDTH * distance );
    fheroes2::Image top( visibleArea.w, visibleArea.h );
    fheroes2::Copy( display, visibleArea.x, visibleArea.y, top, 0, 0, visibleArea.w, visibleArea.h );

    // We need to add an extra one cell as a hero stands exactly in the middle of a cell
    const Point heroPos( gameArea.GetRelativeTilePosition( Maps::GetPoint( from ) ) );
    const fheroes2::Point heroPosOffset( heroPos.x - TILEWIDTH * ( distance / 2 ), heroPos.y - TILEWIDTH * ( distance / 2 ) );
    const Rect spellROI( heroPosOffset.x, heroPosOffset.y, TILEWIDTH * ( distance + 1 ), TILEWIDTH * ( distance + 1 ) );

    if ( isFadingEnabled ) {
        fheroes2::Image middle( spellROI.w, spellROI.h );
        fheroes2::Copy( display, spellROI.x, spellROI.y, middle, 0, 0, spellROI.w, spellROI.h );

        fheroes2::InvertedFadeWithPalette( top, fheroes2::Point( visibleArea.x, visibleArea.y ), middle, heroPosOffset, 5, 300, 9 );
    }

    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();
    s32 dst = -1;
    s32 returnValue = -1;

    const Point exitButtonPos( radarArea.x + 32, radarArea.y + radarArea.h - 37 );
    fheroes2::Button buttonExit( exitButtonPos.x, exitButtonPos.y, ( conf.ExtGameEvilInterface() ? ICN::LGNDXTRE : ICN::LGNDXTRA ), 4, 5 );
    buttonExit.draw();

    while ( le.HandleEvents() ) {
        const Point & mp = le.GetMouseCursor();

        if ( radarArea & mp ) {
            cursor.SetThemes( Cursor::POINTER );

            le.MousePressLeft( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();
            if ( le.MouseClickLeft( buttonExit.area() ) || HotKeyCloseWindow )
                break;
        }
        else if ( visibleArea & mp ) {
            dst = gameArea.GetValidTileIdFromPoint( mp );

            bool valid = ( dst >= 0 );

            if ( valid ) {
                const Maps::Tiles & tile = world.GetTiles( dst );

                valid = ( ( spellROI & mp ) && MP2::isClearGroundObject( tile.GetObject() ) && water == world.GetTiles( dst ).isWater() );
            }

            cursor.SetThemes( valid ? ( water ? Cursor::BOAT : Cursor::MOVE ) : Cursor::WAR_NONE );

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

        // redraw cursor
        if ( !cursor.isVisible() ) {
            cursor.Show();
            display.render();
        }
    }

    cursor.Hide();

    if ( isFadingEnabled )
        fheroes2::Blit( top, display, visibleArea.x, visibleArea.y );

    back.restore();
    cursor.Show();
    display.render();

    return returnValue;
}
