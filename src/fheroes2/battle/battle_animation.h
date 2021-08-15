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

#ifndef H2BATTLE_ANIMATION_H
#define H2BATTLE_ANIMATION_H

#include "bin_info.h"
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

struct monsterReturnAnim
{
    std::vector<int> start;
    std::vector<int> end;
};

class AnimationSequence
{
public:
    explicit AnimationSequence( const std::vector<int> & seq );
    AnimationSequence( const AnimationSequence & ) = default;

    AnimationSequence & operator=( const AnimationSequence & ) = delete;
    AnimationSequence & operator=( const std::vector<int> & rhs );

    virtual ~AnimationSequence();

    int playAnimation( bool loop = false );
    virtual int restartAnimation();

    int getFrame() const;
    int firstFrame() const;
    size_t animationLength() const;
    virtual double movementProgress() const;
    bool isLastFrame() const;
    virtual bool isValid() const;

protected:
    std::vector<int> _seq;
    size_t _currentFrame;
};

class AnimationReference
{
public:
    AnimationReference();
    explicit AnimationReference( int id );

    virtual ~AnimationReference() = default;

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
    monsterReturnAnim _flying;
    std::vector<int> _wince;
    std::vector<int> _death;
    monsterReturnAnim _melee[3];
    monsterReturnAnim _ranged[3];
    std::vector<std::vector<int> > _idle;
    std::vector<std::vector<int> > _offsetX;

    bool appendFrames( std::vector<int> & target, int animID );
};

class AnimationState : public AnimationReference
{
public:
    explicit AnimationState( int monsterID );
    ~AnimationState() override = default;

    bool switchAnimation( int animstate, bool reverse = false );
    bool switchAnimation( const std::vector<int> & animationList, bool reverse = false );
    int getCurrentState() const;

    // pass-down methods
    int playAnimation( bool loop = false );
    int restartAnimation();

    int getFrame() const;
    int firstFrame() const;
    size_t animationLength() const;
    double movementProgress() const;
    bool isLastFrame() const;
    bool isValid() const;

private:
    int _animState;
    AnimationSequence _currentSequence;
};
#endif
