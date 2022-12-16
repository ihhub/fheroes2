/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2022                                             *
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

#include <array>
#include <cstdint>
#include <list>
#include <vector>

#include "battle_animation.h"

class Monster;

namespace Monster_Info
{
    enum AttackDirection : int
    {
        TOP,
        FRONT,
        BOTTOM
    };

    enum AnimationType : int
    {
        NONE,
        STAND_STILL, // This animation contains one frame of a standing unit.
        STATIC, // This is a copy of STAND STILL, with the difference that the game engine may randomly change it to IDLE.
        IDLE, // After the idle animation is done, the game engine changes it to STATIC.
        MOVE_START,
        MOVING,
        MOVE_END,
        MOVE_QUICK,
        FLY_UP,
        FLY_LAND,
        MELEE_TOP,
        MELEE_TOP_END,
        MELEE_FRONT,
        MELEE_FRONT_END,
        MELEE_BOT,
        MELEE_BOT_END,
        RANG_TOP,
        RANG_TOP_END,
        RANG_FRONT,
        RANG_FRONT_END,
        RANG_BOT,
        RANG_BOT_END,
        WNCE_UP,
        WNCE_DOWN,
        WNCE, // combined UP and RETURN anim
        KILL,
        INVALID
    };
}

namespace fheroes2
{
    const std::array<uint8_t, 15> & getMonsterAnimationSequence();

    class RandomMonsterAnimation
    {
    public:
        explicit RandomMonsterAnimation( const Monster & monster );

        RandomMonsterAnimation( const RandomMonsterAnimation & ) = delete;
        RandomMonsterAnimation( RandomMonsterAnimation && ) = default;

        RandomMonsterAnimation & operator=( const RandomMonsterAnimation & ) = delete;

        void increment();

        int icnFile() const;
        int frameId() const;
        int offset() const;

        void reset(); // reset to static animation

    private:
        AnimationReference _reference;
        int _icnID;
        std::vector<int> _validMoves;
        std::list<int> _frameSet;
        std::list<int> _offsetSet;
        int _frameId;
        int _frameOffset;
        bool _isFlyer;

        void _pushFrames( const Monster_Info::AnimationType type );
        void _addValidMove( const Monster_Info::AnimationType type );
        void _updateFrameInfo();
    };
}
