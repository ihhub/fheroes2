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

#include <cmath>

#include "agg.h"
#include "castle.h"
#include "cursor.h"
#include "game.h"
#include "game_interface.h"
#include "ground.h"
#include "interface_radar.h"
#include "settings.h"
#include "world.h"

#define RADARCOLOR 0xB5 // index palette
#define COLOR_DESERT 0x76
#define COLOR_SNOW 0x0D
#define COLOR_SWAMP 0x68
#define COLOR_WASTELAND 0xCD
#define COLOR_BEACH 0x29
#define COLOR_LAVA 0x20
#define COLOR_DIRT 0x36
#define COLOR_GRASS 0x63
#define COLOR_WATER 0x4D
#define COLOR_ROAD 0x7A

#define COLOR_BLUE 0x47
#define COLOR_GREEN 0x67
#define COLOR_RED 0xbd
#define COLOR_YELLOW 0x70
#define COLOR_ORANGE 0xcd
#define COLOR_PURPLE 0x87
#define COLOR_GRAY 0x10

uint8_t GetPaletteIndexFromGround( int ground )
{
    switch ( ground ) {
    case Maps::Ground::DESERT:
        return ( COLOR_DESERT );
    case Maps::Ground::SNOW:
        return ( COLOR_SNOW );
    case Maps::Ground::SWAMP:
        return ( COLOR_SWAMP );
    case Maps::Ground::WASTELAND:
        return ( COLOR_WASTELAND );
    case Maps::Ground::BEACH:
        return ( COLOR_BEACH );
    case Maps::Ground::LAVA:
        return ( COLOR_LAVA );
    case Maps::Ground::DIRT:
        return ( COLOR_DIRT );
    case Maps::Ground::GRASS:
        return ( COLOR_GRASS );
    case Maps::Ground::WATER:
        return ( COLOR_WATER );
    default:
        break;
    }

    return 0;
}

uint8_t GetPaletteIndexFromColor( int color )
{
    switch ( color ) {
    case Color::BLUE:
        return COLOR_BLUE;
    case Color::GREEN:
        return COLOR_GREEN;
    case Color::RED:
        return COLOR_RED;
    case Color::YELLOW:
        return COLOR_YELLOW;
    case Color::ORANGE:
        return COLOR_ORANGE;
    case Color::PURPLE:
        return COLOR_PURPLE;
    default:
        break;
    }

    return COLOR_GRAY;
}

/* constructor */
Interface::Radar::Radar( Basic & basic )
    : BorderWindow( Rect( 0, 0, RADARWIDTH, RADARWIDTH ) )
    , interface( basic )
    , hide( true )
{
    if ( Settings::Get().QVGA() ) {
        // for QVGA set small radar, 1 pixel = 1 tile
        if ( RADARWIDTH > world.w() && RADARWIDTH > world.h() )
            SetPosition( 0, 0, world.w(), world.h() );
    }
}

void Interface::Radar::SavePosition( void )
{
    Settings::Get().SetPosRadar( GetRect() );
}

void Interface::Radar::SetPos( s32 ox, s32 oy )
{
    BorderWindow::SetPosition( ox, oy );
}

/* construct gui */
void Interface::Radar::Build( void )
{
    Generate();
    RedrawCursor();
}

/* generate mini maps */
void Interface::Radar::Generate( void )
{
    const Size & size = GetArea();
    const s32 world_w = world.w();
    const s32 world_h = world.h();

    spriteArea.resize( world_w, world_h );
    spriteArea.reset();

    for ( s32 yy = 0; yy < world_h; ++yy ) {
        for ( s32 xx = 0; xx < world_w; ++xx ) {
            const Maps::Tiles & tile = world.GetTiles( xx, yy );
            uint8_t color = 0;

            if ( tile.isRoad() )
                color = COLOR_ROAD;
            else {
                color = GetPaletteIndexFromGround( tile.GetGround() );

                const int mapObject = tile.GetObject();
                if ( mapObject == MP2::OBJ_MOUNTS || mapObject == MP2::OBJ_TREES )
                    color += 3;
            }

            fheroes2::SetPixel( spriteArea, xx, yy, color );
        }
    }

    if ( spriteArea.width() != size.w || spriteArea.height() != size.h ) {
        Size new_sz;

        if ( world_w < world_h ) {
            new_sz.w = ( world_w * size.h ) / world_h;
            new_sz.h = size.h;
            offset.x = ( size.w - new_sz.w ) / 2;
            offset.y = 0;
        }
        else if ( world_w > world_h ) {
            new_sz.w = size.w;
            new_sz.h = ( world_h * size.w ) / world_w;
            offset.x = 0;
            offset.y = ( size.h - new_sz.h ) / 2;
        }
        else {
            new_sz.w = size.w;
            new_sz.h = size.h;
        }

        fheroes2::Image resized( new_sz.w, new_sz.h );
        fheroes2::Resize( spriteArea, resized );
        spriteArea = resized;
    }
}

void Interface::Radar::SetHide( bool f )
{
    hide = f;
}

void Interface::Radar::SetRedraw( void ) const
{
    interface.SetRedraw( REDRAW_RADAR );
}

void Interface::Radar::Redraw( void )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    const Settings & conf = Settings::Get();
    const Rect & rect = GetArea();

    if ( conf.ExtGameHideInterface() && conf.ShowRadar() ) {
        BorderWindow::Redraw();
    }

    if ( !conf.ExtGameHideInterface() || conf.ShowRadar() ) {
        if ( hide )
            fheroes2::Blit( fheroes2::AGG::GetICN( ( conf.ExtGameEvilInterface() ? ICN::HEROLOGE : ICN::HEROLOGO ), 0 ), display, rect.x, rect.y );
        else {
            cursorArea.hide();
            fheroes2::Blit( spriteArea, display, rect.x + offset.x, rect.y + offset.y );
            RedrawObjects( Players::FriendColors() );
            RedrawCursor();
        }
    }
}

int GetChunkSize( int size1, int size2 )
{
    int res = 1;
    if ( size1 > size2 ) {
        res = size1 / size2;
        if ( ( size1 % size2 ) * 10 > size2 )
            res += 1;
    }

    return res;
}

/* redraw radar area for color */
void Interface::Radar::RedrawObjects( int color )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    const Rect & rect = GetArea();
    const s32 world_w = world.w();
    const s32 world_h = world.h();
    const int areaw = ( offset.x ? rect.w - 2 * offset.x : rect.w );
    const int areah = ( offset.y ? rect.h - 2 * offset.y : rect.h );

    int stepx = world_w / rect.w;
    int stepy = world_h / rect.h;

    if ( 0 == stepx )
        stepx = 1;
    if ( 0 == stepy )
        stepy = 1;

    int sw = 0;

    if ( world_w >= world_h )
        sw = GetChunkSize( areaw, world_w );
    else
        sw = GetChunkSize( areah, world_h );

    fheroes2::Image sf( sw, sw );

    for ( s32 yy = 0; yy < world_h; yy += stepy ) {
        for ( s32 xx = 0; xx < world_w; xx += stepx ) {
            const Maps::Tiles & tile = world.GetTiles( xx, yy );
#ifdef WITH_DEBUG
            bool show_tile = IS_DEVEL() || !tile.isFog( color );
#else
            const bool & show_tile = !tile.isFog( color );
#endif
            uint8_t fillColor = 0;

            if ( show_tile ) {
                switch ( tile.GetObject() ) {
                case MP2::OBJ_HEROES: {
                    const Heroes * hero = world.GetHeroes( tile.GetCenter() );
                    if ( hero )
                        fillColor = GetPaletteIndexFromColor( hero->GetColor() );
                } break;

                case MP2::OBJ_CASTLE:
                case MP2::OBJN_CASTLE: {
                    const Castle * castle = world.GetCastle( tile.GetCenter() );
                    if ( castle )
                        fillColor = GetPaletteIndexFromColor( castle->GetColor() );
                } break;

                case MP2::OBJ_DRAGONCITY:
                case MP2::OBJ_LIGHTHOUSE:
                case MP2::OBJ_ALCHEMYLAB:
                case MP2::OBJ_MINES:
                case MP2::OBJ_SAWMILL:
                    fillColor = GetPaletteIndexFromColor( tile.QuantityColor() );
                    break;

                default:
                    continue;
                }
            }

            const int dstx = rect.x + offset.x + ( xx * areaw ) / world_w;
            const int dsty = rect.y + offset.y + ( yy * areah ) / world_h;

            if ( sw > 1 ) {
                sf.fill( fillColor );
                fheroes2::Blit( sf, display, dstx, dsty );
            }
            else {
                if ( dstx < display.width() && dsty < display.height() )
                    fheroes2::SetPixel( display, dstx, dsty, fillColor );
            }
        }
    }
}

/* redraw radar cursor */
void Interface::Radar::RedrawCursor( void )
{
    const Settings & conf = Settings::Get();

    if ( !conf.ExtGameHideInterface() || conf.ShowRadar() ) {
        if ( world.w() < 1 || world.h() < 1 )
            return;

        const Rect & rect = GetArea();
        const Rect & rectMaps = interface.GetGameArea().GetVisibleTileROI();

        s32 areaw = ( offset.x ? rect.w - 2 * offset.x : rect.w );
        s32 areah = ( offset.y ? rect.h - 2 * offset.y : rect.h );

        int32_t xStart = rectMaps.x;
        int32_t xEnd = rectMaps.x + rectMaps.w;
        int32_t yStart = rectMaps.y;
        int32_t yEnd = rectMaps.y + rectMaps.h;
        if ( xStart < 0 )
            xStart = 0;
        if ( yStart < 0 )
            yStart = 0;
        if ( xEnd >= world.w() )
            xEnd = world.w();
        if ( yEnd >= world.h() )
            yEnd = world.h();

        const uint16_t width = static_cast<uint16_t>( xEnd - xStart );
        const uint16_t height = static_cast<uint16_t>( yEnd - yStart );

        const Size sz( ( width * areaw ) / world.w(), ( height * areah ) / world.h() );

        // check change game area
        if ( cursorArea.width() != sz.w || cursorArea.height() != sz.h ) {
            cursorArea.resize( sz.w, sz.h );
            cursorArea.reset();
            fheroes2::DrawBorder( cursorArea, RADARCOLOR, 6 );
        }

        cursorArea.setPosition( rect.x + offset.x + ( xStart * areaw ) / world.w(), rect.y + offset.y + ( yStart * areah ) / world.h() );
    }
}

void Interface::Radar::QueueEventProcessing( void )
{
    GameArea & gamearea = interface.GetGameArea();
    Settings & conf = Settings::Get();
    LocalEvent & le = LocalEvent::Get();
    const Rect & rect = GetArea();

    // move border
    if ( conf.ShowRadar() && BorderWindow::QueueEventProcessing() ) {
        RedrawCursor();
    }
    else
        // move cursor
        if ( le.MouseCursor( rect ) ) {
        if ( le.MouseClickLeft() || le.MousePressLeft() ) {
            const Point prev( gamearea.GetVisibleTileROI() );
            const Point & pt = le.GetMouseCursor();

            if ( rect & pt ) {
                gamearea.SetCenter( Point( ( pt.x - rect.x ) * world.w() / rect.w, ( pt.y - rect.y ) * world.h() / rect.h ) );

                if ( prev != gamearea.GetVisibleTileROI() ) {
                    Cursor::Get().Hide();
                    RedrawCursor();
                    gamearea.SetRedraw();
                }
            }
        }
        else if ( !conf.ExtPocketTapMode() && le.MousePressRight( GetRect() ) )
            Dialog::Message( _( "World Map" ), _( "A miniature view of the known world. Left click to move viewing area." ), Font::BIG );
        else if ( !conf.QVGA() && conf.ExtGameHideInterface() ) {
            Size newSize( rect.w, rect.h );

            if ( le.MouseWheelUp() ) {
                if ( rect.w != world.w() || rect.h != world.h() )
                    newSize = Size( world.w(), world.h() );
            }
            else if ( le.MouseWheelDn() ) {
                if ( rect.w != RADARWIDTH || rect.h != RADARWIDTH )
                    newSize = Size( RADARWIDTH, RADARWIDTH );
            }

            ChangeAreaSize( newSize );
        }
    }
}

void Interface::Radar::ResetAreaSize( void )
{
    ChangeAreaSize( Size( RADARWIDTH, RADARWIDTH ) );
}

void Interface::Radar::ChangeAreaSize( const Size & newSize )
{
    if ( newSize != area ) {
        const Rect & rect = GetRect();
        Cursor::Get().Hide();
        SetPosition( rect.x < 0 ? 0 : rect.x, rect.y < 0 ? 0 : rect.y, newSize.w, newSize.h );
        Generate();
        RedrawCursor();
        interface.GetGameArea().SetRedraw();
    }
}
