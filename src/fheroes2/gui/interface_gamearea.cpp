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

#include "interface_gamearea.h"

#include "agg_image.h"
#include "cursor.h"
#include "game.h"
#include "game_delays.h"
#include "game_interface.h"
#include "ground.h"
#include "icn.h"
#include "logging.h"
#include "maps.h"
#include "pal.h"
#include "route.h"
#include "settings.h"
#include "tools.h"
#include "world.h"

#include <cassert>
#include <deque>

namespace
{
    struct RenderObjectInfo
    {
        RenderObjectInfo() = default;

        RenderObjectInfo( fheroes2::Sprite in, const uint8_t value )
            : image( std::move( in ) )
            , alphaValue( value )
        {
            // Do nothing.
        }

        fheroes2::Sprite image;

        uint8_t alphaValue{ 255 };
    };
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
{}

fheroes2::Rect Interface::GameArea::GetVisibleTileROI() const
{
    return { _getStartTileId(), _visibleTileCount };
}

void Interface::GameArea::ShiftCenter( const fheroes2::Point & offset )
{
    SetCenterInPixels( _topLeftTileOffset + _middlePoint() + offset );
}

fheroes2::Rect Interface::GameArea::RectFixed( fheroes2::Point & dst, int rw, int rh ) const
{
    std::pair<fheroes2::Rect, fheroes2::Point> res = Fixed4Blit( fheroes2::Rect( dst.x, dst.y, rw, rh ), GetROI() );
    dst = res.second;
    return res.first;
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
    fheroes2::Point dstpt = GetRelativeTilePosition( mp ) + fheroes2::Point( ox, oy );

    const int32_t width = src.width();
    const int32_t height = src.height();

    // In most of cases objects locate within window ROI so we don't need to calculate truncated ROI
    if ( dstpt.x >= _windowROI.x && dstpt.y >= _windowROI.y && dstpt.x + width <= _windowROI.x + _windowROI.width
         && dstpt.y + height <= _windowROI.y + _windowROI.height ) {
        fheroes2::AlphaBlit( src, 0, 0, dst, dstpt.x, dstpt.y, width, height, alpha, flip );
    }
    else if ( _windowROI & fheroes2::Rect( dstpt.x, dstpt.y, width, height ) ) {
        const fheroes2::Rect & fixedRect = RectFixed( dstpt, width, height );
        fheroes2::AlphaBlit( src, fixedRect.x, fixedRect.y, dst, dstpt.x, dstpt.y, fixedRect.width, fixedRect.height, alpha, flip );
    }
}

void Interface::GameArea::DrawTile( fheroes2::Image & dst, const fheroes2::Image & src, const fheroes2::Point & mp ) const
{
    fheroes2::Point dstpt = GetRelativeTilePosition( mp );

    const int32_t width = src.width();
    const int32_t height = src.height();

    // In most of cases objects locate within window ROI so we don't need to calculate truncated ROI
    if ( dstpt.x >= _windowROI.x && dstpt.y >= _windowROI.y && dstpt.x + width <= _windowROI.x + _windowROI.width
         && dstpt.y + height <= _windowROI.y + _windowROI.height ) {
        fheroes2::Copy( src, 0, 0, dst, dstpt.x, dstpt.y, width, height );
    }
    else if ( _windowROI & fheroes2::Rect( dstpt.x, dstpt.y, width, height ) ) {
        const fheroes2::Rect & fixedRect = RectFixed( dstpt, width, height );
        fheroes2::Copy( src, fixedRect.x, fixedRect.y, dst, dstpt.x, dstpt.y, fixedRect.width, fixedRect.height );
    }
}

void Interface::GameArea::Redraw( fheroes2::Image & dst, int flag, bool isPuzzleDraw ) const
{
    const fheroes2::Rect & tileROI = GetVisibleTileROI();

    int32_t minX = tileROI.x;
    int32_t minY = tileROI.y;
    int32_t maxX = tileROI.x + tileROI.width;
    int32_t maxY = tileROI.y + tileROI.height;

    // Ground level. Also find range of X and Y tile positions.
    for ( int32_t y = 0; y < tileROI.height; ++y ) {
        fheroes2::Point offset( tileROI.x, tileROI.y + y );

        if ( offset.y < 0 || offset.y >= world.h() ) {
            for ( ; offset.x < maxX; ++offset.x ) {
                Maps::Tiles::RedrawEmptyTile( dst, offset, tileROI, *this );
            }
        }
        else {
            for ( ; offset.x < maxX; ++offset.x ) {
                if ( offset.x < 0 || offset.x >= world.w() ) {
                    Maps::Tiles::RedrawEmptyTile( dst, offset, tileROI, *this );
                }
                else {
                    world.GetTiles( offset.x, offset.y ).RedrawTile( dst, tileROI, *this );
                }
            }
        }
    }

    if ( minX < 0 )
        minX = 0;
    if ( minY < 0 )
        minY = 0;
    if ( maxX > world.w() )
        maxX = world.w();
    if ( maxY > world.h() )
        maxY = world.h();

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
    //
    // TODO: monster sprites combine the sprite itself and shadow. To properly render it we need to separate both parts and render on different layers.

    // Fading animation can be applied as for tile-fit and tile-unfit objects.
    // In case of tile-fit objects we need to pass UID of the object and alpha values.
    // In case of tile-unfit objects we need to setalpha value while creating RenderObjectInfo instances.
    // So far only one object and multple heroes can have fading animation.

    const bool drawHeroes = ( flag & LEVEL_HEROES ) == LEVEL_HEROES;

#ifdef WITH_DEBUG
    const bool drawFog = ( ( flag & LEVEL_FOG ) == LEVEL_FOG ) && !IS_DEVEL();
#else
    const bool drawFog = ( flag & LEVEL_FOG ) == LEVEL_FOG;
#endif

    std::map<fheroes2::Point, std::deque<RenderObjectInfo>> tileUnfitBottomImages;
    std::map<fheroes2::Point, std::deque<RenderObjectInfo>> tileUnfitBottomBackgroundImages;
    std::map<fheroes2::Point, std::deque<RenderObjectInfo>> tileUnfitTopImages;

    std::map<fheroes2::Point, std::deque<RenderObjectInfo>> tileUnfitLowPriorityBottomImages;
    std::map<fheroes2::Point, std::deque<RenderObjectInfo>> tileUnfitHighPriorityBottomImages;

    std::map<fheroes2::Point, std::deque<RenderObjectInfo>> tileUnfitBottomShadowImages;
    std::map<fheroes2::Point, std::deque<RenderObjectInfo>> tileUnfitBottomBackgroundShadowImages;
    std::map<fheroes2::Point, std::deque<RenderObjectInfo>> tileUnfitTopShadowImages;

    const Heroes * currentHero = drawHeroes ? GetFocusHeroes() : nullptr;

    // TODO: Dragon City with tilset 164 (OBJNMUL2.ICN) and object index 46 is a bottom layer sprite.
    // TODO: When a hero standing besides this turns a part of the hero is visible. This can be fixed only by some hack.

    // Run through all visible tiles and find all tile-unfit objects. Also cover extra tiles from right and bottom sides.
    const int32_t roiToRenderMaxX = std::min( maxX + 2, world.w() );
    const int32_t roiToRenderMaxY = std::min( maxY + 1, world.h() );

    for ( int32_t posY = minY; posY < roiToRenderMaxY; ++posY ) {
        for ( int32_t posX = minX; posX < roiToRenderMaxX; ++posX ) {
            const Maps::Tiles & tile = world.GetTiles( posX, posY );
            const MP2::MapObjectType objectType = tile.GetObject();

            switch ( objectType ) {
            case MP2::OBJ_HEROES: {
                if ( !drawHeroes ) {
                    continue;
                }

                const Heroes * hero = tile.GetHeroes();
                assert( hero != nullptr );

                const fheroes2::Point & heroPos = hero->GetCenter();
                fheroes2::Point nextHeroPos = heroPos;

                const bool movingHero = ( currentHero == hero ) && ( hero->isMoveEnabled() );
                if ( movingHero ) {
                    const Route::Path & path = currentHero->GetPath();
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
                for ( auto & info : spriteInfo ) {
                    if ( movingHero && info.first.y == 0 ) {
                        if ( nextHeroPos.y > heroPos.y && nextHeroPos.x > heroPos.x && info.first.x > 0 ) {
                            // The hero moves south-east. We need to render it over everything.
                            tileUnfitHighPriorityBottomImages[info.first + heroPos].emplace_back( std::move( info.second ), heroAlphaValue );
                            continue;
                        }

                        if ( nextHeroPos.y > heroPos.y && nextHeroPos.x < heroPos.x && info.first.x < 0 ) {
                            // The hero moves south-west. We need to render it over everything.
                            tileUnfitHighPriorityBottomImages[info.first + heroPos].emplace_back( std::move( info.second ), heroAlphaValue );
                            continue;
                        }

                        if ( nextHeroPos.y < heroPos.y && nextHeroPos.x < heroPos.x && info.first.x < 0 ) {
                            // The hero moves north-west. We need to render it under all other objects.
                            tileUnfitLowPriorityBottomImages[info.first + heroPos].emplace_back( std::move( info.second ), heroAlphaValue );
                            continue;
                        }

                        if ( nextHeroPos.y < heroPos.y && nextHeroPos.x > heroPos.x && info.first.x > 0 ) {
                            // The hero moves north-east. We need to render it under all other objects.
                            tileUnfitLowPriorityBottomImages[info.first + heroPos].emplace_back( std::move( info.second ), heroAlphaValue );
                            continue;
                        }
                    }

                    if ( movingHero && info.first.y == 1 ) {
                        if ( nextHeroPos.y > heroPos.y && nextHeroPos.x > heroPos.x && info.first.x > 0 ) {
                            // The hero moves south-east. We need to render it over everything.
                            tileUnfitBottomImages[info.first + heroPos].emplace_back( std::move( info.second ), heroAlphaValue );
                            continue;
                        }

                        if ( nextHeroPos.y > heroPos.y && nextHeroPos.x < heroPos.x && info.first.x < 0 ) {
                            // The hero moves south-west. We need to render it over everything.
                            tileUnfitBottomImages[info.first + heroPos].emplace_back( std::move( info.second ), heroAlphaValue );
                            continue;
                        }
                    }

                    if ( movingHero && info.first.y == -1 ) {
                        if ( nextHeroPos.y < heroPos.y && nextHeroPos.x < heroPos.x && info.first.x < 0 ) {
                            // The hero moves north-west. We need to render it under all other objects.
                            tileUnfitBottomImages[info.first + heroPos].emplace_back( std::move( info.second ), heroAlphaValue );
                            continue;
                        }

                        if ( nextHeroPos.y < heroPos.y && nextHeroPos.x > heroPos.x && info.first.x > 0 ) {
                            // The hero moves north-east. We need to render it under all other objects.
                            tileUnfitBottomImages[info.first + heroPos].emplace_back( std::move( info.second ), heroAlphaValue );
                            continue;
                        }
                    }

                    if ( info.first.y > 0 && !isHeroInCastle ) {
                        if ( info.first.x < 0 ) {
                            tileUnfitBottomBackgroundImages[info.first + heroPos].emplace_front( std::move( info.second ), heroAlphaValue );
                        }
                        else {
                            tileUnfitBottomBackgroundImages[info.first + heroPos].emplace_back( std::move( info.second ), heroAlphaValue );
                        }
                    }
                    else if ( info.first.y == 0 || ( isHeroInCastle && info.first.y > 0 ) ) {
                        if ( info.first.x < 0 ) {
                            tileUnfitBottomImages[info.first + heroPos].emplace_front( std::move( info.second ), heroAlphaValue );
                        }
                        else {
                            tileUnfitBottomImages[info.first + heroPos].emplace_back( std::move( info.second ), heroAlphaValue );
                        }
                    }
                    else {
                        if ( info.first.x < 0 ) {
                            tileUnfitTopImages[info.first + heroPos].emplace_front( std::move( info.second ), heroAlphaValue );
                        }
                        else {
                            tileUnfitTopImages[info.first + heroPos].emplace_back( std::move( info.second ), heroAlphaValue );
                        }
                    }
                }

                // TODO: review shadow rendering long as it jumps over the places.
                auto spriteShadowInfo = hero->getHeroShadowSpritesPerTile();
                for ( auto & info : spriteShadowInfo ) {
                    if ( info.first.y > 0 && !isHeroInCastle ) {
                        if ( info.first.x < 0 ) {
                            tileUnfitBottomBackgroundShadowImages[info.first + heroPos].emplace_front( std::move( info.second ), heroAlphaValue );
                        }
                        else {
                            tileUnfitBottomBackgroundShadowImages[info.first + heroPos].emplace_back( std::move( info.second ), heroAlphaValue );
                        }
                    }
                    else if ( info.first.y == 0 || ( isHeroInCastle && info.first.y > 0 ) ) {
                        if ( info.first.x < 0 ) {
                            tileUnfitBottomShadowImages[info.first + heroPos].emplace_front( std::move( info.second ), heroAlphaValue );
                        }
                        else {
                            tileUnfitBottomShadowImages[info.first + heroPos].emplace_back( std::move( info.second ), heroAlphaValue );
                        }
                    }
                    else {
                        if ( info.first.x < 0 ) {
                            tileUnfitTopShadowImages[info.first + heroPos].emplace_front( std::move( info.second ), heroAlphaValue );
                        }
                        else {
                            tileUnfitTopShadowImages[info.first + heroPos].emplace_back( std::move( info.second ), heroAlphaValue );
                        }
                    }
                }

                break;
            }

            case MP2::OBJ_MONSTER: {
                const uint8_t alphaValue = getObjectAlphaValue( tile.GetIndex(), MP2::OBJ_MONSTER );

                auto spriteInfo = tile.getMonsterSpritesPerTile();
                for ( auto & info : spriteInfo ) {
                    if ( info.first.y > 0 ) {
                        if ( info.first.x < 0 ) {
                            tileUnfitBottomBackgroundImages[info.first + tile.GetCenter()].emplace_front( std::move( info.second ), alphaValue );
                        }
                        else {
                            tileUnfitBottomBackgroundImages[info.first + tile.GetCenter()].emplace_back( std::move( info.second ), alphaValue );
                        }
                    }
                    else if ( info.first.y == 0 ) {
                        if ( info.first.x < 0 ) {
                            tileUnfitBottomImages[info.first + tile.GetCenter()].emplace_front( std::move( info.second ), alphaValue );
                        }
                        else {
                            tileUnfitBottomImages[info.first + tile.GetCenter()].emplace_back( std::move( info.second ), alphaValue );
                        }
                    }
                    else {
                        if ( info.first.x < 0 ) {
                            tileUnfitTopImages[info.first + tile.GetCenter()].emplace_front( std::move( info.second ), alphaValue );
                        }
                        else {
                            tileUnfitTopImages[info.first + tile.GetCenter()].emplace_back( std::move( info.second ), alphaValue );
                        }
                    }
                }

                // TODO: review shadow rendering long as it jumps over the places.
                auto spriteShadowInfo = tile.getMonsterShadowSpritesPerTile();
                for ( auto & info : spriteShadowInfo ) {
                    if ( info.first.y > 0 ) {
                        // TODO: fix incorrect boat sprites being too tall.
                        if ( info.first.x < 0 ) {
                            tileUnfitBottomBackgroundShadowImages[info.first + tile.GetCenter()].emplace_front( std::move( info.second ), alphaValue );
                        }
                        else {
                            tileUnfitBottomBackgroundShadowImages[info.first + tile.GetCenter()].emplace_back( std::move( info.second ), alphaValue );
                        }
                    }
                    else if ( info.first.y == 0 ) {
                        if ( info.first.x < 0 ) {
                            tileUnfitBottomShadowImages[info.first + tile.GetCenter()].emplace_front( std::move( info.second ), alphaValue );
                        }
                        else {
                            tileUnfitBottomShadowImages[info.first + tile.GetCenter()].emplace_back( std::move( info.second ), alphaValue );
                        }
                    }
                    else {
                        if ( info.first.x < 0 ) {
                            tileUnfitTopShadowImages[info.first + tile.GetCenter()].emplace_front( std::move( info.second ), alphaValue );
                        }
                        else {
                            tileUnfitTopShadowImages[info.first + tile.GetCenter()].emplace_back( std::move( info.second ), alphaValue );
                        }
                    }
                }

                break;
            }

            case MP2::OBJ_BOAT: {
                const uint8_t alphaValue = getObjectAlphaValue( tile.GetIndex(), MP2::OBJ_BOAT );

                auto spriteInfo = tile.getBoatSpritesPerTile();
                for ( auto & info : spriteInfo ) {
                    if ( info.first.y > 0 ) {
                        // TODO: fix incorrect boat sprites being too tall.
                        if ( info.first.x < 0 ) {
                            tileUnfitBottomBackgroundImages[info.first + tile.GetCenter()].emplace_front( std::move( info.second ), alphaValue );
                        }
                        else {
                            tileUnfitBottomBackgroundImages[info.first + tile.GetCenter()].emplace_back( std::move( info.second ), alphaValue );
                        }
                    }
                    else if ( info.first.y == 0 ) {
                        if ( info.first.x < 0 ) {
                            tileUnfitBottomImages[info.first + tile.GetCenter()].emplace_front( std::move( info.second ), alphaValue );
                        }
                        else {
                            tileUnfitBottomImages[info.first + tile.GetCenter()].emplace_back( std::move( info.second ), alphaValue );
                        }
                    }
                    else {
                        if ( info.first.x < 0 ) {
                            tileUnfitTopImages[info.first + tile.GetCenter()].emplace_front( std::move( info.second ), alphaValue );
                        }
                        else {
                            tileUnfitTopImages[info.first + tile.GetCenter()].emplace_back( std::move( info.second ), alphaValue );
                        }
                    }
                }

                // TODO: review shadow rendering long as it jumps over the places.
                auto spriteShadowInfo = tile.getBoatShadowSpritesPerTile();
                for ( auto & info : spriteShadowInfo ) {
                    if ( info.first.y > 0 ) {
                        // TODO: fix incorrect boat sprites being too tall.
                        if ( info.first.x < 0 ) {
                            tileUnfitBottomBackgroundShadowImages[info.first + tile.GetCenter()].emplace_front( std::move( info.second ), alphaValue );
                        }
                        else {
                            tileUnfitBottomBackgroundShadowImages[info.first + tile.GetCenter()].emplace_back( std::move( info.second ), alphaValue );
                        }
                    }
                    else if ( info.first.y == 0 ) {
                        if ( info.first.x < 0 ) {
                            tileUnfitBottomShadowImages[info.first + tile.GetCenter()].emplace_front( std::move( info.second ), alphaValue );
                        }
                        else {
                            tileUnfitBottomShadowImages[info.first + tile.GetCenter()].emplace_back( std::move( info.second ), alphaValue );
                        }
                    }
                    else {
                        if ( info.first.x < 0 ) {
                            tileUnfitTopShadowImages[info.first + tile.GetCenter()].emplace_front( std::move( info.second ), alphaValue );
                        }
                        else {
                            tileUnfitTopShadowImages[info.first + tile.GetCenter()].emplace_back( std::move( info.second ), alphaValue );
                        }
                    }
                }

                break;
            }

            default:
                break;
            }
        }
    }

    // TODO: optimize everything to be run in a single loop. We do not need multiple double loops.
    for ( int32_t y = minY; y < maxY; ++y ) {
        for ( int32_t x = minX; x < maxX; ++x ) {
            const Maps::Tiles & tile = world.GetTiles( x, y );

            // Draw roads, rivers and cracks.
            tile.redrawBottomLayerObjects( dst, tileROI, isPuzzleDraw, *this, Maps::TERRAIN_LAYER );
        }
    }

    for ( int32_t y = minY; y < maxY; ++y ) {
        for ( int32_t x = minX; x < maxX; ++x ) {
            const Maps::Tiles & tile = world.GetTiles( x, y );

            // Draw the lower part of tile-unfit object's sprite.
            auto iter = tileUnfitBottomBackgroundImages.find( { x, y } );
            if ( iter != tileUnfitBottomBackgroundImages.end() ) {
                assert( !iter->second.empty() );

                const fheroes2::Point & mp = Maps::GetPoint( tile.GetIndex() );

                for ( const RenderObjectInfo & info : iter->second ) {
                    BlitOnTile( dst, info.image, info.image.x(), info.image.y(), mp, false, info.alphaValue );
                }
            }

            // Draw bottom part of tile-unfit object's shadow.
            iter = tileUnfitBottomBackgroundShadowImages.find( { x, y } );
            if ( iter != tileUnfitBottomBackgroundShadowImages.end() ) {
                assert( !iter->second.empty() );

                const fheroes2::Point & mp = Maps::GetPoint( tile.GetIndex() );

                for ( const RenderObjectInfo & info : iter->second ) {
                    BlitOnTile( dst, info.image, info.image.x(), info.image.y(), mp, false, info.alphaValue );
                }
            }

            tile.redrawBottomLayerObjects( dst, tileROI, isPuzzleDraw, *this, Maps::BACKGROUND_LAYER );
        }
    }

    for ( int32_t y = minY; y < maxY; ++y ) {
        for ( int32_t x = minX; x < maxX; ++x ) {
            const Maps::Tiles & tile = world.GetTiles( x, y );

            tile.redrawBottomLayerObjects( dst, tileROI, isPuzzleDraw, *this, Maps::SHADOW_LAYER );

            // Draw all shadows from tile-unfit objects.
            auto iter = tileUnfitBottomShadowImages.find( { x, y } );
            if ( iter != tileUnfitBottomShadowImages.end() ) {
                assert( !iter->second.empty() );

                const fheroes2::Point & mp = Maps::GetPoint( tile.GetIndex() );

                for ( const RenderObjectInfo & info : iter->second ) {
                    BlitOnTile( dst, info.image, info.image.x(), info.image.y(), mp, false, info.alphaValue );
                }
            }
        }
    }

    for ( int32_t y = minY; y < maxY; ++y ) {
        for ( int32_t x = minX; x < maxX; ++x ) {
            const Maps::Tiles & tile = world.GetTiles( x, y );

            // Low priority images are drawn before any other object on this tile.
            auto iter = tileUnfitLowPriorityBottomImages.find( { x, y } );
            if ( iter != tileUnfitLowPriorityBottomImages.end() ) {
                assert( !iter->second.empty() );

                const fheroes2::Point & mp = Maps::GetPoint( tile.GetIndex() );

                for ( const RenderObjectInfo & info : iter->second ) {
                    BlitOnTile( dst, info.image, info.image.x(), info.image.y(), mp, false, info.alphaValue );
                }
            }

            // TODO: some action objects have tiles above which are still on bottom layer. These images must be drawn last.
            tile.redrawBottomLayerObjects( dst, tileROI, isPuzzleDraw, *this, Maps::ACTION_OBJECT_LAYER );

            // Draw middle part of tile-unfit sprites.
            iter = tileUnfitBottomImages.find( { x, y } );
            if ( iter != tileUnfitBottomImages.end() ) {
                assert( !iter->second.empty() );

                const fheroes2::Point & mp = Maps::GetPoint( tile.GetIndex() );

                for ( const RenderObjectInfo & info : iter->second ) {
                    BlitOnTile( dst, info.image, info.image.x(), info.image.y(), mp, false, info.alphaValue );
                }
            }

            // High priority images are drawn after any other object on this tile.
            iter = tileUnfitHighPriorityBottomImages.find( { x, y } );
            if ( iter != tileUnfitHighPriorityBottomImages.end() ) {
                assert( !iter->second.empty() );

                const fheroes2::Point & mp = Maps::GetPoint( tile.GetIndex() );

                for ( const RenderObjectInfo & info : iter->second ) {
                    BlitOnTile( dst, info.image, info.image.x(), info.image.y(), mp, false, info.alphaValue );
                }
            }
        }
    }

    for ( int32_t y = minY; y < maxY; ++y ) {
        for ( int32_t x = minX; x < maxX; ++x ) {
            const Maps::Tiles & tile = world.GetTiles( x, y );

            // Draw upper part of tile-unit sprite's shadow.
            auto iter = tileUnfitTopShadowImages.find( { x, y } );
            if ( iter != tileUnfitTopShadowImages.end() ) {
                assert( !iter->second.empty() );

                const fheroes2::Point & mp = Maps::GetPoint( tile.GetIndex() );

                for ( const RenderObjectInfo & info : iter->second ) {
                    BlitOnTile( dst, info.image, info.image.x(), info.image.y(), mp, false, info.alphaValue );
                }
            }

            // Since some objects are taller than 2 tiles their top layer sprites must be drawn at the very end.
            // For now what we need to do is to run throught all level 2 objects and verify that the tile below doesn't have
            // any other level 2 objects with the same UID.
            //
            // TODO: This is a very hacky way to do it since we do not take into consideration that tile-unfit objects might be taller than 2 tiles as well.
            // TODO: Also some objects in level 2 might be drawn below tile-unfit objects.Find a better way to deal with this situation.

            bool renderTileUnfitTopImagesBefore = false;
            if ( y + 1 < world.h() ) {
                // There is a tile below the current.
                const Maps::Tiles & tileBelow = world.GetTiles( x, y + 1 );

                const Maps::Addons & currentTileAddons = tile.getLevel2Addons();
                const Maps::Addons & lowerTileAddons = tileBelow.getLevel2Addons();

                for ( const Maps::TilesAddon & currentAddon : currentTileAddons ) {
                    for ( const Maps::TilesAddon & lowerAddon : lowerTileAddons ) {
                        if ( lowerAddon.uniq == currentAddon.uniq ) {
                            // This is a tall object.
                            renderTileUnfitTopImagesBefore = true;
                            break;
                        }
                    }

                    if ( renderTileUnfitTopImagesBefore ) {
                        break;
                    }
                }
            }

            if ( !renderTileUnfitTopImagesBefore ) {
                tile.redrawTopLayerObjects( dst, tileROI, isPuzzleDraw, *this );
            }

            // Draw upper part of tile-unfit sprites.
            iter = tileUnfitTopImages.find( { x, y } );
            if ( iter != tileUnfitTopImages.end() ) {
                assert( !iter->second.empty() );

                const fheroes2::Point & mp = Maps::GetPoint( tile.GetIndex() );

                for ( const RenderObjectInfo & info : iter->second ) {
                    BlitOnTile( dst, info.image, info.image.x(), info.image.y(), mp, false, info.alphaValue );
                }
            }

            if ( renderTileUnfitTopImagesBefore ) {
                tile.redrawTopLayerObjects( dst, tileROI, isPuzzleDraw, *this );
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

        for ( ; currentStep != path.end(); ++currentStep ) {
            const int32_t from = currentStep->GetIndex();
            const fheroes2::Point & mp = Maps::GetPoint( from );

            ++nextStep;
            --greenColorSteps;

            if ( !( tileROI & mp ) ) {
                // The mark is on a tile outside the drawing area. Just skip it.
                continue;
            }

            uint32_t routeSpriteIndex = 0;
            if ( nextStep != path.end() ) {
                const Maps::Tiles & tileTo = world.GetTiles( currentStep->GetIndex() );
                uint32_t cost = Maps::Ground::GetPenalty( tileTo, pathfinding );

                if ( world.GetTiles( currentStep->GetFrom() ).isRoad() && tileTo.isRoad() ) {
                    cost = Maps::Ground::roadPenalty;
                }

                routeSpriteIndex = Route::Path::GetIndexSprite( currentStep->GetDirection(), nextStep->GetDirection(), cost );
            }

            const fheroes2::Sprite & routeSprite = fheroes2::AGG::GetICN( ( ( greenColorSteps < 0 ) ? ICN::ROUTERED : ICN::ROUTE ), routeSpriteIndex );
            BlitOnTile( dst, routeSprite, routeSprite.x() - 12, routeSprite.y() + 2, mp );
        }
    }

#ifdef WITH_DEBUG
    if ( IS_DEVEL() ) {
        // redraw grid
        if ( flag & LEVEL_ALL ) {
            for ( int32_t y = minY; y < maxY; ++y ) {
                for ( int32_t x = minX; x < maxX; ++x ) {
                    world.GetTiles( x, y ).RedrawPassable( dst, tileROI, *this );
                }
            }
        }
    }
    else
#endif
        // redraw fog
        if ( drawFog ) {
        const int friendColors = Players::FriendColors();

        for ( int32_t y = minY; y < maxY; ++y ) {
            for ( int32_t x = minX; x < maxX; ++x ) {
                const Maps::Tiles & tile = world.GetTiles( x, y );

                const bool isFog = drawFog && tile.isFog( friendColors );
                if ( isFog ) {
                    tile.RedrawFogs( dst, friendColors, *this );
                }
            }
        }
    }

    updateObjectAnimationInfo();
}

void Interface::GameArea::Scroll()
{
    const int32_t shift = 2 << Settings::Get().ScrollSpeed();
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

    scrollDirection = 0;
}

void Interface::GameArea::SetRedraw() const
{
    interface.SetRedraw( REDRAW_GAMEAREA );
}

void Interface::GameArea::SetCenter( const fheroes2::Point & pt )
{
    _setCenterToTile( pt );

    scrollDirection = 0;
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

    gamearea.Redraw( result, LEVEL_BOTTOM | LEVEL_TOP, true );

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

void Interface::GameArea::QueueEventProcessing()
{
    LocalEvent & le = LocalEvent::Get();
    const fheroes2::Point & mp = le.GetMouseCursor();

    int32_t index = GetValidTileIdFromPoint( mp );

    // change cusor if need
    if ( updateCursor || index != _prevIndexPos ) {
        Cursor::Get().SetThemes( Interface::Basic::GetCursorTileIndex( index ) );
        _prevIndexPos = index;
        updateCursor = false;
    }

    // out of range
    if ( index < 0 )
        return;

    const Settings & conf = Settings::Get();
    if ( conf.ExtGameHideInterface() && conf.ShowControlPanel() && le.MouseCursor( interface.GetControlPanel().GetArea() ) )
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

fheroes2::Point Interface::GameArea::_middlePoint() const
{
    return { _windowROI.width / 2, _windowROI.height / 2 };
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
    return fheroes2::Point( tileId.x * TILEWIDTH - _topLeftTileOffset.x + _windowROI.x, tileId.y * TILEWIDTH - _topLeftTileOffset.y + _windowROI.y );
}

fheroes2::Point Interface::GameArea::getCurrentCenterInPixels() const
{
    return _topLeftTileOffset + _middlePoint();
}

void Interface::GameArea::addObjectAnimationInfo( std::shared_ptr<BaseObjectAnimationInfo> info )
{
    _animationInfo.emplace_back( std::move( info ) );
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

void Interface::GameArea::runFadingAnimation( const std::shared_ptr<BaseObjectAnimationInfo> info )
{
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
