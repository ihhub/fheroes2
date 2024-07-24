/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2024                                             *
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
#include <utility>

#include "army.h"
#include "artifact.h"
#include "castle.h"
#include "difficulty.h"
#include "direction.h"
#include "game.h"
#include "game_over.h"
#include "ground.h"
#include "heroes.h"
#include "kingdom.h"
#include "maps.h"
#include "maps_fileinfo.h"
#include "maps_tiles.h"
#include "maps_tiles_helper.h"
#include "math_base.h"
#include "pairs.h"
#include "players.h"
#include "rand.h"
#include "route.h"
#include "settings.h"
#include "spell.h"
#include "spell_info.h"
#include "tools.h"
#include "world.h"

namespace
{
    bool isFindArtifactVictoryConditionForHuman( const Artifact & art )
    {
        assert( art.isValid() );

        const Maps::FileInfo & mapInfo = Settings::Get().getCurrentMapInfo();

        if ( ( mapInfo.ConditionWins() & GameOver::WINS_ARTIFACT ) == 0 ) {
            return false;
        }

        if ( mapInfo.WinsFindUltimateArtifact() ) {
            return art.isUltimate();
        }

        return ( art.GetID() == mapInfo.WinsFindArtifactID() );
    }

    bool isTileAvailableForWalkThrough( const int tileIndex, const bool fromWater )
    {
        const Maps::Tiles & tile = world.GetTiles( tileIndex );
        const bool toWater = tile.isWater();
        const MP2::MapObjectType objectType = tile.GetObject();

        if ( objectType == MP2::OBJ_HERO || objectType == MP2::OBJ_MONSTER || objectType == MP2::OBJ_BOAT ) {
            return false;
        }

        if ( MP2::isPickupObject( objectType ) || MP2::isInGameActionObject( objectType, fromWater ) ) {
            return false;
        }

        if ( fromWater && !toWater && objectType == MP2::OBJ_COAST ) {
            return false;
        }

        // In general, direct movement from a shore tile to a water tile is not possible, but AI can use this movement for transparent Summon Boat
        // spellcasting. In this case, it may be necessary to cut the resulting path on a water tile.
        if ( !fromWater && toWater && objectType == MP2::OBJ_NONE ) {
            return false;
        }

        return true;
    }

    bool isTileAvailableForWalkThroughForAIWithArmy( const int tileIndex, const bool fromWater, const int color, const bool isArtifactsBagFull,
                                                     const bool isEquippedWithSpellBook, const double armyStrength, const double minimalAdvantage )
    {
        assert( color & Color::ALL );

        const Maps::Tiles & tile = world.GetTiles( tileIndex );
        const bool toWater = tile.isWater();
        const MP2::MapObjectType objectType = tile.GetObject();

        const auto isTileAccessible = [color, armyStrength, minimalAdvantage, &tile]() {
            // Creating an Army instance is a relatively heavy operation, so cache it to speed up calculations
            static Army tileArmy;
            tileArmy.setFromTile( tile );

            const int tileArmyColor = tileArmy.GetColor();
            // Tile can be guarded by our own or a friendly army (for example, our ally used a Set Elemental Guardian spell on his mine)
            if ( color == tileArmyColor || Players::isFriends( color, tileArmyColor ) ) {
                return true;
            }

            return tileArmy.GetStrength() * minimalAdvantage <= armyStrength;
        };

        // Enemy heroes can be defeated and passed through
        if ( objectType == MP2::OBJ_HERO ) {
            // Heroes on the water can be attacked from the nearby shore, but they cannot be passed through
            if ( fromWater != toWater ) {
                assert( !fromWater && toWater );

                return false;
            }

            const Heroes * otherHero = tile.getHero();
            assert( otherHero != nullptr );

            // Friendly heroes cannot be passed through
            if ( otherHero->isFriends( color ) ) {
                return false;
            }

            // Heroes in castles cannot be passed through
            if ( otherHero->inCastle() ) {
                return false;
            }

            // WINS_HERO victory condition does not apply to AI-controlled players, we have to keep this hero alive for the human player
            if ( otherHero == world.GetHeroesCondWins() ) {
                return false;
            }

            return otherHero->GetArmy().GetStrength() * minimalAdvantage <= armyStrength;
        }

        // Pickupable objects (including artifacts) can be picked up and passed through
        if ( MP2::isPickupObject( objectType ) ) {
            // Genie Lamp is special: there may not be enough money to hire all the genies and remove this object from the map, high-level
            // AI logic will decide what to do with it
            if ( objectType == MP2::OBJ_GENIE_LAMP ) {
                return false;
            }

            // If this object doesn't contain an artifact, then just pick it up and go through
            if ( !MP2::isArtifactObject( objectType ) ) {
                return true;
            }

            const Artifact art = Maps::getArtifactFromTile( tile );
            if ( !art.isValid() ) {
                return true;
            }

            // WINS_ARTIFACT victory condition does not apply to AI-controlled players, we should leave this artifact untouched for the human player
            if ( isFindArtifactVictoryConditionForHuman( art ) ) {
                return false;
            }

            // This object contains an artifact, but it is not an artifact itself, pick it up and go through
            if ( objectType != MP2::OBJ_ARTIFACT ) {
                return true;
            }

            // Hero should have a place for this artifact in his artifact bag
            if ( isArtifactsBagFull ) {
                return false;
            }

            // Hero will not be able to pick up a spell book if he already has one
            if ( art.GetID() == Artifact::MAGIC_BOOK && isEquippedWithSpellBook ) {
                return false;
            }

            // Check the conditions for picking up the artifact (except for the artifact guard): if there are any, then we can't pick it up
            // "automatically", high-level AI logic will decide what to do with it
            const Maps::ArtifactCaptureCondition condition = getArtifactCaptureCondition( tile );

            switch ( condition ) {
            case Maps::ArtifactCaptureCondition::PAY_2000_GOLD:
            case Maps::ArtifactCaptureCondition::PAY_2500_GOLD_AND_3_RESOURCES:
            case Maps::ArtifactCaptureCondition::PAY_3000_GOLD_AND_5_RESOURCES:

            case Maps::ArtifactCaptureCondition::HAVE_WISDOM_SKILL:
            case Maps::ArtifactCaptureCondition::HAVE_LEADERSHIP_SKILL:
                return false;

            default:
                break;
            }

            // Artifact may be guarded, check the power of guardians.
            return isTileAccessible();
        }

        // Monsters can be defeated and passed through
        if ( objectType == MP2::OBJ_MONSTER ) {
            return isTileAccessible();
        }

        // AI may have the key for the barrier
        if ( objectType == MP2::OBJ_BARRIER ) {
            return world.GetKingdom( color ).IsVisitTravelersTent( getColorFromTile( tile ) );
        }

        // AI can use boats to overcome water obstacles
        if ( objectType == MP2::OBJ_BOAT ) {
            return true;
        }

        // If we can't step on this tile, then it cannot be passed through
        if ( MP2::isNeedStayFront( objectType ) ) {
            return false;
        }

        // The castle tile can be passed through if we got there using Town Gate or Town Portal spells, which means that
        // this should be our castle
        if ( objectType == MP2::OBJ_CASTLE ) {
            return color == Maps::getColorFromTile( tile );
        }

        // If we can step on this tile, but it is protected by monsters and it is impossible to refuse a fight, then it
        // can be passed through if we manage to defeat the monsters
        if ( MP2::isBattleMandatoryifObjectIsProtected( objectType ) ) {
            return isTileAccessible();
        }

        // We can step on this tile and it is either not protected by monsters, or we can refuse a fight, just go ahead
        return true;
    }

    bool isMovementAllowedForColor( const int from, const int direction, const int color, const bool isSummonBoatSpellAvailable )
    {
        const Maps::Tiles & fromTile = world.GetTiles( from );
        const bool fromWater = fromTile.isWater();

        // check corner water/coast
        if ( fromWater ) {
            const int mapWidth = world.w();
            switch ( direction ) {
            case Direction::TOP_LEFT: {
                assert( from >= mapWidth + 1 );
                if ( world.GetTiles( from - mapWidth - 1 ).isWater() && ( !world.GetTiles( from - 1 ).isWater() || !world.GetTiles( from - mapWidth ).isWater() ) ) {
                    // Cannot sail through the corner of land.
                    return false;
                }

                break;
            }
            case Direction::TOP_RIGHT: {
                assert( from >= mapWidth && from + 1 < mapWidth * world.h() );
                if ( world.GetTiles( from - mapWidth + 1 ).isWater() && ( !world.GetTiles( from + 1 ).isWater() || !world.GetTiles( from - mapWidth ).isWater() ) ) {
                    // Cannot sail through the corner of land.
                    return false;
                }

                break;
            }
            case Direction::BOTTOM_RIGHT: {
                assert( from + mapWidth + 1 < mapWidth * world.h() );
                if ( world.GetTiles( from + mapWidth + 1 ).isWater() && ( !world.GetTiles( from + 1 ).isWater() || !world.GetTiles( from + mapWidth ).isWater() ) ) {
                    // Cannot sail through the corner of land.
                    return false;
                }

                break;
            }
            case Direction::BOTTOM_LEFT: {
                assert( from >= 1 && from + mapWidth - 1 < mapWidth * world.h() );
                if ( world.GetTiles( from + mapWidth - 1 ).isWater() && ( !world.GetTiles( from - 1 ).isWater() || !world.GetTiles( from + mapWidth ).isWater() ) ) {
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

        const Maps::Tiles & toTile = world.GetTiles( Maps::GetDirectionIndex( from, direction ) );

        if ( toTile.isPassableFrom( Direction::Reflect( direction ), fromWater, false, color ) ) {
            return true;
        }

        // Check whether it is possible to get to this tile using the Summon Boat spell...
        if ( !isSummonBoatSpellAvailable ) {
            return false;
        }

        // ... this only works when moving from the shore to an empty water tile...
        if ( fromWater || !toTile.isWater() || toTile.GetObject() != MP2::OBJ_NONE ) {
            return false;
        }

        // ... and this tile should be reachable from the shore (as if this shore tile were a water tile)
        return toTile.isPassableFrom( Direction::Reflect( direction ), true, false, color );
    }

    bool isTileAccessibleForAIWithArmy( const int tileIndex, const double armyStrength, const double minimalAdvantage )
    {
        // Tiles with monsters are considered accessible regardless of the monsters' power, high-level AI logic
        // will decide what to do with them
        if ( world.GetTiles( tileIndex ).GetObject() == MP2::OBJ_MONSTER ) {
            return true;
        }

        for ( const int32_t monsterIndex : Maps::getMonstersProtectingTile( tileIndex ) ) {
            // Creating an Army instance is a relatively heavy operation, so cache it to speed up calculations
            static Army tileArmy;
            tileArmy.setFromTile( world.GetTiles( monsterIndex ) );

            // Tiles guarded by too powerful wandering monsters are considered inaccessible
            if ( tileArmy.GetStrength() * minimalAdvantage > armyStrength ) {
                return false;
            }
        }

        return true;
    }

    uint32_t subtractMovePoints( const uint32_t movePoints, const uint32_t subtractedMovePoints, const uint32_t maxMovePoints )
    {
        // We do not perform pathfinding for a real hero on the map, this is no-op
        if ( maxMovePoints == 0 ) {
            return 0;
        }

        // This movement takes place at the beginning of a new turn: start with max movement points,
        // don't carry leftovers from the previous turn
        if ( movePoints < subtractedMovePoints ) {
            assert( maxMovePoints >= subtractedMovePoints );

            return maxMovePoints - subtractedMovePoints;
        }

        // This movement takes place on the same turn
        return movePoints - subtractedMovePoints;
    }
}

uint32_t WorldPathfinder::getDistance( int targetIndex ) const
{
    assert( targetIndex >= 0 && static_cast<size_t>( targetIndex ) < _cache.size() );

    return _cache[targetIndex]._cost;
}

uint32_t WorldPathfinder::getMovementPenalty( const int from, const int to, const int direction ) const
{
    const Maps::Tiles & fromTile = world.GetTiles( from );
    const Maps::Tiles & toTile = world.GetTiles( to );

    uint32_t penalty = fromTile.isRoad() && toTile.isRoad() ? Maps::Ground::roadPenalty : Maps::Ground::GetPenalty( fromTile, _pathfindingSkill );

    // Diagonal movement costs 50% more
    if ( Direction::isDiagonal( direction ) ) {
        penalty = penalty * 3 / 2;
    }

    // If we perform pathfinding for a real hero on the map, we have to work out the "last move"
    // logic: if this move is the last one on the current turn, then we can move to any adjacent
    // tile (both in straight and diagonal direction) as long as we have enough movement points
    // to move over our current tile in the straight direction
    if ( getMaxMovePoints( fromTile.isWater() ) > 0 ) {
        const WorldNode & node = _cache[from];

        // No dead ends allowed
        assert( from == _pathStart || node._from != -1 );

        const uint32_t remainingMovePoints = node._remainingMovePoints;
        const uint32_t fromTilePenalty = fromTile.isRoad() ? Maps::Ground::roadPenalty : Maps::Ground::GetPenalty( fromTile, _pathfindingSkill );

        // If we still have enough movement points to move over the source tile in the straight
        // direction, but not enough to move to the destination tile, then the "last move" logic
        // is applied and we can move to the destination tile anyway at the expense of all the
        // remaining movement points
        if ( remainingMovePoints >= fromTilePenalty && remainingMovePoints < penalty ) {
            return remainingMovePoints;
        }
    }

    return penalty;
}

void WorldPathfinder::reset()
{
    // The following optimization will only work correctly for square maps
    assert( world.w() == world.h() );

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

    _pathStart = -1;
    _color = Color::NONE;
    _remainingMovePoints = 0;
    _pathfindingSkill = Skill::Level::EXPERT;
}

void WorldPathfinder::processWorldMap()
{
    assert( _cache.size() == world.getSize() && Maps::isValidAbsIndex( _pathStart ) );

    for ( WorldNode & node : _cache ) {
        node.reset();
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
    const uint32_t maxMovePoints = getMaxMovePoints( world.GetTiles( currentNodeIdx ).isWater() );

    for ( size_t i = 0; i < directions.size(); ++i ) {
        if ( !Maps::isValidDirection( currentNodeIdx, directions[i] ) || !isMovementAllowed( currentNodeIdx, directions[i] ) ) {
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
            newNode._remainingMovePoints = subtractMovePoints( currentNode._remainingMovePoints, movementPenalty, maxMovePoints );

            nodesToExplore.push_back( newIndex );
        }
    }
}

bool WorldPathfinder::isMovementAllowed( const int from, const int direction ) const
{
    return isMovementAllowedForColor( from, direction, _color, false );
}

void PlayerWorldPathfinder::reset()
{
    WorldPathfinder::reset();

    _maxMovePoints = 0;
}

void PlayerWorldPathfinder::reEvaluateIfNeeded( const Heroes & hero )
{
    auto currentSettings = std::tie( _pathStart, _color, _remainingMovePoints, _pathfindingSkill, _maxMovePoints );
    const auto newSettings = std::make_tuple( hero.GetIndex(), hero.GetColor(), hero.GetMovePoints(),
                                              static_cast<uint8_t>( hero.GetLevelSkill( Skill::Secondary::PATHFINDING ) ), hero.GetMaxMovePoints() );

    if ( currentSettings != newSettings ) {
        currentSettings = newSettings;

        processWorldMap();
    }
}

std::list<Route::Step> PlayerWorldPathfinder::buildPath( const int targetIndex ) const
{
    assert( _cache.size() == world.getSize() && Maps::isValidAbsIndex( _pathStart ) && Maps::isValidAbsIndex( targetIndex ) );

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
    const bool isFirstNode = ( currentNodeIdx == _pathStart );
    const WorldNode & currentNode = _cache[currentNodeIdx];
    const bool fromWater = world.GetTiles( _pathStart ).isWater();

    if ( !isFirstNode && !isTileAvailableForWalkThrough( currentNodeIdx, fromWater ) ) {
        return;
    }

    const MapsIndexes & monsters = Maps::getMonstersProtectingTile( currentNodeIdx );

    // If the current tile is protected by monsters, and this tile is not the starting tile, then the hero can only move towards one of the neighboring monsters
    if ( !isFirstNode && !monsters.empty() ) {
        const uint32_t maxMovePoints = getMaxMovePoints( fromWater );

        for ( int monsterIndex : monsters ) {
            const int direction = Maps::GetDirection( currentNodeIdx, monsterIndex );

            if ( direction == Direction::UNKNOWN || direction == Direction::CENTER || !isMovementAllowed( currentNodeIdx, direction ) ) {
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
                monsterNode._remainingMovePoints = subtractMovePoints( currentNode._remainingMovePoints, movementPenalty, maxMovePoints );
            }
        }
    }
    else {
        checkAdjacentNodes( nodesToExplore, currentNodeIdx );
    }
}

uint32_t PlayerWorldPathfinder::getMaxMovePoints( const bool /* onWater */ ) const
{
    return _maxMovePoints;
}

void AIWorldPathfinder::reset()
{
    WorldPathfinder::reset();

    _maxMovePointsOnLand = 0;
    _maxMovePointsOnWater = 0;
    _armyStrength = -1;
    _isArtifactsBagFull = false;
    _isEquippedWithSpellBook = false;
    _isSummonBoatSpellAvailable = false;

    _townGateCastleIndex = -1;
    _townPortalCastleIndexes.clear();
}

void AIWorldPathfinder::reEvaluateIfNeeded( const Heroes & hero )
{
    const bool isSummonBoatSpellAvailable = [this, &hero]() {
        static const Spell summonBoat( Spell::SUMMONBOAT );

        if ( !hero.HaveSpell( summonBoat ) ) {
            return false;
        }

        if ( hero.GetSpellPoints() < summonBoat.spellPoints( &hero ) + hero.GetMaxSpellPoints() * _spellPointsReserveRatio ) {
            return false;
        }

        return ( fheroes2::getSummonableBoat( hero ) != -1 );
    }();

    const int32_t townGateCastleIndex = [this, &hero]() {
        static const Spell townGate( Spell::TOWNGATE );

        if ( hero.Modes( Heroes::PATROL ) || !hero.CanCastSpell( townGate ) ) {
            return -1;
        }

        if ( hero.GetSpellPoints() < townGate.spellPoints( &hero ) + hero.GetMaxSpellPoints() * _spellPointsReserveRatio ) {
            return -1;
        }

        const Castle * castle = fheroes2::getNearestCastleTownGate( hero );
        assert( castle != nullptr && castle->GetHero() == nullptr );

        return castle->GetIndex();
    }();

    const std::vector<int32_t> townPortalCastleIndexes = [this, &hero]() {
        static const Spell townPortal( Spell::TOWNPORTAL );

        std::vector<int32_t> result;

        if ( hero.Modes( Heroes::PATROL ) || !hero.CanCastSpell( townPortal ) ) {
            return result;
        }

        if ( hero.GetSpellPoints() < townPortal.spellPoints( &hero ) + hero.GetMaxSpellPoints() * _spellPointsReserveRatio ) {
            return result;
        }

        for ( const Castle * castle : hero.GetKingdom().GetCastles() ) {
            assert( castle != nullptr );

            if ( castle->GetHero() == nullptr ) {
                result.push_back( castle->GetIndex() );
            }
        }

        return result;
    }();

    auto currentSettings = std::tie( _pathStart, _color, _remainingMovePoints, _pathfindingSkill, _maxMovePointsOnLand, _maxMovePointsOnWater, _armyStrength,
                                     _isArtifactsBagFull, _isEquippedWithSpellBook, _isSummonBoatSpellAvailable, _townGateCastleIndex, _townPortalCastleIndexes );
    const auto newSettings
        = std::make_tuple( hero.GetIndex(), hero.GetColor(), hero.GetMovePoints(), static_cast<uint8_t>( hero.GetLevelSkill( Skill::Secondary::PATHFINDING ) ),
                           hero.GetMaxMovePoints( false ), hero.GetMaxMovePoints( true ), hero.GetArmy().GetStrength(), hero.IsFullBagArtifacts(), hero.HaveSpellBook(),
                           isSummonBoatSpellAvailable, townGateCastleIndex, townPortalCastleIndexes );

    if ( currentSettings != newSettings ) {
        currentSettings = newSettings;

        processWorldMap();
    }
}

void AIWorldPathfinder::reEvaluateIfNeeded( const int start, const int color, const double armyStrength, const uint8_t skill )
{
    auto currentSettings = std::tie( _pathStart, _color, _remainingMovePoints, _pathfindingSkill, _maxMovePointsOnLand, _maxMovePointsOnWater, _armyStrength,
                                     _isArtifactsBagFull, _isEquippedWithSpellBook, _isSummonBoatSpellAvailable, _townGateCastleIndex, _townPortalCastleIndexes );
    const auto newSettings = std::make_tuple( start, color, 0U, skill, 0U, 0U, armyStrength, false, false, false, -1, std::vector<int32_t>{} );

    if ( currentSettings != newSettings ) {
        currentSettings = newSettings;

        processWorldMap();
    }
}

void AIWorldPathfinder::processWorldMap()
{
    assert( _cache.size() == world.getSize() && Maps::isValidAbsIndex( _pathStart ) );

    for ( WorldNode & node : _cache ) {
        node.reset();
    }

    _cache[_pathStart] = WorldNode( -1, 0, MP2::OBJ_NONE, _remainingMovePoints );

    std::vector<int> nodesToExplore;
    nodesToExplore.push_back( _pathStart );

    const auto processTownPortal = [this, &nodesToExplore]( const Spell & spell, const int32_t castleIndex ) {
        assert( castleIndex >= 0 && static_cast<size_t>( castleIndex ) < _cache.size() );
        assert( castleIndex != _pathStart && _cache[castleIndex]._from == -1 );

        const uint32_t cost = spell.movePoints();
        const uint32_t remaining = ( _remainingMovePoints < cost ) ? 0 : _remainingMovePoints - cost;

        _cache[castleIndex] = WorldNode( _pathStart, cost, MP2::OBJ_CASTLE, remaining );
        nodesToExplore.push_back( castleIndex );
    };

    if ( _townGateCastleIndex != -1 ) {
        processTownPortal( Spell::TOWNGATE, _townGateCastleIndex );
    }

    for ( const int32_t idx : _townPortalCastleIndexes ) {
        if ( idx == _townGateCastleIndex ) {
            continue;
        }

        processTownPortal( Spell::TOWNPORTAL, idx );
    }

    for ( size_t lastProcessedNode = 0; lastProcessedNode < nodesToExplore.size(); ++lastProcessedNode ) {
        processCurrentNode( nodesToExplore, nodesToExplore[lastProcessedNode] );
    }
}

bool AIWorldPathfinder::isMovementAllowed( const int from, const int direction ) const
{
    return isMovementAllowedForColor( from, direction, _color, _isSummonBoatSpellAvailable );
}

void AIWorldPathfinder::processCurrentNode( std::vector<int> & nodesToExplore, const int currentNodeIdx )
{
    const bool isFirstNode = ( currentNodeIdx == _pathStart );
    WorldNode & currentNode = _cache[currentNodeIdx];

    // Always allow movement from the starting point to cover the edge case where we got here before this tile became blocked
    if ( !isFirstNode ) {
        if ( !isTileAccessibleForAIWithArmy( currentNodeIdx, _armyStrength, _minimalArmyStrengthAdvantage ) ) {
            // If we can't move here, then reset the node
            currentNode.reset();

            return;
        }

        // No dead ends allowed
        assert( currentNode._from != -1 );

        const bool fromWater = world.GetTiles( currentNode._from ).isWater();

        if ( !isTileAvailableForWalkThroughForAIWithArmy( currentNodeIdx, fromWater, _color, _isArtifactsBagFull, _isEquippedWithSpellBook, _armyStrength,
                                                          _minimalArmyStrengthAdvantage ) ) {
            return;
        }
    }

    MapsIndexes teleports;

    // We shouldn't use teleport at the starting tile
    if ( !isFirstNode ) {
        teleports = world.GetTeleportEndPoints( currentNodeIdx );

        if ( teleports.empty() ) {
            teleports = world.GetWhirlpoolEndPoints( currentNodeIdx );
        }
    }

    // Check adjacent nodes only if we are either not on the teleport tile, or we got here from another endpoint of this teleport.
    // Do not check them if we came to the tile with a teleport from a neighboring tile (and are going to use it for teleportation).
    if ( teleports.empty() || std::find( teleports.begin(), teleports.end(), currentNode._from ) != teleports.end() ) {
        checkAdjacentNodes( nodesToExplore, currentNodeIdx );
    }

    // Special case: movement via teleport
    for ( const int teleportIdx : teleports ) {
        if ( teleportIdx == _pathStart ) {
            continue;
        }

        WorldNode & teleportNode = _cache[teleportIdx];

        // Check if the movement is really faster via teleport
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

uint32_t AIWorldPathfinder::getMaxMovePoints( const bool onWater ) const
{
    return onWater ? _maxMovePointsOnWater : _maxMovePointsOnLand;
}

uint32_t AIWorldPathfinder::getMovementPenalty( const int from, const int to, const int direction ) const
{
    const Maps::Tiles & fromTile = world.GetTiles( from );

    const uint32_t defaultPenalty = [this, from, to, direction, &fromTile]() {
        const uint32_t regularPenalty = WorldPathfinder::getMovementPenalty( from, to, direction );

        if ( from == _pathStart ) {
            return regularPenalty;
        }

        const MP2::MapObjectType objectType = fromTile.GetObject();
        if ( !MP2::isNeedStayFront( objectType ) || objectType == MP2::OBJ_BOAT ) {
            return regularPenalty;
        }

        const WorldNode & node = _cache[from];

        // No dead ends allowed
        assert( node._from != -1 );

        const int prevStepDirection = Maps::GetDirection( node._from, from );
        assert( prevStepDirection != Direction::UNKNOWN && prevStepDirection != Direction::CENTER );

        // If we are moving from a tile that we technically cannot stand on, then it means that there was
        // an object on this tile that we previously removed. Thus, we have spent additional movement points
        // when moving to this tile - once when accessing the object to remove it, and again when moving to
        // this tile.
        //
        // According to a rough estimate, the movement points spent can be considered the same in both cases,
        // therefore, we apply an additional penalty when moving from the tile containing this object to the
        // next tile. In general, it is impossible to perform an accurate estimation, since the stats and
        // skills of a moving hero may change after interacting with the object (e.g. hero can upgrade his
        // Pathfinding skill).
        //
        // The real path will not reach this step, so this logic will be used to estimate distances more
        // accurately when choosing whether to move through objects or past them.
        return regularPenalty + WorldPathfinder::getMovementPenalty( node._from, from, prevStepDirection );
    }();

    const uint32_t maxMovePoints = getMaxMovePoints( fromTile.isWater() );
    assert( maxMovePoints == 0 || defaultPenalty <= maxMovePoints );

    // If we perform pathfinding for a real AI-controlled hero on the map, we should correctly calculate
    // movement penalties when this hero overcomes water obstacles using boats.
    if ( maxMovePoints > 0 ) {
        const WorldNode & node = _cache[from];

        // No dead ends allowed
        assert( from == _pathStart || node._from != -1 );

        const Maps::Tiles & toTile = world.GetTiles( to );

        // AI-controlled hero may get from the shore to an empty water tile using the Summon Boat spell
        const bool isEmptyWaterTile = ( toTile.isWater() && toTile.GetObject() == MP2::OBJ_NONE );
        const bool isComesOnBoard = ( !fromTile.isWater() && ( toTile.GetObject() == MP2::OBJ_BOAT || isEmptyWaterTile ) );
        const bool isDisembarks = ( fromTile.isWater() && toTile.GetObject() == MP2::OBJ_COAST );

        // When the hero gets into a boat or disembarks, he spends all remaining movement points.
        if ( isComesOnBoard || isDisembarks ) {
            // If the hero is not able to make this movement this turn, then he will have to spend
            // all the movement points next turn.
            if ( defaultPenalty > node._remainingMovePoints ) {
                return maxMovePoints;
            }

            return node._remainingMovePoints;
        }
    }

    return defaultPenalty;
}

std::pair<int32_t, bool> AIWorldPathfinder::getFogDiscoveryTile( const Heroes & hero )
{
    reEvaluateIfNeeded( hero );

    const auto findBestTile = [this, scoutingDistance = hero.GetScoutingDistance()]( const auto nearbyTilePredicate ) {
        const Directions & directions = Direction::All();

        struct TileCharacteristics
        {
            int32_t index{ -1 };
            uint32_t cost{ UINT32_MAX };
            int32_t tilesToReveal{ 0 };
        };

        TileCharacteristics bestTile;

        for ( size_t idx = 0; idx < _cache.size(); ++idx ) {
            const uint32_t nodeCost = _cache[idx]._cost;
            if ( nodeCost == 0 ) {
                continue;
            }

            const int32_t tileIdx = static_cast<int32_t>( idx );
            if ( !MP2::isSafeForFogDiscoveryObject( world.GetTiles( tileIdx ).GetObject( true ) ) ) {
                continue;
            }

            bool isTileSuitable = false;

            for ( size_t i = 0; i < directions.size(); ++i ) {
                if ( !Maps::isValidDirection( tileIdx, directions[i] ) ) {
                    continue;
                }

                if ( !nearbyTilePredicate( tileIdx + _mapOffset[i] ) ) {
                    continue;
                }

                isTileSuitable = true;

                break;
            }

            if ( !isTileSuitable ) {
                continue;
            }

            const int32_t tilesToReveal = Maps::getFogTileCountToBeRevealed( tileIdx, scoutingDistance, _color );
            assert( tilesToReveal >= 0 );

            if ( tilesToReveal == 0 ) {
                continue;
            }

            if ( nodeCost < bestTile.cost || ( nodeCost == bestTile.cost && tilesToReveal > bestTile.tilesToReveal ) ) {
                bestTile = { tileIdx, nodeCost, tilesToReveal };
            }
        }

        return bestTile.index;
    };

    // First, consider the accessible tiles, one of the neighboring tiles of which is covered with fog. Most likely, some of these neighboring tiles are also accessible.
    {
        const int32_t bestTileIdx = findBestTile( [this]( const int32_t tileIdx ) { return world.GetTiles( tileIdx ).isFog( _color ); } );
        if ( bestTileIdx != -1 ) {
            return { bestTileIdx, true };
        }
    }

    // If we are unlucky, then we need to do the heavy lifting and consider the accessible tiles that have at least one neighboring tile that is inaccessible to the hero
    // (since there may be unexplored tiles covered with fog on the other side of such an obstacle).
    {
        const int32_t bestTileIdx = findBestTile( [this]( const int32_t tileIdx ) { return _cache[tileIdx]._cost == 0; } );
        if ( bestTileIdx != -1 ) {
            return { bestTileIdx, false };
        }
    }

    return { -1, false };
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
        if ( MP2::isInGameActionObject( world.GetTiles( newIndex ).GetObject( true ) ) ) {
            continue;
        }

        const WorldNode & node = _cache[newIndex];

        // Tile is directly reachable (in one move) and the hero has enough army to defeat potential guards
        if ( node._cost > 0 && node._from == start ) {
            return newIndex;
        }
    }

    return -1;
}

bool AIWorldPathfinder::isHeroPossiblyBlockingWay( const Heroes & hero )
{
    const int32_t heroIndex = hero.GetIndex();
    const int heroColor = hero.GetColor();

    const auto isReachableDirection = [heroIndex, heroColor]( const int direction ) {
        if ( !Maps::isValidDirection( heroIndex, direction ) ) {
            return false;
        }

        if ( !isMovementAllowedForColor( heroIndex, direction, heroColor, false ) ) {
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

    const Maps::Tiles & heroTile = world.GetTiles( heroIndex );

    // Hero in the boat can neither occupy nor block the Stone Liths
    if ( heroTile.isWater() ) {
        assert( heroTile.GetObject( false ) != MP2::OBJ_STONE_LITHS );

        return false;
    }

    // Does the hero potentially block the exit from Stone Liths for another hero?
    for ( const int32_t idx : Maps::ScanAroundObject( heroIndex, MP2::OBJ_STONE_LITHS ) ) {
        const Maps::Tiles & tile = world.GetTiles( idx );

        if ( tile.GetObject() == MP2::OBJ_HERO ) {
            const int direction = Maps::GetDirection( idx, heroIndex );
            assert( CountBits( direction ) == 1 && direction != Direction::CENTER );

            if ( tile.isPassableTo( direction ) && heroTile.isPassableFrom( Direction::Reflect( direction ) ) ) {
                return true;
            }
        }
    }

    // Is the hero standing on Stone Liths?
    return heroTile.GetObject( false ) == MP2::OBJ_STONE_LITHS;
}

std::vector<IndexObject> AIWorldPathfinder::getObjectsOnTheWay( const int targetIndex ) const
{
    assert( _cache.size() == world.getSize() && Maps::isValidAbsIndex( _pathStart ) && _color != Color::NONE && Maps::isValidAbsIndex( targetIndex ) );

    std::vector<IndexObject> result;

    // Destination is not reachable
    if ( _cache[targetIndex]._cost == 0 ) {
        return result;
    }

    const Kingdom & kingdom = world.GetKingdom( _color );
    std::set<int> uniqueIndices;

    const auto validateAndAdd = [&kingdom, &result, &uniqueIndices]( int index, const MP2::MapObjectType objectType ) {
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

        // The path should not pass through the same tile more than once
        assert( uniqPathIndexes.insert( node._from ).second );

        currentNode = node._from;
    }

    return result;
}

std::list<Route::Step> AIWorldPathfinder::getDimensionDoorPath( const Heroes & hero, int targetIndex ) const
{
    uint32_t difficultyLimit = Difficulty::GetDimensionDoorLimitForAI( Game::getDifficulty() );
    const uint32_t spellsUsedThisTurn = hero.getDimensionDoorUses();
    if ( hero.GetIndex() == targetIndex || spellsUsedThisTurn >= difficultyLimit ) {
        return {};
    }

    const Spell dimensionDoor( Spell::DIMENSIONDOOR );
    if ( !hero.HaveSpell( dimensionDoor ) || !Maps::isValidAbsIndex( targetIndex ) ) {
        return {};
    }

    uint32_t currentSpellPoints = hero.GetSpellPoints();

    const Maps::Tiles & tile = world.GetTiles( targetIndex );
    const MP2::MapObjectType objectType = tile.GetObject( true );

    // Reserve spell points only if target isn't a well that will replenish lost SP
    if ( objectType != MP2::OBJ_MAGIC_WELL && objectType != MP2::OBJ_ARTESIAN_SPRING ) {
        if ( currentSpellPoints < hero.GetMaxSpellPoints() * _spellPointsReserveRatio ) {
            return {};
        }

        currentSpellPoints -= static_cast<uint32_t>( hero.GetMaxSpellPoints() * _spellPointsReserveRatio );
    }

    difficultyLimit -= spellsUsedThisTurn;
    const uint32_t movementCost = std::max( 1U, dimensionDoor.movePoints() );
    const uint32_t spellcastsPossible = std::min( currentSpellPoints / std::max( 1U, dimensionDoor.spellPoints( &hero ) ), hero.GetMovePoints() / movementCost );
    const uint32_t maxCasts = std::min( spellcastsPossible, difficultyLimit );

    // Have to explicitly call GetObject( false ) since hero might be standing on it
    if ( tile.GetObject( false ) == MP2::OBJ_CASTLE ) {
        targetIndex = Maps::GetDirectionIndex( targetIndex, Direction::BOTTOM );
        if ( !Maps::isValidAbsIndex( targetIndex ) ) {
            return {};
        }
    }

    if ( !isTileAccessibleForAIWithArmy( targetIndex, _armyStrength, _minimalArmyStrengthAdvantage ) ) {
        return {};
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
                if ( anotherNodeIdx == targetIndex && !isMovementAllowed( newIndex, Direction::Reflect( directions[i] ) ) ) {
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

std::list<Route::Step> AIWorldPathfinder::buildPath( const int targetIndex ) const
{
    assert( _cache.size() == world.getSize() && Maps::isValidAbsIndex( _pathStart ) && Maps::isValidAbsIndex( targetIndex ) );

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

        if ( !isTileAvailableForWalkThrough( currentNode, fromWater ) ) {
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

    // Cut the path to the last valid tile/obstacle
    if ( lastValidNode != targetIndex ) {
        path.erase( std::find_if( path.begin(), path.end(), [lastValidNode]( const Route::Step & step ) { return step.GetFrom() == lastValidNode; } ), path.end() );
    }

    return path;
}

uint32_t AIWorldPathfinder::getDistance( const int start, const int targetIndex, const int color, const double armyStrength,
                                         const uint8_t skill /* = Skill::Level::EXPERT */ )
{
    reEvaluateIfNeeded( start, color, armyStrength, skill );

    return _cache[targetIndex]._cost;
}

void AIWorldPathfinder::setMinimalArmyStrengthAdvantage( const double advantage )
{
    if ( advantage < 0.0 ) {
        assert( 0 );
        return;
    }

    if ( std::fabs( _minimalArmyStrengthAdvantage - advantage ) <= 0.001 ) {
        return;
    }

    _minimalArmyStrengthAdvantage = advantage;

    reset();
}

void AIWorldPathfinder::setSpellPointsReserveRatio( const double ratio )
{
    if ( ratio < 0.0 || ratio > 1.0 ) {
        assert( 0 );
        return;
    }

    if ( std::fabs( _spellPointsReserveRatio - ratio ) <= 0.001 ) {
        return;
    }

    _spellPointsReserveRatio = ratio;

    reset();
}
