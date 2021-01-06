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

#ifndef H2BIN_FRM_H
#define H2BIN_FRM_H

#include "settings.h"

#include <map>
#include <vector>

namespace Bin_Info
{
    struct MonsterAnimInfo
    {
        enum ANIM_TYPE
        {
            MOVE_START, // Start of the moving sequence on 1st animation cycle: flyers will fly up
            MOVE_TILE_START, // Supposed to be played at the beginning of 2nd+ move.
            MOVE_MAIN, // Core animation. Most units only have this one.
            MOVE_TILE_END, // Cavalry & wolf. Played at the end of the cycle (2nd tile to 3rd), but not at the last one
            MOVE_STOP, // End of the moving sequence when arrived: landing for example
            MOVE_ONE, // Used when moving 1 tile. LICH and POWER_LICH doesn't have this, use MOVE_MAIN
            TEMPORARY, // This is an empty placeholder for combined animation built from previous parts
            STATIC, // Frame 1
            IDLE1,
            IDLE2, // Idle animations: picked at random with different probablities, rarely all 5 present
            IDLE3,
            IDLE4,
            IDLE5,
            DEATH,
            WINCE_UP,
            WINCE_END,
            ATTACK1, // Attacks, number represents the angle: 1 is TOP, 2 is CENTER, 3 is BOTTOM
            ATTACK1_END,
            DOUBLEHEX1,
            DOUBLEHEX1_END,
            ATTACK2,
            ATTACK2_END,
            DOUBLEHEX2,
            DOUBLEHEX2_END,
            ATTACK3,
            ATTACK3_END,
            DOUBLEHEX3,
            DOUBLEHEX3_END,
            SHOOT1,
            SHOOT1_END,
            SHOOT2,
            SHOOT2_END,
            SHOOT3,
            SHOOT3_END
        };

        std::vector<std::vector<int> > frameXOffset;
        uint32_t moveSpeed;
        uint32_t shootSpeed;
        uint32_t flightSpeed;
        Point eyePosition;
        int32_t troopCountOffsetLeft;
        int32_t troopCountOffsetRight;
        std::vector<Point> projectileOffset;
        std::vector<float> projectileAngles;
        std::vector<float> idlePriority;
        std::vector<uint32_t> unusedIdleDelays;
        uint32_t idleAnimationCount;
        uint32_t idleAnimationDelay;
        std::vector<std::vector<int> > animationFrames;

        MonsterAnimInfo( int monsterID = 0, const std::vector<uint8_t> & bytes = std::vector<uint8_t>() );
        bool hasAnim( int animID = MonsterAnimInfo::STATIC ) const;
        bool isValid() const;
        size_t getProjectileID( float angle ) const;
    };

    void InitBinInfo();
    MonsterAnimInfo GetMonsterInfo( uint32_t monsterID );
}
#endif
