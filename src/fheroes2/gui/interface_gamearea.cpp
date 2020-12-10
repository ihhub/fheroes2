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

#include "agg.h"
#include "game.h"
#include "game_interface.h"
#include "ground.h"
#include "maps.h"
#include "pal.h"
#include "route.h"
#include "settings.h"
#include "world.h"

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
    _setCenter( _topLeftTileOffset + _middlePoint() + offset );
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
        fheroes2::Blit( src, fixedRect.x, fixedRect.y, dst, dstpt.x, dstpt.y, fixedRect.w, fixedRect.h );
    }
}

void Interface::GameArea::Redraw( fheroes2::Image & dst, int flag, bool isPuzzleDraw ) const
{
    const Rect tileROI = GetVisibleTileROI();

    // ground and bottom layer
    const bool drawBottom = ( flag & LEVEL_BOTTOM ) == LEVEL_BOTTOM;

    for ( int16_t y = 0; y < tileROI.h; ++y ) {
        Point offset( 0, tileROI.y + y );

        if ( offset.y < 0 || offset.y >= world.h() ) {
            for ( s32 x = 0; x < tileROI.w; ++x ) {
                offset.x = tileROI.x + x;
                Maps::Tiles::RedrawEmptyTile( dst, offset );
            }
        }
        else {
            for ( s32 x = 0; x < tileROI.w; ++x ) {
                offset.x = tileROI.x + x;

                if ( offset.x < 0 || offset.x >= world.w() ) {
                    Maps::Tiles::RedrawEmptyTile( dst, offset );
                }
                else {
                    const Maps::Tiles & tile = world.GetTiles( offset.x, offset.y );

                    tile.RedrawTile( dst );

                    // bottom and objects
                    if ( drawBottom ) {
                        tile.RedrawBottom( dst, isPuzzleDraw );
                        tile.RedrawObjects( dst, isPuzzleDraw );
                    }
                }
            }
        }
    }

    // objects
    const bool drawMonstersAndBoats = ( flag & LEVEL_OBJECTS ) && !isPuzzleDraw;
    if ( drawMonstersAndBoats ) {
        for ( int16_t y = 0; y < tileROI.h; ++y ) {
            const int32_t offsetY = tileROI.y + y;
            if ( offsetY < 0 || offsetY >= world.h() )
                continue;
            for ( s32 x = 0; x < tileROI.w; ++x ) {
                const int32_t offsetX = tileROI.x + x;
                if ( offsetX < 0 || offsetX >= world.w() )
                    continue;

                const Maps::Tiles & tile = world.GetTiles( offsetX, offsetY );
                tile.RedrawMonstersAndBoat( dst );
            }
        }
    }

    // top layer
    const bool drawTop = ( flag & LEVEL_TOP ) == LEVEL_TOP;
    const bool drawHeroes = ( flag & LEVEL_HEROES ) == LEVEL_HEROES;
    std::vector<std::pair<Point, const Heroes *> > heroList;

    for ( int16_t y = 0; y < tileROI.h; ++y ) {
        const int32_t offsetY = tileROI.y + y;
        if ( offsetY < 0 || offsetY >= world.h() )
            continue;
        for ( s32 x = 0; x < tileROI.w; ++x ) {
            const int32_t offsetX = tileROI.x + x;
            if ( offsetX < 0 || offsetX >= world.w() )
                continue;

            const Maps::Tiles & tile = world.GetTiles( offsetX, offsetY );

            // top
            if ( drawTop )
                tile.RedrawTop( dst );

            // heroes will be drawn later
            if ( tile.GetObject() == MP2::OBJ_HEROES && drawHeroes ) {
                const Heroes * hero = tile.GetHeroes();
                if ( hero ) {
                    heroList.emplace_back( GetRelativeTilePosition( Point( offsetX, offsetY ) ), hero );
                }
            }
        }
    }

    // object fade in/fade out animation
    Game::ObjectFadeAnimation::Info & fadeInfo = Game::ObjectFadeAnimation::Get();
    if ( fadeInfo.object != MP2::OBJ_ZERO ) {
        const Point & mp = Maps::GetPoint( fadeInfo.tile );
        const int icn = MP2::GetICNObject( fadeInfo.object );

        if ( icn == ICN::MONS32 ) {
            const std::pair<int, int> monsterIndicies = Maps::Tiles::GetMonsterSpriteIndices( world.GetTiles( fadeInfo.tile ), fadeInfo.index );

            // base monster sprite
            if ( monsterIndicies.first >= 0 ) {
                const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::MINIMON, monsterIndicies.first );
                BlitOnTile( dst, sprite, sprite.x() + 16, sprite.y() + TILEWIDTH, mp, false, fadeInfo.alpha );
            }
            // animated monster part
            if ( monsterIndicies.second >= 0 ) {
                const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::MINIMON, monsterIndicies.second );
                BlitOnTile( dst, sprite, sprite.x() + 16, sprite.y() + TILEWIDTH, mp, false, fadeInfo.alpha );
            }
        }
        else if ( fadeInfo.object == MP2::OBJ_BOAT ) {
            const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::BOAT32, fadeInfo.index );
            BlitOnTile( dst, sprite, sprite.x(), sprite.y() + TILEWIDTH - 11, mp, false, fadeInfo.alpha );
        }
        else {
            const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( icn, fadeInfo.index );
            BlitOnTile( dst, sprite, sprite.x(), sprite.y(), mp, false, fadeInfo.alpha );
        }
    }

    for ( const std::pair<Point, const Heroes *> & hero : heroList ) {
        hero.second->Redraw( dst, hero.first.x, hero.first.y - 1, true );
    }

    // route
    const Heroes * hero = drawHeroes ? GetFocusHeroes() : NULL;

    if ( hero && hero->GetPath().isShow() ) {
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
            for ( int16_t y = 0; y < tileROI.h; ++y ) {
                const s32 offsetY = tileROI.y + y;
                if ( offsetY < 0 || offsetY >= world.h() )
                    continue;
                for ( s32 x = 0; x < tileROI.w; ++x ) {
                    const s32 offsetX = tileROI.x + x;
                    if ( offsetX < 0 || offsetX >= world.w() )
                        continue;

                    world.GetTiles( offsetX, offsetY ).RedrawPassable( dst );
                }
            }
        }
    }
    else
#endif
        // redraw fog
        if ( flag & LEVEL_FOG ) {
        const int colors = Players::FriendColors();

        for ( int16_t y = 0; y < tileROI.h; ++y ) {
            const s32 offsetY = tileROI.y + y;
            if ( offsetY < 0 || offsetY >= world.h() )
                continue;
            for ( s32 x = 0; x < tileROI.w; ++x ) {
                const s32 offsetX = tileROI.x + x;
                if ( offsetX < 0 || offsetX >= world.w() )
                    continue;

                const Maps::Tiles & tile = world.GetTiles( offsetX, offsetY );

                if ( tile.isFog( colors ) )
                    tile.RedrawFogs( dst, colors );
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
        DEBUG( DBG_ENGINE, DBG_WARN, "artifact not found" );
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
    fheroes2::ApplyPalette( result, PAL::GetPalette( PAL::TAN ) );

    gamearea.SetAreaPosition( origPosition.x, origPosition.y, origPosition.w, origPosition.h );

    return result;
}

bool Interface::GameArea::NeedScroll( void ) const
{
    return scrollDirection != 0;
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

    scrollTime.Start();
}

void Interface::GameArea::SetUpdateCursor( void )
{
    updateCursor = true;
}

void Interface::GameArea::QueueEventProcessing( void )
{
    const Settings & conf = Settings::Get();
    fheroes2::Display & display = fheroes2::Display::instance();
    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();
    const Point & mp = le.GetMouseCursor();

    s32 index = GetValidTileIdFromPoint( mp );

    // change cusor if need
    if ( updateCursor || index != _prevIndexPos ) {
        cursor.SetThemes( interface.GetCursorTileIndex( index ) );
        _prevIndexPos = index;
        updateCursor = false;
    }

    // out of range
    if ( index < 0 )
        return;

    // fixed pocket pc tap mode
    if ( conf.ExtGameHideInterface() && conf.ShowControlPanel() && le.MouseCursor( interface.GetControlPanel().GetArea() ) )
        return;

    if ( conf.ExtPocketTapMode() ) {
        // drag&drop gamearea: scroll
        if ( conf.ExtPocketDragDropScroll() && le.MousePressLeft() ) {
            Point pt1 = le.GetMouseCursor();
            const int16_t speed = Settings::Get().ScrollSpeed();

            while ( le.HandleEvents() && le.MousePressLeft() ) {
                const Point & pt2 = le.GetMouseCursor();

                if ( pt1 != pt2 ) {
                    s32 dx = pt2.x - pt1.x;
                    s32 dy = pt2.y - pt1.y;
                    s32 d2x = speed;
                    s32 d2y = speed;

                    while ( 1 ) {
                        if ( d2x <= dx ) {
                            SetScroll( SCROLL_LEFT );
                            dx -= d2x;
                        }
                        else if ( -d2x >= dx ) {
                            SetScroll( SCROLL_RIGHT );
                            dx += d2x;
                        }

                        if ( d2y <= dy ) {
                            SetScroll( SCROLL_TOP );
                            dy -= d2y;
                        }
                        else if ( -d2y >= dy ) {
                            SetScroll( SCROLL_BOTTOM );
                            dy += d2y;
                        }

                        if ( NeedScroll() ) {
                            cursor.Hide();
                            Scroll();
                            interface.SetRedraw( REDRAW_GAMEAREA );
                            interface.Redraw();
                            cursor.Show();
                            display.render();
                        }
                        else
                            break;
                    }
                }
            }
        }

        // fixed pocket pc: click on maps after scroll (pause: ~800 ms)
        scrollTime.Stop();
        if ( 800 > scrollTime.Get() )
            return;
    }

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
    _setCenter( Point( tile.x * TILEWIDTH + TILEWIDTH / 2, tile.y * TILEWIDTH + TILEWIDTH / 2 ) );
}

void Interface::GameArea::_setCenter( const Point & point )
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

void Interface::GameArea::ResetCursorPosition()
{
    _prevIndexPos = -1;
}
