/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2025                                             *
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

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "bin_info.h"
#include "math_base.h"
#include "timing.h"

// This timer is used for randomized idle animation delays, automatically setting it in range of 75%-125% of the intended value
class RandomizedDelay : protected fheroes2::TimeDelay
{
public:
    explicit RandomizedDelay( const uint32_t delay )
        : fheroes2::TimeDelay( delay )
        , halfDelay( delay / 2 )
    {
        // Do nothing.
    }

    // This function triggers the current delay, returning true if it is passed and automatically resets the timer.
    bool checkDelay();

private:
    uint32_t halfDelay;
    bool timerIsSet{ false };
};

struct MonsterReturnAnim
{
    std::vector<int> start;
    std::vector<int> end;
};

class AnimationSequence final
{
public:
    explicit AnimationSequence( const std::vector<int> & seq )
        : _seq( seq )
    {
        // Do nothing.
    }

    AnimationSequence( const AnimationSequence & ) = delete;

    ~AnimationSequence() = default;

    AnimationSequence & operator=( const AnimationSequence & ) = delete;

    AnimationSequence & operator=( const std::vector<int> & rhs );

    int playAnimation( const bool loop = false );
    int restartAnimation();

    int getFrame() const
    {
        return isValid() ? _seq[_currentFrame] : 0;
    }

    int firstFrame() const
    {
        return isValid() ? _seq.front() : 0;
    }

    size_t animationLength() const
    {
        return _seq.size();
    }

    double movementProgress() const;
    bool isLastFrame() const
    {
        return ( _currentFrame == _seq.size() - 1 );
    }

    bool isValid() const
    {
        return !_seq.empty();
    }

    size_t getCurrentFrameId() const
    {
        return _currentFrame;
    }

private:
    std::vector<int> _seq;
    size_t _currentFrame{ 0 };
};

class AnimationReference
{
public:
    explicit AnimationReference( const int monsterID );

    AnimationReference( const AnimationReference & ) = delete;
    AnimationReference( AnimationReference && ) = default;

    virtual ~AnimationReference() = default;

    AnimationReference & operator=( const AnimationReference & ) = delete;
    AnimationReference & operator=( AnimationReference && ) = default;

    const std::vector<int> & getAnimationVector( const int animState ) const;
    std::vector<int> getAnimationOffset( const int animState ) const;

    uint32_t getMoveSpeed() const
    {
        return _monsterInfo.moveSpeed;
    }

    uint32_t getFlightSpeed() const
    {
        return _monsterInfo.flightSpeed;
    }

    uint32_t getShootingSpeed() const
    {
        return _monsterInfo.shootSpeed;
    }

    fheroes2::Point getBlindOffset() const
    {
        return _monsterInfo.eyePosition;
    }

    fheroes2::Point getProjectileOffset( const size_t direction ) const;

    int32_t getTroopCountOffset( const bool isReflect ) const
    {
        return isReflect ? _monsterInfo.troopCountOffsetRight : _monsterInfo.troopCountOffsetLeft;
    }

    uint32_t getIdleDelay() const
    {
        return _monsterInfo.idleAnimationDelay;
    }

protected:
    int _monsterID;
    Bin_Info::MonsterAnimInfo _monsterInfo;

    std::vector<int> _static;
    std::vector<int> _moveFirstTile;
    std::vector<int> _moving;
    std::vector<int> _moveLastTile;
    std::vector<int> _moveOneTile;
    MonsterReturnAnim _flying;
    std::vector<int> _winceUp;
    std::vector<int> _winceDown;
    std::vector<int> _wince;
    std::vector<int> _death;
    MonsterReturnAnim _melee[3];
    MonsterReturnAnim _ranged[3];
    std::vector<std::vector<int>> _idle;
    std::vector<std::vector<int>> _offsetX;

    bool appendFrames( std::vector<int> & target, const size_t animID );
};

class AnimationState final : public AnimationReference
{
public:
    explicit AnimationState( const int monsterID );

    AnimationState( const AnimationState & ) = delete;

    ~AnimationState() override = default;

    AnimationState & operator=( const AnimationState & ) = delete;

    bool switchAnimation( const int animState, const bool reverse = false );
    bool switchAnimation( const std::vector<int> & animationList, const bool reverse = false );

    int getCurrentState() const
    {
        return _animState;
    }

    // pass-down methods
    int playAnimation( const bool loop = false )
    {
        return _currentSequence.playAnimation( loop );
    }

    int restartAnimation()
    {
        return _currentSequence.restartAnimation();
    }

    int getFrame() const
    {
        return _currentSequence.getFrame();
    }

    size_t animationLength() const
    {
        return _currentSequence.animationLength();
    }

    int firstFrame() const
    {
        return _currentSequence.firstFrame();
    }

    int32_t getCurrentFrameXOffset() const;

    double movementProgress() const
    {
        return _currentSequence.movementProgress();
    }

    bool isLastFrame() const
    {
        return _currentSequence.isLastFrame();
    }

    bool isValid() const
    {
        return _currentSequence.isValid();
    }

private:
    int _animState;
    AnimationSequence _currentSequence;
};
