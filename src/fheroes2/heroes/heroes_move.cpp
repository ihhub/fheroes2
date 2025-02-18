/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
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
#include <vector>

#include "army.h"
#include "audio_manager.h"
#include "castle.h"
#include "direction.h"
#include "game.h"
#include "game_delays.h"
#include "game_interface.h"
#include "ground.h"
#include "heroes.h" // IWYU pragma: associated
#include "interface_base.h"
#include "interface_gamearea.h"
#include "kingdom.h"
#include "localevent.h"
#include "m82.h"
#include "maps.h"
#include "maps_tiles.h"
#include "math_base.h"
#include "mp2.h"
#include "route.h"
#include "screen.h"
#include "settings.h"
#include "world.h"

namespace
{
    const int32_t heroMoveStep{ 4 }; // in pixels

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

    bool isNeedStayFrontObject( const Heroes & hero, const Maps::Tile & next )
    {
        if ( next.getMainObjectType() == MP2::OBJ_CASTLE ) {
            const Castle * castle = world.getCastleEntrance( next.GetCenter() );
            if ( castle == nullptr ) {
                return false;
            }

            // If this is an allied castle, then we shouldn't be here at all
            assert( hero.GetColor() == castle->GetColor() || !hero.isFriends( castle->GetColor() ) );

            return !hero.isFriends( castle->GetColor() ) && castle->GetActualArmy().isValid();
        }
        if ( hero.isShipMaster() && next.getMainObjectType() == MP2::OBJ_COAST ) {
            return true;
        }
        if ( !hero.isShipMaster() && next.getMainObjectType() == MP2::OBJ_SHIPWRECK ) {
            return true;
        }

        return MP2::isNeedStayFront( next.getMainObjectType() );
    }
}

bool Heroes::isInVisibleMapArea() const
{
    // TODO: this is not entirely correct. Consider a hero being outside the visible area but his shadow is still inside. The visible tile ROI should be extended by
    // TODO: at least 1 tile in each direction.
    return Interface::AdventureMap::Get().getGameArea().GetVisibleTileROI() & GetCenter();
}

bool Heroes::isInDeepOcean() const
{
    // Maximum number of hero's steps per cell is 9 so we check if the hero moved more than half of them.
    const bool isHeroMovedHalfOfCell = ( sprite_index < 45 && ( sprite_index % heroFrameCountPerTile ) > 4 );
    const int32_t tileIndex
        = ( isHeroMovedHalfOfCell && Maps::isValidDirection( GetIndex(), direction ) ) ? Maps::GetDirectionIndex( GetIndex(), direction ) : GetIndex();
    for ( const int32_t nearbyIndex : Maps::getAroundIndexes( tileIndex ) ) {
        if ( !world.getTile( nearbyIndex ).isWater() ) {
            return false;
        }
    }

    return true;
}

bool Heroes::MoveStep( const bool jumpToNextTile )
{
    const int32_t heroIndex = GetIndex();
    const int32_t nextStepIndex = Maps::GetDirectionIndex( heroIndex, path.GetFrontDirection() );

    const auto makeStep = [this, nextStepIndex]( const bool performMovement ) {
        ApplyPenaltyMovement( path.GetFrontPenalty() );

        if ( !performMovement ) {
            // If we are accessing an object located on a tile that we cannot step on, then this should be the last step of the path
            assert( nextStepIndex == path.GetDestinationIndex() );

            path.PopFront();
            assert( path.empty() );

            Action( nextStepIndex );

            return;
        }

        Move2Dest( nextStepIndex );

        if ( isControlHuman() ) {
            // Update the radar map image in the area that is visible to the hero after his movement.
            ScoutRadar();
        }

        ActionNewPosition( true );

        path.PopFront();

        // It is possible that the hero in the new position will be attacked and lose the battle before he can perform the action
        if ( isActive() ) {
            Action( nextStepIndex );
        }
    };

    if ( jumpToNextTile ) {
        if ( isNeedStayFrontObject( *this, world.getTile( nextStepIndex ) ) ) {
            makeStep( false );
        }
        else {
            // Unveil fog before moving the hero.
            Scout( nextStepIndex );

            makeStep( true );
        }

        return true;
    }

    const int currentHeroFrameIndex = ( sprite_index % heroFrameCountPerTile );
    if ( currentHeroFrameIndex == 0 ) {
        if ( isNeedStayFrontObject( *this, world.getTile( nextStepIndex ) ) ) {
            makeStep( false );

            return true;
        }

        if ( GetKingdom().isControlHuman() ) {
            const fheroes2::Point & mp = GetCenter();

            playHeroWalkingSound( world.getTile( mp.x, mp.y ).GetGround() );
        }
    }
    else if ( currentHeroFrameIndex == 1 ) {
        // This is a start of hero's movement. We should clear fog around him.
        Scout( nextStepIndex );
    }
    else if ( currentHeroFrameIndex == 8 ) {
        sprite_index -= 8;

        makeStep( true );

        // if we continue to move into the same direction we must skip first frame as it's for stand position only
        if ( isMoveEnabled() && GetDirection() == path.GetFrontDirection() && !isNeedStayFrontObject( *this, world.getTile( path.GetFrontIndex() ) ) ) {
            if ( GetKingdom().isControlHuman() ) {
                playHeroWalkingSound( world.getTile( heroIndex ).GetGround() );
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
        realOffset.x -= heroMoveStep * frame;
    }
    else if ( direction & DIRECTION_RIGHT_COL ) {
        realOffset.x += heroMoveStep * frame;
    }

    if ( direction & DIRECTION_TOP_ROW ) {
        realOffset.y -= heroMoveStep * frame;
    }
    else if ( direction & DIRECTION_BOTTOM_ROW ) {
        realOffset.y += heroMoveStep * frame;
    }

    return realOffset;
}

void Heroes::FadeOut( const int animSpeedMultiplier, const fheroes2::Point & offset /* = fheroes2::Point() */ ) const
{
    assert( animSpeedMultiplier > 0 );

    if ( !isInVisibleMapArea() ) {
        return;
    }

    Interface::AdventureMap & iface = Interface::AdventureMap::Get();
    Interface::GameArea & gamearea = iface.getGameArea();

    fheroes2::Display & display = fheroes2::Display::instance();
    LocalEvent & le = LocalEvent::Get();

    _alphaValue = 255;

    while ( le.HandleEvents( Game::isDelayNeeded( { Game::HEROES_FADE_DELAY } ) ) && _alphaValue > 0 ) {
        if ( !Game::validateAnimationDelay( Game::HEROES_FADE_DELAY ) ) {
            continue;
        }

        if ( offset.x != 0 || offset.y != 0 ) {
            gamearea.ShiftCenter( offset );
        }

        if ( Game::validateAnimationDelay( Game::MAPS_DELAY ) ) {
            Game::updateAdventureMapAnimationIndex();
            if ( isControlAI() ) {
                // Draw hourglass sand grains animation.
                iface.setRedraw( Interface::REDRAW_STATUS );
            }
        }

        _alphaValue = std::max( 0, _alphaValue - 8 * animSpeedMultiplier );

        iface.redraw( Interface::REDRAW_GAMEAREA );

        display.render();
    }

    _alphaValue = 255;
}

void Heroes::FadeIn( const int animSpeedMultiplier, const fheroes2::Point & offset /* = fheroes2::Point() */ ) const
{
    assert( animSpeedMultiplier > 0 );

    if ( !isInVisibleMapArea() ) {
        return;
    }

    Interface::AdventureMap & iface = Interface::AdventureMap::Get();
    Interface::GameArea & gamearea = iface.getGameArea();

    fheroes2::Display & display = fheroes2::Display::instance();
    LocalEvent & le = LocalEvent::Get();

    _alphaValue = 0;

    while ( le.HandleEvents( Game::isDelayNeeded( { Game::HEROES_FADE_DELAY } ) ) && _alphaValue < 255 ) {
        if ( !Game::validateAnimationDelay( Game::HEROES_FADE_DELAY ) ) {
            continue;
        }

        if ( offset.x != 0 || offset.y != 0 ) {
            gamearea.ShiftCenter( offset );
        }

        if ( Game::validateAnimationDelay( Game::MAPS_DELAY ) ) {
            Game::updateAdventureMapAnimationIndex();
            if ( isControlAI() ) {
                // Draw hourglass sand grains animation.
                iface.setRedraw( Interface::REDRAW_STATUS );
            }
        }

        _alphaValue = std::min( _alphaValue + 8 * animSpeedMultiplier, 255 );

        iface.redraw( Interface::REDRAW_GAMEAREA );

        display.render();
    }

    _alphaValue = 255;
}

bool Heroes::Move( const bool jumpToNextTile /* = false */ )
{
    ResetModes( ACTION );

    if ( path.isValidForMovement() && ( isMoveEnabled() || ( GetSpriteIndex() < 45 && ( GetSpriteIndex() % heroFrameCountPerTile ) > 0 ) || GetSpriteIndex() >= 45 ) ) {
        // Jump to the next position.
        if ( jumpToNextTile ) {
            direction = path.GetFrontDirection();
            MoveStep( jumpToNextTile );

            // TODO: why don't we check isActive() like it is done for a normal movement?
            return true;
        }

        // The hero is changing the direction of movement.
        const int frontDirection = path.GetFrontDirection();

        if ( GetDirection() != frontDirection ) {
            AngleStep( frontDirection );
        }
        else {
            // Set valid direction sprite in case of AI hero appearing from fog.
            SetValidDirectionSprite();

            if ( MoveStep( jumpToNextTile ) ) {
                return isActive();
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
