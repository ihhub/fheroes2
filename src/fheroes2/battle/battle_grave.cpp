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

#include "battle_troop.h"

Battle::Indexes Battle::Graveyard::getOccupiedCells() const
{
    Indexes result;
    result.reserve( size() );

    for ( const auto & [idx, units] : *this ) {
        if ( units.empty() ) {
            continue;
        }

        result.push_back( idx );
    }

    return result;
}

void Battle::Graveyard::addUnit( Unit * unit )
{
    assert( unit != nullptr && Board::isValidIndex( unit->GetHeadIndex() )
            && ( unit->isWide() ? Board::isValidIndex( unit->GetTailIndex() ) : !Board::isValidIndex( unit->GetTailIndex() ) ) );

    Graveyard & graveyard = *this;

    graveyard[unit->GetHeadIndex()].push_back( unit );

    if ( unit->isWide() ) {
        graveyard[unit->GetTailIndex()].push_back( unit );
    }
}

void Battle::Graveyard::removeUnit( Unit * unit )
{
    assert( unit != nullptr && Board::isValidIndex( unit->GetHeadIndex() )
            && ( unit->isWide() ? Board::isValidIndex( unit->GetTailIndex() ) : !Board::isValidIndex( unit->GetTailIndex() ) ) );

    const auto removeFromIndex = [this, unit]( const int32_t idx ) {
        const auto graveyardIter = find( idx );
        if ( graveyardIter == end() ) {
            return;
        }

        auto & [dummy, units] = *graveyardIter;

        const auto unitsIter = std::find( units.begin(), units.end(), unit );
        if ( unitsIter == units.end() ) {
            return;
        }

        units.erase( unitsIter );
    };

    removeFromIndex( unit->GetHeadIndex() );

    if ( unit->isWide() ) {
        removeFromIndex( unit->GetTailIndex() );
    }
}

Battle::Unit * Battle::Graveyard::getLastUnit( const int32_t index ) const
{
    const auto iter = find( index );
    if ( iter == end() ) {
        return nullptr;
    }

    const auto & [dummy, units] = *iter;

    if ( units.empty() ) {
        return nullptr;
    }

    return units.back();
}

std::vector<Battle::Unit *> Battle::Graveyard::getUnits( const int32_t index ) const
{
    const auto iter = find( index );
    if ( iter == end() ) {
        return {};
    }

    return iter->second;
}
