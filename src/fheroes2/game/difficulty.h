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

#include "castle.h"
#include "resource.h"

class Kingdom;

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

    // Returns an extra resource bonus for AI based on difficulty level.
    Funds getResourceIncomeBonusForAI( const int difficulty, const Kingdom & kingdom );

    // Returns an extra gold bonus modifier for AI based on difficulty level. This modifier is applied after applying the resource income bonus.
    double getGoldIncomeBonusForAI( const int difficulty );

    // Returns the ratio of the strength of the enemy army to the strength of the AI army, above which the AI decides to surrender or retreat from the battlefield
    double getArmyStrengthRatioForAIRetreat( const int difficulty );

    // Returns the limit on the number of times the Dimension Door spell can be cast, which is applied to each of the AI-controlled heroes individually during one AI
    // turn
    uint32_t GetDimensionDoorLimitForAI( int difficulty );

    bool areAIHeroRolesAllowed( const int difficulty );

    // Returns the minimum advantage in stats (i.e. the sum of the levels of primary and secondary skills) that a hero must have in order to allow another hero with the
    // same role to meet on his own initiative with this hero to exchange armies and artifacts
    int getMinStatDiffForAIHeroesMeeting( const int difficulty );

    // Returns true if AI should avoid having free slots in the army
    bool allowAIToSplitWeakStacks( const int difficulty );

    bool allowAIToDevelopCastlesOnDay( const int difficulty, const bool isCampaign, const uint32_t day );
    bool allowAIToBuildCastleBuilding( const int difficulty, const bool isCampaign, const building_t building );
}

#endif
