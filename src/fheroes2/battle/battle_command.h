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

#ifndef H2BATTLE_COMMAND_H
#define H2BATTLE_COMMAND_H

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <tuple>
#include <type_traits>
#include <vector>

#include "spell.h"

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

    class Command final : public std::vector<int>
    {
    public:
        static constexpr std::integral_constant<CommandType, CommandType::MSG_BATTLE_MOVE> MOVE{};
        static constexpr std::integral_constant<CommandType, CommandType::MSG_BATTLE_ATTACK> ATTACK{};
        static constexpr std::integral_constant<CommandType, CommandType::MSG_BATTLE_CAST> CAST{};
        static constexpr std::integral_constant<CommandType, CommandType::MSG_BATTLE_MORALE> MORALE{};
        static constexpr std::integral_constant<CommandType, CommandType::MSG_BATTLE_CATAPULT> CATAPULT{};
        static constexpr std::integral_constant<CommandType, CommandType::MSG_BATTLE_TOWER> TOWER{};
        static constexpr std::integral_constant<CommandType, CommandType::MSG_BATTLE_RETREAT> RETREAT{};
        static constexpr std::integral_constant<CommandType, CommandType::MSG_BATTLE_SURRENDER> SURRENDER{};
        static constexpr std::integral_constant<CommandType, CommandType::MSG_BATTLE_SKIP> SKIP{};
        static constexpr std::integral_constant<CommandType, CommandType::MSG_BATTLE_END_TURN> END_TURN{};
        static constexpr std::integral_constant<CommandType, CommandType::MSG_BATTLE_AUTO_SWITCH> AUTO_SWITCH{};
        static constexpr std::integral_constant<CommandType, CommandType::MSG_BATTLE_AUTO_FINISH> AUTO_FINISH{};

        template <CommandType cmd, typename... Types>
        Command( std::integral_constant<CommandType, cmd> /* tag */, const Types... params )
            : _type( cmd )
        {
            if constexpr ( cmd == CommandType::MSG_BATTLE_MOVE ) {
                // UID, cell index
                static_assert( sizeof...( params ) == 2 );
            }
            else if constexpr ( cmd == CommandType::MSG_BATTLE_ATTACK ) {
                // Attacker UID, defender UID, cell index, direction
                static_assert( sizeof...( params ) == 4 );
            }
            else if constexpr ( cmd == CommandType::MSG_BATTLE_CAST ) {
                static_assert( sizeof...( params ) > 0 );

                const int spellId = std::get<0>( std::make_tuple( params... ) );

                if ( spellId == Spell::MIRRORIMAGE ) {
                    // Spell, UID
                    assert( sizeof...( params ) == 2 );
                }
                else if ( spellId == Spell::TELEPORT ) {
                    // Spell, src index, dst index
                    assert( sizeof...( params ) == 3 );
                }
                else {
                    // Spell, cell index
                    assert( sizeof...( params ) == 2 );
                }
            }
            else if constexpr ( cmd == CommandType::MSG_BATTLE_MORALE ) {
                // UID, morale
                static_assert( sizeof...( params ) == 2 );
            }
            else if constexpr ( cmd == CommandType::MSG_BATTLE_TOWER ) {
                // Tower type, UID
                static_assert( sizeof...( params ) == 2 );
            }
            else if constexpr ( cmd == CommandType::MSG_BATTLE_SKIP ) {
                // UID
                static_assert( sizeof...( params ) == 1 );
            }
            else if constexpr ( cmd == CommandType::MSG_BATTLE_END_TURN ) {
                // UID
                static_assert( sizeof...( params ) == 1 );
            }
            else if constexpr ( cmd == CommandType::MSG_BATTLE_AUTO_SWITCH ) {
                // Color
                static_assert( sizeof...( params ) == 1 );
            }
            else {
                static_assert( sizeof...( params ) == 0 );
            }

            if constexpr ( sizeof...( params ) > 0 ) {
                int dummy = 0;

                // Put the elements of the parameter pack in reverse order using the right-to-left associativity of the assignment operator
                (void)( ( *this << params, dummy ) = ... );
            }
        }

        explicit Command( const CommandType cmd )
            : _type( cmd )
        {}

        CommandType GetType() const
        {
            return _type;
        }

        int GetNextValue();

        bool isType( const CommandType cmd ) const
        {
            return _type == cmd;
        }

        Command & operator<<( const int val );
        Command & operator>>( int & val );

        uint32_t updateSeed( const uint32_t seed ) const;

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
