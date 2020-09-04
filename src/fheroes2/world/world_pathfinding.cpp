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

#include "world_pathfinding.h"
#include "ground.h"
#include "world.h"

namespace
{
    uint8_t ReflectDirectionIndex( uint8_t direction )
    {
        return ( direction + 4 ) % 8;
    }

    uint16_t GetDirectionBitmask( uint8_t direction, bool reflect = false )
    {
        return 1 << ( reflect ? ( direction + 4 ) % 8 : direction );
    }

    std::vector<int> GetDirectionOffsets( uint32_t mapWidth )
    {
        const int width = static_cast<int>( mapWidth );
        std::vector<int> offsets( 8 );
        offsets[TOP_LEFT] = -width - 1;
        offsets[TOP] = -width;
        offsets[TOP_RIGHT] = -width + 1;
        offsets[RIGHT] = 1;
        offsets[BOTTOM_RIGHT] = width + 1;
        offsets[BOTTOM] = width;
        offsets[BOTTOM_LEFT] = width - 1;
        offsets[LEFT] = -1;
        return offsets;
    }

    int ConvertExtendedIndex( int index, uint32_t width )
    {
        const uint32_t originalWidth = width - 2;
        return ( index / originalWidth + 1 ) * width + ( index % originalWidth ) + 1;
    }
}

void Pathfinder::reset()
{
    _cache.clear();
    _pathStart = -1;
    _pathfindingSkill = Skill::Level::NONE;
}

std::list<Route::Step> Pathfinder::buildPath( int from, int target, uint8_t skill )
{
    std::list<Route::Step> path;

    // check if we have to re-cache the map (new hero selected, etc)
    reEvaluateIfNeeded( from, skill, target );

    // trace the path from end point
    int currentNode = target;
    uint32_t cost = _cache[currentNode]._cost;
    while ( currentNode != from && currentNode != -1 ) {
        PathfindingNode & node = _cache[currentNode];

        path.push_front( { node._from, Direction::Get( node._from, currentNode ), cost - node._cost } );
        currentNode = node._from;
        cost = node._cost;
    }

    return path;
}

bool Pathfinder::isBlockedByObject( int from, int target )
{
    int currentNode = target;
    while ( currentNode != from && currentNode != -1 ) {
        if ( world.GetTiles( currentNode ).GetObject() != MP2::OBJ_ZERO ) {
            return true;
        }
        currentNode = _cache[currentNode]._from;
    }
    return false;
}

uint32_t Pathfinder::getDistance( int from, int target, uint8_t skill )
{
    reEvaluateIfNeeded( from, skill, target );
    return _cache[target]._cost;
}

bool Pathfinder::reEvaluateIfNeeded( int from, uint8_t skill, int destination )
{
    if ( _pathStart != from || _pathfindingSkill != skill ) {
        evaluateMap( from, skill, destination );
        return true;
    }
    return false;
}

uint32_t Pathfinder::getMovementPenalty( int from, int target, int direction, uint8_t skill )
{
    const Maps::Tiles & tileTo = world.GetTiles( target );
    uint32_t penalty = ( world.GetTiles( from ).isRoad( direction ) || tileTo.isRoad( Direction::Reflect( direction ) ) ) ? Maps::Ground::roadPenalty
                                                                                                                          : Maps::Ground::GetPenalty( tileTo, skill );

    // diagonal move costs 50% extra
    if ( direction & ( Direction::TOP_RIGHT | Direction::BOTTOM_RIGHT | Direction::BOTTOM_LEFT | Direction::TOP_LEFT ) )
        penalty = penalty * 3 / 2;

    return penalty;
}

// Destination is optional
void Pathfinder::evaluateMap( int start, uint8_t skill, int destination )
{
    const bool fromWater = world.GetTiles( start ).isWater();
    const int width = world.w();
    const int height = world.h();

    _pathStart = start;
    _pathfindingSkill = skill;

    _cache.clear();
    _cache.resize( width * height );
    _cache[start] = PathfindingNode( -1, 0 );

    std::vector<int> nodesToExplore;
    nodesToExplore.push_back( start );
    size_t lastProcessedNode = 0;
    while ( lastProcessedNode != nodesToExplore.size() ) {
        const int currentNodeIdx = nodesToExplore[lastProcessedNode];
        for ( uint8_t direction = 0; direction < 8; ++direction ) {
            const int dirBitmask = GetDirectionBitmask( direction );
            if ( Maps::isValidDirection( currentNodeIdx, dirBitmask ) ) {
                const int newIndex = Maps::GetDirectionIndex( currentNodeIdx, dirBitmask );
                Maps::Tiles & newTile = world.GetTiles( newIndex );

                uint32_t moveCost = _cache[currentNodeIdx]._cost + getMovementPenalty( currentNodeIdx, newIndex, skill );
                if ( Route::PassableFromToTile( currentNodeIdx, newIndex, dirBitmask, destination, fromWater ) ) {
                    if ( _cache[newIndex]._from == -1 || _cache[newIndex]._cost > moveCost ) {
                        _cache[newIndex]._from = currentNodeIdx;
                        _cache[newIndex]._cost = moveCost;
                        newTile.test_value = moveCost;

                        // duplicates are allowed if we find a cheaper way there
                        nodesToExplore.push_back( newIndex );
                    }
                }
            }
        }

        ++lastProcessedNode;
    }
}
