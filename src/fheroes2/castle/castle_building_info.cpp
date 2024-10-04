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

#include "castle_building_info.h"

#include <cassert>
#include <cstdint>

#include "maps_fileinfo.h"
#include "race.h"
#include "translations.h"

namespace
{
    fheroes2::Rect getKnightBuildingArea( const BuildingType buildingId )
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

    fheroes2::Rect getBarbarianBuildingArea( const BuildingType buildingId )
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

    fheroes2::Rect getSorceressBuildingArea( const BuildingType buildingId )
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

    fheroes2::Rect getWarlockBuildingArea( const BuildingType buildingId )
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

    fheroes2::Rect getWizardBuildingArea( const BuildingType buildingId )
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

    fheroes2::Rect getNecromancerBuildingArea( const BuildingType buildingId )
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

    const char * getKnightBuildingName( const BuildingType buildingId )
    {
        switch ( buildingId ) {
        case BUILD_SPEC:
            return _( "Fortifications" );
        case BUILD_WEL2:
            return _( "Farm" );
        case DWELLING_MONSTER1:
            return _( "Thatched Hut" );
        case DWELLING_MONSTER2:
            return _( "Archery Range" );
        case DWELLING_UPGRADE2:
            return _( "Upg. Archery Range" );
        case DWELLING_MONSTER3:
            return _( "Blacksmith" );
        case DWELLING_UPGRADE3:
            return _( "Upg. Blacksmith" );
        case DWELLING_MONSTER4:
            return _( "Armory" );
        case DWELLING_UPGRADE4:
            return _( "Upg. Armory" );
        case DWELLING_MONSTER5:
            return _( "Jousting Arena" );
        case DWELLING_UPGRADE5:
            return _( "Upg. Jousting Arena" );
        case DWELLING_MONSTER6:
            return _( "Cathedral" );
        case DWELLING_UPGRADE6:
            return _( "Upg. Cathedral" );
        default:
            break;
        }

        // Did you add a new building?
        assert( 0 );
        return nullptr;
    }

    const char * getBarbarianBuildingName( const BuildingType buildingId )
    {
        switch ( buildingId ) {
        case BUILD_SPEC:
            return _( "Coliseum" );
        case BUILD_WEL2:
            return _( "Garbage Heap" );
        case DWELLING_MONSTER1:
            return _( "Hut" );
        case DWELLING_MONSTER2:
            return _( "Stick Hut" );
        case DWELLING_UPGRADE2:
            return _( "Upg. Stick Hut" );
        case DWELLING_MONSTER3:
            return _( "Den" );
        case DWELLING_MONSTER4:
            return _( "Adobe" );
        case DWELLING_UPGRADE4:
            return _( "Upg. Adobe" );
        case DWELLING_MONSTER5:
            return _( "Bridge" );
        case DWELLING_UPGRADE5:
            return _( "Upg. Bridge" );
        case DWELLING_MONSTER6:
            return _( "Pyramid" );
        default:
            break;
        }

        // Did you add a new building?
        assert( 0 );
        return nullptr;
    }

    const char * getSorceressBuildingName( const BuildingType buildingId )
    {
        switch ( buildingId ) {
        case BUILD_SPEC:
            return _( "Rainbow" );
        case BUILD_WEL2:
            return _( "Crystal Garden" );
        case DWELLING_MONSTER1:
            return _( "Treehouse" );
        case DWELLING_MONSTER2:
            return _( "Cottage" );
        case DWELLING_UPGRADE2:
            return _( "Upg. Cottage" );
        case DWELLING_MONSTER3:
            return _( "Archery Range" );
        case DWELLING_UPGRADE3:
            return _( "Upg. Archery Range" );
        case DWELLING_MONSTER4:
            return _( "Stonehenge" );
        case DWELLING_UPGRADE4:
            return _( "Upg. Stonehenge" );
        case DWELLING_MONSTER5:
            return _( "Fenced Meadow" );
        case DWELLING_MONSTER6:
            return _( "sorceress|Red Tower" );
        default:
            break;
        }

        // Did you add a new building?
        assert( 0 );
        return nullptr;
    }

    const char * getWarlockBuildingName( const BuildingType buildingId )
    {
        switch ( buildingId ) {
        case BUILD_SPEC:
            return _( "Dungeon" );
        case BUILD_WEL2:
            return _( "Waterfall" );
        case DWELLING_MONSTER1:
            return _( "Cave" );
        case DWELLING_MONSTER2:
            return _( "Crypt" );
        case DWELLING_MONSTER3:
            return _( "Nest" );
        case DWELLING_MONSTER4:
            return _( "Maze" );
        case DWELLING_UPGRADE4:
            return _( "Upg. Maze" );
        case DWELLING_MONSTER5:
            return _( "Swamp" );
        case DWELLING_MONSTER6:
            return _( "Green Tower" );
        case DWELLING_UPGRADE6:
            return _( "warlock|Red Tower" );
        case DWELLING_UPGRADE7:
            return _( "Black Tower" );
        default:
            break;
        }

        // Did you add a new building?
        assert( 0 );
        return nullptr;
    }

    const char * getWizardBuildingName( const BuildingType buildingId )
    {
        switch ( buildingId ) {
        case BUILD_SPEC:
            return _( "Library" );
        case BUILD_WEL2:
            return _( "Orchard" );
        case DWELLING_MONSTER1:
            return _( "Habitat" );
        case DWELLING_MONSTER2:
            return _( "Pen" );
        case DWELLING_MONSTER3:
            return _( "Foundry" );
        case DWELLING_UPGRADE3:
            return _( "Upg. Foundry" );
        case DWELLING_MONSTER4:
            return _( "Cliff Nest" );
        case DWELLING_MONSTER5:
            return _( "Ivory Tower" );
        case DWELLING_UPGRADE5:
            return _( "Upg. Ivory Tower" );
        case DWELLING_MONSTER6:
            return _( "Cloud Castle" );
        case DWELLING_UPGRADE6:
            return _( "Upg. Cloud Castle" );
        default:
            break;
        }

        // Did you add a new building?
        assert( 0 );
        return nullptr;
    }

    const char * getNecromancerBuildingName( const BuildingType buildingId )
    {
        switch ( buildingId ) {
        case BUILD_SPEC:
            return _( "Storm" );
        case BUILD_WEL2:
            return _( "Skull Pile" );
        case DWELLING_MONSTER1:
            return _( "Excavation" );
        case DWELLING_MONSTER2:
            return _( "Graveyard" );
        case DWELLING_UPGRADE2:
            return _( "Upg. Graveyard" );
        case DWELLING_MONSTER3:
            return _( "Pyramid" );
        case DWELLING_UPGRADE3:
            return _( "Upg. Pyramid" );
        case DWELLING_MONSTER4:
            return _( "Mansion" );
        case DWELLING_UPGRADE4:
            return _( "Upg. Mansion" );
        case DWELLING_MONSTER5:
            return _( "Mausoleum" );
        case DWELLING_UPGRADE5:
            return _( "Upg. Mausoleum" );
        case DWELLING_MONSTER6:
            return _( "Laboratory" );
        case BUILD_SHRINE:
            return _( "Shrine" );
        default:
            break;
        }

        // Did you add a new building?
        assert( 0 );
        return nullptr;
    }

    const char * getRandomBuildingName( const BuildingType buildingId )
    {
        switch ( buildingId ) {
        case BUILD_SPEC:
            return _( "Special" );
        case BUILD_WEL2:
            return _( "Horde Building" );
        case DWELLING_MONSTER1:
            return _( "Dwelling 1" );
        case DWELLING_MONSTER2:
            return _( "Dwelling 2" );
        case DWELLING_UPGRADE2:
            return _( "Upg. Dwelling 2" );
        case DWELLING_MONSTER3:
            return _( "Dwelling 3" );
        case DWELLING_UPGRADE3:
            return _( "Upg. Dwelling 3" );
        case DWELLING_MONSTER4:
            return _( "Dwelling 4" );
        case DWELLING_UPGRADE4:
            return _( "Upg. Dwelling 4" );
        case DWELLING_MONSTER5:
            return _( "Dwelling 5" );
        case DWELLING_UPGRADE5:
            return _( "Upg. Dwelling 5" );
        case DWELLING_MONSTER6:
            return _( "Dwelling 6" );
        case DWELLING_UPGRADE6:
            return _( "Upg. Dwelling 6" );
        case DWELLING_UPGRADE7:
            return _( "2x Upg. Dwelling 6" );
        default:
            break;
        }

        // Did you add a new building?
        assert( 0 );
        return nullptr;
    }

    const char * getKnightBuildingDescription( const BuildingType buildingId )
    {
        switch ( buildingId ) {
        case BUILD_SPEC:
            return _( "The Fortifications increase the toughness of the walls, increasing the number of turns it takes to knock them down." );
        case BUILD_WEL2:
            return _( "The Farm increases production of Peasants by %{count} per week." );
        default:
            break;
        }

        // Did you add a new building?
        assert( 0 );
        return nullptr;
    }

    const char * getBarbarianBuildingDescription( const BuildingType buildingId )
    {
        switch ( buildingId ) {
        case BUILD_SPEC:
            return _( "The Coliseum provides inspiring spectacles to defending troops, raising their morale by two during combat." );
        case BUILD_WEL2:
            return _( "The Garbage Heap increases production of Goblins by %{count} per week." );
        default:
            break;
        }

        // Did you add a new building?
        assert( 0 );
        return nullptr;
    }

    const char * getSorceressBuildingDescription( const BuildingType buildingId )
    {
        switch ( buildingId ) {
        case BUILD_SPEC:
            return _( "The Rainbow increases the luck of the defending units by two." );
        case BUILD_WEL2:
            return _( "The Crystal Garden increases production of Sprites by %{count} per week." );
        default:
            break;
        }

        // Did you add a new building?
        assert( 0 );
        return nullptr;
    }

    const char * getWarlockBuildingDescription( const BuildingType buildingId )
    {
        switch ( buildingId ) {
        case BUILD_SPEC:
            return _( "The Dungeon increases the income of the town by %{count} gold per day." );
        case BUILD_WEL2:
            return _( "The Waterfall increases production of Centaurs by %{count} per week." );
        default:
            break;
        }

        // Did you add a new building?
        assert( 0 );
        return nullptr;
    }

    const char * getWizardBuildingDescription( const BuildingType buildingId )
    {
        switch ( buildingId ) {
        case BUILD_SPEC:
            return _( "The Library increases the number of spells in the Guild by one for each level of the guild." );
        case BUILD_WEL2:
            return _( "The Orchard increases production of Halflings by %{count} per week." );
        default:
            break;
        }

        // Did you add a new building?
        assert( 0 );
        return nullptr;
    }

    const char * getNecromancerBuildingDescription( const BuildingType buildingId )
    {
        switch ( buildingId ) {
        case BUILD_SPEC:
            return _( "The Storm adds +2 to the power of spells of a defending spell caster." );
        case BUILD_WEL2:
            return _( "The Skull Pile increases production of Skeletons by %{count} per week." );
        default:
            break;
        }

        // Did you add a new building?
        assert( 0 );
        return nullptr;
    }

    const char * getRandomBuildingDescription( const BuildingType buildingId )
    {
        switch ( buildingId ) {
        case BUILD_SPEC:
            return _( "The Special building gives a specific bonus to the chosen castle type." );
        case BUILD_WEL2:
            return _( "The Horde Building increases the growth rate of the level 1 creatures by 8 per week." );
        default:
            break;
        }

        // Did you add a new building?
        assert( 0 );
        return nullptr;
    }
}

namespace fheroes2
{
    Rect getCastleBuildingArea( const int race, const BuildingType buildingId )
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

    const char * getBuildingName( const int race, const BuildingType buildingId )
    {
        if ( buildingId == BUILD_NOTHING ) {
            // Special case which we should ignore.
            return "";
        }

        // Check common buildings as they have the same name.
        switch ( buildingId ) {
        case BUILD_THIEVESGUILD:
            return _( "Thieves' Guild" );
        case BUILD_TAVERN:
            return _( "Tavern" );
        case BUILD_SHIPYARD:
            return _( "Shipyard" );
        case BUILD_WELL:
            return _( "Well" );
        case BUILD_STATUE:
            return _( "Statue" );
        case BUILD_LEFTTURRET:
            return _( "Left Turret" );
        case BUILD_RIGHTTURRET:
            return _( "Right Turret" );
        case BUILD_MARKETPLACE:
            return _( "Marketplace" );
        case BUILD_MOAT:
            return _( "Moat" );
        case BUILD_CASTLE:
            return _( "Castle" );
        case BUILD_TENT:
            return _( "Tent" );
        case BUILD_CAPTAIN:
            return _( "Captain's Quarters" );
        case BUILD_MAGEGUILD1:
            return _( "Mage Guild, Level 1" );
        case BUILD_MAGEGUILD2:
            return _( "Mage Guild, Level 2" );
        case BUILD_MAGEGUILD3:
            return _( "Mage Guild, Level 3" );
        case BUILD_MAGEGUILD4:
            return _( "Mage Guild, Level 4" );
        case BUILD_MAGEGUILD5:
            return _( "Mage Guild, Level 5" );
        default:
            break;
        }

        switch ( race ) {
        case Race::KNGT:
            return getKnightBuildingName( buildingId );
        case Race::BARB:
            return getBarbarianBuildingName( buildingId );
        case Race::SORC:
            return getSorceressBuildingName( buildingId );
        case Race::WRLK:
            return getWarlockBuildingName( buildingId );
        case Race::WZRD:
            return getWizardBuildingName( buildingId );
        case Race::NECR:
            return getNecromancerBuildingName( buildingId );
        case Race::RAND:
            return getRandomBuildingName( buildingId );
        default:
            break;
        }

        // Did you add a new castle?
        assert( 0 );

        return nullptr;
    }

    const char * getBuildingDescription( const int race, const BuildingType buildingId )
    {
        if ( buildingId == BUILD_NOTHING ) {
            // Special case which we should ignore.
            return "";
        }

        switch ( buildingId ) {
        case BUILD_SHRINE:
            return _( "The Shrine increases the necromancy skill of all your necromancers by 10 percent." );
        case BUILD_THIEVESGUILD:
            return _(
                "The Thieves' Guild provides information on enemy players. Thieves' Guilds can also provide scouting information on enemy towns. Additional Guilds provide more information." );
        case BUILD_TAVERN:
            return _( "The Tavern increases morale for troops defending the castle." );
        case BUILD_SHIPYARD:
            return _( "The Shipyard allows ships to be built." );
        case BUILD_WELL:
            return _( "The Well increases the growth rate of all dwellings by %{count} creatures per week." );
        case BUILD_STATUE:
            return _( "The Statue increases the town's income by %{count} gold per day." );
        case BUILD_LEFTTURRET:
            return _( "The Left Turret provides extra firepower during castle combat." );
        case BUILD_RIGHTTURRET:
            return _( "The Right Turret provides extra firepower during castle combat." );
        case BUILD_MARKETPLACE:
            return _( "The Marketplace can be used to convert one type of resource into another. The more marketplaces you control, the better the exchange rate." );
        case BUILD_MOAT:
            return _( "The Moat slows attacking units. Any unit entering the moat must end its turn there and becomes more vulnerable to attack." );
        case BUILD_CASTLE:
            return _( "The Castle improves the town's defense and increases its income to %{count} gold per day." );
        case BUILD_TENT:
            return _( "The Tent provides workers to build a castle, provided the materials and the gold are available." );
        case BUILD_CAPTAIN:
            return _( "The Captain's Quarters provides a captain to assist in the castle's defense when no hero is present." );
        case BUILD_MAGEGUILD1:
        case BUILD_MAGEGUILD2:
        case BUILD_MAGEGUILD3:
        case BUILD_MAGEGUILD4:
        case BUILD_MAGEGUILD5:
            return _( "The Mage Guild allows heroes to learn spells and replenish their spell points." );
        default:
            break;
        }

        switch ( race ) {
        case Race::KNGT:
            return getKnightBuildingDescription( buildingId );
        case Race::BARB:
            return getBarbarianBuildingDescription( buildingId );
        case Race::SORC:
            return getSorceressBuildingDescription( buildingId );
        case Race::WRLK:
            return getWarlockBuildingDescription( buildingId );
        case Race::WZRD:
            return getWizardBuildingDescription( buildingId );
        case Race::NECR:
            return getNecromancerBuildingDescription( buildingId );
        case Race::RAND:
            return getRandomBuildingDescription( buildingId );
        default:
            break;
        }

        // Did you add a new castle?
        assert( 0 );

        return nullptr;
    }

    BuildingType getUpgradeForBuilding( const int race, const BuildingType buildingId )
    {
        switch ( buildingId ) {
        case BUILD_TENT:
            return BUILD_CASTLE;
        case BUILD_MAGEGUILD1:
            return BUILD_MAGEGUILD2;
        case BUILD_MAGEGUILD2:
            return BUILD_MAGEGUILD3;
        case BUILD_MAGEGUILD3:
            return BUILD_MAGEGUILD4;
        case BUILD_MAGEGUILD4:
            return BUILD_MAGEGUILD5;
        default:
            break;
        }

        if ( race == Race::BARB ) {
            switch ( buildingId ) {
            case DWELLING_MONSTER2:
                return DWELLING_UPGRADE2;
            case DWELLING_MONSTER4:
                return DWELLING_UPGRADE4;
            case DWELLING_MONSTER5:
                return DWELLING_UPGRADE5;
            default:
                break;
            }
        }
        else if ( race == Race::KNGT ) {
            switch ( buildingId ) {
            case DWELLING_MONSTER2:
                return DWELLING_UPGRADE2;
            case DWELLING_MONSTER3:
                return DWELLING_UPGRADE3;
            case DWELLING_MONSTER4:
                return DWELLING_UPGRADE4;
            case DWELLING_MONSTER5:
                return DWELLING_UPGRADE5;
            case DWELLING_MONSTER6:
                return DWELLING_UPGRADE6;
            default:
                break;
            }
        }
        else if ( race == Race::NECR ) {
            switch ( buildingId ) {
            case DWELLING_MONSTER2:
                return DWELLING_UPGRADE2;
            case DWELLING_MONSTER3:
                return DWELLING_UPGRADE3;
            case DWELLING_MONSTER4:
                return DWELLING_UPGRADE4;
            case DWELLING_MONSTER5:
                return DWELLING_UPGRADE5;
            default:
                break;
            }
        }
        else if ( race == Race::SORC ) {
            switch ( buildingId ) {
            case DWELLING_MONSTER2:
                return DWELLING_UPGRADE2;
            case DWELLING_MONSTER3:
                return DWELLING_UPGRADE3;
            case DWELLING_MONSTER4:
                return DWELLING_UPGRADE4;
            default:
                break;
            }
        }
        else if ( race == Race::WRLK ) {
            switch ( buildingId ) {
            case DWELLING_MONSTER4:
                return DWELLING_UPGRADE4;
            case DWELLING_MONSTER6:
                return DWELLING_UPGRADE6;
            case DWELLING_UPGRADE6:
                return DWELLING_UPGRADE7;
            default:
                break;
            }
        }
        else if ( race == Race::WZRD ) {
            switch ( buildingId ) {
            case DWELLING_MONSTER3:
                return DWELLING_UPGRADE3;
            case DWELLING_MONSTER5:
                return DWELLING_UPGRADE5;
            case DWELLING_MONSTER6:
                return DWELLING_UPGRADE6;
            default:
                break;
            }
        }
        else if ( race == Race::RAND ) {
            switch ( buildingId ) {
            case DWELLING_MONSTER2:
                return DWELLING_UPGRADE2;
            case DWELLING_MONSTER3:
                return DWELLING_UPGRADE3;
            case DWELLING_MONSTER4:
                return DWELLING_UPGRADE4;
            case DWELLING_MONSTER5:
                return DWELLING_UPGRADE5;
            case DWELLING_MONSTER6:
                return DWELLING_UPGRADE6;
            case DWELLING_UPGRADE6:
                return DWELLING_UPGRADE7;
            default:
                break;
            }
        }

        return buildingId;
    }

    BuildingType getBuildingRequirement( const int race, const BuildingType building )
    {
        uint32_t requirement = 0;

        switch ( building ) {
        case BUILD_SPEC:
            if ( race == Race::WZRD ) {
                requirement |= BUILD_MAGEGUILD1;
            }
            break;

        case DWELLING_MONSTER2:
            switch ( race ) {
            case Race::KNGT:
            case Race::BARB:
            case Race::WZRD:
            case Race::WRLK:
            case Race::NECR:
                requirement |= DWELLING_MONSTER1;
                break;

            case Race::SORC:
                requirement |= DWELLING_MONSTER1;
                requirement |= BUILD_TAVERN;
                break;

            default:
                break;
            }
            break;

        case DWELLING_MONSTER3:
            switch ( race ) {
            case Race::KNGT:
                requirement |= DWELLING_MONSTER1;
                requirement |= BUILD_WELL;
                break;

            case Race::BARB:
            case Race::SORC:
            case Race::WZRD:
            case Race::WRLK:
            case Race::NECR:
                requirement |= DWELLING_MONSTER1;
                break;

            default:
                break;
            }
            break;

        case DWELLING_MONSTER4:
            switch ( race ) {
            case Race::KNGT:
                requirement |= DWELLING_MONSTER1;
                requirement |= BUILD_TAVERN;
                break;

            case Race::BARB:
                requirement |= DWELLING_MONSTER1;
                break;

            case Race::SORC:
                requirement |= DWELLING_MONSTER3;
                requirement |= BUILD_MAGEGUILD1;
                break;

            case Race::WZRD:
            case Race::WRLK:
                requirement |= DWELLING_MONSTER2;
                break;

            case Race::NECR:
                requirement |= DWELLING_MONSTER3;
                requirement |= BUILD_THIEVESGUILD;
                break;

            default:
                break;
            }
            break;

        case DWELLING_MONSTER5:
            switch ( race ) {
            case Race::KNGT:
            case Race::BARB:
                requirement |= DWELLING_MONSTER2;
                requirement |= DWELLING_MONSTER3;
                requirement |= DWELLING_MONSTER4;
                break;

            case Race::SORC:
                requirement |= DWELLING_MONSTER4;
                break;

            case Race::WRLK:
                requirement |= DWELLING_MONSTER3;
                break;

            case Race::WZRD:
                requirement |= DWELLING_MONSTER3;
                requirement |= BUILD_MAGEGUILD1;
                break;

            case Race::NECR:
                requirement |= DWELLING_MONSTER2;
                requirement |= BUILD_MAGEGUILD1;
                break;

            default:
                break;
            }
            break;

        case DWELLING_MONSTER6:
            switch ( race ) {
            case Race::KNGT:
                requirement |= DWELLING_MONSTER2;
                requirement |= DWELLING_MONSTER3;
                requirement |= DWELLING_MONSTER4;
                break;

            case Race::BARB:
            case Race::SORC:
            case Race::NECR:
                requirement |= DWELLING_MONSTER5;
                break;

            case Race::WRLK:
            case Race::WZRD:
                requirement |= DWELLING_MONSTER4;
                requirement |= DWELLING_MONSTER5;
                break;

            default:
                break;
            }
            break;

        case DWELLING_UPGRADE2:
            switch ( race ) {
            case Race::KNGT:
            case Race::BARB:
                requirement |= DWELLING_MONSTER2;
                requirement |= DWELLING_MONSTER3;
                requirement |= DWELLING_MONSTER4;
                break;

            case Race::SORC:
                requirement |= DWELLING_MONSTER2;
                requirement |= BUILD_WELL;
                break;

            case Race::NECR:
                requirement |= DWELLING_MONSTER2;
                break;

            default:
                break;
            }
            break;

        case DWELLING_UPGRADE3:
            switch ( race ) {
            case Race::KNGT:
                requirement |= DWELLING_MONSTER2;
                requirement |= DWELLING_MONSTER3;
                requirement |= DWELLING_MONSTER4;
                break;

            case Race::SORC:
                requirement |= DWELLING_MONSTER3;
                requirement |= DWELLING_MONSTER4;
                break;

            case Race::WZRD:
                requirement |= DWELLING_MONSTER3;
                requirement |= BUILD_WELL;
                break;

            case Race::NECR:
                requirement |= DWELLING_MONSTER3;
                break;

            default:
                break;
            }
            break;

        case DWELLING_UPGRADE4:
            switch ( race ) {
            case Race::KNGT:
            case Race::BARB:
                requirement |= DWELLING_MONSTER2;
                requirement |= DWELLING_MONSTER3;
                requirement |= DWELLING_MONSTER4;
                break;

            case Race::SORC:
            case Race::WRLK:
            case Race::NECR:
                requirement |= DWELLING_MONSTER4;
                break;

            default:
                break;
            }
            break;

        case DWELLING_UPGRADE5:
            switch ( race ) {
            case Race::KNGT:
            case Race::BARB:
                requirement |= DWELLING_MONSTER5;
                break;

            case Race::WZRD:
                requirement |= BUILD_SPEC;
                requirement |= DWELLING_MONSTER5;
                break;

            case Race::NECR:
                requirement |= BUILD_MAGEGUILD2;
                requirement |= DWELLING_MONSTER5;
                break;

            default:
                break;
            }
            break;

        case DWELLING_UPGRADE6:
            switch ( race ) {
            case Race::KNGT:
            case Race::WRLK:
            case Race::WZRD:
                requirement |= DWELLING_MONSTER6;
                break;

            default:
                break;
            }
            break;
        case DWELLING_UPGRADE7:
            if ( race == Race::WRLK )
                requirement |= DWELLING_UPGRADE6;
            break;

        default:
            break;
        }

        return static_cast<BuildingType>( requirement );
    }

    std::string getBuildingRequirementString( const int race, const BuildingType building )
    {
        // prepare requirement build string
        std::string requirement;
        const uint32_t requirementBuildingIds = fheroes2::getBuildingRequirement( race, building );
        const char sep = '\n';

        for ( uint32_t itr = 0x00000001; itr; itr <<= 1 )
            if ( requirementBuildingIds & itr ) {
                requirement.append( Castle::GetStringBuilding( itr, race ) );
                requirement += sep;
            }

        // Remove the last separator.
        if ( !requirement.empty() ) {
            requirement.pop_back();
        }

        return requirement;
    }

    int getIndexBuildingSprite( const BuildingType build )
    {
        switch ( build ) {
        case DWELLING_MONSTER1:
            return 19;
        case DWELLING_MONSTER2:
            return 20;
        case DWELLING_MONSTER3:
            return 21;
        case DWELLING_MONSTER4:
            return 22;
        case DWELLING_MONSTER5:
            return 23;
        case DWELLING_MONSTER6:
            return 24;
        case DWELLING_UPGRADE2:
            return 25;
        case DWELLING_UPGRADE3:
            return 26;
        case DWELLING_UPGRADE4:
            return 27;
        case DWELLING_UPGRADE5:
            return 28;
        case DWELLING_UPGRADE6:
            return 29;
        case DWELLING_UPGRADE7:
            return 30;
        case BUILD_MAGEGUILD1:
        case BUILD_MAGEGUILD2:
        case BUILD_MAGEGUILD3:
        case BUILD_MAGEGUILD4:
        case BUILD_MAGEGUILD5:
            return 0;
        case BUILD_THIEVESGUILD:
            return 1;
        case BUILD_SHRINE:
        case BUILD_TAVERN:
            return 2;
        case BUILD_SHIPYARD:
            return 3;
        case BUILD_WELL:
            return 4;
        case BUILD_CASTLE:
            return 6;
        case BUILD_STATUE:
            return 7;
        case BUILD_LEFTTURRET:
            return 8;
        case BUILD_RIGHTTURRET:
            return 9;
        case BUILD_MARKETPLACE:
            return 10;
        case BUILD_WEL2:
            return 11;
        case BUILD_MOAT:
            return 12;
        case BUILD_SPEC:
            return 13;
        case BUILD_CAPTAIN:
            return 15;
        default:
            break;
        }

        return 0;
    }

    std::vector<BuildingType> getBuildingDrawingPriorities( const int race, const GameVersion version )
    {
        std::vector<BuildingType> priorities;
        priorities.reserve( 32 );

        switch ( race ) {
        case Race::KNGT:
            priorities.emplace_back( BUILD_TENT );
            priorities.emplace_back( BUILD_WEL2 );
            priorities.emplace_back( BUILD_CASTLE );
            priorities.emplace_back( BUILD_SPEC );
            priorities.emplace_back( BUILD_CAPTAIN );
            priorities.emplace_back( BUILD_LEFTTURRET );
            priorities.emplace_back( BUILD_RIGHTTURRET );
            priorities.emplace_back( BUILD_MOAT );
            priorities.emplace_back( BUILD_MARKETPLACE );
            priorities.emplace_back( DWELLING_MONSTER2 );
            priorities.emplace_back( DWELLING_UPGRADE2 );
            priorities.emplace_back( BUILD_THIEVESGUILD );
            priorities.emplace_back( BUILD_TAVERN );
            priorities.emplace_back( BUILD_MAGEGUILD1 );
            priorities.emplace_back( BUILD_MAGEGUILD2 );
            priorities.emplace_back( BUILD_MAGEGUILD3 );
            priorities.emplace_back( BUILD_MAGEGUILD4 );
            priorities.emplace_back( BUILD_MAGEGUILD5 );
            priorities.emplace_back( DWELLING_MONSTER5 );
            priorities.emplace_back( DWELLING_UPGRADE5 );
            priorities.emplace_back( DWELLING_MONSTER6 );
            priorities.emplace_back( DWELLING_UPGRADE6 );
            priorities.emplace_back( DWELLING_MONSTER1 );
            priorities.emplace_back( DWELLING_MONSTER3 );
            priorities.emplace_back( DWELLING_UPGRADE3 );
            priorities.emplace_back( DWELLING_MONSTER4 );
            priorities.emplace_back( DWELLING_UPGRADE4 );
            priorities.emplace_back( BUILD_WELL );
            priorities.emplace_back( BUILD_SHIPYARD );
            priorities.emplace_back( BUILD_STATUE );
            break;
        case Race::BARB:
            priorities.emplace_back( BUILD_SPEC );
            priorities.emplace_back( BUILD_WEL2 );
            priorities.emplace_back( DWELLING_MONSTER6 );
            priorities.emplace_back( BUILD_MAGEGUILD1 );
            priorities.emplace_back( BUILD_MAGEGUILD2 );
            priorities.emplace_back( BUILD_MAGEGUILD3 );
            priorities.emplace_back( BUILD_MAGEGUILD4 );
            priorities.emplace_back( BUILD_MAGEGUILD5 );
            priorities.emplace_back( BUILD_CAPTAIN );
            priorities.emplace_back( BUILD_TENT );
            priorities.emplace_back( BUILD_CASTLE );
            priorities.emplace_back( BUILD_LEFTTURRET );
            priorities.emplace_back( BUILD_RIGHTTURRET );
            priorities.emplace_back( BUILD_MOAT );
            priorities.emplace_back( DWELLING_MONSTER3 );
            priorities.emplace_back( BUILD_THIEVESGUILD );
            priorities.emplace_back( DWELLING_MONSTER1 );
            priorities.emplace_back( BUILD_MARKETPLACE );
            priorities.emplace_back( DWELLING_MONSTER2 );
            priorities.emplace_back( DWELLING_UPGRADE2 );
            priorities.emplace_back( BUILD_TAVERN );
            priorities.emplace_back( DWELLING_MONSTER4 );
            priorities.emplace_back( DWELLING_UPGRADE4 );
            priorities.emplace_back( DWELLING_MONSTER5 );
            priorities.emplace_back( DWELLING_UPGRADE5 );
            priorities.emplace_back( BUILD_WELL );
            priorities.emplace_back( BUILD_STATUE );
            priorities.emplace_back( BUILD_SHIPYARD );
            break;
        case Race::SORC:
            priorities.emplace_back( BUILD_SPEC );
            priorities.emplace_back( DWELLING_MONSTER6 );
            priorities.emplace_back( BUILD_MAGEGUILD1 );
            priorities.emplace_back( BUILD_MAGEGUILD2 );
            priorities.emplace_back( BUILD_MAGEGUILD3 );
            priorities.emplace_back( BUILD_MAGEGUILD4 );
            priorities.emplace_back( BUILD_MAGEGUILD5 );
            priorities.emplace_back( BUILD_CAPTAIN );
            priorities.emplace_back( BUILD_TENT );
            priorities.emplace_back( BUILD_CASTLE );
            priorities.emplace_back( BUILD_LEFTTURRET );
            priorities.emplace_back( BUILD_RIGHTTURRET );
            priorities.emplace_back( BUILD_MOAT );
            priorities.emplace_back( DWELLING_MONSTER3 );
            priorities.emplace_back( DWELLING_UPGRADE3 );
            priorities.emplace_back( BUILD_SHIPYARD );
            priorities.emplace_back( BUILD_MARKETPLACE );
            priorities.emplace_back( DWELLING_MONSTER2 );
            priorities.emplace_back( DWELLING_UPGRADE2 );
            priorities.emplace_back( BUILD_THIEVESGUILD );
            priorities.emplace_back( DWELLING_MONSTER1 );
            priorities.emplace_back( BUILD_TAVERN );
            priorities.emplace_back( BUILD_STATUE );
            priorities.emplace_back( BUILD_WEL2 );
            priorities.emplace_back( DWELLING_MONSTER4 );
            priorities.emplace_back( DWELLING_UPGRADE4 );
            priorities.emplace_back( BUILD_WELL );
            priorities.emplace_back( DWELLING_MONSTER5 );
            break;
        case Race::WRLK:
            priorities.emplace_back( DWELLING_MONSTER5 );
            priorities.emplace_back( DWELLING_MONSTER3 );
            priorities.emplace_back( BUILD_TENT );
            priorities.emplace_back( BUILD_CASTLE );
            priorities.emplace_back( BUILD_LEFTTURRET );
            priorities.emplace_back( BUILD_RIGHTTURRET );
            priorities.emplace_back( BUILD_MOAT );
            priorities.emplace_back( BUILD_CAPTAIN );
            priorities.emplace_back( BUILD_SHIPYARD );
            priorities.emplace_back( BUILD_MAGEGUILD1 );
            priorities.emplace_back( BUILD_MAGEGUILD2 );
            priorities.emplace_back( BUILD_MAGEGUILD3 );
            priorities.emplace_back( BUILD_MAGEGUILD4 );
            priorities.emplace_back( BUILD_MAGEGUILD5 );
            priorities.emplace_back( BUILD_TAVERN );
            priorities.emplace_back( BUILD_THIEVESGUILD );
            priorities.emplace_back( BUILD_MARKETPLACE );
            priorities.emplace_back( BUILD_STATUE );
            priorities.emplace_back( DWELLING_MONSTER1 );
            priorities.emplace_back( BUILD_WEL2 );
            priorities.emplace_back( BUILD_SPEC );
            priorities.emplace_back( DWELLING_MONSTER4 );
            priorities.emplace_back( DWELLING_UPGRADE4 );
            priorities.emplace_back( DWELLING_MONSTER2 );
            priorities.emplace_back( DWELLING_MONSTER6 );
            priorities.emplace_back( DWELLING_UPGRADE6 );
            priorities.emplace_back( DWELLING_UPGRADE7 );
            priorities.emplace_back( BUILD_WELL );
            break;
        case Race::WZRD:
            priorities.emplace_back( DWELLING_MONSTER6 );
            priorities.emplace_back( DWELLING_UPGRADE6 );
            priorities.emplace_back( BUILD_TENT );
            priorities.emplace_back( BUILD_CASTLE );
            priorities.emplace_back( BUILD_LEFTTURRET );
            priorities.emplace_back( BUILD_RIGHTTURRET );
            priorities.emplace_back( BUILD_MOAT );
            priorities.emplace_back( BUILD_CAPTAIN );
            priorities.emplace_back( DWELLING_MONSTER2 );
            priorities.emplace_back( BUILD_THIEVESGUILD );
            priorities.emplace_back( BUILD_TAVERN );
            priorities.emplace_back( BUILD_SHIPYARD );
            priorities.emplace_back( BUILD_WELL );
            priorities.emplace_back( DWELLING_MONSTER3 );
            priorities.emplace_back( DWELLING_UPGRADE3 );
            priorities.emplace_back( DWELLING_MONSTER5 );
            priorities.emplace_back( DWELLING_UPGRADE5 );
            priorities.emplace_back( BUILD_MAGEGUILD1 );
            priorities.emplace_back( BUILD_MAGEGUILD2 );
            priorities.emplace_back( BUILD_MAGEGUILD3 );
            priorities.emplace_back( BUILD_MAGEGUILD4 );
            priorities.emplace_back( BUILD_MAGEGUILD5 );
            priorities.emplace_back( BUILD_SPEC );
            priorities.emplace_back( BUILD_STATUE );
            priorities.emplace_back( DWELLING_MONSTER1 );
            priorities.emplace_back( DWELLING_MONSTER4 );
            priorities.emplace_back( BUILD_MARKETPLACE );
            priorities.emplace_back( BUILD_WEL2 );
            break;
        case Race::NECR:
            priorities.emplace_back( BUILD_SPEC );
            if ( version == GameVersion::PRICE_OF_LOYALTY || version == GameVersion::RESURRECTION ) {
                priorities.emplace_back( BUILD_SHRINE );
            }
            priorities.emplace_back( BUILD_TENT );
            priorities.emplace_back( BUILD_CASTLE );
            priorities.emplace_back( BUILD_LEFTTURRET );
            priorities.emplace_back( BUILD_RIGHTTURRET );
            priorities.emplace_back( BUILD_MOAT );
            priorities.emplace_back( BUILD_CAPTAIN );
            priorities.emplace_back( DWELLING_MONSTER6 );
            priorities.emplace_back( DWELLING_MONSTER1 );
            priorities.emplace_back( BUILD_THIEVESGUILD );
            priorities.emplace_back( DWELLING_MONSTER3 );
            priorities.emplace_back( DWELLING_UPGRADE3 );
            priorities.emplace_back( DWELLING_MONSTER5 );
            priorities.emplace_back( DWELLING_UPGRADE5 );
            priorities.emplace_back( DWELLING_MONSTER2 );
            priorities.emplace_back( DWELLING_UPGRADE2 );
            priorities.emplace_back( DWELLING_MONSTER4 );
            priorities.emplace_back( DWELLING_UPGRADE4 );
            priorities.emplace_back( BUILD_MAGEGUILD1 );
            priorities.emplace_back( BUILD_MAGEGUILD2 );
            priorities.emplace_back( BUILD_MAGEGUILD3 );
            priorities.emplace_back( BUILD_MAGEGUILD4 );
            priorities.emplace_back( BUILD_MAGEGUILD5 );
            priorities.emplace_back( BUILD_SHIPYARD );
            priorities.emplace_back( BUILD_WEL2 );
            priorities.emplace_back( BUILD_MARKETPLACE );
            priorities.emplace_back( BUILD_STATUE );
            priorities.emplace_back( BUILD_WELL );
            break;
        default:
            // Did you add a new castle?
            assert( 0 );
            break;
        }

        return priorities;
    }
}
