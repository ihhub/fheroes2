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
#include <utility>

Battle::Indexes Battle::Graveyard::GetOccupiedCells() const
{
    Indexes result;
    result.reserve( size() );

    for ( const auto & [idx, graves] : *this ) {
        if ( graves.empty() ) {
            continue;
        }

        result.push_back( idx );
    }

    return result;
}

void Battle::Graveyard::AddTroop( const Unit & unit )
{
    assert( Board::isValidIndex( unit.GetHeadIndex() ) && ( unit.isWide() ? Board::isValidIndex( unit.GetTailIndex() ) : !Board::isValidIndex( unit.GetTailIndex() ) ) );

    Graveyard & graveyard = *this;

    graveyard[unit.GetHeadIndex()].emplace_back( unit );

    if ( unit.isWide() ) {
        graveyard[unit.GetTailIndex()].emplace_back( unit );
    }
}

void Battle::Graveyard::RemoveTroop( const Unit & unit )
{
    assert( Board::isValidIndex( unit.GetHeadIndex() ) && ( unit.isWide() ? Board::isValidIndex( unit.GetTailIndex() ) : !Board::isValidIndex( unit.GetTailIndex() ) ) );

    const auto removeUIDFromIndex = [this]( const int32_t idx, const uint32_t uid ) {
        const auto graveyardIter = find( idx );
        if ( graveyardIter == end() ) {
            return;
        }

        auto & [dummy, graves] = *graveyardIter;

        const auto gravesIter = std::find_if( graves.begin(), graves.end(), [uid]( const Grave & grave ) { return grave.uid == uid; } );
        if ( gravesIter != graves.end() ) {
            graves.erase( gravesIter );
        }
    };

    removeUIDFromIndex( unit.GetHeadIndex(), unit.GetUID() );

    if ( unit.isWide() ) {
        removeUIDFromIndex( unit.GetTailIndex(), unit.GetUID() );
    }
}

uint32_t Battle::Graveyard::GetUIDOfLastTroop( const int32_t index ) const
{
    const auto iter = find( index );
    if ( iter == end() ) {
        return 0;
    }

    const auto & [dummy, graves] = *iter;

    if ( graves.empty() ) {
        return 0;
    }

    const uint32_t result = graves.back().uid;

    assert( result > 0 );

    return result;
}

uint32_t Battle::Graveyard::GetUIDOfLastTroopWithColor( const int32_t index, const int color ) const
{
    const auto graveyardIter = find( index );
    if ( graveyardIter == end() ) {
        return 0;
    }

    const auto & [dummy, graves] = *graveyardIter;

    const auto gravesIter = std::find_if( graves.rbegin(), graves.rend(), [color]( const Grave & grave ) { return grave.color == color; } );
    if ( gravesIter == graves.rend() ) {
        return 0;
    }

    const uint32_t result = gravesIter->uid;

    assert( result > 0 );

    return result;
}

Battle::Graves Battle::Graveyard::GetGraves( const int32_t index ) const
{
    const auto iter = find( index );
    if ( iter == end() ) {
        return {};
    }

    return iter->second;
}
