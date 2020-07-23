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

const Rect & Interface::GameArea::GetROI( void ) const
{
    return _windowROI;
}

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
    std::pair<Rect, Point> res = Rect::Fixed4Blit( Rect( dst.x, dst.y, rw, rh ), interface.GetGameArea().GetROI() );
    dst = res.second;
    return res.first;
}

void Interface::GameArea::Build( void )
{
    if ( Settings::Get().ExtGameHideInterface() )
        SetAreaPosition( 0, 0, Display::Get().w(), Display::Get().h() );
    else
        SetAreaPosition( BORDERWIDTH, BORDERWIDTH, Display::Get().w() - RADARWIDTH - 3 * BORDERWIDTH, Display::Get().h() - 2 * BORDERWIDTH );
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

void Interface::GameArea::UpdateCyclingPalette( int frame )
{
    const std::vector<uint8_t> & colorIndexes = PAL::GetCyclingPalette( frame );
    const std::vector<uint32_t> & rgbColors = PAL::GetRGBColors();

    if ( colorIndexes.size() == rgbColors.size() ) {
        PAL::SetCustomSDLPalette( colorIndexes );

        _cyclingRGBPalette.resize( colorIndexes.size() );
        for ( size_t i = 0; i < colorIndexes.size(); ++i )
            _cyclingRGBPalette[i] = rgbColors[colorIndexes[i]];

        // reset cache as we'll need to re-color tiles
        _spriteCache.clear();
    }
}

const std::vector<uint32_t> & Interface::GameArea::GetCyclingRGBPalette() const
{
    return _cyclingRGBPalette;
}

MapObjectSprite & Interface::GameArea::GetSpriteCache()
{
    return _spriteCache;
}

void Interface::GameArea::BlitOnTile( Surface & dst, const Sprite & src, const Point & mp ) const
{
    BlitOnTile( dst, src, src.x(), src.y(), mp );
}

void Interface::GameArea::BlitOnTile( Surface & dst, const Surface & src, s32 ox, s32 oy, const Point & mp ) const
{
    Point dstpt = GetRelativeTilePosition( mp ) + Point( ox, oy );

    if ( _windowROI & Rect( dstpt, src.w(), src.h() ) ) {
        src.Blit( RectFixed( dstpt, src.w(), src.h() ), dstpt, dst );
    }
}

void Interface::GameArea::Redraw( Surface & dst, int flag ) const
{
    const Rect tileROI = GetVisibleTileROI();

    // ground
    for ( int16_t y = 0; y < tileROI.h; ++y ) {
        const s32 offsetY = tileROI.y + y;
        bool isEmptyTile = offsetY < 0 || offsetY >= world.h();
        for ( s32 x = 0; x < tileROI.w; ++x ) {
            const s32 offsetX = tileROI.x + x;
            if ( isEmptyTile || offsetX < 0 || offsetX >= world.w() )
                Maps::Tiles::RedrawEmptyTile( dst, Point( offsetX, offsetY ) );
            else
                world.GetTiles( offsetX, offsetY ).RedrawTile( dst );
        }
    }

    // bottom
    if ( flag & LEVEL_BOTTOM ) {
        for ( int16_t y = 0; y < tileROI.h; ++y ) {
            const s32 offsetY = tileROI.y + y;
            if ( offsetY < 0 || offsetY >= world.h() )
                continue;
            for ( s32 x = 0; x < tileROI.w; ++x ) {
                const s32 offsetX = tileROI.x + x;
                if ( offsetX < 0 || offsetX >= world.w() )
                    continue;
                world.GetTiles( offsetX, offsetY ).RedrawBottom( dst, !( flag & LEVEL_OBJECTS ) );
            }
        }
    }

    // object fade in/fade out animation
    Game::ObjectFadeAnimation::Info & fadeInfo = Game::ObjectFadeAnimation::Get();
    if ( fadeInfo.object != MP2::OBJ_ZERO ) {
        const Point mp = Maps::GetPoint( fadeInfo.tile );
        const int icn = MP2::GetICNObject( fadeInfo.object );

        if ( icn == ICN::MONS32 ) {
            const std::pair<int, int> monsterIndicies = Maps::Tiles::GetMonsterSpriteIndices( world.GetTiles( fadeInfo.tile ), fadeInfo.index );

            // base monster sprite
            if ( monsterIndicies.first >= 0 ) {
                Sprite sprite = AGG::GetICN( ICN::MINIMON, monsterIndicies.first );
                sprite.SetAlphaMod( fadeInfo.alpha, true );
                BlitOnTile( dst, sprite, sprite.x() + 16, sprite.y() + TILEWIDTH, mp );
            }
            // animated monster part
            if ( monsterIndicies.second >= 0 ) {
                Sprite sprite = AGG::GetICN( ICN::MINIMON, monsterIndicies.second );
                sprite.SetAlphaMod( fadeInfo.alpha, true );
                BlitOnTile( dst, sprite, sprite.x() + 16, sprite.y() + TILEWIDTH, mp );
            }
        }
        else if ( fadeInfo.object == MP2::OBJ_BOAT ) {
            Sprite sprite = AGG::GetICN( ICN::BOAT32, fadeInfo.index );
            sprite.SetAlphaMod( fadeInfo.alpha, true );
            BlitOnTile( dst, sprite, sprite.x(), sprite.y() + TILEWIDTH, mp );
        }
        else {
            Sprite sprite = AGG::GetICN( icn, fadeInfo.index );
            sprite.SetAlphaMod( fadeInfo.alpha, true );
            BlitOnTile( dst, sprite, sprite.x(), sprite.y(), mp );
        }
    }

    // ext object
    if ( flag & LEVEL_OBJECTS ) {
        for ( int16_t y = 0; y < tileROI.h; ++y ) {
            const s32 offsetY = tileROI.y + y;
            if ( offsetY < 0 || offsetY >= world.h() )
                continue;
            for ( s32 x = 0; x < tileROI.w; ++x ) {
                const s32 offsetX = tileROI.x + x;
                if ( offsetX < 0 || offsetX >= world.w() )
                    continue;
                world.GetTiles( offsetX, offsetY ).RedrawObjects( dst );
            }
        }
    }

    // top
    if ( flag & LEVEL_TOP ) {
        for ( int16_t y = 0; y < tileROI.h; ++y ) {
            const s32 offsetY = tileROI.y + y;
            if ( offsetY < 0 || offsetY >= world.h() )
                continue;
            for ( s32 x = 0; x < tileROI.w; ++x ) {
                const s32 offsetX = tileROI.x + x;
                if ( offsetX < 0 || offsetX >= world.w() )
                    continue;
                world.GetTiles( offsetX, offsetY ).RedrawTop( dst );
            }
        }
    }

    // heroes
    for ( int16_t y = 0; y < tileROI.h; ++y ) {
        const s32 offsetY = tileROI.y + y;
        if ( offsetY < 0 || offsetY >= world.h() )
            continue;
        for ( s32 x = 0; x < tileROI.w; ++x ) {
            const s32 offsetX = tileROI.x + x;
            if ( offsetX < 0 || offsetX >= world.w() )
                continue;
            const Maps::Tiles & tile = world.GetTiles( offsetX, offsetY );

            if ( tile.GetObject() == MP2::OBJ_HEROES && ( flag & LEVEL_HEROES ) ) {
                const Heroes * hero = tile.GetHeroes();
                if ( hero ) {
                    const Point pos = GetRelativeTilePosition( Point( offsetX, offsetY ) );
                    hero->Redraw( dst, pos.x, pos.y, true );
                }
            }
        }
    }

    // route
    const Heroes * hero = flag & LEVEL_HEROES ? GetFocusHeroes() : NULL;

    if ( hero && hero->GetPath().isShow() ) {
        const Route::Path & path = hero->GetPath();
        s32 green = path.GetAllowStep();

        const int pathfinding = hero->GetLevelSkill( Skill::Secondary::PATHFINDING );
        const bool skipfirst = hero->isEnableMove() && 45 > hero->GetSpriteIndex() && 2 < ( hero->GetSpriteIndex() % 9 );

        Route::Path::const_iterator pathEnd = path.end();
        Route::Path::const_iterator currentStep = path.begin();
        Route::Path::const_iterator nextStep = currentStep;

        for ( ; currentStep != pathEnd; ++currentStep ) {
            const s32 & from = ( *currentStep ).GetIndex();
            const Point mp = Maps::GetPoint( from );

            ++nextStep;
            --green;

            // is visible
            if ( ( tileROI & mp ) && !( currentStep == path.begin() && skipfirst ) ) {
                uint32_t index = 0;
                if ( pathEnd != nextStep ) {
                    const uint32_t penaltyTo = Maps::Ground::GetPenalty( currentStep->GetFrom(), currentStep->GetDirection(), pathfinding, false );
                    const uint32_t penaltyReverse
                        = Maps::Ground::GetPenalty( currentStep->GetIndex(), Direction::Reflect( currentStep->GetDirection() ), pathfinding, false );

                    index = Route::Path::GetIndexSprite( ( *currentStep ).GetDirection(), ( *nextStep ).GetDirection(), std::min( penaltyTo, penaltyReverse ) );
                }

                const Sprite & sprite = AGG::GetICN( 0 > green ? ICN::ROUTERED : ICN::ROUTE, index );
                BlitOnTile( dst, sprite, sprite.x() - 14, sprite.y(), mp );
            }
        }
    }

#ifdef WITH_DEBUG
    if ( IS_DEVEL() ) {
        // redraw grid
        if ( flag & LEVEL_ALL ) {
            const RGBA col = RGBA( 0x90, 0xA4, 0xE0 );

            for ( int16_t y = 0; y < tileROI.h; ++y ) {
                const s32 offsetY = tileROI.y + y;
                if ( offsetY < 0 || offsetY >= world.h() )
                    continue;
                for ( s32 x = 0; x < tileROI.w; ++x ) {
                    const s32 offsetX = tileROI.x + x;
                    if ( offsetX < 0 || offsetX >= world.w() )
                        continue;

                    const Point pos = GetRelativeTilePosition( Point( offsetX, offsetY ) );
                    if ( _windowROI & pos )
                        dst.DrawPoint( pos, col );

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

Surface Interface::GameArea::GenerateUltimateArtifactAreaSurface( s32 index )
{
    Surface sf;

    if ( Maps::isValidAbsIndex( index ) ) {
        sf.Set( 448, 448, false );

        GameArea & gamearea = Basic::Get().GetGameArea();
        const Rect origPosition( gamearea._windowROI );
        gamearea.SetAreaPosition( 0, 0, sf.w(), sf.h() );

        const Rect & rectMaps = gamearea.GetVisibleTileROI();
        Point pt = Maps::GetPoint( index );

        gamearea.SetCenter( pt );
        gamearea.Redraw( sf, LEVEL_BOTTOM | LEVEL_TOP );

        // blit marker
        for ( u32 ii = 0; ii < rectMaps.h; ++ii )
            if ( index < Maps::GetIndexFromAbsPoint( rectMaps.x + rectMaps.w - 1, rectMaps.y + ii ) ) {
                pt.y = ii;
                break;
            }
        for ( u32 ii = 0; ii < rectMaps.w; ++ii )
            if ( index == Maps::GetIndexFromAbsPoint( rectMaps.x + ii, rectMaps.y + pt.y ) ) {
                pt.x = ii;
                break;
            }
        const Sprite & marker = AGG::GetICN( ICN::ROUTE, 0 );
        const Point markerPos( gamearea.GetRelativeTilePosition( pt ) - gamearea._middlePoint() - Point( gamearea._windowROI.x, gamearea._windowROI.y )
                               + Point( sf.w() / 2, sf.h() / 2 ) );

        marker.Blit( markerPos.x, markerPos.y + 8, sf );

        sf = ( Settings::Get().ExtGameEvilInterface() ? sf.RenderGrayScale() : sf.RenderSepia() );

        if ( Settings::Get().QVGA() )
            sf = Sprite::ScaleQVGASurface( sf );

        gamearea.SetAreaPosition( origPosition.x, origPosition.y, origPosition.w, origPosition.h );
    }
    else
        DEBUG( DBG_ENGINE, DBG_WARN, "artifact not found" );

    return sf;
}

bool Interface::GameArea::NeedScroll( void ) const
{
    return scrollDirection;
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
    Display & display = Display::Get();
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
                            display.Flip();
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
    return _getRelativePosition( Point( tileId.x * TILEWIDTH, tileId.y * TILEWIDTH ) );
}

Point Interface::GameArea::_getRelativePosition( const Point & point ) const
{
    return point - _topLeftTileOffset + Point( _windowROI.x, _windowROI.y );
}

void Interface::GameArea::ResetCursorPosition()
{
    _prevIndexPos = -1;
}
