/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <map>
#include <memory>
#include <ostream>
#include <set>
#include <utility>

#include "battle.h"
#include "battle_arena.h"
#include "battle_army.h"
#include "battle_board.h"
#include "battle_bridge.h"
#include "battle_troop.h"
#include "castle.h"
#include "game_static.h"
#include "ground.h"
#include "icn.h"
#include "logging.h"
#include "maps_tiles.h"
#include "mp2.h"
#include "rand.h"
#include "tools.h"
#include "translations.h"

namespace
{
    int GetRandomObstaclePosition( std::mt19937 & gen )
    {
        return Rand::GetWithGen( 3, 6, gen ) + ( 11 * Rand::GetWithGen( 1, 7, gen ) );
    }

    bool isTwoHexObject( const int icnId )
    {
        switch ( icnId ) {
        case ICN::COBJ0004:
        case ICN::COBJ0005:
        case ICN::COBJ0007:
        case ICN::COBJ0011:
        case ICN::COBJ0014:
        case ICN::COBJ0015:
        case ICN::COBJ0017:
        case ICN::COBJ0018:
        case ICN::COBJ0019:
        case ICN::COBJ0020:
        case ICN::COBJ0022:
        case ICN::COBJ0030:
        case ICN::COBJ0031:
            return true;

        default:
            break;
        }

        return false;
    }
}

Battle::Board::Board()
{
    reserve( ARENASIZE );
    for ( uint32_t ii = 0; ii < ARENASIZE; ++ii )
        push_back( Cell( ii ) );
}

void Battle::Board::SetArea( const fheroes2::Rect & area )
{
    for ( iterator it = begin(); it != end(); ++it )
        ( *it ).SetArea( area );
}

void Battle::Board::Reset()
{
    for ( iterator it = begin(); it != end(); ++it ) {
        Unit * unit = it->GetUnit();
        if ( unit && !unit->isValid() ) {
            unit->PostKilledAction();
        }
        it->resetReachability();
        it->ResetQuality();
    }
}

void Battle::Board::SetPositionQuality( const Unit & b ) const
{
    const Arena * arena = GetArena();
    assert( arena != nullptr );

    Units enemies( arena->getEnemyForce( b.GetCurrentColor() ).getUnits(), true );

    // Make sure archers are first here, so melee unit's score won't be double counted
    enemies.SortArchers();

    for ( const Unit * unit : enemies ) {
        if ( !unit || !unit->isValid() ) {
            continue;
        }

        const Indexes around = GetAroundIndexes( *unit );
        for ( const int32_t index : around ) {
            Cell * cell2 = GetCell( index );
            if ( !cell2 || !cell2->isPassableForUnit( b ) )
                continue;

            const int32_t quality = cell2->GetQuality();
            const int32_t attackValue = OptimalAttackValue( b, *unit, index );

            // Only sum up quality score if it's archers; otherwise just pick the highest
            if ( unit->isArchers() )
                cell2->SetQuality( quality + attackValue );
            else if ( attackValue > quality )
                cell2->SetQuality( attackValue );
        }
    }
}

void Battle::Board::SetEnemyQuality( const Unit & unit ) const
{
    const Arena * arena = GetArena();
    assert( arena != nullptr );

    Units enemies( arena->getEnemyForce( unit.GetColor() ).getUnits(), true );
    if ( unit.Modes( SP_BERSERKER ) ) {
        Units allies( arena->getForce( unit.GetColor() ).getUnits(), true );
        enemies.insert( enemies.end(), allies.begin(), allies.end() );
    }

    for ( Units::const_iterator it = enemies.begin(); it != enemies.end(); ++it ) {
        const Unit * enemy = *it;

        if ( enemy && enemy->isValid() ) {
            const int32_t score = enemy->GetScoreQuality( unit );
            Cell * cell = GetCell( enemy->GetHeadIndex() );

            cell->SetQuality( score );

            if ( enemy->isWide() )
                GetCell( enemy->GetTailIndex() )->SetQuality( score );

            DEBUG_LOG( DBG_BATTLE, DBG_TRACE, score << " for " << enemy->String() )
        }
    }
}

uint32_t Battle::Board::GetDistance( int32_t index1, int32_t index2 )
{
    if ( isValidIndex( index1 ) && isValidIndex( index2 ) ) {
        const int32_t x1 = index1 % ARENAW;
        const int32_t y1 = index1 / ARENAW;

        const int32_t x2 = index2 % ARENAW;
        const int32_t y2 = index2 / ARENAW;

        const int32_t du = y2 - y1;
        const int32_t dv = ( x2 + y2 / 2 ) - ( x1 + y1 / 2 );

        if ( ( du >= 0 && dv >= 0 ) || ( du < 0 && dv < 0 ) ) {
            return std::max( std::abs( du ), std::abs( dv ) );
        }
        else {
            return std::abs( du ) + std::abs( dv );
        }
    }

    return 0;
}

void Battle::Board::SetScanPassability( const Unit & unit )
{
    std::for_each( begin(), end(), []( Battle::Cell & cell ) { cell.resetReachability(); } );

    at( unit.GetHeadIndex() ).setReachableForHead();

    if ( unit.isWide() ) {
        at( unit.GetTailIndex() ).setReachableForTail();
    }

    if ( unit.isFlying() ) {
        const Bridge * bridge = Arena::GetBridge();
        const bool isPassableBridge = bridge == nullptr || bridge->isPassable( unit );

        for ( std::size_t i = 0; i < size(); ++i ) {
            if ( at( i ).isPassableForUnit( unit ) && ( isPassableBridge || !isBridgeIndex( static_cast<int32_t>( i ), unit ) ) ) {
                at( i ).setReachableForHead();

                if ( unit.isWide() ) {
                    at( i ).setReachableForTail();
                }
            }
        }
    }
    else {
        // Set passable cells.
        for ( const int32_t idx : GetDistanceIndexes( unit.GetHeadIndex(), unit.GetSpeed() ) ) {
            GetPath( unit, Position::GetPosition( unit, idx ), false );
        }
    }
}

bool Battle::Board::GetPathForUnit( const Unit & unit, const Position & destination, const uint32_t remainingSteps, const int32_t currentCellId,
                                    std::vector<bool> & visitedCells, Indexes & result ) const
{
    if ( remainingSteps == 0 ) {
        return false;
    }

    const Castle * castle = Arena::GetCastle();
    const bool isMoatBuilt = castle && castle->isBuild( BUILD_MOAT );

    const int32_t dstCellId = destination.GetHead()->GetIndex();

    // Upper distance limit
    if ( GetDistance( currentCellId, dstCellId ) > remainingSteps ) {
        return false;
    }

    std::multimap<uint32_t, int32_t> cellCosts;

    for ( const int32_t cellId : GetAroundIndexes( currentCellId ) ) {
        const Cell & cell = at( cellId );

        // Ignore already visited or impassable cell
        if ( visitedCells.at( cellId ) || !cell.isPassableFromAdjacent( unit, at( currentCellId ) ) ) {
            continue;
        }

        // Unit is already at its destination
        if ( cellId == dstCellId ) {
            result.push_back( cellId );

            return true;
        }

        // Unit steps into the moat, do not let it pass through the moat
        if ( isMoatBuilt && isMoatIndex( cellId, unit ) ) {
            continue;
        }

        // Calculate the distance from the cell in question to the destination, sort cells by distance
        cellCosts.emplace( GetDistance( cellId, dstCellId ), cellId );
    }

    // Scan the available cells recursively in ascending order of distance
    for ( const auto & cellCost : cellCosts ) {
        const int32_t cellId = cellCost.second;

        // Mark the cell as visited for further steps
        visitedCells.at( cellId ) = true;

        if ( GetPathForUnit( unit, destination, remainingSteps - 1, cellId, visitedCells, result ) ) {
            result.push_back( cellId );

            return true;
        }

        // Unmark the cell as visited
        visitedCells.at( cellId ) = false;
    }

    return false;
}

bool Battle::Board::GetPathForWideUnit( const Unit & unit, const Position & destination, const uint32_t remainingSteps, const int32_t currentHeadCellId,
                                        const int32_t prevHeadCellId, std::vector<bool> & visitedCells, Indexes & result ) const
{
    if ( remainingSteps == 0 ) {
        return false;
    }

    const Castle * castle = Arena::GetCastle();
    const bool isMoatBuilt = castle && castle->isBuild( BUILD_MOAT );

    const int32_t dstHeadCellId = destination.GetHead()->GetIndex();
    const int32_t dstTailCellId = destination.GetTail()->GetIndex();

    const bool isCurrentLeftDirection = prevHeadCellId < 0 ? unit.isReflect() : ( ( GetDirection( prevHeadCellId, currentHeadCellId ) & LEFT_SIDE ) != 0 );
    const int32_t currentTailCellId = isCurrentLeftDirection ? currentHeadCellId + 1 : currentHeadCellId - 1;

    // Upper distance limit
    if ( GetDistance( currentHeadCellId, dstHeadCellId ) > remainingSteps && GetDistance( currentTailCellId, dstHeadCellId ) > remainingSteps ) {
        return false;
    }

    std::multimap<uint32_t, int32_t> cellCosts;

    for ( const int32_t headCellId : GetMoveWideIndexes( currentHeadCellId, isCurrentLeftDirection ) ) {
        const Cell & cell = at( headCellId );

        // Ignore already visited or impassable cell
        if ( visitedCells.at( headCellId ) || !cell.isPassableFromAdjacent( unit, at( currentHeadCellId ) ) ) {
            continue;
        }

        const int32_t tailCellId = ( GetDirection( currentHeadCellId, headCellId ) & LEFT_SIDE ) ? headCellId + 1 : headCellId - 1;

        // Unit is already at its destination
        if ( headCellId == dstHeadCellId && tailCellId == dstTailCellId ) {
            result.push_back( headCellId );

            return true;
        }

        // Unit is already at its destination, but in the opposite direction
        if ( headCellId == dstTailCellId && tailCellId == dstHeadCellId ) {
            result.push_back( tailCellId );
            result.push_back( headCellId );

            return true;
        }

        // Unit steps into the moat
        if ( isMoatBuilt && ( isMoatIndex( headCellId, unit ) || isMoatIndex( tailCellId, unit ) ) ) {
            // In the moat it is only allowed to turn back, do not let the unit pass through the moat
            if ( ( tailCellId != currentHeadCellId || !isMoatIndex( tailCellId, unit ) ) && ( headCellId != currentTailCellId || !isMoatIndex( headCellId, unit ) ) ) {
                continue;
            }
        }

        // Calculate the distance from the cell in question to the destination, sort cells by distance
        cellCosts.emplace( GetDistance( headCellId, dstHeadCellId ) + GetDistance( tailCellId, dstTailCellId ), headCellId );
    }

    // Scan the available cells recursively in ascending order of distance
    for ( const auto & cellCost : cellCosts ) {
        const int32_t headCellId = cellCost.second;

        // Mark the cell as visited for further steps
        visitedCells.at( headCellId ) = true;

        // Turning back is not a movement
        const uint32_t steps = headCellId == currentTailCellId ? remainingSteps : remainingSteps - 1;

        if ( GetPathForWideUnit( unit, destination, steps, headCellId, currentHeadCellId, visitedCells, result ) ) {
            result.push_back( headCellId );

            return true;
        }

        // Unmark the cell as visited
        visitedCells.at( headCellId ) = false;
    }

    return false;
}

void Battle::Board::StraightenPathForUnit( const int32_t currentCellId, Indexes & path ) const
{
    // A path less than 2 steps long cannot contain detours, leave it as is
    if ( path.size() < 2 ) {
        return;
    }

    // Remember that the steps in the path are stored in reverse order
    // Temporarily append the current cell of the unit to the end of the path
    path.push_back( currentCellId );

    for ( std::size_t curr = 0; path.size() > 2 && curr < path.size() - 2; ++curr ) {
        const std::size_t next = curr + 1;

        // Check whether we are passing through one of the neighboring cells at any of the future steps (excluding the next step)
        for ( const int32_t cellId : GetAroundIndexes( path[curr] ) ) {
            std::size_t pos;

            // Search for the last occurence of the current neighboring cell in the path (excluding the next step)
            // Using path.size() - 1 should be safe here, because, due to the condition in the outer loop, path should never be empty
            assert( !path.empty() );
            for ( pos = path.size() - 1; pos > next; --pos ) {
                if ( path[pos] == cellId ) {
                    break;
                }
            }

            // If found, then remove the extra steps
            if ( pos > next ) {
                path.erase( path.begin() + next, path.begin() + pos );

                break;
            }
        }
    }

    // Remove the current cell of the unit from the path
    assert( !path.empty() );
    path.pop_back();
}

Battle::Indexes Battle::Board::GetPath( const Unit & unit, const Position & destination, const bool debug ) const
{
    Indexes result;

    const bool isWideUnit = unit.isWide();

    // Check if destination is valid
    if ( destination.GetHead() == nullptr || ( isWideUnit && destination.GetTail() == nullptr ) ) {
        return result;
    }

    result.reserve( 15 );

    std::vector<bool> visitedCells( ARENASIZE, false );

    // Mark the current cell of the unit as visited
    visitedCells.at( unit.GetHeadIndex() ) = true;

    if ( isWideUnit ) {
        GetPathForWideUnit( unit, destination, unit.GetSpeed(), unit.GetHeadIndex(), -1, visitedCells, result );
    }
    else {
        GetPathForUnit( unit, destination, unit.GetSpeed(), unit.GetHeadIndex(), visitedCells, result );

        // Try to straighten the unit's path by eliminating possible detours
        StraightenPathForUnit( unit.GetHeadIndex(), result );
    }

    if ( !result.empty() ) {
        std::reverse( result.begin(), result.end() );

        // Set direction info for cells
        for ( std::size_t i = 0; i < result.size(); ++i ) {
            const int32_t cellId = result[i];

            Cell * headCell = GetCell( cellId );
            assert( headCell != nullptr );

            headCell->setReachableForHead();

            if ( isWideUnit ) {
                const int32_t prevCellId = i == 0 ? unit.GetHeadIndex() : result[i - 1];

                Cell * tailCell = GetCell( cellId, LEFT_SIDE & GetDirection( cellId, prevCellId ) ? LEFT : RIGHT );
                assert( tailCell != nullptr );

                tailCell->setReachableForTail();
            }
        }
    }

    if ( debug && result.empty() ) {
        DEBUG_LOG( DBG_BATTLE, DBG_WARN,
                   "Path was not found for " << unit.String() << ", destination: "
                                             << "(head cell ID: " << destination.GetHead()->GetIndex()
                                             << ", tail cell ID: " << ( isWideUnit ? destination.GetTail()->GetIndex() : -1 ) << ")" )
    }

    return result;
}

std::vector<Battle::Unit *> Battle::Board::GetNearestTroops( const Unit * startUnit, const std::vector<Battle::Unit *> & blackList )
{
    std::vector<std::pair<Battle::Unit *, uint32_t>> foundUnits;

    for ( Cell & cell : *this ) {
        Unit * cellUnit = cell.GetUnit();
        if ( cellUnit == nullptr || startUnit == cellUnit || cell.GetIndex() != cellUnit->GetHeadIndex() ) {
            continue;
        }

        const bool isBlackListed = std::find( blackList.begin(), blackList.end(), cellUnit ) != blackList.end();
        if ( !isBlackListed ) {
            foundUnits.emplace_back( cellUnit, GetDistance( startUnit->GetHeadIndex(), cellUnit->GetHeadIndex() ) );
        }
    }

    std::sort( foundUnits.begin(), foundUnits.end(),
               []( const std::pair<Battle::Unit *, uint32_t> & first, const std::pair<Battle::Unit *, uint32_t> & second ) { return first.second < second.second; } );

    std::vector<Battle::Unit *> units;
    units.reserve( foundUnits.size() );

    for ( const auto & foundUnit : foundUnits ) {
        units.push_back( foundUnit.first );
    }

    return units;
}

int32_t Battle::Board::DoubleCellAttackValue( const Unit & attacker, const Unit & target, const int32_t from, const int32_t targetCell )
{
    const Cell * behind = GetCell( targetCell, GetDirection( from, targetCell ) );
    const Unit * secondaryTarget = ( behind != nullptr ) ? behind->GetUnit() : nullptr;
    if ( secondaryTarget && secondaryTarget->GetUID() != target.GetUID() && secondaryTarget->GetUID() != attacker.GetUID() ) {
        return secondaryTarget->GetScoreQuality( attacker );
    }
    return 0;
}

int32_t Battle::Board::OptimalAttackTarget( const Unit & attacker, const Unit & target, const int32_t from )
{
    const int32_t headIndex = target.GetHeadIndex();
    const int32_t tailIndex = target.GetTailIndex();

    // isNearIndexes should return false if we pass in invalid tail index (-1)
    if ( isNearIndexes( from, tailIndex ) ) {
        if ( attacker.isDoubleCellAttack() && isNearIndexes( from, headIndex )
             && DoubleCellAttackValue( attacker, target, from, headIndex ) > DoubleCellAttackValue( attacker, target, from, tailIndex ) ) {
            // Special case when attacking wide unit from the middle cell and could turn around
            return headIndex;
        }
        return tailIndex;
    }
    return headIndex;
}

int32_t Battle::Board::OptimalAttackValue( const Unit & attacker, const Unit & target, const int32_t from )
{
    if ( attacker.isDoubleCellAttack() ) {
        const int32_t targetCell = OptimalAttackTarget( attacker, target, from );
        return target.GetScoreQuality( attacker ) + DoubleCellAttackValue( attacker, target, from, targetCell );
    }

    if ( attacker.isAllAdjacentCellsAttack() ) {
        Position position = Position::GetPosition( attacker, from );

        if ( position.GetHead() == nullptr || ( attacker.isWide() && position.GetTail() == nullptr ) ) {
            DEBUG_LOG( DBG_BATTLE, DBG_WARN, "Invalid position for " << attacker.String() << ", target: " << target.String() << ", cell: " << from )

            return 0;
        }

        Indexes aroundAttacker = GetAroundIndexes( position );

        std::set<const Unit *> unitsUnderAttack;
        Board * board = Arena::GetBoard();
        for ( const int32_t index : aroundAttacker ) {
            const Unit * unit = board->at( index ).GetUnit();
            if ( unit != nullptr && unit->GetColor() != attacker.GetCurrentColor() ) {
                unitsUnderAttack.insert( unit );
            }
        }

        int32_t attackValue = 0;
        for ( const Unit * unit : unitsUnderAttack ) {
            attackValue += unit->GetScoreQuality( attacker );
        }

        return attackValue;
    }

    return target.GetScoreQuality( attacker );
}

int Battle::Board::GetDirection( int32_t index1, int32_t index2 )
{
    if ( isValidIndex( index1 ) && isValidIndex( index2 ) ) {
        if ( index1 == index2 )
            return CENTER;
        else
            for ( direction_t dir = TOP_LEFT; dir < CENTER; ++dir )
                if ( isValidDirection( index1, dir ) && index2 == GetIndexDirection( index1, dir ) )
                    return dir;
    }

    return UNKNOWN;
}

bool Battle::Board::isNearIndexes( int32_t index1, int32_t index2 )
{
    return index1 != index2 && UNKNOWN != GetDirection( index1, index2 );
}

int Battle::Board::GetReflectDirection( int d )
{
    switch ( d ) {
    case TOP_LEFT:
        return BOTTOM_RIGHT;
    case TOP_RIGHT:
        return BOTTOM_LEFT;
    case LEFT:
        return RIGHT;
    case RIGHT:
        return LEFT;
    case BOTTOM_LEFT:
        return TOP_RIGHT;
    case BOTTOM_RIGHT:
        return TOP_LEFT;
    default:
        break;
    }

    return UNKNOWN;
}

bool Battle::Board::IsLeftDirection( const int32_t startCellId, const int32_t endCellId, const bool prevLeftDirection )
{
    const int startX = startCellId % ARENAW;
    const int endX = endCellId % ARENAW;

    if ( prevLeftDirection )
        return endX <= startX;
    else
        return endX < startX;
}

bool Battle::Board::isNegativeDistance( int32_t index1, int32_t index2 )
{
    return ( index1 % ARENAW ) - ( index2 % ARENAW ) < 0;
}

int Battle::Board::DistanceFromOriginX( int32_t index, bool reflect )
{
    const int xPos = index % ARENAW;
    return std::max( 1, reflect ? ARENAW - xPos - 1 : xPos );
}

bool Battle::Board::isValidDirection( int32_t index, int dir )
{
    if ( isValidIndex( index ) ) {
        const int32_t x = index % ARENAW;
        const int32_t y = index / ARENAW;

        switch ( dir ) {
        case CENTER:
            return true;
        case TOP_LEFT:
            return !( 0 == y || ( 0 == x && ( y % 2 ) ) );
        case TOP_RIGHT:
            return !( 0 == y || ( ( ARENAW - 1 ) == x && !( y % 2 ) ) );
        case LEFT:
            return !( 0 == x );
        case RIGHT:
            return !( ( ARENAW - 1 ) == x );
        case BOTTOM_LEFT:
            return !( ( ARENAH - 1 ) == y || ( 0 == x && ( y % 2 ) ) );
        case BOTTOM_RIGHT:
            return !( ( ARENAH - 1 ) == y || ( ( ARENAW - 1 ) == x && !( y % 2 ) ) );
        default:
            break;
        }
    }

    return false;
}

int32_t Battle::Board::GetIndexDirection( int32_t index, int dir )
{
    if ( isValidIndex( index ) ) {
        const int32_t y = index / ARENAW;

        switch ( dir ) {
        case CENTER:
            return index;
        case TOP_LEFT:
            return index - ( ( y % 2 ) ? ARENAW + 1 : ARENAW );
        case TOP_RIGHT:
            return index - ( ( y % 2 ) ? ARENAW : ARENAW - 1 );
        case LEFT:
            return index - 1;
        case RIGHT:
            return index + 1;
        case BOTTOM_LEFT:
            return index + ( ( y % 2 ) ? ARENAW - 1 : ARENAW );
        case BOTTOM_RIGHT:
            return index + ( ( y % 2 ) ? ARENAW : ARENAW + 1 );
        default:
            break;
        }
    }

    return -1;
}

int32_t Battle::Board::GetIndexAbsPosition( const fheroes2::Point & pt ) const
{
    const_iterator it = begin();

    for ( ; it != end(); ++it )
        if ( ( *it ).isPositionIncludePoint( pt ) )
            break;

    return it != end() ? ( *it ).GetIndex() : -1;
}

bool Battle::Board::isValidIndex( int32_t index )
{
    return 0 <= index && index < ARENASIZE;
}

bool Battle::Board::isCastleIndex( int32_t index )
{
    return ( ( 8 < index && index <= 10 ) || ( 19 < index && index <= 21 ) || ( 29 < index && index <= 32 ) || ( 40 < index && index <= 43 )
             || ( 50 < index && index <= 54 ) || ( 62 < index && index <= 65 ) || ( 73 < index && index <= 76 ) || ( 85 < index && index <= 87 )
             || ( 96 < index && index <= 98 ) );
}

bool Battle::Board::isOutOfWallsIndex( int32_t index )
{
    return ( ( index <= 8 ) || ( 11 <= index && index <= 19 ) || ( 22 <= index && index <= 29 ) || ( 33 <= index && index <= 40 ) || ( 44 <= index && index <= 50 )
             || ( 55 <= index && index <= 62 ) || ( 66 <= index && index <= 73 ) || ( 77 <= index && index <= 85 ) || ( 88 <= index && index <= 96 ) );
}

bool Battle::Board::isBridgeIndex( int32_t index, const Unit & b )
{
    const Bridge * bridge = Arena::GetBridge();

    return ( index == 49 && !b.isFlying() && bridge && bridge->isPassable( b ) ) || index == 50;
}

bool Battle::Board::isMoatIndex( int32_t index, const Unit & b )
{
    switch ( index ) {
    case 7:
    case 18:
    case 28:
    case 39:
    case 61:
    case 72:
    case 84:
    case 95:
        return true;
    case 49: {
        const Bridge * bridge = Arena::GetBridge();
        return b.isFlying() || bridge == nullptr || !bridge->isPassable( b );
    }

    default:
        break;
    }

    return false;
}

void Battle::Board::SetCobjObjects( const Maps::Tiles & tile, std::mt19937 & gen )
{
    bool grave = MP2::OBJ_GRAVEYARD == tile.GetObject( false );
    int ground = tile.GetGround();
    std::vector<int> objs;

    if ( grave ) {
        objs.push_back( ICN::COBJ0000 );
        objs.push_back( ICN::COBJ0001 );
        objs.push_back( ICN::COBJ0025 );
    }
    else
        switch ( ground ) {
        case Maps::Ground::DESERT:
            objs.push_back( ICN::COBJ0009 );
            objs.push_back( ICN::COBJ0024 );
            break;

        case Maps::Ground::SNOW:
            objs.push_back( ICN::COBJ0022 );
            objs.push_back( ICN::COBJ0026 );
            break;

        case Maps::Ground::SWAMP:
            objs.push_back( ICN::COBJ0005 );
            objs.push_back( ICN::COBJ0006 );
            objs.push_back( ICN::COBJ0007 );
            objs.push_back( ICN::COBJ0008 );
            objs.push_back( ICN::COBJ0011 );
            objs.push_back( ICN::COBJ0012 );
            objs.push_back( ICN::COBJ0014 );
            objs.push_back( ICN::COBJ0015 );
            objs.push_back( ICN::COBJ0016 );
            objs.push_back( ICN::COBJ0017 );
            objs.push_back( ICN::COBJ0027 );
            break;

        case Maps::Ground::BEACH:
            objs.push_back( ICN::COBJ0005 );
            objs.push_back( ICN::COBJ0011 );
            objs.push_back( ICN::COBJ0017 );
            break;

        case Maps::Ground::DIRT:
            objs.push_back( ICN::COBJ0002 );
            objs.push_back( ICN::COBJ0005 );
            objs.push_back( ICN::COBJ0007 );
            objs.push_back( ICN::COBJ0011 );
            objs.push_back( ICN::COBJ0014 );
            objs.push_back( ICN::COBJ0019 );
            objs.push_back( ICN::COBJ0027 );
            break;

        case Maps::Ground::GRASS:
            objs.push_back( ICN::COBJ0002 );
            objs.push_back( ICN::COBJ0004 );
            objs.push_back( ICN::COBJ0005 );
            objs.push_back( ICN::COBJ0008 );
            objs.push_back( ICN::COBJ0011 );
            objs.push_back( ICN::COBJ0012 );
            objs.push_back( ICN::COBJ0014 );
            objs.push_back( ICN::COBJ0015 );
            objs.push_back( ICN::COBJ0019 );
            objs.push_back( ICN::COBJ0027 );
            objs.push_back( ICN::COBJ0028 );
            break;

        case Maps::Ground::WASTELAND:
            objs.push_back( ICN::COBJ0009 );
            objs.push_back( ICN::COBJ0013 );
            objs.push_back( ICN::COBJ0018 );
            objs.push_back( ICN::COBJ0020 );
            objs.push_back( ICN::COBJ0021 );
            objs.push_back( ICN::COBJ0024 );
            break;

        case Maps::Ground::LAVA:
            objs.push_back( ICN::COBJ0007 );
            objs.push_back( ICN::COBJ0029 );
            objs.push_back( ICN::COBJ0031 );
            break;

        case Maps::Ground::WATER:
            objs.push_back( ICN::COBJ0003 );
            objs.push_back( ICN::COBJ0010 );
            objs.push_back( ICN::COBJ0023 );
            break;

        default:
            break;
        }

    Rand::ShuffleWithGen( objs, gen );

    const size_t objectsToPlace = std::min( objs.size(), static_cast<size_t>( Rand::GetWithGen( 0, 4, gen ) ) );

    for ( size_t i = 0; i < objectsToPlace; ++i ) {
        const bool checkRightCell = isTwoHexObject( objs[i] );

        int32_t dest = GetRandomObstaclePosition( gen );
        while ( at( dest ).GetObject() != 0 || ( checkRightCell && at( dest + 1 ).GetObject() != 0 ) ) {
            dest = GetRandomObstaclePosition( gen );
        }

        SetCobjObject( objs[i], dest );
    }
}

void Battle::Board::SetCobjObject( const int icn, const int32_t dst )
{
    at( dst ).SetObject( 0x80 + ( icn - ICN::COBJ0000 ) );

    if ( isTwoHexObject( icn ) ) {
        assert( at( dst + 1 ).GetObject() == 0 );
        at( dst + 1 ).SetObject( 0x40 );
    }
}

void Battle::Board::SetCovrObjects( int icn )
{
    switch ( icn ) {
    case ICN::COVR0001:
    case ICN::COVR0007:
    case ICN::COVR0013:
    case ICN::COVR0019:
        at( 25 ).SetObject( 0x40 );
        at( 26 ).SetObject( 0x40 );
        at( 27 ).SetObject( 0x40 );
        at( 28 ).SetObject( 0x40 );
        at( 40 ).SetObject( 0x40 );
        at( 51 ).SetObject( 0x40 );
        break;

    case ICN::COVR0002:
    case ICN::COVR0008:
    case ICN::COVR0014:
    case ICN::COVR0020:
        at( 47 ).SetObject( 0x40 );
        at( 48 ).SetObject( 0x40 );
        at( 49 ).SetObject( 0x40 );
        at( 50 ).SetObject( 0x40 );
        at( 51 ).SetObject( 0x40 );
        break;

    case ICN::COVR0003:
    case ICN::COVR0015:
    case ICN::COVR0021:
        at( 35 ).SetObject( 0x40 );
        at( 41 ).SetObject( 0x40 );
        at( 46 ).SetObject( 0x40 );
        at( 47 ).SetObject( 0x40 );
        at( 48 ).SetObject( 0x40 );
        at( 49 ).SetObject( 0x40 );
        at( 50 ).SetObject( 0x40 );
        at( 51 ).SetObject( 0x40 );
        break;

    case ICN::COVR0009:
        at( 35 ).SetObject( 0x40 );
        at( 40 ).SetObject( 0x40 );
        at( 46 ).SetObject( 0x40 );
        at( 47 ).SetObject( 0x40 );
        at( 48 ).SetObject( 0x40 );
        at( 49 ).SetObject( 0x40 );
        at( 50 ).SetObject( 0x40 );
        break;

    case ICN::COVR0004:
    case ICN::COVR0010:
    case ICN::COVR0016:
    case ICN::COVR0022:
        at( 41 ).SetObject( 0x40 );
        at( 51 ).SetObject( 0x40 );
        at( 58 ).SetObject( 0x40 );
        at( 59 ).SetObject( 0x40 );
        at( 60 ).SetObject( 0x40 );
        at( 61 ).SetObject( 0x40 );
        at( 62 ).SetObject( 0x40 );
        break;

    case ICN::COVR0005:
    case ICN::COVR0017:
        at( 24 ).SetObject( 0x40 );
        at( 25 ).SetObject( 0x40 );
        at( 26 ).SetObject( 0x40 );
        at( 27 ).SetObject( 0x40 );
        at( 28 ).SetObject( 0x40 );
        at( 29 ).SetObject( 0x40 );
        at( 30 ).SetObject( 0x40 );
        at( 58 ).SetObject( 0x40 );
        at( 59 ).SetObject( 0x40 );
        at( 60 ).SetObject( 0x40 );
        at( 61 ).SetObject( 0x40 );
        at( 62 ).SetObject( 0x40 );
        at( 63 ).SetObject( 0x40 );
        at( 68 ).SetObject( 0x40 );
        at( 74 ).SetObject( 0x40 );
        break;

    case ICN::COVR0006:
    case ICN::COVR0018:
        at( 14 ).SetObject( 0x40 );
        at( 15 ).SetObject( 0x40 );
        at( 16 ).SetObject( 0x40 );
        at( 17 ).SetObject( 0x40 );
        at( 18 ).SetObject( 0x40 );
        at( 24 ).SetObject( 0x40 );
        at( 68 ).SetObject( 0x40 );
        at( 80 ).SetObject( 0x40 );
        at( 81 ).SetObject( 0x40 );
        at( 82 ).SetObject( 0x40 );
        at( 83 ).SetObject( 0x40 );
        at( 84 ).SetObject( 0x40 );
        break;

    case ICN::COVR0011:
    case ICN::COVR0023:
        at( 15 ).SetObject( 0x40 );
        at( 25 ).SetObject( 0x40 );
        at( 36 ).SetObject( 0x40 );
        at( 51 ).SetObject( 0x40 );
        at( 62 ).SetObject( 0x40 );
        at( 71 ).SetObject( 0x40 );
        at( 72 ).SetObject( 0x40 );
        break;

    case ICN::COVR0012:
    case ICN::COVR0024:
        at( 18 ).SetObject( 0x40 );
        at( 29 ).SetObject( 0x40 );
        at( 41 ).SetObject( 0x40 );
        at( 59 ).SetObject( 0x40 );
        at( 70 ).SetObject( 0x40 );
        at( 82 ).SetObject( 0x40 );
        at( 83 ).SetObject( 0x40 );
        break;

    default:
        break;
    }
}

Battle::Cell * Battle::Board::GetCell( int32_t position, int dir )
{
    if ( isValidIndex( position ) && dir != UNKNOWN ) {
        Board * board = Arena::GetBoard();
        if ( dir == CENTER ) {
            return &board->at( position );
        }
        if ( isValidDirection( position, dir ) ) {
            return &board->at( GetIndexDirection( position, dir ) );
        }
    }

    return nullptr;
}

Battle::Indexes Battle::Board::GetMoveWideIndexes( int32_t center, bool reflect )
{
    Indexes result;

    if ( isValidIndex( center ) ) {
        result.reserve( 4 );

        if ( reflect ) {
            if ( isValidDirection( center, LEFT ) )
                result.push_back( GetIndexDirection( center, LEFT ) );
            if ( isValidDirection( center, RIGHT ) )
                result.push_back( GetIndexDirection( center, RIGHT ) );
            if ( isValidDirection( center, TOP_LEFT ) )
                result.push_back( GetIndexDirection( center, TOP_LEFT ) );
            if ( isValidDirection( center, BOTTOM_LEFT ) )
                result.push_back( GetIndexDirection( center, BOTTOM_LEFT ) );
        }
        else {
            if ( isValidDirection( center, LEFT ) )
                result.push_back( GetIndexDirection( center, LEFT ) );
            if ( isValidDirection( center, RIGHT ) )
                result.push_back( GetIndexDirection( center, RIGHT ) );
            if ( isValidDirection( center, TOP_RIGHT ) )
                result.push_back( GetIndexDirection( center, TOP_RIGHT ) );
            if ( isValidDirection( center, BOTTOM_RIGHT ) )
                result.push_back( GetIndexDirection( center, BOTTOM_RIGHT ) );
        }
    }
    return result;
}

Battle::Indexes Battle::Board::GetAroundIndexes( int32_t center, int32_t ignore )
{
    Indexes result;

    if ( isValidIndex( center ) ) {
        result.reserve( 12 );

        for ( direction_t dir = TOP_LEFT; dir < CENTER; ++dir )
            if ( isValidDirection( center, dir ) && GetIndexDirection( center, dir ) != ignore )
                result.push_back( GetIndexDirection( center, dir ) );
    }

    return result;
}

Battle::Indexes Battle::Board::GetAroundIndexes( const Unit & b )
{
    return GetAroundIndexes( b.GetPosition() );
}

Battle::Indexes Battle::Board::GetAroundIndexes( const Position & position )
{
    const int headIdx = position.GetHead()->GetIndex();

    if ( position.GetTail() ) {
        const int tailIdx = position.GetTail()->GetIndex();

        Indexes around = GetAroundIndexes( headIdx, tailIdx );
        const Indexes & tail = GetAroundIndexes( tailIdx, headIdx );
        around.insert( around.end(), tail.begin(), tail.end() );

        std::sort( around.begin(), around.end() );
        around.erase( std::unique( around.begin(), around.end() ), around.end() );

        return around;
    }

    return GetAroundIndexes( headIdx );
}

Battle::Indexes Battle::Board::GetDistanceIndexes( int32_t center, uint32_t radius )
{
    Indexes result;

    if ( isValidIndex( center ) ) {
        std::set<int32_t> st;
        Indexes abroad;

        st.insert( center );
        abroad.push_back( center );

        while ( !abroad.empty() && radius ) {
            std::set<int32_t> tm = st;

            for ( Indexes::const_iterator it = abroad.begin(); it != abroad.end(); ++it ) {
                const Indexes around = GetAroundIndexes( *it );
                tm.insert( around.begin(), around.end() );
            }

            abroad.resize( tm.size() );

            Indexes::iterator abroad_end = std::set_difference( tm.begin(), tm.end(), st.begin(), st.end(), abroad.begin() );

            abroad.resize( std::distance( abroad.begin(), abroad_end ) );

            st.swap( tm );
            --radius;
        }

        st.erase( center );
        result.reserve( st.size() );
        std::copy( st.begin(), st.end(), std::back_inserter( result ) );
    }

    return result;
}

bool Battle::Board::isValidMirrorImageIndex( const int32_t index, const Unit * unit )
{
    if ( unit == nullptr ) {
        return false;
    }

    const Position mirrorPos = Position::GetPosition( *unit, index );

    if ( mirrorPos.GetHead() == nullptr || ( unit->isWide() && mirrorPos.GetTail() == nullptr ) ) {
        return false;
    }

    if ( unit->GetPosition().contains( mirrorPos.GetHead()->GetIndex() ) ) {
        return false;
    }
    if ( unit->isWide() && unit->GetPosition().contains( mirrorPos.GetTail()->GetIndex() ) ) {
        return false;
    }

    return true;
}

bool Battle::Board::CanAttackFromCell( const Unit & currentUnit, const int32_t from )
{
    const Cell * fromCell = GetCell( from );
    assert( fromCell != nullptr );

    // Target unit cannot be attacked if out of reach
    if ( !fromCell->isReachableForHead() && ( !currentUnit.isWide() || !fromCell->isReachableForTail() ) ) {
        return false;
    }

    const Castle * castle = Arena::GetCastle();

    // No moat - no further restrictions
    if ( !castle || !castle->isBuild( BUILD_MOAT ) ) {
        return true;
    }

    // Target unit isn't attacked from the moat
    if ( !isMoatIndex( from, currentUnit ) ) {
        return true;
    }

    // The moat doesn't stop flying units
    if ( currentUnit.isFlying() ) {
        return true;
    }

    // Attacker is already near the target
    if ( from == currentUnit.GetHeadIndex() || ( currentUnit.isWide() && from == currentUnit.GetTailIndex() ) ) {
        return true;
    }

    // In all other cases, the attack is prohibited
    return false;
}

bool Battle::Board::CanAttackTargetFromPosition( const Unit & currentUnit, const Unit & target, const int32_t dst )
{
    // Get the actual position of the attacker before attacking
    const Position pos = Position::GetReachable( currentUnit, dst );

    // Check that the attacker is actually capable of attacking the target from this position
    const std::array<const Cell *, 2> cells = { pos.GetHead(), pos.GetTail() };

    for ( const Cell * cell : cells ) {
        if ( cell == nullptr ) {
            continue;
        }

        if ( !CanAttackFromCell( currentUnit, cell->GetIndex() ) ) {
            continue;
        }

        for ( const int32_t nearbyIdx : GetAroundIndexes( cell->GetIndex() ) ) {
            const Cell * nearbyCell = GetCell( nearbyIdx );
            assert( nearbyCell != nullptr );

            if ( nearbyCell->GetUnit() == &target ) {
                return true;
            }
        }
    }

    return false;
}

Battle::Indexes Battle::Board::GetAdjacentEnemies( const Unit & unit )
{
    Indexes result;
    const bool isWide = unit.isWide();
    const int currentColor = unit.GetArmyColor();
    result.reserve( isWide ? 8 : 6 );

    const int leftmostIndex = ( isWide && !unit.isReflect() ) ? unit.GetTailIndex() : unit.GetHeadIndex();
    const int x = leftmostIndex % ARENAW;
    const int y = leftmostIndex / ARENAW;
    const int mod = y % 2;

    auto validateAndInsert = [&result, &currentColor]( const int index ) {
        const Unit * vUnit = GetCell( index )->GetUnit();
        if ( vUnit && currentColor != vUnit->GetArmyColor() )
            result.push_back( index );
    };

    if ( y > 0 ) {
        const int topRowIndex = ( y - 1 ) * ARENAW + x - mod;
        if ( x - mod >= 0 )
            validateAndInsert( topRowIndex );

        if ( x < ARENAW - 1 )
            validateAndInsert( topRowIndex + 1 );

        if ( isWide && x < ARENAW - 2 )
            validateAndInsert( topRowIndex + 2 );
    }

    if ( x > 0 )
        validateAndInsert( leftmostIndex - 1 );

    if ( x < ARENAW - ( isWide ? 2 : 1 ) )
        validateAndInsert( leftmostIndex + ( isWide ? 2 : 1 ) );

    if ( y < ARENAH - 1 ) {
        const int bottomRowIndex = ( y + 1 ) * ARENAW + x - mod;
        if ( x - mod >= 0 )
            validateAndInsert( bottomRowIndex );

        if ( x < ARENAW - 1 )
            validateAndInsert( bottomRowIndex + 1 );

        if ( isWide && x < ARENAW - 2 )
            validateAndInsert( bottomRowIndex + 2 );
    }

    return result;
}

std::string Battle::Board::GetMoatInfo()
{
    std::string msg = _( "The Moat reduces by -%{count} the defense skill of any unit and slows to half movement rate." );
    StringReplace( msg, "%{count}", GameStatic::GetBattleMoatReduceDefense() );

    return msg;
}
