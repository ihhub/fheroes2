/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2025                                                    *
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

#pragma once

#include <type_traits>

namespace Enum
{
    template <typename FlagBitsType>
    struct FlagTraits
    {
        static constexpr bool isBitmask = false;
    };

    // Wrapper for bitwise operations of enum classes.
    template <typename BitType>
    class Flags
    {
    public:
        using BitsType = BitType;
        using MaskType = std::underlying_type_t<BitType>;

        constexpr Flags() noexcept
            : m_mask( 0 )
        {}

        constexpr Flags( BitType bit ) noexcept
            : m_mask( static_cast<MaskType>( bit ) )
        {}

        constexpr Flags( Flags const & rhs ) noexcept = default;

        constexpr explicit Flags( MaskType flags ) noexcept
            : m_mask( flags )
        {}

        Flags( Flags && other ) noexcept = default;

        Flags & operator=( Flags && other ) noexcept = default;

        ~Flags() = default;

        constexpr bool operator<( Flags const & rhs ) const noexcept
        {
            return m_mask < rhs.m_mask;
        }

        constexpr bool operator<=( Flags const & rhs ) const noexcept
        {
            return m_mask <= rhs.m_mask;
        }

        constexpr bool operator>( Flags const & rhs ) const noexcept
        {
            return m_mask > rhs.m_mask;
        }

        constexpr bool operator>=( Flags const & rhs ) const noexcept
        {
            return m_mask >= rhs.m_mask;
        }

        constexpr bool operator==( Flags const & rhs ) const noexcept
        {
            return m_mask == rhs.m_mask;
        }

        constexpr bool operator!=( Flags const & rhs ) const noexcept
        {
            return m_mask != rhs.m_mask;
        }

        constexpr bool operator!() const noexcept
        {
            return !m_mask;
        }

        constexpr Flags operator&( Flags const & rhs ) const noexcept
        {
            return Flags( m_mask & rhs.m_mask );
        }

        constexpr Flags operator|( Flags const & rhs ) const noexcept
        {
            return Flags( m_mask | rhs.m_mask );
        }

        constexpr Flags operator^( Flags const & rhs ) const noexcept
        {
            return Flags( m_mask ^ rhs.m_mask );
        }

        constexpr Flags operator~() const noexcept
        {
            return Flags( m_mask ^ FlagTraits<BitType>::allFlags.m_mask );
        }

        constexpr Flags & operator=( Flags const & rhs ) noexcept = default;

        constexpr Flags & operator|=( Flags const & rhs ) noexcept
        {
            m_mask |= rhs.m_mask;
            return *this;
        }

        constexpr Flags & operator&=( Flags const & rhs ) noexcept
        {
            m_mask &= rhs.m_mask;
            return *this;
        }

        constexpr Flags & operator^=( Flags const & rhs ) noexcept
        {
            m_mask ^= rhs.m_mask;
            return *this;
        }

        // Cast operators
        explicit constexpr operator bool() const noexcept
        {
            return !!m_mask;
        }

        explicit constexpr operator MaskType() const noexcept
        {
            return m_mask;
        }

    private:
        MaskType m_mask;
    };

    template <typename BitType>
    constexpr bool operator<( BitType bit, Flags<BitType> const & flags ) noexcept
    {
        return flags.operator>( bit );
    }

    template <typename BitType>
    constexpr bool operator<=( BitType bit, Flags<BitType> const & flags ) noexcept
    {
        return flags.operator>=( bit );
    }

    template <typename BitType>
    constexpr bool operator>( BitType bit, Flags<BitType> const & flags ) noexcept
    {
        return flags.operator<( bit );
    }

    template <typename BitType>
    constexpr bool operator>=( BitType bit, Flags<BitType> const & flags ) noexcept
    {
        return flags.operator<=( bit );
    }

    template <typename BitType>
    constexpr bool operator==( BitType bit, Flags<BitType> const & flags ) noexcept
    {
        return flags.operator==( bit );
    }

    template <typename BitType>
    constexpr bool operator!=( BitType bit, Flags<BitType> const & flags ) noexcept
    {
        return flags.operator!=( bit );
    }

    // bitwise operators
    template <typename BitType>
    constexpr Flags<BitType> operator&( BitType bit, Flags<BitType> const & flags ) noexcept
    {
        return flags.operator&( bit );
    }

    template <typename BitType>
    constexpr Flags<BitType> operator|( BitType bit, Flags<BitType> const & flags ) noexcept
    {
        return flags.operator|( bit );
    }

    template <typename BitType>
    constexpr Flags<BitType> operator^( BitType bit, Flags<BitType> const & flags ) noexcept
    {
        return flags.operator^( bit );
    }

    // bitwise operators on BitType
    template <typename BitType, std::enable_if_t<FlagTraits<BitType>::isBitmask, bool> = true>
    constexpr Flags<BitType> operator&( BitType lhs, BitType rhs ) noexcept
    {
        return Flags<BitType>( lhs ) & rhs;
    }

    template <typename BitType, std::enable_if_t<FlagTraits<BitType>::isBitmask, bool> = true>
    constexpr Flags<BitType> operator|( BitType lhs, BitType rhs ) noexcept
    {
        return Flags<BitType>( lhs ) | rhs;
    }

    template <typename BitType, std::enable_if_t<FlagTraits<BitType>::isBitmask, bool> = true>
    constexpr Flags<BitType> operator^( BitType lhs, BitType rhs ) noexcept
    {
        return Flags<BitType>( lhs ) ^ rhs;
    }

    template <typename BitType, std::enable_if_t<FlagTraits<BitType>::isBitmask, bool> = true>
    constexpr Flags<BitType> operator~( BitType bit ) noexcept
    {
        return ~Flags<BitType>( bit );
    }

}
