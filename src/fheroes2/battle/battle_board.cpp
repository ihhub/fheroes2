/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
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

#include "battle_board.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <set>
#include <utility>

#include "battle_arena.h"
#include "battle_bridge.h"
#include "battle_troop.h"
#include "castle.h"
#include "game_static.h"
#include "ground.h"
#include "icn.h"
#include "maps_tiles.h"
#include "mp2.h"
#include "rand.h"
#include "tools.h"
#include "translations.h"

namespace
{
    uint32_t GetRandomObstaclePosition( std::mt19937 & gen )
    {
        return Rand::GetWithGen( 2, 8, gen ) + ( 11 * Rand::GetWithGen( 0, 8, gen ) );
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

    bool isTallObject( const int icnId )
    {
        switch ( icnId ) {
        case ICN::COBJ0002:
        case ICN::COBJ0009:
        case ICN::COBJ0013:
        case ICN::COBJ0021:
        case ICN::COBJ0027:
        case ICN::COBJ0028:
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
    reserve( sizeInCells );

    for ( int i = 0; i < sizeInCells; ++i ) {
        emplace_back( i );
    }
}

void Battle::Board::SetArea( const fheroes2::Rect & area )
{
    std::for_each( begin(), end(), [&area]( Cell & cell ) { cell.SetArea( area ); } );
}

void Battle::Board::removeDeadUnits()
{
    for ( Cell & cell : *this ) {
        Unit * unit = cell.GetUnit();

        if ( unit && !unit->isValid() ) {
            unit->PostKilledAction();
        }
    }
}

uint32_t Battle::Board::GetDistance( const int32_t index1, const int32_t index2 )
{
    if ( !isValidIndex( index1 ) || !isValidIndex( index2 ) ) {
        return 0;
    }

    const int32_t x1 = index1 % widthInCells;
    const int32_t y1 = index1 / widthInCells;

    const int32_t x2 = index2 % widthInCells;
    const int32_t y2 = index2 / widthInCells;

    const int32_t du = y2 - y1;
    const int32_t dv = ( x2 + y2 / 2 ) - ( x1 + y1 / 2 );

    if ( ( du >= 0 && dv >= 0 ) || ( du < 0 && dv < 0 ) ) {
        return std::max( std::abs( du ), std::abs( dv ) );
    }

    return std::abs( du ) + std::abs( dv );
}

uint32_t Battle::Board::GetDistance( const Position & pos1, const Position & pos2 )
{
    if ( pos1.GetHead() == nullptr || pos2.GetHead() == nullptr ) {
        return 0;
    }

    const int32_t head1Idx = pos1.GetHead()->GetIndex();
    const int32_t tail1Idx = pos1.GetTail() ? pos1.GetTail()->GetIndex() : -1;

    const int32_t head2Idx = pos2.GetHead()->GetIndex();
    const int32_t tail2Idx = pos2.GetTail() ? pos2.GetTail()->GetIndex() : -1;

    uint32_t distance = Board::GetDistance( head1Idx, head2Idx );

    if ( tail2Idx != -1 ) {
        distance = std::min( distance, Board::GetDistance( head1Idx, tail2Idx ) );
    }

    if ( tail1Idx != -1 ) {
        distance = std::min( distance, Board::GetDistance( tail1Idx, head2Idx ) );

        if ( tail2Idx != -1 ) {
            distance = std::min( distance, Board::GetDistance( tail1Idx, tail2Idx ) );
        }
    }

    return distance;
}

uint32_t Battle::Board::GetDistance( const Position & pos, const int32_t index )
{
    if ( pos.GetHead() == nullptr || !isValidIndex( index ) ) {
        return 0;
    }

    const int32_t headIdx = pos.GetHead()->GetIndex();
    const int32_t tailIdx = pos.GetTail() ? pos.GetTail()->GetIndex() : -1;

    uint32_t distance = Board::GetDistance( headIdx, index );

    if ( tailIdx != -1 ) {
        distance = std::min( distance, Board::GetDistance( tailIdx, index ) );
    }

    return distance;
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
            foundUnits.emplace_back( cellUnit, GetDistance( startUnit->GetPosition(), cellUnit->GetPosition() ) );
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

int Battle::Board::GetDirection( const int32_t index1, const int32_t index2 )
{
    if ( !isValidIndex( index1 ) || !isValidIndex( index2 ) ) {
        return UNKNOWN;
    }

    if ( index1 == index2 ) {
        return CENTER;
    }

    for ( CellDirection dir = TOP_LEFT; dir < CENTER; ++dir ) {
        if ( isValidDirection( index1, dir ) && index2 == GetIndexDirection( index1, dir ) ) {
            return dir;
        }
    }

    return UNKNOWN;
}

bool Battle::Board::isNearIndexes( const int32_t index1, const int32_t index2 )
{
    return index1 != index2 && UNKNOWN != GetDirection( index1, index2 );
}

int Battle::Board::GetReflectDirection( const int dir )
{
    switch ( dir ) {
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

uint32_t Battle::Board::GetDistanceFromBoardEdgeAlongXAxis( const int32_t index, const bool fromRightEdge )
{
    assert( isValidIndex( index ) );

    const uint32_t x = index % widthInCells;

    return ( fromRightEdge ? widthInCells - x : x + 1 );
}

bool Battle::Board::isValidDirection( const int32_t index, const int dir )
{
    if ( !isValidIndex( index ) ) {
        return false;
    }

    const int32_t x = index % widthInCells;
    const int32_t y = index / widthInCells;

    switch ( dir ) {
    case CENTER:
        return true;
    case TOP_LEFT:
        return !( 0 == y || ( 0 == x && ( y % 2 ) ) );
    case TOP_RIGHT:
        return !( 0 == y || ( ( widthInCells - 1 ) == x && !( y % 2 ) ) );
    case LEFT:
        return !( 0 == x );
    case RIGHT:
        return !( ( widthInCells - 1 ) == x );
    case BOTTOM_LEFT:
        return !( ( heightInCells - 1 ) == y || ( 0 == x && ( y % 2 ) ) );
    case BOTTOM_RIGHT:
        return !( ( heightInCells - 1 ) == y || ( ( widthInCells - 1 ) == x && !( y % 2 ) ) );
    default:
        break;
    }

    return false;
}

int32_t Battle::Board::GetIndexDirection( const int32_t index, const int dir )
{
    if ( !isValidIndex( index ) ) {
        return -1;
    }

    const int32_t y = index / widthInCells;

    switch ( dir ) {
    case CENTER:
        return index;
    case TOP_LEFT:
        return index - ( ( y % 2 ) ? widthInCells + 1 : widthInCells );
    case TOP_RIGHT:
        return index - ( ( y % 2 ) ? widthInCells : widthInCells - 1 );
    case LEFT:
        return index - 1;
    case RIGHT:
        return index + 1;
    case BOTTOM_LEFT:
        return index + ( ( y % 2 ) ? widthInCells - 1 : widthInCells );
    case BOTTOM_RIGHT:
        return index + ( ( y % 2 ) ? widthInCells : widthInCells + 1 );
    default:
        break;
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

bool Battle::Board::isValidIndex( const int32_t index )
{
    return 0 <= index && index < sizeInCells;
}

bool Battle::Board::isCastleIndex( const int32_t index )
{
    return ( ( 8 <= index && index <= 10 ) || ( 19 < index && index <= 21 ) || ( 29 <= index && index <= 32 ) || ( 40 < index && index <= 43 )
             || ( 50 < index && index <= 54 ) || ( 62 < index && index <= 65 ) || ( 73 <= index && index <= 76 ) || ( 85 < index && index <= 87 )
             || ( 96 <= index && index <= 98 ) );
}

bool Battle::Board::isOutOfWallsIndex( const int32_t index )
{
    return ( ( index <= 8 ) || ( 11 <= index && index <= 19 ) || ( 22 <= index && index <= 29 ) || ( 33 <= index && index <= 40 ) || ( 44 <= index && index <= 50 )
             || ( 55 <= index && index <= 62 ) || ( 66 <= index && index <= 73 ) || ( 77 <= index && index <= 85 ) || ( 88 <= index && index <= 96 ) );
}

bool Battle::Board::isMoatIndex( const int32_t index, const Unit & unit )
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
        return unit.isFlying() || bridge == nullptr || !bridge->isPassable( unit );
    }

    default:
        break;
    }

    return false;
}

void Battle::Board::SetCobjObjects( const Maps::Tile & tile, std::mt19937 & gen )
{
    std::vector<int> objs;

    if ( tile.getMainObjectType( false ) == MP2::OBJ_GRAVEYARD ) {
        objs.push_back( ICN::COBJ0000 );
        objs.push_back( ICN::COBJ0001 );
        objs.push_back( ICN::COBJ0025 );
    }
    else {
        switch ( tile.GetGround() ) {
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
    }

    Rand::ShuffleWithGen( objs, gen );

    const auto largeObstacleHexCount = std::count_if( begin(), end(), []( const Cell & cell ) { return cell.GetObject() == 0x40; } );

    uint8_t maxSmallObstacleCount = 0;
    uint8_t maxTwoHexObstacleCount = 0;

    if ( largeObstacleHexCount == 0 ) {
        maxSmallObstacleCount = 6;
        maxTwoHexObstacleCount = 3;
    }
    else if ( largeObstacleHexCount <= 7 ) {
        maxSmallObstacleCount = 4;
        maxTwoHexObstacleCount = 2;
    }
    else if ( largeObstacleHexCount <= 13 ) {
        maxSmallObstacleCount = 3;
        maxTwoHexObstacleCount = 2;
    }
    else {
        maxSmallObstacleCount = 2;
        maxTwoHexObstacleCount = 1;
    }

    // Limit the number of two-hex obstacles to maxTwoHexObstacleCount
    uint8_t twoHexCount = 0;
    for ( auto iter = objs.begin(); iter != objs.end(); ) {
        if ( isTwoHexObject( *iter ) ) {
            ++twoHexCount;
            if ( twoHexCount > maxTwoHexObstacleCount ) {
                iter = objs.erase( iter );
                continue;
            }
        }
        ++iter;
    }

    const size_t objectsToPlace = std::min( objs.size(), static_cast<size_t>( Rand::GetWithGen( 0, maxSmallObstacleCount, gen ) ) );

    for ( size_t i = 0; i < objectsToPlace; ++i ) {
        // two-hex obstacles are not allowed on column 8 as they would cover column 9 which is reserved for units
        const bool checkRightCell = isTwoHexObject( objs[i] );

        // tall obstacles like trees should not be placed on top 2 rows
        const bool isTallObstacle = isTallObject( objs[i] );

        uint32_t dest;
        do {
            dest = GetRandomObstaclePosition( gen );
        } while ( at( dest ).GetObject() != 0 || ( checkRightCell && ( at( dest + 1 ).GetObject() != 0 || ( dest % widthInCells ) == 8 ) )
                  || ( isTallObstacle && dest < ( widthInCells * 2 ) ) );

        SetCobjObject( objs[i], dest );
    }
}

void Battle::Board::SetCobjObject( const int icn, const uint32_t dst )
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

Battle::Cell * Battle::Board::GetCell( const int32_t position, const int dir /* = CENTER */ )
{
    if ( !isValidDirection( position, dir ) ) {
        return nullptr;
    }

    Board * board = Arena::GetBoard();
    assert( board != nullptr );

    const int32_t idx = GetIndexDirection( position, dir );
    assert( isValidIndex( idx ) );

    return &board->at( idx );
}

Battle::Indexes Battle::Board::GetMoveWideIndexes( const int32_t head, const bool reflect )
{
    if ( !isValidIndex( head ) ) {
        return {};
    }

    Indexes result;
    result.reserve( 4 );

    if ( isValidDirection( head, LEFT ) ) {
        result.push_back( GetIndexDirection( head, LEFT ) );
    }
    if ( isValidDirection( head, RIGHT ) ) {
        result.push_back( GetIndexDirection( head, RIGHT ) );
    }

    if ( reflect ) {
        if ( isValidDirection( head, TOP_LEFT ) ) {
            result.push_back( GetIndexDirection( head, TOP_LEFT ) );
        }
        if ( isValidDirection( head, BOTTOM_LEFT ) ) {
            result.push_back( GetIndexDirection( head, BOTTOM_LEFT ) );
        }
    }
    else {
        if ( isValidDirection( head, TOP_RIGHT ) ) {
            result.push_back( GetIndexDirection( head, TOP_RIGHT ) );
        }
        if ( isValidDirection( head, BOTTOM_RIGHT ) ) {
            result.push_back( GetIndexDirection( head, BOTTOM_RIGHT ) );
        }
    }

    return result;
}

Battle::Indexes Battle::Board::GetAroundIndexes( const int32_t center )
{
    if ( !isValidIndex( center ) ) {
        return {};
    }

    Indexes result;
    result.reserve( 6 );

    for ( CellDirection dir = TOP_LEFT; dir < CENTER; ++dir ) {
        if ( !isValidDirection( center, dir ) ) {
            continue;
        }

        result.push_back( GetIndexDirection( center, dir ) );
    }

    return result;
}

Battle::Indexes Battle::Board::GetAroundIndexes( const Unit & unit )
{
    return GetAroundIndexes( unit.GetPosition() );
}

Battle::Indexes Battle::Board::GetAroundIndexes( const Position & pos )
{
    if ( pos.GetHead() == nullptr ) {
        return {};
    }

    const int32_t headIdx = pos.GetHead()->GetIndex();

    if ( pos.GetTail() == nullptr ) {
        return GetAroundIndexes( headIdx );
    }

    const int32_t tailIdx = pos.GetTail()->GetIndex();

    if ( !isValidIndex( headIdx ) || !isValidIndex( tailIdx ) ) {
        return {};
    }

    Indexes result;
    result.reserve( 8 );

    // Traversing cells in a clockwise direction
    if ( headIdx > tailIdx ) {
        if ( isValidDirection( tailIdx, TOP_LEFT ) ) {
            result.push_back( GetIndexDirection( tailIdx, TOP_LEFT ) );
        }
        if ( isValidDirection( tailIdx, TOP_RIGHT ) ) {
            result.push_back( GetIndexDirection( tailIdx, TOP_RIGHT ) );
        }
        if ( isValidDirection( headIdx, TOP_RIGHT ) ) {
            result.push_back( GetIndexDirection( headIdx, TOP_RIGHT ) );
        }
        if ( isValidDirection( headIdx, RIGHT ) ) {
            result.push_back( GetIndexDirection( headIdx, RIGHT ) );
        }
        if ( isValidDirection( headIdx, BOTTOM_RIGHT ) ) {
            result.push_back( GetIndexDirection( headIdx, BOTTOM_RIGHT ) );
        }
        if ( isValidDirection( tailIdx, BOTTOM_RIGHT ) ) {
            result.push_back( GetIndexDirection( tailIdx, BOTTOM_RIGHT ) );
        }
        if ( isValidDirection( tailIdx, BOTTOM_LEFT ) ) {
            result.push_back( GetIndexDirection( tailIdx, BOTTOM_LEFT ) );
        }
        if ( isValidDirection( tailIdx, LEFT ) ) {
            result.push_back( GetIndexDirection( tailIdx, LEFT ) );
        }
    }
    else if ( headIdx < tailIdx ) {
        if ( isValidDirection( headIdx, TOP_LEFT ) ) {
            result.push_back( GetIndexDirection( headIdx, TOP_LEFT ) );
        }
        if ( isValidDirection( headIdx, TOP_RIGHT ) ) {
            result.push_back( GetIndexDirection( headIdx, TOP_RIGHT ) );
        }
        if ( isValidDirection( tailIdx, TOP_RIGHT ) ) {
            result.push_back( GetIndexDirection( tailIdx, TOP_RIGHT ) );
        }
        if ( isValidDirection( tailIdx, RIGHT ) ) {
            result.push_back( GetIndexDirection( tailIdx, RIGHT ) );
        }
        if ( isValidDirection( tailIdx, BOTTOM_RIGHT ) ) {
            result.push_back( GetIndexDirection( tailIdx, BOTTOM_RIGHT ) );
        }
        if ( isValidDirection( headIdx, BOTTOM_RIGHT ) ) {
            result.push_back( GetIndexDirection( headIdx, BOTTOM_RIGHT ) );
        }
        if ( isValidDirection( headIdx, BOTTOM_LEFT ) ) {
            result.push_back( GetIndexDirection( headIdx, BOTTOM_LEFT ) );
        }
        if ( isValidDirection( headIdx, LEFT ) ) {
            result.push_back( GetIndexDirection( headIdx, LEFT ) );
        }
    }

    return result;
}

Battle::Indexes Battle::Board::GetDistanceIndexes( const int32_t center, const uint32_t radius )
{
    if ( !isValidIndex( center ) ) {
        return {};
    }

    const int32_t centerX = center % widthInCells;
    const int32_t centerY = center / widthInCells;

    // Axial coordinates
    const int32_t centerQ = centerX - ( centerY + ( centerY % 2 ) ) / 2;
    const int32_t centerR = centerY;

    Indexes result;
    result.reserve( 3 * radius * ( radius + 1 ) + 1 );

    const int32_t intRadius = radius;

    for ( int32_t dq = -intRadius; dq <= intRadius; ++dq ) {
        for ( int32_t dr = std::max( -intRadius, -intRadius - dq ); dr <= std::min( intRadius, intRadius - dq ); ++dr ) {
            // Center should not be included
            if ( dq == 0 && dr == 0 ) {
                continue;
            }

            const int32_t q = centerQ + dq;
            const int32_t r = centerR + dr;

            const int32_t x = q + ( r + ( r % 2 ) ) / 2;
            const int32_t y = r;

            if ( x < 0 || x >= widthInCells || y < 0 || y >= heightInCells ) {
                continue;
            }

            const int32_t idx = y * widthInCells + x;
            assert( isValidIndex( idx ) );

            result.push_back( idx );
        }
    }

    return result;
}

Battle::Indexes Battle::Board::GetDistanceIndexes( const Position & pos, const uint32_t radius )
{
    const std::array<int32_t, 2> posIndexes = { pos.GetHead() ? pos.GetHead()->GetIndex() : -1, pos.GetTail() ? pos.GetTail()->GetIndex() : -1 };

    std::set<int32_t> boardIndexes;

    for ( const int32_t posIdx : posIndexes ) {
        if ( !Board::isValidIndex( posIdx ) ) {
            continue;
        }

        for ( const int32_t idx : Board::GetDistanceIndexes( posIdx, radius ) ) {
            assert( Board::isValidIndex( idx ) );

            if ( std::find( posIndexes.begin(), posIndexes.end(), idx ) != posIndexes.end() ) {
                continue;
            }

            boardIndexes.insert( idx );
        }
    }

    Indexes result;

    result.reserve( boardIndexes.size() );
    result.assign( boardIndexes.begin(), boardIndexes.end() );

    return result;
}

Battle::Indexes Battle::Board::GetDistanceIndexes( const Unit & unit, const uint32_t radius )
{
    return GetDistanceIndexes( unit.GetPosition(), radius );
}

bool Battle::Board::isValidMirrorImageIndex( const int32_t index, const Unit * unit )
{
    if ( unit == nullptr ) {
        return false;
    }

    const Position mirrorPos = Position::GetPosition( *unit, index );
    if ( !mirrorPos.isValidForUnit( unit ) ) {
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

bool Battle::Board::CanAttackFromCell( const Unit & unit, const int32_t from )
{
    const Position pos = Position::GetReachable( unit, from );

    // Target unit cannot be attacked if out of reach
    if ( pos.GetHead() == nullptr ) {
        return false;
    }

    assert( pos.isValidForUnit( unit ) );

    const Castle * castle = Arena::GetCastle();

    // No moat - no further restrictions
    if ( !castle || !castle->isBuild( BUILD_MOAT ) ) {
        return true;
    }

    // Target unit isn't attacked from the moat
    if ( !isMoatIndex( from, unit ) ) {
        return true;
    }

    // The moat doesn't stop flying units
    if ( unit.isFlying() ) {
        return true;
    }

    // Attacker is already near the target
    if ( from == unit.GetHeadIndex() || ( unit.isWide() && from == unit.GetTailIndex() ) ) {
        return true;
    }

    // In all other cases, the attack is prohibited
    return false;
}

bool Battle::Board::CanAttackTargetFromPosition( const Unit & attacker, const Unit & target, const int32_t dst )
{
    // Get the actual position of the attacker before attacking
    const Position pos = Position::GetReachable( attacker, dst );

    // Check that the attacker is actually capable of attacking the target from this position
    const std::array<const Cell *, 2> cells = { pos.GetHead(), pos.GetTail() };

    for ( const Cell * cell : cells ) {
        if ( cell == nullptr ) {
            continue;
        }

        const int32_t cellIdx = cell->GetIndex();

        if ( Board::GetDistance( target.GetPosition(), cellIdx ) > 1 ) {
            continue;
        }

        if ( !CanAttackFromCell( attacker, cellIdx ) ) {
            continue;
        }

        return true;
    }

    return false;
}

std::string Battle::Board::GetMoatInfo()
{
    std::string msg = _( "The Moat reduces the defense skill of troops trapped in it by %{count} and restricts their movement range." );
    StringReplace( msg, "%{count}", GameStatic::GetBattleMoatReduceDefense() );

    return msg;
}
