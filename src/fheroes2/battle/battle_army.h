/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
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

#ifndef H2BATTLE_ARMY_H
#define H2BATTLE_ARMY_H

#include <cstdint>
#include <vector>

#include "army.h"
#include "bitmodes.h"
#include "monster.h"

class HeroBase;

namespace Rand
{
    class DeterministicRandomGenerator;
}

namespace Battle
{
    class Unit;
    class TroopsUidGenerator;

    class Units : public std::vector<Unit *>
    {
    public:
        Units();

        // Creates a shallow copy of 'units' (only pointers are copied), removing
        // invalid units (i.e. empty slots) if requested
        Units( const Units & units, const bool isRemoveInvalidUnits );

        // Creates a shallow copy of 'units' (only pointers are copied), removing
        // invalid units (i.e. empty slots) as well as the specified unit
        Units( const Units & units, const Unit * unitToRemove );

        Units( const Units & ) = delete;

        virtual ~Units() = default;

        Units & operator=( const Units & ) = delete;

        Unit * FindMode( uint32_t mod ) const;
        Unit * FindUID( uint32_t pid ) const;

        void SortFastest();
        void SortArchers();
    };

    class Force : public Units, public BitModes
    {
    public:
        Force( Army & parent, bool opposite, const Rand::DeterministicRandomGenerator & randomGenerator, TroopsUidGenerator & generator );
        Force( const Force & ) = delete;

        ~Force() override;

        Force & operator=( const Force & ) = delete;

        HeroBase * GetCommander();
        const HeroBase * GetCommander() const;

        const Units & getUnits() const;

        bool isValid( const bool considerBattlefieldArmy = true ) const;
        bool HasMonster( const Monster & ) const;
        uint32_t GetDeadHitPoints() const;
        uint32_t GetDeadCounts() const;
        int GetColor() const;
        int GetControl() const;
        uint32_t GetSurrenderCost() const;
        Troops GetKilledTroops() const;
        bool animateIdleUnits();
        void resetIdleAnimation();

        void NewTurn();
        void SyncArmyCount();

    private:
        Army & army;
        std::vector<uint32_t> uids;
    };
}

#endif
