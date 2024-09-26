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
        MOVE,
        ATTACK,
        SPELLCAST,
        MORALE,
        CATAPULT,
        TOWER,
        RETREAT,
        SURRENDER,
        SKIP,
        AUTO_SWITCH,
        AUTO_FINISH
    };

    class Command final : public std::vector<int>
    {
    public:
        static constexpr std::integral_constant<CommandType, CommandType::MOVE> MOVE{};
        static constexpr std::integral_constant<CommandType, CommandType::ATTACK> ATTACK{};
        static constexpr std::integral_constant<CommandType, CommandType::SPELLCAST> SPELLCAST{};
        static constexpr std::integral_constant<CommandType, CommandType::MORALE> MORALE{};
        static constexpr std::integral_constant<CommandType, CommandType::CATAPULT> CATAPULT{};
        static constexpr std::integral_constant<CommandType, CommandType::TOWER> TOWER{};
        static constexpr std::integral_constant<CommandType, CommandType::RETREAT> RETREAT{};
        static constexpr std::integral_constant<CommandType, CommandType::SURRENDER> SURRENDER{};
        static constexpr std::integral_constant<CommandType, CommandType::SKIP> SKIP{};
        static constexpr std::integral_constant<CommandType, CommandType::AUTO_SWITCH> AUTO_SWITCH{};
        static constexpr std::integral_constant<CommandType, CommandType::AUTO_FINISH> AUTO_FINISH{};

        template <CommandType cmd, typename... Types>
        explicit Command( std::integral_constant<CommandType, cmd> /* tag */, const Types... params )
            : _type( cmd )
        {
            if constexpr ( cmd == CommandType::MOVE ) {
                // UID, cell index
                static_assert( sizeof...( params ) == 2 );
            }
            else if constexpr ( cmd == CommandType::ATTACK ) {
                // Attacker UID, defender UID, cell index to move, cell index to attack, attack direction
                static_assert( sizeof...( params ) == 5 );
            }
            else if constexpr ( cmd == CommandType::SPELLCAST ) {
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
            else if constexpr ( cmd == CommandType::MORALE ) {
                // UID, morale
                static_assert( sizeof...( params ) == 2 );
            }
            else if constexpr ( cmd == CommandType::TOWER ) {
                // Tower type, UID
                static_assert( sizeof...( params ) == 2 );
            }
            else if constexpr ( cmd == CommandType::SKIP ) {
                // UID
                static_assert( sizeof...( params ) == 1 );
            }
            else if constexpr ( cmd == CommandType::AUTO_SWITCH ) {
                // Color
                static_assert( sizeof...( params ) == 1 );
            }
            else {
                static_assert( sizeof...( params ) == 0 );
            }

            if constexpr ( sizeof...( params ) > 0 ) {
                reserve( sizeof...( params ) );

                // Put the elements of the parameter pack in reverse order using the right-to-left sequencing of the assignment operator
                int dummy = 0;
                (void)( ( *this << params, dummy ) = ... );
            }
        }

        CommandType GetType() const
        {
            return _type;
        }

        int GetNextValue();

        // Updates the specified seed using the contents of this command. Returns the updated seed (or the original seed if this command is not suitable for seed update).
        uint32_t updateSeed( uint32_t seed ) const;

        Command & operator<<( const int val );

    private:
        Command & operator>>( int & val );

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
            using BattleCommandTypeUnderlyingType = std::underlying_type_t<Battle::CommandType>;

            const std::hash<BattleCommandTypeUnderlyingType> hasher;

            return hasher( static_cast<BattleCommandTypeUnderlyingType>( key ) );
        }
    };
}

#endif
