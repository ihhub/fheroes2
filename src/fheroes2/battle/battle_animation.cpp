/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2020                                                    *
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
#include "monster.h"
#include "settings.h"
#include <algorithm>

RandomizedDelay::RandomizedDelay( uint32_t delay )
    : TimeDelay( delay )
    , halfDelay( delay / 2 )
    , timerIsSet( false )
{}

bool RandomizedDelay::checkDelay()
{
    if ( !timerIsSet ) {
        // Randomize delay as 0.75 to 1.25 original value
        second = Rand::Get( 0, halfDelay ) + halfDelay * 3 / 2;
        timerIsSet = true;
    }
    const bool res = Trigger();
    if ( res )
        timerIsSet = false;
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

AnimationSequence::~AnimationSequence()
{
    _seq.clear();
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

void AnimationSequence::setToLastFrame()
{
    if ( isValid() )
        _currentFrame = _seq.size() - 1;
}

double AnimationSequence::movementProgress() const
{
    if ( _seq.size() > 1 )
        return static_cast<double>( _currentFrame ) / ( static_cast<double>( animationLength() ) );

    return 0;
}

bool AnimationSequence::isFirstFrame() const
{
    return _currentFrame == 0;
}

bool AnimationSequence::isLastFrame() const
{
    return _currentFrame == _seq.size() - 1;
}

bool AnimationSequence::isValid() const
{
    return _seq.size() > 0;
}

TimedSequence::TimedSequence( const std::vector<int> & seq, uint32_t duration )
    : AnimationSequence( seq )
    , _duration( duration )
    , _currentTime( 0 )
{}

int TimedSequence::playAnimation( uint32_t delta, bool loop )
{
    _currentTime += delta;
    if ( _currentTime > _duration ) {
        _currentTime = loop ? _currentTime % _duration : _duration;
    }

    _currentFrame = getFrameID( _currentTime );
    return getFrame();
}

int TimedSequence::restartAnimation()
{
    _currentTime = 0;
    _currentFrame = 0;
    return getFrame();
}

int TimedSequence::getFrameAt( uint32_t time ) const
{
    return isValid() ? _seq[getFrameID( time )] : 0;
}

size_t TimedSequence::getFrameID( uint32_t time ) const
{
    // isValid makes sure duration and length is not 0
    if ( isValid() ) {
        const size_t frame = static_cast<size_t>( time / ( _duration / static_cast<double>( animationLength() ) ) );
        // check if time >= duration
        return ( frame < animationLength() ) ? frame : animationLength() - 1;
    }
    return 0;
}

uint32_t TimedSequence::getCurrentTime() const
{
    return _currentTime;
}

uint32_t TimedSequence::getDuration() const
{
    return _duration;
}

double TimedSequence::movementProgress() const
{
    if ( isValid() )
        return static_cast<double>( _currentTime ) / static_cast<double>( _duration );

    return 0;
}

bool TimedSequence::isValid() const
{
    return _seq.size() > 0 && _currentTime <= _duration && _duration > 0;
}

AnimationReference::AnimationReference()
    : _monsterID( Monster::UNKNOWN )
{}

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
    else { // TODO: this must be LICH or POWER_LICH. Check it!
        _moveOneTile = _moving;
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

AnimationReference::~AnimationReference() {}

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
    case Monster_Info::STATIC:
        return _static;
    case Monster_Info::IDLE:
        // Pick random animation
        if ( _idle.size() > 0 && _idle.size() == _monsterInfo.idlePriority.size() ) {
            Rand::Queue picker;

            for ( size_t i = 0; i < _idle.size(); ++i ) {
                picker.Push( i, static_cast<uint32_t>( _monsterInfo.idlePriority[i] * 100 ) );
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
        offset.resize( _moveOneTile.size(), 0 );
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

AnimationSequence AnimationReference::getAnimationSequence( int animState ) const
{
    return AnimationSequence( getAnimationVector( animState ) );
}

int AnimationReference::getStaticFrame() const
{
    return _static.back();
}

int AnimationReference::getDeathFrame() const
{
    return ( _death.empty() ) ? _static.back() : _death.back();
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

size_t AnimationReference::getProjectileID( float angle ) const
{
    return _monsterInfo.getProjectileID( angle );
}

Point AnimationReference::getBlindOffset() const
{
    return _monsterInfo.eyePosition;
}

int AnimationReference::getTroopCountOffset( bool isReflect ) const
{
    return isReflect ? _monsterInfo.troopCountOffsetRight : _monsterInfo.troopCountOffsetLeft;
}

Point AnimationReference::getProjectileOffset( size_t direction ) const
{
    if ( _monsterInfo.projectileOffset.size() > direction ) {
        return _monsterInfo.projectileOffset[direction];
    }
    return Point();
}

uint32_t AnimationReference::getIdleDelay() const
{
    return _monsterInfo.idleAnimationDelay;
}

AnimationState::AnimationState( int monsterID )
    : AnimationReference( monsterID )
    , _currentSequence( _static )
    , _animState( Monster_Info::STATIC )
{}

AnimationState::AnimationState( const AnimationReference & ref, int state )
    : AnimationReference( ref )
    , _currentSequence( _static )
{
    switchAnimation( state );
}

AnimationState::~AnimationState() {}

bool AnimationState::switchAnimation( int animState, bool reverse )
{
    std::vector<int> seq = getAnimationVector( animState );
    if ( seq.size() > 0 ) {
        _animState = animState;
        if ( reverse )
            std::reverse( seq.begin(), seq.end() );
        _currentSequence = seq;
        _currentSequence.restartAnimation();
        return true;
    }
    else {
        DEBUG( DBG_GAME, DBG_WARN, " AnimationState switched to invalid anim " << animState << " length " << _currentSequence.animationLength() );
    }
    return false;
}

bool AnimationState::switchAnimation( const std::vector<int> & animationList, bool reverse )
{
    std::vector<int> combinedAnimation;

    for ( std::vector<int>::const_iterator it = animationList.begin(); it != animationList.end(); ++it ) {
        const std::vector<int> & seq = getAnimationVector( *it );
        if ( seq.size() > 0 ) {
            _animState = *it;
            combinedAnimation.insert( combinedAnimation.end(), seq.begin(), seq.end() );
        }
    }

    if ( combinedAnimation.size() > 0 ) {
        if ( reverse )
            std::reverse( combinedAnimation.begin(), combinedAnimation.end() );

        _currentSequence = combinedAnimation;
        _currentSequence.restartAnimation();
        return true;
    }
    else {
        DEBUG( DBG_GAME, DBG_WARN, " AnimationState switched to invalid anim list of length " << animationList.size() );
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

int AnimationState::animationLength() const
{
    return _currentSequence.animationLength();
}

int AnimationState::firstFrame() const
{
    return _currentSequence.firstFrame();
}

void AnimationState::setToLastFrame()
{
    return _currentSequence.setToLastFrame();
}

double AnimationState::movementProgress() const
{
    return _currentSequence.movementProgress();
}

bool AnimationState::isFirstFrame() const
{
    return _currentSequence.isFirstFrame();
}

bool AnimationState::isLastFrame() const
{
    return _currentSequence.isLastFrame();
}

bool AnimationState::isValid() const
{
    return _currentSequence.isValid();
}
