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
    explicit RandomizedDelay( const uint32_t delay );

    // This function triggers the current delay, returning true if it is passed and automatically resets the timer.
    bool checkDelay();

private:
    uint32_t halfDelay;
    bool timerIsSet;
};

struct MonsterReturnAnim
{
    std::vector<int> start;
    std::vector<int> end;
};

class AnimationSequence final
{
public:
    explicit AnimationSequence( const std::vector<int> & seq );

    AnimationSequence( const AnimationSequence & ) = delete;

    ~AnimationSequence() = default;

    AnimationSequence & operator=( const AnimationSequence & ) = delete;

    AnimationSequence & operator=( const std::vector<int> & rhs );

    int playAnimation( bool loop = false );
    int restartAnimation();

    int getFrame() const;
    int firstFrame() const;
    size_t animationLength() const;
    double movementProgress() const;
    bool isLastFrame() const;
    bool isValid() const;

    size_t getCurrentFrameId() const
    {
        return _currentFrame;
    }

private:
    std::vector<int> _seq;
    size_t _currentFrame;
};

class AnimationReference
{
public:
    explicit AnimationReference( int id );

    AnimationReference( const AnimationReference & ) = delete;
    AnimationReference( AnimationReference && ) = default;

    virtual ~AnimationReference() = default;

    AnimationReference & operator=( const AnimationReference & ) = delete;
    AnimationReference & operator=( AnimationReference && ) = default;

    const std::vector<int> & getAnimationVector( int animState ) const;
    std::vector<int> getAnimationOffset( int animState ) const;
    uint32_t getMoveSpeed() const;
    uint32_t getFlightSpeed() const;
    uint32_t getShootingSpeed() const;
    fheroes2::Point getBlindOffset() const;
    fheroes2::Point getProjectileOffset( size_t direction ) const;
    int getTroopCountOffset( bool isReflect ) const;
    uint32_t getIdleDelay() const;

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

    bool appendFrames( std::vector<int> & target, int animID );
};

class AnimationState final : public AnimationReference
{
public:
    explicit AnimationState( int monsterID );

    AnimationState( const AnimationState & ) = delete;

    ~AnimationState() override = default;

    AnimationState & operator=( const AnimationState & ) = delete;

    bool switchAnimation( int animstate, bool reverse = false );
    bool switchAnimation( const std::vector<int> & animationList, bool reverse = false );
    int getCurrentState() const;

    // pass-down methods
    int playAnimation( bool loop = false );
    int restartAnimation();

    int getFrame() const;
    int firstFrame() const;
    int32_t getCurrentFrameXOffset() const;
    size_t animationLength() const;
    double movementProgress() const;
    bool isLastFrame() const;
    bool isValid() const;

private:
    int _animState;
    AnimationSequence _currentSequence;
};
