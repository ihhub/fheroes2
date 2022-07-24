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

void Interface::GameArea::BlitOnTile( fheroes2::Image & dst, const fheroes2::Sprite & src, const fheroes2::Point & mp ) const
{
    BlitOnTile( dst, src, src.x(), src.y(), mp );
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
    // 1 - background objects like lake or bushes.
    // 2 - shadows
    // 3 - roads, water flaws and cracks. Essentially everything what is a part of terrain.
    // The correct order of levels is 3 --> 1 --> 2 --> 0.
    //
    // There are also two groups of objects: ground objects (bottom layer) and high objects (top layer). High objects are the parts of the objects which are taller than
    // 1 tile. For example, a castle. All ground objects are drawn first.
    //
    // However, there are some objects which appear to be more than 1 tile (32 x 32 pixels) size such as heroes, monsters and boats.
    // To render all these 'special' objects we need to create a copy of object sprite stacks for each tile, add temporary extra sprites and render them.
    //
    // TODO: to proceed with this concept we need to put an object info stored in class Tiles into either groud object stack or high object stack. For example, a tile
    // TODO: which contains only one top castle sprite would have data only in Tiles class but a hero could be at the same tile. To correctly render objects we need to
    // TODO: render the hero first and only then render castle's sprite. Side note: from the map format Tiles class must contain only objects from level 1.
    //
    // TODO: take into account that objects on the map such as monsters, heroes, boats, resources can have a fading effect so it is advisable to keep alpha value for
    // TODO: each tile sprite.

    std::vector<const Maps::Tiles *> drawList;
    std::vector<const Maps::Tiles *> monsterList;
    std::vector<const Maps::Tiles *> topList;
    std::vector<const Maps::Tiles *> objectList;
    std::vector<const Maps::Tiles *> fogList;

    const int32_t areaSize = ( maxY - minY ) * ( maxX - minX );
    topList.reserve( areaSize );
    objectList.reserve( areaSize );

    // Bottom layer and objects.
    const bool drawBottom = ( flag & LEVEL_BOTTOM ) == LEVEL_BOTTOM;
    const bool drawMonstersAndBoats = ( flag & LEVEL_OBJECTS ) && !isPuzzleDraw;
    const bool drawHeroes = ( flag & LEVEL_HEROES ) == LEVEL_HEROES;
    const bool drawTop = ( flag & LEVEL_TOP ) == LEVEL_TOP;
#ifdef WITH_DEBUG
    const bool drawFog = ( ( flag & LEVEL_FOG ) == LEVEL_FOG ) && !IS_DEVEL();
#else
    const bool drawFog = ( flag & LEVEL_FOG ) == LEVEL_FOG;
#endif

    const int friendColors = Players::FriendColors();

    for ( int32_t y = minY; y < maxY; ++y ) {
        for ( int32_t x = minX; x < maxX; ++x ) {
            const Maps::Tiles & tile = world.GetTiles( x, y );

            if ( drawFog && tile.isFog( friendColors ) ) {
                // don't redraw tile if fog all around
                fogList.emplace_back( &tile );
                if ( tile.isFogAllAround( friendColors ) ) {
                    continue;
                }
            }

            const MP2::MapObjectType objectType = tile.GetObject();

            switch ( objectType ) {
            case MP2::OBJ_ZERO: {
                if ( drawBottom ) {
                    tile.RedrawBottom( dst, tileROI, isPuzzleDraw, *this );
                    const uint8_t objectTileset = tile.GetObjectTileset();
                    const int icn = MP2::GetICNObject( objectTileset );
                    if ( ICN::UNKNOWN != icn && ( !isPuzzleDraw || !MP2::isHiddenForPuzzle( objectTileset, tile.GetObjectSpriteIndex() ) ) ) {
                        objectList.emplace_back( &tile );
                    }
                }
                if ( drawTop ) {
                    topList.emplace_back( &tile );
                }
                break;
            }
            case MP2::OBJ_BOAT: {
                if ( drawBottom ) {
                    tile.RedrawBottom( dst, tileROI, isPuzzleDraw, *this );
                }
                if ( drawMonstersAndBoats ) {
                    drawList.emplace_back( &tile );
                }
                else if ( drawTop ) {
                    topList.emplace_back( &tile );
                }
                break;
            }
            case MP2::OBJ_MONSTER: {
                if ( drawBottom ) {
                    tile.RedrawBottom( dst, tileROI, isPuzzleDraw, *this );
                }
                if ( drawTop ) {
                    topList.emplace_back( &tile );
                }
                if ( drawMonstersAndBoats ) {
                    monsterList.emplace_back( &tile );
                }
                break;
            }
            case MP2::OBJ_HEROES: {
                if ( drawBottom ) {
                    tile.RedrawBottom( dst, tileROI, isPuzzleDraw, *this );
                    if ( !isPuzzleDraw || !MP2::isHiddenForPuzzle( tile.GetObjectTileset(), tile.GetObjectSpriteIndex() ) ) {
                        objectList.emplace_back( &tile );
                    }
                }
                if ( drawHeroes ) {
                    drawList.emplace_back( &tile );
                    Heroes * hero = tile.GetHeroes();
                    if ( hero && ( drawTop || drawBottom ) ) {
                        hero->SetRedrawIndexes();
                    }
                }
                else if ( drawTop ) {
                    topList.emplace_back( &tile );
                }
                break;
            }
            default: {
                if ( drawBottom ) {
                    tile.RedrawBottom( dst, tileROI, isPuzzleDraw, *this );
                    if ( !isPuzzleDraw || !MP2::isHiddenForPuzzle( tile.GetObjectTileset(), tile.GetObjectSpriteIndex() ) ) {
                        objectList.emplace_back( &tile );
                    }
                }
                if ( drawTop ) {
                    topList.emplace_back( &tile );
                }
                break;
            }
            }
        }
    }

    for ( const Maps::Tiles * tile : drawList ) {
        Heroes * hero = tile->GetHeroes();
        if ( hero == nullptr ) {
            continue;
        }
        if ( drawTop ) {
            // looking for heroes nearby current hero
            // check and reset index for matching tiles for which we need to be redraw top layer
            const fheroes2::Point center = tile->GetCenter();
            if ( center.x + 1 < world.w() ) {
                hero->UpdateRedrawTop( world.GetTiles( center.x + 1, center.y ) );
            }
            if ( center.x > 0 ) {
                hero->UpdateRedrawTop( world.GetTiles( center.x - 1, center.y ) );
            }
            if ( center.y + 1 < world.h() ) {
                if ( center.x + 1 < world.w() ) {
                    hero->UpdateRedrawTop( world.GetTiles( center.x + 1, center.y + 1 ) );
                }
                if ( center.x > 0 ) {
                    hero->UpdateRedrawTop( world.GetTiles( center.x - 1, center.y + 1 ) );
                }
                hero->UpdateRedrawTop( world.GetTiles( center.x, center.y + 1 ) );
            }
            // remove a tile from topLits, if it will be drawn while drawing the hero
            const Heroes::RedrawIndex & redrawIndex = hero->GetRedrawIndex();
            if ( redrawIndex.topOnBottom > -1 ) {
                topList.erase( std::remove( topList.begin(), topList.end(), &world.GetTiles( redrawIndex.topOnBottom ) ), topList.end() );
            }
            if ( redrawIndex.topOnDirectionBottom > -1 ) {
                topList.erase( std::remove( topList.begin(), topList.end(), &world.GetTiles( redrawIndex.topOnDirectionBottom ) ), topList.end() );
            }
            if ( redrawIndex.topOnDirection > -1 ) {
                topList.erase( std::remove( topList.begin(), topList.end(), &world.GetTiles( redrawIndex.topOnDirection ) ), topList.end() );
            }
        }
        if ( drawBottom ) {
            const fheroes2::Point center = tile->GetCenter();
            if ( center.x + 1 < world.w() ) {
                hero->UpdateRedrawBottom( world.GetTiles( center.x + 1, center.y ) );
            }
            if ( center.x > 0 ) {
                hero->UpdateRedrawBottom( world.GetTiles( center.x - 1, center.y ) );
            }
            if ( center.y + 1 < world.h() ) {
                if ( center.x + 1 < world.w() ) {
                    hero->UpdateRedrawBottom( world.GetTiles( center.x + 1, center.y + 1 ) );
                }
                if ( center.x > 0 ) {
                    hero->UpdateRedrawBottom( world.GetTiles( center.x - 1, center.y + 1 ) );
                }
                hero->UpdateRedrawBottom( world.GetTiles( center.x, center.y + 1 ) );
            }
            const Heroes::RedrawIndex & redrawIndex = hero->GetRedrawIndex();
            if ( redrawIndex.objectsOnBottom > -1 ) {
                objectList.erase( std::remove( objectList.begin(), objectList.end(), &world.GetTiles( redrawIndex.objectsOnBottom ) ), objectList.end() );
            }
            if ( redrawIndex.objectsOnDirectionBottom > -1 ) {
                objectList.erase( std::remove( objectList.begin(), objectList.end(), &world.GetTiles( redrawIndex.objectsOnDirectionBottom ) ), objectList.end() );
            }
        }
    }

    for ( const Maps::Tiles * tile : objectList ) {
        tile->RedrawObjects( dst, isPuzzleDraw, *this );
    }

    for ( const Maps::Tiles * tile : drawList ) {
        const MP2::MapObjectType objectType = tile->GetObject();
        if ( drawHeroes && MP2::OBJ_HEROES == objectType ) {
            const Heroes * hero = tile->GetHeroes();
            if ( hero ) {
                hero->RedrawShadow( dst, GetRelativeTilePosition( tile->GetCenter() ), tileROI, *this );
            }
        }
        else if ( drawMonstersAndBoats && MP2::OBJ_BOAT == objectType ) {
            tile->RedrawBoatShadow( dst, tileROI, *this );
        }
    }

    const auto & fadeTask = Game::ObjectFadeAnimation::GetFadeTask();

    // fade out animation for objects only
    if ( drawBottom && fadeTask.fadeOut && MP2::OBJ_ZERO != fadeTask.object && MP2::OBJ_BOAT != fadeTask.object && MP2::OBJ_MONSTER != fadeTask.object ) {
        const int icn = MP2::GetICNObject( fadeTask.objectTileset );
        const fheroes2::Point & mp = Maps::GetPoint( fadeTask.fromIndex );

        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( icn, fadeTask.objectIndex );
        BlitOnTile( dst, sprite, sprite.x(), sprite.y(), mp, false, fadeTask.alpha );

        // possible animation
        if ( fadeTask.animationIndex ) {
            const fheroes2::Sprite & animationSprite = fheroes2::AGG::GetICN( icn, fadeTask.animationIndex );
            BlitOnTile( dst, animationSprite, animationSprite.x(), animationSprite.y(), mp, false, fadeTask.alpha );
        }
    }

    // Monsters.
    if ( drawMonstersAndBoats ) {
        for ( const Maps::Tiles * tile : monsterList ) {
            tile->RedrawMonster( dst, tileROI, *this );
        }

        // fade out animation for monsters only
        if ( MP2::OBJ_MONSTER == fadeTask.object && fadeTask.fadeOut ) {
            const fheroes2::Point & mp = Maps::GetPoint( fadeTask.fromIndex );
            const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::MINIMON, fadeTask.objectIndex );
            BlitOnTile( dst, sprite, sprite.x() + 16, sprite.y() + 30, mp, false, fadeTask.alpha );

            if ( fadeTask.animationIndex ) {
                const fheroes2::Sprite & animatedSprite = fheroes2::AGG::GetICN( ICN::MINIMON, fadeTask.animationIndex );
                BlitOnTile( dst, animatedSprite, animatedSprite.x() + 16, animatedSprite.y() + 30, mp, false, fadeTask.alpha );
            }
        }
    }

    // Top layer.
    for ( const Maps::Tiles * tile : topList ) {
        tile->RedrawTop( dst, tileROI, isPuzzleDraw, *this );
    }

    // Heroes and boats.
    if ( drawTop || drawBottom ) {
        for ( const Maps::Tiles * tile : drawList ) {
            const MP2::MapObjectType objectType = tile->GetObject();
            if ( drawHeroes && MP2::OBJ_HEROES == objectType ) {
                const Heroes * hero = tile->GetHeroes();
                if ( hero ) {
                    hero->Redraw( dst, GetRelativeTilePosition( tile->GetCenter() ), tileROI, *this );
                    if ( drawBottom ) {
                        hero->RedrawBottom( dst, tileROI, *this, isPuzzleDraw );
                    }
                    if ( drawTop ) {
                        hero->RedrawTop( dst, tileROI, *this );
                    }
                }
            }
            else if ( drawMonstersAndBoats && MP2::OBJ_BOAT == objectType ) {
                tile->RedrawBoat( dst, tileROI, *this );
                if ( drawTop ) {
                    tile->RedrawTop( dst, tileROI, isPuzzleDraw, *this );
                }
            }
        }
    }

    // Route
    const Heroes * hero = drawHeroes ? GetFocusHeroes() : nullptr;
    const bool drawRoutes = ( flag & LEVEL_ROUTES ) != 0;

    if ( hero && hero->GetPath().isShow() && drawRoutes ) {
        const Route::Path & path = hero->GetPath();
        int green = path.GetAllowedSteps();

        const int pathfinding = hero->GetLevelSkill( Skill::Secondary::PATHFINDING );
        const int heroSpriteIndex = hero->GetSpriteIndex();
        const bool skipfirst = hero->isMoveEnabled() && 45 > heroSpriteIndex && 2 < ( heroSpriteIndex % 9 );

        Route::Path::const_iterator pathEnd = path.end();
        Route::Path::const_iterator currentStep = path.begin();
        Route::Path::const_iterator nextStep = currentStep;

        for ( ; currentStep != pathEnd; ++currentStep ) {
            const fheroes2::Point & mp = Maps::GetPoint( currentStep->GetIndex() );

            ++nextStep;
            --green;

            // is visible
            if ( ( tileROI & mp ) && !( currentStep == path.begin() && skipfirst ) ) {
                uint32_t index = 0;

                if ( pathEnd != nextStep ) {
                    const Maps::Tiles & tile = world.GetTiles( currentStep->GetIndex() );

                    // Make no mistake: the cost of moving to this tile depends on the penalty of the PREVIOUS tile,
                    // BUT the length of the route arrow on this tile depends on the penalty of THIS tile
                    const uint32_t penalty = tile.isRoad() ? Maps::Ground::roadPenalty : Maps::Ground::GetPenalty( tile, pathfinding );

                    index = Route::Path::GetIndexSprite( currentStep->GetDirection(), nextStep->GetDirection(), penalty );
                }

                const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( 0 > green ? ICN::ROUTERED : ICN::ROUTE, index );
                BlitOnTile( dst, sprite, sprite.x() - 12, sprite.y() + 2, mp );
            }
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
        for ( const Maps::Tiles * tile : fogList ) {
            tile->RedrawFogs( dst, friendColors, *this );
        }
    }
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
        DEBUG_LOG( DBG_ENGINE, DBG_WARN, "artifact not found" )
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
