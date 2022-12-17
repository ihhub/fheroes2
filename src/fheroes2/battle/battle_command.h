/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
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

#ifndef H2BATTLE_COMMAND_H
#define H2BATTLE_COMMAND_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <vector>

namespace Battle
{
    enum class CommandType : int32_t
    {
        MSG_BATTLE_MOVE,
        MSG_BATTLE_ATTACK,
        MSG_BATTLE_CAST,
        MSG_BATTLE_MORALE,
        MSG_BATTLE_CATAPULT,
        MSG_BATTLE_TOWER,
        MSG_BATTLE_RETREAT,
        MSG_BATTLE_SURRENDER,
        MSG_BATTLE_SKIP,
        MSG_BATTLE_END_TURN,
        MSG_BATTLE_AUTO_SWITCH,
        MSG_BATTLE_AUTO_FINISH
    };

    class Command : public std::vector<int>
    {
    public:
        explicit Command( const CommandType cmd );
        Command( const CommandType cmd, const int param1, const int param2 = -1, const int param3 = -1, const int param4 = -1 );

        CommandType GetType() const
        {
            return _type;
        }

        int GetValue();

        bool isType( const CommandType cmd ) const
        {
            return _type == cmd;
        }

        Command & operator<<( const int );
        Command & operator>>( int & );

    private:
        CommandType _type;
    };
}

namespace std
{
    template <>
    struct hash<Battle::CommandType>
    {
        std::size_t operator()( const Battle::CommandType key ) const noexcept
        {
            using UnderlyingCommandType = typename std::underlying_type<Battle::CommandType>::type;

            std::hash<UnderlyingCommandType> hasher;

            return hasher( static_cast<UnderlyingCommandType>( key ) );
        }
    };
}

#endif
