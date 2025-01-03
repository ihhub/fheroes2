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

#include "battle_bridge.h"

#include "battle_arena.h"
#include "battle_board.h"
#include "battle_cell.h"
#include "battle_grave.h"
#include "battle_interface.h"
#include "battle_troop.h"
#include "castle.h"

bool Battle::Bridge::isOccupied()
{
    const Battle::Graveyard * graveyard = Arena::GetGraveyard();
    assert( graveyard != nullptr );

    // Yes if there are any troops (alive or dead) on CELL_MOAT and CELL_GATES tiles
    return Board::GetCell( CELL_MOAT )->GetUnit() || Board::GetCell( CELL_GATES )->GetUnit() || graveyard->getLastUnit( CELL_MOAT )
           || graveyard->getLastUnit( CELL_GATES );
}

bool Battle::Bridge::NeedDown( const Unit & unit, const int32_t dstIdx ) const
{
    // No if bridge is destroyed or already lowered or unit does not belong to the castle or there are any troops (alive or dead) on or under the bridge
    if ( !isValid() || isDown() || unit.GetColor() != Arena::GetCastle()->GetColor() || isOccupied() ) {
        return false;
    }

    if ( unit.isFlying() ) {
        return dstIdx == CELL_GATES;
    }

    const int32_t headIdx = unit.GetHeadIndex();

    if ( dstIdx == CELL_GATES && ( headIdx == CELL_AFTER_GATES || headIdx == CELL_BELOW_BRIDGE || headIdx == CELL_ABOVE_BRIDGE ) ) {
        return true;
    }
    if ( dstIdx == CELL_MOAT && headIdx != CELL_GATES ) {
        return true;
    }
    if ( unit.isWide() ) {
        const int32_t tailIdx = unit.GetTailIndex();

        if ( dstIdx == CELL_BEFORE_MOAT && ( tailIdx == CELL_BELOW_BRIDGE || tailIdx == CELL_ABOVE_BRIDGE ) ) {
            return true;
        }
    }

    return false;
}

bool Battle::Bridge::isPassable( const Unit & unit ) const
{
    // Yes if bridge is lowered (or destroyed), or unit belongs to the castle and there are no any troops (alive or dead) on or under the bridge
    return isDown() || ( unit.GetColor() == Arena::GetCastle()->GetColor() && !isOccupied() );
}

void Battle::Bridge::SetDestroyed()
{
    _isDestroyed = true;
    _isDown = true;

    Board::GetCell( CELL_GATES )->SetObject( 0 );
}

void Battle::Bridge::SetPassability( const Unit & unit ) const
{
    if ( isPassable( unit ) ) {
        Board::GetCell( CELL_GATES )->SetObject( 0 );
    }
    else {
        Board::GetCell( CELL_GATES )->SetObject( 1 );
    }
}

void Battle::Bridge::ActionUp()
{
    assert( AllowUp() );

    if ( Arena::GetInterface() ) {
        Arena::GetInterface()->RedrawBridgeAnimation( false );
    }

    _isDown = false;
}

void Battle::Bridge::ActionDown()
{
    assert( isValid() && !isDown() && !isOccupied() );

    if ( Arena::GetInterface() ) {
        Arena::GetInterface()->RedrawBridgeAnimation( true );
    }

    _isDown = true;
}
