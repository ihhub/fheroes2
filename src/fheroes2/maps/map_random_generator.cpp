/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2025 - 2026                                             *
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

#include "map_random_generator.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <map>
#include <ostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "color.h"
#include "direction.h"
#include "editor_interface.h"
#include "ground.h"
#include "logging.h"
#include "map_format_helper.h"
#include "map_format_info.h"
#include "map_object_info.h"
#include "map_random_generator_helper.h"
#include "map_random_generator_info.h"
#include "maps.h"
#include "maps_tiles.h"
#include "math_base.h"
#include "mp2.h"
#include "rand.h"
#include "resource.h"
#include "translations.h"
#include "ui_map_object.h"
#include "world.h"

namespace
{
    constexpr int32_t smallestStartingRegionSize{ 200 };
    constexpr int32_t regionSizeForSecondaryMines{ 250 };
    constexpr int32_t regionSizeForGoldMine{ 350 };
    constexpr int32_t emptySpacePercentage{ 40 };
    const std::vector<int> playerStartingTerrain = { Maps::Ground::GRASS, Maps::Ground::DIRT, Maps::Ground::SNOW, Maps::Ground::LAVA, Maps::Ground::WASTELAND };
    const std::vector<int> neutralTerrain = { Maps::Ground::GRASS,     Maps::Ground::DIRT,  Maps::Ground::SNOW,  Maps::Ground::LAVA,
                                              Maps::Ground::WASTELAND, Maps::Ground::BEACH, Maps::Ground::SWAMP, Maps::Ground::DESERT };

    constexpr std::array<Maps::Random_Generator::RegionalObjects, static_cast<size_t>( Maps::Random_Generator::ResourceDensity::ITEM_COUNT )> regionObjectSetup = { {
        { 1, 2, 0, 1, 1, 2, 8500 }, // ResourceDensity::SCARCE
        { 1, 4, 0, 2, 1, 3, 15000 }, // ResourceDensity::NORMAL
        { 1, 4, 1, 2, 2, 5, 25000 } // ResourceDensity::ABUNDANT
    } };

    int32_t calculateRegionSizeLimit( const Maps::Random_Generator::Configuration & config, const int32_t width, const int32_t height )
    {
        // Water percentage cannot be 100 or more, or negative.
        assert( config.waterPercentage >= 0 && config.waterPercentage <= 100 );

        int32_t requiredSpace = 0;

        // Determine required space based on expected object count and their footprint (in tiles).
        const auto & objectSet = regionObjectSetup[static_cast<size_t>( config.resourceDensity )];
        requiredSpace += objectSet.castleCount * 49;
        requiredSpace += objectSet.mineCount * 15;
        requiredSpace += objectSet.objectCount * 6;
        requiredSpace += objectSet.powerUpsCount * 9;
        requiredSpace += objectSet.treasureCount * 16;

        DEBUG_LOG( DBG_DEVEL, DBG_TRACE, "Space required for density " << static_cast<int32_t>( config.resourceDensity ) << " is " << requiredSpace );

        requiredSpace = requiredSpace * 100 / ( 100 - emptySpacePercentage );

        const double innerRadius = std::ceil( sqrt( requiredSpace / M_PI ) );
        const int32_t borderSize = static_cast<int32_t>( 2 * ( innerRadius + 1 ) * M_PI );
        const int32_t targetRegionSize = requiredSpace + borderSize;

        DEBUG_LOG( DBG_DEVEL, DBG_TRACE, "Region target size is " << requiredSpace << " + " << borderSize << " = " << targetRegionSize );

        // Inner and outer circles, update later to handle other layouts.
        const int32_t upperLimit = config.playerCount * 3;

        const int32_t totalTileCount = width * height;
        const int32_t groundTiles = totalTileCount * ( 100 - config.waterPercentage ) / 100;

        const int32_t average = groundTiles / targetRegionSize;
        const int32_t canFit = std::min( std::max( config.playerCount + 1, average ), upperLimit );

        return groundTiles / canFit;
    }
}

namespace Maps::Random_Generator
{
    const std::vector<ObjectSet> prefabObjectSets{ ObjectSet{ // Obstacles.
                                                              { { { 0, -1 }, ObjectGroup::LANDSCAPE_TREES, 3 },
                                                                { { 3, -1 }, ObjectGroup::LANDSCAPE_TREES, 2 },
                                                                { { 3, 1 }, ObjectGroup::LANDSCAPE_TREES, 3 } },
                                                              // Valuables.
                                                              { { { 1, -1 }, ObjectGroup::ADVENTURE_TREASURES, 9 },
                                                                { { 2, -1 }, ObjectGroup::ADVENTURE_TREASURES, 9 },
                                                                { { 1, 0 }, ObjectGroup::ADVENTURE_TREASURES, 9 },
                                                                { { 2, 0 }, ObjectGroup::ADVENTURE_TREASURES, 9 } },
                                                              // Entrance check.
                                                              { { -1, 0 }, { -1, 1 }, { 0, 1 } } },
                                                   ObjectSet{ // Obstacles.
                                                              { { { -3, 0 }, ObjectGroup::LANDSCAPE_TREES, 3 },
                                                                { { -3, 1 }, ObjectGroup::LANDSCAPE_TREES, 0 },
                                                                { { 0, 2 }, ObjectGroup::LANDSCAPE_TREES, 3 } },
                                                              // Valuables.
                                                              { { { -2, 0 }, ObjectGroup::ADVENTURE_TREASURES, 9 },
                                                                { { -1, 0 }, ObjectGroup::ADVENTURE_TREASURES, 9 },
                                                                { { -2, 1 }, ObjectGroup::ADVENTURE_TREASURES, 9 },
                                                                { { -1, 1 }, ObjectGroup::ADVENTURE_TREASURES, 9 } },
                                                              // Entrance check.
                                                              { { 0, -1 }, { 1, -1 }, { 1, 0 } } },
                                                   ObjectSet{ // Obstacles.
                                                              { { { 3, -1 }, ObjectGroup::LANDSCAPE_TREES, 0 },
                                                                { { 3, 1 }, ObjectGroup::LANDSCAPE_TREES, 1 },
                                                                { { 1, 2 }, ObjectGroup::LANDSCAPE_TREES, 5 },
                                                                { { 0, 2 }, ObjectGroup::LANDSCAPE_TREES, 4 } },
                                                              // Valuables.
                                                              { { { 1, 0 }, ObjectGroup::ADVENTURE_TREASURES, 9 },
                                                                { { 2, 0 }, ObjectGroup::ADVENTURE_TREASURES, 9 },
                                                                { { 1, 1 }, ObjectGroup::ADVENTURE_TREASURES, 9 },
                                                                { { 2, 1 }, ObjectGroup::ADVENTURE_TREASURES, 9 } },
                                                              // Entrance check.
                                                              { { -1, -1 }, { -1, 0 }, { -1, 1 }, { 0, -1 } } },
                                                   ObjectSet{ // Obstacles.
                                                              { { { -3, -1 }, ObjectGroup::LANDSCAPE_TREES, 1 },
                                                                { { -1, 2 }, ObjectGroup::LANDSCAPE_TREES, 4 },
                                                                { { -3, 1 }, ObjectGroup::LANDSCAPE_TREES, 0 },
                                                                { { 0, 2 }, ObjectGroup::LANDSCAPE_TREES, 5 } },
                                                              // Valuables.
                                                              { { { -1, 0 }, ObjectGroup::ADVENTURE_TREASURES, 9 },
                                                                { { -2, 0 }, ObjectGroup::ADVENTURE_TREASURES, 9 },
                                                                { { -1, 1 }, ObjectGroup::ADVENTURE_TREASURES, 9 },
                                                                { { -2, 1 }, ObjectGroup::ADVENTURE_TREASURES, 9 } },
                                                              // Entrance check.
                                                              { { 1, -1 }, { 1, 0 }, { 1, 1 }, { 0, -1 } } },
                                                   ObjectSet{ // Obstacles.
                                                              { { { -3, 0 }, ObjectGroup::LANDSCAPE_TREES, 3 },
                                                                { { -1, -1 }, ObjectGroup::LANDSCAPE_TREES, 5 },
                                                                { { -3, 1 }, ObjectGroup::LANDSCAPE_TREES, 2 } },
                                                              // Valuables.
                                                              { { { -2, 0 }, ObjectGroup::ADVENTURE_TREASURES, 9 }, { { -1, 0 }, ObjectGroup::ADVENTURE_TREASURES, 9 } },
                                                              // Entrance check.
                                                              { { 1, -1 }, { 1, 0 }, { 1, 1 }, { 0, -1 }, { 0, 1 } } } };

    const std::vector<ObjectSet> powerupObjectSets{
        ObjectSet{ // Obstacles.
                   { { { 0, -1 }, ObjectGroup::LANDSCAPE_TREES, 3 }, { { 3, -1 }, ObjectGroup::LANDSCAPE_TREES, 2 }, { { 3, 1 }, ObjectGroup::LANDSCAPE_TREES, 3 } },
                   // Valuables.
                   { { { 2, -1 }, ObjectGroup::ADVENTURE_POWER_UPS, 10 },
                     { { 1, 0 }, ObjectGroup::ADVENTURE_TREASURES, 9 },
                     { { 2, 0 }, ObjectGroup::ADVENTURE_TREASURES, 9 } },
                   // Entrance check.
                   { { -1, 0 }, { -1, 1 }, { 0, 1 } } },
        ObjectSet{ // Obstacles.
                   { { { 0, -1 }, ObjectGroup::LANDSCAPE_TREES, 3 }, { { 3, -1 }, ObjectGroup::LANDSCAPE_TREES, 2 }, { { 3, 1 }, ObjectGroup::LANDSCAPE_TREES, 3 } },
                   // Valuables.
                   { { { 1, -1 }, ObjectGroup::ADVENTURE_POWER_UPS, 18 },
                     { { 1, 0 }, ObjectGroup::ADVENTURE_TREASURES, 9 },
                     { { 2, 0 }, ObjectGroup::ADVENTURE_TREASURES, 9 } },
                   // Entrance check.
                   { { -1, 0 }, { -1, 1 }, { 0, 1 } } },
        ObjectSet{ // Obstacles.
                   { { { 1, -2 }, ObjectGroup::LANDSCAPE_TREES, 1 }, { { 3, -1 }, ObjectGroup::LANDSCAPE_TREES, 2 }, { { 3, 1 }, ObjectGroup::LANDSCAPE_TREES, 3 } },
                   // Valuables.
                   { { { 2, -1 }, ObjectGroup::ADVENTURE_POWER_UPS, 12 },
                     { { 1, 0 }, ObjectGroup::ADVENTURE_TREASURES, 9 },
                     { { 2, 0 }, ObjectGroup::ADVENTURE_TREASURES, 9 } },
                   // Entrance check.
                   { { -1, 0 }, { -1, 1 }, { 0, 1 } } },
        ObjectSet{ // Obstacles.
                   { { { 0, -1 }, ObjectGroup::LANDSCAPE_TREES, 3 }, { { 3, -1 }, ObjectGroup::LANDSCAPE_TREES, 2 }, { { 3, 1 }, ObjectGroup::LANDSCAPE_TREES, 3 } },
                   // Valuables.
                   { { { 1, -1 }, ObjectGroup::ADVENTURE_POWER_UPS, 13 },
                     { { 1, 0 }, ObjectGroup::ADVENTURE_TREASURES, 9 },
                     { { 2, 0 }, ObjectGroup::ADVENTURE_TREASURES, 9 } },
                   // Entrance check.
                   { { -1, 0 }, { -1, 1 }, { 0, 1 } } },
        ObjectSet{ // Obstacles.
                   { { { -3, -1 }, ObjectGroup::LANDSCAPE_MOUNTAINS, 5 },
                     { { -1, -1 }, ObjectGroup::LANDSCAPE_MOUNTAINS, 4 },
                     { { -3, 0 }, ObjectGroup::LANDSCAPE_TREES, 0 } },
                   // Valuables.
                   { { { -2, -1 }, ObjectGroup::ADVENTURE_POWER_UPS, 16 },
                     { { -1, 0 }, ObjectGroup::ADVENTURE_TREASURES, 9 },
                     { { -2, 0 }, ObjectGroup::ADVENTURE_TREASURES, 9 } },
                   // Entrance check.
                   { { 1, 0 }, { -1, 1 }, { 0, 1 }, { 1, 1 } } },
    };

    const std::map<int32_t, std::vector<DecorationSet>> decorationsPerGround{
        { Maps::Ground::ALL,
          {
              { // Large mountain cross.
                {
                    { { 0, 0 }, ObjectGroup::LANDSCAPE_MOUNTAINS, 1 },
                    { { -2, 2 }, ObjectGroup::LANDSCAPE_MOUNTAINS, 1 },
                    { { -3, 0 }, ObjectGroup::LANDSCAPE_MOUNTAINS, 4 },
                    { { 1, 2 }, ObjectGroup::LANDSCAPE_MOUNTAINS, 2 },
                },
                // Optional.
                {} },
              { // Two large mountains.
                {
                    { { 0, 0 }, ObjectGroup::LANDSCAPE_MOUNTAINS, 0 },
                    { { -3, 1 }, ObjectGroup::LANDSCAPE_MOUNTAINS, 1 },
                    { { -2, 2 }, ObjectGroup::LANDSCAPE_TREES, 3 },
                },
                // Optional.
                {} },
              { // Vertical mountain chain.
                {
                    { { 0, 0 }, ObjectGroup::LANDSCAPE_MOUNTAINS, 2 },
                    { { 0, 2 }, ObjectGroup::LANDSCAPE_MOUNTAINS, 3 },
                    { { 0, 4 }, ObjectGroup::LANDSCAPE_MOUNTAINS, 2 },
                },
                {
                    // Optional.
                    { { -1, 0 }, ObjectGroup::LANDSCAPE_TREES, 0 },
                    { { 1, 3 }, ObjectGroup::LANDSCAPE_TREES, 0 },
                    { { -1, 4 }, ObjectGroup::LANDSCAPE_TREES, 0 },
                } },
              { // Small mountains.
                {
                    { { 0, 0 }, ObjectGroup::LANDSCAPE_MOUNTAINS, 2 },
                    { { 1, 1 }, ObjectGroup::LANDSCAPE_MOUNTAINS, 3 },
                    { { -1, 1 }, ObjectGroup::LANDSCAPE_MOUNTAINS, 5 },
                },
                // Optional.
                {} },
              { // Small mountains.
                {
                    { { 0, 0 }, ObjectGroup::LANDSCAPE_MOUNTAINS, 4 },
                    { { -1, 1 }, ObjectGroup::LANDSCAPE_MOUNTAINS, 5 },
                    { { 0, 1 }, ObjectGroup::LANDSCAPE_TREES, 4 },
                },
                // Optional.
                {} },
              { // Horizontal tree line.
                {
                    { { -2, 0 }, ObjectGroup::LANDSCAPE_TREES, 3 },
                    { { 0, 0 }, ObjectGroup::LANDSCAPE_TREES, 2 },
                    { { 2, -1 }, ObjectGroup::LANDSCAPE_TREES, 1 },
                    { { 4, 0 }, ObjectGroup::LANDSCAPE_TREES, 2 },
                },
                // Optional.
                {
                    { { -2, -1 }, ObjectGroup::LANDSCAPE_TREES, 0 },
                } },
              { // Y tree group.
                {
                    { { 0, 0 }, ObjectGroup::LANDSCAPE_TREES, 0 },
                    { { -1, 1 }, ObjectGroup::LANDSCAPE_TREES, 1 },
                },
                // Optional.
                {} },
          } },
        { Maps::Ground::GRASS,
          {
              {
                  // Obstacles.
                  { { { 0, 0 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 4 },
                    { { -2, -1 }, ObjectGroup::LANDSCAPE_MOUNTAINS, 5 },
                    { { 0, -2 }, ObjectGroup::LANDSCAPE_MOUNTAINS, 5 },
                    { { 1, -3 }, ObjectGroup::LANDSCAPE_TREES, 0 },
                    { { 3, -2 }, ObjectGroup::LANDSCAPE_TREES, 0 },
                    { { 4, -1 }, ObjectGroup::LANDSCAPE_TREES, 0 },
                    { { -3, -1 }, ObjectGroup::LANDSCAPE_TREES, 3 },
                    { { 2, 0 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 14 },
                    { { -2, 0 }, ObjectGroup::LANDSCAPE_ROCKS, 19 } },
                  // Optional.
                  {},
              },
              {
                  // Obstacles.
                  { { { 0, 0 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 5 },
                    { { -3, -1 }, ObjectGroup::LANDSCAPE_TREES, 1 },
                    { { 1, -1 }, ObjectGroup::LANDSCAPE_TREES, 2 },
                    { { -1, -2 }, ObjectGroup::LANDSCAPE_TREES, 3 },
                    { { 0, 0 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 14 },
                    { { 0, 0 }, ObjectGroup::LANDSCAPE_TREES, 4 } },
                  // Optional.
                  {},
              },
          } },
        { Maps::Ground::SNOW,
          {
              {
                  // Obstacles.
                  { { { 0, 0 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 37 },
                    { { -1, 0 }, ObjectGroup::LANDSCAPE_TREES, 0 },
                    { { 1, 1 }, ObjectGroup::LANDSCAPE_TREES, 0 },
                    { { -2, 2 }, ObjectGroup::LANDSCAPE_TREES, 1 },
                    { { 0, 3 }, ObjectGroup::LANDSCAPE_TREES, 2 },
                    { { 1, 0 }, ObjectGroup::LANDSCAPE_TREES, 3 },
                    { { 2, 0 }, ObjectGroup::LANDSCAPE_ROCKS, 12 } },
                  // Optional.
                  {},
              },
              {
                  // Obstacles.
                  { { { 0, 0 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 37 },
                    { { -1, -1 }, ObjectGroup::LANDSCAPE_TREES, 41 },
                    { { 2, 0 }, ObjectGroup::LANDSCAPE_TREES, 2 },
                    { { -1, 0 }, ObjectGroup::LANDSCAPE_TREES, 39 } },
                  // Optional.
                  {},
              },
              {
                  // Obstacles.
                  {
                      { { 0, 0 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 38 },
                      { { 2, -1 }, ObjectGroup::LANDSCAPE_TREES, 0 },
                      { { 1, 1 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 33 },
                  },
                  // Optional.
                  {},
              },
          } },
        { Maps::Ground::SWAMP,
          {
              {
                  // Swampy lake.
                  {
                      { { 0, 0 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 43 },
                      { { 1, -1 }, ObjectGroup::LANDSCAPE_TREES, 0 },
                      { { -2, 1 }, ObjectGroup::LANDSCAPE_TREES, 50 },
                      { { 0, 1 }, ObjectGroup::LANDSCAPE_TREES, 4 },
                  },
                  // Optional.
                  {},
              },
              {
                  // Smaller lakes.
                  {
                      { { 0, 0 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 44 },
                      { { 1, -4 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 45 },
                      { { -1, -3 }, ObjectGroup::LANDSCAPE_TREES, 0 },
                      { { 0, -2 }, ObjectGroup::LANDSCAPE_TREES, 0 },
                      { { -2, -1 }, ObjectGroup::LANDSCAPE_TREES, 1 },
                  },
                  // Optional.
                  {},
              },
          } },
        { Maps::Ground::LAVA,
          {
              {
                  // Large mountain with lava.
                  {
                      { { 0, 0 }, ObjectGroup::LANDSCAPE_MOUNTAINS, 1 },
                      { { 0, 2 }, ObjectGroup::LANDSCAPE_MOUNTAINS, 4 },
                      { { 3, -1 }, ObjectGroup::LANDSCAPE_MOUNTAINS, 3 },
                      { { 2, 0 }, ObjectGroup::LANDSCAPE_MOUNTAINS, 55 },
                  },
                  // Optional.
                  {
                      { { 3, 2 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 74 },
                      { { 1, 1 }, ObjectGroup::LANDSCAPE_TREES, 5 },
                      { { 0, -3 }, ObjectGroup::LANDSCAPE_MOUNTAINS, 2 },
                      { { -1, -2 }, ObjectGroup::LANDSCAPE_TREES, 2 },
                      { { -1, 2 }, ObjectGroup::LANDSCAPE_TREES, 0 },
                      { { 5, 0 }, ObjectGroup::LANDSCAPE_TREES, 0 },
                  },
              },
              {
                  // Large lava pools.
                  {
                      { { 0, 0 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 74 },
                      { { -2, -1 }, ObjectGroup::LANDSCAPE_TREES, 3 },
                      { { -3, -1 }, ObjectGroup::LANDSCAPE_MOUNTAINS, 57 },
                      { { 2, -1 }, ObjectGroup::LANDSCAPE_MOUNTAINS, 2 },
                      { { 0, 1 }, ObjectGroup::LANDSCAPE_TREES, 3 },
                      { { -2, 1 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 80 },
                      { { 2, 1 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 81 },
                  },
                  // Optional.
                  {},
              },
              {
                  // Two large lava pools.
                  {
                      { { 0, 0 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 74 },
                      { { 2, -1 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 75 },
                      { { 0, -1 }, ObjectGroup::LANDSCAPE_MOUNTAINS, 57 },
                      { { 3, -1 }, ObjectGroup::LANDSCAPE_MOUNTAINS, 55 },
                      { { 2, 0 }, ObjectGroup::LANDSCAPE_MOUNTAINS, 5 },
                      { { 2, -2 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 79 },
                      { { -3, 0 }, ObjectGroup::LANDSCAPE_TREES, 0 },
                  },
                  // Optional.
                  {
                      { { 2, 1 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 81 },
                      { { -2, -1 }, ObjectGroup::LANDSCAPE_TREES, 3 },
                      { { -3, 0 }, ObjectGroup::LANDSCAPE_TREES, 3 },
                  },
              },
              {
                  // Small lava pool.
                  {
                      { { 0, 0 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 75 },
                      { { 0, -1 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 77 },
                      { { 2, -1 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 78 },
                      { { 1, -1 }, ObjectGroup::LANDSCAPE_TREES, 0 },
                  },
                  // Optional.
                  {
                      { { 1, 1 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 77 },
                      { { 2, 0 }, ObjectGroup::LANDSCAPE_TREES, 0 },
                      { { -1, 1 }, ObjectGroup::LANDSCAPE_TREES, 2 },
                  },
              },
              {
                  // Craters.
                  {
                      { { 0, 0 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 69 },
                      { { 0, 1 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 70 },
                      { { 1, -1 }, ObjectGroup::LANDSCAPE_MOUNTAINS, 5 },
                  },
                  // Optional.
                  {
                      { { 4, -1 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 69 },
                      { { 1, -2 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 70 },
                      { { 2, 0 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 79 },
                  },
              },
          } },
        { Maps::Ground::DIRT,
          { {
              // Large dirt water lake.
              {
                  { { 0, 0 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 103 },
                  { { 2, -2 }, ObjectGroup::LANDSCAPE_TREES, 0 },
                  { { 3, 1 }, ObjectGroup::LANDSCAPE_TREES, 1 },
                  { { -4, -1 }, ObjectGroup::LANDSCAPE_TREES, 1 },
              },
              // Optional.
              {
                  { { 3, 1 }, ObjectGroup::LANDSCAPE_TREES, 1 },
                  { { 4, -1 }, ObjectGroup::LANDSCAPE_TREES, 1 },
              },
          } } },
        { Maps::Ground::WASTELAND,
          {
              {
                  // Obstacles.
                  { { { 0, 0 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 138 },
                    { { 1, -1 }, ObjectGroup::LANDSCAPE_MOUNTAINS, 2 },
                    { { -3, 1 }, ObjectGroup::LANDSCAPE_MOUNTAINS, 6 },
                    { { 1, -2 }, ObjectGroup::LANDSCAPE_MOUNTAINS, 6 },
                    { { -2, -1 }, ObjectGroup::LANDSCAPE_TREES, 3 },
                    { { -4, 1 }, ObjectGroup::LANDSCAPE_TREES, 5 },
                    { { -2, 1 }, ObjectGroup::LANDSCAPE_ROCKS, 29 } },
                  // Optional.
                  {
                      { { 2, 1 }, ObjectGroup::LANDSCAPE_TREES, 1 },
                      { { 0, 3 }, ObjectGroup::LANDSCAPE_TREES, 3 },
                      { { -1, -2 }, ObjectGroup::LANDSCAPE_ROCKS, 26 },
                      { { -4, -1 }, ObjectGroup::LANDSCAPE_TREES, 1 },
                  },
              },
              {
                  // Obstacles.
                  {
                      { { 0, 0 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 134 },
                      { { 2, 0 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 139 },
                      { { 0, -1 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 141 },
                  },
                  // Optional.
                  {
                      { { -2, 1 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 139 },
                      { { 0, 1 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 140 },
                      { { -2, 0 }, ObjectGroup::LANDSCAPE_ROCKS, 39 },
                      { { 2, 0 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 132 },
                  },
              },
              {
                  // Obstacles.
                  {
                      { { 0, 0 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 138 },
                      { { 0, -1 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 139 },
                      { { -3, -1 }, ObjectGroup::LANDSCAPE_TREES, 1 },
                      { { 2, 0 }, ObjectGroup::LANDSCAPE_TREES, 2 },
                      { { -1, 1 }, ObjectGroup::LANDSCAPE_TREES, 4 },
                  },
                  // Optional.
                  {
                      { { 1, 1 }, ObjectGroup::LANDSCAPE_TREES, 1 },
                      { { -3, 1 }, ObjectGroup::LANDSCAPE_TREES, 2 },
                      { { -4, 1 }, ObjectGroup::LANDSCAPE_ROCKS, 30 },
                  },
              },
          } },
        { Maps::Ground::BEACH,
          {
              {
                  // Large dirt water lake.
                  {
                      { { 0, 0 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 103 },
                      { { 0, 1 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 56 },
                      { { -4, -1 }, ObjectGroup::LANDSCAPE_ROCKS, 22 },
                      { { 1, -2 }, ObjectGroup::LANDSCAPE_MOUNTAINS, 5 },
                      { { 2, -2 }, ObjectGroup::LANDSCAPE_TREES, 1 },
                      { { 3, 1 }, ObjectGroup::LANDSCAPE_TREES, 1 },
                      { { -4, 0 }, ObjectGroup::LANDSCAPE_TREES, 0 },
                  },
                  // Optional.
                  {},
              },
              {
                  // Small grass water lake.
                  {
                      { { 0, 0 }, ObjectGroup::LANDSCAPE_MISCELLANEOUS, 5 },
                      { { 1, -1 }, ObjectGroup::LANDSCAPE_MOUNTAINS, 2 },
                      { { 1, 1 }, ObjectGroup::LANDSCAPE_MOUNTAINS, 5 },
                      { { -2, -1 }, ObjectGroup::LANDSCAPE_TREES, 1 },
                      { { 2, 0 }, ObjectGroup::LANDSCAPE_TREES, 5 },
                      { { 0, 1 }, ObjectGroup::LANDSCAPE_TREES, 55 },
                  },
                  // Optional.
                  {
                      { { -2, 1 }, ObjectGroup::LANDSCAPE_TREES, 1 },
                      { { 0, -2 }, ObjectGroup::LANDSCAPE_TREES, 1 },
                  },
              },
          } },
    };

    std::string layoutToString( const Layout layout )
    {
        switch ( layout ) {
        case Layout::MIRRORED:
            return _( "map_layout|Mirrored" );
        case Layout::BALANCED:
            return _( "map_layout|Balanced" );
        case Layout::ISLANDS:
            return _( "map_layout|Islands" );
        case Layout::PYRAMID:
            return _( "map_layout|Pyramid" );
        case Layout::QUEST:
            return _( "map_layout|Quest" );
        default:
            // Did you add a new layout type? Add the logic above!
            assert( 0 );
            break;
        }

        return {};
    }

    std::string resourceDensityToString( const ResourceDensity resources )
    {
        switch ( resources ) {
        case ResourceDensity::SCARCE:
            return _( "resource_density|Scarce" );
        case ResourceDensity::NORMAL:
            return _( "resource_density|Normal" );
        case ResourceDensity::ABUNDANT:
            return _( "resource_density|Abundant" );
        default:
            // Did you add a new resource density type? Add the logic above!
            assert( 0 );
            break;
        }

        return {};
    }

    std::string monsterStrengthToString( const MonsterStrength monsters )
    {
        switch ( monsters ) {
        case MonsterStrength::WEAK:
            return _( "monster_strength|Weak" );
        case MonsterStrength::NORMAL:
            return _( "monster_strength|Normal" );
        case MonsterStrength::STRONG:
            return _( "monster_strength|Strong" );
        case MonsterStrength::DEADLY:
            return _( "monster_strength|Deadly" );
        default:
            // Did you add a new monster strength type? Add the logic above!
            assert( 0 );
            break;
        }

        return {};
    }

    int32_t calculateMaximumWaterPercentage( const int32_t playerCount, const int32_t mapWidth )
    {
        assert( playerCount > 0 && mapWidth > 0 );

        const int32_t minimumRegionCount = playerCount + 1;
        const int32_t tileCount = mapWidth * mapWidth;
        const int32_t waterTiles = tileCount - ( smallestStartingRegionSize * minimumRegionCount );
        return std::max<int32_t>( 0, waterTiles * 100 / tileCount );
    }

    bool generateMap( Map_Format::MapFormat & mapFormat, const Configuration & config, const int32_t width, const int32_t height )
    {
        // Make sure that we are generating a valid map.
        assert( width > 0 && height > 0 );

        if ( config.playerCount < 2 || config.playerCount > 6 ) {
            assert( config.playerCount <= 6 );
            return false;
        }

        // Initialization step. Reset the current map in `world` and `mapFormat` containers first.
        Interface::EditorInterface & interface = Interface::EditorInterface::Get();
        if ( !interface.generateNewMap( width ) ) {
            return false;
        }

        const int32_t regionSizeLimit = calculateRegionSizeLimit( config, width, height );
        if ( regionSizeLimit <= 0 ) {
            DEBUG_LOG( DBG_DEVEL, DBG_WARN, "Region size limit is " << regionSizeLimit )
            return false;
        }

        const uint32_t generatorSeed = ( config.seed > 0 ) ? config.seed : Rand::Get( 999999 );
        DEBUG_LOG( DBG_DEVEL, DBG_INFO, "Generating a map with seed " << generatorSeed );
        DEBUG_LOG( DBG_DEVEL, DBG_INFO, "Region size limit " << regionSizeLimit << ", water " << config.waterPercentage << "%" );

        Rand::PCG32 randomGenerator( generatorSeed );

        MapStateManager mapState( width, height );

        auto mapBoundsCheck = [width, height]( int32_t x, int32_t y ) {
            x = std::clamp<int32_t>( x, 0, width - 1 );
            y = std::clamp<int32_t>( y, 0, height - 1 );
            return x * width + y;
        };

        // Step 1. Setup map generator configuration.
        // TODO: Add support for layouts other than MIRRORED
        const int32_t groundTiles = ( width * height ) * ( 100 - config.waterPercentage ) / 100;
        const int expectedRegionCount = groundTiles / regionSizeLimit;
        const RegionalObjects & regionConfiguration = regionObjectSetup[static_cast<size_t>( config.resourceDensity )];

        // Step 2. Determine region layout and placement.
        //         Insert empty region that represents water and map edges
        std::vector<Region> mapRegions = { { 0, mapState.getNodeToUpdate( 0 ), neutralColorIndex, Ground::WATER, 1, 0, RegionType::NEUTRAL } };

        const int32_t neutralRegionCount = std::max<int32_t>( 1, expectedRegionCount - config.playerCount );
        const int32_t innerLayer = std::min<int32_t>( neutralRegionCount, config.playerCount );
        const int32_t outerLayer = std::max<int32_t>( std::min<int32_t>( neutralRegionCount, innerLayer * 2 ), config.playerCount );
        const double distanceModifier = ( config.waterPercentage > 20 ) ? 0.8 : 0.9;

        const double radius = sqrt( ( innerLayer + outerLayer ) * regionSizeLimit / M_PI );
        const double outerRadius = ( ( innerLayer + outerLayer ) > expectedRegionCount ) ? std::max( width, height ) * 0.47 : radius * distanceModifier;
        const double innerRadius = ( innerLayer == 1 ) ? 0 : outerRadius / 3;

        const std::vector<std::pair<int, double>> mapLayers = { { innerLayer, innerRadius }, { outerLayer, outerRadius } };

        int placedPlayers = 0;
        for ( size_t layer = 0; layer < mapLayers.size(); ++layer ) {
            const int regionCount = mapLayers[layer].first;
            const double startingAngle = Rand::GetWithGen( 0, 360, randomGenerator );
            const double offsetAngle = 360.0 / regionCount;
            const bool regionCountIsEven = ( regionCount % config.playerCount ) == 0;
            for ( int i = 0; i < regionCount; ++i ) {
                const double radians = ( startingAngle + offsetAngle * i ) * M_PI / 180;
                const double distance = mapLayers[layer].second;

                const int x = width / 2 + static_cast<int>( cos( radians ) * distance );
                const int y = height / 2 + static_cast<int>( sin( radians ) * distance );
                const int centerTile = mapBoundsCheck( x, y );

                const int factor = regionCount * placedPlayers / config.playerCount;
                const bool isPlayerRegion = ( layer == 1 && factor == i );

                const int groundType = isPlayerRegion ? Rand::GetWithGen( playerStartingTerrain, randomGenerator ) : Rand::GetWithGen( neutralTerrain, randomGenerator );
                const int regionColor = isPlayerRegion ? placedPlayers : neutralColorIndex;
                const int32_t treasureLimit = isPlayerRegion ? regionConfiguration.treasureValueLimit : regionConfiguration.treasureValueLimit * 2;

                const uint32_t regionID = static_cast<uint32_t>( mapRegions.size() );
                Node & centerNode = mapState.getNodeToUpdate( centerTile );
                const RegionType neutralType = ( layer == 1 && regionCountIsEven ) ? RegionType::EXPANSION : RegionType::NEUTRAL;
                const RegionType type = isPlayerRegion ? RegionType::STARTING : neutralType;
                const int32_t regionSize
                    = ( type == RegionType::STARTING && config.resourceDensity == ResourceDensity::SCARCE ) ? regionSizeLimit : regionSizeLimit * 6 / 5;
                mapRegions.emplace_back( regionID, centerNode, regionColor, groundType, regionSize, treasureLimit, type );

                if ( isPlayerRegion ) {
                    ++placedPlayers;
                }

                DEBUG_LOG( DBG_DEVEL, DBG_TRACE,
                           "Region " << regionID << " defined. Location " << centerTile << ", " << Ground::String( groundType ) << " terrain, owner "
                                     << Color::String( Color::IndexToColor( regionColor ) ) )
            }
        }

        // If this assertion blows up it means that the code above wasn't capable to place all players on the map.
        assert( placedPlayers == config.playerCount );

        // Step 3. Grow all regions one step at the time so they would compete for space.
        bool stillRoomToExpand = true;
        while ( stillRoomToExpand ) {
            stillRoomToExpand = false;
            // Skip the first region which is the border region.
            for ( size_t regionID = 1; regionID < mapRegions.size(); ++regionID ) {
                if ( mapRegions[regionID].regionExpansion( mapState, randomGenerator ) ) {
                    stillRoomToExpand = true;
                }
            }
        }

        // Step 4. Apply terrain changes into the map format.
        for ( const Region & region : mapRegions ) {
            if ( region.id == 0 ) {
                continue;
            }

            for ( const Node & node : region.nodes ) {
                Maps::setTerrainOnTile( mapFormat, node.index, region.groundType );
            }

            // Fix missing references.
            for ( const uint32_t adjacent : region.neighbours ) {
                const auto & [iter, inserted] = mapRegions[adjacent].neighbours.insert( region.id );
                if ( inserted ) {
                    DEBUG_LOG( DBG_DEVEL, DBG_WARN, "Missing link between " << region.id << " and " << adjacent )
                }
            }
        }

        // Step 5. Fix terrain transitions and place Castles
        MapEconomy mapEconomy;

        std::set<int32_t> primaryMineLocations;
        for ( Region & region : mapRegions ) {
            if ( region.id == 0 ) {
                continue;
            }

            DEBUG_LOG( DBG_DEVEL, DBG_TRACE, "Region #" << region.id << " of size " << region.nodes.size() << " tiles has " << region.neighbours.size() << " neighbours" )

            std::set<int32_t> extraNodes;
            for ( const Node & node : region.nodes ) {
                if ( node.type == NodeType::BORDER ) {
                    Maps::setTerrainWithTransition( mapFormat, node.index, node.index, region.groundType );

                    // Detect additional ground tiles created by setTerrainWithTransition
                    for ( const int32_t adjacentIndex : Maps::getAroundIndexes( node.index ) ) {
                        if ( world.getTile( adjacentIndex ).isWater() ) {
                            continue;
                        }

                        const Node & adjacentNode = mapState.getNode( adjacentIndex );
                        if ( adjacentNode.region == 0 && adjacentNode.index == adjacentIndex ) {
                            extraNodes.insert( adjacentIndex );
                        }
                    }
                }
            }

            for ( const int32_t extraNodeIndex : extraNodes ) {
                Node & extra = mapState.getNodeToUpdate( extraNodeIndex );
                region.nodes.emplace_back( extra );
                extra.region = region.id;
                extra.type = NodeType::BORDER;
            }

            if ( region.colorIndex != neutralColorIndex ) {
                const fheroes2::Point castlePos = region.adjustRegionToFitCastle( mapFormat );
                if ( !placeCastle( mapFormat, mapState, region, castlePos, true ) ) {
                    // Return early if we can't place a starting player castle.
                    DEBUG_LOG( DBG_DEVEL, DBG_WARN, "Not able to place a starting player castle on tile " << castlePos.x << ", " << castlePos.y )
                    return false;
                }
            }
            else if ( static_cast<int32_t>( region.nodes.size() ) > regionSizeLimit ) {
                // Place non-mandatory castles in bigger neutral regions.
                const bool useNeutralCastles = ( config.resourceDensity == ResourceDensity::ABUNDANT );
                const fheroes2::Point castlePos = region.adjustRegionToFitCastle( mapFormat );
                placeCastle( mapFormat, mapState, region, castlePos, useNeutralCastles );
            }
            else {
                mapState.getNodeToUpdate( region.centerIndex ).type = NodeType::PATH;
            }

            if ( region.type == RegionType::STARTING ) {
                const std::vector<std::vector<int32_t>> tileRings = findOpenTilesSortedJittered( region, width, randomGenerator );

                assert( tileRings.size() > 4 );

                for ( const int resource : { Resource::WOOD, Resource::ORE } ) {
                    for ( size_t ringIndex = 4; ringIndex < tileRings.size(); ++ringIndex ) {
                        const int32_t mineIndex = placeMine( mapFormat, mapState, mapEconomy, tileRings[ringIndex], resource, config.monsterStrength );
                        if ( mineIndex != -1 ) {
                            primaryMineLocations.insert( mineIndex );
                            break;
                        }
                    }
                }
            }
        }

        Maps::updatePlayerRelatedObjects( mapFormat );

        // Step 6. Set up region connectors based on frequency settings and border length.
        for ( Region & region : mapRegions ) {
            if ( region.groundType == Ground::WATER ) {
                continue;
            }
            for ( Node & node : region.nodes ) {
                region.checkNodeForConnections( mapState, mapRegions, node );
            }
        }

        for ( const Region & region : mapRegions ) {
            for ( const Node & node : region.nodes ) {
                if ( node.type == NodeType::BORDER ) {
                    placeBorderObstacle( mapFormat, mapState, node, region.groundType, randomGenerator );
                }
            }
        }

        // Step 7. Place mines.
        for ( const Region & region : mapRegions ) {
            if ( region.groundType == Ground::WATER ) {
                continue;
            }

            for ( const auto & [regionId, tileIndex] : region.connections ) {
                mapState.getNodeToUpdate( tileIndex ).type = NodeType::PATH;
                const auto & path = findPathToNearestRoad( mapState, width, region.id, tileIndex );
                for ( const auto & step : path ) {
                    mapState.getNodeToUpdate( step ).type = NodeType::PATH;
                    forceTempRoadOnTile( mapFormat, step );
                }
            }

            const auto & mineInfo = Maps::getObjectInfo( ObjectGroup::ADVENTURE_MINES, fheroes2::getMineObjectInfoId( Resource::GOLD, Ground::GRASS ) );
            std::vector<int32_t> options = findTilesForPlacement( mapState, width, region.id, findTilesByType( region, NodeType::OPEN ), mineInfo );
            if ( options.empty() ) {
                continue;
            }

            const uint8_t secondaryMineCount
                = ( regionSizeLimit > regionSizeForSecondaryMines || region.type == RegionType::NEUTRAL ) ? regionConfiguration.mineCount : 1;
            const std::vector<int32_t> avoidance( primaryMineLocations.begin(), primaryMineLocations.end() );
            options = pickEvenlySpacedTiles( options, static_cast<size_t>( secondaryMineCount ) * 3, avoidance );
            // It is expected that the container is not empty due to the check above.
            assert( !options.empty() );

            for ( size_t idx = 0; idx < secondaryMineCount; ++idx ) {
                const int resource = mapEconomy.pickNextMineResource();
                placeMine( mapFormat, mapState, mapEconomy, options, resource, config.monsterStrength );
            }

            if ( regionSizeLimit > regionSizeForGoldMine ) {
                for ( size_t idx = 0; idx < regionConfiguration.goldMineCount; ++idx ) {
                    placeMine( mapFormat, mapState, mapEconomy, options, Resource::GOLD, config.monsterStrength );
                }
            }
        }

        // Step 8: Place power-ups and treasure clusters while avoiding the paths.
        for ( Region & region : mapRegions ) {
            if ( region.groundType == Ground::WATER ) {
                continue;
            }
            placeObjectSet( mapFormat, mapState, region, powerupObjectSets, config.monsterStrength, regionConfiguration.powerUpsCount, randomGenerator );

            const auto & plan = planObjectPlacement( mapState, width, region, prefabObjectSets, randomGenerator );
            if ( plan.empty() ) {
                continue;
            }

            std::vector<int32_t> candidates;
            candidates.reserve( plan.size() );
            for ( const auto & [nodeIndex, placement] : plan ) {
                candidates.push_back( nodeIndex );
            }

            for ( const int32_t tileIndex : pickEvenlySpacedTiles( candidates, regionConfiguration.treasureCount, { region.centerIndex } ) ) {
                for ( const auto & [nodeIndex, placement] : plan ) {
                    if ( nodeIndex == tileIndex ) {
                        placeValidTreasures( mapFormat, mapState, region, placement, tileIndex, config.monsterStrength, randomGenerator );
                    }
                }
            }
        }

        // Step 9: Detect and fill empty areas with decorative/flavour objects.
        for ( Region & region : mapRegions ) {
            const auto it = decorationsPerGround.find( region.groundType );
            if ( it != decorationsPerGround.end() ) {
                placeDecorations( mapFormat, mapState, region, it->second, randomGenerator );
            }
            if ( region.groundType != Ground::WATER ) {
                placeDecorations( mapFormat, mapState, region, decorationsPerGround.at( Ground::ALL ), randomGenerator );
            }
        }

        // Step 10: Place missing monsters.
        const bool isSmallMap = ( mapFormat.width == Maps::SMALL );
        const auto & weakGuard = getMonstersByValue( config.monsterStrength, isSmallMap ? 3500 : 4500 );
        const auto & strongGuard = getMonstersByValue( config.monsterStrength, isSmallMap ? 6000 : 7500 );
        for ( const Region & region : mapRegions ) {
            for ( const auto & [regionId, tileIndex] : region.connections ) {
                if ( region.type == mapRegions[regionId].type ) {
                    placeMonster( mapFormat, mapState, tileIndex, strongGuard );
                }
                else {
                    placeMonster( mapFormat, mapState, tileIndex, weakGuard );
                }
            }
        }

        // Step 11. Place free pickup objects
        const auto & randomResourceInfo = convertMP2ToObjectInfo( MP2::OBJ_RANDOM_RESOURCE );
        for ( const Region & region : mapRegions ) {
            const auto & pathTiles = findTilesByType( region, NodeType::PATH );
            if ( pathTiles.empty() ) {
                continue;
            }

            for ( int32_t count = 0; count < regionConfiguration.treasureCount * 2; ++count ) {
                int32_t tileIndex = Rand::GetWithGen( pathTiles, randomGenerator );

                const int32_t direction = Rand::GetWithGen( Direction::allNeighboringDirections, randomGenerator );
                const int32_t adjancent = Maps::GetDirectionIndex( tileIndex, direction );
                if ( Maps::isValidDirection( tileIndex, direction ) && mapState.getNode( adjancent ).type == NodeType::OPEN ) {
                    tileIndex = adjancent;
                }

                if ( putObjectOnMap( mapFormat, world.getTile( tileIndex ), randomResourceInfo.first, randomResourceInfo.second ) ) {
                    mapState.getNodeToUpdate( tileIndex ).type = NodeType::ACTION;
                }
            }
        }

        Maps::updateAllRoads( mapFormat );
        Maps::updatePlayerRelatedObjects( mapFormat );

        // Visual debug
        for ( const Region & region : mapRegions ) {
            for ( const Node & node : region.nodes ) {
                const uint32_t metadata = static_cast<uint32_t>( node.type ) + 100 * node.region;
                world.getTile( node.index ).UpdateRegion( metadata );
            }
        }

        Maps::updateMapPlayers( mapFormat );

        // Set random map name and description to be unique.
        mapFormat.name = "Random map " + std::to_string( generatorSeed );
        mapFormat.description = "Randomly generated map of " + std::to_string( width ) + "x" + std::to_string( height ) + " with seed " + std::to_string( generatorSeed )
                                + ", " + std::to_string( config.playerCount ) + " players, up to " + std::to_string( config.waterPercentage ) + "% of water, "
                                + layoutToString( config.mapLayout ) + " layout, " + resourceDensityToString( config.resourceDensity ) + " resource density and "
                                + monsterStrengthToString( config.monsterStrength ) + " monster strength.";

        return true;
    }
}
