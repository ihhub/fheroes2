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

#include "interface_radar.h"

#include <algorithm>
#include <cassert>
#include <cstddef>

#include "agg_image.h"
#include "castle.h"
#include "color.h"
#include "dialog.h"
#include "game_interface.h"
#include "gamedefs.h"
#include "ground.h"
#include "heroes.h"
#include "icn.h"
#include "interface_gamearea.h"
#include "localevent.h"
#include "maps.h"
#include "maps_tiles.h"
#include "mp2.h"
#include "players.h"
#include "screen.h"
#ifdef WITH_DEBUG
#include "logging.h"
#endif
#include "settings.h"
#include "text.h"
#include "translations.h"
#include "world.h"

namespace
{
    enum : uint8_t
    {
        RADARCOLOR = 0xB5, // index palette
        COLOR_DESERT = 0x76,
        COLOR_SNOW = 0x0D,
        COLOR_SWAMP = 0x68,
        COLOR_WASTELAND = 0xCE,
        COLOR_BEACH = 0x29,
        COLOR_LAVA = 0x20,
        COLOR_DIRT = 0x36,
        COLOR_GRASS = 0x62,
        COLOR_WATER = 0x4D,
        COLOR_ROAD = 0x7A,

        COLOR_BLUE = 0x47,
        COLOR_GREEN = 0x5D,
        COLOR_RED = 0xbd,
        COLOR_YELLOW = 0x70,
        COLOR_ORANGE = 0xCA,
        COLOR_PURPLE = 0x87,
        COLOR_GRAY = 0x10,
        COLOR_WHITE = 0x0a,

        COLOR_BLACK = 0x00
    };

    uint8_t GetPaletteIndexFromGround( int ground )
    {
        switch ( ground ) {
        case Maps::Ground::DESERT:
            return COLOR_DESERT;
        case Maps::Ground::SNOW:
            return COLOR_SNOW;
        case Maps::Ground::SWAMP:
            return COLOR_SWAMP;
        case Maps::Ground::WASTELAND:
            return COLOR_WASTELAND;
        case Maps::Ground::BEACH:
            return COLOR_BEACH;
        case Maps::Ground::LAVA:
            return COLOR_LAVA;
        case Maps::Ground::DIRT:
            return COLOR_DIRT;
        case Maps::Ground::GRASS:
            return COLOR_GRASS;
        case Maps::Ground::WATER:
            return COLOR_WATER;
        default:
            break;
        }

        return COLOR_BLACK;
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

        return COLOR_WHITE;
    }

    bool getCastleColor( uint8_t & fillColor, const fheroes2::Point & position )
    {
        const Castle * castle = world.getCastle( position );
        if ( castle != nullptr ) {
            fillColor = GetPaletteIndexFromColor( castle->GetColor() );
            return true;
        }

        return false;
    }
}

Interface::Radar::Radar( Basic & basic )
    : BorderWindow( { 0, 0, RADARWIDTH, RADARWIDTH } )
    , _radarType( RadarType::WorldMap )
    , _interface( basic )
{
    // Radar image can not be transparent so we disable the transform layer to speed up rendering.
    _map._disableTransformLayer();
}

Interface::Radar::Radar( const Radar & radar, const fheroes2::Display & display )
    : BorderWindow( { display.width() - BORDERWIDTH - RADARWIDTH, BORDERWIDTH, RADARWIDTH, RADARWIDTH } )
    , _radarType( RadarType::ViewWorld )
    , _interface( radar._interface )
    , _roi( 0, 0, world.w(), world.h() )
    , _zoom( radar._zoom )
    , _hide( false )
{
    // Radar image can not be transparent so we disable the transform layer to speed up rendering.
    _map._disableTransformLayer();
}

void Interface::Radar::SavePosition()
{
    Settings & conf = Settings::Get();

    conf.SetPosRadar( GetRect().getPosition() );
    conf.Save( Settings::configFileName );
}

void Interface::Radar::SetPos( int32_t ox, int32_t oy )
{
    BorderWindow::SetPosition( ox, oy );
}

void Interface::Radar::Build()
{
    SetZoom();
    _roi = { 0, 0, world.w(), world.h() };
}

void Interface::Radar::SetZoom()
{
    const int32_t worldWidth = world.w();

    // Currently we have and support only square size maps.
    assert( worldWidth == world.h() );

    _zoom = static_cast<double>( area.width ) / worldWidth;

    // Currently we have and support only maps with 36 - 144 tiles width and height.
    assert( ( _zoom >= 1.0 ) && ( _zoom <= 4.0 ) );
}

void Interface::Radar::SetRedraw( const uint32_t redrawMode ) const
{
    // Only radar redraws are allowed here.
    assert( ( redrawMode & ~( REDRAW_RADAR_CURSOR | REDRAW_RADAR ) ) == 0 );

    _interface.SetRedraw( redrawMode );
}

void Interface::Radar::SetRenderArea( const fheroes2::Rect & roi )
{
    const Settings & conf = Settings::Get();
    // We set ROI only if radar is visible as there will be no render of radar map image if it is hidden.
    if ( !conf.isHideInterfaceEnabled() || conf.ShowRadar() ) {
        _roi.x = roi.x < 0 ? 0 : roi.x;
        _roi.y = roi.y < 0 ? 0 : roi.y;
        _roi.width = ( roi.width + roi.x ) > world.w() ? ( world.w() - _roi.x ) : roi.width;
        _roi.height = ( roi.height + roi.y ) > world.h() ? ( world.h() - _roi.y ) : roi.height;
    }
}

void Interface::Radar::Redraw( const bool redrawMapObjects )
{
    const Settings & conf = Settings::Get();
    if ( conf.isHideInterfaceEnabled() ) {
        if ( conf.ShowRadar() ) {
            BorderWindow::Redraw();
        }
        else {
            // We are in "Hide Interface" mode and radar is turned off so we have nothing to render.
            return;
        }
    }

    fheroes2::Display & display = fheroes2::Display::instance();
    const fheroes2::Rect & rect = GetArea();
    if ( _hide ) {
        fheroes2::Blit( fheroes2::AGG::GetICN( ( conf.isEvilInterfaceEnabled() ? ICN::HEROLOGE : ICN::HEROLOGO ), 0 ), display, rect.x, rect.y );
    }
    else {
        _cursorArea.hide();

        if ( redrawMapObjects ) {
            RedrawObjects( Players::FriendColors(), ViewWorldMode::OnlyVisible );
        }

        fheroes2::Copy( _map, 0, 0, display, rect.x, rect.y, _map.width(), _map.height() );

        _cursorArea.show();
        RedrawCursor();
    }
}

void Interface::Radar::RedrawForViewWorld( const ViewWorld::ZoomROIs & roi, const ViewWorldMode mode, const bool renderMapObjects )
{
    _cursorArea.hide();

    if ( renderMapObjects ) {
        RedrawObjects( Players::FriendColors(), mode );
        fheroes2::Display & display = fheroes2::Display::instance();
        const fheroes2::Rect & rect = GetArea();
        fheroes2::Copy( _map, 0, 0, display, rect.x, rect.y, _map.width(), _map.height() );
    }

    const fheroes2::Rect roiInTiles = roi.GetROIinTiles();
    _cursorArea.show();
    RedrawCursor( &roiInTiles );
}

void Interface::Radar::RedrawObjects( const int32_t playerColor, const ViewWorldMode flags )
{
#ifdef WITH_DEBUG
    const bool revealAll = ( flags == ViewWorldMode::ViewAll ) || IS_DEVEL();
#else
    const bool revealAll = flags == ViewWorldMode::ViewAll;
#endif

    uint8_t * radarImage = _map.image();

    assert( _roi.x >= 0 && _roi.y >= 0 && ( _roi.width + _roi.x ) <= world.w() && ( _roi.height + _roi.y ) <= world.h() );

    // Fill the radar map with black color ( 0 ) only if we are redrawing the entire map.
    if ( _roi.x == 0 && _roi.y == 0 && _roi.width == world.w() && _roi.height == world.h() ) {
        std::fill( radarImage, radarImage + static_cast<ptrdiff_t>( area.width ) * area.height, COLOR_BLACK );
    }

    const bool revealMines = revealAll || ( flags == ViewWorldMode::ViewMines );
    const bool revealHeroes = revealAll || ( flags == ViewWorldMode::ViewHeroes );
    const bool revealTowns = revealAll || ( flags == ViewWorldMode::ViewTowns );
    const bool revealArtifacts = revealAll || ( flags == ViewWorldMode::ViewArtifacts );
    const bool revealResources = revealAll || ( flags == ViewWorldMode::ViewResources );
    const bool revealOnlyVisible = revealAll || ( flags == ViewWorldMode::OnlyVisible );

    const int32_t radarWidth = _map.width();

    const bool isZoomIn = _zoom > 1.0;

    const int32_t maxRoiX = _roi.width + _roi.x;
    const int32_t maxRoiY = _roi.height + _roi.y;

    for ( int32_t y = _roi.y; y < maxRoiY; ++y ) {
        uint8_t * radarY = radarImage + static_cast<ptrdiff_t>( y * _zoom ) * radarWidth;
        const ptrdiff_t radarYStep = isZoomIn ? ( static_cast<ptrdiff_t>( ( y + 1 ) * _zoom ) * radarWidth ) : 0;

        for ( int32_t x = _roi.x; x < maxRoiX; ++x ) {
            const Maps::Tiles & tile = world.GetTiles( x, y );
            const bool visibleTile = revealAll || !tile.isFog( playerColor );

            uint8_t fillColor = 0;

            switch ( tile.GetObject( revealOnlyVisible || revealHeroes ) ) {
            case MP2::OBJ_HEROES: {
                if ( visibleTile || revealHeroes ) {
                    const Heroes * hero = world.GetHeroes( tile.GetCenter() );
                    if ( hero ) {
                        fillColor = GetPaletteIndexFromColor( hero->GetColor() );
                    }
                }
                break;
            }
            case MP2::OBJ_DRAGON_CITY:
            case MP2::OBJ_LIGHTHOUSE:
            case MP2::OBJ_ALCHEMIST_LAB:
            case MP2::OBJ_MINES:
            case MP2::OBJ_SAWMILL:
                // TODO: why Dragon City and Lighthouse are in this category? Verify the logic!
                if ( visibleTile || revealMines ) {
                    fillColor = GetPaletteIndexFromColor( tile.QuantityColor() );
                }
                break;
            case MP2::OBJ_NON_ACTION_DRAGON_CITY:
            case MP2::OBJ_NON_ACTION_LIGHTHOUSE:
            case MP2::OBJ_NON_ACTION_ALCHEMIST_LAB:
            case MP2::OBJ_NON_ACTION_MINES:
            case MP2::OBJ_NON_ACTION_SAWMILL:
                // TODO: why Dragon City and Lighthouse are in this category? Verify the logic!
                if ( visibleTile || revealMines ) {
                    const int32_t mainTileIndex = Maps::Tiles::getIndexOfMainTile( tile );
                    if ( mainTileIndex >= 0 ) {
                        fillColor = GetPaletteIndexFromColor( world.GetTiles( mainTileIndex ).QuantityColor() );
                    }
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
                // Castles and Towns can be partially covered by other non-action objects so we need to rely on special storage of castle's tiles.
                if ( visibleTile ) {
                    if ( !getCastleColor( fillColor, tile.GetCenter() ) ) {
                        // This is a visible tile and not covered by other objects, so fill it with the ground tile data.
                        if ( tile.isRoad() ) {
                            fillColor = COLOR_ROAD;
                        }
                        else {
                            fillColor = GetPaletteIndexFromGround( tile.GetGround() );

                            const MP2::MapObjectType objectType = tile.GetObject();
                            if ( objectType == MP2::OBJ_MOUNTAINS || objectType == MP2::OBJ_TREES ) {
                                fillColor += 3;
                            }
                        }
                    }
                }
                else {
                    if ( revealTowns ) {
                        getCastleColor( fillColor, tile.GetCenter() );
                    }
                    else {
                        // Non visible tile, we have already black radar so skip the render of this tile.
                        continue;
                    }
                }
            }

            uint8_t * radarX = radarY + static_cast<ptrdiff_t>( x * _zoom );
            if ( isZoomIn ) {
                const uint8_t * radarYEnd = radarImage + radarYStep + static_cast<ptrdiff_t>( x * _zoom );
                uint8_t * radarXEnd = radarY + static_cast<ptrdiff_t>( ( x + 1 ) * _zoom );

                for ( ; radarX != radarYEnd; radarX += radarWidth, radarXEnd += radarWidth ) {
                    std::fill( radarX, radarXEnd, fillColor );
                }
            }
            else {
                *radarX = fillColor;
            }
        }
    }

    // Reset ROI to full radar image to be able to redraw the mini-map without calling 'SetMapRedraw()'.
    _roi = { 0, 0, world.w(), world.h() };
}

// Redraw radar cursor. RoiRectangle is a rectangle in tile unit of the current radar view.
void Interface::Radar::RedrawCursor( const fheroes2::Rect * roiRectangle /* =nullptr */ )
{
    const Settings & conf = Settings::Get();
    if ( conf.isHideInterfaceEnabled() && !conf.ShowRadar() && _radarType != RadarType::ViewWorld ) {
        return;
    }

    const fheroes2::Rect worldSize{ 0, 0, world.w(), world.h() };
    if ( worldSize.width < 1 || worldSize.height < 1 ) {
        return;
    }

    const fheroes2::Rect & viewableWorldArea = ( roiRectangle == nullptr ) ? _interface.GetGameArea().GetVisibleTileROI() : *roiRectangle;

    if ( ( viewableWorldArea.width > worldSize.width ) && ( viewableWorldArea.height > worldSize.height ) ) {
        // We hide the cursor if the whole map is displayed.
        _cursorArea.resize( 0, 0 );
        _cursorArea.reset();
        _cursorArea.setPosition( 0, 0 );
    }
    else {
        const fheroes2::Rect radarWorldArea = worldSize ^ viewableWorldArea;

        const fheroes2::Rect & totalRenderingArea = GetArea();
        const fheroes2::Size actualRenderingArea{ totalRenderingArea.width, totalRenderingArea.height };

        const fheroes2::Size cursorSize{ ( radarWorldArea.width * actualRenderingArea.width ) / worldSize.width,
                                         ( radarWorldArea.height * actualRenderingArea.height ) / worldSize.height };

        if ( _cursorArea.width() != cursorSize.width || _cursorArea.height() != cursorSize.height ) {
            _cursorArea.resize( cursorSize.width, cursorSize.height );
            _cursorArea.reset();
            fheroes2::DrawBorder( _cursorArea, RADARCOLOR, 6 );
        }

        _cursorArea.setPosition( totalRenderingArea.x + ( radarWorldArea.x * actualRenderingArea.width ) / worldSize.width,
                                 totalRenderingArea.y + ( radarWorldArea.y * actualRenderingArea.height ) / worldSize.height );
    }
}

void Interface::Radar::QueueEventProcessing()
{
    const Settings & conf = Settings::Get();
    LocalEvent & le = LocalEvent::Get();
    const fheroes2::Rect & rect = GetArea();
    const fheroes2::Rect & borderArea = GetRect();

    if ( !le.MouseCursor( borderArea ) || le.MouseCursor( rect ) ) {
        _mouseDraggingMovement = false;
    }

    // Move border window
    if ( conf.ShowRadar() && BorderWindow::QueueEventProcessing() ) {
        _cursorArea.hide();
        _interface.SetRedraw( REDRAW_RADAR_CURSOR );
    }
    else if ( le.MouseCursor( rect ) ) {
        // move cursor
        if ( le.MouseClickLeft() || le.MousePressLeft() ) {
            _mouseDraggingMovement = true;
            const fheroes2::Point & pt = le.GetMouseCursor();

            if ( rect & pt ) {
                GameArea & gamearea = _interface.GetGameArea();
                fheroes2::Rect visibleROI( gamearea.GetVisibleTileROI() );
                const fheroes2::Point prev( visibleROI.x, visibleROI.y );
                gamearea.SetCenter( { ( pt.x - rect.x ) * world.w() / rect.width, ( pt.y - rect.y ) * world.h() / rect.height } );
                visibleROI = gamearea.GetVisibleTileROI();
                if ( prev.x != visibleROI.x || prev.y != visibleROI.y ) {
                    _interface.SetRedraw( REDRAW_RADAR_CURSOR );
                    gamearea.SetRedraw();
                }
            }
        }
        else if ( le.MousePressRight( GetRect() ) ) {
            Dialog::Message( _( "World Map" ), _( "A miniature view of the known world. Left click to move viewing area." ), Font::BIG );
        }
    }
}

bool Interface::Radar::QueueEventProcessingForWorldView( ViewWorld::ZoomROIs & roi ) const
{
    LocalEvent & le = LocalEvent::Get();
    const fheroes2::Rect & rect = GetArea();

    // move cursor
    if ( le.MouseCursor( rect ) ) {
        if ( le.MouseClickLeft() || le.MousePressLeft() ) {
            const fheroes2::Point & pt = le.GetMouseCursor();

            if ( rect & pt ) {
                const fheroes2::Rect & initROI = roi.GetROIinPixels();
                const fheroes2::Point prevCoordsTopLeft( initROI.x, initROI.y );
                const fheroes2::Point newCoordsCenter( ( pt.x - rect.x ) * world.w() / rect.width, ( pt.y - rect.y ) * world.h() / rect.height );
                const fheroes2::Point newCoordsTopLeft( newCoordsCenter.x - initROI.width / 2, newCoordsCenter.y - initROI.height / 2 );

                if ( prevCoordsTopLeft != newCoordsTopLeft ) {
                    return roi.ChangeCenter( { newCoordsCenter.x * TILEWIDTH - TILEWIDTH / 2, newCoordsCenter.y * TILEWIDTH - TILEWIDTH / 2 } );
                }
            }
        }
        else if ( le.MousePressRight( GetRect() ) ) {
            Dialog::Message( _( "World Map" ), _( "A miniature view of the known world. Left click to move viewing area." ), Font::BIG );
        }
        else if ( le.MouseWheelUp() ) {
            return roi.zoomIn( false );
        }
        else if ( le.MouseWheelDn() ) {
            return roi.zoomOut( false );
        }
    }
    return false;
}
