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
        hero.GetArmy().UpgradeTroops( castle );
        castle.recruitBestAvailable( budget );
        hero.GetArmy().JoinStrongestFromArmy( castle.GetArmy() );
    }
}
