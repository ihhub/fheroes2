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

#include <algorithm>
#include <cassert>
#include <vector>

#include "castle.h"
#include "kingdom.h"
#include "profit.h"
#include "race.h"
#include "translations.h"
#include "world.h"

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

Funds Difficulty::getResourceIncomeBonusForAI( const int difficulty, const Kingdom & kingdom )
{
    assert( kingdom.isControlAI() );

    const auto getIncomeFromSetsOfResourceMines = []( const int resourceTypes, const uint32_t numOfSets ) {
        Funds result;

        Resource::forEach( resourceTypes, [&result]( const int res ) { result += ProfitConditions::FromMine( res ); } );

        return result * numOfSets;
    };

    const auto getBonusForCastles = [kingdomColor = kingdom.GetColor(), &kingdomCastles = kingdom.GetCastles()]() {
        Funds result;

        const bool kingdomHasMarketplace = std::any_of( kingdomCastles.begin(), kingdomCastles.end(), []( const Castle * castle ) {
            assert( castle != nullptr );

            return castle->isBuild( BUILD_MARKETPLACE );
        } );

        // Additional rare resources for hiring units from higher-level dwellings can only be provided if the kingdom already has some source of those resources - either
        // through trade or through mining
        const auto doesKingdomHaveResourceSource = [kingdomColor, kingdomHasMarketplace]( const int resourceType ) {
            return ( kingdomHasMarketplace || world.CountCapturedMines( resourceType, kingdomColor ) > 0 );
        };

        for ( const Castle * castle : kingdomCastles ) {
            assert( castle != nullptr );

            // AI at higher difficulty levels should be able to fully redeem the weekly unit growth in its castles
            result += ProfitConditions::FromMine( Resource::GOLD );

            // Provide additional resources only if there are higher-level dwellings in the castle to avoid distortions in the castle's development rate
            if ( !castle->isBuild( DWELLING_MONSTER6 ) ) {
                continue;
            }

            switch ( castle->GetRace() ) {
            case Race::KNGT:
            case Race::NECR:
                // Rare resources are not required to hire maximum-level units in these castles
                break;
            case Race::BARB:
                if ( doesKingdomHaveResourceSource( Resource::CRYSTAL ) ) {
                    result += ProfitConditions::FromMine( Resource::CRYSTAL );
                }
                break;
            case Race::SORC:
                if ( doesKingdomHaveResourceSource( Resource::MERCURY ) ) {
                    result += ProfitConditions::FromMine( Resource::MERCURY );
                }
                break;
            case Race::WRLK:
                if ( doesKingdomHaveResourceSource( Resource::SULFUR ) ) {
                    result += ProfitConditions::FromMine( Resource::SULFUR );
                }
                // The maximum level units in this castle are more expensive than in others
                result += ProfitConditions::FromMine( Resource::GOLD );
                break;
            case Race::WZRD:
                if ( doesKingdomHaveResourceSource( Resource::GEMS ) ) {
                    result += ProfitConditions::FromMine( Resource::GEMS );
                }
                // The maximum level units in this castle are more expensive than in others
                result += ProfitConditions::FromMine( Resource::GOLD );
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
        return getIncomeFromSetsOfResourceMines( Resource::GOLD, 1 );
    case Difficulty::EXPERT:
        return getIncomeFromSetsOfResourceMines( Resource::GOLD, 1 ) + getBonusForCastles();
    case Difficulty::IMPOSSIBLE:
        return getIncomeFromSetsOfResourceMines( Resource::GOLD, 2 ) + getBonusForCastles();
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

bool Difficulty::allowAIToBuildCastleBuilding( const int difficulty, const bool isCampaign, const BuildingType building )
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
