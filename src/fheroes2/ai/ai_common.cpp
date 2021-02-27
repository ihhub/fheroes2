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

#include "ai.h"
#include "army.h"
#include "castle.h"
#include "kingdom.h"
#include "normal/ai_normal.h"

namespace AI
{
    // AI Selector here
    Base & Get( AI_TYPE /*type*/ ) // type might be used sometime in the future
    {
        static AI::Normal normal;
        return normal;
    }

    bool BuildIfAvailable( Castle & castle, int building )
    {
        if ( !castle.isBuild( building ) )
            return castle.BuyBuilding( building );
        return false;
    }

    bool BuildIfEnoughResources( Castle & castle, int building, uint32_t minimumMultiplicator )
    {
        if ( minimumMultiplicator < 1 || minimumMultiplicator > 99 ) // can't be that we need more than 100 times resources
            return false;

        const Kingdom & kingdom = castle.GetKingdom();
        if ( kingdom.GetFunds() >= PaymentConditions::BuyBuilding( castle.GetRace(), building ) * minimumMultiplicator )
            return BuildIfAvailable( castle, building );
        return false;
    }

    uint32_t GetResourceMultiplier( const Castle & castle, uint32_t min, uint32_t max )
    {
        return castle.isCapital() ? 1 : Rand::Get( min, max );
    }

    void ReinforceHeroInCastle( Heroes & hero, Castle & castle, const Funds & budget )
    {
        if ( !hero.HaveSpellBook() && castle.GetLevelMageGuild() > 0 && !hero.IsFullBagArtifacts() ) {
            // this call will check if AI kingdom have enough resources to buy book
            hero.BuySpellBook( &castle );
        }

        Army & heroArmy = hero.GetArmy();
        heroArmy.UpgradeTroops( castle );
        castle.recruitBestAvailable( budget );
        heroArmy.JoinStrongestFromArmy( castle.GetArmy() );
        OptimizeTroopsOrder( heroArmy );
    }

    void OptimizeTroopsOrder( Army & army )
    {
        // Optimize troops placement before the battle
        std::vector<Troop> archers;
        std::vector<Troop> others;

        // Validate and pick the troops
        for ( size_t slot = 0; slot < ARMYMAXTROOPS; ++slot ) {
            Troop * troop = army.GetTroop( slot );
            if ( troop && troop->isValid() ) {
                if ( troop->isArchers() ) {
                    archers.push_back( *troop );
                }
                else {
                    others.push_back( *troop );
                }
            }
        }

        // Sort troops by tactical priority. For melee:
        // 1. Faster units first
        // 2. Flyers first
        // 3. Finally if unit type and speed is same, compare by strength
        std::sort( others.begin(), others.end(), []( const Troop & left, const Troop & right ) {
            if ( left.GetSpeed() == right.GetSpeed() ) {
                if ( left.isFlying() == right.isFlying() ) {
                    return left.GetStrength() < right.GetStrength();
                }
                return right.isFlying();
            }
            return left.GetSpeed() < right.GetSpeed();
        } );

        // Archers sorted purely by strength.
        std::sort( archers.begin(), archers.end(), []( const Troop & left, const Troop & right ) { return left.GetStrength() < right.GetStrength(); } );

        std::vector<size_t> slotOrder = {2, 1, 3, 0, 4};
        switch ( archers.size() ) {
        case 1:
            slotOrder = {0, 2, 1, 3, 4};
            break;
        case 2:
            slotOrder = {0, 4, 2, 1, 3};
            break;
        case 3:
            slotOrder = {0, 4, 2, 1, 3};
            break;
        case 4:
            slotOrder = {0, 4, 2, 3, 1};
            break;
        case 5:
            slotOrder = {0, 4, 1, 2, 3};
            break;
        default:
            break;
        }

        // Re-arrange troops in army
        army.Clean();
        for ( const size_t slot : slotOrder ) {
            if ( !archers.empty() ) {
                army.GetTroop( slot )->Set( archers.back() );
                archers.pop_back();
            }
            else if ( !others.empty() ) {
                army.GetTroop( slot )->Set( others.back() );
                others.pop_back();
            }
            else {
                break;
            }
        }
    }
}
