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
#include "gamedefs.h"
#include "monster_info.h"

struct monsterReturnAnim
{
    std::vector<int> start;
    std::vector<int> end;
};

class AnimationSequence
{
public:
    AnimationSequence( const std::vector<int> & seq );

    AnimationSequence & operator=( const std::vector<int> & rhs );

    int playAnimation( bool loop = false );
    int restartAnimation();
    void setToLastFrame();

    int getFrame() const;
    int firstFrame() const;
    int animationLength() const;
    double movementProgress() const;
    bool isFirstFrame() const;
    bool isLastFrame() const;
    bool isValid() const;

private:
    std::vector<int> _seq;
    size_t _currentFrame;

    AnimationSequence();
};

class AnimationReference
{
public:
    AnimationReference();
    AnimationReference( const Bin_Info::MonsterAnimInfo & info, int id );
    virtual ~AnimationReference();

    int getStaticFrame() const;
    int getDeathFrame() const;

    const std::vector<int> & getAnimationVector( int animstate ) const;
    AnimationSequence getAnimationSequence( int animstate ) const;

protected:
    int _type;

    std::vector<int> _static;
    std::vector<int> _quickMove;
    std::vector<int> _loopMove;
    monsterReturnAnim _moveModes;
    std::vector<int> _wince;
    std::vector<int> _death;
    monsterReturnAnim _melee[3];
    monsterReturnAnim _ranged[3];
    std::vector<std::vector<int> > _idle;

    bool appendFrames( const Bin_Info::MonsterAnimInfo & info, std::vector<int> & target, int animID, bool critical = false );
};

class AnimationState : public AnimationReference
{
public:
    AnimationState( const AnimationReference & animMap, int state );
    virtual ~AnimationState();

    bool switchAnimation( int animstate, bool reverse = false );
    bool switchAnimation( const std::vector<int> & animationList, bool reverse = false );
    int getCurrentState() const;

    // pass-down methods
    int playAnimation( bool loop = false );
    int restartAnimation();
    void setToLastFrame();

    int getFrame() const;
    int firstFrame() const;
    int animationLength() const;
    double movementProgress() const;
    bool isFirstFrame() const;
    bool isLastFrame() const;
    bool isValid() const;

private:
    int _animState;
    AnimationSequence _currentSequence;
};
#endif
