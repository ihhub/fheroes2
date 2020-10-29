/***************************************************************************
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
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
#include <functional>
#include <iterator>
#include <set>

#include "battle_arena.h"
#include "battle_bridge.h"
#include "battle_troop.h"
#include "castle.h"
#include "game_static.h"
#include "ground.h"
#include "settings.h"
#include "world.h"

namespace Battle
{
    int GetObstaclePosition( void )
    {
        return Rand::Get( 3, 6 ) + ( 11 * Rand::Get( 1, 7 ) );
    }

    bool WideDifficultDirection( int where, int whereto )
    {
        return ( ( TOP_LEFT == where ) && ( whereto & ( LEFT | TOP_RIGHT ) ) ) || ( ( TOP_RIGHT == where ) && ( whereto & ( RIGHT | TOP_LEFT ) ) )
               || ( ( BOTTOM_LEFT == where ) && ( whereto & ( LEFT | BOTTOM_RIGHT ) ) ) || ( ( BOTTOM_RIGHT == where ) && ( whereto & ( RIGHT | BOTTOM_LEFT ) ) );
    }
}

Battle::Board::Board()
{
    reserve( ARENASIZE );
    for ( u32 ii = 0; ii < ARENASIZE; ++ii )
        push_back( Cell( ii ) );
}

void Battle::Board::SetArea( const Rect & area )
{
    for ( iterator it = begin(); it != end(); ++it )
        ( *it ).SetArea( area );
}

Rect Battle::Board::GetArea( void ) const
{
    Rects rects;
    rects.reserve( size() );

    for ( const_iterator it = begin(); it != end(); ++it )
        rects.push_back( ( *it ).GetPos() );

    return rects.GetRect();
}

void Battle::Board::Reset( void )
{
    for ( iterator it = begin(); it != end(); ++it ) {
        Unit * unit = it->GetUnit();
        if ( unit && !unit->isValid() ) {
            unit->PostKilledAction();
        }
        it->ResetDirection();
        it->ResetQuality();
    }
}

void Battle::Board::SetPositionQuality( const Unit & b )
{
    Arena * arena = GetArena();
    Units enemies( arena->GetForce( b.GetColor(), true ), true );

    // Make sure archers are first here, so melee unit's score won't be double counted
    enemies.SortArchers();

    for ( Units::const_iterator it1 = enemies.begin(); it1 != enemies.end(); ++it1 ) {
        const Unit * unit = *it1;

        if ( unit && unit->isValid() ) {
            const s32 unitStrength = unit->GetScoreQuality( b );
            const Indexes around = GetAroundIndexes( *unit );

            for ( Indexes::const_iterator it2 = around.begin(); it2 != around.end(); ++it2 ) {
                Cell * cell2 = GetCell( *it2 );
                if ( cell2 && cell2->isPassable3( b, false ) ) {
                    const s32 quality = cell2->GetQuality();
                    // Only sum up quality score if it's archers; otherwise just pick the strongest
                    if ( unit->isArchers() )
                        cell2->SetQuality( quality + unitStrength );
                    else if ( unitStrength > quality )
                        cell2->SetQuality( unitStrength );
                }
            }
        }
    }
}

void Battle::Board::SetEnemyQuality( const Unit & unit )
{
    Arena * arena = GetArena();
    Units enemies( arena->GetForce( unit.GetColor(), true ), true );
    if ( unit.Modes( SP_BERSERKER ) ) {
        Units allies( arena->GetForce( unit.GetColor(), false ), true );
        enemies.insert( enemies.end(), allies.begin(), allies.end() );
    }

    for ( Units::const_iterator it = enemies.begin(); it != enemies.end(); ++it ) {
        Unit * enemy = *it;

        if ( enemy && enemy->isValid() ) {
            const s32 score = enemy->GetScoreQuality( unit );
            Cell * cell = GetCell( enemy->GetHeadIndex() );

            cell->SetQuality( score );

            if ( enemy->isWide() )
                GetCell( enemy->GetTailIndex() )->SetQuality( score );

            DEBUG( DBG_BATTLE, DBG_TRACE, score << " for " << enemy->String() );
        }
    }
}

s32 Battle::Board::GetDistance( s32 index1, s32 index2 )
{
    if ( isValidIndex( index1 ) && isValidIndex( index2 ) ) {
        const int dx = std::abs( ( index1 % ARENAW ) - ( index2 % ARENAW ) );
        const int dy = std::abs( ( index1 / ARENAW ) - ( index2 / ARENAW ) );
        const int roundingUp = index1 / ARENAW % 2;

        // hexagonal grid: you only move half as much on X axis when diagonal!
        return dy + std::max( dx - ( dy + roundingUp ) / 2, 0 );
    }

    return 0;
}

void Battle::Board::SetScanPassability( const Unit & b )
{
    std::for_each( begin(), end(), std::mem_fun_ref( &Cell::ResetDirection ) );

    at( b.GetHeadIndex() ).SetDirection( CENTER );

    if ( b.isFlying() ) {
        for ( iterator it = begin(); it != end(); ++it )
            if ( ( *it ).isPassable3( b, false ) )
                ( *it ).SetDirection( CENTER );
    }
    else {
        Indexes indexes = GetDistanceIndexes( b.GetHeadIndex(), b.GetSpeed() );
        indexes.resize( std::distance( indexes.begin(), std::remove_if( indexes.begin(), indexes.end(), isImpassableIndex ) ) );

        // set pasable
        for ( Indexes::const_iterator it = indexes.begin(); it != indexes.end(); ++it )
            GetAStarPath( b, Position::GetCorrect( b, *it ), false );
    }
}

struct bcell_t
{
    s32 cost;
    s32 prnt;
    bool open;

    bcell_t()
        : cost( MAXU16 )
        , prnt( -1 )
        , open( true )
    {}
};

Battle::Indexes Battle::Board::GetAStarPath( const Unit & b, const Position & dst, bool debug )
{
    Indexes result;
    const bool isWide = b.isWide();

    // check if target position is valid
    if ( !dst.GetHead() || ( isWide && !dst.GetTail() ) ) {
        ERROR( "Board::GetAStarPath invalid destination for unit " + b.String() );
        return result;
    }

    s32 cur = b.GetHeadIndex();
    const Castle * castle = Arena::GetCastle();
    const Bridge * bridge = Arena::GetBridge();

    std::map<s32, bcell_t> list;
    list[cur].prnt = -1;
    list[cur].cost = 0;
    list[cur].open = false;

    while ( cur != dst.GetHead()->GetIndex() ) {
        const Cell & center = at( cur );
        Indexes around
            = isWide ? GetMoveWideIndexes( cur, ( 0 > list[cur].prnt ? b.isReflect() : ( RIGHT_SIDE & GetDirection( cur, list[cur].prnt ) ) ) ) : GetAroundIndexes( cur );

        for ( Indexes::const_iterator it = around.begin(); it != around.end(); ++it ) {
            Cell & cell = at( *it );

            if ( list[*it].open && cell.isPassable4( b, center ) &&
                 // check bridge
                 ( !bridge || !Board::isBridgeIndex( *it ) || bridge->isPassable( b.GetColor() ) ) ) {
                const s32 cost = 100 * Board::GetDistance( *it, dst.GetHead()->GetIndex() )
                                 + ( isWide && WideDifficultDirection( center.GetDirection(), GetDirection( *it, cur ) ) ? 100 : 0 )
                                 + ( castle && castle->isBuild( BUILD_MOAT ) && Board::isMoatIndex( *it ) ? 100 : 0 );

                // new cell
                if ( 0 > list[*it].prnt ) {
                    list[*it].prnt = cur;
                    list[*it].cost = cost + list[cur].cost;
                }
                else
                    // change parent
                    if ( list[*it].cost > cost + list[cur].cost ) {
                    list[*it].prnt = cur;
                    list[*it].cost = cost + list[cur].cost;
                }
            }
        }

        list[cur].open = false;
        s32 cost = MAXU16;

        // find min cost opens
        for ( std::map<s32, bcell_t>::const_iterator it = list.begin(); it != list.end(); ++it )
            if ( ( *it ).second.open && cost > ( *it ).second.cost ) {
                cur = ( *it ).first;
                cost = ( *it ).second.cost;
            }

        if ( MAXU16 == cost )
            break;
    }

    result.reserve( 15 );

    // save path
    if ( cur == dst.GetHead()->GetIndex() ) {
        while ( cur != b.GetHeadIndex() && isValidIndex( cur ) && ( !isWide || isValidDirection( cur, b.isReflect() ? RIGHT : LEFT ) ) ) {
            result.push_back( cur );
            cur = list[cur].prnt;
        }

        std::reverse( result.begin(), result.end() );

        // correct wide position
        if ( isWide && result.size() ) {
            const s32 head = dst.GetHead()->GetIndex();
            const s32 tail = dst.GetTail()->GetIndex();
            const s32 prev = 1 < result.size() ? result[result.size() - 2] : b.GetHeadIndex();

            if ( result.back() == head ) {
                int side = RIGHT == GetDirection( head, tail ) ? RIGHT_SIDE : LEFT_SIDE;

                if ( !( side & GetDirection( head, prev ) ) )
                    result.push_back( tail );
            }
            else if ( result.back() == tail ) {
                int side = RIGHT == GetDirection( head, tail ) ? LEFT_SIDE : RIGHT_SIDE;

                if ( !( side & GetDirection( tail, prev ) ) )
                    result.push_back( head );
            }
        }

        if ( result.size() > b.GetSpeed() )
            result.resize( b.GetSpeed() );

        // skip moat position
        if ( castle && castle->isBuild( BUILD_MOAT ) && !Board::isMoatIndex( b.GetHeadIndex() ) ) {
            Indexes::iterator moat = std::find_if( result.begin(), result.end(), Board::isMoatIndex );
            if ( moat != result.end() )
                result.resize( std::distance( result.begin(), ++moat ) );
        }

        // set passable info
        for ( Indexes::iterator it = result.begin(); it != result.end(); ++it ) {
            Cell * cell = GetCell( *it );
            cell->SetDirection( cell->GetDirection() | GetDirection( *it, it == result.begin() ? b.GetHeadIndex() : *( it - 1 ) ) );

            if ( isWide ) {
                const s32 head = *it;
                const s32 prev = it != result.begin() ? *( it - 1 ) : b.GetHeadIndex();
                Cell * tail = GetCell( head, LEFT_SIDE & GetDirection( head, prev ) ? LEFT : RIGHT );

                if ( tail && UNKNOWN == tail->GetDirection() )
                    tail->SetDirection( GetDirection( tail->GetIndex(), head ) );
            }
        }
    }

    if ( debug && result.empty() ) {
        DEBUG( DBG_BATTLE, DBG_WARN,
               "path not found: " << b.String() << ", dst: "
                                  << "(head: " << dst.GetHead()->GetIndex() << ", tail: " << ( dst.GetTail() ? dst.GetTail()->GetIndex() : -1 ) << ")" );
    }

    return result;
}

std::string Battle::Board::AllUnitsInfo( void ) const
{
    std::ostringstream os;

    for ( const_iterator it = begin(); it != end(); ++it ) {
        const Unit * b = ( *it ).GetUnit();
        if ( b )
            os << "\t" << b->String( true ) << std::endl;
    }

    return os.str();
}

Battle::Indexes Battle::Board::GetPassableQualityPositions( const Unit & b )
{
    Indexes result;
    result.reserve( 30 );

    // make sure we check current position first to avoid unnecessary move
    const int headIndex = b.GetHeadIndex();
    if ( GetCell( headIndex )->GetQuality() ) {
        result.push_back( headIndex );
    }

    for ( const_iterator it = begin(); it != end(); ++it )
        if ( ( *it ).isPassable3( b, false ) && ( *it ).GetQuality() )
            result.push_back( ( *it ).GetIndex() );

    if ( IS_DEBUG( DBG_BATTLE, DBG_TRACE ) ) {
        std::stringstream ss;
        if ( result.empty() )
            ss << "empty";
        else
            for ( Indexes::const_iterator it = result.begin(); it != result.end(); ++it )
                ss << *it << ", ";
        DEBUG( DBG_BATTLE, DBG_TRACE, ss.str() );
    }

    return result;
}

struct IndexDistanceEqualDistance : std::binary_function<IndexDistance, u32, bool>
{
    bool operator()( const IndexDistance & id, u32 dist ) const
    {
        return id.second == dist;
    };
};

Battle::Indexes Battle::Board::GetNearestTroopIndexes( s32 pos, const Indexes * black ) const
{
    Indexes result;
    std::vector<IndexDistance> dists;
    dists.reserve( 15 );

    for ( const_iterator it = begin(); it != end(); ++it ) {
        const Battle::Unit * b = ( *it ).GetUnit();

        if ( b ) {
            // check black list
            if ( black && black->end() != std::find( black->begin(), black->end(), b->GetHeadIndex() ) )
                continue;
            // added
            if ( pos != b->GetHeadIndex() )
                dists.push_back( IndexDistance( b->GetHeadIndex(), GetDistance( pos, b->GetHeadIndex() ) ) );
        }
    }

    if ( 1 < dists.size() ) {
        std::sort( dists.begin(), dists.end(), IndexDistance::Shortest );
        dists.resize( std::count_if( dists.begin(), dists.end(), std::bind2nd( IndexDistanceEqualDistance(), dists.front().second ) ) );
    }

    if ( dists.size() ) {
        result.reserve( dists.size() );
        for ( std::vector<IndexDistance>::const_iterator it = dists.begin(); it != dists.end(); ++it )
            result.push_back( ( *it ).first );
    }

    return result;
}

int Battle::Board::GetDirection( s32 index1, s32 index2 )
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

bool Battle::Board::isNearIndexes( s32 index1, s32 index2 )
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

bool Battle::Board::isReflectDirection( int d )
{
    switch ( d ) {
    case TOP_LEFT:
    case LEFT:
    case BOTTOM_LEFT:
        return true;
    default:
        break;
    }

    return false;
}

bool Battle::Board::isNegativeDistance( s32 index1, s32 index2 )
{
    return ( index1 % ARENAW ) - ( index2 % ARENAW ) < 0;
}

bool Battle::Board::isValidDirection( s32 index, int dir )
{
    if ( isValidIndex( index ) ) {
        const s32 x = index % ARENAW;
        const s32 y = index / ARENAW;

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

s32 Battle::Board::GetIndexDirection( s32 index, int dir )
{
    if ( isValidIndex( index ) ) {
        const s32 y = index / ARENAW;

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

s32 Battle::Board::GetIndexAbsPosition( const Point & pt ) const
{
    const_iterator it = begin();

    for ( ; it != end(); ++it )
        if ( ( *it ).isPositionIncludePoint( pt ) )
            break;

    return it != end() ? ( *it ).GetIndex() : -1;
}

bool Battle::Board::isValidIndex( s32 index )
{
    return 0 <= index && index < ARENASIZE;
}

bool Battle::Board::isCastleIndex( s32 index )
{
    return ( ( 8 < index && index <= 10 ) || ( 19 < index && index <= 21 ) || ( 29 < index && index <= 32 ) || ( 40 < index && index <= 43 )
             || ( 50 < index && index <= 54 ) || ( 62 < index && index <= 65 ) || ( 73 < index && index <= 76 ) || ( 85 < index && index <= 87 )
             || ( 96 < index && index <= 98 ) );
}

bool Battle::Board::isOutOfWallsIndex( s32 index )
{
    return ( ( index <= 8 ) || ( 11 <= index && index <= 19 ) || ( 22 <= index && index <= 29 ) || ( 33 <= index && index <= 40 ) || ( 44 <= index && index <= 50 )
             || ( 55 <= index && index <= 62 ) || ( 66 <= index && index <= 73 ) || ( 77 <= index && index <= 85 ) || ( 88 <= index && index <= 96 ) );
}

bool Battle::Board::isImpassableIndex( s32 index )
{
    const Cell * cell = Board::GetCell( index );
    return !cell || !cell->isPassable1( true );
}

bool Battle::Board::isBridgeIndex( s32 index )
{
    switch ( index ) {
    case 49:
    case 50:
        return true;

    default:
        break;
    }

    return false;
}

bool Battle::Board::isMoatIndex( s32 index )
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

    default:
        break;
    }

    return false;
}

void Battle::Board::SetCobjObjects( const Maps::Tiles & tile )
{
    //    bool trees = Maps::ScanAroundObject(center, MP2::OBJ_TREES).size();
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

    const size_t objectsToPlace = std::min( objs.size(), static_cast<size_t>( Rand::Get( 0, 4 ) ) );
    std::random_shuffle( objs.begin(), objs.end() );

    for ( size_t i = 0; i < objectsToPlace; ++i ) {
        s32 dest = GetObstaclePosition();
        while ( at( dest ).GetObject() )
            dest = GetObstaclePosition();

        SetCobjObject( objs[i], dest );
    }
}

void Battle::Board::SetCobjObject( int icn, s32 dst )
{
    at( dst ).SetObject( 0x80 + ( icn - ICN::COBJ0000 ) );

    switch ( icn ) {
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
        at( dst + 1 ).SetObject( 0x40 );
        break;

    default:
        break;
    }
}

void Battle::Board::SetCovrObjects( int icn )
{
    switch ( icn ) {
    case ICN::COVR0001:
    case ICN::COVR0007:
    case ICN::COVR0013:
    case ICN::COVR0019:
        at( 15 ).SetObject( 0x40 );
        at( 16 ).SetObject( 0x40 );
        at( 17 ).SetObject( 0x40 );
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
    case ICN::COVR0009:
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

Battle::Cell * Battle::Board::GetCell( s32 position, int dir )
{
    Board * board = Arena::GetBoard();

    if ( isValidIndex( position ) && dir != UNKNOWN ) {
        if ( dir == CENTER )
            return &board->at( position );
        else if ( Board::isValidDirection( position, dir ) )
            return &board->at( GetIndexDirection( position, dir ) );
    }

    return NULL;
}

Battle::Indexes Battle::Board::GetMoveWideIndexes( s32 center, bool reflect )
{
    Indexes result;
    result.reserve( 8 );

    if ( isValidIndex( center ) ) {
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

Battle::Indexes Battle::Board::GetAroundIndexes( s32 center, s32 ignore )
{
    Indexes result;
    result.reserve( 12 );

    if ( isValidIndex( center ) ) {
        for ( direction_t dir = TOP_LEFT; dir < CENTER; ++dir )
            if ( isValidDirection( center, dir ) && GetIndexDirection( center, dir ) != ignore )
                result.push_back( GetIndexDirection( center, dir ) );
    }

    return result;
}

Battle::Indexes Battle::Board::GetAroundIndexes( const Unit & b )
{
    const int headIdx = b.GetHeadIndex();

    if ( b.isWide() ) {
        const int tailIdx = b.GetTailIndex();

        Indexes around = GetAroundIndexes( headIdx, tailIdx );
        const Indexes & tail = GetAroundIndexes( tailIdx, headIdx );
        around.insert( around.end(), tail.begin(), tail.end() );

        std::sort( around.begin(), around.end() );
        around.erase( std::unique( around.begin(), around.end() ), around.end() );

        return around;
    }

    return GetAroundIndexes( headIdx );
}

Battle::Indexes Battle::Board::GetDistanceIndexes( s32 center, u32 radius )
{
    Indexes result;

    if ( isValidIndex( center ) ) {
        std::set<s32> st;
        Indexes abroad;

        st.insert( center );
        abroad.push_back( center );

        while ( abroad.size() && radius ) {
            std::set<s32> tm = st;

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

bool Battle::Board::isValidMirrorImageIndex( s32 index, const Unit * troop )
{
    if ( troop == NULL )
        return false;

    const Cell * cell = GetCell( index );
    if ( cell == NULL )
        return false;

    const bool doubleHex = troop->isWide();
    if ( index == troop->GetHeadIndex() || ( doubleHex && index == troop->GetTailIndex() ) )
        return false;

    if ( !cell->isPassable3( *troop, true ) )
        return false;

    if ( doubleHex ) {
        const bool isReflected = troop->GetHeadIndex() < troop->GetTailIndex();
        const int32_t tailIndex = isReflected ? index + 1 : index - 1;
        const Cell * tailCell = GetCell( tailIndex );
        if ( tailCell == NULL || tailIndex == troop->GetHeadIndex() || tailIndex == troop->GetTailIndex() )
            return false;

        if ( !tailCell->isPassable3( *troop, true ) )
            return false;
    }

    return true;
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
        Unit * unit = GetCell( index )->GetUnit();
        if ( unit && currentColor != unit->GetArmyColor() )
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

std::string Battle::Board::GetMoatInfo( void )
{
    std::string msg = _( "The Moat reduces by -%{count} the defense skill of any unit and slows to half movement rate." );
    StringReplace( msg, "%{count}", GameStatic::GetBattleMoatReduceDefense() );

    return msg;
}
