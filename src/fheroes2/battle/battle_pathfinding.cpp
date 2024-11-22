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

#include "battle_pathfinding.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <set>
#include <tuple>
#include <vector>

#include "battle_arena.h"
#include "battle_cell.h"
#include "battle_troop.h"
#include "castle.h"
#include "speed.h"

namespace
{
    const uint32_t MOAT_PENALTY = UINT16_MAX;
}

namespace Battle
{
    void BattlePathfinder::reEvaluateIfNeeded( const Unit & unit )
    {
        assert( unit.GetHeadIndex() != -1 && ( unit.isWide() ? unit.GetTailIndex() != -1 : unit.GetTailIndex() == -1 ) );

        const Board * board = Arena::GetBoard();
        assert( board != nullptr );

        // Passability of the board cells can change during the unit's turn even without its intervention (for example, because of a hero's spell cast),
        // we need to keep track of this
        std::array<bool, Board::sizeInCells> boardStatus{};
        for ( const Cell & cell : *board ) {
            const int32_t cellIdx = cell.GetIndex();
            assert( Board::isValidIndex( cellIdx ) );

            boardStatus[cellIdx] = cell.isPassable( true );
        }

        auto currentSettings = std::tie( _pathStart, _speed, _isWide, _isFlying, _color, _boardStatus );
        const auto newSettings = std::make_tuple( BattleNodeIndex{ unit.GetHeadIndex(), unit.GetTailIndex() }, unit.GetSpeed(), unit.isWide(), unit.isFlying(),
                                                  unit.GetColor(), std::cref( boardStatus ) );

        // If all the current parameters match the parameters for which the current cache was built, then there is no need to rebuild it
        if ( currentSettings == newSettings ) {
            return;
        }

        currentSettings = newSettings;

        const Castle * castle = Arena::GetCastle();
        const bool isMoatBuilt = castle && castle->isBuild( BUILD_MOAT );

        _cache.clear();
        _cache.try_emplace( _pathStart );

        // Flying units can land wherever they can fit
        if ( _isFlying ) {
            for ( const Cell & cell : *board ) {
                const Position pos = Position::GetPosition( unit, cell.GetIndex() );

                if ( pos.GetHead() == nullptr ) {
                    continue;
                }

                assert( pos.isValidForUnit( unit ) );

                const int32_t headCellIdx = pos.GetHead()->GetIndex();
                const int32_t tailCellIdx = pos.GetTail() ? pos.GetTail()->GetIndex() : -1;

                if ( const auto [iter, inserted] = _cache.try_emplace( { headCellIdx, tailCellIdx } ); inserted ) {
                    // Wide units can occupy overlapping positions, the distance between which is actually zero,
                    // but since the movement takes place, we will consider the distance equal to 1 in this case
                    const uint32_t distance = std::max( Board::GetDistance( unit.GetPosition(), pos ), 1U );

                    iter->second.update( _pathStart, 1, distance );
                }
            }

            return;
        }

        // The index of that of the cells of the initial unit's position, which is located
        // in the moat (-1, if there is none)
        const int32_t pathStartMoatCellIdx = [this, &unit, isMoatBuilt]() {
            if ( !isMoatBuilt ) {
                return -1;
            }

            assert( _pathStart.first != -1 );

            if ( Board::isMoatIndex( _pathStart.first, unit ) ) {
                return _pathStart.first;
            }

            if ( _isWide ) {
                assert( _pathStart.second != -1 );

                if ( Board::isMoatIndex( _pathStart.second, unit ) ) {
                    return _pathStart.second;
                }
            }

            return -1;
        }();

        std::vector<BattleNodeIndex> nodesToExplore;
        nodesToExplore.reserve( Board::sizeInCells * 2 );
        nodesToExplore.push_back( _pathStart );

        for ( size_t nodesToExploreIdx = 0; nodesToExploreIdx < nodesToExplore.size(); ++nodesToExploreIdx ) {
            const BattleNodeIndex currentNodeIdx = nodesToExplore[nodesToExploreIdx];
            const BattleNode & currentNode = _cache[currentNodeIdx];

            if ( _isWide ) {
                assert( currentNodeIdx.first != -1 && currentNodeIdx.second != -1 );

                const auto [currentHeadCellIdx, currentTailCellIdx] = currentNodeIdx;
                const BattleNodeIndex flippedCurrentNodeIdx = { currentTailCellIdx, currentHeadCellIdx };

                const Cell * currentHeadCell = Board::GetCell( currentHeadCellIdx );
                assert( currentHeadCell != nullptr );

                const bool isCurrentLeftDirection = ( ( Board::GetDirection( currentTailCellIdx, currentHeadCellIdx ) & LEFT_SIDE ) != 0 );
                // The moat restrictions can be ignored if the wide unit originally occupied a moat cell, and at the current step any part
                // of this unit occupies the same moat cell
                const bool isIgnoreMoat = ( currentHeadCellIdx == pathStartMoatCellIdx || currentTailCellIdx == pathStartMoatCellIdx );
                const bool isInMoat = isMoatBuilt && ( Board::isMoatIndex( currentHeadCellIdx, unit ) || Board::isMoatIndex( currentTailCellIdx, unit ) );
                const uint32_t movementPenalty = ( !isIgnoreMoat && isInMoat ) ? MOAT_PENALTY : 1;

                for ( const int32_t headCellIdx : Board::GetMoveWideIndexes( currentHeadCellIdx, isCurrentLeftDirection ) ) {
                    const Cell * cell = Board::GetCell( headCellIdx );
                    assert( cell != nullptr );

                    if ( !cell->isPassableFromAdjacent( unit, *currentHeadCell ) ) {
                        continue;
                    }

                    const int32_t tailCellIdx = ( Board::GetDirection( currentHeadCellIdx, headCellIdx ) & LEFT_SIDE ) ? headCellIdx + 1 : headCellIdx - 1;

                    const BattleNodeIndex newNodeIdx = { headCellIdx, tailCellIdx };
                    if ( newNodeIdx == _pathStart ) {
                        continue;
                    }

                    // Reversal is not a movement
                    const uint32_t cost = currentNode._cost + ( newNodeIdx == flippedCurrentNodeIdx ? 0 : movementPenalty );
                    const uint32_t distance = currentNode._distance + ( newNodeIdx == flippedCurrentNodeIdx ? 0 : 1 );

                    BattleNode & newNode = _cache[newNodeIdx];
                    if ( newNode._from == BattleNodeIndex{ -1, -1 } || newNode._cost > cost ) {
                        newNode.update( currentNodeIdx, cost, distance );

                        nodesToExplore.push_back( newNodeIdx );
                    }
                }
            }
            else {
                assert( currentNodeIdx.first != -1 && currentNodeIdx.second == -1 );

                const auto [currentCellIdx, dummy] = currentNodeIdx;

                const Cell * currentCell = Board::GetCell( currentCellIdx );
                assert( currentCell != nullptr );

                // The moat restrictions can be ignored at the first step if the unit starts its movement from the moat
                const bool isIgnoreMoat = ( currentCellIdx == pathStartMoatCellIdx );
                const bool isInMoat = isMoatBuilt && Board::isMoatIndex( currentCellIdx, unit );
                const uint32_t movementPenalty = ( !isIgnoreMoat && isInMoat ) ? MOAT_PENALTY : 1;

                for ( const int32_t cellIdx : Board::GetAroundIndexes( currentCellIdx ) ) {
                    const Cell * cell = Board::GetCell( cellIdx );
                    assert( cell != nullptr );

                    if ( !cell->isPassableFromAdjacent( unit, *currentCell ) ) {
                        continue;
                    }

                    const BattleNodeIndex newNodeIdx = { cellIdx, -1 };
                    if ( newNodeIdx == _pathStart ) {
                        continue;
                    }

                    const uint32_t cost = currentNode._cost + movementPenalty;
                    const uint32_t distance = currentNode._distance + 1;

                    BattleNode & newNode = _cache[newNodeIdx];
                    if ( newNode._from == BattleNodeIndex{ -1, -1 } || newNode._cost > cost ) {
                        newNode.update( currentNodeIdx, cost, distance );

                        nodesToExplore.push_back( newNodeIdx );
                    }
                }
            }
        }
    }

    bool BattlePathfinder::isPositionReachable( const Unit & unit, const Position & position, const bool isOnCurrentTurn )
    {
        // Invalid positions are allowed here, but they are always unreachable
        if ( position.GetHead() == nullptr ) {
            return false;
        }

        reEvaluateIfNeeded( unit );

        const BattleNodeIndex nodeIdx = { position.GetHead()->GetIndex(), position.GetTail() ? position.GetTail()->GetIndex() : -1 };

        const auto iter = _cache.find( nodeIdx );
        if ( iter == _cache.end() ) {
            return false;
        }

        const auto & [index, node] = *iter;

        return ( index == _pathStart || node._from != BattleNodeIndex{ -1, -1 } ) && ( !isOnCurrentTurn || node._cost <= _speed );
    }

    uint32_t BattlePathfinder::getCost( const Unit & unit, const Position & position )
    {
        assert( position.GetHead() != nullptr );

        reEvaluateIfNeeded( unit );

        const BattleNodeIndex nodeIdx = { position.GetHead()->GetIndex(), position.GetTail() ? position.GetTail()->GetIndex() : -1 };

        const auto iter = _cache.find( nodeIdx );
        assert( iter != _cache.end() );

        const auto & [index, node] = *iter;
        // MSVC 2017 fails to properly expand the assert() macro without additional parentheses
        assert( ( index == _pathStart || node._from != BattleNodeIndex{ -1, -1 } ) );

        return node._cost;
    }

    uint32_t BattlePathfinder::getDistance( const Unit & unit, const Position & position )
    {
        assert( position.GetHead() != nullptr );

        reEvaluateIfNeeded( unit );

        const BattleNodeIndex nodeIdx = { position.GetHead()->GetIndex(), position.GetTail() ? position.GetTail()->GetIndex() : -1 };

        const auto iter = _cache.find( nodeIdx );
        assert( iter != _cache.end() );

        const auto & [index, node] = *iter;
        // MSVC 2017 fails to properly expand the assert() macro without additional parentheses
        assert( ( index == _pathStart || node._from != BattleNodeIndex{ -1, -1 } ) );

        return node._distance;
    }

    Indexes BattlePathfinder::getAllAvailableMoves( const Unit & unit )
    {
        reEvaluateIfNeeded( unit );

        std::set<int32_t> boardIndexes;

        for ( const auto & [index, node] : _cache ) {
            if ( index == _pathStart || node._from == BattleNodeIndex{ -1, -1 } || node._cost > _speed ) {
                continue;
            }

            assert( index.first != -1 );

            boardIndexes.insert( index.first );
        }

        Indexes result;

        result.reserve( boardIndexes.size() );
        result.assign( boardIndexes.begin(), boardIndexes.end() );

        return result;
    }

    Indexes BattlePathfinder::buildPath( const Unit & unit, const Position & position )
    {
        assert( position.GetHead() != nullptr );

        reEvaluateIfNeeded( unit );

        Indexes result;
        result.reserve( Speed::INSTANT );

        BattleNodeIndex lastReachableNodeIdx{ -1, -1 };
        BattleNodeIndex nodeIdx = { position.GetHead()->GetIndex(), position.GetTail() ? position.GetTail()->GetIndex() : -1 };

        for ( auto iter = _cache.find( nodeIdx ); iter != _cache.end(); iter = _cache.find( nodeIdx ) ) {
            const auto & [index, node] = *iter;

            if ( index == _pathStart ) {
                break;
            }

            // MSVC 2017 fails to properly expand the assert() macro without additional parentheses
            assert( ( node._from != BattleNodeIndex{ -1, -1 } ) );

            nodeIdx = node._from;

            // A given position may be reachable in principle, but is not reachable on the current turn.
            // Skip the steps that are not reachable on this turn.
            if ( node._cost > _speed ) {
                continue;
            }

            if ( _isWide && lastReachableNodeIdx == BattleNodeIndex{ -1, -1 } ) {
                assert( index.first != -1 && index.second != -1 );

                lastReachableNodeIdx = index;
            }

            result.push_back( index.first );
        }

        std::reverse( result.begin(), result.end() );

        // If a given position is not reachable on the current turn, then the last reachable position of
        // a wide unit may be reversed in regard to the target one. Detect this and add an extra U-turn.
        if ( _isWide && !result.empty() ) {
            assert( lastReachableNodeIdx.first != -1 && lastReachableNodeIdx.second != -1 );

            const bool isReflect = lastReachableNodeIdx.first < lastReachableNodeIdx.second;

            if ( isReflect != position.isReflect() ) {
                // The last reachable position should not be a reversed version of the target position
                assert( !position.contains( lastReachableNodeIdx.first ) || !position.contains( lastReachableNodeIdx.second ) );

                result.push_back( lastReachableNodeIdx.second );
            }
        }

        return result;
    }

    Position BattlePathfinder::getClosestReachablePosition( const Unit & unit, const Position & position )
    {
        assert( position.GetHead() != nullptr );

        reEvaluateIfNeeded( unit );

        BattleNodeIndex nodeIdx = { position.GetHead()->GetIndex(), position.GetTail() ? position.GetTail()->GetIndex() : -1 };

        for ( auto iter = _cache.find( nodeIdx ); iter != _cache.end(); iter = _cache.find( nodeIdx ) ) {
            const auto & [index, node] = *iter;

            if ( index == _pathStart ) {
                break;
            }

            // MSVC 2017 fails to properly expand the assert() macro without additional parentheses
            assert( ( node._from != BattleNodeIndex{ -1, -1 } ) );

            nodeIdx = node._from;

            // A given position may be reachable in principle, but is not reachable on the current turn.
            // Skip the steps that are not reachable on this turn.
            if ( node._cost > _speed ) {
                continue;
            }

            Position result;

            if ( _isWide ) {
                assert( index.first != -1 && index.second != -1 );

                const bool isReflect = index.first < index.second;

                result.Set( index.first, true, isReflect );

                assert( result.GetHead() != nullptr && result.GetHead()->GetIndex() == index.first && result.GetTail() != nullptr
                        && result.GetTail()->GetIndex() == index.second );

                // If a given position is not reachable on the current turn, then the last reachable position of
                // a wide unit may be reversed in regard to the target one. Detect this and turn it over.
                if ( isReflect != position.isReflect() ) {
                    // The last reachable position should not be a reversed version of the target position
                    assert( !position.contains( index.first ) || !position.contains( index.second ) );

                    result.Swap();
                }
            }
            else {
                result.Set( index.first, false, false );
            }

            return result;
        }

        return {};
    }
}
