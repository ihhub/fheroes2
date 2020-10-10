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

bool isTileBlockedForArmy( double armyStrength, int color, int tileIndex, bool fromWater )
{
    const Maps::Tiles & tile = world.GetTiles( tileIndex );
    const bool toWater = tile.isWater();
    const int object = tile.GetObject();

    if ( object == MP2::OBJ_BOAT )
        return true;

    if ( object == MP2::OBJ_HEROES ) {
        const Heroes * otherHero = tile.GetHeroes();
        if ( otherHero ) {
            if ( otherHero->isFriends( color ) )
                return true;
            else
                return !otherHero->AllowBattle( false ) || otherHero->GetArmy().GetStrength() > armyStrength;
        }
    }

    if ( object == MP2::OBJ_MONSTER )
        return Army( tile ).GetStrength() > armyStrength;

    if ( fromWater && !toWater && object == MP2::OBJ_COAST )
        return true;

    return false;
}

bool World::isTileBlocked( int tileIndex, bool fromWater ) const
{
    const Maps::Tiles & tile = world.GetTiles( tileIndex );
    const bool toWater = tile.isWater();
    const int object = tile.GetObject();

    if ( object == MP2::OBJ_HEROES || object == MP2::OBJ_MONSTER || object == MP2::OBJ_BOAT )
        return true;

    if ( MP2::isPickupObject( object ) || MP2::isActionObject( object, fromWater ) )
        return true;

    if ( fromWater && !toWater && object == MP2::OBJ_COAST )
        return true;

    return false;
}

bool World::isValidPath( int index, int direction ) const
{
    const Maps::Tiles & fromTile = GetTiles( index );
    const Maps::Tiles & toTile = GetTiles( Maps::GetDirectionIndex( index, direction ) );
    const bool fromWater = fromTile.isWater();

    // check corner water/coast
    if ( fromWater ) {
        const int mapWidth = world.w();
        switch ( direction ) {
        case Direction::TOP_LEFT:
            if ( !GetTiles( index - mapWidth ).isWater() || !GetTiles( index - 1 ).isWater() )
                return false;
            break;

        case Direction::TOP_RIGHT:
            if ( !GetTiles( index - mapWidth ).isWater() || !GetTiles( index + 1 ).isWater() )
                return false;
            break;

        case Direction::BOTTOM_RIGHT:
            if ( !GetTiles( index + mapWidth ).isWater() || !GetTiles( index + 1 ).isWater() )
                return false;
            break;

        case Direction::BOTTOM_LEFT:
            if ( !GetTiles( index + mapWidth ).isWater() || !GetTiles( index - 1 ).isWater() )
                return false;
            break;

        default:
            break;
        }
    }

    if ( !fromTile.isPassable( direction, fromWater, false ) )
        return false;

    return toTile.isPassable( Direction::Reflect( direction ), fromWater, false );
}

WorldPathfinder::WorldPathfinder() {}

void WorldPathfinder::reset()
{
    const size_t worldSize = world.getSize();
    const bool needResizing = _cache.size() != worldSize;

    if ( needResizing || _pathStart != -1 || _pathfindingSkill != Skill::Level::NONE ) {
        _cache.clear();
        _pathStart = -1;
        _pathfindingSkill = Skill::Level::NONE;
    }

    if ( needResizing ) {
        _cache.resize( worldSize );

        const Directions directions = Direction::All();
        _mapOffset.resize( directions.size() );
        for ( size_t i = 0; i < directions.size(); ++i ) {
            _mapOffset[i] = Maps::GetDirectionIndex( 0, directions[i] );
        }
    }
}

std::list<Route::Step> WorldPathfinder::buildPath( int target ) const
{
    std::list<Route::Step> path;

    // trace the path from end point
    int currentNode = target;
    while ( currentNode != _pathStart && currentNode != -1 ) {
        const PathfindingNode & node = _cache[currentNode];
        const uint32_t cost = ( node._from != -1 ) ? node._cost - _cache[node._from]._cost : node._cost;

        path.emplace_front( node._from, Maps::GetDirection( node._from, currentNode ), cost );
        currentNode = node._from;
    }

    return path;
}

bool WorldPathfinder::isBlockedByObject( int target, bool fromWater )
{
    int currentNode = target;
    while ( currentNode != _pathStart && currentNode != -1 ) {
        if ( world.isTileBlocked( currentNode, fromWater ) ) {
            return true;
        }
        currentNode = _cache[currentNode]._from;
    }
    return false;
}

void WorldPathfinder::reEvaluateIfNeeded( int start, uint8_t skill )
{
    if ( _pathStart != start || _pathfindingSkill != skill ) {
        _pathStart = start;
        _pathfindingSkill = skill;

        processWorldMap( start );
    }
}

uint32_t WorldPathfinder::getMovementPenalty( int from, int target, int direction, uint8_t skill ) const
{
    const Maps::Tiles & tileTo = world.GetTiles( target );
    uint32_t penalty = ( world.GetTiles( from ).isRoad() && tileTo.isRoad() ) ? Maps::Ground::roadPenalty : Maps::Ground::GetPenalty( tileTo, skill );

    // diagonal move costs 50% extra
    if ( Direction::isDiagonal( direction ) )
        penalty = penalty * 3 / 2;

    return penalty;
}

int WorldPathfinder::searchForFog( int playerColor, int start, uint8_t skill )
{
    // paths have to be pre-calculated to find a spot where we're able to move
    reEvaluateIfNeeded( start, skill );

    const Directions directions = Direction::All();

    std::vector<bool> tilesVisited( world.getSize(), false );

    std::vector<int> nodesToExplore;
    nodesToExplore.push_back( start );
    tilesVisited[start] = true;

    for ( size_t lastProcessedNode = 0; lastProcessedNode < nodesToExplore.size(); ++lastProcessedNode ) {
        const int currentNodeIdx = nodesToExplore[lastProcessedNode];

        for ( size_t i = 0; i < directions.size(); ++i ) {
            if ( Maps::isValidDirection( currentNodeIdx, directions[i] ) ) {
                const int newIndex = currentNodeIdx + _mapOffset[i];
                if ( newIndex == start )
                    continue;

                if ( world.GetTiles( newIndex ).isFog( playerColor ) ) {
                    return currentNodeIdx;
                }
                else if ( !tilesVisited[newIndex] ) {
                    tilesVisited[newIndex] = true;

                    const MapsIndexes & monsters = Maps::GetTilesUnderProtection( newIndex );
                    if ( _cache[newIndex]._cost && monsters.empty() )
                        nodesToExplore.push_back( newIndex );
                }
            }
        }
    }
    return -1;
}

void WorldPathfinder::processWorldMap( int pathStart )
{
    const bool fromWater = world.GetTiles( pathStart ).isWater();

    // reset cache back to default value
    for ( size_t idx = 0; idx < _cache.size(); ++idx ) {
        _cache[idx]._from = -1;
        _cache[idx]._cost = 0;
    }
    _cache[pathStart] = PathfindingNode( -1, 0 );

    std::vector<int> nodesToExplore;
    nodesToExplore.push_back( pathStart );

    for ( size_t lastProcessedNode = 0; lastProcessedNode < nodesToExplore.size(); ++lastProcessedNode ) {
        const int currentNodeIdx = nodesToExplore[lastProcessedNode];
        processCurrentNode( nodesToExplore, pathStart, currentNodeIdx, fromWater );
    }
}

void WorldPathfinder::processCurrentNode( std::vector<int> & nodesToExplore, int pathStart, int currentNodeIdx, bool fromWater )
{
    const MapsIndexes & monsters = Maps::GetTilesUnderProtection( currentNodeIdx );

    // check if current tile is protected, can move only to adjacent monster
    if ( currentNodeIdx != pathStart && !monsters.empty() ) {
        for ( int monsterIndex : monsters ) {
            const int direction = Maps::GetDirection( currentNodeIdx, monsterIndex );

            if ( direction != Direction::UNKNOWN && direction != Direction::CENTER && world.isValidPath( currentNodeIdx, direction ) ) {
                // add straight to cache, can't move further from the monster
                const uint32_t moveCost = _cache[currentNodeIdx]._cost + getMovementPenalty( currentNodeIdx, monsterIndex, direction, _pathfindingSkill );
                PathfindingNode & monsterNode = _cache[monsterIndex];
                if ( monsterNode._from == -1 || monsterNode._cost > moveCost ) {
                    monsterNode._from = currentNodeIdx;
                    monsterNode._cost = moveCost;
                }
            }
        }
    }
    else if ( currentNodeIdx == pathStart || !world.isTileBlocked( currentNodeIdx, fromWater ) ) {
        checkAdjacentNodes( nodesToExplore, pathStart, currentNodeIdx, fromWater );
    }

}

void WorldPathfinder::checkAdjacentNodes( std::vector<int> & nodesToExplore, int pathStart, int currentNodeIdx, bool fromWater )
{
    const Directions directions = Direction::All();
    const PathfindingNode & currentNode = _cache[currentNodeIdx];

    for ( size_t i = 0; i < directions.size(); ++i ) {
        if ( Maps::isValidDirection( currentNodeIdx, directions[i] ) ) {
            const int newIndex = currentNodeIdx + _mapOffset[i];
            if ( newIndex == pathStart )
                continue;

            const uint32_t moveCost = currentNode._cost + getMovementPenalty( currentNodeIdx, newIndex, directions[i], _pathfindingSkill );
            PathfindingNode & newNode = _cache[newIndex];
            if ( world.isValidPath( currentNodeIdx, directions[i] ) && ( newNode._from == -1 || newNode._cost > moveCost ) ) {
                newNode._from = currentNodeIdx;
                newNode._cost = moveCost;

                // duplicates are allowed if we find a cheaper way there
                if ( world.GetTiles( newIndex ).isWater() == fromWater )
                    nodesToExplore.push_back( newIndex );
            }
        }
    }
}

AIWorldPathfinder::AIWorldPathfinder() {}

void AIWorldPathfinder::reset()
{
    WorldPathfinder::reset();

    if ( _pathStart != -1 || _pathfindingSkill != Skill::Level::NONE || _armyStrength >= 0 || _currentColor != Color::NONE ) {
        _cache.clear();
        _pathStart = -1;
        _pathfindingSkill = Skill::Level::NONE;
        _armyStrength = -1;
        _currentColor = Color::NONE;
    }
}

void AIWorldPathfinder::reEvaluateIfNeeded( int start, uint8_t skill, double armyStrength, int color )
{
    if ( _pathStart != start || _pathfindingSkill != skill || _armyStrength != armyStrength || _currentColor != color ) {
        _pathStart = start;
        _pathfindingSkill = skill;
        _armyStrength = armyStrength;
        _currentColor = color;

        processWorldMap( start );
    }
}

// Overwrites base version in WorldPathfinder, using custom node passability rules
void AIWorldPathfinder::processCurrentNode( std::vector<int> & nodesToExplore, int pathStart, int currentNodeIdx, bool fromWater )
{
    if ( currentNodeIdx == pathStart || !isTileBlockedForArmy( _armyStrength, Color::BLUE, currentNodeIdx, fromWater ) ) {
        checkAdjacentNodes( nodesToExplore, pathStart, currentNodeIdx, fromWater );
    }
}
