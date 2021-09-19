/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2021                                                    *
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

#include "castle_building_info.h"
#include "race.h"

#include <cassert>

namespace
{
    fheroes2::Rect getKnightBuildingArea( const building_t buildingId )
    {
        switch ( buildingId ) {
        case BUILD_THIEVESGUILD:
            return { 0, 130, 53, 63 };
        case BUILD_TAVERN:
            return { 345, 114, 83, 62 };
        case BUILD_SHIPYARD:
            return { 531, 211, 108, 45 };
        case BUILD_WELL:
            return { 188, 214, 39, 42 };
        case BUILD_STATUE:
            return { 478, 193, 46, 63 };
        case BUILD_MARKETPLACE:
            return { 219, 138, 120, 30 };
        case BUILD_WEL2:
            return { 241, 102, 141, 24 };
        case BUILD_MOAT:
            return { 0, 146, 311, 30 };
        case BUILD_SPEC:
            return { 0, 78, 251, 22 };
        case BUILD_CASTLE:
            return { 0, 49, 286, 116 };
        case BUILD_CAPTAIN:
            return { 293, 107, 59, 35 };
        case BUILD_MAGEGUILD1:
            return { 397, 150, 84, 32 };
        case BUILD_MAGEGUILD2:
            return { 397, 128, 84, 54 };
        case BUILD_MAGEGUILD3:
            return { 397, 105, 84, 77 };
        case BUILD_MAGEGUILD4:
            return { 397, 85, 84, 97 };
        case BUILD_MAGEGUILD5:
            return { 397, 55, 84, 127 };
        case BUILD_TENT:
            return { 69, 108, 67, 55 };
        case DWELLING_MONSTER1:
            return { 192, 163, 69, 52 };
        case DWELLING_MONSTER2:
        case DWELLING_UPGRADE2:
            return { 135, 149, 73, 32 };
        case DWELLING_MONSTER3:
        case DWELLING_UPGRADE3:
            return { 240, 166, 91, 66 };
        case DWELLING_MONSTER4:
        case DWELLING_UPGRADE4:
            return { 323, 174, 132, 73 };
        case DWELLING_MONSTER5:
        case DWELLING_UPGRADE5:
            return { 0, 176, 152, 79 };
        case DWELLING_MONSTER6:
        case DWELLING_UPGRADE6:
        case DWELLING_UPGRADE7:
            return { 445, 50, 194, 157 };
        case BUILD_LEFTTURRET:
            return { 7, 33, 0, 0 };
        case BUILD_RIGHTTURRET:
            return { 134, 37, 0, 0 };
        default:
            break;
        }

        // Did you add a new building but forgot to add its area?
        assert( 0 );
        return {};
    }

    fheroes2::Rect getBarbarianBuildingArea( const building_t buildingId )
    {
        switch ( buildingId ) {
        case BUILD_THIEVESGUILD:
            return { 466, 94, 87, 47 };
        case BUILD_TAVERN:
            return { 0, 161, 136, 90 };
        case BUILD_SHIPYARD:
            return { 505, 199, 134, 56 };
        case BUILD_WELL:
            return { 268, 189, 50, 58 };
        case BUILD_STATUE:
            return { 463, 154, 38, 81 };
        case BUILD_MARKETPLACE:
            return { 217, 166, 67, 43 };
        case BUILD_WEL2:
            return { 240, 106, 73, 34 };
        case BUILD_MOAT:
            return { 115, 138, 182, 42 };
        case BUILD_SPEC:
            return { 210, 80, 197, 61 };
        case BUILD_CASTLE:
            return { 0, 0, 214, 175 };
        case BUILD_CAPTAIN:
            return { 206, 99, 46, 42 };
        case BUILD_MAGEGUILD1:
            return { 348, 118, 50, 25 };
        case BUILD_MAGEGUILD2:
            return { 348, 94, 50, 49 };
        case BUILD_MAGEGUILD3:
            return { 348, 72, 50, 72 };
        case BUILD_MAGEGUILD4:
            return { 348, 48, 50, 96 };
        case BUILD_MAGEGUILD5:
            return { 348, 20, 50, 124 };
        case BUILD_TENT:
            return { 44, 109, 87, 52 };
        case DWELLING_MONSTER1:
            return { 290, 138, 58, 45 };
        case DWELLING_MONSTER2:
        case DWELLING_UPGRADE2:
            return { 145, 195, 76, 52 };
        case DWELLING_MONSTER3:
        case DWELLING_UPGRADE3:
            return { 557, 48, 83, 83 };
        case DWELLING_MONSTER4:
        case DWELLING_UPGRADE4:
            return { 496, 136, 138, 64 };
        case DWELLING_MONSTER5:
        case DWELLING_UPGRADE5:
            return { 318, 174, 131, 54 };
        case DWELLING_MONSTER6:
        case DWELLING_UPGRADE6:
        case DWELLING_UPGRADE7:
            return { 407, 0, 113, 106 };
        case BUILD_LEFTTURRET:
            return { 5, 50, 0, 0 };
        case BUILD_RIGHTTURRET:
            return { 118, 45, 0, 0 };
        default:
            break;
        }

        // Did you add a new building but forgot to add its area?
        assert( 0 );
        return {};
    }

    fheroes2::Rect getSorceressBuildingArea( const building_t buildingId )
    {
        switch ( buildingId ) {
        case BUILD_THIEVESGUILD:
            return { 423, 167, 87, 50 };
        case BUILD_TAVERN:
            return { 490, 141, 148, 91 };
        case BUILD_SHIPYARD:
            return { 0, 208, 178, 48 };
        case BUILD_WELL:
            return { 335, 205, 45, 29 };
        case BUILD_STATUE:
            return { 152, 163, 28, 65 };
        case BUILD_MARKETPLACE:
            return { 404, 122, 69, 45 };
        case BUILD_WEL2:
            return { 131, 185, 71, 53 };
        case BUILD_MOAT:
            return { 0, 171, 272, 23 };
        case BUILD_SPEC:
            return { 152, 0, 236, 84 };
        case BUILD_CASTLE:
            return { 0, 0, 201, 179 };
        case BUILD_CAPTAIN:
            return { 223, 122, 37, 52 };
        case BUILD_MAGEGUILD1:
            return { 280, 21, 60, 143 };
        case BUILD_MAGEGUILD2:
        case BUILD_MAGEGUILD3:
        case BUILD_MAGEGUILD4:
        case BUILD_MAGEGUILD5:
            return { 280, 0, 60, 164 };
        case BUILD_TENT:
            return { 104, 130, 59, 42 };
        case DWELLING_MONSTER1:
            return { 472, 59, 111, 92 };
        case DWELLING_MONSTER2:
        case DWELLING_UPGRADE2:
            return { 338, 146, 93, 61 };
        case DWELLING_MONSTER3:
        case DWELLING_UPGRADE3:
            return { 51, 164, 106, 40 };
        case DWELLING_MONSTER4:
        case DWELLING_UPGRADE4:
            return { 198, 178, 143, 71 };
        case DWELLING_MONSTER5:
        case DWELLING_UPGRADE5:
            return { 263, 226, 296, 30 };
        case DWELLING_MONSTER6:
        case DWELLING_UPGRADE6:
        case DWELLING_UPGRADE7:
            return { 179, 0, 84, 119 };
        case BUILD_LEFTTURRET:
            return { 98, 99, 0, 0 };
        case BUILD_RIGHTTURRET:
            return { 151, 98, 0, 0 };
        default:
            break;
        }

        // Did you add a new building but forgot to add its area?
        assert( 0 );
        return {};
    }

    fheroes2::Rect getWarlockBuildingArea( const building_t buildingId )
    {
        switch ( buildingId ) {
        case BUILD_THIEVESGUILD:
            return { 520, 103, 64, 54 };
        case BUILD_TAVERN:
            return { 476, 96, 82, 55 };
        case BUILD_SHIPYARD:
            return { 517, 200, 122, 56 };
        case BUILD_WELL:
            return { 342, 205, 67, 42 };
        case BUILD_STATUE:
            return { 478, 161, 37, 63 };
        case BUILD_MARKETPLACE:
            return { 386, 171, 71, 40 };
        case BUILD_WEL2:
            return { 60, 32, 63, 186 };
        case BUILD_MOAT:
            return { 211, 166, 301, 21 };
        case BUILD_SPEC:
            return { 0, 160, 59, 95 };
        case BUILD_CASTLE:
            return { 241, 18, 181, 150 };
        case BUILD_CAPTAIN:
            return { 418, 83, 53, 84 };
        case BUILD_MAGEGUILD1:
            return { 590, 135, 49, 35 };
        case BUILD_MAGEGUILD2:
            return { 590, 108, 49, 60 };
        case BUILD_MAGEGUILD3:
            return { 590, 77, 49, 90 };
        case BUILD_MAGEGUILD4:
            return { 590, 45, 49, 125 };
        case BUILD_MAGEGUILD5:
            return { 590, 14, 49, 155 };
        case BUILD_TENT:
            return { 298, 135, 72, 31 };
        case DWELLING_MONSTER1:
            return { 0, 64, 48, 50 };
        case DWELLING_MONSTER2:
        case DWELLING_UPGRADE2:
            return { 237, 168, 78, 79 };
        case DWELLING_MONSTER3:
        case DWELLING_UPGRADE3:
            return { 492, 50, 53, 39 };
        case DWELLING_MONSTER4:
        case DWELLING_UPGRADE4:
            return { 139, 163, 190, 83 };
        case DWELLING_MONSTER5:
        case DWELLING_UPGRADE5:
            return { 82, 92, 178, 68 };
        case DWELLING_MONSTER6:
        case DWELLING_UPGRADE6:
        case DWELLING_UPGRADE7:
            return { 92, 0, 64, 255 };
        case BUILD_LEFTTURRET:
            return { 311, 84, 0, 0 };
        case BUILD_RIGHTTURRET:
            return { 359, 83, 0, 0 };
        default:
            break;
        }

        // Did you add a new building but forgot to add its area?
        assert( 0 );
        return {};
    }

    fheroes2::Rect getWizardBuildingArea( const building_t buildingId )
    {
        switch ( buildingId ) {
        case BUILD_THIEVESGUILD:
            return { 505, 50, 51, 49 };
        case BUILD_TAVERN:
            return { 0, 149, 118, 76 };
        case BUILD_SHIPYARD:
            return { 0, 206, 206, 49 };
        case BUILD_WELL:
            return { 249, 139, 28, 33 };
        case BUILD_STATUE:
            return { 464, 45, 24, 72 };
        case BUILD_MARKETPLACE:
            return { 255, 163, 108, 53 };
        case BUILD_WEL2:
            return { 237, 208, 137, 39 };
        case BUILD_MOAT:
            return { 0, 90, 223, 14 };
        case BUILD_SPEC:
            return { 297, 95, 109, 78 };
        case BUILD_CASTLE:
            return { 0, 0, 200, 99 };
        case BUILD_CAPTAIN:
            return { 210, 52, 28, 35 };
        case BUILD_MAGEGUILD1:
            return { 585, 73, 54, 48 };
        case BUILD_MAGEGUILD2:
            return { 585, 69, 54, 50 };
        case BUILD_MAGEGUILD3:
            return { 585, 44, 54, 78 };
        case BUILD_MAGEGUILD4:
            return { 585, 20, 54, 102 };
        case BUILD_MAGEGUILD5:
            return { 585, 0, 54, 122 };
        case BUILD_TENT:
            return { 58, 60, 49, 42 };
        case DWELLING_MONSTER1:
            return { 467, 181, 38, 30 };
        case DWELLING_MONSTER2:
        case DWELLING_UPGRADE2:
            return { 231, 68, 192, 36 };
        case DWELLING_MONSTER3:
        case DWELLING_UPGRADE3:
            return { 152, 130, 96, 60 };
        case DWELLING_MONSTER4:
        case DWELLING_UPGRADE4:
            return { 593, 184, 46, 31 };
        case DWELLING_MONSTER5:
        case DWELLING_UPGRADE5:
            return { 411, 0, 49, 167 };
        case DWELLING_MONSTER6:
        case DWELLING_UPGRADE6:
        case DWELLING_UPGRADE7:
            return { 160, 0, 178, 67 };
        case BUILD_LEFTTURRET:
            return { 30, 17, 0, 0 };
        case BUILD_RIGHTTURRET:
            return { 127, 17, 0, 0 };
        default:
            break;
        }

        // Did you add a new building but forgot to add its area?
        assert( 0 );
        return {};
    }

    fheroes2::Rect getNecromancerBuildingArea( const building_t buildingId )
    {
        switch ( buildingId ) {
        case BUILD_THIEVESGUILD:
            return { 275, 124, 62, 77 };
        case BUILD_SHRINE:
            return { 455, 39, 51, 103 };
        case BUILD_SHIPYARD:
            return { 500, 220, 139, 36 };
        case BUILD_WELL:
            return { 215, 213, 29, 39 };
        case BUILD_STATUE:
            return { 365, 154, 41, 93 };
        case BUILD_MARKETPLACE:
            return { 412, 193, 98, 61 };
        case BUILD_WEL2:
            return { 263, 181, 90, 65 };
        case BUILD_MOAT:
            return { 258, 171, 193, 19 };
        case BUILD_SPEC:
            return { 0, 0, 640, 63 };
        case BUILD_CASTLE:
            return { 289, 10, 134, 164 };
        case BUILD_CAPTAIN:
            return { 441, 77, 22, 99 };
        case BUILD_MAGEGUILD1:
            return { 565, 131, 73, 74 };
        case BUILD_MAGEGUILD2:
            return { 568, 102, 62, 104 };
        case BUILD_MAGEGUILD3:
            return { 570, 79, 56, 130 };
        case BUILD_MAGEGUILD4:
            return { 570, 61, 60, 146 };
        case BUILD_MAGEGUILD5:
            return { 570, 45, 61, 162 };
        case BUILD_TENT:
            return { 333, 115, 47, 70 };
        case DWELLING_MONSTER1:
            return { 396, 177, 71, 35 };
        case DWELLING_MONSTER2:
        case DWELLING_UPGRADE2:
            return { 110, 174, 141, 45 };
        case DWELLING_MONSTER3:
        case DWELLING_UPGRADE3:
            return { 0, 28, 241, 142 };
        case DWELLING_MONSTER4:
        case DWELLING_UPGRADE4:
            return { 0, 107, 140, 129 };
        case DWELLING_MONSTER5:
        case DWELLING_UPGRADE5:
            return { 221, 127, 66, 84 };
        case DWELLING_MONSTER6:
        case DWELLING_UPGRADE6:
        case DWELLING_UPGRADE7:
            return { 464, 72, 105, 124 };
        case BUILD_LEFTTURRET:
            return { 330, 47, 0, 0 };
        case BUILD_RIGHTTURRET:
            return { 360, 46, 0, 0 };
        default:
            break;
        }

        // Did you add a new building but forgot to add its area?
        assert( 0 );
        return {};
    }
}

namespace fheroes2
{
    Rect getCastleBuildingArea( const int race, const building_t buildingId )
    {
        if ( buildingId == BUILD_NOTHING ) {
            // Special case which we should ignore.
            return {};
        }

        switch ( race ) {
        case Race::KNGT:
            return getKnightBuildingArea( buildingId );
        case Race::BARB:
            return getBarbarianBuildingArea( buildingId );
        case Race::SORC:
            return getSorceressBuildingArea( buildingId );
        case Race::WRLK:
            return getWarlockBuildingArea( buildingId );
        case Race::WZRD:
            return getWizardBuildingArea( buildingId );
        case Race::NECR:
            return getNecromancerBuildingArea( buildingId );
        default:
            break;
        }

        // Did you add a new castle?
        assert( 0 );

        return {};
    }
}
