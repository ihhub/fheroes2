/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2024                                             *
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

#include "battle_animation.h"

#include <algorithm>
#include <ostream>

#include "logging.h"
#include "monster.h"
#include "monster_anim.h"
#include "rand.h"

RandomizedDelay::RandomizedDelay( const uint32_t delay )
    : fheroes2::TimeDelay( delay )
    , halfDelay( delay / 2 )
    , timerIsSet( false )
{}

bool RandomizedDelay::checkDelay()
{
    if ( !timerIsSet ) {
        // Randomize delay as 0.75 to 1.25 original value
        setDelay( Rand::Get( 0, halfDelay ) + halfDelay * 3 / 2 );
        timerIsSet = true;
    }
    const bool res = isPassed();
    if ( res ) {
        reset();
        timerIsSet = false;
    }
    return res;
}

AnimationSequence::AnimationSequence( const std::vector<int> & seq )
    : _seq( seq )
    , _currentFrame( 0 )
{}

AnimationSequence & AnimationSequence::operator=( const std::vector<int> & rhs )
{
    _seq = rhs;
    _currentFrame = 0;

    return *this;
}

int AnimationSequence::playAnimation( bool loop )
{
    if ( !isValid() )
        return 0;

    if ( isLastFrame() ) {
        if ( loop )
            restartAnimation();
    }
    else {
        ++_currentFrame;
    }
    return _seq[_currentFrame];
}

int AnimationSequence::restartAnimation()
{
    _currentFrame = 0;
    return getFrame();
}

int AnimationSequence::getFrame() const
{
    return isValid() ? _seq[_currentFrame] : 0;
}

size_t AnimationSequence::animationLength() const
{
    return _seq.size();
}

int AnimationSequence::firstFrame() const
{
    return isValid() ? _seq.front() : 0;
}

double AnimationSequence::movementProgress() const
{
    if ( !isValid() ) {
        return 0;
    }

    // We return the progress coefficient for moving creature sprite from one position to another.
    // At the start and the end of the path we also have creature's static position,
    // but there is no such position on the cells which creature path through.
    // So to make the movement start and end more smoothly we add 0.5 to the frame number (since it starts from zero).
    return ( static_cast<double>( _currentFrame ) + 0.5 ) / static_cast<double>( animationLength() );
}

bool AnimationSequence::isLastFrame() const
{
    return _currentFrame == _seq.size() - 1;
}

bool AnimationSequence::isValid() const
{
    return !_seq.empty();
}

AnimationReference::AnimationReference( int monsterID )
    : _monsterID( monsterID )
{
    if ( monsterID < Monster::PEASANT || monsterID > Monster::WATER_ELEMENT )
        return;

    _monsterInfo = Bin_Info::GetMonsterInfo( monsterID );

    // STATIC is our default
    // appendFrames inserts to vector so ref is still valid
    if ( !appendFrames( _static, Bin_Info::MonsterAnimInfo::STATIC ) ) {
        // fall back to this, to avoid crashes
        _static.push_back( 1 );
    }

    // Taking damage
    appendFrames( _wince, Bin_Info::MonsterAnimInfo::WINCE_UP );
    appendFrames( _wince, Bin_Info::MonsterAnimInfo::WINCE_END ); // TODO: play it back together for now
    appendFrames( _winceUp, Bin_Info::MonsterAnimInfo::WINCE_UP );
    appendFrames( _winceDown, Bin_Info::MonsterAnimInfo::WINCE_END );
    appendFrames( _death, Bin_Info::MonsterAnimInfo::DEATH );

    // Idle animations
    for ( uint32_t idx = Bin_Info::MonsterAnimInfo::IDLE1; idx < _monsterInfo.idleAnimationCount + Bin_Info::MonsterAnimInfo::IDLE1; ++idx ) {
        std::vector<int> idleAnim;

        if ( appendFrames( idleAnim, idx ) ) {
            _idle.push_back( idleAnim );
        }
    }

    // Movement sequences
    _offsetX = _monsterInfo.frameXOffset;

    // Every unit has MOVE_MAIN anim, use it as a base
    appendFrames( _moving, Bin_Info::MonsterAnimInfo::MOVE_TILE_START );
    appendFrames( _moving, Bin_Info::MonsterAnimInfo::MOVE_MAIN );
    appendFrames( _moving, Bin_Info::MonsterAnimInfo::MOVE_TILE_END );

    if ( _monsterInfo.hasAnim( Bin_Info::MonsterAnimInfo::MOVE_ONE ) ) {
        appendFrames( _moveOneTile, Bin_Info::MonsterAnimInfo::MOVE_ONE );
    }
    else {
        // If there is no animation for one tile movement (fix for LICH and POWER_LICH)
        // make it from sequent MOVE_START, MOVE_MAIN, MOVE_STOP.
        appendFrames( _moveOneTile, Bin_Info::MonsterAnimInfo::MOVE_START );
        appendFrames( _moveOneTile, Bin_Info::MonsterAnimInfo::MOVE_MAIN );
        appendFrames( _moveOneTile, Bin_Info::MonsterAnimInfo::MOVE_STOP );
    }

    // First tile move: 1 + 3 + 4
    appendFrames( _moveFirstTile, Bin_Info::MonsterAnimInfo::MOVE_START );
    appendFrames( _moveFirstTile, Bin_Info::MonsterAnimInfo::MOVE_MAIN );
    appendFrames( _moveFirstTile, Bin_Info::MonsterAnimInfo::MOVE_TILE_END );

    // Last tile move: 2 + 3 + 5
    appendFrames( _moveLastTile, Bin_Info::MonsterAnimInfo::MOVE_TILE_START );
    appendFrames( _moveLastTile, Bin_Info::MonsterAnimInfo::MOVE_MAIN );
    appendFrames( _moveLastTile, Bin_Info::MonsterAnimInfo::MOVE_STOP );

    // Special for flyers
    appendFrames( _flying.start, Bin_Info::MonsterAnimInfo::MOVE_START );
    appendFrames( _flying.end, Bin_Info::MonsterAnimInfo::MOVE_STOP );

    // Attack sequences
    appendFrames( _melee[Monster_Info::TOP].start, Bin_Info::MonsterAnimInfo::ATTACK1 );
    appendFrames( _melee[Monster_Info::TOP].end, Bin_Info::MonsterAnimInfo::ATTACK1_END );

    appendFrames( _melee[Monster_Info::FRONT].start, Bin_Info::MonsterAnimInfo::ATTACK2 );
    appendFrames( _melee[Monster_Info::FRONT].end, Bin_Info::MonsterAnimInfo::ATTACK2_END );

    appendFrames( _melee[Monster_Info::BOTTOM].start, Bin_Info::MonsterAnimInfo::ATTACK3 );
    appendFrames( _melee[Monster_Info::BOTTOM].end, Bin_Info::MonsterAnimInfo::ATTACK3_END );

    // Use either shooting or breath attack animation as ranged
    if ( _monsterInfo.hasAnim( Bin_Info::MonsterAnimInfo::SHOOT2 ) ) {
        appendFrames( _ranged[Monster_Info::TOP].start, Bin_Info::MonsterAnimInfo::SHOOT1 );
        appendFrames( _ranged[Monster_Info::TOP].end, Bin_Info::MonsterAnimInfo::SHOOT1_END );

        appendFrames( _ranged[Monster_Info::FRONT].start, Bin_Info::MonsterAnimInfo::SHOOT2 );
        appendFrames( _ranged[Monster_Info::FRONT].end, Bin_Info::MonsterAnimInfo::SHOOT2_END );

        appendFrames( _ranged[Monster_Info::BOTTOM].start, Bin_Info::MonsterAnimInfo::SHOOT3 );
        appendFrames( _ranged[Monster_Info::BOTTOM].end, Bin_Info::MonsterAnimInfo::SHOOT3_END );
    }
    else if ( _monsterInfo.hasAnim( Bin_Info::MonsterAnimInfo::DOUBLEHEX2 ) ) {
        // Only 6 units should have this (in the original game)
        appendFrames( _ranged[Monster_Info::TOP].start, Bin_Info::MonsterAnimInfo::DOUBLEHEX1 );
        appendFrames( _ranged[Monster_Info::TOP].end, Bin_Info::MonsterAnimInfo::DOUBLEHEX1_END );

        appendFrames( _ranged[Monster_Info::FRONT].start, Bin_Info::MonsterAnimInfo::DOUBLEHEX2 );
        appendFrames( _ranged[Monster_Info::FRONT].end, Bin_Info::MonsterAnimInfo::DOUBLEHEX2_END );

        appendFrames( _ranged[Monster_Info::BOTTOM].start, Bin_Info::MonsterAnimInfo::DOUBLEHEX3 );
        appendFrames( _ranged[Monster_Info::BOTTOM].end, Bin_Info::MonsterAnimInfo::DOUBLEHEX3_END );
    }
}

bool AnimationReference::appendFrames( std::vector<int> & target, int animID )
{
    if ( _monsterInfo.hasAnim( animID ) ) {
        target.insert( target.end(), _monsterInfo.animationFrames.at( animID ).begin(), _monsterInfo.animationFrames.at( animID ).end() );
        return true;
    }
    return false;
}

const std::vector<int> & AnimationReference::getAnimationVector( int animState ) const
{
    switch ( animState ) {
    case Monster_Info::STAND_STILL:
    case Monster_Info::STATIC:
        return _static;
    case Monster_Info::IDLE:
        // Pick random animation
        if ( !_idle.empty() && _idle.size() == _monsterInfo.idlePriority.size() ) {
            Rand::Queue picker;

            for ( size_t i = 0; i < _idle.size(); ++i ) {
                picker.Push( static_cast<int32_t>( i ), static_cast<uint32_t>( _monsterInfo.idlePriority[i] * 100 ) );
            }
            // picker is expected to return at least 0
            const size_t id = static_cast<size_t>( picker.Get() );
            return _idle[id];
        }
        break;
    case Monster_Info::MOVE_START:
        return _moveFirstTile;
    case Monster_Info::MOVING:
        return _moving;
    case Monster_Info::MOVE_END:
        return _moveLastTile;
    case Monster_Info::MOVE_QUICK:
        return _moveOneTile;
    case Monster_Info::FLY_UP:
        return _flying.start;
    case Monster_Info::FLY_LAND:
        return _flying.end;
    case Monster_Info::MELEE_TOP:
        return _melee[Monster_Info::TOP].start;
    case Monster_Info::MELEE_TOP_END:
        return _melee[Monster_Info::TOP].end;
    case Monster_Info::MELEE_FRONT:
        return _melee[Monster_Info::FRONT].start;
    case Monster_Info::MELEE_FRONT_END:
        return _melee[Monster_Info::FRONT].end;
    case Monster_Info::MELEE_BOT:
        return _melee[Monster_Info::BOTTOM].start;
    case Monster_Info::MELEE_BOT_END:
        return _melee[Monster_Info::BOTTOM].end;
    case Monster_Info::RANG_TOP:
        return _ranged[Monster_Info::TOP].start;
    case Monster_Info::RANG_TOP_END:
        return _ranged[Monster_Info::TOP].end;
    case Monster_Info::RANG_FRONT:
        return _ranged[Monster_Info::FRONT].start;
    case Monster_Info::RANG_FRONT_END:
        return _ranged[Monster_Info::FRONT].end;
    case Monster_Info::RANG_BOT:
        return _ranged[Monster_Info::BOTTOM].start;
    case Monster_Info::RANG_BOT_END:
        return _ranged[Monster_Info::BOTTOM].end;
    case Monster_Info::WNCE_UP:
        return _winceUp;
    case Monster_Info::WNCE_DOWN:
        return _winceDown;
    case Monster_Info::WNCE:
        return _wince;
    case Monster_Info::KILL:
        return _death;
    default:
        break;
    }
    return _static;
}

std::vector<int> AnimationReference::getAnimationOffset( int animState ) const
{
    std::vector<int> offset;
    switch ( animState ) {
    case Monster_Info::STAND_STILL:
    case Monster_Info::STATIC:
        offset.resize( _static.size(), 0 );
        break;
    case Monster_Info::IDLE:
        offset.resize( _idle.front().size(), 0 );
        break;
    case Monster_Info::MOVE_START:
        offset.insert( offset.end(), _offsetX[Bin_Info::MonsterAnimInfo::MOVE_START].begin(), _offsetX[Bin_Info::MonsterAnimInfo::MOVE_START].end() );
        offset.insert( offset.end(), _offsetX[Bin_Info::MonsterAnimInfo::MOVE_MAIN].begin(), _offsetX[Bin_Info::MonsterAnimInfo::MOVE_MAIN].end() );
        offset.insert( offset.end(), _offsetX[Bin_Info::MonsterAnimInfo::MOVE_TILE_END].begin(), _offsetX[Bin_Info::MonsterAnimInfo::MOVE_TILE_END].end() );
        break;
    case Monster_Info::MOVING:
        offset.insert( offset.end(), _offsetX[Bin_Info::MonsterAnimInfo::MOVE_TILE_START].begin(), _offsetX[Bin_Info::MonsterAnimInfo::MOVE_TILE_START].end() );
        offset.insert( offset.end(), _offsetX[Bin_Info::MonsterAnimInfo::MOVE_MAIN].begin(), _offsetX[Bin_Info::MonsterAnimInfo::MOVE_MAIN].end() );
        offset.insert( offset.end(), _offsetX[Bin_Info::MonsterAnimInfo::MOVE_TILE_END].begin(), _offsetX[Bin_Info::MonsterAnimInfo::MOVE_TILE_END].end() );
        break;
    case Monster_Info::MOVE_END:
        offset.insert( offset.end(), _offsetX[Bin_Info::MonsterAnimInfo::MOVE_TILE_START].begin(), _offsetX[Bin_Info::MonsterAnimInfo::MOVE_TILE_START].end() );
        offset.insert( offset.end(), _offsetX[Bin_Info::MonsterAnimInfo::MOVE_MAIN].begin(), _offsetX[Bin_Info::MonsterAnimInfo::MOVE_MAIN].end() );
        offset.insert( offset.end(), _offsetX[Bin_Info::MonsterAnimInfo::MOVE_STOP].begin(), _offsetX[Bin_Info::MonsterAnimInfo::MOVE_STOP].end() );
        break;
    case Monster_Info::MOVE_QUICK:
        offset.insert( offset.end(), _offsetX[Bin_Info::MonsterAnimInfo::MOVE_START].begin(), _offsetX[Bin_Info::MonsterAnimInfo::MOVE_START].end() );
        offset.insert( offset.end(), _offsetX[Bin_Info::MonsterAnimInfo::MOVE_MAIN].begin(), _offsetX[Bin_Info::MonsterAnimInfo::MOVE_MAIN].end() );
        offset.insert( offset.end(), _offsetX[Bin_Info::MonsterAnimInfo::MOVE_STOP].begin(), _offsetX[Bin_Info::MonsterAnimInfo::MOVE_STOP].end() );
        break;
    case Monster_Info::FLY_UP:
        offset.resize( _flying.start.size(), 0 );
        break;
    case Monster_Info::FLY_LAND:
        offset.resize( _flying.end.size(), 0 );
        break;
    case Monster_Info::MELEE_TOP:
        offset.resize( _melee[Monster_Info::TOP].start.size(), 0 );
        break;
    case Monster_Info::MELEE_TOP_END:
        offset.resize( _melee[Monster_Info::TOP].end.size(), 0 );
        break;
    case Monster_Info::MELEE_FRONT:
        offset.resize( _melee[Monster_Info::FRONT].start.size(), 0 );
        break;
    case Monster_Info::MELEE_FRONT_END:
        offset.resize( _melee[Monster_Info::FRONT].end.size(), 0 );
        break;
    case Monster_Info::MELEE_BOT:
        offset.resize( _melee[Monster_Info::BOTTOM].start.size(), 0 );
        break;
    case Monster_Info::MELEE_BOT_END:
        offset.resize( _melee[Monster_Info::BOTTOM].end.size(), 0 );
        break;
    case Monster_Info::RANG_TOP:
        offset.resize( _ranged[Monster_Info::TOP].start.size(), 0 );
        break;
    case Monster_Info::RANG_TOP_END:
        offset.resize( _ranged[Monster_Info::TOP].end.size(), 0 );
        break;
    case Monster_Info::RANG_FRONT:
        offset.resize( _ranged[Monster_Info::FRONT].start.size(), 0 );
        break;
    case Monster_Info::RANG_FRONT_END:
        offset.resize( _ranged[Monster_Info::FRONT].end.size(), 0 );
        break;
    case Monster_Info::RANG_BOT:
        offset.resize( _ranged[Monster_Info::BOTTOM].start.size(), 0 );
        break;
    case Monster_Info::RANG_BOT_END:
        offset.resize( _ranged[Monster_Info::BOTTOM].end.size(), 0 );
        break;
    case Monster_Info::WNCE_UP:
        offset.resize( _winceUp.size(), 0 );
        break;
    case Monster_Info::WNCE_DOWN:
        offset.resize( _winceDown.size(), 0 );
        break;
    case Monster_Info::WNCE:
        offset.resize( _wince.size(), 0 );
        break;
    case Monster_Info::KILL:
        offset.resize( _death.size(), 0 );
        break;
    default:
        break;
    }
    return offset;
}

uint32_t AnimationReference::getMoveSpeed() const
{
    return _monsterInfo.moveSpeed;
}

uint32_t AnimationReference::getFlightSpeed() const
{
    return _monsterInfo.flightSpeed;
}

uint32_t AnimationReference::getShootingSpeed() const
{
    return _monsterInfo.shootSpeed;
}

fheroes2::Point AnimationReference::getBlindOffset() const
{
    return _monsterInfo.eyePosition;
}

int AnimationReference::getTroopCountOffset( bool isReflect ) const
{
    return isReflect ? _monsterInfo.troopCountOffsetRight : _monsterInfo.troopCountOffsetLeft;
}

fheroes2::Point AnimationReference::getProjectileOffset( size_t direction ) const
{
    if ( _monsterInfo.projectileOffset.size() > direction ) {
        return _monsterInfo.projectileOffset[direction];
    }
    return fheroes2::Point();
}

uint32_t AnimationReference::getIdleDelay() const
{
    return _monsterInfo.idleAnimationDelay;
}

AnimationState::AnimationState( int monsterID )
    : AnimationReference( monsterID )
    , _animState( Monster_Info::STATIC )
    , _currentSequence( _static )
{}

bool AnimationState::switchAnimation( int animState, bool reverse )
{
    std::vector<int> seq = getAnimationVector( animState );
    if ( !seq.empty() ) {
        _animState = animState;
        if ( reverse )
            std::reverse( seq.begin(), seq.end() );
        _currentSequence = seq;
        _currentSequence.restartAnimation();
        return true;
    }
    else {
        DEBUG_LOG( DBG_GAME, DBG_WARN, " AnimationState switched to invalid anim " << animState << " length " << _currentSequence.animationLength() )
    }
    return false;
}

bool AnimationState::switchAnimation( const std::vector<int> & animationList, bool reverse )
{
    std::vector<int> combinedAnimation;

    for ( std::vector<int>::const_iterator it = animationList.begin(); it != animationList.end(); ++it ) {
        const std::vector<int> & seq = getAnimationVector( *it );
        if ( !seq.empty() ) {
            _animState = *it;
            combinedAnimation.insert( combinedAnimation.end(), seq.begin(), seq.end() );
        }
    }

    if ( !combinedAnimation.empty() ) {
        if ( reverse )
            std::reverse( combinedAnimation.begin(), combinedAnimation.end() );

        _currentSequence = combinedAnimation;
        _currentSequence.restartAnimation();
        return true;
    }
    else {
        DEBUG_LOG( DBG_GAME, DBG_WARN, " AnimationState switched to invalid anim list of length " << animationList.size() )
    }
    return false;
}

int AnimationState::getCurrentState() const
{
    return _animState;
}

int AnimationState::playAnimation( bool loop )
{
    return _currentSequence.playAnimation( loop );
}

int AnimationState::restartAnimation()
{
    return _currentSequence.restartAnimation();
}

int AnimationState::getFrame() const
{
    return _currentSequence.getFrame();
}

size_t AnimationState::animationLength() const
{
    return _currentSequence.animationLength();
}

int AnimationState::firstFrame() const
{
    return _currentSequence.firstFrame();
}

int32_t AnimationState::getCurrentFrameXOffset() const
{
    // Return the horizontal frame offset to use in rendering.
    std::vector<int32_t> animSubsequences;

    // The animations consist of some subsequences. Put into vector the animation subsequences queue.
    switch ( _animState ) {
    case Monster_Info::MOVE_START:
        animSubsequences = { Bin_Info::MonsterAnimInfo::MOVE_START, Bin_Info::MonsterAnimInfo::MOVE_MAIN, Bin_Info::MonsterAnimInfo::MOVE_TILE_END };
        break;
    case Monster_Info::MOVING: {
        animSubsequences = { Bin_Info::MonsterAnimInfo::MOVE_TILE_START, Bin_Info::MonsterAnimInfo::MOVE_MAIN, Bin_Info::MonsterAnimInfo::MOVE_TILE_END };
        break;
    }
    case Monster_Info::MOVE_END: {
        animSubsequences = { Bin_Info::MonsterAnimInfo::MOVE_TILE_START, Bin_Info::MonsterAnimInfo::MOVE_MAIN, Bin_Info::MonsterAnimInfo::MOVE_STOP };
        break;
    }
    case Monster_Info::MOVE_QUICK: {
        animSubsequences = { Bin_Info::MonsterAnimInfo::MOVE_START, Bin_Info::MonsterAnimInfo::MOVE_MAIN, Bin_Info::MonsterAnimInfo::MOVE_STOP };
        break;
    }
    default:
        // If there is no horizontal offset data for current animation state, return 0 as offset.
        return 0;
    }

    // The frame number of current subsequence start.
    size_t subequenceStart = 0;
    // The frame number in the full animation sequence, which include subsequences.
    const size_t currentFrame = _currentSequence.getCurrentFrameId();

    // Get frame offset from _offsetX, analyzing in which subsequence it is.
    for ( const int32_t animSubsequence : animSubsequences ) {
        // Get the current subsequence end (it is the frame number after the last subsequence frame).
        const size_t subequenceEnd = _offsetX[animSubsequence].size() + subequenceStart;
        if ( currentFrame < subequenceEnd ) {
            return _offsetX[animSubsequence][currentFrame - subequenceStart];
        }
        subequenceStart = subequenceEnd;
    }

    // If there is no horizontal offset data for currentFrame, return 0 as offset.
    DEBUG_LOG( DBG_GAME, DBG_WARN, "Frame " << currentFrame << " is outside _offsetX [0 - " << subequenceStart << "] for animation state " << _animState )
    return 0;
}

double AnimationState::movementProgress() const
{
    return _currentSequence.movementProgress();
}

bool AnimationState::isLastFrame() const
{
    return _currentSequence.isLastFrame();
}

bool AnimationState::isValid() const
{
    return _currentSequence.isValid();
}
