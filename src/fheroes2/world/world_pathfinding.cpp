/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2023                                             *
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

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <set>
#include <tuple>
#include <type_traits>
#include <utility>

#include "army.h"
#include "artifact.h"
#include "castle.h"
#include "direction.h"
#include "game_over.h"
#include "ground.h"
#include "heroes.h"
#include "kingdom.h"
#include "maps.h"
#include "maps_tiles.h"
#include "math_base.h"
#include "pairs.h"
#include "rand.h"
#include "route.h"
#include "settings.h"
#include "spell.h"
#include "spell_info.h"
#include "world.h"

namespace
{
    bool isFindArtifactVictoryConditionForHuman( const Artifact & art )
    {
        assert( art.isValid() );

        const Settings & conf = Settings::Get();

        if ( ( conf.ConditionWins() & GameOver::WINS_ARTIFACT ) == 0 ) {
            return false;
        }

        if ( conf.WinsFindUltimateArtifact() ) {
            return art.isUltimate();
        }

        return ( art.GetID() == conf.WinsFindArtifactID() );
    }

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

    bool isTileBlockedForAIWithArmy( const int tileIndex, const int color, const double armyStrength, const bool isArtifactBagFull )
    {
        const Maps::Tiles & tile = world.GetTiles( tileIndex );
        const MP2::MapObjectType objectType = tile.GetObject();

        // Special cases: check if we can defeat the Hero/Monster and pass through
        if ( objectType == MP2::OBJ_HEROES ) {
            const Heroes * otherHero = tile.GetHeroes();
            assert( otherHero != nullptr );

            if ( otherHero->isFriends( color ) ) {
                return true;
            }

            // WINS_HERO victory condition does not apply to AI-controlled players, we have to keep this hero alive for the human player
            if ( otherHero == world.GetHeroesCondWins() ) {
                return true;
            }

            return otherHero->GetArmy().GetStrength() > armyStrength;
        }

        if ( MP2::isArtifactObject( objectType ) ) {
            const Artifact art = Maps::getArtifactFromTile( tile );
            if ( art.isValid() ) {
                if ( isFindArtifactVictoryConditionForHuman( art ) ) {
                    // WINS_ARTIFACT victory condition does not apply to AI-controlled players, we should leave this artifact untouched for the human player.
                    return true;
                }

                if ( isArtifactBagFull && MP2::isPickupObject( objectType ) ) {
                    // A hero cannot pickup this object on his way since his artifact bag is full.
                    return true;
                }
            }
        }

        // Monster or artifact guarded by a monster
        if ( objectType == MP2::OBJ_MONSTER || ( objectType == MP2::OBJ_ARTIFACT && tile.QuantityVariant() > 5 ) )
            return Army( tile ).GetStrength() > armyStrength;

        // Check if AI has the key for the barrier
        if ( objectType == MP2::OBJ_BARRIER && world.GetKingdom( color ).IsVisitTravelersTent( tile.QuantityColor() ) )
            return false;

        // AI can use boats to overcome water obstacles
        if ( objectType == MP2::OBJ_BOAT )
            return false;

        // If none of the special cases apply, check if tile can be moved on
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

    bool isTileProtectedForAI( const int index, const double armyStrength, const double advantage )
    {
        const Maps::Tiles & tile = world.GetTiles( index );

        if ( MP2::isProtectedObject( tile.GetObject() ) ) {
            // creating an Army instance is a relatively heavy operation, so cache it to speed up calculations
            static Army tileArmy;

            tileArmy.setFromTile( tile );

            return tileArmy.GetStrength() * advantage > armyStrength;
        }

        return false;
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

uint32_t WorldPathfinder::subtractMovePoints( const uint32_t movePoints, const uint32_t subtractedMovePoints ) const
{
    // We do not perform pathfinding for a real hero on the map, this is no-op
    if ( _maxMovePoints == 0 ) {
        return 0;
    }

    // This movement takes place at the beginning of a new turn: start with max movement points,
    // don't carry leftovers from the previous turn
    if ( movePoints < subtractedMovePoints ) {
        assert( _maxMovePoints >= subtractedMovePoints );

        return _maxMovePoints - subtractedMovePoints;
    }

    // This movement takes place on the same turn
    return movePoints - subtractedMovePoints;
}

void WorldPathfinder::processWorldMap()
{
    // reset cache back to default value
    for ( size_t idx = 0; idx < _cache.size(); ++idx ) {
        _cache[idx].resetNode();
    }
    _cache[_pathStart] = WorldNode( -1, 0, MP2::OBJ_NONE, _remainingMovePoints );

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
        if ( !Maps::isValidDirection( currentNodeIdx, directions[i] ) || !isValidPath( currentNodeIdx, directions[i], _currentColor ) ) {
            continue;
        }

        const int newIndex = currentNodeIdx + _mapOffset[i];
        if ( newIndex == _pathStart ) {
            continue;
        }

        const uint32_t movementPenalty = getMovementPenalty( currentNodeIdx, newIndex, directions[i] );
        const uint32_t movementCost = currentNode._cost + movementPenalty;

        WorldNode & newNode = _cache[newIndex];

        if ( newNode._from == -1 || newNode._cost > movementCost ) {
            const Maps::Tiles & newTile = world.GetTiles( newIndex );

            newNode._from = currentNodeIdx;
            newNode._cost = movementCost;
            newNode._objectID = newTile.GetObject();
            newNode._remainingMovePoints = subtractMovePoints( currentNode._remainingMovePoints, movementPenalty );

            nodesToExplore.push_back( newIndex );
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
    auto currentSettings = std::tie( _pathStart, _pathfindingSkill, _currentColor, _remainingMovePoints, _maxMovePoints );
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

void PlayerWorldPathfinder::processCurrentNode( std::vector<int> & nodesToExplore, const int currentNodeIdx )
{
    const bool isFirstNode = currentNodeIdx == _pathStart;
    const WorldNode & currentNode = _cache[currentNodeIdx];

    if ( !isFirstNode && isTileBlocked( currentNodeIdx, world.GetTiles( _pathStart ).isWater() ) ) {
        return;
    }

    const MapsIndexes & monsters = Maps::getMonstersProtectingTile( currentNodeIdx );

    // If the current tile is protected, then the hero can only move to one of the neighboring monsters
    if ( !isFirstNode && !monsters.empty() ) {
        for ( int monsterIndex : monsters ) {
            const int direction = Maps::GetDirection( currentNodeIdx, monsterIndex );

            if ( direction == Direction::UNKNOWN || direction == Direction::CENTER || !isValidPath( currentNodeIdx, direction, _currentColor ) ) {
                continue;
            }

            const uint32_t movementPenalty = getMovementPenalty( currentNodeIdx, monsterIndex, direction );
            const uint32_t movementCost = currentNode._cost + movementPenalty;

            WorldNode & monsterNode = _cache[monsterIndex];

            if ( monsterNode._from == -1 || monsterNode._cost > movementCost ) {
                const Maps::Tiles & monsterTile = world.GetTiles( monsterIndex );

                monsterNode._from = currentNodeIdx;
                monsterNode._cost = movementCost;
                monsterNode._objectID = monsterTile.GetObject();
                monsterNode._remainingMovePoints = subtractMovePoints( currentNode._remainingMovePoints, movementPenalty );
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
        _hero = nullptr;
        _isArtifactBagFull = false;
    }
}

void AIWorldPathfinder::reEvaluateIfNeeded( const Heroes & hero )
{
    auto currentSettings = std::tie( _pathStart, _pathfindingSkill, _currentColor, _remainingMovePoints, _maxMovePoints, _hero, _armyStrength, _isArtifactBagFull );
    const auto newSettings = std::make_tuple( hero.GetIndex(), static_cast<uint8_t>( hero.GetLevelSkill( Skill::Secondary::PATHFINDING ) ), hero.GetColor(),
                                              hero.GetMovePoints(), hero.GetMaxMovePoints(), &hero, hero.GetArmy().GetStrength(), hero.GetBagArtifacts().isFull() );

    if ( currentSettings != newSettings ) {
        currentSettings = newSettings;

        processWorldMap();
    }
}

void AIWorldPathfinder::reEvaluateIfNeeded( const int start, const int color, const double armyStrength, const uint8_t skill )
{
    auto currentSettings = std::tie( _pathStart, _pathfindingSkill, _currentColor, _remainingMovePoints, _maxMovePoints, _hero, _armyStrength, _isArtifactBagFull );
    const auto newSettings = std::make_tuple( start, skill, color, 0U, 0U, nullptr, armyStrength, false );

    if ( currentSettings != newSettings ) {
        currentSettings = newSettings;

        processWorldMap();
    }
}

void AIWorldPathfinder::processWorldMap()
{
    // reset cache back to default value
    for ( auto & node : _cache ) {
        node.resetNode();
    }
    _cache[_pathStart] = WorldNode( -1, 0, MP2::OBJ_NONE, _remainingMovePoints );

    std::vector<int> nodesToExplore;
    nodesToExplore.push_back( _pathStart );

    if ( _hero && !_hero->Modes( Heroes::PATROL ) && !_hero->isShipMaster() ) {
        static const Spell townGate( Spell::TOWNGATE );
        static const Spell townPortal( Spell::TOWNPORTAL );
        const uint32_t currentSpellPoints = _hero->GetSpellPoints();

        auto tryPortalToTown = [this, &nodesToExplore]( const Spell & spell, const Castle * castle ) {
            if ( !castle || castle->GetHero() )
                return;

            const int castleIndex = castle->GetIndex();
            const uint32_t movePointCost = spell.movePoints();
            const uint32_t movePointsAfter = ( _remainingMovePoints < movePointCost ) ? 0 : _remainingMovePoints - movePointCost;

            _cache[castleIndex] = WorldNode( _pathStart, movePointCost, MP2::OBJ_CASTLE, movePointsAfter );
            nodesToExplore.push_back( castleIndex );
        };

        if ( _hero->CanCastSpell( townGate ) && currentSpellPoints > townGate.spellPoints() + currentSpellPoints * _spellPointsReserved ) {
            const Castle * castle = fheroes2::getNearestCastleTownGate( *_hero );
            tryPortalToTown( townGate, castle );
        }
        if ( _hero->CanCastSpell( townPortal ) && currentSpellPoints > townPortal.spellPoints() + currentSpellPoints * _spellPointsReserved ) {
            for ( const Castle * castle : _hero->GetKingdom().GetCastles() ) {
                tryPortalToTown( townPortal, castle );
            }
        }
    }

    for ( size_t lastProcessedNode = 0; lastProcessedNode < nodesToExplore.size(); ++lastProcessedNode ) {
        processCurrentNode( nodesToExplore, nodesToExplore[lastProcessedNode] );
    }
}

void AIWorldPathfinder::processCurrentNode( std::vector<int> & nodesToExplore, const int currentNodeIdx )
{
    const bool isFirstNode = currentNodeIdx == _pathStart;
    WorldNode & currentNode = _cache[currentNodeIdx];

    // Find out if current node is protected by a strong army
    bool isProtected = isTileProtectedForAI( currentNodeIdx, _armyStrength, _advantage );
    if ( !isProtected ) {
        const MapsIndexes & monsters = Maps::getMonstersProtectingTile( currentNodeIdx );
        for ( auto it = monsters.begin(); it != monsters.end(); ++it ) {
            if ( isTileProtectedForAI( *it, _armyStrength, _advantage ) ) {
                isProtected = true;
                break;
            }
        }
    }

    // If we can't move here, reset
    if ( isProtected ) {
        currentNode.resetNode();
    }

    // Always allow move from the starting spot to cover edge case if got there before tile became blocked/protected
    if ( !isFirstNode && ( isProtected || isTileBlockedForAIWithArmy( currentNodeIdx, _currentColor, _armyStrength, _isArtifactBagFull ) ) ) {
        return;
    }

    MapsIndexes teleports;

    // We shouldn't use teleport at the starting tile
    if ( !isFirstNode ) {
        teleports = world.GetTeleportEndPoints( currentNodeIdx );

        if ( teleports.empty() ) {
            teleports = world.GetWhirlpoolEndPoints( currentNodeIdx );
        }
    }

    // Do not check adjacent if we're going through the teleport in the middle of the path
    if ( teleports.empty() || std::find( teleports.begin(), teleports.end(), currentNode._from ) != teleports.end() ) {
        checkAdjacentNodes( nodesToExplore, currentNodeIdx );
    }

    // Special case: move through teleports
    for ( const int teleportIdx : teleports ) {
        if ( teleportIdx == _pathStart ) {
            continue;
        }

        WorldNode & teleportNode = _cache[teleportIdx];

        // Check if move is actually faster through teleport
        if ( teleportNode._from == -1 || teleportNode._cost > currentNode._cost ) {
            const Maps::Tiles & teleportTile = world.GetTiles( teleportIdx );

            teleportNode._from = currentNodeIdx;
            teleportNode._cost = currentNode._cost;
            teleportNode._objectID = teleportTile.GetObject();
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

int AIWorldPathfinder::getFogDiscoveryTile( const Heroes & hero, bool & isTerritoryExpansion )
{
    isTerritoryExpansion = false;

    // paths have to be pre-calculated to find a spot where we're able to move
    reEvaluateIfNeeded( hero );

    const int start = hero.GetIndex();
    const int scoutingDistance = hero.GetScoutingDistance();

    const Directions & directions = Direction::All();
    std::vector<bool> tilesVisited( world.getSize(), false );
    std::vector<int> nodesToExplore;

    tilesVisited[start] = true;

    nodesToExplore.push_back( start );

    int bestIndex = -1;

    for ( size_t lastProcessedNode = 0; lastProcessedNode < nodesToExplore.size(); ++lastProcessedNode ) {
        const int currentNodeIdx = nodesToExplore[lastProcessedNode];

        if ( bestIndex == -1 && start != currentNodeIdx ) {
            int32_t maxTilesToReveal = Maps::getFogTileCountToBeRevealed( currentNodeIdx, scoutingDistance, _currentColor );
            if ( maxTilesToReveal > 0 ) {
                // Found a tile where we can reveal fog. Check for other tiles in the queue to find the one with the highest value.
                bestIndex = currentNodeIdx;
                for ( size_t i = lastProcessedNode + 1; i < nodesToExplore.size(); ++i ) {
                    const int nodeIdx = nodesToExplore[i];
                    const int32_t tilesToReveal = Maps::getFogTileCountToBeRevealed( nodeIdx, scoutingDistance, _currentColor );

                    if ( std::make_tuple( maxTilesToReveal, _cache[nodeIdx]._cost ) < std::make_tuple( tilesToReveal, _cache[bestIndex]._cost ) ) {
                        maxTilesToReveal = tilesToReveal;
                        bestIndex = nodeIdx;
                    }
                }
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

            for ( const int32_t tileIndex : Maps::getAroundIndexes( newIndex ) ) {
                if ( world.GetTiles( tileIndex ).isFog( _currentColor ) ) {
                    // We found a tile which has a neighboring tile covered in fog.
                    // Since the current tile is accessible for the hero, the tile covered by fog most likely is accessible too.
                    isTerritoryExpansion = true;
                    return newIndex;
                }
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

                for ( const int32_t tileIndex : Maps::getAroundIndexes( teleportIndex ) ) {
                    if ( world.GetTiles( tileIndex ).isFog( _currentColor ) ) {
                        // We found a tile which has a neighboring tile covered in fog.
                        // Since the current tile is accessible for the hero, the tile covered by fog most likely is accessible too.
                        isTerritoryExpansion = true;
                        return teleportIndex;
                    }
                }

                nodesToExplore.push_back( teleportIndex );
            }
        }
    }

    // If we reach here it means that no tiles covered by fog are really useful. Let's at least uncover something even if it's useless.
    return bestIndex;
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
    const int32_t heroIndex = hero.GetIndex();
    const int heroColor = hero.GetColor();

    auto isReachableDirection = [heroIndex, heroColor]( const int direction ) {
        if ( !Maps::isValidDirection( heroIndex, direction ) ) {
            return false;
        }

        if ( !isValidPath( heroIndex, direction, heroColor ) ) {
            return false;
        }

        return true;
    };

    const bool leftReachable = isReachableDirection( Direction::LEFT );
    const bool rightReachable = isReachableDirection( Direction::RIGHT );
    const bool topReachable = isReachableDirection( Direction::TOP );
    const bool bottomReachable = isReachableDirection( Direction::BOTTOM );
    const bool topLeftReachable = isReachableDirection( Direction::TOP_LEFT );
    const bool topRightReachable = isReachableDirection( Direction::TOP_RIGHT );
    const bool bottomLeftReachable = isReachableDirection( Direction::BOTTOM_LEFT );
    const bool bottomRightReachable = isReachableDirection( Direction::BOTTOM_RIGHT );

    // There are multiple cases when a hero might block way.
    // H - hero
    // x - unreachable tile
    // r - always reachable tile
    // o - optionally reachable tile

    // |   | r |   |
    // | x | H | x |
    // |   | r |   |
    if ( topReachable && bottomReachable && !leftReachable && !rightReachable ) {
        return true;
    }

    // |   | x |   |
    // | r | H | r |
    // |   | x |   |
    if ( leftReachable && rightReachable && !topReachable && !bottomReachable ) {
        return true;
    }

    // | x | o |   |
    // | r | H | o |
    // |   | x |   |
    if ( leftReachable && ( topReachable || rightReachable ) && !topLeftReachable && !bottomReachable ) {
        return true;
    }

    // |   | o | x |
    // | o | H | r |
    // |   | x |   |
    if ( rightReachable && ( topReachable || leftReachable ) && !topRightReachable && !bottomReachable ) {
        return true;
    }

    // |   | x |   |
    // | r | H | o |
    // | x | o |   |
    if ( leftReachable && ( bottomReachable || rightReachable ) && !topReachable && !bottomLeftReachable ) {
        return true;
    }

    // |   | x |   |
    // | o | H | r |
    // |   | o | x |
    if ( rightReachable && ( bottomReachable || leftReachable ) && !topReachable && !bottomRightReachable ) {
        return true;
    }

    // | x | o |   |
    // | r | H | o |
    // | x | o |   |
    if ( leftReachable && ( topReachable || bottomReachable || rightReachable ) && !topLeftReachable && !bottomLeftReachable ) {
        return true;
    }

    // | x | r | x |
    // | o | H | o |
    // |   | o |   |
    if ( topReachable && ( bottomReachable || rightReachable || leftReachable ) && !topLeftReachable && !topRightReachable ) {
        return true;
    }

    // |   | o | x |
    // | o | H | r |
    // |   | o | x |
    if ( rightReachable && ( topReachable || bottomReachable || leftReachable ) && !topRightReachable && !bottomRightReachable ) {
        return true;
    }

    // |   | o |   |
    // | o | H | o |
    // | x | r | x |
    if ( bottomReachable && ( topReachable || leftReachable || rightReachable ) && !bottomLeftReachable && !bottomRightReachable ) {
        return true;
    }

    // |   | r | x |
    // | x | H | o |
    // |   | o |   |
    if ( topReachable && ( rightReachable || bottomReachable ) && !topRightReachable && !leftReachable ) {
        return true;
    }

    // |   | o |   |
    // | x | H | o |
    // |   | r | x |
    if ( bottomReachable && ( rightReachable || topReachable ) && !bottomRightReachable && !leftReachable ) {
        return true;
    }

    // | x | r |   |
    // | o | H | x |
    // |   | o |   |
    if ( topReachable && ( leftReachable || bottomReachable ) && !topLeftReachable && !rightReachable ) {
        return true;
    }

    // |   | r |   |
    // | o | H | x |
    // | x | r |   |
    if ( bottomReachable && ( leftReachable || topReachable ) && !bottomLeftReachable && !rightReachable ) {
        return true;
    }

    // Is the hero standing on Stone Liths?
    return world.GetTiles( heroIndex ).GetObject( false ) == MP2::OBJ_STONE_LITHS;
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

    std::set<int> uniqueIndices;
    auto validateAndAdd = [&kingdom, &result, &uniqueIndices]( int index, const MP2::MapObjectType objectType ) {
        // std::set insert returns a pair, second value is true if it was unique
        if ( uniqueIndices.insert( index ).second && kingdom.isValidKingdomObject( world.GetTiles( index ), objectType ) ) {
            result.emplace_back( index, objectType );
        }
    };

    // skip the target itself to make sure we don't double count
    uniqueIndices.insert( targetIndex );

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
    if ( hero.GetIndex() == targetIndex ) {
        return {};
    }

    const Spell dimensionDoor( Spell::DIMENSIONDOOR );
    if ( !hero.HaveSpell( dimensionDoor ) || !Maps::isValidAbsIndex( targetIndex ) )
        return {};

    uint32_t currentSpellPoints = hero.GetSpellPoints();

    const Maps::Tiles & tile = world.GetTiles( targetIndex );
    const MP2::MapObjectType objectType = tile.GetObject( true );

    // Reserve spell points only if target isn't a well that will replenish lost SP
    if ( objectType != MP2::OBJ_MAGIC_WELL && objectType != MP2::OBJ_ARTESIAN_SPRING ) {
        if ( currentSpellPoints < hero.GetMaxSpellPoints() * _spellPointsReserved )
            return {};

        currentSpellPoints -= static_cast<uint32_t>( hero.GetMaxSpellPoints() * _spellPointsReserved );
    }

    const uint32_t movementCost = std::max( 1U, dimensionDoor.movePoints() );
    const uint32_t maxCasts = std::min( currentSpellPoints / std::max( 1U, dimensionDoor.spellPoints( &hero ) ), hero.GetMovePoints() / movementCost );

    // Have to explicitly call GetObject( false ) since hero might be standing on it
    if ( tile.GetObject( false ) == MP2::OBJ_CASTLE ) {
        targetIndex = Maps::GetDirectionIndex( targetIndex, Direction::BOTTOM );
        if ( !Maps::isValidAbsIndex( targetIndex ) )
            return {};
    }

    // The object requires to stand on it. In this case we need to check if it is protected by monsters.
    if ( !MP2::isNeedStayFront( objectType ) ) {
        const MapsIndexes & monsters = Maps::getMonstersProtectingTile( targetIndex );
        for ( const int32_t monsterIndex : monsters ) {
            if ( isTileProtectedForAI( monsterIndex, _armyStrength, _advantage ) ) {
                // The tile is protected by monsters. No reason to try to get it.
                return {};
            }
        }
    }

    const fheroes2::Point targetPoint = Maps::GetPoint( targetIndex );

    fheroes2::Point current = Maps::GetPoint( hero.GetIndex() );
    fheroes2::Point difference = targetPoint - current;

    const bool water = hero.isShipMaster();
    const Directions & directions = Direction::All();
    const int32_t distanceLimit = Spell::CalculateDimensionDoorDistance() / 2;

    std::list<Route::Step> path;

    uint32_t spellsUsed = 0;
    while ( maxCasts > spellsUsed ) {
        const int32_t currentNodeIdx = Maps::GetIndexFromAbsPoint( current );
        fheroes2::Point another = current;
        another.x += ( difference.x > 0 ) ? std::min( difference.x, distanceLimit ) : std::max( difference.x, -distanceLimit );
        another.y += ( difference.y > 0 ) ? std::min( difference.y, distanceLimit ) : std::max( difference.y, -distanceLimit );

        const int32_t anotherNodeIdx = Maps::GetIndexFromAbsPoint( another );
        bool found = Maps::isValidForDimensionDoor( anotherNodeIdx, water );

        if ( !found ) {
            fheroes2::Point bestDirectionDiff;
            int bestNextIdx = -1;

            for ( size_t i = 0; i < directions.size(); ++i ) {
                if ( !Maps::isValidDirection( anotherNodeIdx, directions[i] ) )
                    continue;

                const int newIndex = anotherNodeIdx + _mapOffset[i];
                if ( !Maps::isValidForDimensionDoor( newIndex, water ) )
                    continue;

                // If we are near the destination and we cannot reach the cell, skip it.
                if ( anotherNodeIdx == targetIndex && !isValidPath( anotherNodeIdx, directions[i], _currentColor ) ) {
                    continue;
                }

                const fheroes2::Point newPoint = Maps::GetPoint( newIndex );
                const fheroes2::Point directionDiff{ std::abs( current.x - newPoint.x ), std::abs( current.y - newPoint.y ) };

                if ( directionDiff.x > distanceLimit || directionDiff.y > distanceLimit ) {
                    continue;
                }

                if ( ( bestNextIdx < 0 ) || ( bestDirectionDiff.x + bestDirectionDiff.y > directionDiff.x + directionDiff.y ) ) {
                    bestNextIdx = newIndex;
                    bestDirectionDiff = directionDiff;
                }
            }

            if ( bestNextIdx == -1 ) {
                return {};
            }

            path.emplace_back( bestNextIdx, currentNodeIdx, Direction::CENTER, movementCost );
            current = Maps::GetPoint( bestNextIdx );
        }
        else {
            path.emplace_back( anotherNodeIdx, currentNodeIdx, Direction::CENTER, movementCost );
            current = another;
        }

        ++spellsUsed;

        difference = targetPoint - current;
        if ( std::abs( difference.x ) <= 1 && std::abs( difference.y ) <= 1 ) {
            // If this assertion blows up the logic above is wrong!
            assert( !path.empty() );
            return path;
        }
    }

    return {};
}

bool AIWorldPathfinder::isHeroJustInFrontOfDestination( const int currentIndex, const int targetIndex, const int color )
{
    // It could happen in 3 situations:
    // 1. The target is unreachable for the hero.
    // 2. Hero jumped using Dimension Door or Town Portal spell just in front of the object and this object requires to stay in front of it.
    // 3. An enemy hero came just in front of the hero.
    const bool isNeededToStayInFront = MP2::isNeedStayFront( world.GetTiles( targetIndex ).GetObject() );
    if ( !isNeededToStayInFront ) {
        // Object is not reachable.
        return false;
    }

    const Maps::Tiles & heroTile = world.GetTiles( currentIndex );
    const int direction = Maps::GetDirection( currentIndex, targetIndex );

    if ( !world.GetTiles( targetIndex ).isPassableFrom( direction, heroTile.isWater(), false, color ) ) {
        return false;
    }

    return true;
}

std::list<Route::Step> AIWorldPathfinder::buildPath( const int targetIndex, const bool isPlanningMode /* = false */ ) const
{
    assert( _pathStart != -1 && targetIndex != -1 );

    std::list<Route::Step> path;

    if ( _cache[targetIndex]._cost == 0 ) {
        // It could happen in 3 situations:
        // 1. The target is unreachable for the hero.
        // 2. Hero jumped using Dimension Door or Town Portal spell just in front of the object and this object requires to stay in front of it.
        // 3. An enemy hero came just in front of the hero.
        if ( !isHeroJustInFrontOfDestination( _pathStart, targetIndex, _currentColor ) ) {
            return path;
        }

        // This is case 2 or 3.
        path.emplace_front( targetIndex, _pathStart, Maps::GetDirection( _pathStart, targetIndex ), 0 );
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

void AIWorldPathfinder::setSpellPointReserve( const double reserve )
{
    _spellPointsReserved = reserve;
}
