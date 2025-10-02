/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2024 - 2025                                             *
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

#include <cstdint>
#include <optional>

class Army;
class Castle;
class Heroes;
class Kingdom;
class UltimateArtifact;

struct Funds;

enum BuildingType : uint32_t;

namespace AI
{
    // Builds the given building in the given castle if possible. If possible and necessary, obtains the resources
    // that are missing for construction through trading on the marketplace. Returns true if the construction was
    // successful, otherwise returns false.
    bool BuildIfPossible( Castle & castle, const BuildingType building );

    // Builds the given building in the given castle if possible and if there is a sufficient supply of resources
    // (see the implementation for details). If possible and necessary, obtains the resources that are missing for
    // construction through trading on the marketplace. Returns true if the construction was successful, otherwise
    // returns false.
    bool BuildIfEnoughFunds( Castle & castle, const BuildingType building, const uint32_t fundsMultiplier );

    // Performs the pre-battle arrangement of the given army, see the implementation for details
    void OptimizeTroopsOrder( Army & army );

    // Transfers the slowest troops from the hero's army to the garrison to try to get a movement bonus on the next turn
    void transferSlowestTroopsToGarrison( Heroes * hero, Castle * castle );

    // Calculates a marketplace transaction, after which the kingdom would be able to make a payment in the amount of
    // at least 'fundsToObtain'. Returns the corresponding transaction if it was found, otherwise returns an empty
    // result. In order to receive the necessary funds, the returned transaction must be deducted from the funds of
    // the kingdom.
    std::optional<Funds> calculateMarketplaceTransaction( const Kingdom & kingdom, const Funds & fundsToObtain );

    // Performs operations on the marketplace necessary for the kingdom to be able to make a payment in the amount
    // of at least 'fundsToObtain'. If the necessary funds cannot be obtained as a result of trading, then the current
    // funds of the kingdom remain unchanged. Returns true if the trade was successful, otherwise returns false.
    bool tradeAtMarketplace( Kingdom & kingdom, const Funds & fundsToObtain );

    // Shares information about an object located at a given tile with AI allies.
    // It is assumed that allied AI players can talk like human players do in real life.
    // The information is shared only if all allies are AI. If any of them is a human player then no information will be shared.
    void shareObjectVisitInfoWithAllies( const Kingdom & kingdom, const int32_t tileIndex );

    // Returns true if Ultimate Artifact is available to the given hero for pickup, otherwise returns false. In short,
    // the Ultimate Artifact is considered available to the given hero if this hero knows its exact location and there
    // is a free slot in the hero's artifact bag. See the implementation for details.
    bool isUltimateArtifactAvailableToHero( const UltimateArtifact & art, const Heroes & hero );
}
