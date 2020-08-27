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

#include <ctime>
#include <sstream>

#include "agg.h"
#include "dialog.h"
#include "direction.h"
#include "game.h"
#include "game_interface.h"
#include "maps.h"
#include "mp2.h"
#include "players.h"
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

    scrollLeft = Rect( 0, 0, BORDERWIDTH, display.height() );
    scrollRight = Rect( display.width() - BORDERWIDTH, 0, BORDERWIDTH, display.height() );
    scrollTop = Rect( 0, 0, display.width(), BORDERWIDTH );
    scrollBottom = Rect( 0, display.height() - BORDERWIDTH, display.width(), BORDERWIDTH );

    system_info.Set( Font::YELLOW_SMALL );
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
            pos_radr = Point( BORDERWIDTH, conf.QVGA() ? TILEWIDTH : BORDERWIDTH );
        if ( 0 == pos_icon.x && 0 == pos_icon.y )
            pos_icon = Point( conf.QVGA() ? BORDERWIDTH : px - BORDERWIDTH, conf.QVGA() ? TILEWIDTH : radar.GetArea().y + radar.GetArea().h );
        if ( 0 == pos_bttn.x && 0 == pos_bttn.y )
            pos_bttn = Point( conf.QVGA() ? BORDERWIDTH : px - BORDERWIDTH, conf.QVGA() ? TILEWIDTH : iconsPanel.GetArea().y + iconsPanel.GetArea().h );
        if ( 0 == pos_stat.x && 0 == pos_stat.y )
            pos_stat = Point( conf.QVGA() ? BORDERWIDTH : px - BORDERWIDTH, conf.QVGA() ? TILEWIDTH : buttonsArea.GetArea().y + buttonsArea.GetArea().h );

        controlPanel.SetPos( display.width() - controlPanel.GetArea().w - BORDERWIDTH, 0 );
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

const Rect & Interface::Basic::GetScrollLeft( void ) const
{
    return scrollLeft;
}

const Rect & Interface::Basic::GetScrollRight( void ) const
{
    return scrollRight;
}

const Rect & Interface::Basic::GetScrollTop( void ) const
{
    return scrollTop;
}

const Rect & Interface::Basic::GetScrollBottom( void ) const
{
    return scrollBottom;
}

bool Interface::Basic::NeedRedraw( void ) const
{
    return redraw;
}

void Interface::Basic::SetRedraw( int f )
{
    redraw |= f;
}

void Interface::Basic::Redraw( int force )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    Settings & conf = Settings::Get();

    if ( ( redraw | force ) & REDRAW_GAMEAREA )
        gameArea.Redraw( display, LEVEL_ALL );

    if ( ( conf.ExtGameHideInterface() && conf.ShowRadar() ) || ( ( redraw | force ) & REDRAW_RADAR ) )
        radar.Redraw();

    if ( ( conf.ExtGameHideInterface() && conf.ShowIcons() ) || ( ( redraw | force ) & REDRAW_ICONS ) )
        iconsPanel.Redraw();
    else if ( ( redraw | force ) & REDRAW_HEROES )
        iconsPanel.RedrawIcons( ICON_HEROES );
    else if ( ( redraw | force ) & REDRAW_CASTLES )
        iconsPanel.RedrawIcons( ICON_CASTLES );

    if ( ( conf.ExtGameHideInterface() && conf.ShowButtons() ) || ( ( redraw | force ) & REDRAW_BUTTONS ) )
        buttonsArea.Redraw();

    if ( ( conf.ExtGameHideInterface() && conf.ShowStatus() ) || ( ( redraw | force ) & REDRAW_STATUS ) )
        statusWindow.Redraw();

    if ( conf.ExtGameHideInterface() && conf.ShowControlPanel() && ( redraw & REDRAW_GAMEAREA ) )
        controlPanel.Redraw();

    // show system info
    if ( conf.ExtGameShowSystemInfo() )
        RedrawSystemInfo( ( conf.ExtGameHideInterface() ? 10 : 26 ), display.height() - ( conf.ExtGameHideInterface() ? 14 : 30 ), System::GetMemoryUsage() );

    if ( ( redraw | force ) & REDRAW_BORDER )
        GameBorderRedraw();

    redraw = 0;
}

void Interface::Basic::RedrawSystemInfo( s32 cx, s32 cy, u32 usage )
{
    std::ostringstream os;

    os << "mem. usage: " << usage / 1024 << "Kb"
       << ", cur. time: ";

    time_t rawtime;
    std::time( &rawtime );
    // strtime format: Www Mmm dd hh:mm:ss yyyy
    const char * strtime = std::ctime( &rawtime );

    // draw info
    os << std::string( &strtime[11], 8 );

    system_info.Set( os.str() );
    system_info.Blit( cx, cy );
}

s32 Interface::Basic::GetDimensionDoorDestination( s32 from, u32 distance, bool water ) const
{
    fheroes2::Display & display = fheroes2::Display::instance();

    Interface::Radar & radar = Interface::Basic::Get().GetRadar();
    const Rect & radarArea = radar.GetArea();
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

                valid = ( ( spellROI & mp ) && ( !tile.isFog( conf.CurrentColor() ) ) && MP2::isClearGroundObject( tile.GetObject() )
                          && water == world.GetTiles( dst ).isWater() );
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
