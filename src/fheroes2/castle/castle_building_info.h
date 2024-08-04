/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2024                                             *
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

#include <string>
#include <vector>

#include "castle.h"
#include "math_base.h"

enum class GameVersion : int;

namespace fheroes2
{
    Rect getCastleBuildingArea( const int race, const BuildingType buildingId );

    const char * getBuildingName( const int race, const BuildingType buildingId );

    const char * getBuildingDescription( const int race, const BuildingType buildingId );

    // Returns the upgraded building ID for the given one or the input building if no upgrade is available.
    BuildingType getUpgradeForBuilding( const int race, const BuildingType buildingId );

    BuildingType getBuildingRequirement( const int race, const BuildingType building );

    std::string getBuildingRequirementString( const int race, const BuildingType building );

    int getIndexBuildingSprite( const BuildingType build );

    std::vector<BuildingType> getBuildingDrawingPriorities( const int race, const GameVersion version );
}
