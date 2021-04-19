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
    // yes if not destroyed and lowered and there are no any troops (alive or dead) on or under the bridge
    return isValid() && isDown() && !isBridgeOccupied();
}

bool Battle::Bridge::isBridgeOccupied( void ) const
{
    const Battle::Graveyard * graveyard = Arena::GetGraveyard();

    // yes if there are any troops (alive or dead) on MOAT_CELL and GATES_CELL tiles
    return Board::GetCell( MOAT_CELL )->GetUnit() || Board::GetCell( GATES_CELL )->GetUnit() || graveyard->GetLastTroopUID( MOAT_CELL )
           || graveyard->GetLastTroopUID( GATES_CELL );
}

bool Battle::Bridge::NeedDown( const Unit & b, s32 dstPos ) const
{
    // no if bridge is destroyed or already lowered or unit does not belong to the castle or there are any troops (alive or dead) on or under the bridge
    if ( !isValid() || isDown() || b.GetColor() != Arena::GetCastle()->GetColor() || isBridgeOccupied() )
        return false;

    if ( b.isFlying() ) {
        return dstPos == GATES_CELL;
    }
    else {
        const int32_t prevPos = b.GetHeadIndex();

        if ( dstPos == GATES_CELL && ( prevPos == CELL_AFTER_GATES || prevPos == BELOW_BRIDGE_CELL || prevPos == ABOVE_BRIDGE_CELL ) ) {
            return true;
        }
        else if ( dstPos == MOAT_CELL && prevPos != GATES_CELL ) {
            return true;
        }
    }

    return false;
}

bool Battle::Bridge::isPassable( const Unit & b ) const
{
    // yes if bridge is lowered (or destroyed), or unit belongs to the castle and there are no any troops (alive or dead) on or under the bridge
    return isDown() || ( b.GetColor() == Arena::GetCastle()->GetColor() && !isBridgeOccupied() );
}

void Battle::Bridge::SetDestroy( void )
{
    destroy = true;

    Board::GetCell( GATES_CELL )->SetObject( 0 );
}

void Battle::Bridge::SetPassable( const Unit & b ) const
{
    if ( isPassable( b ) ) {
        Board::GetCell( GATES_CELL )->SetObject( 0 );
    }
    else {
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
