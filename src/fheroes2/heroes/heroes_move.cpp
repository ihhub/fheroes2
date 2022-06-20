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

#include "agg_image.h"
#include "audio_manager.h"
#include "castle.h"
#include "direction.h"
#include "game.h"
#include "game_delays.h"
#include "game_interface.h"
#include "ground.h"
#include "heroes.h"
#include "icn.h"
#include "kingdom.h"
#include "logging.h"
#include "m82.h"
#include "maps_tiles.h"
#include "race.h"
#include "settings.h"
#include "world.h"

#include <cassert>

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

    AudioManager::PlaySound( wav, true );
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

const fheroes2::Sprite & SpriteHero( const Heroes & hero, int index, bool rotate )
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
            DEBUG_LOG( DBG_GAME, DBG_WARN, "unknown race" )
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
            DEBUG_LOG( DBG_GAME, DBG_WARN, "unknown direction" )
            break;
        }

    return fheroes2::AGG::GetICN( icn_hero, index_sprite + ( index % 9 ) );
}

const fheroes2::Sprite & SpriteFlag( const Heroes & hero, int index, bool rotate, fheroes2::Point & offset )
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
        DEBUG_LOG( DBG_GAME, DBG_WARN, "unknown color" )
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
            DEBUG_LOG( DBG_GAME, DBG_WARN, "unknown direction" )
            break;
        }

    const int frameId = index % heroFrameCount;
    const fheroes2::Sprite & flag = fheroes2::AGG::GetICN( icn_flag, index_sprite + frameId );
    if ( !hero.isMoveEnabled() ) {
        static const fheroes2::Point offsetTop[heroFrameCount] = { { 0, 0 }, { 0, 2 }, { 0, 3 }, { 0, 2 }, { 0, 0 }, { 0, 1 }, { 0, 3 }, { 0, 2 }, { 0, 1 } };
        static const fheroes2::Point offsetBottom[heroFrameCount] = { { 0, 0 }, { 0, -1 }, { 0, -2 }, { 0, 0 }, { 0, -1 }, { 0, -2 }, { 0, -3 }, { 0, 0 }, { 0, -1 } };
        static const fheroes2::Point offsetSideways[heroFrameCount] = { { 0, 0 }, { -1, 0 }, { 0, 0 }, { 1, 0 }, { 1, -1 }, { 2, -1 }, { 1, 0 }, { 0, 0 }, { 1, 0 } };
        static const fheroes2::Point offsetTopSideways[heroFrameCount]
            = { { 0, 0 }, { -1, 0 }, { 0, 0 }, { -1, -1 }, { -2, -1 }, { -2, 0 }, { -1, 0 }, { 0, 0 }, { 1, 0 } };
        static const fheroes2::Point offsetBottomSideways[heroFrameCount]
            = { { 0, 0 }, { -1, 0 }, { 0, -1 }, { 2, -2 }, { 0, -2 }, { -1, -3 }, { -1, -2 }, { -1, -1 }, { 1, 0 } };

        static const fheroes2::Point offsetShipTopBottom[heroFrameCount] = { { 0, -1 }, { 0, 0 }, { 0, 1 }, { 0, 1 }, { 0, 1 }, { 0, 0 }, { 0, 1 }, { 0, 1 }, { 0, 1 } };
        static const fheroes2::Point offsetShipSideways[heroFrameCount]
            = { { 0, -2 }, { 0, -1 }, { 0, 0 }, { 0, 1 }, { 0, 0 }, { 0, -1 }, { 0, 0 }, { 0, -1 }, { 0, 1 } };
        static const fheroes2::Point offsetShipTopSideways[heroFrameCount]
            = { { 0, 0 }, { 0, -1 }, { 0, 0 }, { 0, 1 }, { 0, 0 }, { 0, -1 }, { 0, 0 }, { 0, -1 }, { 0, 1 } };
        static const fheroes2::Point offsetShipBottomSideways[heroFrameCount]
            = { { 0, -2 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } };

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

        default:
            DEBUG_LOG( DBG_GAME, DBG_WARN, "unknown direction" )
            break;
        }
    }
    return flag;
}

const fheroes2::Sprite & SpriteShad( const Heroes & hero, int index )
{
    if ( hero.isShipMaster() ) {
        int indexSprite = 0;

        switch ( hero.GetDirection() ) {
        case Direction::TOP:
            indexSprite = 0;
            break;
        case Direction::TOP_RIGHT:
            indexSprite = 9;
            break;
        case Direction::RIGHT:
            indexSprite = 18;
            break;
        case Direction::BOTTOM_RIGHT:
            indexSprite = 27;
            break;
        case Direction::BOTTOM:
            indexSprite = 36;
            break;
        case Direction::BOTTOM_LEFT:
            indexSprite = 45;
            break;
        case Direction::LEFT:
            indexSprite = 54;
            break;
        case Direction::TOP_LEFT:
            indexSprite = 63;
            break;
        default:
            DEBUG_LOG( DBG_GAME, DBG_WARN, "unknown direction" )
            break;
        }

        return fheroes2::AGG::GetICN( ICN::BOATSHAD, indexSprite + ( index % 9 ) );
    }
    else {
        int indexSprite = index;

        if ( indexSprite == 51 )
            indexSprite = 56;
        else if ( indexSprite == 50 )
            indexSprite = 57;
        else if ( indexSprite == 49 )
            indexSprite = 58;
        else if ( indexSprite == 47 )
            indexSprite = 55;
        else if ( indexSprite == 46 )
            indexSprite = 55;

        const int indexOffset = ( indexSprite < 9 || indexSprite >= 36 ) ? 0 : 50;

        return fheroes2::AGG::GetICN( ICN::SHADOW32, indexSprite + indexOffset );
    }
}

const fheroes2::Sprite & SpriteFroth( const Heroes & hero, int index )
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
        DEBUG_LOG( DBG_GAME, DBG_WARN, "unknown direction" )
        break;
    }

    return fheroes2::AGG::GetICN( ICN::FROTH, index_sprite + ( index % 9 ) );
}

bool isNeedStayFrontObject( const Heroes & hero, const Maps::Tiles & next )
{
    if ( next.GetObject() == MP2::OBJ_CASTLE ) {
        const Castle * castle = world.getCastleEntrance( next.GetCenter() );
        return castle && !hero.isFriends( castle->GetColor() ) && castle->GetActualArmy().isValid();
    }
    if ( hero.isShipMaster() && next.GetObject() == MP2::OBJ_COAST ) {
        return true;
    }
    if ( !hero.isShipMaster() && next.GetObject() == MP2::OBJ_SHIPWRECK ) {
        return true;
    }

    return MP2::isNeedStayFront( next.GetObject() );
}

bool Heroes::isInVisibleMapArea() const
{
    return Interface::Basic::Get().GetGameArea().GetVisibleTileROI() & GetCenter();
}

bool Heroes::isInDeepOcean() const
{
    // Maximum number of hero's steps per cell is 9 so we check if the hero moved more than half of them.
    const bool isHeroMovedHalfOfCell = ( sprite_index < 45 && sprite_index % 9 > 4 );
    const int32_t tileIndex
        = ( isHeroMovedHalfOfCell && Maps::isValidDirection( GetIndex(), direction ) ) ? Maps::GetDirectionIndex( GetIndex(), direction ) : GetIndex();
    for ( const int32_t nearbyIndex : Maps::getAroundIndexes( tileIndex ) ) {
        if ( !world.GetTiles( nearbyIndex ).isWater() ) {
            return false;
        }
    }
    return true;
}

void Heroes::RedrawShadow( fheroes2::Image & dst, const int32_t dx, int32_t dy, const fheroes2::Rect & visibleTileROI, const Interface::GameArea & area ) const
{
    if ( !( visibleTileROI & GetCenter() ) )
        return;

    const bool reflect = ReflectSprite( direction );

    // boat sprite have to be shifted so it matches other boats
    if ( isShipMaster() )
        dy -= 10;

    const fheroes2::Sprite & spriteShad = SpriteShad( *this, sprite_index );
    const fheroes2::Sprite & spriteFroth = SpriteFroth( *this, sprite_index );

    fheroes2::Point dstShad( dx + spriteShad.x(), dy + spriteShad.y() + TILEWIDTH );
    fheroes2::Point dstFroth( dx + ( reflect ? TILEWIDTH - spriteFroth.x() - spriteFroth.width() : spriteFroth.x() ), dy + spriteFroth.y() + TILEWIDTH );

    // apply offset
    const fheroes2::Point realOffset = getCurrentPixelOffset();
    dstShad += realOffset;
    dstFroth += realOffset;

    assert( _alphaValue >= 0 && _alphaValue <= 255 );

    if ( isShipMaster() && isMoveEnabled() && isInDeepOcean() ) {
        const fheroes2::Rect blitArea = area.RectFixed( dstFroth, spriteFroth.width(), spriteFroth.height() );
        fheroes2::AlphaBlit( spriteFroth, blitArea.x, blitArea.y, dst, dstFroth.x, dstFroth.y, blitArea.width, blitArea.height, static_cast<uint8_t>( _alphaValue ),
                             reflect );
    }

    const fheroes2::Rect blitArea = area.RectFixed( dstShad, spriteShad.width(), spriteShad.height() );
    fheroes2::AlphaBlit( spriteShad, blitArea.x, blitArea.y, dst, dstShad.x, dstShad.y, blitArea.width, blitArea.height, static_cast<uint8_t>( _alphaValue ) );
}

void Heroes::Redraw( fheroes2::Image & dst, const int32_t dx, int32_t dy, const fheroes2::Rect & visibleTileROI, const Interface::GameArea & area ) const
{
    if ( !( visibleTileROI & GetCenter() ) )
        return;

    const bool reflect = ReflectSprite( direction );

    int flagFrameID = sprite_index;
    if ( !isMoveEnabled() ) {
        flagFrameID = isShipMaster() ? 0 : Game::MapsAnimationFrame();
    }

    // boat sprite have to be shifted so it matches other boats
    if ( isShipMaster() )
        dy -= 10;

    fheroes2::Point flagOffset;
    const fheroes2::Sprite & spriteHero = SpriteHero( *this, sprite_index, false );
    const fheroes2::Sprite & spriteFlag = SpriteFlag( *this, flagFrameID, false, flagOffset );

    // Reflected hero sprite should be shifted by 1 pixel to right.
    fheroes2::Point dstHero( dx + ( reflect ? TILEWIDTH + 1 - spriteHero.x() - spriteHero.width() : spriteHero.x() ), dy + spriteHero.y() + TILEWIDTH );
    fheroes2::Point dstFlag( dx + ( reflect ? TILEWIDTH - spriteFlag.x() - flagOffset.x - spriteFlag.width() : spriteFlag.x() + flagOffset.x ),
                             dy + spriteFlag.y() + flagOffset.y + TILEWIDTH );

    // apply offset
    const fheroes2::Point realOffset = getCurrentPixelOffset();
    dstHero += realOffset;
    dstFlag += realOffset;

    // redraw sprites hero and flag
    assert( _alphaValue >= 0 && _alphaValue <= 255 );

    const fheroes2::Rect blitAreaHero = area.RectFixed( dstHero, spriteHero.width(), spriteHero.height() );
    fheroes2::AlphaBlit( spriteHero, blitAreaHero.x, blitAreaHero.y, dst, dstHero.x, dstHero.y, blitAreaHero.width, blitAreaHero.height,
                         static_cast<uint8_t>( _alphaValue ), reflect );

    const fheroes2::Rect blitAreaFlag = area.RectFixed( dstFlag, spriteFlag.width(), spriteFlag.height() );
    fheroes2::AlphaBlit( spriteFlag, blitAreaFlag.x, blitAreaFlag.y, dst, dstFlag.x, dstFlag.y, blitAreaFlag.width, blitAreaFlag.height,
                         static_cast<uint8_t>( _alphaValue ), reflect );
}

void Heroes::SetRedrawIndexes()
{
    const int32_t centerIndex = GetIndex();
    _redrawIndex.topOnBottom = -1;
    _redrawIndex.objectsOnBottom = -1;
    if ( Maps::isValidDirection( centerIndex, Direction::BOTTOM ) ) {
        _redrawIndex.topOnBottom = Maps::GetDirectionIndex( centerIndex, Direction::BOTTOM );
        const Maps::Tiles & tileBottom = world.GetTiles( _redrawIndex.topOnBottom );
        if ( !Interface::SkipRedrawTileBottom4Hero( tileBottom.GetObjectTileset(), tileBottom.GetObjectSpriteIndex(), tileBottom.GetPassable() ) ) {
            _redrawIndex.objectsOnBottom = _redrawIndex.topOnBottom;
        }
    }

    _redrawIndex.topOnDirectionBottom = -1;
    _redrawIndex.objectsOnDirectionBottom = -1;
    if ( 45 > GetSpriteIndex() && Direction::BOTTOM != direction && Direction::TOP != direction && Direction::BOTTOM_LEFT != direction
         && Direction::BOTTOM_RIGHT != direction && Maps::isValidDirection( centerIndex, direction ) ) {
        const int32_t directionIndex = Maps::GetDirectionIndex( centerIndex, direction );
        if ( Maps::isValidDirection( directionIndex, Direction::BOTTOM ) ) {
            _redrawIndex.topOnDirectionBottom = Maps::GetDirectionIndex( directionIndex, Direction::BOTTOM );
            const Maps::Tiles & tileDirectionBottom = world.GetTiles( _redrawIndex.topOnDirectionBottom );
            if ( !Interface::SkipRedrawTileBottom4Hero( tileDirectionBottom.GetObjectTileset(), tileDirectionBottom.GetObjectSpriteIndex(),
                                                        tileDirectionBottom.GetPassable() ) ) {
                _redrawIndex.objectsOnDirectionBottom = _redrawIndex.topOnDirectionBottom;
            }
        }
    }
    _redrawIndex.topOnDirection = ( Direction::BOTTOM != direction && Direction::TOP != direction && Maps::isValidDirection( centerIndex, direction ) )
                                      ? Maps::GetDirectionIndex( centerIndex, direction )
                                      : -1;
}

void Heroes::UpdateRedrawBottom( const Maps::Tiles & tile )
{
    const Heroes * hero = tile.GetHeroes();
    if ( hero == nullptr ) {
        return;
    }
    if ( _redrawIndex.objectsOnDirectionBottom == tile.GetIndex() ) {
        _redrawIndex.objectsOnDirectionBottom = -1;
    }
    if ( _redrawIndex.objectsOnBottom == tile.GetIndex() ) {
        _redrawIndex.objectsOnBottom = -1;
    }
    const Heroes::RedrawIndex & redrawIndex = hero->GetRedrawIndex();
    if ( _redrawIndex.objectsOnBottom == redrawIndex.objectsOnDirectionBottom ) {
        _redrawIndex.objectsOnBottom = -1;
    }
}

void Heroes::UpdateRedrawTop( const Maps::Tiles & tile )
{
    const MP2::MapObjectType objectType = tile.GetObject();
    if ( MP2::OBJ_BOAT != objectType && MP2::OBJ_HEROES != objectType ) {
        return;
    }
    if ( _redrawIndex.topOnBottom == tile.GetIndex() ) {
        _redrawIndex.topOnBottom = -1;
    }
    else if ( _redrawIndex.topOnDirection == tile.GetIndex() ) {
        _redrawIndex.topOnDirection = -1;
    }
    else if ( _redrawIndex.topOnDirectionBottom == tile.GetIndex() ) {
        _redrawIndex.topOnDirectionBottom = -1;
    }
    const Heroes * hero = tile.GetHeroes();
    if ( hero == nullptr ) {
        return;
    }
    const Heroes::RedrawIndex & redrawIndex = hero->GetRedrawIndex();
    if ( _redrawIndex.topOnBottom == redrawIndex.topOnDirection || _redrawIndex.topOnBottom == redrawIndex.topOnDirectionBottom ) {
        _redrawIndex.topOnBottom = -1;
    }
    if ( _redrawIndex.topOnDirection == redrawIndex.topOnBottom || _redrawIndex.topOnDirection == redrawIndex.topOnDirectionBottom ) {
        _redrawIndex.topOnDirection = -1;
    }
}

void Heroes::RedrawTop( fheroes2::Image & dst, const fheroes2::Rect & visibleTileROI, const Interface::GameArea & area ) const
{
    const Maps::Tiles & tile = world.GetTiles( center.x, center.y );
    const bool skipGround = MP2::isActionObject( tile.GetObject( false ), isShipMaster() );

    tile.RedrawTop( dst, visibleTileROI, false, area );

    const int32_t centerIndex = GetIndex();

    if ( Maps::isValidDirection( centerIndex, Direction::TOP ) )
        world.GetTiles( Maps::GetDirectionIndex( centerIndex, Direction::TOP ) ).RedrawTop4Hero( dst, visibleTileROI, skipGround, area );

    if ( 45 > GetSpriteIndex() ) {
        if ( Direction::BOTTOM != direction && Direction::TOP != direction && Maps::isValidDirection( centerIndex, direction ) ) {
            if ( Direction::TOP_LEFT != direction && Direction::TOP_RIGHT != direction
                 && Maps::isValidDirection( Maps::GetDirectionIndex( centerIndex, direction ), Direction::TOP ) ) {
                const Maps::Tiles & tileDirectionTop = world.GetTiles( Maps::GetDirectionIndex( Maps::GetDirectionIndex( centerIndex, direction ), Direction::TOP ) );
                tileDirectionTop.RedrawTop4Hero( dst, visibleTileROI, skipGround, area );
            }
        }
    }

    if ( _redrawIndex.topOnBottom != -1 ) {
        const Maps::Tiles & tileBottom = world.GetTiles( _redrawIndex.topOnBottom );
        tileBottom.RedrawTop( dst, visibleTileROI, false, area );
        tileBottom.RedrawTopFromBottom( dst, area );
    }
    if ( _redrawIndex.topOnDirection != -1 ) {
        const Maps::Tiles & tileDirection = world.GetTiles( _redrawIndex.topOnDirection );
        tileDirection.RedrawTop( dst, visibleTileROI, false, area );
        tileDirection.RedrawTopFromBottom( dst, area );
    }
    if ( _redrawIndex.topOnDirectionBottom != -1 ) {
        const Maps::Tiles & tileDirectionBottom = world.GetTiles( _redrawIndex.topOnDirectionBottom );
        tileDirectionBottom.RedrawTop( dst, visibleTileROI, false, area );
        tileDirectionBottom.RedrawTopFromBottom( dst, area );
    }
}

void Heroes::RedrawBottom( fheroes2::Image & dst, const fheroes2::Rect & visibleTileROI, const Interface::GameArea & area, bool isPuzzleDraw ) const
{
    if ( _redrawIndex.objectsOnDirectionBottom != -1 ) {
        const Maps::Tiles & tile = world.GetTiles( _redrawIndex.objectsOnDirectionBottom );
        tile.RedrawBottom4Hero( dst, visibleTileROI, area );
        tile.RedrawObjects( dst, isPuzzleDraw, area );
    }
    if ( _redrawIndex.objectsOnBottom != -1 ) {
        const Maps::Tiles & tile = world.GetTiles( _redrawIndex.objectsOnBottom );
        tile.RedrawBottom4Hero( dst, visibleTileROI, area );
        tile.RedrawObjects( dst, isPuzzleDraw, area );
    }
}

const Heroes::RedrawIndex & Heroes::GetRedrawIndex() const
{
    return _redrawIndex;
}

void Heroes::MoveStep( Heroes & hero, int32_t indexTo, bool newpos )
{
    Route::Path & path = hero.GetPath();
    hero.ApplyPenaltyMovement( path.GetFrontPenalty() );
    if ( newpos ) {
        hero.Move2Dest( indexTo );
        hero.ActionNewPosition( true );
        path.PopFront();

        // possible that hero loses the battle
        if ( !hero.isFreeman() ) {
            const bool isDestination = indexTo == hero.GetPath().GetDestinationIndex( true );
            hero.Action( indexTo, isDestination );

            if ( isDestination ) {
                hero.GetPath().Reset();
                hero.SetMove( false );
            }
        }
    }
    else {
        hero.GetPath().Reset();
        hero.Action( indexTo, true );
        hero.SetMove( false );
    }
}

bool Heroes::MoveStep( bool fast )
{
    const int32_t indexTo = Maps::GetDirectionIndex( GetIndex(), path.GetFrontDirection() );
    const int32_t indexDest = path.GetDestinationIndex( true );
    const fheroes2::Point & mp = GetCenter();

    if ( fast ) {
        // Unveil fog before moving the hero.
        Scoute( indexTo );
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
    else if ( sprite_index % 9 == 1 ) {
        // This is a start of hero's movement. We should clear fog around him.
        Scoute( indexTo );
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

fheroes2::Point Heroes::getCurrentPixelOffset() const
{
    if ( sprite_index >= 45 ) {
        return {};
    }

    int frame = ( sprite_index % 9 );
    if ( frame > 0 )
        --frame;

    if ( frame == 0 ) {
        return _offset;
    }

    fheroes2::Point realOffset{ _offset };

    if ( direction & DIRECTION_LEFT_COL ) {
        realOffset.x -= HERO_MOVE_STEP * frame;
    }
    else if ( direction & DIRECTION_RIGHT_COL ) {
        realOffset.x += HERO_MOVE_STEP * frame;
    }

    if ( direction & DIRECTION_TOP_ROW ) {
        realOffset.y -= HERO_MOVE_STEP * frame;
    }
    else if ( direction & DIRECTION_BOTTOM_ROW ) {
        realOffset.y += HERO_MOVE_STEP * frame;
    }

    return realOffset;
}

void Heroes::FadeOut( const fheroes2::Point & offset ) const
{
    if ( !isInVisibleMapArea() )
        return;

    Interface::Basic & iface = Interface::Basic::Get();
    Interface::GameArea & gamearea = iface.GetGameArea();

    int multiplier = std::max( offset.x < 0 ? -offset.x : offset.x, offset.y < 0 ? -offset.y : offset.y );
    if ( multiplier < 1 )
        multiplier = 1;

    const bool offsetScreen = offset.x != 0 || offset.y != 0;

    fheroes2::Display & display = fheroes2::Display::instance();
    LocalEvent & le = LocalEvent::Get();
    _alphaValue = 255 - 8 * multiplier;

    const std::vector<Game::DelayType> delayTypes = { Game::HEROES_FADE_DELAY };
    while ( le.HandleEvents( Game::isDelayNeeded( delayTypes ) ) && _alphaValue > 0 ) {
        if ( Game::validateAnimationDelay( Game::HEROES_FADE_DELAY ) ) {
            if ( offsetScreen ) {
                gamearea.ShiftCenter( offset );
            }

            iface.Redraw( Interface::REDRAW_GAMEAREA );

            display.render();
            _alphaValue -= 8 * multiplier;
        }
    }

    _alphaValue = 255;
}

void Heroes::FadeIn( const fheroes2::Point & offset ) const
{
    if ( !isInVisibleMapArea() )
        return;

    Interface::Basic & iface = Interface::Basic::Get();
    Interface::GameArea & gamearea = iface.GetGameArea();

    int multiplier = std::max( offset.x < 0 ? -offset.x : offset.x, offset.y < 0 ? -offset.y : offset.y );
    if ( multiplier < 1 )
        multiplier = 1;

    const bool offsetScreen = offset.x != 0 || offset.y != 0;

    fheroes2::Display & display = fheroes2::Display::instance();
    LocalEvent & le = LocalEvent::Get();
    _alphaValue = 8 * multiplier;

    const std::vector<Game::DelayType> delayTypes = { Game::HEROES_FADE_DELAY };
    while ( le.HandleEvents( Game::isDelayNeeded( delayTypes ) ) && _alphaValue < 250 ) {
        if ( Game::validateAnimationDelay( Game::HEROES_FADE_DELAY ) ) {
            if ( offsetScreen ) {
                gamearea.ShiftCenter( offset );
            }

            iface.Redraw( Interface::REDRAW_GAMEAREA );

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

fheroes2::Point Heroes::MovementDirection() const
{
    const int32_t from = GetIndex();
    if ( from == -1 ) {
        return {};
    }

    const int32_t to = Maps::GetDirectionIndex( from, path.GetFrontDirection() );
    if ( to == -1 ) {
        return {};
    }

    if ( direction == Direction::TOP ) {
        if ( sprite_index > 1 && sprite_index < 9 ) {
            return { 0, -1 };
        }
    }
    else if ( direction == Direction::TOP_RIGHT || direction == Direction::TOP_LEFT ) {
        if ( sprite_index > 9 + 1 && sprite_index < 18 ) {
            return { direction == Direction::TOP_RIGHT ? 1 : -1, -1 };
        }
    }
    else if ( direction == Direction::RIGHT || direction == Direction::LEFT ) {
        if ( sprite_index > 18 + 1 && sprite_index < 27 ) {
            return { direction == Direction::RIGHT ? 1 : -1, 0 };
        }
    }
    else if ( direction == Direction::BOTTOM_RIGHT || direction == Direction::BOTTOM_LEFT ) {
        if ( sprite_index > 27 + 1 && sprite_index < 36 ) {
            return { direction == Direction::BOTTOM_RIGHT ? 1 : -1, 1 };
        }
    }
    else if ( direction == Direction::BOTTOM ) {
        if ( sprite_index > 36 + 1 && sprite_index < 45 ) {
            return { 0, 1 };
        }
    }

    return {};
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
