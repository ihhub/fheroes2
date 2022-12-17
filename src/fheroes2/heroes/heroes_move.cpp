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
#include <cstdint>
#include <memory>
#include <ostream>
#include <utility>
#include <vector>

#include "agg_image.h"
#include "army.h"
#include "audio_manager.h"
#include "castle.h"
#include "color.h"
#include "direction.h"
#include "game.h"
#include "game_delays.h"
#include "game_interface.h"
#include "ground.h"
#include "heroes.h"
#include "icn.h"
#include "image.h"
#include "interface_gamearea.h"
#include "kingdom.h"
#include "localevent.h"
#include "logging.h"
#include "m82.h"
#include "maps.h"
#include "maps_tiles.h"
#include "math_base.h"
#include "mp2.h"
#include "race.h"
#include "route.h"
#include "screen.h"
#include "settings.h"
#include "world.h"

namespace
{
    const int heroFrameCountPerTile = 9;

    void playHeroWalkingSound( const int groundType )
    {
        const int heroMovementSpeed = Settings::Get().HeroesMoveSpeed();
        int speed = 1;

        // Speed 10 is actually Jump so essentially we have speed range from 1 to 9.
        if ( heroMovementSpeed >= 7 ) {
            speed = 3;
        }
        else if ( heroMovementSpeed >= 4 ) {
            speed = 2;
        }

        int soundId = M82::UNKNOWN;

        switch ( groundType ) {
        case Maps::Ground::WATER:
            if ( speed == 1 ) {
                soundId = M82::WSND00;
            }
            else if ( speed == 2 ) {
                soundId = M82::WSND10;
            }
            else {
                soundId = M82::WSND20;
            }
            break;
        case Maps::Ground::GRASS:
            if ( speed == 1 ) {
                soundId = M82::WSND01;
            }
            else if ( speed == 2 ) {
                soundId = M82::WSND11;
            }
            else {
                soundId = M82::WSND21;
            }
            break;
        case Maps::Ground::WASTELAND:
            if ( speed == 1 ) {
                soundId = M82::WSND02;
            }
            else if ( speed == 2 ) {
                soundId = M82::WSND12;
            }
            else {
                soundId = M82::WSND22;
            }
            break;
        case Maps::Ground::SWAMP:
        case Maps::Ground::BEACH:
            if ( speed == 1 ) {
                soundId = M82::WSND03;
            }
            else if ( speed == 2 ) {
                soundId = M82::WSND13;
            }
            else {
                soundId = M82::WSND23;
            }
            break;
        case Maps::Ground::LAVA:
            if ( speed == 1 ) {
                soundId = M82::WSND04;
            }
            else if ( speed == 2 ) {
                soundId = M82::WSND14;
            }
            else {
                soundId = M82::WSND24;
            }
            break;
        case Maps::Ground::DESERT:
        case Maps::Ground::SNOW:
            if ( speed == 1 ) {
                soundId = M82::WSND05;
            }
            else if ( speed == 2 ) {
                soundId = M82::WSND15;
            }
            else {
                soundId = M82::WSND25;
            }
            break;
        case Maps::Ground::DIRT:
            if ( speed == 1 ) {
                soundId = M82::WSND06;
            }
            else if ( speed == 2 ) {
                soundId = M82::WSND16;
            }
            else {
                soundId = M82::WSND26;
            }
            break;

        default:
            // Did you add a new terrain type? Add the corresponding logic above!
            assert( 0 );
            return;
        }

        assert( soundId != M82::UNKNOWN );
        AudioManager::PlaySoundAsync( soundId );
    }

    bool doesHeroImageNeedToBeReflected( const int directionFrom )
    {
        switch ( directionFrom ) {
        case Direction::TOP_LEFT:
        case Direction::LEFT:
        case Direction::BOTTOM_LEFT:
            return true;

        default:
            break;
        }

        return false;
    }

    const fheroes2::Sprite & getHeroSprite( const Heroes & hero, const int heroMovementIndex, const bool isHeroChangingDirection )
    {
        int icnId = ICN::UNKNOWN;

        if ( hero.isShipMaster() ) {
            icnId = ICN::BOAT32;
        }
        else {
            const int raceId = hero.GetRace();
            switch ( raceId ) {
            case Race::KNGT:
                icnId = ICN::KNGT32;
                break;
            case Race::BARB:
                icnId = ICN::BARB32;
                break;
            case Race::SORC:
                icnId = ICN::SORC32;
                break;
            case Race::WRLK:
                icnId = ICN::WRLK32;
                break;
            case Race::WZRD:
                icnId = ICN::WZRD32;
                break;
            case Race::NECR:
                icnId = ICN::NECR32;
                break;

            default:
                // Did you add a new race? Add logic above!
                assert( 0 );
                DEBUG_LOG( DBG_GAME, DBG_WARN, "Unknown hero race " << raceId )
                break;
            }
        }

        int icnIndex = 0;

        if ( isHeroChangingDirection ) {
            icnIndex = 45;
        }
        else {
            const int heroDirection = hero.GetDirection();
            switch ( heroDirection ) {
            case Direction::TOP:
                icnIndex = 0;
                break;
            case Direction::TOP_RIGHT:
                icnIndex = 9;
                break;
            case Direction::RIGHT:
                icnIndex = 18;
                break;
            case Direction::BOTTOM_RIGHT:
                icnIndex = 27;
                break;
            case Direction::BOTTOM:
                icnIndex = 36;
                break;
            case Direction::BOTTOM_LEFT:
                icnIndex = 27;
                break;
            case Direction::LEFT:
                icnIndex = 18;
                break;
            case Direction::TOP_LEFT:
                icnIndex = 9;
                break;

            default:
                DEBUG_LOG( DBG_GAME, DBG_WARN, "Unknown hero direction" << heroDirection )
                break;
            }
        }

        return fheroes2::AGG::GetICN( icnId, icnIndex + ( heroMovementIndex % heroFrameCountPerTile ) );
    }

    const fheroes2::Sprite & getFlagSprite( const Heroes & hero, const int heroMovementIndex, const bool isHeroChangingDirection, fheroes2::Point & flagOffset )
    {
        int icnId = ICN::UNKNOWN;

        const int heroColor = hero.GetColor();
        switch ( heroColor ) {
        case Color::BLUE:
            icnId = hero.isShipMaster() ? ICN::B_BFLG32 : ICN::B_FLAG32;
            break;
        case Color::GREEN:
            icnId = hero.isShipMaster() ? ICN::G_BFLG32 : ICN::G_FLAG32;
            break;
        case Color::RED:
            icnId = hero.isShipMaster() ? ICN::R_BFLG32 : ICN::R_FLAG32;
            break;
        case Color::YELLOW:
            icnId = hero.isShipMaster() ? ICN::Y_BFLG32 : ICN::Y_FLAG32;
            break;
        case Color::ORANGE:
            icnId = hero.isShipMaster() ? ICN::O_BFLG32 : ICN::O_FLAG32;
            break;
        case Color::PURPLE:
            icnId = hero.isShipMaster() ? ICN::P_BFLG32 : ICN::P_FLAG32;
            break;

        default:
            DEBUG_LOG( DBG_GAME, DBG_WARN, "Unknown hero color " << heroColor )
            break;
        }

        int icnIndex = 0;

        if ( isHeroChangingDirection ) {
            icnIndex = 45;
        }
        else {
            const int heroDirection = hero.GetDirection();
            switch ( heroDirection ) {
            case Direction::TOP:
                icnIndex = 0;
                break;
            case Direction::TOP_RIGHT:
                icnIndex = 9;
                break;
            case Direction::RIGHT:
                icnIndex = 18;
                break;
            case Direction::BOTTOM_RIGHT:
                icnIndex = 27;
                break;
            case Direction::BOTTOM:
                icnIndex = 36;
                break;
            case Direction::BOTTOM_LEFT:
                icnIndex = 27;
                break;
            case Direction::LEFT:
                icnIndex = 18;
                break;
            case Direction::TOP_LEFT:
                icnIndex = 9;
                break;

            default:
                DEBUG_LOG( DBG_GAME, DBG_WARN, "Unknown hero direction " << heroDirection )
                break;
            }
        }

        const int frameId = heroMovementIndex % heroFrameCountPerTile;
        const fheroes2::Sprite & flag = fheroes2::AGG::GetICN( icnId, icnIndex + frameId );
        if ( !hero.isMoveEnabled() ) {
            static const fheroes2::Point offsetTop[heroFrameCountPerTile] = { { 0, 0 }, { 0, 2 }, { 0, 3 }, { 0, 2 }, { 0, 0 }, { 0, 1 }, { 0, 3 }, { 0, 2 }, { 0, 1 } };
            static const fheroes2::Point offsetBottom[heroFrameCountPerTile]
                = { { 0, 0 }, { 0, -1 }, { 0, -2 }, { 0, 0 }, { 0, -1 }, { 0, -2 }, { 0, -3 }, { 0, 0 }, { 0, -1 } };
            static const fheroes2::Point offsetSideways[heroFrameCountPerTile]
                = { { 0, 0 }, { -1, 0 }, { 0, 0 }, { 1, 0 }, { 1, -1 }, { 2, -1 }, { 1, 0 }, { 0, 0 }, { 1, 0 } };
            static const fheroes2::Point offsetTopSideways[heroFrameCountPerTile]
                = { { 0, 0 }, { -1, 0 }, { 0, 0 }, { -1, -1 }, { -2, -1 }, { -2, 0 }, { -1, 0 }, { 0, 0 }, { 1, 0 } };
            static const fheroes2::Point offsetBottomSideways[heroFrameCountPerTile]
                = { { 0, 0 }, { -1, 0 }, { 0, -1 }, { 2, -2 }, { 0, -2 }, { -1, -3 }, { -1, -2 }, { -1, -1 }, { 1, 0 } };

            static const fheroes2::Point offsetShipTopBottom[heroFrameCountPerTile]
                = { { 0, -1 }, { 0, 0 }, { 0, 1 }, { 0, 1 }, { 0, 1 }, { 0, 0 }, { 0, 1 }, { 0, 1 }, { 0, 1 } };
            static const fheroes2::Point offsetShipSideways[heroFrameCountPerTile]
                = { { 0, -2 }, { 0, -1 }, { 0, 0 }, { 0, 1 }, { 0, 0 }, { 0, -1 }, { 0, 0 }, { 0, -1 }, { 0, 1 } };
            static const fheroes2::Point offsetShipTopSideways[heroFrameCountPerTile]
                = { { 0, 0 }, { 0, -1 }, { 0, 0 }, { 0, 1 }, { 0, 0 }, { 0, -1 }, { 0, 0 }, { 0, -1 }, { 0, 1 } };
            static const fheroes2::Point offsetShipBottomSideways[heroFrameCountPerTile]
                = { { 0, -2 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } };

            const int heroDirection = hero.GetDirection();
            switch ( heroDirection ) {
            case Direction::TOP:
                flagOffset = hero.isShipMaster() ? offsetShipTopBottom[frameId] : offsetTop[frameId];
                break;
            case Direction::BOTTOM:
                flagOffset = hero.isShipMaster() ? offsetShipTopBottom[frameId] : offsetBottom[frameId];
                break;
            case Direction::RIGHT:
            case Direction::LEFT:
                flagOffset = hero.isShipMaster() ? offsetShipSideways[frameId] : offsetSideways[frameId];
                break;
            case Direction::TOP_RIGHT:
            case Direction::TOP_LEFT:
                flagOffset = hero.isShipMaster() ? offsetShipTopSideways[frameId] : offsetTopSideways[frameId];
                break;
            case Direction::BOTTOM_RIGHT:
            case Direction::BOTTOM_LEFT:
                flagOffset = hero.isShipMaster() ? offsetShipBottomSideways[frameId] : offsetBottomSideways[frameId];
                break;

            default:
                DEBUG_LOG( DBG_GAME, DBG_WARN, "Unknown hero direction " << heroDirection )
                break;
            }
        }

        return flag;
    }

    const fheroes2::Sprite & getShadowSprite( const Heroes & hero, const int heroMovementIndex )
    {
        if ( hero.isShipMaster() ) {
            int indexSprite = 0;

            const int heroDirection = hero.GetDirection();
            switch ( heroDirection ) {
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
                DEBUG_LOG( DBG_GAME, DBG_WARN, "Unknown hero direction " << heroDirection )
                break;
            }

            return fheroes2::AGG::GetICN( ICN::BOATSHAD, indexSprite + ( heroMovementIndex % heroFrameCountPerTile ) );
        }

        // TODO: this is incorrect logic of choosing shadow sprite. Fix it!
        int indexSprite = heroMovementIndex;

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

    const fheroes2::Sprite & getFrothSprite( const Heroes & hero, const int heroMovementIndex )
    {
        int icnIndex = 0;

        const int heroDirection = hero.GetDirection();
        switch ( heroDirection ) {
        case Direction::TOP:
            icnIndex = 0;
            break;
        case Direction::TOP_RIGHT:
            icnIndex = 9;
            break;
        case Direction::RIGHT:
            icnIndex = 18;
            break;
        case Direction::BOTTOM_RIGHT:
            icnIndex = 27;
            break;
        case Direction::BOTTOM:
            icnIndex = 36;
            break;
        case Direction::BOTTOM_LEFT:
            icnIndex = 27;
            break;
        case Direction::LEFT:
            icnIndex = 18;
            break;
        case Direction::TOP_LEFT:
            icnIndex = 9;
            break;

        default:
            DEBUG_LOG( DBG_GAME, DBG_WARN, "Unknown hero direction " << heroDirection )
            break;
        }

        return fheroes2::AGG::GetICN( ICN::FROTH, icnIndex + ( heroMovementIndex % heroFrameCountPerTile ) );
    }

    bool isNeedStayFrontObject( const Heroes & hero, const Maps::Tiles & next )
    {
        if ( next.GetObject() == MP2::OBJ_CASTLE ) {
            const Castle * castle = world.getCastleEntrance( next.GetCenter() );
            if ( castle == nullptr ) {
                return false;
            }

            // If this is an allied castle, then we shouldn't be here at all
            assert( hero.GetColor() == castle->GetColor() || !hero.isFriends( castle->GetColor() ) );

            return !hero.isFriends( castle->GetColor() ) && castle->GetActualArmy().isValid();
        }
        if ( hero.isShipMaster() && next.GetObject() == MP2::OBJ_COAST ) {
            return true;
        }
        if ( !hero.isShipMaster() && next.GetObject() == MP2::OBJ_SHIPWRECK ) {
            return true;
        }

        return MP2::isNeedStayFront( next.GetObject() );
    }
}

bool Heroes::isInVisibleMapArea() const
{
    // TODO: this is not entirely correct. Consider a hero being outside the visible are but his shadow is still inside. The visible tile ROI should be extended by
    // TODO: at least 1 tile in each direction.
    return Interface::Basic::Get().GetGameArea().GetVisibleTileROI() & GetCenter();
}

bool Heroes::isInDeepOcean() const
{
    // Maximum number of hero's steps per cell is 9 so we check if the hero moved more than half of them.
    const bool isHeroMovedHalfOfCell = ( sprite_index < 45 && ( sprite_index % heroFrameCountPerTile ) > 4 );
    const int32_t tileIndex
        = ( isHeroMovedHalfOfCell && Maps::isValidDirection( GetIndex(), direction ) ) ? Maps::GetDirectionIndex( GetIndex(), direction ) : GetIndex();
    for ( const int32_t nearbyIndex : Maps::getAroundIndexes( tileIndex ) ) {
        if ( !world.GetTiles( nearbyIndex ).isWater() ) {
            return false;
        }
    }

    return true;
}

std::vector<std::pair<fheroes2::Point, fheroes2::Sprite>> Heroes::getHeroSpritesPerTile() const
{
    // Reflected hero sprite should be shifted by 1 pixel to right.
    const bool reflect = doesHeroImageNeedToBeReflected( direction );

    int flagFrameID = sprite_index;
    if ( !isMoveEnabled() ) {
        flagFrameID = isShipMaster() ? 0 : Game::MapsAnimationFrame();
    }

    fheroes2::Point offset;
    // Boat sprite has to be shifted so it matches other boats.
    if ( isShipMaster() ) {
        offset.y -= 11;
    }
    else {
        offset.y -= 1;
    }

    // Apply hero offset when he moves from one tile to another.
    offset += getCurrentPixelOffset();

    const fheroes2::Sprite & spriteHero = getHeroSprite( *this, sprite_index, false );
    const fheroes2::Point heroSpriteOffset( offset.x + ( reflect ? ( TILEWIDTH + 1 - spriteHero.x() - spriteHero.width() ) : spriteHero.x() ),
                                            offset.y + spriteHero.y() + TILEWIDTH );

    fheroes2::Point flagOffset;
    const fheroes2::Sprite & spriteFlag = getFlagSprite( *this, flagFrameID, false, flagOffset );
    const fheroes2::Point flagSpriteOffset( offset.x + ( reflect ? ( TILEWIDTH - spriteFlag.x() - flagOffset.x - spriteFlag.width() ) : spriteFlag.x() + flagOffset.x ),
                                            offset.y + spriteFlag.y() + flagOffset.y + TILEWIDTH );

    std::vector<std::pair<fheroes2::Point, fheroes2::Sprite>> output;
    fheroes2::DivideImageBySquares( heroSpriteOffset, spriteHero, TILEWIDTH, reflect, output );
    fheroes2::DivideImageBySquares( flagSpriteOffset, spriteFlag, TILEWIDTH, reflect, output );

    if ( isShipMaster() && isMoveEnabled() && isInDeepOcean() ) {
        // TODO: draw froth for all boats in deep water, not only for a moving boat.
        const fheroes2::Sprite & spriteFroth = getFrothSprite( *this, sprite_index );
        const fheroes2::Point frothSpriteOffset( offset.x + ( reflect ? TILEWIDTH - spriteFroth.x() - spriteFroth.width() : spriteFroth.x() ),
                                                 offset.y + spriteFroth.y() + TILEWIDTH );

        fheroes2::DivideImageBySquares( frothSpriteOffset, spriteFroth, TILEWIDTH, reflect, output );
    }

    return output;
}

std::vector<std::pair<fheroes2::Point, fheroes2::Sprite>> Heroes::getHeroShadowSpritesPerTile() const
{
    fheroes2::Point offset;
    // Boat sprite has to be shifted so it matches other boats.
    if ( isShipMaster() ) {
        offset.y -= 11;
    }
    else {
        offset.y -= 1;
    }

    // Apply hero offset when he moves from one tile to another.
    offset += getCurrentPixelOffset();

    const fheroes2::Sprite & spriteShadow = getShadowSprite( *this, sprite_index );
    const fheroes2::Point shadowSpriteOffset( offset.x + spriteShadow.x(), offset.y + spriteShadow.y() + TILEWIDTH );

    std::vector<std::pair<fheroes2::Point, fheroes2::Sprite>> output;
    fheroes2::DivideImageBySquares( shadowSpriteOffset, spriteShadow, TILEWIDTH, false, output );

    return output;
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

    if ( fast ) {
        // Unveil fog before moving the hero.
        Scoute( indexTo );
        if ( indexTo == indexDest && isNeedStayFrontObject( *this, world.GetTiles( indexTo ) ) )
            MoveStep( *this, indexTo, false );
        else
            MoveStep( *this, indexTo, true );

        return true;
    }

    const fheroes2::Point & mp = GetCenter();

    if ( ( sprite_index % heroFrameCountPerTile ) == 0 ) {
        if ( indexTo == indexDest && isNeedStayFrontObject( *this, world.GetTiles( indexTo ) ) ) {
            MoveStep( *this, indexTo, false );

            return true;
        }

        // play sound
        if ( GetKingdom().isControlHuman() ) {
            playHeroWalkingSound( world.GetTiles( mp.x, mp.y ).GetGround() );
        }
    }
    else if ( ( sprite_index % heroFrameCountPerTile ) == 1 ) {
        // This is a start of hero's movement. We should clear fog around him.
        Scoute( indexTo );
    }
    else if ( ( sprite_index % heroFrameCountPerTile ) == 8 ) {
        sprite_index -= 8;
        MoveStep( *this, indexTo, true );

        // if we continue to move into the same direction we must skip first frame as it's for stand position only
        if ( isMoveEnabled() && GetDirection() == path.GetFrontDirection() && !isNeedStayFrontObject( *this, world.GetTiles( path.front().GetIndex() ) ) ) {
            if ( GetKingdom().isControlHuman() ) {
                playHeroWalkingSound( world.GetTiles( mp.x, mp.y ).GetGround() );
            }
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
    if ( 45 > sprite_index && ( sprite_index % heroFrameCountPerTile ) == 0 ) {
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

    int frame = ( sprite_index % heroFrameCountPerTile );
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
    if ( path.isValid() && ( isMoveEnabled() || ( GetSpriteIndex() < 45 && ( GetSpriteIndex() % heroFrameCountPerTile ) > 0 ) || GetSpriteIndex() >= 45 ) ) {
        // fast move for hide AI
        if ( fast ) {
            direction = path.GetFrontDirection();
            MoveStep( true );

            return true;
        }

        // if need change through the circle
        if ( GetDirection() != path.GetFrontDirection() ) {
            AngleStep( path.GetFrontDirection() );
        }
        else {
            SetValidDirectionSprite(); // in case of AI hero

            if ( MoveStep() ) { // move
                return !isFreeman();
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
