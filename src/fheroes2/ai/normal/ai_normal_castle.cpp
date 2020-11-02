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

#include "ai_normal.h"
#include "castle.h"
#include "heroes.h"
#include "kingdom.h"
#include "race.h"
#include "world.h"

namespace AI
{
    struct BuildOrder
    {
        building_t building = BUILD_NOTHING;
        int priority = 1;

        BuildOrder() {}
        BuildOrder( building_t b, int p )
            : building( b )
            , priority( p )
        {}
    };

    const std::vector<BuildOrder> & GetIncomeStructures( int type )
    {
        static const std::vector<BuildOrder> standard = {{BUILD_CASTLE, 1}, {BUILD_STATUE, 1}};
        static const std::vector<BuildOrder> warlock = {{BUILD_CASTLE, 1}, {BUILD_STATUE, 1}, {BUILD_SPEC, 1}};

        return ( type == Race::WRLK ) ? warlock : standard;
    }

    const std::vector<BuildOrder> & GetDefensiveStructures( int )
    {
        static const std::vector<BuildOrder> defensive
            = {{BUILD_LEFTTURRET, 1}, {BUILD_RIGHTTURRET, 1}, {BUILD_MOAT, 1}, {BUILD_CAPTAIN, 1}, {BUILD_SPEC, 2}, {BUILD_TAVERN, 1}};

        return defensive;
    }

    const std::vector<BuildOrder> & GetBuildOrder( int type )
    {
        static const std::vector<BuildOrder> genericBuildOrder
            = {{BUILD_CASTLE, 2},      {BUILD_STATUE, 1},      {DWELLING_UPGRADE7, 1}, {DWELLING_UPGRADE6, 1}, {DWELLING_MONSTER6, 1}, {DWELLING_UPGRADE5, 1},
               {DWELLING_MONSTER5, 1}, {DWELLING_UPGRADE4, 1}, {DWELLING_MONSTER4, 1}, {DWELLING_UPGRADE3, 2}, {DWELLING_MONSTER3, 2}, {DWELLING_UPGRADE2, 3},
               {DWELLING_MONSTER2, 3}, {DWELLING_MONSTER1, 4}, {BUILD_MAGEGUILD1, 2},  {BUILD_WEL2, 10},       {BUILD_TAVERN, 5},      {BUILD_THIEVESGUILD, 10},
               {BUILD_MAGEGUILD2, 3},  {BUILD_MAGEGUILD3, 4},  {BUILD_MAGEGUILD4, 5},  {BUILD_MAGEGUILD5, 5},  {BUILD_SHIPYARD, 4},    {BUILD_MARKETPLACE, 10}};

        // De-prioritizing dwelling 5 (you can reach 6 without it), 1 and upgrades of 3 and 4
        // Well, tavern and Archery upgrade are more important
        static const std::vector<BuildOrder> knightBuildOrder
            = {{BUILD_CASTLE, 2},      {BUILD_STATUE, 1},        {DWELLING_UPGRADE6, 2}, {DWELLING_MONSTER6, 1}, {DWELLING_UPGRADE5, 2},
               {DWELLING_MONSTER5, 2}, {DWELLING_UPGRADE4, 2},   {DWELLING_MONSTER4, 1}, {DWELLING_UPGRADE3, 2}, {DWELLING_MONSTER3, 1},
               {DWELLING_UPGRADE2, 1}, {DWELLING_MONSTER2, 3},   {DWELLING_MONSTER1, 4}, {BUILD_WELL, 1},        {BUILD_TAVERN, 1},
               {BUILD_MAGEGUILD1, 2},  {BUILD_MAGEGUILD2, 3},    {BUILD_MAGEGUILD3, 5},  {BUILD_MAGEGUILD4, 5},  {BUILD_MAGEGUILD5, 5},
               {BUILD_SPEC, 5},        {BUILD_THIEVESGUILD, 10}, {BUILD_WEL2, 20},       {BUILD_SHIPYARD, 4},    {BUILD_MARKETPLACE, 10}};

        // Priority on Dwellings 5/6 and Mage guild level 2
        static const std::vector<BuildOrder> necromancerBuildOrder
            = {{BUILD_CASTLE, 2},      {BUILD_STATUE, 1},      {DWELLING_UPGRADE6, 1}, {DWELLING_MONSTER6, 1}, {DWELLING_UPGRADE5, 2}, {DWELLING_MONSTER5, 1},
               {BUILD_MAGEGUILD1, 1},  {DWELLING_UPGRADE4, 2}, {DWELLING_MONSTER4, 1}, {DWELLING_UPGRADE3, 3}, {DWELLING_MONSTER3, 3}, {DWELLING_UPGRADE2, 4},
               {DWELLING_MONSTER2, 2}, {DWELLING_MONSTER1, 3}, {BUILD_MAGEGUILD2, 2},  {BUILD_WEL2, 8},        {BUILD_MAGEGUILD3, 4},  {BUILD_MAGEGUILD4, 5},
               {BUILD_MAGEGUILD5, 5},  {BUILD_SHRINE, 10},     {BUILD_SHIPYARD, 4},    {BUILD_MARKETPLACE, 10}};

        // Priority on Mage tower/guild and library
        static const std::vector<BuildOrder> wizardBuildOrder
            = {{BUILD_CASTLE, 2},      {BUILD_STATUE, 1},        {DWELLING_UPGRADE6, 1}, {DWELLING_MONSTER6, 1}, {DWELLING_UPGRADE5, 1}, {DWELLING_MONSTER5, 1},
               {DWELLING_MONSTER4, 1}, {DWELLING_MONSTER3, 1},   {DWELLING_MONSTER2, 1}, {DWELLING_MONSTER1, 1}, {BUILD_MAGEGUILD1, 1},  {DWELLING_UPGRADE3, 4},
               {BUILD_SPEC, 2},        {BUILD_WEL2, 8},          {BUILD_MAGEGUILD2, 3},  {BUILD_MAGEGUILD3, 4},  {BUILD_MAGEGUILD4, 4},  {BUILD_MAGEGUILD5, 4},
               {BUILD_TAVERN, 10},     {BUILD_THIEVESGUILD, 10}, {BUILD_SHIPYARD, 4},    {BUILD_MARKETPLACE, 10}};

        switch ( type ) {
        case Race::KNGT:
            return knightBuildOrder;
        case Race::NECR:
            return necromancerBuildOrder;
        case Race::WZRD:
            return wizardBuildOrder;
        default:
            break;
        }

        return genericBuildOrder;
    }

    bool Build( Castle & castle, const std::vector<BuildOrder> & buildOrderList, int multiplier = 1 )
    {
        for ( std::vector<BuildOrder>::const_iterator it = buildOrderList.begin(); it != buildOrderList.end(); ++it ) {
            const int priority = it->priority * multiplier;
            if ( priority == 1 ) {
                if ( BuildIfAvailable( castle, it->building ) )
                    return true;
            }
            else {
                if ( BuildIfEnoughResources( castle, it->building, GetResourceMultiplier( castle, priority, priority + 1 ) ) )
                    return true;
            }
        }
        return false;
    }

    bool CastleDevelopment( Castle & castle )
    {
        if ( !castle.isBuild( BUILD_WELL ) && world.LastDay() ) {
            // return right away - if you can't buy Well you can't buy anything else
            return BuildIfAvailable( castle, BUILD_WELL );
        }

        if ( Build( castle, GetIncomeStructures( castle.GetRace() ) ) ) {
            return true;
        }

        const size_t neighbourRegions = world.getRegion( world.GetTiles( castle.GetIndex() ).GetRegion() ).getNeighboursCount();
        const bool islandOrPeninsula = neighbourRegions < 3;

        // force building a shipyard, +1 to cost check since we can have 0 neighbours
        if ( islandOrPeninsula && BuildIfEnoughResources( castle, BUILD_SHIPYARD, neighbourRegions + 1 ) ) {
            return true;
        }

        if ( Build( castle, GetBuildOrder( castle.GetRace() ) ) ) {
            return true;
        }

        // Call internally checks if it's valid (space/resources) to buy one
        if ( castle.GetKingdom().GetFunds() >= PaymentConditions::BuyBoat() * ( islandOrPeninsula ? 2 : 4 ) )
            castle.BuyBoat();

        return Build( castle, GetDefensiveStructures( castle.GetRace() ), 10 );
    }

    void Normal::CastleTurn( Castle & castle, bool defensive )
    {
        if ( defensive ) {
            Build( castle, GetDefensiveStructures( castle.GetRace() ) );

            castle.recruitBestAvailable( castle.GetKingdom().GetFunds() );
        }
        else {
            CastleDevelopment( castle );
        }
    }
}
