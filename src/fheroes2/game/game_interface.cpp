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
    Settings & conf = Settings::Get().Get();
    const Display & display = Display::Get();
    const int scroll_width = conf.QVGA() ? 12 : BORDERWIDTH;

    SetHideInterface( conf.ExtGameHideInterface() );

    scrollLeft = Rect( 0, 0, scroll_width, display.h() );
    scrollRight = Rect( display.w() - scroll_width, 0, scroll_width, display.h() );
    scrollTop = conf.QVGA() ? Rect( 0, 0, controlPanel.GetArea().x, scroll_width ) : Rect( 0, 0, display.w(), scroll_width );
    scrollBottom = Rect( 0, display.h() - scroll_width, display.w(), scroll_width );

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
    Settings & conf = Settings::Get().Get();
    const Display & display = Display::Get();
    const u32 px = display.w() - BORDERWIDTH - RADARWIDTH;
    const u32 scroll_width = conf.QVGA() ? 12 : BORDERWIDTH;

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

        controlPanel.SetPos( display.w() - controlPanel.GetArea().w - scroll_width, 0 );
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
    Settings & conf = Settings::Get();

    if ( ( redraw | force ) & REDRAW_GAMEAREA )
        gameArea.Redraw( Display::Get(), LEVEL_ALL );

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
        RedrawSystemInfo( ( conf.ExtGameHideInterface() ? 10 : 26 ), Display::Get().h() - ( conf.ExtGameHideInterface() ? 14 : 30 ), System::GetMemoryUsage() );

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
    Display & display = Display::Get();

    Interface::Radar & radar = Interface::Basic::Get().GetRadar();
    const Rect & radarArea = radar.GetArea();
    Settings & conf = Settings::Get();
    const Sprite & viewDoor = AGG::GetICN( ( conf.ExtGameEvilInterface() ? ICN::EVIWDDOR : ICN::VIEWDDOR ), 0 );
    SpriteBack back( Rect( radarArea.x, radarArea.y, radarArea.w, radarArea.h ) );
    viewDoor.Blit( radarArea );

    const Rect & visibleArea = gameArea.GetROI();
    const bool isFadingEnabled = ( gameArea.GetROI().w > TILEWIDTH * distance ) || ( gameArea.GetROI().h > TILEWIDTH * distance );
    const Surface & top = display.GetSurface( visibleArea );

    // We need to add an extra one cell as a hero stands exactly in the middle of a cell
    const Point heroPos( gameArea.GetRelativeTilePosition( Maps::GetPoint( from ) ) );
    const Point heroPosOffset( heroPos.x - TILEWIDTH * ( distance / 2 ), heroPos.y - TILEWIDTH * ( distance / 2 ) );
    const Rect spellROI( heroPosOffset.x, heroPosOffset.y, TILEWIDTH * ( distance + 1 ), TILEWIDTH * ( distance + 1 ) );

    if ( isFadingEnabled ) {
        Surface back( top.GetSize(), false );
        back.Fill( ColorBlack );

        const Surface middle = display.GetSurface( spellROI );
        display.InvertedFade( top, back, Point( visibleArea.x, visibleArea.y ), middle, heroPosOffset, 105, 300 );
    }

    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();
    s32 dst = -1;
    s32 returnValue = -1;

    const Point exitButtonPos( radarArea.x + 32, radarArea.y + radarArea.h - 37 );
    Button buttonExit( exitButtonPos.x, exitButtonPos.y, ( conf.ExtGameEvilInterface() ? ICN::LGNDXTRE : ICN::LGNDXTRA ), 4, 5 );
    buttonExit.Draw();

    while ( le.HandleEvents() ) {
        const Point & mp = le.GetMouseCursor();

        if ( radarArea & mp ) {
            cursor.SetThemes( Cursor::POINTER );

            le.MousePressLeft( buttonExit ) ? buttonExit.PressDraw() : buttonExit.ReleaseDraw();
            if ( le.MouseClickLeft( buttonExit ) || HotKeyCloseWindow )
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
            display.Flip();
        }
    }

    cursor.Hide();

    if ( isFadingEnabled )
        top.Blit( Point( visibleArea.x, visibleArea.y ), display );

    back.Restore();
    cursor.Show();
    display.Flip();

    return returnValue;
}
