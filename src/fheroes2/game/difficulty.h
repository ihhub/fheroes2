/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
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
#ifndef H2DIFFICULTY_H
#define H2DIFFICULTY_H

#include <cstdint>
#include <string>

namespace Difficulty
{
    // !!! IMPORTANT !!!
    // Do NOT change the order of the items as they are used for the map format.
    enum DifficultyLevel : int
    {
        EASY,
        NORMAL,
        HARD,
        EXPERT,
        IMPOSSIBLE
    };

    std::string String( int );

    int GetScoutingBonus( int difficulty );

    // Returns an extra gold bonus modifier for AI based on difficulty level.
    double getGoldIncomeBonusForAI( const int difficulty );

    // Returns an extra growth bonus modifier for AI based on difficulty level.
    double GetUnitGrowthBonusForAI( const int difficulty );

    int GetHeroMovementBonus( int difficulty );

    // Returns the ratio of the strength of the enemy army to the strength of the AI army, above which the AI decides to retreat from the battlefield
    double getArmyStrengthRatioForAIRetreat( const int difficulty );

    // Returns the ratio of the AI kingdom's gold reserve to the cost of surrender, above which the AI will prefer surrender to retreat from the battlefield
    uint32_t getGoldReserveRatioForAISurrender( const int difficulty );

    uint32_t GetDimensionDoorLimit( int difficulty );

    bool areAIHeroRolesAllowed( const int difficulty );

    int getMinStatDiffBetweenAIRoles( const int difficulty );

    // Returns true if AI should avoid having free slots in the army
    bool allowAIToSplitWeakStacks( const int difficulty );
}

#endif
