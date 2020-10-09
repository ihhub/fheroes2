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

#pragma once

#include "pathfinding.h"
#include "route.h"

class WorldPathfinder : public Pathfinder<PathfindingNode>
{
public:
    WorldPathfinder() {}
    virtual void reset();
    virtual std::list<Route::Step> buildPath( int target ) const;
    void reEvaluateIfNeeded( int start, uint8_t skill );
    bool isBlockedByObject( int target, bool fromWater = false );
    uint32_t getMovementPenalty( int start, int target, int direction, uint8_t skill = Skill::Level::NONE ) const;
    int searchForFog( int playerColor, int start, uint8_t skill = Skill::Level::NONE );

protected:
    void evaluateMap( int start, uint8_t skill );

    uint8_t _pathfindingSkill = 0;
};

class AIWorldPathfinder : public WorldPathfinder
{
public:
    AIWorldPathfinder() {}
    virtual void reset();
    void reEvaluateIfNeeded( int start, uint8_t skill, double armyStrength, int color );

private:
    void evaluateMap( int start, uint8_t skill, double armyStrength, int color );

    double _armyStrength = -1;
    int _currentColor = Color::NONE;
};
