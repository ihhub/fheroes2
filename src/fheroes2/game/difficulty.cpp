/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
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

#include "difficulty.h"

#include <cassert>

#include "translations.h"

std::string Difficulty::String( int difficulty )
{
    switch ( difficulty ) {
    case Difficulty::EASY:
        return _( "difficulty|Easy" );
    case Difficulty::NORMAL:
        return _( "difficulty|Normal" );
    case Difficulty::HARD:
        return _( "difficulty|Hard" );
    case Difficulty::EXPERT:
        return _( "difficulty|Expert" );
    case Difficulty::IMPOSSIBLE:
        return _( "difficulty|Impossible" );
    default:
        break;
    }

    return "Unknown";
}

int Difficulty::GetScoutingBonus( int difficulty )
{
    switch ( difficulty ) {
    case Difficulty::NORMAL:
        return 1;
    case Difficulty::HARD:
        return 2;
    case Difficulty::EXPERT:
        return 3;
    case Difficulty::IMPOSSIBLE:
        return 4;
    default:
        break;
    }
    return 0;
}

double Difficulty::getGoldIncomeBonusForAI( const int difficulty )
{
    switch ( difficulty ) {
    case Difficulty::EASY:
        // It is deduction from the income.
        return -0.25;
    case Difficulty::HARD:
        return 0.29;
    case Difficulty::EXPERT:
        return 0.45;
    case Difficulty::IMPOSSIBLE:
        return 0.6;
    default:
        break;
    }
    return 0;
}

double Difficulty::GetUnitGrowthBonusForAI( const int difficulty )
{
    // In the original game AI has a cheeky monster growth bonus depending on difficulty:
    // Easy - 0.0 (no bonus)
    // Normal - 0.0 (no bonus)
    // Hard - 0.20 (or 20% extra)
    // Expert - 0.32 (or 32% extra)
    // Impossible - 0.44 (or 44% extra)
    // This bonus was introduced to compensate weak AI in the game.
    //
    // However, with introduction of proper AI in this engine AI has become much stronger and some maps are impossible to beat.
    // Also this bonus can be abused by players while capturing AI castles on a first day of a week.
    //
    // Completely removing these bonuses might break some maps and they become unplayable.
    // Therefore, these bonuses are reduced by 5% which is the value of noise in many processes / systems.

    switch ( difficulty ) {
    case Difficulty::EASY:
    case Difficulty::NORMAL:
        return 0;
    case Difficulty::HARD:
        return 0.14;
    case Difficulty::EXPERT:
        return 0.254;
    case Difficulty::IMPOSSIBLE:
        return 0.368;
    default:
        // Did you add a new difficulty level? Add the logic above!
        assert( 0 );
        break;
    }
    return 0;
}

int Difficulty::GetHeroMovementBonus( int difficulty )
{
    switch ( difficulty ) {
    case Difficulty::EXPERT:
    case Difficulty::IMPOSSIBLE:
        return 75;
    default:
        break;
    }
    return 0;
}

double Difficulty::GetAIRetreatRatio( int difficulty )
{
    switch ( difficulty ) {
    case Difficulty::NORMAL:
        return 100.0 / 7.5;
    case Difficulty::HARD: // fall-through
    case Difficulty::EXPERT:
        return 100.0 / 8.5;
    case Difficulty::IMPOSSIBLE:
        return 100.0 / 10.0;
    default:
        break;
    }
    return 100.0 / 6.0;
}

uint32_t Difficulty::GetDimensionDoorLimit( int difficulty )
{
    switch ( difficulty ) {
    case Difficulty::EASY:
        return 1;
    case Difficulty::NORMAL:
        return 2;
    case Difficulty::HARD:
        return 3;
    default:
        break;
    }
    return UINT32_MAX;
}

bool Difficulty::areAIHeroRolesAllowed( const int difficulty )
{
    switch ( difficulty ) {
    case Difficulty::EASY:
        return false;
    case Difficulty::NORMAL:
    case Difficulty::HARD:
    case Difficulty::EXPERT:
    case Difficulty::IMPOSSIBLE:
        return true;
    default:
        // Did you add a new difficulty level? Add the logic above!
        assert( 0 );
        break;
    }

    return true;
}

int Difficulty::getMinStatDiffBetweenAIRoles( const int difficulty )
{
    switch ( difficulty ) {
    case Difficulty::EASY:
        // Easy difficulty still allows to merge armies but only if the difference in stats is huge.
        return 10;
    case Difficulty::NORMAL:
    case Difficulty::HARD:
    case Difficulty::EXPERT:
    case Difficulty::IMPOSSIBLE:
        return 2;
    default:
        // Did you add a new difficulty level? Add the logic above!
        assert( 0 );
        break;
    }

    return 2;
}

bool Difficulty::allowAIToSplitWeakStacks( const int difficulty )
{
    switch ( difficulty ) {
    case Difficulty::EASY:
    case Difficulty::NORMAL:
        return false;
    default:
        break;
    }
    return true;
}
