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

#include "agg_image.h"
#include "castle.h"
#include "cursor.h"
#include "game.h"
#include "game_interface.h"
#include "ground.h"
#include "icn.h"
#include "interface_radar.h"
#include "logging.h"
#include "text.h"
#include "world.h"

namespace
{
    int GetChunkSize( int size1, int size2 )
    {
        if ( size1 > size2 ) {
            const int res = size1 / size2;
            if ( ( size1 % size2 ) * 10 > size2 ) {
                return res + 1;
            }
            return res;
        }

        return 1;
    }

    enum
    {
        RADARCOLOR = 0xB5, // index palette
        COLOR_DESERT = 0x76,
        COLOR_SNOW = 0x0D,
        COLOR_SWAMP = 0x68,
        COLOR_WASTELAND = 0xCD,
        COLOR_BEACH = 0x29,
        COLOR_LAVA = 0x20,
        COLOR_DIRT = 0x36,
        COLOR_GRASS = 0x63,
        COLOR_WATER = 0x4D,
        COLOR_ROAD = 0x7A,

        COLOR_BLUE = 0x47,
        COLOR_GREEN = 0x67,
        COLOR_RED = 0xbd,
        COLOR_YELLOW = 0x70,
        COLOR_ORANGE = 0xcd,
        COLOR_PURPLE = 0x87,
        COLOR_GRAY = 0x10
    };
}

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
    , radarType( RadarType::WorldMap )
    , interface( basic )
    , hide( true )
{}

Interface::Radar::Radar( const Radar & radar )
    : BorderWindow( radar.area )
    , radarType( radar.radarType )
    , interface( radar.interface )
    , spriteArea( radar.spriteArea )
    , hide( radar.hide )
{}

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
    const int32_t worldWidth = world.w();
    const int32_t worldHeight = world.h();

    spriteArea.resize( worldWidth, worldHeight );
    spriteArea.reset();

    for ( int32_t yy = 0; yy < worldHeight; ++yy ) {
        for ( int32_t xx = 0; xx < worldWidth; ++xx ) {
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
        fheroes2::Size new_sz;

        if ( worldWidth < worldHeight ) {
            new_sz.width = ( worldWidth * size.h ) / worldHeight;
            new_sz.height = size.h;
            offset.x = ( size.w - new_sz.width ) / 2;
            offset.y = 0;
        }
        else if ( worldWidth > worldHeight ) {
            new_sz.width = size.w;
            new_sz.height = ( worldHeight * size.w ) / worldWidth;
            offset.x = 0;
            offset.y = ( size.h - new_sz.height ) / 2;
        }
        else {
            new_sz.width = size.w;
            new_sz.height = size.h;
        }

        fheroes2::Image resized( new_sz.width, new_sz.height );
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

void Interface::Radar::Redraw()
{
    const Settings & conf = Settings::Get();
    const bool hideInterface = conf.ExtGameHideInterface();

    if ( hideInterface && conf.ShowRadar() ) {
        BorderWindow::Redraw();
    }

    if ( !hideInterface || conf.ShowRadar() ) {
        fheroes2::Display & display = fheroes2::Display::instance();
        const Rect & rect = GetArea();
        if ( hide ) {
            fheroes2::Blit( fheroes2::AGG::GetICN( ( conf.ExtGameEvilInterface() ? ICN::HEROLOGE : ICN::HEROLOGO ), 0 ), display, rect.x, rect.y );
        }
        else {
            cursorArea.hide();
            fheroes2::Blit( spriteArea, display, rect.x + offset.x, rect.y + offset.y );
            RedrawObjects( Players::FriendColors(), ViewWorldMode::OnlyVisible );
            RedrawCursor();
        }
    }
}

void Interface::Radar::RedrawForViewWorld( const ViewWorld::ZoomROIs & roi, const ViewWorldMode mode )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    const Rect & rect = GetArea();
    cursorArea.hide();
    fheroes2::Blit( spriteArea, display, rect.x + offset.x, rect.y + offset.y );
    RedrawObjects( Players::FriendColors(), mode );
    const Rect roiInTiles = roi.GetROIinTiles();
    RedrawCursor( &roiInTiles );
}

void Interface::Radar::RedrawObjects( int color, ViewWorldMode flags ) const
{
    const bool revealAll = flags == ViewWorldMode::ViewAll;
    const bool revealMines = revealAll || ( flags == ViewWorldMode::ViewMines );
    const bool revealHeroes = revealAll || ( flags == ViewWorldMode::ViewHeroes );
    const bool revealTowns = revealAll || ( flags == ViewWorldMode::ViewTowns );
    const bool revealArtifacts = revealAll || ( flags == ViewWorldMode::ViewArtifacts );
    const bool revealResources = revealAll || ( flags == ViewWorldMode::ViewResources );

    const Rect & rect = GetArea();

    fheroes2::Display & display = fheroes2::Display::instance();

    const int32_t worldWidth = world.w();
    const int32_t worldHeight = world.h();
    const int areaw = ( offset.x ? rect.w - 2 * offset.x : rect.w );
    const int areah = ( offset.y ? rect.h - 2 * offset.y : rect.h );

    int stepx = worldWidth / rect.w;
    int stepy = worldHeight / rect.h;

    if ( 0 == stepx )
        stepx = 1;
    if ( 0 == stepy )
        stepy = 1;

    int sw = 0;

    if ( worldWidth >= worldHeight )
        sw = GetChunkSize( areaw, worldWidth );
    else
        sw = GetChunkSize( areah, worldHeight );

    const int32_t offsetX = rect.x + offset.x;
    const int32_t offsetY = rect.y + offset.y;

    for ( int32_t y = 0; y < worldHeight; y += stepy ) {
        const int dsty = offsetY + ( y * areah ) / worldHeight; // calculate once per row

        int tileIndex = y * worldWidth;
        for ( int32_t x = 0; x < worldWidth; x += stepx, tileIndex += stepx ) {
            const Maps::Tiles & tile = world.GetTiles( tileIndex );
#ifdef WITH_DEBUG
            const bool visibleTile = revealAll || IS_DEVEL() || !tile.isFog( color );
#else
            const bool visibleTile = revealAll || !tile.isFog( color );
#endif
            uint8_t fillColor = 0;

            switch ( tile.GetObject( revealHeroes ) ) {
            case MP2::OBJ_HEROES: {
                if ( visibleTile || revealHeroes ) {
                    const Heroes * hero = world.GetHeroes( tile.GetCenter() );
                    if ( hero )
                        fillColor = GetPaletteIndexFromColor( hero->GetColor() );
                }
            } break;

            case MP2::OBJ_CASTLE:
            case MP2::OBJN_CASTLE: {
                if ( visibleTile || revealTowns ) {
                    const Castle * castle = world.GetCastle( tile.GetCenter() );
                    if ( castle )
                        fillColor = GetPaletteIndexFromColor( castle->GetColor() );
                }
            } break;

            case MP2::OBJ_DRAGONCITY:
            case MP2::OBJ_LIGHTHOUSE:
            case MP2::OBJ_ALCHEMYLAB:
            case MP2::OBJ_MINES:
            case MP2::OBJ_SAWMILL:
                if ( visibleTile || revealMines ) {
                    fillColor = GetPaletteIndexFromColor( tile.QuantityColor() );
                }
                break;

            case MP2::OBJ_ARTIFACT:
                if ( visibleTile || revealArtifacts ) {
                    fillColor = COLOR_GRAY;
                }
                break;

            case MP2::OBJ_RESOURCE:
                if ( visibleTile || revealResources ) {
                    fillColor = COLOR_GRAY;
                }
                break;

            default:
                if ( visibleTile ) {
                    continue;
                }
            }

            const int dstx = offsetX + ( x * areaw ) / worldWidth;

            if ( sw > 1 ) {
                fheroes2::Fill( display, dstx, dsty, sw, sw, fillColor );
            }
            else {
                fheroes2::SetPixel( display, dstx, dsty, fillColor );
            }
        }
    }
}

// Redraw radar cursor. RoiRectangle is a rectangle in tile unit of the current radar view.
void Interface::Radar::RedrawCursor( const Rect * roiRectangle /* =nullptr */ )
{
    const Settings & conf = Settings::Get();

    if ( !conf.ExtGameHideInterface() || conf.ShowRadar() || radarType == RadarType::ViewWorld ) {
        if ( world.w() < 1 || world.h() < 1 )
            return;

        const Rect & rect = GetArea();
        const Rect & rectMaps = roiRectangle == nullptr ? interface.GetGameArea().GetVisibleTileROI() : *roiRectangle;

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

        const fheroes2::Size sz( ( width * areaw ) / world.w(), ( height * areah ) / world.h() );

        // check change game area
        if ( cursorArea.width() != sz.width || cursorArea.height() != sz.height ) {
            cursorArea.resize( sz.width, sz.height );
            cursorArea.reset();
            fheroes2::DrawBorder( cursorArea, RADARCOLOR, 6 );
        }

        cursorArea.setPosition( rect.x + offset.x + ( xStart * areaw ) / world.w(), rect.y + offset.y + ( yStart * areah ) / world.h() );
    }
}

void Interface::Radar::QueueEventProcessing( void )
{
    GameArea & gamearea = interface.GetGameArea();
    const Settings & conf = Settings::Get();
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
        else if ( le.MousePressRight( GetRect() ) )
            Dialog::Message( _( "World Map" ), _( "A miniature view of the known world. Left click to move viewing area." ), Font::BIG );
        else if ( conf.ExtGameHideInterface() ) {
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

bool Interface::Radar::QueueEventProcessingForWorldView( ViewWorld::ZoomROIs & roi )
{
    LocalEvent & le = LocalEvent::Get();
    const Rect & rect = GetArea();

    // move cursor
    if ( le.MouseCursor( rect ) ) {
        if ( le.MouseClickLeft() || le.MousePressLeft() ) {
            const fheroes2::Rect initROI = roi.GetROIinPixels();
            const fheroes2::Point prevCoordsTopLeft( initROI.x, initROI.y );
            const Point & pt = le.GetMouseCursor();

            if ( rect & pt ) {
                const fheroes2::Point newCoordsCenter( ( pt.x - rect.x ) * world.w() / rect.w, ( pt.y - rect.y ) * world.h() / rect.h );
                const fheroes2::Point newCoordsTopLeft( newCoordsCenter.x - initROI.width / 2, newCoordsCenter.y - initROI.height / 2 );

                if ( prevCoordsTopLeft != newCoordsTopLeft ) {
                    Cursor::Get().Hide();
                    return roi.ChangeCenter( fheroes2::Point( newCoordsCenter.x * TILEWIDTH, newCoordsCenter.y * TILEWIDTH ) );
                }
            }
        }
        else if ( le.MousePressRight( GetRect() ) ) {
            Dialog::Message( _( "World Map" ), _( "A miniature view of the known world. Left click to move viewing area." ), Font::BIG );
        }
        else if ( le.MouseWheelUp() ) {
            return roi.ChangeZoom( true );
        }
        else if ( le.MouseWheelDn() ) {
            return roi.ChangeZoom( false );
        }
    }
    return false;
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

// New Radar but copy mini-map data
Interface::Radar Interface::Radar::MakeRadarViewWorld( const Interface::Radar & radar )
{
    Radar newRadar( radar );
    newRadar.hide = false;
    newRadar.radarType = RadarType::ViewWorld;
    const fheroes2::Display & display = fheroes2::Display::instance();
    newRadar.area.x = display.width() - BORDERWIDTH - RADARWIDTH;
    newRadar.area.y = BORDERWIDTH;
    return newRadar;
}
