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

#include <set>

#include "world_pathfinding.h"
#include "ai.h"
#include "ground.h"
#include "world.h"

bool isTileBlockedForArmy( int tileIndex, int color, double armyStrength, bool fromWater )
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

    if ( object == MP2::OBJ_MONSTER || ( object == MP2::OBJ_ARTIFACT && tile.QuantityVariant() > 5 ) )
        return Army( tile ).GetStrength() > armyStrength;

    return ( fromWater && !toWater && object == MP2::OBJ_COAST );
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

void WorldPathfinder::checkWorldSize()
{
    const size_t worldSize = world.getSize();

    if ( _cache.size() != worldSize ) {
        _cache.clear();
        _cache.resize( worldSize );

        const Directions & directions = Direction::All();
        _mapOffset.resize( directions.size() );
        for ( size_t i = 0; i < directions.size(); ++i ) {
            _mapOffset[i] = Maps::GetDirectionIndex( 0, directions[i] );
        }
    }
}

bool WorldPathfinder::isBlockedByObject( int target, bool fromWater ) const
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

uint32_t WorldPathfinder::getMovementPenalty( int from, int target, int direction, uint8_t skill ) const
{
    const Maps::Tiles & tileTo = world.GetTiles( target );
    uint32_t penalty = ( world.GetTiles( from ).isRoad() && tileTo.isRoad() ) ? Maps::Ground::roadPenalty : Maps::Ground::GetPenalty( tileTo, skill );

    // diagonal move costs 50% extra
    if ( Direction::isDiagonal( direction ) )
        penalty = penalty * 3 / 2;

    return penalty;
}

void WorldPathfinder::processWorldMap( int pathStart )
{
    const bool fromWater = world.GetTiles( pathStart ).isWater();

    // reset cache back to default value
    for ( size_t idx = 0; idx < _cache.size(); ++idx ) {
        _cache[idx].resetNode();
    }
    _cache[pathStart] = PathfindingNode( -1, 0, 0 );

    std::vector<int> nodesToExplore;
    nodesToExplore.push_back( pathStart );

    for ( size_t lastProcessedNode = 0; lastProcessedNode < nodesToExplore.size(); ++lastProcessedNode ) {
        processCurrentNode( nodesToExplore, pathStart, nodesToExplore[lastProcessedNode], fromWater );
    }
}

void WorldPathfinder::checkAdjacentNodes( std::vector<int> & nodesToExplore, int pathStart, int currentNodeIdx, bool fromWater )
{
    const Directions & directions = Direction::All();
    const PathfindingNode & currentNode = _cache[currentNodeIdx];

    for ( size_t i = 0; i < directions.size(); ++i ) {
        if ( Maps::isValidDirection( currentNodeIdx, directions[i] ) ) {
            const int newIndex = currentNodeIdx + _mapOffset[i];
            if ( newIndex == pathStart )
                continue;

            const uint32_t moveCost = currentNode._cost + getMovementPenalty( currentNodeIdx, newIndex, directions[i], _pathfindingSkill );
            PathfindingNode & newNode = _cache[newIndex];
            if ( world.isValidPath( currentNodeIdx, directions[i] ) && ( newNode._from == -1 || newNode._cost > moveCost ) ) {
                const Maps::Tiles & tile = world.GetTiles( newIndex );

                newNode._from = currentNodeIdx;
                newNode._cost = moveCost;
                newNode._objectID = tile.GetObject();

                // duplicates are allowed if we find a cheaper way there
                if ( tile.isWater() == fromWater )
                    nodesToExplore.push_back( newIndex );
            }
        }
    }
}

void PlayerWorldPathfinder::reset()
{
    WorldPathfinder::checkWorldSize();

    if ( _pathStart != -1 ) {
        _pathStart = -1;
        _pathfindingSkill = Skill::Level::EXPERT;
    }
}

void PlayerWorldPathfinder::reEvaluateIfNeeded( const Heroes & hero )
{
    const int startIndex = hero.GetIndex();
    const uint32_t skill = hero.GetLevelSkill( Skill::Secondary::PATHFINDING );

    if ( _pathStart != startIndex || _pathfindingSkill != skill ) {
        _pathStart = startIndex;
        _pathfindingSkill = skill;

        processWorldMap( startIndex );
    }
}

std::list<Route::Step> PlayerWorldPathfinder::buildPath( int targetIndex ) const
{
    std::list<Route::Step> path;

    // trace the path from end point
    int currentNode = targetIndex;
    while ( currentNode != _pathStart && currentNode != -1 ) {
        const PathfindingNode & node = _cache[currentNode];
        const uint32_t cost = ( node._from != -1 ) ? node._cost - _cache[node._from]._cost : node._cost;

        path.emplace_front( currentNode, node._from, Maps::GetDirection( node._from, currentNode ), cost );

        // Sanity check
        if ( node._from != -1 && _cache[node._from]._from == currentNode ) {
            DEBUG( DBG_GAME, DBG_WARN, "Circular path found! " << node._from << " to " << currentNode );
            break;
        }
        else {
            currentNode = node._from;
        }
    }

    return path;
}

// Follows regular (for user's interface) passability rules
void PlayerWorldPathfinder::processCurrentNode( std::vector<int> & nodesToExplore, int pathStart, int currentNodeIdx, bool fromWater )
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

void AIWorldPathfinder::reset()
{
    WorldPathfinder::checkWorldSize();

    if ( _pathStart != -1 ) {
        _pathStart = -1;
        _currentColor = Color::NONE;
        _armyStrength = -1;
        _pathfindingSkill = Skill::Level::EXPERT;
    }
}

void AIWorldPathfinder::reEvaluateIfNeeded( const Heroes & hero )
{
    reEvaluateIfNeeded( hero.GetIndex(), hero.GetColor(), hero.GetArmy().GetStrength(), hero.GetLevelSkill( Skill::Secondary::PATHFINDING ) );
}

void AIWorldPathfinder::reEvaluateIfNeeded( int start, int color, double armyStrength, uint8_t skill )
{
    if ( _pathStart != start || _currentColor != color || _armyStrength != armyStrength || _pathfindingSkill != skill ) {
        _pathStart = start;
        _currentColor = color;
        _armyStrength = armyStrength;
        _pathfindingSkill = skill;

        processWorldMap( start );
    }
}

// Overwrites base version in WorldPathfinder, using custom node passability rules
void AIWorldPathfinder::processCurrentNode( std::vector<int> & nodesToExplore, int pathStart, int currentNodeIdx, bool fromWater )
{
    const bool isFirstNode = currentNodeIdx == pathStart;
    PathfindingNode & currentNode = _cache[currentNodeIdx];

    // find out if current node is protected by a strong army
    auto protectionCheck = [this]( const int index ) {
        const Maps::Tiles & tile = world.GetTiles( index );
        if ( MP2::isProtectedObject( tile.GetObject() ) ) {
            _temporaryArmy.setFromTile( tile );
            return _temporaryArmy.GetStrength() * _advantage > _armyStrength;
        }
        return false;
    };

    bool isProtected = protectionCheck( currentNodeIdx );
    if ( !isProtected ) {
        const MapsIndexes & monsters = Maps::GetTilesUnderProtection( currentNodeIdx );
        for ( auto it = monsters.begin(); it != monsters.end(); ++it ) {
            if ( protectionCheck( *it ) ) {
                isProtected = true;
                break;
            }
        }
    }

    // if we can't move here, reset
    if ( isProtected )
        currentNode.resetNode();

    // always allow move from the starting spot to cover edge case if got there before tile became blocked/protected
    if ( isFirstNode || ( !isProtected && !isTileBlockedForArmy( currentNodeIdx, _currentColor, _armyStrength, fromWater ) ) ) {
        const MapsIndexes & teleporters = world.GetTeleportEndPoints( currentNodeIdx );

        // do not check adjacent if we're going through the teleport in the middle of the path
        if ( isFirstNode || teleporters.empty() || std::find( teleporters.begin(), teleporters.end(), currentNode._from ) != teleporters.end() ) {
            checkAdjacentNodes( nodesToExplore, pathStart, currentNodeIdx, fromWater );
        }

        // special case: move through teleporters
        for ( const int teleportIdx : teleporters ) {
            if ( teleportIdx == pathStart )
                continue;

            PathfindingNode & teleportNode = _cache[teleportIdx];

            // check if move is actually faster through teleporter
            if ( teleportNode._from == -1 || teleportNode._cost > currentNode._cost ) {
                teleportNode._from = currentNodeIdx;
                teleportNode._cost = currentNode._cost;
                teleportNode._objectID = MP2::OBJ_STONELITHS;
                nodesToExplore.push_back( teleportIdx );
            }
        }
    }
}

int AIWorldPathfinder::getFogDiscoveryTile( const Heroes & hero )
{
    // paths have to be pre-calculated to find a spot where we're able to move
    reEvaluateIfNeeded( hero );
    const int start = hero.GetIndex();

    const Directions & directions = Direction::All();
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

                if ( world.GetTiles( newIndex ).isFog( _currentColor ) ) {
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

std::vector<IndexObject> AIWorldPathfinder::getObjectsOnTheWay( int targetIndex, bool checkAdjacent )
{
    std::vector<IndexObject> result;
    // validate that path can be created
    if ( _pathStart == -1 || _currentColor == Color::NONE || targetIndex == -1 || _cache[targetIndex]._cost == 0 )
        return result;

    const Kingdom & kingdom = world.GetKingdom( _currentColor );
    const Directions & directions = Direction::All();

    std::set<int> uniqueIndicies;
    auto validateAndAdd = [&kingdom, &result, &uniqueIndicies]( int index, int object ) {
        // std::set insert returns a pair, second value is true if it was unique
        if ( uniqueIndicies.insert( index ).second && kingdom.isValidKingdomObject( world.GetTiles( index ), object ) ) {
            result.emplace_back( index, object );
        }
    };

    // skip the target itself to make sure we don't double count
    uniqueIndicies.insert( targetIndex );

    // trace the path from end point
    int currentNode = targetIndex;
    while ( currentNode != _pathStart && currentNode != -1 ) {
        const PathfindingNode & node = _cache[currentNode];

        validateAndAdd( currentNode, node._objectID );

        if ( checkAdjacent ) {
            for ( size_t i = 0; i < directions.size(); ++i ) {
                if ( Maps::isValidDirection( currentNode, directions[i] ) ) {
                    const int newIndex = currentNode + _mapOffset[i];
                    const PathfindingNode & adjacent = _cache[newIndex];

                    if ( adjacent._cost == 0 || adjacent._objectID == 0 )
                        continue;

                    validateAndAdd( newIndex, adjacent._objectID );
                }
            }
        }

        // Sanity check
        if ( node._from != -1 && _cache[node._from]._from == currentNode ) {
            DEBUG( DBG_GAME, DBG_WARN, "Circular path found! " << node._from << " to " << currentNode );
            break;
        }

        currentNode = node._from;
    }

    return result;
}

std::list<Route::Step> AIWorldPathfinder::buildPath( int targetIndex ) const
{
    std::list<Route::Step> path;
    if ( _pathStart == -1 )
        return path;

    const bool fromWater = world.GetTiles( _pathStart ).isWater();

    // trace the path from end point
    int lastValidNode = targetIndex;
    int currentNode = targetIndex;
    while ( currentNode != _pathStart && currentNode != -1 ) {
        if ( world.isTileBlocked( currentNode, fromWater ) ) {
            lastValidNode = currentNode;
        }

        const PathfindingNode & node = _cache[currentNode];
        const uint32_t cost = ( node._from != -1 ) ? node._cost - _cache[node._from]._cost : node._cost;

        path.emplace_front( currentNode, node._from, Maps::GetDirection( node._from, currentNode ), cost );

        // Sanity check
        if ( node._from != -1 && _cache[node._from]._from == currentNode ) {
            DEBUG( DBG_GAME, DBG_WARN, "Circular path found! " << node._from << " to " << currentNode );
            break;
        }
        else {
            currentNode = node._from;
        }
    }

    // Cut the path to the last valid tile
    if ( lastValidNode != targetIndex ) {
        path.erase( std::find_if( path.begin(), path.end(), [&lastValidNode]( const Route::Step & step ) { return step.GetFrom() == lastValidNode; } ), path.end() );
    }

    return path;
}

uint32_t AIWorldPathfinder::getDistance( const Heroes & hero, int targetIndex )
{
    reEvaluateIfNeeded( hero );
    return _cache[targetIndex]._cost;
}

uint32_t AIWorldPathfinder::getDistance( int start, int targetIndex, int color, double armyStrength, uint8_t skill )
{
    reEvaluateIfNeeded( start, color, armyStrength, skill );
    return _cache[targetIndex]._cost;
}
