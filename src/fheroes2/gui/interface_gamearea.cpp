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

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <deque>
#include <list>
#include <map>
#include <ostream>
#include <type_traits>

#include "agg_image.h"
#include "castle.h"
#include "cursor.h"
#include "game_delays.h"
#include "game_interface.h"
#include "gamedefs.h"
#include "ground.h"
#include "heroes.h"
#include "icn.h"
#include "interface_cpanel.h"
#include "interface_gamearea.h"
#include "localevent.h"
#include "logging.h"
#include "maps.h"
#include "maps_tiles.h"
#include "pal.h"
#include "players.h"
#include "route.h"
#include "screen.h"
#include "settings.h"
#include "skill.h"
#include "ui_object_rendering.h"
#include "world.h"

namespace
{
    const int32_t minimalRequiredDraggingMovement = 10;

    struct TileUnfitRenderObjectInfo
    {
        std::map<fheroes2::Point, std::deque<fheroes2::ObjectRenderingInfo>> bottomImages;
        std::map<fheroes2::Point, std::deque<fheroes2::ObjectRenderingInfo>> bottomBackgroundImages;
        std::map<fheroes2::Point, std::deque<fheroes2::ObjectRenderingInfo>> topImages;

        std::map<fheroes2::Point, std::deque<fheroes2::ObjectRenderingInfo>> lowPriorityBottomImages;
        std::map<fheroes2::Point, std::deque<fheroes2::ObjectRenderingInfo>> highPriorityBottomImages;

        std::map<fheroes2::Point, std::deque<fheroes2::ObjectRenderingInfo>> shadowImages;
    };

    void populateStaticTileUnfitObjectInfo( TileUnfitRenderObjectInfo & tileUnfit, std::vector<fheroes2::ObjectRenderingInfo> & imageInfo,
                                            std::vector<fheroes2::ObjectRenderingInfo> & shadowInfo, const fheroes2::Point & offset, const uint8_t alphaValue )
    {
        for ( auto & objectInfo : imageInfo ) {
            const fheroes2::Point imagePos = objectInfo.tileOffset;
            objectInfo.alphaValue = alphaValue;

            if ( imagePos.y > 0 ) {
                if ( imagePos.x < 0 ) {
                    tileUnfit.bottomBackgroundImages[imagePos + offset].emplace_front( std::move( objectInfo ) );
                }
                else {
                    tileUnfit.bottomBackgroundImages[imagePos + offset].emplace_back( std::move( objectInfo ) );
                }
            }
            else if ( imagePos.y == 0 ) {
                if ( imagePos.x < 0 ) {
                    tileUnfit.bottomImages[imagePos + offset].emplace_front( std::move( objectInfo ) );
                }
                else {
                    tileUnfit.bottomImages[imagePos + offset].emplace_back( std::move( objectInfo ) );
                }
            }
            else {
                if ( imagePos.x < 0 ) {
                    tileUnfit.topImages[imagePos + offset].emplace_front( std::move( objectInfo ) );
                }
                else {
                    tileUnfit.topImages[imagePos + offset].emplace_back( std::move( objectInfo ) );
                }
            }
        }

        // Static object's shadows are always on the same layer.
        for ( auto & objectInfo : shadowInfo ) {
            const fheroes2::Point imagePos = objectInfo.tileOffset;
            objectInfo.alphaValue = alphaValue;

            tileUnfit.shadowImages[imagePos + offset].emplace_back( std::move( objectInfo ) );
        }
    }

    void populateStaticTileUnfitBackgroundObjectInfo( TileUnfitRenderObjectInfo & tileUnfit, std::vector<fheroes2::ObjectRenderingInfo> & imageInfo,
                                                      const fheroes2::Point & offset, const uint8_t alphaValue )
    {
        for ( auto & objectInfo : imageInfo ) {
            const fheroes2::Point imagePos = objectInfo.tileOffset;
            objectInfo.alphaValue = alphaValue;

            if ( imagePos.y > 0 ) {
                tileUnfit.bottomBackgroundImages[imagePos + offset].emplace_front( std::move( objectInfo ) );
            }
            else if ( imagePos.y == 0 ) {
                tileUnfit.bottomImages[imagePos + offset].emplace_front( std::move( objectInfo ) );
            }
            else {
                tileUnfit.topImages[imagePos + offset].emplace_front( std::move( objectInfo ) );
            }
        }
    }

    void populateHeroObjectInfo( TileUnfitRenderObjectInfo & tileUnfit, const Heroes * hero )
    {
        assert( hero != nullptr );

        const fheroes2::Point & heroPos = hero->GetCenter();
        fheroes2::Point nextHeroPos = heroPos;

        const bool movingHero = hero->isMoveEnabled();
        if ( movingHero ) {
            const Route::Path & path = hero->GetPath();
            assert( !path.empty() );

            nextHeroPos = Maps::GetPoint( Maps::GetDirectionIndex( hero->GetIndex(), path.GetFrontDirection() ) );
        }

        // A castle's road south from a castle should actually be level 3 but it is level 2 causing a hero's horse legs to be truncated.
        // In order to render the legs properly we need to make the bottom part of the hero's sprite to be rendered after castle's road.
        // This happens only when a hero stands in a castle.
        const Castle * castle = world.getCastleEntrance( heroPos );
        const bool isHeroInCastle = ( castle != nullptr && castle->GetCenter() == heroPos );

        const uint8_t heroAlphaValue = hero->getAlphaValue();

        auto spriteInfo = hero->getHeroSpritesPerTile();
        auto spriteShadowInfo = hero->getHeroShadowSpritesPerTile();

        for ( auto & objectInfo : spriteInfo ) {
            const fheroes2::Point imagePos = objectInfo.tileOffset;
            objectInfo.alphaValue = heroAlphaValue;

            if ( movingHero && imagePos.y == 0 ) {
                if ( nextHeroPos.y > heroPos.y && nextHeroPos.x > heroPos.x && imagePos.x > 0 ) {
                    // The hero moves south-east. We need to render it over everything.
                    tileUnfit.highPriorityBottomImages[imagePos + heroPos].emplace_back( std::move( objectInfo ) );
                    continue;
                }

                if ( nextHeroPos.y > heroPos.y && nextHeroPos.x < heroPos.x && imagePos.x < 0 ) {
                    // The hero moves south-west. We need to render it over everything.
                    tileUnfit.highPriorityBottomImages[imagePos + heroPos].emplace_back( std::move( objectInfo ) );
                    continue;
                }

                if ( nextHeroPos.y < heroPos.y && nextHeroPos.x < heroPos.x && imagePos.x < 0 ) {
                    // The hero moves north-west. We need to render it under all other objects.
                    tileUnfit.lowPriorityBottomImages[imagePos + heroPos].emplace_back( std::move( objectInfo ) );
                    continue;
                }

                if ( nextHeroPos.y < heroPos.y && nextHeroPos.x > heroPos.x && imagePos.x > 0 ) {
                    // The hero moves north-east. We need to render it under all other objects.
                    tileUnfit.lowPriorityBottomImages[imagePos + heroPos].emplace_back( std::move( objectInfo ) );
                    continue;
                }
            }

            if ( movingHero && imagePos.y == 1 ) {
                if ( nextHeroPos.y > heroPos.y && nextHeroPos.x > heroPos.x && imagePos.x > 0 ) {
                    // The hero moves south-east. We need to render it over everything.
                    tileUnfit.bottomImages[imagePos + heroPos].emplace_back( std::move( objectInfo ) );
                    continue;
                }

                if ( nextHeroPos.y > heroPos.y && nextHeroPos.x < heroPos.x && imagePos.x < 0 ) {
                    // The hero moves south-west. We need to render it over everything.
                    tileUnfit.bottomImages[imagePos + heroPos].emplace_back( std::move( objectInfo ) );
                    continue;
                }
            }

            if ( movingHero && imagePos.y == -1 ) {
                if ( nextHeroPos.y < heroPos.y && nextHeroPos.x < heroPos.x && imagePos.x < 0 ) {
                    // The hero moves north-west. We need to render it under all other objects.
                    tileUnfit.bottomImages[imagePos + heroPos].emplace_back( std::move( objectInfo ) );
                    continue;
                }

                if ( nextHeroPos.y < heroPos.y && nextHeroPos.x > heroPos.x && imagePos.x > 0 ) {
                    // The hero moves north-east. We need to render it under all other objects.
                    tileUnfit.bottomImages[imagePos + heroPos].emplace_back( std::move( objectInfo ) );
                    continue;
                }
            }

            if ( imagePos.y > 0 && !isHeroInCastle ) {
                if ( imagePos.x < 0 ) {
                    tileUnfit.bottomBackgroundImages[imagePos + heroPos].emplace_front( std::move( objectInfo ) );
                }
                else {
                    tileUnfit.bottomBackgroundImages[imagePos + heroPos].emplace_back( std::move( objectInfo ) );
                }
            }
            else if ( imagePos.y == 0 || ( isHeroInCastle && imagePos.y > 0 ) ) {
                if ( imagePos.x < 0 ) {
                    tileUnfit.bottomImages[imagePos + heroPos].emplace_front( std::move( objectInfo ) );
                }
                else {
                    tileUnfit.bottomImages[imagePos + heroPos].emplace_back( std::move( objectInfo ) );
                }
            }
            else {
                if ( imagePos.x < 0 ) {
                    tileUnfit.topImages[imagePos + heroPos].emplace_front( std::move( objectInfo ) );
                }
                else {
                    tileUnfit.topImages[imagePos + heroPos].emplace_back( std::move( objectInfo ) );
                }
            }
        }

        for ( auto & objectInfo : spriteShadowInfo ) {
            const fheroes2::Point imagePos = objectInfo.tileOffset;
            objectInfo.alphaValue = heroAlphaValue;

            tileUnfit.shadowImages[imagePos + heroPos].emplace_back( std::move( objectInfo ) );
        }
    }

    void renderImagesOnTile( fheroes2::Image & output, const std::map<fheroes2::Point, std::deque<fheroes2::ObjectRenderingInfo>> & images,
                             const fheroes2::Point & offset, const Interface::GameArea & area )
    {
        auto iter = images.find( offset );
        if ( iter == images.end() ) {
            return;
        }

        assert( !iter->second.empty() );

        for ( auto & info : iter->second ) {
            area.BlitOnTile( output, fheroes2::AGG::GetICN( info.icnId, info.icnIndex ), info.area, info.imageOffset.x, info.imageOffset.y, offset, info.isFlipped,
                             info.alphaValue );
        }
    }

    void renderOutOfMapTile( const TileUnfitRenderObjectInfo & tileUnfit, fheroes2::Image & output, const fheroes2::Point & offset, const Interface::GameArea & area )
    {
        renderImagesOnTile( output, tileUnfit.bottomBackgroundImages, offset, area );
        renderImagesOnTile( output, tileUnfit.lowPriorityBottomImages, offset, area );
        renderImagesOnTile( output, tileUnfit.bottomImages, offset, area );
        renderImagesOnTile( output, tileUnfit.highPriorityBottomImages, offset, area );
        renderImagesOnTile( output, tileUnfit.topImages, offset, area );
    }

    bool isTallTopLayerObject( const int32_t x, const int32_t y, const uint32_t uid )
    {
        if ( y + 1 >= world.h() ) {
            // There is nothing below so it's not a tall object.
            return false;
        }

        // There is a tile below the current.
        const Maps::Tiles & tileBelow = world.GetTiles( x, y + 1 );
        const Maps::Addons & lowerTileAddons = tileBelow.getLevel2Addons();

        for ( const Maps::TilesAddon & lowerAddon : lowerTileAddons ) {
            if ( lowerAddon.uniq == uid ) {
                // This is a tall object.
                return true;
            }
        }

        return false;
    }
}

Interface::GameArea::GameArea( Basic & basic )
    : interface( basic )
    , _minLeftOffset( 0 )
    , _maxLeftOffset( 0 )
    , _minTopOffset( 0 )
    , _maxTopOffset( 0 )
    , _prevIndexPos( 0 )
    , scrollDirection( 0 )
    , updateCursor( false )
    , _mouseDraggingInitiated( false )
    , _mouseDraggingMovement( false )
{
    // Do nothing.
}

void Interface::GameArea::generate( const fheroes2::Size & screenSize, const bool withoutBorders )
{
    if ( withoutBorders )
        SetAreaPosition( 0, 0, screenSize.width, screenSize.height );
    else
        SetAreaPosition( BORDERWIDTH, BORDERWIDTH, screenSize.width - RADARWIDTH - 3 * BORDERWIDTH, screenSize.height - 2 * BORDERWIDTH );
}

void Interface::GameArea::SetAreaPosition( int32_t x, int32_t y, int32_t w, int32_t h )
{
    _windowROI = { x, y, w, h };
    const fheroes2::Size worldSize( world.w() * TILEWIDTH, world.h() * TILEWIDTH );

    if ( worldSize.width > w ) {
        _minLeftOffset = -( w / 2 ) - TILEWIDTH / 2;
        _maxLeftOffset = worldSize.width - w / 2;
    }
    else {
        _minLeftOffset = -( w - worldSize.width ) / 2;
        _maxLeftOffset = _minLeftOffset;
    }

    if ( worldSize.height > h ) {
        _minTopOffset = -( h / 2 ) - TILEWIDTH / 2;
        _maxTopOffset = worldSize.height - h / 2;
    }
    else {
        _minTopOffset = -( h - worldSize.height ) / 2;
        _maxTopOffset = _minTopOffset;
    }

    // adding 1 extra tile for both axes in case of drawing tiles partially near sides
    _visibleTileCount = { ( w + TILEWIDTH - 1 ) / TILEWIDTH + 1, ( h + TILEWIDTH - 1 ) / TILEWIDTH + 1 };

    _setCenterToTile( fheroes2::Point( world.w() / 2, world.h() / 2 ) );
}

void Interface::GameArea::BlitOnTile( fheroes2::Image & dst, const fheroes2::Image & src, int32_t ox, int32_t oy, const fheroes2::Point & mp, bool flip,
                                      uint8_t alpha ) const
{
    const fheroes2::Point tileOffset = GetRelativeTilePosition( mp );

    const fheroes2::Rect imageRoi{ tileOffset.x + ox, tileOffset.y + oy, src.width(), src.height() };
    const fheroes2::Rect overlappedRoi = _windowROI ^ imageRoi;

    fheroes2::AlphaBlit( src, overlappedRoi.x - imageRoi.x, overlappedRoi.y - imageRoi.y, dst, overlappedRoi.x, overlappedRoi.y, overlappedRoi.width,
                         overlappedRoi.height, alpha, flip );
}

void Interface::GameArea::BlitOnTile( fheroes2::Image & dst, const fheroes2::Image & src, const fheroes2::Rect & srcRoi, int32_t ox, int32_t oy,
                                        const fheroes2::Point & mp, bool flip, uint8_t alpha ) const
{
    const fheroes2::Point tileOffset = GetRelativeTilePosition( mp );

    const fheroes2::Rect imageRoi{ tileOffset.x + ox, tileOffset.y + oy, srcRoi.width, srcRoi.height };
    const fheroes2::Rect overlappedRoi = _windowROI ^ imageRoi;

    fheroes2::AlphaBlit( src, srcRoi.x + overlappedRoi.x - imageRoi.x, srcRoi.y + overlappedRoi.y - imageRoi.y, dst, overlappedRoi.x, overlappedRoi.y, overlappedRoi.width,
                         overlappedRoi.height, alpha, flip );
}

void Interface::GameArea::DrawTile( fheroes2::Image & dst, const fheroes2::Image & src, const fheroes2::Point & mp ) const
{
    const fheroes2::Point tileOffset = GetRelativeTilePosition( mp );

    const fheroes2::Rect imageRoi{ tileOffset.x, tileOffset.y, src.width(), src.height() };
    const fheroes2::Rect overlappedRoi = _windowROI ^ imageRoi;

    fheroes2::Copy( src, overlappedRoi.x - imageRoi.x, overlappedRoi.y - imageRoi.y, dst, overlappedRoi.x, overlappedRoi.y, overlappedRoi.width, overlappedRoi.height );
}

void Interface::GameArea::Redraw( fheroes2::Image & dst, int flag, bool isPuzzleDraw ) const
{
    const fheroes2::Rect & tileROI = GetVisibleTileROI();

    int32_t minX = tileROI.x;
    int32_t minY = tileROI.y;
    int32_t maxX = tileROI.x + tileROI.width;
    int32_t maxY = tileROI.y + tileROI.height;

    // Render terrain.
    for ( int32_t y = 0; y < tileROI.height; ++y ) {
        fheroes2::Point offset( tileROI.x, tileROI.y + y );

        if ( offset.y < 0 || offset.y >= world.h() ) {
            for ( ; offset.x < maxX; ++offset.x ) {
                Maps::Tiles::RedrawEmptyTile( dst, offset, *this );
            }
        }
        else {
            for ( ; offset.x < maxX; ++offset.x ) {
                if ( offset.x < 0 || offset.x >= world.w() ) {
                    Maps::Tiles::RedrawEmptyTile( dst, offset, *this );
                }
                else {
                    DrawTile( dst, world.GetTiles( offset.x, offset.y ).GetTileSurface(), offset );
                }
            }
        }
    }

    minX = std::max( minX, 0 );
    minY = std::max( minY, 0 );
    maxX = std::min( maxX, world.w() );
    maxY = std::min( maxY, world.h() );

    if ( minX >= maxX || minY >= maxY ) {
        // This can't be true! Please check your code changes as we shouldn't have an empty area.
        assert( 0 );
        return;
    }

    // Each tile can contain multiple object parts or sprites. Each object part has its own level or in other words layer of rendering.
    // We need to use a correct order of levels to render objects on tiles. The levels are:
    // 0 - main and action objects like mines, forest, castle and etc.
    // 1 - background objects like lakes or bushes.
    // 2 - shadows and some special objects like castle's entrance road.
    // 3 - roads, water flaws and cracks. Essentially everything what is a part of terrain.
    // The correct order of levels is 3 --> 1 --> 2 --> 0.
    //
    // There are also two groups of objects: ground objects (bottom layer) and high objects (top layer). High objects are the parts of the objects which are taller than
    // 1 tile. For example, a castle. All ground objects are drawn first.
    //
    // However, there are some objects which appear to be more than 1 tile (32 x 32 pixels) size such as heroes, monsters and boats.
    // To render all these 'special' objects we need to create a copy of object sprite stacks for each tile, add temporary extra sprites and render them.
    // Let's call these objects as tile-unfit objects, the rest of objects will be called tile-fit objects.

    // Fading animation can be applied as for tile-fit and tile-unfit objects.
    // In case of tile-fit objects we need to pass UID of the object and alpha values.
    // In case of tile-unfit objects we need to pass tile ID and set alpha value while creating RenderObjectInfo instances.

    const bool drawHeroes = ( flag & LEVEL_HEROES ) == LEVEL_HEROES;

#ifdef WITH_DEBUG
    const bool drawFog = ( ( flag & LEVEL_FOG ) == LEVEL_FOG ) && !IS_DEVEL();
#else
    const bool drawFog = ( flag & LEVEL_FOG ) == LEVEL_FOG;
#endif

    const int friendColors = Players::FriendColors();

    TileUnfitRenderObjectInfo tileUnfit;

    const Heroes * currentHero = drawHeroes ? GetFocusHeroes() : nullptr;

    // TODO: Dragon City with tilset 164 (OBJNMUL2.ICN) and object index 46 is a bottom layer sprite.
    // TODO: When a hero standing besides this turns a part of the hero is visible. This can be fixed only by some hack.

    // Run through all visible tiles and find all tile-unfit objects.
    // Also cover extra tiles from right and bottom sides because these objects are usually bigger than 1x1 tiles.
    const int32_t roiToRenderMinX = std::max( minX - 1, 0 );
    const int32_t roiToRenderMinY = std::max( minY - 1, 0 );
    const int32_t roiToRenderMaxX = std::min( maxX + 2, world.w() );
    const int32_t roiToRenderMaxY = std::min( maxY + 2, world.h() );

    // These are parts of original action objects which must be rendered under heroes / boats.
    std::vector<int32_t> staticActionObjectTiles;

    for ( int32_t posY = roiToRenderMinY; posY < roiToRenderMaxY; ++posY ) {
        for ( int32_t posX = roiToRenderMinX; posX < roiToRenderMaxX; ++posX ) {
            const Maps::Tiles & tile = world.GetTiles( posX, posY );
            MP2::MapObjectType objectType = tile.GetObject();

            switch ( objectType ) {
            case MP2::OBJ_HEROES: {
                if ( !drawHeroes ) {
                    continue;
                }

                populateHeroObjectInfo( tileUnfit, tile.GetHeroes() );

                break;
            }

            case MP2::OBJ_MONSTER: {
                if ( isPuzzleDraw ) {
                    continue;
                }

                const uint8_t alphaValue = getObjectAlphaValue( tile.GetIndex(), MP2::OBJ_MONSTER );

                auto spriteInfo = tile.getMonsterSpritesPerTile();
                auto spriteShadowInfo = tile.getMonsterShadowSpritesPerTile();

                populateStaticTileUnfitObjectInfo( tileUnfit, spriteInfo, spriteShadowInfo, tile.GetCenter(), alphaValue );

                continue;
            }

            case MP2::OBJ_BOAT: {
                if ( !drawHeroes ) {
                    // Boats can be occupied by heroes so they are considered as the same objects.
                    continue;
                }

                const uint8_t alphaValue = getObjectAlphaValue( tile.GetIndex(), MP2::OBJ_BOAT );

                auto spriteInfo = tile.getBoatSpritesPerTile();
                auto spriteShadowInfo = tile.getBoatShadowSpritesPerTile();

                populateStaticTileUnfitObjectInfo( tileUnfit, spriteInfo, spriteShadowInfo, tile.GetCenter(), alphaValue );

                continue;
            }

            default:
                break;
            }

            if ( objectType == MP2::OBJ_HEROES ) {
                objectType = tile.GetObject( false );
            }

            switch ( objectType ) {
            case MP2::OBJ_MINES: {
                staticActionObjectTiles.push_back( tile.GetIndex() );
                break;
            }

            default:
                break;
            }
        }
    }

    for ( const int32_t tileId : staticActionObjectTiles ) {
        const Maps::Tiles & tile = world.GetTiles( tileId );
        MP2::MapObjectType objectType = tile.GetObject( false );
        switch ( objectType ) {
        case MP2::OBJ_MINES: {
            auto spriteInfo = tile.getMineGuardianSpritesPerTile();
            if ( !spriteInfo.empty() ) {
                const uint8_t alphaValue = getObjectAlphaValue( tile.GetObjectUID() );
                populateStaticTileUnfitBackgroundObjectInfo( tileUnfit, spriteInfo, tile.GetCenter(), alphaValue );
            }

            break;
        }

        default:
            break;
        }
    }

    // Tile unfit objects should be rendered over the edge of the map, except the bottom. We also shouldn't render their shadows over the edge.
    // If a tile contains fog we shouldn't draw anything over the edge.
    if ( minY == 0 ) {
        for ( int32_t x = minX - 1; x < maxX + 1; ++x ) {
            if ( drawFog && world.GetTiles( std::clamp( x, 0, maxX - 1 ), minY ).isFog( friendColors ) ) {
                continue;
            }

            // Boat is taller than 2 tiles.
            renderOutOfMapTile( tileUnfit, dst, { x, minY - 2 }, *this );
            renderOutOfMapTile( tileUnfit, dst, { x, minY - 1 }, *this );
        }
    }

    if ( minX == 0 ) {
        for ( int32_t y = minY; y < maxY; ++y ) {
            if ( drawFog && world.GetTiles( minX, y ).isFog( friendColors ) ) {
                continue;
            }

            renderOutOfMapTile( tileUnfit, dst, { minX - 1, y }, *this );
        }
    }

    if ( maxX == world.w() ) {
        for ( int32_t y = minY; y < maxY; ++y ) {
            if ( drawFog && world.GetTiles( maxX - 1, y ).isFog( friendColors ) ) {
                continue;
            }

            renderOutOfMapTile( tileUnfit, dst, { maxX + 1, y }, *this );
        }
    }

    // Render all terrain layer objects.
    for ( int32_t y = minY; y < maxY; ++y ) {
        for ( int32_t x = minX; x < maxX; ++x ) {
            const Maps::Tiles & tile = world.GetTiles( x, y );

            // Draw roads, rivers and cracks.
            tile.redrawBottomLayerObjects( dst, isPuzzleDraw, *this, Maps::TERRAIN_LAYER );
        }
    }

    // Render all background layer object.
    for ( int32_t y = minY; y < maxY; ++y ) {
        for ( int32_t x = minX; x < maxX; ++x ) {
            const Maps::Tiles & tile = world.GetTiles( x, y );

            tile.redrawBottomLayerObjects( dst, isPuzzleDraw, *this, Maps::BACKGROUND_LAYER );

            // Draw the lower part of tile-unfit object's sprite.
            renderImagesOnTile( dst, tileUnfit.bottomBackgroundImages, { x, y }, *this );
        }
    }

    for ( int32_t y = minY; y < maxY; ++y ) {
        for ( int32_t x = minX; x < maxX; ++x ) {
            const Maps::Tiles & tile = world.GetTiles( x, y );

            tile.redrawBottomLayerObjects( dst, isPuzzleDraw, *this, Maps::SHADOW_LAYER );

            // Draw all shadows from tile-unfit objects.
            renderImagesOnTile( dst, tileUnfit.shadowImages, { x, y }, *this );
        }
    }

    for ( int32_t y = minY; y < maxY; ++y ) {
        for ( int32_t x = minX; x < maxX; ++x ) {
            const Maps::Tiles & tile = world.GetTiles( x, y );

            // Low priority images are drawn before any other object on this tile.
            renderImagesOnTile( dst, tileUnfit.lowPriorityBottomImages, { x, y }, *this );

            // TODO: some action objects have tiles above which are still on bottom layer. These images must be drawn last.
            tile.redrawBottomLayerObjects( dst, isPuzzleDraw, *this, Maps::OBJECT_LAYER );

            // Draw middle part of tile-unfit sprites.
            renderImagesOnTile( dst, tileUnfit.bottomImages, { x, y }, *this );

            // High priority images are drawn after any other object on this tile.
            renderImagesOnTile( dst, tileUnfit.highPriorityBottomImages, { x, y }, *this );
        }
    }

    std::vector<const Maps::TilesAddon *> topLayerTallObjects;

    for ( int32_t y = minY; y < maxY; ++y ) {
        for ( int32_t x = minX; x < maxX; ++x ) {
            const Maps::Tiles & tile = world.GetTiles( x, y );

            // Since some objects are taller than 2 tiles their top layer sprites must be drawn at the very end.
            // For now what we need to do is to run throught all level 2 objects and verify that the tile below doesn't have
            // any other level 2 objects with the same UID.

            topLayerTallObjects.clear();
            for ( const Maps::TilesAddon & addon : tile.getLevel2Addons() ) {
                if ( isTallTopLayerObject( x, y, addon.uniq ) ) {
                    topLayerTallObjects.emplace_back( &addon );
                }
                else {
                    tile.redrawTopLayerObject( dst, isPuzzleDraw, *this, addon );
                }
            }

            tile.redrawTopLayerExtraObjects( dst, isPuzzleDraw, *this );

            // Draw upper part of tile-unfit sprites.
            renderImagesOnTile( dst, tileUnfit.topImages, { x, y }, *this );

            for ( const Maps::TilesAddon * addon : topLayerTallObjects ) {
                tile.redrawTopLayerObject( dst, isPuzzleDraw, *this, *addon );
            }
        }
    }

    // Draw hero's route. It should be drawn on top of everything.
    const bool drawRoutes = ( flag & LEVEL_ROUTES ) != 0;

    if ( drawRoutes && ( currentHero != nullptr ) && currentHero->GetPath().isShow() ) {
        const Route::Path & path = currentHero->GetPath();
        int greenColorSteps = path.GetAllowedSteps();

        const int pathfinding = currentHero->GetLevelSkill( Skill::Secondary::PATHFINDING );

        Route::Path::const_iterator currentStep = path.begin();
        Route::Path::const_iterator nextStep = currentStep;

        if ( currentHero->isMoveEnabled() ) {
            // Do not draw the first path mark when hero / boat is moving.
            ++currentStep;
            ++nextStep;
            --greenColorSteps;
        }

        // Not all arrows and their shadows fit in 1 tile. We need to consider by 1 tile bigger area to properly render eveything.
        const fheroes2::Rect extendedVisibleRoi{ tileROI.x - 1, tileROI.y - 1, tileROI.width + 2, tileROI.height + 2 };

        for ( ; currentStep != path.end(); ++currentStep ) {
            const int32_t tileIndex = currentStep->GetIndex();
            const fheroes2::Point & mp = Maps::GetPoint( tileIndex );

            ++nextStep;
            --greenColorSteps;

            if ( !( extendedVisibleRoi & mp ) ) {
                // The mark is on a tile outside the drawing area. Just skip it.
                continue;
            }

            uint32_t routeSpriteIndex = 0;
            if ( nextStep != path.end() ) {
                const Maps::Tiles & tile = world.GetTiles( tileIndex );
                const uint32_t cost = tile.isRoad() ? Maps::Ground::roadPenalty : Maps::Ground::GetPenalty( tile, pathfinding );

                routeSpriteIndex = Route::Path::GetIndexSprite( currentStep->GetDirection(), nextStep->GetDirection(), cost );
            }

            const fheroes2::Sprite & routeSprite = fheroes2::AGG::GetICN( ( ( greenColorSteps < 0 ) ? ICN::ROUTERED : ICN::ROUTE ), routeSpriteIndex );
            BlitOnTile( dst, routeSprite, routeSprite.x() - 12, routeSprite.y() + 2, mp, false, 255 );
        }
    }

    const bool drawTowns = ( flag & LEVEL_TOWNS );

#ifdef WITH_DEBUG
    if ( IS_DEVEL() ) {
        // redraw grid
        if ( flag & LEVEL_ALL ) {
            for ( int32_t y = minY; y < maxY; ++y ) {
                for ( int32_t x = minX; x < maxX; ++x ) {
                    world.GetTiles( x, y ).RedrawPassable( dst, friendColors, *this );
                }
            }
        }
    }
    else
#endif
        // redraw fog
        if ( drawFog ) {
        for ( int32_t y = minY; y < maxY; ++y ) {
            for ( int32_t x = minX; x < maxX; ++x ) {
                const Maps::Tiles & tile = world.GetTiles( x, y );

                if ( tile.isFog( friendColors ) ) {
                    tile.drawFog( dst, friendColors, *this );

                    if ( drawTowns ) {
                        tile.drawByIcnId( dst, *this, ICN::OBJNTWBA );

                        const MP2::MapObjectType objectType = tile.GetObject( false );
                        if ( objectType == MP2::OBJ_CASTLE || objectType == MP2::OBJN_CASTLE ) {
                            tile.drawByIcnId( dst, *this, ICN::OBJNTOWN );
                        }
                    }
                }
            }
        }
    }

    updateObjectAnimationInfo();
}

void Interface::GameArea::Scroll()
{
    const int32_t scrollSpeed = Settings::Get().ScrollSpeed();
    if ( scrollSpeed == SCROLL_SPEED_NONE ) {
        // No scrolling.
        scrollDirection = SCROLL_NONE;
        return;
    }

    const int32_t shift = 2 << scrollSpeed;
    fheroes2::Point offset;

    if ( scrollDirection & SCROLL_LEFT ) {
        offset.x = -shift;
    }
    else if ( scrollDirection & SCROLL_RIGHT ) {
        offset.x = shift;
    }

    if ( scrollDirection & SCROLL_TOP ) {
        offset.y = -shift;
    }
    else if ( scrollDirection & SCROLL_BOTTOM ) {
        offset.y = shift;
    }

    ShiftCenter( offset );

    scrollDirection = SCROLL_NONE;
}

void Interface::GameArea::SetRedraw() const
{
    interface.SetRedraw( REDRAW_GAMEAREA );
}

fheroes2::Image Interface::GameArea::GenerateUltimateArtifactAreaSurface( const int32_t index, const fheroes2::Point & offset )
{
    if ( !Maps::isValidAbsIndex( index ) ) {
        DEBUG_LOG( DBG_ENGINE, DBG_WARN, "Ultimate artifact is not found on index " << index )
        return fheroes2::Image();
    }

    fheroes2::Image result( 448, 448 );
    result.reset();

    // Make a temporary copy
    GameArea gamearea = Basic::Get().GetGameArea();

    gamearea.SetAreaPosition( 0, 0, result.width(), result.height() );

    const fheroes2::Point pt = Maps::GetPoint( index );
    gamearea.SetCenter( pt + offset );

    gamearea.Redraw( result, LEVEL_OBJECTS, true );

    const fheroes2::Sprite & marker = fheroes2::AGG::GetICN( ICN::ROUTE, 0 );
    const fheroes2::Point markerPos( gamearea.GetRelativeTilePosition( pt ) - gamearea._middlePoint() - fheroes2::Point( gamearea._windowROI.x, gamearea._windowROI.y )
                                     + fheroes2::Point( result.width() / 2, result.height() / 2 ) );

    fheroes2::Blit( marker, result, markerPos.x, markerPos.y + 8 );
    fheroes2::ApplyPalette( result, PAL::GetPalette( PAL::PaletteType::TAN ) );
    result._disableTransformLayer();

    return result;
}

int Interface::GameArea::GetScrollCursor() const
{
    switch ( scrollDirection ) {
    case SCROLL_LEFT | SCROLL_TOP:
        return Cursor::SCROLL_TOPLEFT;
    case SCROLL_LEFT | SCROLL_BOTTOM:
        return Cursor::SCROLL_BOTTOMLEFT;
    case SCROLL_RIGHT | SCROLL_TOP:
        return Cursor::SCROLL_TOPRIGHT;
    case SCROLL_RIGHT | SCROLL_BOTTOM:
        return Cursor::SCROLL_BOTTOMRIGHT;
    case SCROLL_TOP:
        return Cursor::SCROLL_TOP;
    case SCROLL_BOTTOM:
        return Cursor::SCROLL_BOTTOM;
    case SCROLL_RIGHT:
        return Cursor::SCROLL_RIGHT;
    case SCROLL_LEFT:
        return Cursor::SCROLL_LEFT;
    default:
        break;
    }

    return Cursor::NONE;
}

void Interface::GameArea::SetScroll( int direct )
{
    if ( ( direct & SCROLL_LEFT ) == SCROLL_LEFT ) {
        if ( _topLeftTileOffset.x > _minLeftOffset ) {
            scrollDirection |= direct;
            updateCursor = true;
        }
    }
    else if ( ( direct & SCROLL_RIGHT ) == SCROLL_RIGHT ) {
        if ( _topLeftTileOffset.x < _maxLeftOffset ) {
            scrollDirection |= direct;
            updateCursor = true;
        }
    }

    if ( ( direct & SCROLL_TOP ) == SCROLL_TOP ) {
        if ( _topLeftTileOffset.y > _minTopOffset ) {
            scrollDirection |= direct;
            updateCursor = true;
        }
    }
    else if ( ( direct & SCROLL_BOTTOM ) == SCROLL_BOTTOM ) {
        if ( _topLeftTileOffset.y < _maxTopOffset ) {
            scrollDirection |= direct;
            updateCursor = true;
        }
    }

    scrollTime.reset();
}

void Interface::GameArea::QueueEventProcessing( bool isCursorOverGamearea )
{
    LocalEvent & le = LocalEvent::Get();
    const fheroes2::Point & mp = le.GetMouseCursor();

    if ( !le.MousePressLeft() ) {
        _mouseDraggingInitiated = false;
        _mouseDraggingMovement = false;
    }
    else if ( !_mouseDraggingInitiated ) {
        _mouseDraggingInitiated = true;
        _startMouseDragPosition = mp;
    }
    else if ( ( std::abs( _startMouseDragPosition.x - mp.x ) > minimalRequiredDraggingMovement
                || std::abs( _startMouseDragPosition.y - mp.y ) > minimalRequiredDraggingMovement )
              && isCursorOverGamearea ) {
        _mouseDraggingMovement = true;
    }

    if ( _mouseDraggingMovement ) {
        SetCenterInPixels( getCurrentCenterInPixels() + _startMouseDragPosition - mp );
        _startMouseDragPosition = mp;
        return;
    }

    int32_t index = GetValidTileIdFromPoint( mp );

    // change cursor if need
    if ( ( updateCursor || index != _prevIndexPos ) && isCursorOverGamearea ) {
        Cursor::Get().SetThemes( Interface::Basic::GetCursorTileIndex( index ) );
        _prevIndexPos = index;
        updateCursor = false;
    }

    // out of range
    if ( index < 0 )
        return;

    const Settings & conf = Settings::Get();
    if ( conf.isHideInterfaceEnabled() && conf.ShowControlPanel() && le.MouseCursor( interface.GetControlPanel().GetArea() ) )
        return;

    const fheroes2::Point tileOffset = _topLeftTileOffset + mp - _windowROI.getPosition();
    const fheroes2::Point tilePos( ( tileOffset.x / TILEWIDTH ) * TILEWIDTH - _topLeftTileOffset.x + _windowROI.x,
                                   ( tileOffset.y / TILEWIDTH ) * TILEWIDTH - _topLeftTileOffset.y + _windowROI.x );

    const fheroes2::Rect tileROI( tilePos.x, tilePos.y, TILEWIDTH, TILEWIDTH );

    if ( le.MouseClickLeft( tileROI ) )
        interface.MouseCursorAreaClickLeft( index );
    else if ( le.MousePressRight( tileROI ) )
        interface.MouseCursorAreaPressRight( index );
}

fheroes2::Point Interface::GameArea::_getStartTileId() const
{
    const int32_t x = ( _topLeftTileOffset.x < 0 ? ( _topLeftTileOffset.x - TILEWIDTH - 1 ) / TILEWIDTH : _topLeftTileOffset.x / TILEWIDTH );
    const int32_t y = ( _topLeftTileOffset.y < 0 ? ( _topLeftTileOffset.y - TILEWIDTH - 1 ) / TILEWIDTH : _topLeftTileOffset.y / TILEWIDTH );

    return { x, y };
}

void Interface::GameArea::_setCenterToTile( const fheroes2::Point & tile )
{
    SetCenterInPixels( { tile.x * TILEWIDTH + TILEWIDTH / 2, tile.y * TILEWIDTH + TILEWIDTH / 2 } );
}

void Interface::GameArea::SetCenterInPixels( const fheroes2::Point & point )
{
    const fheroes2::Point & middlePos = _middlePoint();

    int32_t offsetX = point.x - middlePos.x;
    int32_t offsetY = point.y - middlePos.y;
    if ( offsetX < _minLeftOffset )
        offsetX = _minLeftOffset;
    else if ( offsetX > _maxLeftOffset )
        offsetX = _maxLeftOffset;

    if ( offsetY < _minTopOffset )
        offsetY = _minTopOffset;
    else if ( offsetY > _maxTopOffset )
        offsetY = _maxTopOffset;

    _topLeftTileOffset = fheroes2::Point( offsetX, offsetY );
}

int32_t Interface::GameArea::GetValidTileIdFromPoint( const fheroes2::Point & point ) const
{
    const fheroes2::Point offset = _topLeftTileOffset + point - _windowROI.getPosition();
    if ( offset.x < 0 || offset.y < 0 )
        return -1;

    const int32_t x = offset.x / TILEWIDTH;
    const int32_t y = offset.y / TILEWIDTH;

    if ( x >= world.w() || y >= world.h() )
        return -1;

    return y * world.w() + x;
}

fheroes2::Point Interface::GameArea::GetRelativeTilePosition( const fheroes2::Point & tileId ) const
{
    return { tileId.x * TILEWIDTH - _topLeftTileOffset.x + _windowROI.x, tileId.y * TILEWIDTH - _topLeftTileOffset.y + _windowROI.y };
}

void Interface::GameArea::updateObjectAnimationInfo() const
{
    for ( auto iter = _animationInfo.begin(); iter != _animationInfo.end(); ) {
        if ( ( *iter )->update() ) {
            iter = _animationInfo.erase( iter );
        }
        else {
            ++iter;
        }
    }
}

uint8_t Interface::GameArea::getObjectAlphaValue( const int32_t tileId, const MP2::MapObjectType type ) const
{
    for ( const auto & info : _animationInfo ) {
        if ( info->tileId == tileId && type == info->type ) {
            return info->alphaValue;
        }
    }

    return 255;
}

uint8_t Interface::GameArea::getObjectAlphaValue( const uint32_t uid ) const
{
    for ( const auto & info : _animationInfo ) {
        if ( uid == info->uid ) {
            return info->alphaValue;
        }
    }

    return 255;
}

void Interface::GameArea::runSingleObjectAnimation( const std::shared_ptr<BaseObjectAnimationInfo> & info )
{
    if ( !info ) {
        assert( 0 );
        return;
    }

    addObjectAnimationInfo( info );

    LocalEvent & le = LocalEvent::Get();

    while ( le.HandleEvents() && !info->isAnimationCompleted() ) {
        if ( Game::validateAnimationDelay( Game::HEROES_PICKUP_DELAY ) ) {
            Interface::Basic::Get().Redraw( Interface::REDRAW_GAMEAREA );
            fheroes2::Display::instance().render();
        }
    }
}

Interface::ObjectFadingOutInfo::~ObjectFadingOutInfo()
{
    Maps::Tiles & tile = world.GetTiles( tileId );

    if ( tile.GetObject() == type ) {
        tile.RemoveObjectSprite();
        tile.setAsEmpty();
    }
}
