/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2012 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "battle_grave.h"

#include <algorithm>
#include <cassert>
#include <type_traits>
#include <utility>

#include "battle_board.h"
#include "battle_troop.h"

Battle::Indexes Battle::Graveyard::GetOccupiedCells() const
{
    Indexes result;
    result.reserve( size() );

    std::for_each( begin(), end(), [&result]( const auto & item ) {
        const auto & [idx, troopUIDs] = item;

        if ( !troopUIDs.empty() ) {
            result.push_back( idx );
        }
    } );

    return result;
}

void Battle::Graveyard::AddTroop( const Unit & unit )
{
    assert( Board::isValidIndex( unit.GetHeadIndex() ) && ( unit.isWide() ? Board::isValidIndex( unit.GetTailIndex() ) : !Board::isValidIndex( unit.GetTailIndex() ) ) );

    Graveyard & graveyard = *this;

    graveyard[unit.GetHeadIndex()].push_back( unit.GetUID() );

    if ( unit.isWide() ) {
        graveyard[unit.GetTailIndex()].push_back( unit.GetUID() );
    }
}

void Battle::Graveyard::RemoveTroop( const Unit & unit )
{
    assert( Board::isValidIndex( unit.GetHeadIndex() ) && ( unit.isWide() ? Board::isValidIndex( unit.GetTailIndex() ) : !Board::isValidIndex( unit.GetTailIndex() ) ) );

    const auto removeUIDFromIndex = [this]( const int32_t idx, const uint32_t uid ) {
        const auto idxIter = find( idx );
        if ( idxIter == end() ) {
            return;
        }

        auto & [dummy, troopUIDs] = *idxIter;

        const auto troopUIDIter = std::find( troopUIDs.begin(), troopUIDs.end(), uid );
        if ( troopUIDIter != troopUIDs.end() ) {
            troopUIDs.erase( troopUIDIter );
        }
    };

    removeUIDFromIndex( unit.GetHeadIndex(), unit.GetUID() );

    if ( unit.isWide() ) {
        removeUIDFromIndex( unit.GetTailIndex(), unit.GetUID() );
    }
}

uint32_t Battle::Graveyard::GetLastTroopUID( const int32_t index ) const
{
    const auto iter = find( index );
    if ( iter == end() ) {
        return 0;
    }

    const auto & [dummy, troopUIDs] = *iter;

    if ( troopUIDs.empty() ) {
        return 0;
    }

    assert( troopUIDs.back() > 0 );

    return troopUIDs.back();
}

Battle::TroopUIDs Battle::Graveyard::GetTroopUIDs( const int32_t index ) const
{
    const auto iter = find( index );
    if ( iter == end() ) {
        return {};
    }

    return iter->second;
}
