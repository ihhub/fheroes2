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

#include "battle_bridge.h"
#include "battle_cell.h"
#include "battle_interface.h"
#include "battle_troop.h"
#include "castle.h"

Battle::Bridge::Bridge()
    : destroy( false )
    , down( false )
{}

bool Battle::Bridge::isValid( void ) const
{
    return !isDestroy();
}

bool Battle::Bridge::isDestroy( void ) const
{
    return destroy;
}

bool Battle::Bridge::isDown( void ) const
{
    return down || isDestroy();
}

void Battle::Bridge::SetDown( bool f )
{
    down = f;
}

bool Battle::Bridge::AllowUp( void ) const
{
    if ( !isValid() || !isDown() )
        return false;

    if ( isDeadBodyOnABridge() )
        return false;

    const bool isNoUnitOnMoat = NULL == Board::GetCell( MOAT_CELL )->GetUnit();
    const bool isNoUnitOnGates = NULL == Board::GetCell( GATES_CELL )->GetUnit();
    return isNoUnitOnMoat && isNoUnitOnGates;
}

bool Battle::Bridge::isDeadBodyOnABridge( void ) const
{
    const Battle::Graveyard * graveyard = GetArena()->GetGraveyard();
    return graveyard->GetLastTroopUID( MOAT_CELL ) || graveyard->GetLastTroopUID( GATES_CELL );
}

bool Battle::Bridge::NeedDown( const Unit & b, s32 dstPos ) const
{
    if ( !isValid() || isDown() ) // destroyed or already in down state
        return false;

    if ( isDeadBodyOnABridge() ) // under bridge
        return false;

    const s32 prevPos = b.GetHeadIndex();

    if ( dstPos == GATES_CELL ) {
        if ( prevPos == CELL_AFTER_GATES )
            return true;
        if ( ( prevPos == BELOW_BRIDGE_CELL || prevPos == ABOVE_BRIDGE_CELL ) && b.GetColor() == Arena::GetCastle()->GetColor() )
            return true;
    }
    else if ( dstPos == MOAT_CELL ) {
        if ( prevPos != GATES_CELL && b.GetColor() == Arena::GetCastle()->GetColor() )
            return true;
    }

    return false;
}

bool Battle::Bridge::isPassable( int color ) const
{
    if ( !isDown() && isDeadBodyOnABridge() ) // if bridge not in a down state and dead body's exists on MOAT_CELL and GATES_CELL tiles
        return false;

    return color == Arena::GetCastle()->GetColor() || isDown();
}

void Battle::Bridge::SetDestroy( void )
{
    destroy = true;
    Board::GetCell( MOAT_CELL )->SetObject( 0 );
    Board::GetCell( GATES_CELL )->SetObject( 0 );
}

void Battle::Bridge::SetPassable( const Unit & b )
{
    if ( Board::isCastleIndex( b.GetHeadIndex() ) || b.GetColor() == Arena::GetCastle()->GetColor() ) {
        Board::GetCell( MOAT_CELL )->SetObject( 0 );
        Board::GetCell( GATES_CELL )->SetObject( 0 );
    }
    else {
        Board::GetCell( MOAT_CELL )->SetObject( 1 );
        Board::GetCell( GATES_CELL )->SetObject( 1 );
    }
}

void Battle::Bridge::Action( const Unit & b, s32 dst )
{
    bool action_down = false;

    if ( NeedDown( b, dst ) )
        action_down = true;

    if ( Arena::GetInterface() )
        Arena::GetInterface()->RedrawBridgeAnimation( action_down );

    SetDown( action_down );
}

bool Battle::Bridge::isMoatCell( int cellId ) const
{
    return cellId == MOAT_CELL;
}
