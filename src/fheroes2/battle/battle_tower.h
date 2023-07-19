/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
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

#ifndef H2BATTLE_TOWER_H
#define H2BATTLE_TOWER_H

#include <cstdint>
#include <string>

#include "battle_troop.h"
#include "math_base.h"

namespace Rand
{
    class DeterministicRandomGenerator;
}

class Castle;

namespace Battle
{
    enum class TowerType : uint8_t
    {
        TWR_LEFT,
        TWR_CENTER,
        TWR_RIGHT
    };

    class Tower : public Unit
    {
    public:
        Tower( const Castle & castle, const TowerType type, const Rand::DeterministicRandomGenerator & randomGenerator, const uint32_t uid );
        Tower( const Tower & ) = delete;

        Tower & operator=( const Tower & ) = delete;

        bool isValid() const override;
        TowerType GetType() const;
        uint32_t GetAttackBonus() const;
        uint32_t GetAttack() const override;

        const char * GetName() const;

        void SetDestroy();
        fheroes2::Point GetPortPosition() const;

        // Returns a text description of the parameters of the towers of a given castle. Can be
        // called both during combat and outside of it. In the former case, the current state of
        // the towers destroyed during the siege will be reflected.
        static std::string GetInfo( const Castle & castle );

    private:
        TowerType _towerType;
        uint32_t _attackBonus;
        bool _isValid;
    };
}

#endif
