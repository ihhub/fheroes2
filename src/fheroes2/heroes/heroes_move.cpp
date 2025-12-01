/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2025                                             *
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
        if ( hero.isShipMaster() && next.isSuitableForDisembarkation() ) {
            return true;
        }

        return MP2::isNeedStayFront( next.getMainObjectType() );
    }
}

bool Heroes::_isInVisibleMapArea() const
{
    // TODO: this is not entirely correct. Consider a hero being outside the visible area but his shadow is still inside. The visible tile ROI should be extended by
    // TODO: at least 1 tile in each direction.
    return Interface::AdventureMap::Get().getGameArea().GetVisibleTileROI() & GetCenter();
}

bool Heroes::isInDeepOcean() const
{
    // Maximum number of hero's steps per cell is 9 so we check if the hero moved more than half of them.
    const bool isHeroMovedHalfOfCell = ( _spriteIndex < 45 && ( _spriteIndex % heroFrameCountPerTile ) > 4 );
    const int32_t tileIndex
        = ( isHeroMovedHalfOfCell && Maps::isValidDirection( GetIndex(), _direction ) ) ? Maps::GetDirectionIndex( GetIndex(), _direction ) : GetIndex();
    for ( const int32_t nearbyIndex : Maps::getAroundIndexes( tileIndex ) ) {
        if ( !world.getTile( nearbyIndex ).isWater() ) {
            return false;
        }
    }

    return true;
}

bool Heroes::_moveStep( const bool jumpToNextTile )
{
    const int32_t heroIndex = GetIndex();
    const int32_t nextStepIndex = Maps::GetDirectionIndex( heroIndex, _path.GetFrontDirection() );

    const auto makeStep = [this, nextStepIndex]( const bool performMovement ) {
        _applyMovementPenalty( _path.GetFrontPenalty() );

        if ( !performMovement ) {
            // If we are accessing an object located on a tile that we cannot step on, then this should be the last step of the path
            assert( nextStepIndex == _path.GetDestinationIndex() );

            _path.PopFront();
            assert( _path.empty() );

            Action( nextStepIndex );

            return;
        }

        Move2Dest( nextStepIndex );

        if ( isControlHuman() ) {
            // Update the radar map image in the area that is visible to the hero after his movement.
            ScoutRadar();
        }

        ActionNewPosition( true );

        _path.PopFront();

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

    const int currentHeroFrameIndex = ( _spriteIndex % heroFrameCountPerTile );
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
        _spriteIndex -= 8;

        makeStep( true );

        // if we continue to move into the same direction we must skip first frame as it's for stand position only
        if ( isMoveEnabled() && GetDirection() == _path.GetFrontDirection() && !isNeedStayFrontObject( *this, world.getTile( _path.GetFrontIndex() ) ) ) {
            if ( GetKingdom().isControlHuman() ) {
                playHeroWalkingSound( world.getTile( heroIndex ).GetGround() );
            }
            ++_spriteIndex;
        }

        return true;
    }

    ++_spriteIndex;

    return false;
}

void Heroes::_angleStep( const int targetDirection )
{
    const bool clockwise = Direction::ShortDistanceClockWise( _direction, targetDirection );

    // start index
    if ( 45 > _spriteIndex && ( _spriteIndex % heroFrameCountPerTile ) == 0 ) {
        switch ( _direction ) {
        case Direction::TOP:
            _spriteIndex = 45;
            break;
        case Direction::TOP_RIGHT:
            _spriteIndex = clockwise ? 47 : 46;
            break;
        case Direction::TOP_LEFT:
            _spriteIndex = clockwise ? 46 : 47;
            break;
        case Direction::RIGHT:
            _spriteIndex = clockwise ? 49 : 48;
            break;
        case Direction::LEFT:
            _spriteIndex = clockwise ? 48 : 49;
            break;
        case Direction::BOTTOM_RIGHT:
            _spriteIndex = clockwise ? 51 : 50;
            break;
        case Direction::BOTTOM_LEFT:
            _spriteIndex = clockwise ? 50 : 51;
            break;
        case Direction::BOTTOM:
            _spriteIndex = clockwise ? 52 : 53;
            break;

        default:
            break;
        }
    }
    // animation process
    else {
        switch ( _direction ) {
        case Direction::TOP_RIGHT:
        case Direction::RIGHT:
        case Direction::BOTTOM_RIGHT:
            clockwise ? ++_spriteIndex : --_spriteIndex;
            break;

        case Direction::TOP:
            ++_spriteIndex;
            break;

        case Direction::TOP_LEFT:
        case Direction::LEFT:
        case Direction::BOTTOM_LEFT:
            clockwise ? --_spriteIndex : ++_spriteIndex;
            break;

        case Direction::BOTTOM:
            --_spriteIndex;
            break;

        default:
            break;
        }

        bool end = false;
        int next = Direction::UNKNOWN;

        switch ( _direction ) {
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
            end = ( _spriteIndex == 44 );
            break;
        case Direction::TOP_RIGHT:
            end = ( _spriteIndex == ( clockwise ? 47 : 46 ) );
            break;
        case Direction::TOP_LEFT:
            end = ( _spriteIndex == ( clockwise ? 46 : 47 ) );
            break;
        case Direction::RIGHT:
            end = ( _spriteIndex == ( clockwise ? 49 : 48 ) );
            break;
        case Direction::LEFT:
            end = ( _spriteIndex == ( clockwise ? 48 : 49 ) );
            break;
        case Direction::BOTTOM_RIGHT:
            end = ( _spriteIndex == ( clockwise ? 51 : 50 ) );
            break;
        case Direction::BOTTOM_LEFT:
            end = ( _spriteIndex == ( clockwise ? 50 : 51 ) );
            break;
        case Direction::BOTTOM:
            end = ( _spriteIndex == 53 );
            break;

        default:
            break;
        }

        if ( end ) {
            switch ( next ) {
            case Direction::TOP:
                _spriteIndex = 0;
                break;
            case Direction::BOTTOM:
                _spriteIndex = 36;
                break;
            case Direction::TOP_RIGHT:
            case Direction::TOP_LEFT:
                _spriteIndex = 9;
                break;
            case Direction::BOTTOM_RIGHT:
            case Direction::BOTTOM_LEFT:
                _spriteIndex = 27;
                break;
            case Direction::RIGHT:
            case Direction::LEFT:
                _spriteIndex = 18;
                break;

            default:
                break;
            }

            _direction = next;
        }
    }
}

void Heroes::_applyMovementPenalty( const uint32_t penalty )
{
    if ( penalty > _movePoints ) {
        _movePoints = 0;

        return;
    }

    _movePoints -= penalty;
}

fheroes2::Point Heroes::getCurrentPixelOffset() const
{
    if ( _spriteIndex >= 45 ) {
        return {};
    }

    int frame = ( _spriteIndex % heroFrameCountPerTile );
    if ( frame > 0 )
        --frame;

    if ( frame == 0 ) {
        return _offset;
    }

    fheroes2::Point realOffset{ _offset };

    if ( _direction & DIRECTION_LEFT_COL ) {
        realOffset.x -= heroMoveStep * frame;
    }
    else if ( _direction & DIRECTION_RIGHT_COL ) {
        realOffset.x += heroMoveStep * frame;
    }

    if ( _direction & DIRECTION_TOP_ROW ) {
        realOffset.y -= heroMoveStep * frame;
    }
    else if ( _direction & DIRECTION_BOTTOM_ROW ) {
        realOffset.y += heroMoveStep * frame;
    }

    return realOffset;
}

void Heroes::FadeOut( const int animSpeedMultiplier, const fheroes2::Point & offset /* = {} */ ) const
{
    assert( animSpeedMultiplier > 0 );

    if ( !_isInVisibleMapArea() ) {
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

void Heroes::FadeIn( const int animSpeedMultiplier, const fheroes2::Point & offset /* = {} */ ) const
{
    assert( animSpeedMultiplier > 0 );

    if ( !_isInVisibleMapArea() ) {
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

    if ( _path.isValidForMovement() && ( isMoveEnabled() || ( GetSpriteIndex() < 45 && ( GetSpriteIndex() % heroFrameCountPerTile ) > 0 ) || GetSpriteIndex() >= 45 ) ) {
        // Jump to the next position.
        if ( jumpToNextTile ) {
            _direction = _path.GetFrontDirection();

            if ( !_moveStep( jumpToNextTile ) ) {
                assert( 0 );
            }

            return isActive();
        }

        if ( const int frontDirection = _path.GetFrontDirection(); GetDirection() != frontDirection ) {
            // The hero is changing the direction of movement.
            _angleStep( frontDirection );
        }
        else {
            // Set valid direction sprite in case of AI hero appearing from fog.
            _setValidDirectionSprite();

            if ( _moveStep( jumpToNextTile ) ) {
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

    const int32_t to = Maps::GetDirectionIndex( from, _path.GetFrontDirection() );
    if ( to == -1 ) {
        return {};
    }

    if ( _direction == Direction::TOP ) {
        if ( _spriteIndex > 1 && _spriteIndex < 9 ) {
            return { 0, -1 };
        }
    }
    else if ( _direction == Direction::TOP_RIGHT || _direction == Direction::TOP_LEFT ) {
        if ( _spriteIndex > 9 + 1 && _spriteIndex < 18 ) {
            return { _direction == Direction::TOP_RIGHT ? 1 : -1, -1 };
        }
    }
    else if ( _direction == Direction::RIGHT || _direction == Direction::LEFT ) {
        if ( _spriteIndex > 18 + 1 && _spriteIndex < 27 ) {
            return { _direction == Direction::RIGHT ? 1 : -1, 0 };
        }
    }
    else if ( _direction == Direction::BOTTOM_RIGHT || _direction == Direction::BOTTOM_LEFT ) {
        if ( _spriteIndex > 27 + 1 && _spriteIndex < 36 ) {
            return { _direction == Direction::BOTTOM_RIGHT ? 1 : -1, 1 };
        }
    }
    else if ( _direction == Direction::BOTTOM ) {
        if ( _spriteIndex > 36 + 1 && _spriteIndex < 45 ) {
            return { 0, 1 };
        }
    }

    return {};
}

void Heroes::_setValidDirectionSprite()
{
    const int32_t from = GetIndex();
    const int32_t to = Maps::GetDirectionIndex( from, _path.GetFrontDirection() );
    if ( from == -1 || to == -1 )
        return;

    if ( _direction == Direction::TOP ) {
        if ( _spriteIndex < 0 || _spriteIndex >= 9 ) {
            _spriteIndex = 0;
        }
    }
    else if ( _direction == Direction::TOP_RIGHT || _direction == Direction::TOP_LEFT ) {
        if ( _spriteIndex < 9 || _spriteIndex >= 18 ) {
            _spriteIndex = 9;
        }
    }
    else if ( _direction == Direction::RIGHT || _direction == Direction::LEFT ) {
        if ( _spriteIndex < 18 || _spriteIndex >= 27 ) {
            _spriteIndex = 18;
        }
    }
    else if ( _direction == Direction::BOTTOM_RIGHT || _direction == Direction::BOTTOM_LEFT ) {
        if ( _spriteIndex < 27 || _spriteIndex >= 36 ) {
            _spriteIndex = 27;
        }
    }
    else if ( _direction == Direction::BOTTOM ) {
        if ( _spriteIndex < 36 || _spriteIndex >= 45 ) {
            _spriteIndex = 36;
        }
    }
}
