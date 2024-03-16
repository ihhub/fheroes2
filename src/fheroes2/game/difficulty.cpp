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

#include "difficulty.h"

#include <cassert>
#include <vector>

#include "profit.h"
#include "race.h"
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

int Difficulty::GetScoutingBonusForAI( int difficulty )
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

Funds Difficulty::getResourceIncomeBonusForAI( const int difficulty, const VecCastles & castles )
{
    const auto getIncomeFromSetsOfResourceMines = []( const uint32_t numOfSets ) {
        Funds result;

        Resource::forEach( ( Resource::ALL & ~Resource::GOLD ), [&result]( const int res ) { result += ProfitConditions::FromMine( res ); } );

        return result * numOfSets;
    };

    const auto getBonusForCastles = [&castles]() {
        Funds result;

        for ( const Castle * castle : castles ) {
            assert( castle != nullptr );

            switch ( castle->GetRace() ) {
            case Race::KNGT:
            case Race::BARB:
                result += { 0, 0, 0, 0, 1, 0, 0 }; // 1 unit of Crystal
                break;
            case Race::SORC:
                result += { 0, 0, 1, 0, 0, 0, 0 }; // 1 unit of Mercury
                break;
            case Race::WRLK:
            case Race::NECR:
                result += { 0, 0, 0, 1, 0, 0, 0 }; // 1 unit of Sulfur
                break;
            case Race::WZRD:
                result += { 0, 0, 0, 0, 0, 1, 0 }; // 1 unit of Gems
                break;
            default:
                assert( 0 );
                break;
            }
        }

        return result;
    };

    switch ( difficulty ) {
    case Difficulty::HARD:
        return getIncomeFromSetsOfResourceMines( 1 );
    case Difficulty::EXPERT:
        return getIncomeFromSetsOfResourceMines( 2 ) + getBonusForCastles();
    case Difficulty::IMPOSSIBLE:
        return getIncomeFromSetsOfResourceMines( 3 ) + getBonusForCastles();
    default:
        break;
    }

    return {};
}

double Difficulty::getGoldIncomeBonusForAI( const int difficulty )
{
    switch ( difficulty ) {
    case Difficulty::EASY:
        // It is deduction from the income.
        return -0.25;
    case Difficulty::HARD:
        return 1.0;
    case Difficulty::EXPERT:
        return 2.0;
    case Difficulty::IMPOSSIBLE:
        return 3.0;
    default:
        break;
    }

    return 0;
}

int Difficulty::GetHeroMovementBonusForAI( int difficulty )
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

double Difficulty::getArmyStrengthRatioForAIRetreat( const int difficulty )
{
    switch ( difficulty ) {
    case Difficulty::NORMAL:
        return 100.0 / 7.5;
    case Difficulty::HARD:
    case Difficulty::EXPERT:
        return 100.0 / 8.5;
    case Difficulty::IMPOSSIBLE:
        return 100.0 / 10.0;
    default:
        break;
    }

    return 100.0 / 6.0;
}

uint32_t Difficulty::GetDimensionDoorLimitForAI( int difficulty )
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

int Difficulty::getMinStatDiffForAIHeroesMeeting( const int difficulty )
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

bool Difficulty::allowAIToDevelopCastlesOnDay( const int difficulty, const bool isCampaign, const uint32_t day )
{
    switch ( difficulty ) {
    case Difficulty::EASY:
        return isCampaign || day % 2 == 0;
    default:
        break;
    }

    return true;
}

bool Difficulty::allowAIToBuildCastleBuilding( const int difficulty, const bool isCampaign, const building_t building )
{
    switch ( difficulty ) {
    case Difficulty::EASY:
        // Only the construction of the corresponding dwelling is limited, but not its upgrade
        return isCampaign || ( building != DWELLING_MONSTER6 && building != BUILD_MAGEGUILD5 );
    default:
        break;
    }

    return true;
}
