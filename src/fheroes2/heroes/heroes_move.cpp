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

#include "agg.h"
#include "castle.h"
#include "cursor.h"
#include "direction.h"
#include "game.h"
#include "game_interface.h"
#include "ground.h"
#include "heroes.h"
#include "kingdom.h"
#include "m82.h"
#include "maps_tiles.h"
#include "race.h"
#include "settings.h"
#include "world.h"

namespace
{
    const int heroFrameCount = 9;
}

void PlayWalkSound( int ground )
{
    int wav = M82::UNKNOWN;
    const int speed = ( 4 > Settings::Get().HeroesMoveSpeed() ? 1 : ( 7 > Settings::Get().HeroesMoveSpeed() ? 2 : 3 ) );

    // play sound
    switch ( ground ) {
    case Maps::Ground::WATER:
        wav = ( 1 == speed ? M82::WSND00 : ( 2 == speed ? M82::WSND10 : M82::WSND20 ) );
        break;
    case Maps::Ground::GRASS:
        wav = ( 1 == speed ? M82::WSND01 : ( 2 == speed ? M82::WSND11 : M82::WSND21 ) );
        break;
    case Maps::Ground::WASTELAND:
        wav = ( 1 == speed ? M82::WSND02 : ( 2 == speed ? M82::WSND12 : M82::WSND22 ) );
        break;
    case Maps::Ground::SWAMP:
    case Maps::Ground::BEACH:
        wav = ( 1 == speed ? M82::WSND03 : ( 2 == speed ? M82::WSND13 : M82::WSND23 ) );
        break;
    case Maps::Ground::LAVA:
        wav = ( 1 == speed ? M82::WSND04 : ( 2 == speed ? M82::WSND14 : M82::WSND24 ) );
        break;
    case Maps::Ground::DESERT:
    case Maps::Ground::SNOW:
        wav = ( 1 == speed ? M82::WSND05 : ( 2 == speed ? M82::WSND15 : M82::WSND25 ) );
        break;
    case Maps::Ground::DIRT:
        wav = ( 1 == speed ? M82::WSND06 : ( 2 == speed ? M82::WSND16 : M82::WSND26 ) );
        break;

    default:
        break;
    }

    if ( wav != M82::UNKNOWN )
        AGG::PlaySound( wav );
}

bool ReflectSprite( int from )
{
    switch ( from ) {
    case Direction::BOTTOM_LEFT:
    case Direction::LEFT:
    case Direction::TOP_LEFT:
        return true;

    default:
        break;
    }

    return false;
}

fheroes2::Sprite SpriteHero( const Heroes & hero, int index, bool rotate )
{
    int icn_hero = ICN::UNKNOWN;
    int index_sprite = 0;

    if ( hero.isShipMaster() )
        icn_hero = ICN::BOAT32;
    else
        switch ( hero.GetRace() ) {
        case Race::KNGT:
            icn_hero = ICN::KNGT32;
            break;
        case Race::BARB:
            icn_hero = ICN::BARB32;
            break;
        case Race::SORC:
            icn_hero = ICN::SORC32;
            break;
        case Race::WRLK:
            icn_hero = ICN::WRLK32;
            break;
        case Race::WZRD:
            icn_hero = ICN::WZRD32;
            break;
        case Race::NECR:
            icn_hero = ICN::NECR32;
            break;

        default:
            DEBUG( DBG_GAME, DBG_WARN, "unknown race" );
            break;
        }

    if ( rotate )
        index_sprite = 45;
    else
        switch ( hero.GetDirection() ) {
        case Direction::TOP:
            index_sprite = 0;
            break;
        case Direction::TOP_RIGHT:
            index_sprite = 9;
            break;
        case Direction::RIGHT:
            index_sprite = 18;
            break;
        case Direction::BOTTOM_RIGHT:
            index_sprite = 27;
            break;
        case Direction::BOTTOM:
            index_sprite = 36;
            break;
        case Direction::BOTTOM_LEFT:
            index_sprite = 27;
            break;
        case Direction::LEFT:
            index_sprite = 18;
            break;
        case Direction::TOP_LEFT:
            index_sprite = 9;
            break;

        default:
            DEBUG( DBG_GAME, DBG_WARN, "unknown direction" );
            break;
        }

    return fheroes2::AGG::GetICN( icn_hero, index_sprite + ( index % 9 ) );
}

fheroes2::Sprite SpriteFlag( const Heroes & hero, int index, bool rotate )
{
    int icn_flag = ICN::UNKNOWN;
    int index_sprite = 0;

    switch ( hero.GetColor() ) {
    case Color::BLUE:
        icn_flag = hero.isShipMaster() ? ICN::B_BFLG32 : ICN::B_FLAG32;
        break;
    case Color::GREEN:
        icn_flag = hero.isShipMaster() ? ICN::G_BFLG32 : ICN::G_FLAG32;
        break;
    case Color::RED:
        icn_flag = hero.isShipMaster() ? ICN::R_BFLG32 : ICN::R_FLAG32;
        break;
    case Color::YELLOW:
        icn_flag = hero.isShipMaster() ? ICN::Y_BFLG32 : ICN::Y_FLAG32;
        break;
    case Color::ORANGE:
        icn_flag = hero.isShipMaster() ? ICN::O_BFLG32 : ICN::O_FLAG32;
        break;
    case Color::PURPLE:
        icn_flag = hero.isShipMaster() ? ICN::P_BFLG32 : ICN::P_FLAG32;
        break;

    default:
        DEBUG( DBG_GAME, DBG_WARN, "unknown color" );
        break;
    }

    if ( rotate )
        index_sprite = 45;
    else
        switch ( hero.GetDirection() ) {
        case Direction::TOP:
            index_sprite = 0;
            break;
        case Direction::TOP_RIGHT:
            index_sprite = 9;
            break;
        case Direction::RIGHT:
            index_sprite = 18;
            break;
        case Direction::BOTTOM_RIGHT:
            index_sprite = 27;
            break;
        case Direction::BOTTOM:
            index_sprite = 36;
            break;
        case Direction::BOTTOM_LEFT:
            index_sprite = 27;
            break;
        case Direction::LEFT:
            index_sprite = 18;
            break;
        case Direction::TOP_LEFT:
            index_sprite = 9;
            break;

        default:
            DEBUG( DBG_GAME, DBG_WARN, "unknown direction" );
            break;
        }

    const int frameId = index % heroFrameCount;
    fheroes2::Sprite flag = fheroes2::AGG::GetICN( icn_flag, index_sprite + frameId );
    if ( !hero.isMoveEnabled() ) {
        static const Point offsetTop[heroFrameCount]
            = {Point( 0, 0 ), Point( 0, 2 ), Point( 0, 3 ), Point( 0, 2 ), Point( 0, 0 ), Point( 0, 1 ), Point( 0, 3 ), Point( 0, 2 ), Point( 0, 1 )};
        static const Point offsetBottom[heroFrameCount]
            = {Point( 0, 0 ), Point( 0, -1 ), Point( 0, -2 ), Point( 0, 0 ), Point( 0, -1 ), Point( 0, -2 ), Point( 0, -3 ), Point( 0, 0 ), Point( 0, -1 )};
        static const Point offsetSideways[heroFrameCount]
            = {Point( 0, 0 ), Point( -1, 0 ), Point( 0, 0 ), Point( 1, 0 ), Point( 1, -1 ), Point( 2, -1 ), Point( 1, 0 ), Point( 0, 0 ), Point( 1, 0 )};
        static const Point offsetTopSideways[heroFrameCount]
            = {Point( 0, 0 ), Point( -1, 0 ), Point( 0, 0 ), Point( -1, -1 ), Point( -2, -1 ), Point( -2, 0 ), Point( -1, 0 ), Point( 0, 0 ), Point( 1, 0 )};
        static const Point offsetBottomSideways[heroFrameCount]
            = {Point( 0, 0 ), Point( -1, 0 ), Point( 0, -1 ), Point( 2, -2 ), Point( 0, -2 ), Point( -1, -3 ), Point( -1, -2 ), Point( -1, -1 ), Point( 1, 0 )};

        static const Point offsetShipTopBottom[heroFrameCount]
            = {Point( 0, -1 ), Point( 0, 0 ), Point( 0, 1 ), Point( 0, 1 ), Point( 0, 1 ), Point( 0, 0 ), Point( 0, 1 ), Point( 0, 1 ), Point( 0, 1 )};
        static const Point offsetShipSideways[heroFrameCount]
            = {Point( 0, -2 ), Point( 0, -1 ), Point( 0, 0 ), Point( 0, 1 ), Point( 0, 0 ), Point( 0, -1 ), Point( 0, 0 ), Point( 0, -1 ), Point( 0, 1 )};
        static const Point offsetShipTopSideways[heroFrameCount]
            = {Point( 0, 0 ), Point( 0, -1 ), Point( 0, 0 ), Point( 0, 1 ), Point( 0, 0 ), Point( 0, -1 ), Point( 0, 0 ), Point( 0, -1 ), Point( 0, 1 )};
        static const Point offsetShipBottomSideways[heroFrameCount]
            = {Point( 0, -2 ), Point( 0, 0 ), Point( 0, 0 ), Point( 0, 0 ), Point( 0, 0 ), Point( 0, 0 ), Point( 0, 0 ), Point( 0, 0 ), Point( 0, 0 )};

        Point offset;
        switch ( hero.GetDirection() ) {
        case Direction::TOP:
            offset = hero.isShipMaster() ? offsetShipTopBottom[frameId] : offsetTop[frameId];
            break;
        case Direction::BOTTOM:
            offset = hero.isShipMaster() ? offsetShipTopBottom[frameId] : offsetBottom[frameId];
            break;
        case Direction::RIGHT:
        case Direction::LEFT:
            offset = hero.isShipMaster() ? offsetShipSideways[frameId] : offsetSideways[frameId];
            break;
        case Direction::TOP_RIGHT:
        case Direction::TOP_LEFT:
            offset = hero.isShipMaster() ? offsetShipTopSideways[frameId] : offsetTopSideways[frameId];
            break;
        case Direction::BOTTOM_RIGHT:
        case Direction::BOTTOM_LEFT:
            offset = hero.isShipMaster() ? offsetShipBottomSideways[frameId] : offsetBottomSideways[frameId];
            break;
        }

        flag.setPosition( flag.x() + offset.x, flag.y() + offset.y );
    }
    return flag;
}

fheroes2::Sprite SpriteShad( const Heroes & hero, int index )
{
    int icn_shad = hero.isShipMaster() ? ICN::BOATSHAD : ICN::SHADOW32;
    int index_sprite = 0;

    switch ( hero.GetDirection() ) {
    case Direction::TOP:
        index_sprite = 0;
        break;
    case Direction::TOP_RIGHT:
        index_sprite = 9;
        break;
    case Direction::RIGHT:
        index_sprite = 18;
        break;
    case Direction::BOTTOM_RIGHT:
        index_sprite = 27;
        break;
    case Direction::BOTTOM:
        index_sprite = 36;
        break;
    case Direction::BOTTOM_LEFT:
        index_sprite = 45;
        break;
    case Direction::LEFT:
        index_sprite = 54;
        break;
    case Direction::TOP_LEFT:
        index_sprite = 63;
        break;

    default:
        DEBUG( DBG_GAME, DBG_WARN, "unknown direction" );
        break;
    }

    return fheroes2::AGG::GetICN( icn_shad, index_sprite + ( index % 9 ) );
}

fheroes2::Sprite SpriteFroth( const Heroes & hero, int index )
{
    int index_sprite = 0;

    switch ( hero.GetDirection() ) {
    case Direction::TOP:
        index_sprite = 0;
        break;
    case Direction::TOP_RIGHT:
        index_sprite = 9;
        break;
    case Direction::RIGHT:
        index_sprite = 18;
        break;
    case Direction::BOTTOM_RIGHT:
        index_sprite = 27;
        break;
    case Direction::BOTTOM:
        index_sprite = 36;
        break;
    case Direction::BOTTOM_LEFT:
        index_sprite = 27;
        break;
    case Direction::LEFT:
        index_sprite = 18;
        break;
    case Direction::TOP_LEFT:
        index_sprite = 9;
        break;

    default:
        DEBUG( DBG_GAME, DBG_WARN, "unknown direction" );
        break;
    }

    return fheroes2::AGG::GetICN( ICN::FROTH, index_sprite + ( index % 9 ) );
}

bool isNeedStayFrontObject( const Heroes & hero, const Maps::Tiles & next )
{
    if ( next.GetObject() == MP2::OBJ_CASTLE ) {
        const Castle * castle = world.GetCastle( next.GetCenter() );

        return castle && !hero.isFriends( castle->GetColor() ) && castle->GetActualArmy().isValid();
    }
    else
        // to coast action
        if ( hero.isShipMaster() && next.GetObject() == MP2::OBJ_COAST )
        return true;

    return MP2::isNeedStayFront( next.GetObject() );
}

bool Heroes::isInVisibleMapArea() const
{
    return Interface::Basic::Get().GetGameArea().GetVisibleTileROI() & GetCenter();
}

void Heroes::Redraw( fheroes2::Image & dst, s32 dx, s32 dy, bool withShadow ) const
{
    if ( !isInVisibleMapArea() )
        return;

    const s32 centerIndex = GetIndex();
    const bool reflect = ReflectSprite( direction );
    const Interface::GameArea & gamearea = Interface::Basic::Get().GetGameArea();

    int flagFrameID = sprite_index;
    if ( !isMoveEnabled() ) {
        flagFrameID = isShipMaster() ? 0 : Game::MapsAnimationFrame();
    }

    // boat sprite have to be shifted so it matches other boats
    if ( isShipMaster() )
        dy -= 10;

    const fheroes2::Sprite & sprite1 = SpriteHero( *this, sprite_index, false );
    const fheroes2::Sprite & sprite2 = SpriteFlag( *this, flagFrameID, false );
    const fheroes2::Sprite & sprite3 = SpriteShad( *this, sprite_index );
    const fheroes2::Sprite & sprite4 = SpriteFroth( *this, sprite_index );

    Point dst_pt1( dx + ( reflect ? TILEWIDTH - sprite1.x() - sprite1.width() : sprite1.x() ), dy + sprite1.y() + TILEWIDTH );
    Point dst_pt2( dx + ( reflect ? TILEWIDTH - sprite2.x() - sprite2.width() : sprite2.x() ), dy + sprite2.y() + TILEWIDTH );
    Point dst_pt3( dx + sprite3.x(), dy + sprite3.y() + TILEWIDTH );
    Point dst_pt4( dx + ( reflect ? TILEWIDTH - sprite4.x() - sprite4.width() : sprite4.x() ), dy + sprite4.y() + TILEWIDTH );

    // apply offset
    if ( sprite_index < 45 ) {
        s32 ox = 0;
        s32 oy = 0;
        int frame = ( sprite_index % 9 );
        if ( frame > 0 )
            --frame;

        switch ( direction ) {
        case Direction::TOP:
            oy = -HERO_MOVE_STEP * frame;
            break;
        case Direction::TOP_RIGHT:
            ox = HERO_MOVE_STEP * frame;
            oy = -HERO_MOVE_STEP * frame;
            break;
        case Direction::TOP_LEFT:
            ox = -HERO_MOVE_STEP * frame;
            oy = -HERO_MOVE_STEP * frame;
            break;
        case Direction::BOTTOM_RIGHT:
            ox = HERO_MOVE_STEP * frame;
            oy = HERO_MOVE_STEP * frame;
            break;
        case Direction::BOTTOM:
            oy = HERO_MOVE_STEP * frame;
            break;
        case Direction::BOTTOM_LEFT:
            ox = -HERO_MOVE_STEP * frame;
            oy = HERO_MOVE_STEP * frame;
            break;
        case Direction::RIGHT:
            ox = HERO_MOVE_STEP * frame;
            break;
        case Direction::LEFT:
            ox = -HERO_MOVE_STEP * frame;
            break;
        default:
            break;
        }

        ox += _offset.x;
        oy += _offset.y;

        dst_pt1.x += ox;
        dst_pt1.y += oy;
        dst_pt2.x += ox;
        dst_pt2.y += oy;
        dst_pt3.x += ox;
        dst_pt3.y += oy;
        dst_pt4.x += ox;
        dst_pt4.y += oy;
    }

    if ( isShipMaster() ) {
        const Directions & directions = Direction::All();
        const int filter = DIRECTION_BOTTOM_ROW | Direction::LEFT | Direction::RIGHT;

        bool ocean = true;
        for ( Directions::const_iterator it = directions.begin(); it != directions.end(); ++it ) {
            if ( ( *it & filter ) && Maps::isValidDirection( centerIndex, *it ) && !world.GetTiles( Maps::GetDirectionIndex( centerIndex, *it ) ).isWater() ) {
                ocean = false;
                break;
            }
        }

        if ( ocean ) {
            const Rect blitArea = gamearea.RectFixed( dst_pt4, sprite4.width(), sprite4.height() );
            fheroes2::AlphaBlit( sprite4, blitArea.x, blitArea.y, dst, dst_pt4.x, dst_pt4.y, blitArea.w, blitArea.h, _alphaValue, reflect );
        }
    }

    // redraw sprites for shadow
    if ( withShadow ) {
        const Rect blitArea = gamearea.RectFixed( dst_pt3, sprite3.width(), sprite3.height() );
        fheroes2::AlphaBlit( sprite3, blitArea.x, blitArea.y, dst, dst_pt3.x, dst_pt3.y, blitArea.w, blitArea.h, _alphaValue );
    }

    // redraw sprites hero and flag
    const Rect blitAreaHero = gamearea.RectFixed( dst_pt1, sprite1.width(), sprite1.height() );
    fheroes2::AlphaBlit( sprite1, blitAreaHero.x, blitAreaHero.y, dst, dst_pt1.x, dst_pt1.y, blitAreaHero.w, blitAreaHero.h, _alphaValue, reflect );
    const Rect blitAreaFlag = gamearea.RectFixed( dst_pt2, sprite2.width(), sprite2.height() );
    fheroes2::AlphaBlit( sprite2, blitAreaFlag.x, blitAreaFlag.y, dst, dst_pt2.x, dst_pt2.y, blitAreaFlag.w, blitAreaFlag.h, _alphaValue, reflect );

    // redraw dependences tiles
    Maps::Tiles & tile = world.GetTiles( center.x, center.y );
    const bool skipGround = MP2::isActionObject( tile.GetObject( false ), isShipMaster() );

    tile.RedrawTop( dst );

    if ( Maps::isValidDirection( centerIndex, Direction::TOP ) )
        world.GetTiles( Maps::GetDirectionIndex( centerIndex, Direction::TOP ) ).RedrawTop4Hero( dst, skipGround );

    if ( Maps::isValidDirection( centerIndex, Direction::BOTTOM ) ) {
        Maps::Tiles & tile_bottom = world.GetTiles( Maps::GetDirectionIndex( centerIndex, Direction::BOTTOM ) );
        tile_bottom.RedrawBottom4Hero( dst );
        tile_bottom.RedrawTop( dst );
    }

    if ( 45 > GetSpriteIndex() ) {
        if ( Direction::BOTTOM != direction && Direction::TOP != direction && Maps::isValidDirection( centerIndex, direction ) ) {
            if ( Maps::isValidDirection( Maps::GetDirectionIndex( centerIndex, direction ), Direction::BOTTOM ) ) {
                Maps::Tiles & tile_dir_bottom = world.GetTiles( Maps::GetDirectionIndex( Maps::GetDirectionIndex( centerIndex, direction ), Direction::BOTTOM ) );
                tile_dir_bottom.RedrawBottom4Hero( dst );
                tile_dir_bottom.RedrawTop( dst );
            }
            if ( Maps::isValidDirection( Maps::GetDirectionIndex( centerIndex, direction ), Direction::TOP ) ) {
                Maps::Tiles & tile_dir_top = world.GetTiles( Maps::GetDirectionIndex( Maps::GetDirectionIndex( centerIndex, direction ), Direction::TOP ) );
                tile_dir_top.RedrawTop4Hero( dst, skipGround );
            }
        }

        if ( Maps::isValidDirection( centerIndex, Direction::BOTTOM ) ) {
            Maps::Tiles & tile_bottom = world.GetTiles( Maps::GetDirectionIndex( centerIndex, Direction::BOTTOM ) );

            if ( tile_bottom.GetObject() == MP2::OBJ_BOAT )
                tile_bottom.RedrawObjects( dst );
        }
    }

    if ( Maps::isValidDirection( centerIndex, direction ) ) {
        if ( Direction::TOP == direction )
            world.GetTiles( Maps::GetDirectionIndex( centerIndex, direction ) ).RedrawTop4Hero( dst, skipGround );
        else
            world.GetTiles( Maps::GetDirectionIndex( centerIndex, direction ) ).RedrawTop( dst );
    }
}

void Heroes::MoveStep( Heroes & hero, s32 indexTo, bool newpos )
{
    Route::Path & path = hero.GetPath();
    hero.ApplyPenaltyMovement( path.GetFrontPenalty() );
    if ( newpos ) {
        hero.Move2Dest( indexTo );
        hero.ActionNewPosition();
        path.PopFront();

        // possible that hero loses the battle
        if ( !hero.isFreeman() ) {
            hero.Action( indexTo );

            if ( indexTo == hero.GetPath().GetDestinationIndex() ) {
                hero.GetPath().Reset();
                hero.SetMove( false );
            }
        }
    }
    else {
        hero.GetPath().Reset();
        hero.Action( indexTo );
        hero.SetMove( false );
    }
}

bool Heroes::MoveStep( bool fast )
{
    const int32_t indexTo = Maps::GetDirectionIndex( GetIndex(), path.GetFrontDirection() );
    const int32_t indexDest = path.GetDestinationIndex();
    const Point & mp = GetCenter();

    if ( fast ) {
        if ( indexTo == indexDest && isNeedStayFrontObject( *this, world.GetTiles( indexTo ) ) )
            MoveStep( *this, indexTo, false );
        else
            MoveStep( *this, indexTo, true );

        return true;
    }
    else if ( 0 == sprite_index % 9 ) {
        if ( indexTo == indexDest && isNeedStayFrontObject( *this, world.GetTiles( indexTo ) ) ) {
            MoveStep( *this, indexTo, false );

            return true;
        }
        else {
            // play sound
            if ( GetKingdom().isControlHuman() )
                PlayWalkSound( world.GetTiles( mp.x, mp.y ).GetGround() );
        }
    }
    else if ( 8 == sprite_index % 9 ) {
        sprite_index -= 8;
        MoveStep( *this, indexTo, true );

        // if we continue to move into the same direction we must skip first frame as it's for stand position only
        if ( isMoveEnabled() && GetDirection() == path.GetFrontDirection() && !isNeedStayFrontObject( *this, world.GetTiles( path.front().GetIndex() ) ) ) {
            if ( GetKingdom().isControlHuman() )
                PlayWalkSound( world.GetTiles( mp.x, mp.y ).GetGround() );
            ++sprite_index;
        }

        return true;
    }

    ++sprite_index;

    return false;
}

void Heroes::AngleStep( int to_direct )
{
    // bool check = false;
    bool clockwise = Direction::ShortDistanceClockWise( direction, to_direct );

    // start index
    if ( 45 > sprite_index && 0 == sprite_index % 9 ) {
        switch ( direction ) {
        case Direction::TOP:
            sprite_index = 45;
            break;
        case Direction::TOP_RIGHT:
            sprite_index = clockwise ? 47 : 46;
            break;
        case Direction::TOP_LEFT:
            sprite_index = clockwise ? 46 : 47;
            break;
        case Direction::RIGHT:
            sprite_index = clockwise ? 49 : 48;
            break;
        case Direction::LEFT:
            sprite_index = clockwise ? 48 : 49;
            break;
        case Direction::BOTTOM_RIGHT:
            sprite_index = clockwise ? 51 : 50;
            break;
        case Direction::BOTTOM_LEFT:
            sprite_index = clockwise ? 50 : 51;
            break;
        case Direction::BOTTOM:
            sprite_index = clockwise ? 52 : 53;
            break;

        default:
            break;
        }
    }
    // animation process
    else {
        switch ( direction ) {
        case Direction::TOP_RIGHT:
        case Direction::RIGHT:
        case Direction::BOTTOM_RIGHT:
            clockwise ? ++sprite_index : --sprite_index;
            break;

        case Direction::TOP:
            ++sprite_index;
            break;

        case Direction::TOP_LEFT:
        case Direction::LEFT:
        case Direction::BOTTOM_LEFT:
            clockwise ? --sprite_index : ++sprite_index;
            break;

        case Direction::BOTTOM:
            --sprite_index;
            break;

        default:
            break;
        }

        bool end = false;
        int next = Direction::UNKNOWN;

        switch ( direction ) {
        case Direction::TOP:
            next = clockwise ? Direction::TOP_RIGHT : Direction::TOP_LEFT;
            break;
        case Direction::TOP_RIGHT:
            next = clockwise ? Direction::RIGHT : Direction::TOP;
            break;
        case Direction::TOP_LEFT:
            next = clockwise ? Direction::TOP : Direction::LEFT;
            break;
        case Direction::RIGHT:
            next = clockwise ? Direction::BOTTOM_RIGHT : Direction::TOP_RIGHT;
            break;
        case Direction::LEFT:
            next = clockwise ? Direction::TOP_LEFT : Direction::BOTTOM_LEFT;
            break;
        case Direction::BOTTOM_RIGHT:
            next = clockwise ? Direction::BOTTOM : Direction::RIGHT;
            break;
        case Direction::BOTTOM_LEFT:
            next = clockwise ? Direction::LEFT : Direction::BOTTOM;
            break;
        case Direction::BOTTOM:
            next = clockwise ? Direction::BOTTOM_LEFT : Direction::BOTTOM_RIGHT;
            break;

        default:
            break;
        }

        switch ( next ) {
        case Direction::TOP:
            end = ( sprite_index == 44 );
            break;
        case Direction::TOP_RIGHT:
            end = ( sprite_index == ( clockwise ? 47 : 46 ) );
            break;
        case Direction::TOP_LEFT:
            end = ( sprite_index == ( clockwise ? 46 : 47 ) );
            break;
        case Direction::RIGHT:
            end = ( sprite_index == ( clockwise ? 49 : 48 ) );
            break;
        case Direction::LEFT:
            end = ( sprite_index == ( clockwise ? 48 : 49 ) );
            break;
        case Direction::BOTTOM_RIGHT:
            end = ( sprite_index == ( clockwise ? 51 : 50 ) );
            break;
        case Direction::BOTTOM_LEFT:
            end = ( sprite_index == ( clockwise ? 50 : 51 ) );
            break;
        case Direction::BOTTOM:
            end = ( sprite_index == 53 );
            break;

        default:
            break;
        }

        if ( end ) {
            switch ( next ) {
            case Direction::TOP:
                sprite_index = 0;
                break;
            case Direction::BOTTOM:
                sprite_index = 36;
                break;
            case Direction::TOP_RIGHT:
            case Direction::TOP_LEFT:
                sprite_index = 9;
                break;
            case Direction::BOTTOM_RIGHT:
            case Direction::BOTTOM_LEFT:
                sprite_index = 27;
                break;
            case Direction::RIGHT:
            case Direction::LEFT:
                sprite_index = 18;
                break;

            default:
                break;
            }

            direction = next;
        }
    }
}

void Heroes::FadeOut( const Point & offset ) const
{
    if ( !isInVisibleMapArea() )
        return;

    Interface::GameArea & gamearea = Interface::Basic::Get().GetGameArea();

    int multiplier = std::max( std::abs( offset.x ), std::abs( offset.y ) );
    if ( multiplier < 1 )
        multiplier = 1;

    const bool offsetScreen = offset.x != 0 || offset.y != 0;

    fheroes2::Display & display = fheroes2::Display::instance();
    LocalEvent & le = LocalEvent::Get();
    _alphaValue = 255 - 8 * multiplier;

    while ( le.HandleEvents() && _alphaValue > 0 ) {
        if ( Game::AnimateInfrequentDelay( Game::HEROES_FADE_DELAY ) ) {
            Cursor::Get().Hide();

            if ( offsetScreen ) {
                gamearea.ShiftCenter( offset );
            }

            gamearea.Redraw( display, LEVEL_ALL );

            Cursor::Get().Show();
            display.render();
            _alphaValue -= 8 * multiplier;
        }
    }

    _alphaValue = 255;
}

void Heroes::FadeIn( const Point & offset ) const
{
    if ( !isInVisibleMapArea() )
        return;

    Interface::GameArea & gamearea = Interface::Basic::Get().GetGameArea();

    int multiplier = std::max( std::abs( offset.x ), std::abs( offset.y ) );
    if ( multiplier < 1 )
        multiplier = 1;

    const bool offsetScreen = offset.x != 0 || offset.y != 0;

    fheroes2::Display & display = fheroes2::Display::instance();
    LocalEvent & le = LocalEvent::Get();
    _alphaValue = 8 * multiplier;

    while ( le.HandleEvents() && _alphaValue < 250 ) {
        if ( Game::AnimateInfrequentDelay( Game::HEROES_FADE_DELAY ) ) {
            Cursor::Get().Hide();

            if ( offsetScreen ) {
                gamearea.ShiftCenter( offset );
            }

            gamearea.Redraw( display, LEVEL_ALL );

            Cursor::Get().Show();
            display.render();
            _alphaValue += 8 * multiplier;
        }
    }

    _alphaValue = 255;
}

bool Heroes::Move( bool fast )
{
    if ( Modes( ACTION ) )
        ResetModes( ACTION );

    // move hero
    if ( path.isValid() && ( isMoveEnabled() || ( GetSpriteIndex() < 45 && GetSpriteIndex() % 9 ) || GetSpriteIndex() >= 45 ) ) {
        // fast move for hide AI
        if ( fast ) {
            direction = path.GetFrontDirection();
            MoveStep( true );

            return true;
        }
        else {
            // if need change through the circle
            if ( GetDirection() != path.GetFrontDirection() ) {
                AngleStep( path.GetFrontDirection() );
            }
            else {
                SetValidDirectionSprite(); // in case of AI hero

                if ( MoveStep() ) { // move
                    if ( isFreeman() )
                        return false;

                    return true;
                }
            }
        }
    }
    else {
        SetMove( false );
    }

    return false;
}

Point Heroes::MovementDirection() const
{
    const int32_t from = GetIndex();
    const int32_t to = Maps::GetDirectionIndex( from, path.GetFrontDirection() );
    if ( from == -1 || to == -1 )
        return Point();

    if ( direction == Direction::TOP ) {
        if ( sprite_index > 1 && sprite_index < 9 ) {
            return Point( 0, -1 );
        }
    }
    else if ( direction == Direction::TOP_RIGHT || direction == Direction::TOP_LEFT ) {
        if ( sprite_index > 9 + 1 && sprite_index < 18 ) {
            return Point( direction == Direction::TOP_RIGHT ? 1 : -1, -1 );
        }
    }
    else if ( direction == Direction::RIGHT || direction == Direction::LEFT ) {
        if ( sprite_index > 18 + 1 && sprite_index < 27 ) {
            return Point( direction == Direction::RIGHT ? 1 : -1, 0 );
        }
    }
    else if ( direction == Direction::BOTTOM_RIGHT || direction == Direction::BOTTOM_LEFT ) {
        if ( sprite_index > 27 + 1 && sprite_index < 36 ) {
            return Point( direction == Direction::BOTTOM_RIGHT ? 1 : -1, 1 );
        }
    }
    else if ( direction == Direction::BOTTOM ) {
        if ( sprite_index > 36 + 1 && sprite_index < 45 ) {
            return Point( 0, 1 );
        }
    }

    return Point();
}

void Heroes::SetValidDirectionSprite()
{
    const int32_t from = GetIndex();
    const int32_t to = Maps::GetDirectionIndex( from, path.GetFrontDirection() );
    if ( from == -1 || to == -1 )
        return;

    if ( direction == Direction::TOP ) {
        if ( sprite_index < 0 || sprite_index >= 9 ) {
            sprite_index = 0;
        }
    }
    else if ( direction == Direction::TOP_RIGHT || direction == Direction::TOP_LEFT ) {
        if ( sprite_index < 9 || sprite_index >= 18 ) {
            sprite_index = 9;
        }
    }
    else if ( direction == Direction::RIGHT || direction == Direction::LEFT ) {
        if ( sprite_index < 18 || sprite_index >= 27 ) {
            sprite_index = 18;
        }
    }
    else if ( direction == Direction::BOTTOM_RIGHT || direction == Direction::BOTTOM_LEFT ) {
        if ( sprite_index < 27 || sprite_index >= 36 ) {
            sprite_index = 27;
        }
    }
    else if ( direction == Direction::BOTTOM ) {
        if ( sprite_index < 36 || sprite_index >= 45 ) {
            sprite_index = 36;
        }
    }
}
