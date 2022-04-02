/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2020 - 2022                                             *
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

#include <cassert>
#include <cmath>
#include <set>
#include <tuple>

#include "ground.h"
#include "logging.h"
#include "rand.h"
#include "world.h"
#include "world_pathfinding.h"

namespace
{
    bool isTileBlocked( int tileIndex, bool fromWater )
    {
        const Maps::Tiles & tile = world.GetTiles( tileIndex );
        const bool toWater = tile.isWater();
        const MP2::MapObjectType objectType = tile.GetObject();

        if ( objectType == MP2::OBJ_HEROES || objectType == MP2::OBJ_MONSTER || objectType == MP2::OBJ_BOAT )
            return true;

        if ( MP2::isPickupObject( objectType ) || MP2::isActionObject( objectType, fromWater ) )
            return true;

        if ( fromWater && !toWater && objectType == MP2::OBJ_COAST )
            return true;

        return false;
    }

    bool isTileBlockedForAIWithArmy( int tileIndex, int color, double armyStrength )
    {
        const Maps::Tiles & tile = world.GetTiles( tileIndex );
        const MP2::MapObjectType objectType = tile.GetObject();

        // Special cases: check if we can defeat the Hero/Monster and pass through
        if ( objectType == MP2::OBJ_HEROES ) {
            const Heroes * otherHero = tile.GetHeroes();
            if ( otherHero ) {
                if ( otherHero->isFriends( color ) ) {
                    return true;
                }

                return otherHero->GetArmy().GetStrength() > armyStrength;
            }
        }

        if ( objectType == MP2::OBJ_MONSTER || ( objectType == MP2::OBJ_ARTIFACT && tile.QuantityVariant() > 5 ) )
            return Army( tile ).GetStrength() > armyStrength;

        // check if AI has the key for the barrier
        if ( objectType == MP2::OBJ_BARRIER && world.GetKingdom( color ).IsVisitTravelersTent( tile.QuantityColor() ) )
            return false;

        // AI can use boats to overcome water obstacles
        if ( objectType == MP2::OBJ_BOAT )
            return false;

        // if none of the special cases apply, check if tile can be moved on
        return MP2::isNeedStayFront( objectType );
    }

    bool isValidPath( const int index, const int direction, const int heroColor )
    {
        const Maps::Tiles & fromTile = world.GetTiles( index );
        const bool fromWater = fromTile.isWater();

        // check corner water/coast
        if ( fromWater ) {
            const int mapWidth = world.w();
            switch ( direction ) {
            case Direction::TOP_LEFT: {
                assert( index >= mapWidth + 1 );
                if ( world.GetTiles( index - mapWidth - 1 ).isWater() && ( !world.GetTiles( index - 1 ).isWater() || !world.GetTiles( index - mapWidth ).isWater() ) ) {
                    // Cannot sail through the corner of land.
                    return false;
                }

                break;
            }
            case Direction::TOP_RIGHT: {
                assert( index >= mapWidth && index + 1 < mapWidth * world.h() );
                if ( world.GetTiles( index - mapWidth + 1 ).isWater() && ( !world.GetTiles( index + 1 ).isWater() || !world.GetTiles( index - mapWidth ).isWater() ) ) {
                    // Cannot sail through the corner of land.
                    return false;
                }

                break;
            }
            case Direction::BOTTOM_RIGHT: {
                assert( index + mapWidth + 1 < mapWidth * world.h() );
                if ( world.GetTiles( index + mapWidth + 1 ).isWater() && ( !world.GetTiles( index + 1 ).isWater() || !world.GetTiles( index + mapWidth ).isWater() ) ) {
                    // Cannot sail through the corner of land.
                    return false;
                }

                break;
            }
            case Direction::BOTTOM_LEFT: {
                assert( index >= 1 && index + mapWidth - 1 < mapWidth * world.h() );
                if ( world.GetTiles( index + mapWidth - 1 ).isWater() && ( !world.GetTiles( index - 1 ).isWater() || !world.GetTiles( index + mapWidth ).isWater() ) ) {
                    // Cannot sail through the corner of land.
                    return false;
                }

                break;
            }
            default:
                break;
            }
        }

        if ( !fromTile.isPassableTo( direction ) ) {
            return false;
        }

        const Maps::Tiles & toTile = world.GetTiles( Maps::GetDirectionIndex( index, direction ) );
        return toTile.isPassableFrom( Direction::Reflect( direction ), fromWater, false, heroColor );
    }
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

uint32_t WorldPathfinder::calculatePathPenalty( const std::list<Route::Step> & path )
{
    uint32_t dist = 0;
    for ( const Route::Step & step : path ) {
        dist += step.GetPenalty();
    }
    return dist;
}

uint32_t WorldPathfinder::getMovementPenalty( int src, int dst, int direction ) const
{
    const Maps::Tiles & srcTile = world.GetTiles( src );
    const Maps::Tiles & dstTile = world.GetTiles( dst );

    uint32_t penalty = srcTile.isRoad() && dstTile.isRoad() ? Maps::Ground::roadPenalty : Maps::Ground::GetPenalty( srcTile, _pathfindingSkill );

    // Diagonal movement costs 50% more
    if ( Direction::isDiagonal( direction ) ) {
        penalty = penalty * 3 / 2;
    }

    // If we perform pathfinding for a real hero on the map, we have to work out the "last move"
    // logic: if this move is the last one on the current turn, then we can move to any adjacent
    // tile (both in straight and diagonal direction) as long as we have enough movement points
    // to move over our current tile in the straight direction
    if ( _maxMovePoints > 0 ) {
        const WorldNode & node = _cache[src];

        // No dead ends allowed
        assert( src == _pathStart || node._from != -1 );

        const uint32_t remainingMovePoints = node._remainingMovePoints;
        const uint32_t srcTilePenalty = srcTile.isRoad() ? Maps::Ground::roadPenalty : Maps::Ground::GetPenalty( srcTile, _pathfindingSkill );

        // If we still have enough movement points to move over the src tile in the straight
        // direction, but not enough to move to the dst tile, then the "last move" logic is
        // applied and we can move to the dst tile anyway at the expense of all the remaining
        // movement points
        if ( remainingMovePoints >= srcTilePenalty && remainingMovePoints < penalty ) {
            return remainingMovePoints;
        }
    }

    return penalty;
}

uint32_t WorldPathfinder::substractMovePoints( const uint32_t movePoints, const uint32_t substractedMovePoints ) const
{
    // We do not perform pathfinding for a real hero on the map, this is no-op
    if ( _maxMovePoints == 0 ) {
        return 0;
    }

    // This movement takes place at the beginning of a new turn: start with max movement points,
    // don't carry leftovers from the previous turn
    if ( movePoints < substractedMovePoints ) {
        assert( _maxMovePoints >= substractedMovePoints );

        return _maxMovePoints - substractedMovePoints;
    }

    // This movement takes place on the same turn
    return movePoints - substractedMovePoints;
}

void WorldPathfinder::processWorldMap()
{
    // reset cache back to default value
    for ( size_t idx = 0; idx < _cache.size(); ++idx ) {
        _cache[idx].resetNode();
    }
    _cache[_pathStart] = WorldNode( -1, 0, MP2::MapObjectType::OBJ_ZERO, _remainingMovePoints );

    std::vector<int> nodesToExplore;
    nodesToExplore.push_back( _pathStart );

    for ( size_t lastProcessedNode = 0; lastProcessedNode < nodesToExplore.size(); ++lastProcessedNode ) {
        processCurrentNode( nodesToExplore, nodesToExplore[lastProcessedNode] );
    }
}

void WorldPathfinder::checkAdjacentNodes( std::vector<int> & nodesToExplore, int currentNodeIdx )
{
    const Directions & directions = Direction::All();
    const WorldNode & currentNode = _cache[currentNodeIdx];

    for ( size_t i = 0; i < directions.size(); ++i ) {
        if ( Maps::isValidDirection( currentNodeIdx, directions[i] ) ) {
            const int newIndex = currentNodeIdx + _mapOffset[i];
            if ( newIndex == _pathStart )
                continue;

            const uint32_t movementPenalty = getMovementPenalty( currentNodeIdx, newIndex, directions[i] );
            const uint32_t moveCost = currentNode._cost + movementPenalty;
            const uint32_t remainingMovePoints = substractMovePoints( currentNode._remainingMovePoints, movementPenalty );

            WorldNode & newNode = _cache[newIndex];

            if ( isValidPath( currentNodeIdx, directions[i], _currentColor ) && ( newNode._from == -1 || newNode._cost > moveCost ) ) {
                const Maps::Tiles & tile = world.GetTiles( newIndex );

                newNode._from = currentNodeIdx;
                newNode._cost = moveCost;
                newNode._objectID = tile.GetObject();
                newNode._remainingMovePoints = remainingMovePoints;

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
        _currentColor = Color::NONE;
        _remainingMovePoints = 0;
        _maxMovePoints = 0;
    }
}

void PlayerWorldPathfinder::reEvaluateIfNeeded( const Heroes & hero )
{
    auto currentSettings = std::forward_as_tuple( _pathStart, _pathfindingSkill, _currentColor, _remainingMovePoints, _maxMovePoints );
    const auto newSettings = std::make_tuple( hero.GetIndex(), static_cast<uint8_t>( hero.GetLevelSkill( Skill::Secondary::PATHFINDING ) ), hero.GetColor(),
                                              hero.GetMovePoints(), hero.GetMaxMovePoints() );

    if ( currentSettings != newSettings ) {
        currentSettings = newSettings;

        processWorldMap();
    }
}

std::list<Route::Step> PlayerWorldPathfinder::buildPath( const int targetIndex ) const
{
    assert( _pathStart != -1 && targetIndex != -1 );

    std::list<Route::Step> path;

    // Destination is not reachable
    if ( _cache[targetIndex]._cost == 0 ) {
        return path;
    }

#ifndef NDEBUG
    std::set<int> uniqPathIndexes;
#endif

    int currentNode = targetIndex;

    while ( currentNode != _pathStart ) {
        assert( currentNode != -1 );

        const WorldNode & node = _cache[currentNode];

        assert( node._from != -1 );

        const uint32_t cost = node._cost - _cache[node._from]._cost;

        path.emplace_front( currentNode, node._from, Maps::GetDirection( node._from, currentNode ), cost );

        // The path should not pass through the same tile more than once
        assert( uniqPathIndexes.insert( node._from ).second );

        currentNode = node._from;
    }

    return path;
}

// Follows regular (for user's interface) passability rules
void PlayerWorldPathfinder::processCurrentNode( std::vector<int> & nodesToExplore, int currentNodeIdx )
{
    const bool isFirstNode = currentNodeIdx == _pathStart;

    if ( !isFirstNode && isTileBlocked( currentNodeIdx, world.GetTiles( _pathStart ).isWater() ) ) {
        return;
    }

    const MapsIndexes & monsters = Maps::getMonstersProtectingTile( currentNodeIdx );

    // check if current tile is protected, can move only to adjacent monster
    if ( !isFirstNode && !monsters.empty() ) {
        for ( int monsterIndex : monsters ) {
            const int direction = Maps::GetDirection( currentNodeIdx, monsterIndex );

            if ( direction != Direction::UNKNOWN && direction != Direction::CENTER && isValidPath( currentNodeIdx, direction, _currentColor ) ) {
                // add straight to cache, can't move further from the monster
                const uint32_t movementPenalty = getMovementPenalty( currentNodeIdx, monsterIndex, direction );
                const uint32_t moveCost = _cache[currentNodeIdx]._cost + movementPenalty;
                const uint32_t remainingMovePoints = substractMovePoints( _cache[currentNodeIdx]._remainingMovePoints, movementPenalty );

                WorldNode & monsterNode = _cache[monsterIndex];

                if ( monsterNode._from == -1 || monsterNode._cost > moveCost ) {
                    monsterNode._from = currentNodeIdx;
                    monsterNode._cost = moveCost;
                    monsterNode._remainingMovePoints = remainingMovePoints;
                }
            }
        }
    }
    else {
        checkAdjacentNodes( nodesToExplore, currentNodeIdx );
    }
}

void AIWorldPathfinder::reset()
{
    WorldPathfinder::checkWorldSize();

    if ( _pathStart != -1 ) {
        _pathStart = -1;

        _pathfindingSkill = Skill::Level::EXPERT;
        _currentColor = Color::NONE;
        _remainingMovePoints = 0;
        _maxMovePoints = 0;

        _armyStrength = -1;
    }
}

void AIWorldPathfinder::reEvaluateIfNeeded( const Heroes & hero )
{
    auto currentSettings = std::forward_as_tuple( _pathStart, _pathfindingSkill, _currentColor, _remainingMovePoints, _maxMovePoints, _armyStrength );
    const auto newSettings = std::make_tuple( hero.GetIndex(), static_cast<uint8_t>( hero.GetLevelSkill( Skill::Secondary::PATHFINDING ) ), hero.GetColor(),
                                              hero.GetMovePoints(), hero.GetMaxMovePoints(), hero.GetArmy().GetStrength() );

    if ( currentSettings != newSettings ) {
        currentSettings = newSettings;

        processWorldMap();
    }
}

void AIWorldPathfinder::reEvaluateIfNeeded( const int start, const int color, const double armyStrength, const uint8_t skill )
{
    auto currentSettings = std::forward_as_tuple( _pathStart, _pathfindingSkill, _currentColor, _remainingMovePoints, _maxMovePoints, _armyStrength );
    const auto newSettings = std::make_tuple( start, skill, color, 0U, 0U, armyStrength );

    if ( currentSettings != newSettings ) {
        currentSettings = newSettings;

        processWorldMap();
    }
}

// Overwrites base version in WorldPathfinder, using custom node passability rules
void AIWorldPathfinder::processCurrentNode( std::vector<int> & nodesToExplore, int currentNodeIdx )
{
    const bool isFirstNode = currentNodeIdx == _pathStart;
    WorldNode & currentNode = _cache[currentNodeIdx];

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
        const MapsIndexes & monsters = Maps::getMonstersProtectingTile( currentNodeIdx );
        for ( auto it = monsters.begin(); it != monsters.end(); ++it ) {
            if ( protectionCheck( *it ) ) {
                isProtected = true;
                break;
            }
        }
    }

    // if we can't move here, reset
    if ( isProtected ) {
        currentNode.resetNode();
    }

    // always allow move from the starting spot to cover edge case if got there before tile became blocked/protected
    if ( !isFirstNode && ( isProtected || isTileBlockedForAIWithArmy( currentNodeIdx, _currentColor, _armyStrength ) ) ) {
        return;
    }

    MapsIndexes teleports;

    // we shouldn't use teleport at the starting tile
    if ( !isFirstNode ) {
        teleports = world.GetTeleportEndPoints( currentNodeIdx );

        if ( teleports.empty() ) {
            teleports = world.GetWhirlpoolEndPoints( currentNodeIdx );
        }
    }

    // do not check adjacent if we're going through the teleport in the middle of the path
    if ( teleports.empty() || std::find( teleports.begin(), teleports.end(), currentNode._from ) != teleports.end() ) {
        checkAdjacentNodes( nodesToExplore, currentNodeIdx );
    }

    // special case: move through teleports
    for ( const int teleportIdx : teleports ) {
        if ( teleportIdx == _pathStart ) {
            continue;
        }

        WorldNode & teleportNode = _cache[teleportIdx];

        // check if move is actually faster through teleport
        if ( teleportNode._from == -1 || teleportNode._cost > currentNode._cost ) {
            teleportNode._from = currentNodeIdx;
            teleportNode._cost = currentNode._cost;
            teleportNode._objectID = world.GetTiles( teleportIdx ).GetObject();
            teleportNode._remainingMovePoints = currentNode._remainingMovePoints;

            nodesToExplore.push_back( teleportIdx );
        }
    }
}

uint32_t AIWorldPathfinder::getMovementPenalty( int src, int dst, int direction ) const
{
    const uint32_t defaultPenalty = WorldPathfinder::getMovementPenalty( src, dst, direction );

    // If we perform pathfinding for a real AI-controlled hero on the map, we should encourage him
    // to overcome water obstacles using boats.
    if ( _maxMovePoints > 0 ) {
        const WorldNode & node = _cache[src];

        // No dead ends allowed
        assert( src == _pathStart || node._from != -1 );

        const Maps::Tiles & srcTile = world.GetTiles( src );
        const Maps::Tiles & dstTile = world.GetTiles( dst );

        // When the hero gets into a boat or disembarks, he spends all remaining movement points.
        if ( ( !srcTile.isWater() && dstTile.GetObject() == MP2::OBJ_BOAT ) || ( srcTile.isWater() && dstTile.GetObject() == MP2::OBJ_COAST ) ) {
            // If the hero is not able to make this movement this turn, then he will have to spend
            // all the movement points next turn.
            if ( defaultPenalty > node._remainingMovePoints ) {
                return _maxMovePoints;
            }

            return node._remainingMovePoints;
        }
    }

    return defaultPenalty;
}

int AIWorldPathfinder::getFogDiscoveryTile( const Heroes & hero )
{
    // paths have to be pre-calculated to find a spot where we're able to move
    reEvaluateIfNeeded( hero );

    const int start = hero.GetIndex();
    const int scouteValue = hero.GetScoute();

    const Directions & directions = Direction::All();
    std::vector<bool> tilesVisited( world.getSize(), false );
    std::vector<int> nodesToExplore;

    tilesVisited[start] = true;

    nodesToExplore.push_back( start );

    for ( size_t lastProcessedNode = 0; lastProcessedNode < nodesToExplore.size(); ++lastProcessedNode ) {
        const int currentNodeIdx = nodesToExplore[lastProcessedNode];

        if ( start != currentNodeIdx ) {
            int32_t maxTilesToReveal = Maps::getFogTileCountToBeRevealed( currentNodeIdx, scouteValue, _currentColor );
            if ( maxTilesToReveal > 0 ) {
                // Found a tile where we can reveal fog. Check for other tiles in the queue to find the one with the highest value.
                int bestIndex = currentNodeIdx;
                for ( ; lastProcessedNode < nodesToExplore.size(); ++lastProcessedNode ) {
                    const int nodeIdx = nodesToExplore[lastProcessedNode];
                    const int32_t tilesToReveal = Maps::getFogTileCountToBeRevealed( nodeIdx, scouteValue, _currentColor );
                    if ( maxTilesToReveal < tilesToReveal || ( maxTilesToReveal == tilesToReveal && _cache[nodeIdx]._cost < _cache[bestIndex]._cost ) ) {
                        maxTilesToReveal = tilesToReveal;
                        bestIndex = nodeIdx;
                    }
                }

                return bestIndex;
            }
        }

        for ( size_t i = 0; i < directions.size(); ++i ) {
            if ( !Maps::isValidDirection( currentNodeIdx, directions[i] ) ) {
                continue;
            }

            const int newIndex = currentNodeIdx + _mapOffset[i];

            if ( tilesVisited[newIndex] ) {
                continue;
            }

            tilesVisited[newIndex] = true;

            if ( !MP2::isSafeForFogDiscoveryObject( world.GetTiles( newIndex ).GetObject( true ) ) ) {
                continue;
            }

            // Tile is unreachable (maybe because it is guarded by too strong an army)
            if ( _cache[newIndex]._cost == 0 ) {
                continue;
            }

            nodesToExplore.push_back( newIndex );

            // If there is a teleport on this tile, we should also consider the endpoints
            MapsIndexes teleports = world.GetTeleportEndPoints( newIndex );

            if ( teleports.empty() ) {
                teleports = world.GetWhirlpoolEndPoints( newIndex );
            }

            for ( const int teleportIndex : teleports ) {
                if ( tilesVisited[teleportIndex] ) {
                    continue;
                }

                tilesVisited[teleportIndex] = true;

                // Teleport endpoint is unreachable (maybe because it is guarded by too strong an army)
                if ( _cache[teleportIndex]._cost == 0 ) {
                    continue;
                }

                nodesToExplore.push_back( teleportIndex );
            }
        }
    }

    return -1;
}

int AIWorldPathfinder::getNearestTileToMove( const Heroes & hero )
{
    // paths have to be pre-calculated to find a spot where we're able to move
    reEvaluateIfNeeded( hero );

    const int start = hero.GetIndex();

    Directions directions = Direction::All();
    // We have to shuffle directions to avoid cases when heroes repeat the same steps again and again.
    Rand::Shuffle( directions );

    for ( size_t i = 0; i < directions.size(); ++i ) {
        if ( !Maps::isValidDirection( start, directions[i] ) ) {
            continue;
        }

        const int newIndex = Maps::GetDirectionIndex( start, directions[i] );
        if ( newIndex == start ) {
            continue;
        }

        // Don't go onto action objects as they might be castles or dwellings with guards.
        if ( MP2::isActionObject( world.GetTiles( newIndex ).GetObject( true ) ) ) {
            continue;
        }

        // Tile is reachable and the hero has enough army to defeat potential guards
        if ( _cache[newIndex]._cost > 0 ) {
            return newIndex;
        }
    }

    return -1;
}

bool AIWorldPathfinder::isHeroPossiblyBlockingWay( const Heroes & hero )
{
    // paths have to be pre-calculated to find a spot where we're able to move
    reEvaluateIfNeeded( hero );

    const int32_t start = hero.GetIndex();

    const bool leftSideUnreachable = !Maps::isValidDirection( start, Direction::LEFT ) || _cache[start - 1]._cost == 0;
    const bool rightSideUnreachable = !Maps::isValidDirection( start, Direction::RIGHT ) || _cache[start + 1]._cost == 0;
    if ( leftSideUnreachable && rightSideUnreachable ) {
        return true;
    }

    const bool topSideUnreachable = !Maps::isValidDirection( start, Direction::TOP ) || _cache[start - world.w()]._cost == 0;
    const bool bottomSideUnreachable = !Maps::isValidDirection( start, Direction::BOTTOM ) || _cache[start + world.w()]._cost == 0;
    if ( topSideUnreachable && bottomSideUnreachable ) {
        return true;
    }

    // Is the hero standing on Stoneliths?
    return world.GetTiles( start ).GetObject( false ) == MP2::OBJ_STONELITHS;
}

std::vector<IndexObject> AIWorldPathfinder::getObjectsOnTheWay( const int targetIndex, const bool checkAdjacent /* = false */ ) const
{
    assert( _pathStart != -1 && _currentColor != Color::NONE && targetIndex != -1 );

    std::vector<IndexObject> result;

    // Destination is not reachable
    if ( _cache[targetIndex]._cost == 0 ) {
        return result;
    }

    const Kingdom & kingdom = world.GetKingdom( _currentColor );
    const Directions & directions = Direction::All();

    std::set<int> uniqueIndicies;
    auto validateAndAdd = [&kingdom, &result, &uniqueIndicies]( int index, const MP2::MapObjectType objectType ) {
        // std::set insert returns a pair, second value is true if it was unique
        if ( uniqueIndicies.insert( index ).second && kingdom.isValidKingdomObject( world.GetTiles( index ), objectType ) ) {
            result.emplace_back( index, objectType );
        }
    };

    // skip the target itself to make sure we don't double count
    uniqueIndicies.insert( targetIndex );

#ifndef NDEBUG
    std::set<int> uniqPathIndexes;
#endif

    int currentNode = targetIndex;

    while ( currentNode != _pathStart ) {
        assert( currentNode != -1 );

        const WorldNode & node = _cache[currentNode];

        assert( node._from != -1 );

        validateAndAdd( currentNode, node._objectID );

        if ( checkAdjacent ) {
            for ( size_t i = 0; i < directions.size(); ++i ) {
                if ( Maps::isValidDirection( currentNode, directions[i] ) ) {
                    const int newIndex = currentNode + _mapOffset[i];
                    const WorldNode & adjacent = _cache[newIndex];

                    if ( adjacent._cost == 0 || adjacent._objectID == 0 )
                        continue;

                    validateAndAdd( newIndex, adjacent._objectID );
                }
            }
        }

        // The path should not pass through the same tile more than once
        assert( uniqPathIndexes.insert( node._from ).second );

        currentNode = node._from;
    }

    return result;
}

std::list<Route::Step> AIWorldPathfinder::getDimensionDoorPath( const Heroes & hero, int targetIndex ) const
{
    std::list<Route::Step> path;

    const Spell dimensionDoor( Spell::DIMENSIONDOOR );
    if ( !hero.HaveSpell( dimensionDoor ) || !Maps::isValidAbsIndex( targetIndex ) )
        return path;

    const uint32_t spCost = std::max( 1U, dimensionDoor.SpellPoint() );
    const uint32_t movementCost = std::max( 1U, dimensionDoor.MovePoint() );

    const uint32_t maxCasts = std::min( hero.GetSpellPoints() / spCost, hero.GetMovePoints() / movementCost );
    if ( maxCasts == 0 )
        return path;

    if ( world.GetTiles( targetIndex ).GetObject( false ) == MP2::OBJ_CASTLE ) {
        targetIndex = Maps::GetDirectionIndex( targetIndex, Direction::BOTTOM );
        if ( !Maps::isValidAbsIndex( targetIndex ) )
            return path;
    }

    const fheroes2::Point startPoint = Maps::GetPoint( hero.GetIndex() );
    const fheroes2::Point targetPoint = Maps::GetPoint( targetIndex );

    fheroes2::Point current = startPoint;
    fheroes2::Point difference = targetPoint - startPoint;

    const bool water = hero.isShipMaster();
    const Directions & directions = Direction::All();
    const int32_t distanceLimit = Spell::CalculateDimensionDoorDistance() / 2;

    uint32_t spellsUsed = 1; // start with 1 to avoid spending ALL move/spell points
    while ( maxCasts > spellsUsed ) {
        const int32_t currentNodeIdx = Maps::GetIndexFromAbsPoint( current );
        fheroes2::Point another = current;
        another.x += ( difference.x > 0 ) ? std::min( difference.x, distanceLimit ) : std::max( difference.x, -distanceLimit );
        another.y += ( difference.y > 0 ) ? std::min( difference.y, distanceLimit ) : std::max( difference.y, -distanceLimit );

        const int32_t anotherNodeIdx = Maps::GetIndexFromAbsPoint( another );
        bool found = Maps::isValidForDimensionDoor( anotherNodeIdx, water );

        if ( !found ) {
            for ( size_t i = 0; i < directions.size(); ++i ) {
                if ( !Maps::isValidDirection( anotherNodeIdx, directions[i] ) )
                    continue;

                const int newIndex = anotherNodeIdx + _mapOffset[i];
                const fheroes2::Point newPoint = Maps::GetPoint( newIndex );
                if ( std::abs( current.x - newPoint.x ) <= distanceLimit && std::abs( current.y - newPoint.y ) <= distanceLimit
                     && Maps::isValidForDimensionDoor( newIndex, water ) ) {
                    path.emplace_back( newIndex, currentNodeIdx, Direction::CENTER, movementCost );
                    current = newPoint;
                    found = true;
                    break;
                }
            }

            if ( !found )
                return std::list<Route::Step>();
        }
        else {
            path.emplace_back( anotherNodeIdx, currentNodeIdx, Direction::CENTER, movementCost );
            current = another;
        }

        ++spellsUsed;

        difference = targetPoint - current;
        if ( std::abs( difference.x ) <= 1 && std::abs( difference.y ) <= 1 ) {
            return path;
        }
    }

    return std::list<Route::Step>();
}

std::list<Route::Step> AIWorldPathfinder::buildPath( const int targetIndex, const bool isPlanningMode /* = false */ ) const
{
    assert( _pathStart != -1 && targetIndex != -1 );

    std::list<Route::Step> path;

    // Destination is not reachable
    if ( _cache[targetIndex]._cost == 0 ) {
        return path;
    }

    const bool fromWater = world.GetTiles( _pathStart ).isWater();

#ifndef NDEBUG
    std::set<int> uniqPathIndexes;
#endif

    int lastValidNode = targetIndex;
    int currentNode = targetIndex;

    while ( currentNode != _pathStart ) {
        assert( currentNode != -1 );

        if ( isTileBlocked( currentNode, fromWater ) ) {
            lastValidNode = currentNode;
        }

        const WorldNode & node = _cache[currentNode];

        assert( node._from != -1 );

        const uint32_t cost = node._cost - _cache[node._from]._cost;

        path.emplace_front( currentNode, node._from, Maps::GetDirection( node._from, currentNode ), cost );

        // The path should not pass through the same tile more than once
        assert( uniqPathIndexes.insert( node._from ).second );

        currentNode = node._from;
    }

    // Cut the path to the last valid tile/obstacle if not in planning mode.
    if ( !isPlanningMode && lastValidNode != targetIndex ) {
        path.erase( std::find_if( path.begin(), path.end(), [lastValidNode]( const Route::Step & step ) { return step.GetFrom() == lastValidNode; } ), path.end() );
    }

    return path;
}

uint32_t AIWorldPathfinder::getDistance( int start, int targetIndex, int color, double armyStrength, uint8_t skill )
{
    reEvaluateIfNeeded( start, color, armyStrength, skill );

    return _cache[targetIndex]._cost;
}

void AIWorldPathfinder::setArmyStrengthMultiplier( const double multiplier )
{
    if ( multiplier > 0 && std::fabs( _advantage - multiplier ) > 0.001 ) {
        _advantage = multiplier;
        reset();
    }
}
