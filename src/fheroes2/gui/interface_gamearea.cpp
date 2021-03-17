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

#include "interface_gamearea.h"

#include "agg_image.h"
#include "game.h"
#include "game_interface.h"
#include "ground.h"
#include "icn.h"
#include "logging.h"
#include "maps.h"
#include "pal.h"
#include "route.h"
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

Rect Interface::GameArea::GetVisibleTileROI( void ) const
{
    return Rect( _getStartTileId(), _visibleTileCount );
}

void Interface::GameArea::ShiftCenter( const Point & offset )
{
    SetCenterInPixels( _topLeftTileOffset + _middlePoint() + offset );
}

Rect Interface::GameArea::RectFixed( Point & dst, int rw, int rh ) const
{
    std::pair<Rect, Point> res = Rect::Fixed4Blit( Rect( dst.x, dst.y, rw, rh ), GetROI() );
    dst = res.second;
    return res.first;
}

void Interface::GameArea::Build( void )
{
    const fheroes2::Display & display = fheroes2::Display::instance();

    if ( Settings::Get().ExtGameHideInterface() )
        SetAreaPosition( 0, 0, display.width(), display.height() );
    else
        SetAreaPosition( BORDERWIDTH, BORDERWIDTH, display.width() - RADARWIDTH - 3 * BORDERWIDTH, display.height() - 2 * BORDERWIDTH );
}

void Interface::GameArea::SetAreaPosition( s32 x, s32 y, u32 w, u32 h )
{
    _windowROI = Rect( x, y, w, h );
    const Size worldSize = Size( world.w() * TILEWIDTH, world.h() * TILEWIDTH );

    if ( worldSize.w > w ) {
        _minLeftOffset = -static_cast<int16_t>( w / 2 ) - TILEWIDTH / 2;
        _maxLeftOffset = worldSize.w - w / 2;
    }
    else {
        _minLeftOffset = -static_cast<int16_t>( w - worldSize.w ) / 2;
        _maxLeftOffset = _minLeftOffset;
    }

    if ( worldSize.h > h ) {
        _minTopOffset = -static_cast<int16_t>( h / 2 ) - TILEWIDTH / 2;
        _maxTopOffset = worldSize.h - h / 2;
    }
    else {
        _minTopOffset = -static_cast<int16_t>( h - worldSize.h ) / 2;
        _maxTopOffset = _minTopOffset;
    }

    // adding 1 extra tile for both axes in case of drawing tiles partially near sides
    _visibleTileCount = Size( ( w + TILEWIDTH - 1 ) / TILEWIDTH + 1, ( h + TILEWIDTH - 1 ) / TILEWIDTH + 1 );

    _setCenterToTile( Point( world.w() / 2, world.h() / 2 ) );
}

void Interface::GameArea::BlitOnTile( fheroes2::Image & dst, const fheroes2::Sprite & src, const Point & mp ) const
{
    BlitOnTile( dst, src, src.x(), src.y(), mp );
}

void Interface::GameArea::BlitOnTile( fheroes2::Image & dst, const fheroes2::Image & src, int32_t ox, int32_t oy, const Point & mp, bool flip, uint8_t alpha ) const
{
    Point dstpt = GetRelativeTilePosition( mp ) + Point( ox, oy );

    const int32_t width = src.width();
    const int32_t height = src.height();

    // In most of cases objects locate within window ROI so we don't need to calculate truncated ROI
    if ( dstpt.x >= _windowROI.x && dstpt.y >= _windowROI.y && dstpt.x + width <= _windowROI.x + _windowROI.w && dstpt.y + height <= _windowROI.y + _windowROI.h ) {
        fheroes2::AlphaBlit( src, 0, 0, dst, dstpt.x, dstpt.y, width, height, alpha, flip );
    }
    else if ( _windowROI & Rect( dstpt, width, height ) ) {
        const Rect & fixedRect = RectFixed( dstpt, width, height );
        fheroes2::AlphaBlit( src, fixedRect.x, fixedRect.y, dst, dstpt.x, dstpt.y, fixedRect.w, fixedRect.h, alpha, flip );
    }
}

void Interface::GameArea::DrawTile( fheroes2::Image & dst, const fheroes2::Image & src, const Point & mp ) const
{
    Point dstpt = GetRelativeTilePosition( mp );

    const int32_t width = src.width();
    const int32_t height = src.height();

    // In most of cases objects locate within window ROI so we don't need to calculate truncated ROI
    if ( dstpt.x >= _windowROI.x && dstpt.y >= _windowROI.y && dstpt.x + width <= _windowROI.x + _windowROI.w && dstpt.y + height <= _windowROI.y + _windowROI.h ) {
        fheroes2::Copy( src, 0, 0, dst, dstpt.x, dstpt.y, width, height );
    }
    else if ( _windowROI & Rect( dstpt, width, height ) ) {
        const Rect & fixedRect = RectFixed( dstpt, width, height );
        fheroes2::Copy( src, fixedRect.x, fixedRect.y, dst, dstpt.x, dstpt.y, fixedRect.w, fixedRect.h );
    }
}

void Interface::GameArea::Redraw( fheroes2::Image & dst, int flag, bool isPuzzleDraw ) const
{
    const Rect tileROI = GetVisibleTileROI();

    int32_t minX = tileROI.x;
    int32_t minY = tileROI.y;
    int32_t maxX = tileROI.x + tileROI.w;
    int32_t maxY = tileROI.y + tileROI.h;

    // Ground level. Also find range of X and Y tile positions.
    for ( int32_t y = 0; y < tileROI.h; ++y ) {
        Point offset( tileROI.x, tileROI.y + y );

        if ( offset.y < 0 || offset.y >= world.h() ) {
            for ( ; offset.x < maxX; ++offset.x ) {
                Maps::Tiles::RedrawEmptyTile( dst, offset, tileROI );
            }
        }
        else {
            for ( ; offset.x < maxX; ++offset.x ) {
                if ( offset.x < 0 || offset.x >= world.w() ) {
                    Maps::Tiles::RedrawEmptyTile( dst, offset, tileROI );
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

    // Bottom layer and objects.
    const bool drawBottom = ( flag & LEVEL_BOTTOM ) == LEVEL_BOTTOM;
    if ( drawBottom ) {
        for ( int32_t y = minY; y < maxY; ++y ) {
            for ( int32_t x = minX; x < maxX; ++x ) {
                const Maps::Tiles & tile = world.GetTiles( x, y );
                tile.RedrawBottom( dst, tileROI, isPuzzleDraw, *this );
                tile.RedrawObjects( dst, isPuzzleDraw, *this );
            }
        }
    }

    const auto & fadeTask = Game::ObjectFadeAnimation::GetFadeTask();

    // fade out animation for objects only
    if ( drawBottom && fadeTask.fadeOut && MP2::OBJ_ZERO != fadeTask.object && MP2::OBJ_BOAT != fadeTask.object && MP2::OBJ_MONSTER != fadeTask.object ) {
        const int icn = MP2::GetICNObject( fadeTask.objectTileset );
        const Point & mp = Maps::GetPoint( fadeTask.fromIndex );

        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( icn, fadeTask.objectIndex );
        BlitOnTile( dst, sprite, sprite.x(), sprite.y(), mp, false, fadeTask.alpha );

        // possible animation
        if ( fadeTask.animationIndex ) {
            const fheroes2::Sprite & animationSprite = fheroes2::AGG::GetICN( icn, fadeTask.animationIndex );
            BlitOnTile( dst, animationSprite, animationSprite.x(), animationSprite.y(), mp, false, fadeTask.alpha );
        }
    }

    // Monsters and boats.
    const bool drawMonstersAndBoats = ( flag & LEVEL_OBJECTS ) && !isPuzzleDraw;
    if ( drawMonstersAndBoats ) {
        for ( int32_t y = minY; y < maxY; ++y ) {
            for ( int32_t x = minX; x < maxX; ++x ) {
                world.GetTiles( x, y ).RedrawMonstersAndBoat( dst, tileROI, true, *this );
            }
        }

        // fade out animation for monsters only
        if ( MP2::OBJ_MONSTER == fadeTask.object && fadeTask.fadeOut ) {
            const Point & mp = Maps::GetPoint( fadeTask.fromIndex );
            const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::MINIMON, fadeTask.objectIndex );
            BlitOnTile( dst, sprite, sprite.x() + 16, sprite.y() + TILEWIDTH, mp, false, fadeTask.alpha );

            if ( fadeTask.animationIndex ) {
                const fheroes2::Sprite & animatedSprite = fheroes2::AGG::GetICN( ICN::MINIMON, fadeTask.animationIndex );
                BlitOnTile( dst, animatedSprite, animatedSprite.x() + 16, animatedSprite.y() + TILEWIDTH, mp, false, fadeTask.alpha );
            }
        }
    }

    // Top layer and heroes.
    const bool drawTop = ( flag & LEVEL_TOP ) == LEVEL_TOP;
    const bool drawHeroes = ( flag & LEVEL_HEROES ) == LEVEL_HEROES;
    std::vector<std::pair<Point, const Heroes *> > heroList;

    for ( int32_t y = minY; y < maxY; ++y ) {
        for ( int32_t x = minX; x < maxX; ++x ) {
            const Maps::Tiles & tile = world.GetTiles( x, y );

            // top
            if ( drawTop )
                tile.RedrawTop( dst, tileROI, *this );

            // heroes will be drawn later
            if ( tile.GetObject() == MP2::OBJ_HEROES && drawHeroes ) {
                const Heroes * hero = tile.GetHeroes();
                if ( hero ) {
                    heroList.emplace_back( GetRelativeTilePosition( Point( x, y ) ), hero );
                }
            }
        }
    }

    for ( const std::pair<Point, const Heroes *> & hero : heroList ) {
        hero.second->Redraw( dst, hero.first.x, hero.first.y - 1, tileROI, true, *this );
    }

    // Route
    const Heroes * hero = drawHeroes ? GetFocusHeroes() : NULL;
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
            const int32_t from = ( *currentStep ).GetIndex();
            const Point & mp = Maps::GetPoint( from );

            ++nextStep;
            --green;

            // is visible
            if ( ( tileROI & mp ) && !( currentStep == path.begin() && skipfirst ) ) {
                uint32_t index = 0;
                if ( pathEnd != nextStep ) {
                    const Maps::Tiles & tileTo = world.GetTiles( currentStep->GetIndex() );
                    uint32_t cost = Maps::Ground::GetPenalty( tileTo, pathfinding );

                    if ( world.GetTiles( currentStep->GetFrom() ).isRoad() && tileTo.isRoad() )
                        cost = Maps::Ground::roadPenalty;

                    index = Route::Path::GetIndexSprite( ( *currentStep ).GetDirection(), ( *nextStep ).GetDirection(), cost );
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
                    world.GetTiles( x, y ).RedrawPassable( dst, tileROI );
                }
            }
        }
    }
    else
#endif
        // redraw fog
        if ( flag & LEVEL_FOG ) {
        const int colors = Players::FriendColors();

        for ( int32_t y = minY; y < maxY; ++y ) {
            for ( int32_t x = minX; x < maxX; ++x ) {
                const Maps::Tiles & tile = world.GetTiles( x, y );

                if ( tile.isFog( colors ) )
                    tile.RedrawFogs( dst, colors, *this );
            }
        }
    }
}

void Interface::GameArea::Scroll( void )
{
    const int16_t speed = Settings::Get().ScrollSpeed();
    Point offset;

    if ( scrollDirection & SCROLL_LEFT ) {
        offset.x = -speed;
    }
    else if ( scrollDirection & SCROLL_RIGHT ) {
        offset.x = speed;
    }

    if ( scrollDirection & SCROLL_TOP ) {
        offset.y = -speed;
    }
    else if ( scrollDirection & SCROLL_BOTTOM ) {
        offset.y = speed;
    }

    ShiftCenter( offset );

    scrollDirection = 0;
}

void Interface::GameArea::SetRedraw( void ) const
{
    interface.SetRedraw( REDRAW_GAMEAREA );
}

/* scroll area to center point maps */
void Interface::GameArea::SetCenter( const Point & pt )
{
    _setCenterToTile( pt );

    scrollDirection = 0;
}

fheroes2::Image Interface::GameArea::GenerateUltimateArtifactAreaSurface( int32_t index )
{
    if ( !Maps::isValidAbsIndex( index ) ) {
        DEBUG_LOG( DBG_ENGINE, DBG_WARN, "artifact not found" );
        return fheroes2::Image();
    }

    fheroes2::Image result( 448, 448 );
    result.reset();

    GameArea & gamearea = Basic::Get().GetGameArea();
    const Rect origPosition( gamearea._windowROI );
    gamearea.SetAreaPosition( 0, 0, result.width(), result.height() );

    Point pt = Maps::GetPoint( index );
    gamearea.SetCenter( pt );

    gamearea.Redraw( result, LEVEL_BOTTOM | LEVEL_TOP, true );

    const fheroes2::Sprite & marker = fheroes2::AGG::GetICN( ICN::ROUTE, 0 );
    const Point markerPos( gamearea.GetRelativeTilePosition( pt ) - gamearea._middlePoint() - Point( gamearea._windowROI.x, gamearea._windowROI.y )
                           + Point( result.width() / 2, result.height() / 2 ) );

    fheroes2::Blit( marker, result, markerPos.x, markerPos.y + 8 );
    fheroes2::ApplyPalette( result, PAL::GetPalette( PAL::PaletteType::TAN ) );

    gamearea.SetAreaPosition( origPosition.x, origPosition.y, origPosition.w, origPosition.h );

    return result;
}

int Interface::GameArea::GetScrollCursor( void ) const
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

void Interface::GameArea::QueueEventProcessing( void )
{
    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();
    const Point & mp = le.GetMouseCursor();

    int32_t index = GetValidTileIdFromPoint( mp );

    // change cusor if need
    if ( updateCursor || index != _prevIndexPos ) {
        cursor.SetThemes( Interface::Basic::GetCursorTileIndex( index ) );
        _prevIndexPos = index;
        updateCursor = false;
    }

    // out of range
    if ( index < 0 )
        return;

    const Settings & conf = Settings::Get();

    // fixed pocket pc tap mode
    if ( conf.ExtGameHideInterface() && conf.ShowControlPanel() && le.MouseCursor( interface.GetControlPanel().GetArea() ) )
        return;

    const Point tileOffset = _topLeftTileOffset + mp - Point( _windowROI.x, _windowROI.y );
    const Point tilePos( ( tileOffset.x / TILEWIDTH ) * TILEWIDTH - _topLeftTileOffset.x + _windowROI.x,
                         ( tileOffset.y / TILEWIDTH ) * TILEWIDTH - _topLeftTileOffset.y + _windowROI.x );

    const Rect tileROI( tilePos.x, tilePos.y, TILEWIDTH, TILEWIDTH );

    if ( le.MouseClickLeft( tileROI ) )
        interface.MouseCursorAreaClickLeft( index );
    else if ( le.MousePressRight( tileROI ) )
        interface.MouseCursorAreaPressRight( index );
}

Point Interface::GameArea::_middlePoint() const
{
    return Point( _windowROI.w / 2, _windowROI.h / 2 );
}

Point Interface::GameArea::_getStartTileId() const
{
    const int16_t x = ( _topLeftTileOffset.x < 0 ? ( _topLeftTileOffset.x - TILEWIDTH - 1 ) / TILEWIDTH : _topLeftTileOffset.x / TILEWIDTH );
    const int16_t y = ( _topLeftTileOffset.y < 0 ? ( _topLeftTileOffset.y - TILEWIDTH - 1 ) / TILEWIDTH : _topLeftTileOffset.y / TILEWIDTH );

    return Point( x, y );
}

void Interface::GameArea::_setCenterToTile( const Point & tile )
{
    SetCenterInPixels( Point( tile.x * TILEWIDTH + TILEWIDTH / 2, tile.y * TILEWIDTH + TILEWIDTH / 2 ) );
}

void Interface::GameArea::SetCenterInPixels( const Point & point )
{
    int16_t offsetX = point.x - _middlePoint().x;
    int16_t offsetY = point.y - _middlePoint().y;
    if ( offsetX < _minLeftOffset )
        offsetX = _minLeftOffset;
    else if ( offsetX > _maxLeftOffset )
        offsetX = _maxLeftOffset;

    if ( offsetY < _minTopOffset )
        offsetY = _minTopOffset;
    else if ( offsetY > _maxTopOffset )
        offsetY = _maxTopOffset;

    _topLeftTileOffset = Point( offsetX, offsetY );
}

int32_t Interface::GameArea::GetValidTileIdFromPoint( const Point & point ) const
{
    const Point offset = _topLeftTileOffset + point - Point( _windowROI.x, _windowROI.y );
    if ( offset.x < 0 || offset.y < 0 )
        return -1;

    const int16_t x = offset.x / TILEWIDTH;
    const int16_t y = offset.y / TILEWIDTH;

    if ( x >= world.w() || y >= world.h() )
        return -1;

    return y * world.w() + x;
}

Point Interface::GameArea::GetRelativeTilePosition( const Point & tileId ) const
{
    return Point( tileId.x * TILEWIDTH - _topLeftTileOffset.x + _windowROI.x, tileId.y * TILEWIDTH - _topLeftTileOffset.y + _windowROI.y );
}
