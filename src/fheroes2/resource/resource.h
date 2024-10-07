/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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
#ifndef H2RESOURCE_H
#define H2RESOURCE_H

#include <cstdint>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>

#include "math_base.h"

class IStreamBase;
class OStreamBase;

struct Cost
{
    uint16_t gold;
    uint8_t wood;
    uint8_t mercury;
    uint8_t ore;
    uint8_t sulfur;
    uint8_t crystal;
    uint8_t gems;
};

namespace Resource
{
    enum
    {
        UNKNOWN = 0x00,
        WOOD = 0x01,
        MERCURY = 0x02,
        ORE = 0x04,
        SULFUR = 0x08,
        CRYSTAL = 0x10,
        GEMS = 0x20,
        GOLD = 0x40,
        ALL = WOOD | MERCURY | ORE | SULFUR | CRYSTAL | GEMS | GOLD
    };
}

struct Funds
{
    Funds() = default;

    Funds( const int32_t _ore, const int32_t _wood, const int32_t _mercury, const int32_t _sulfur, const int32_t _crystal, const int32_t _gems, const int32_t _gold )
        : wood( _wood )
        , mercury( _mercury )
        , ore( _ore )
        , sulfur( _sulfur )
        , crystal( _crystal )
        , gems( _gems )
        , gold( _gold )
    {
        // Do nothing.
    }

    Funds( const int type, const uint32_t count );

    explicit Funds( const Cost & cost )
        : wood( cost.wood )
        , mercury( cost.mercury )
        , ore( cost.ore )
        , sulfur( cost.sulfur )
        , crystal( cost.crystal )
        , gems( cost.gems )
        , gold( cost.gold )
    {
        // Do nothing.
    }

    Funds operator+( const Funds & pm ) const;
    Funds operator*( uint32_t mul ) const;
    Funds operator-( const Funds & pm ) const;
    Funds operator/( const int32_t div ) const;
    Funds & operator+=( const Funds & pm );
    Funds & operator*=( uint32_t mul );
    Funds & operator/=( const int32_t div );
    Funds & operator-=( const Funds & pm );
    Funds & operator=( const Cost & cost );

    bool operator==( const Funds & other ) const;
    bool operator>=( const Funds & other ) const;

    bool operator<( const Funds & other ) const
    {
        return !operator>=( other );
    }

    Funds max( const Funds & other ) const;

    int32_t Get( const int type ) const;
    int32_t * GetPtr( int rs );

    int getLowestQuotient( const Funds & divisor ) const;

    int GetValidItems() const;
    uint32_t GetValidItemsCount() const;
    std::pair<int, int32_t> getFirstValidResource() const;

    // Sets all values to be >= 0
    void Trim();
    void Reset();

    std::string String() const;

    int32_t wood{ 0 };
    int32_t mercury{ 0 };
    int32_t ore{ 0 };
    int32_t sulfur{ 0 };
    int32_t crystal{ 0 };
    int32_t gems{ 0 };
    int32_t gold{ 0 };
};

OStreamBase & operator<<( OStreamBase & stream, const Funds & res );
IStreamBase & operator>>( IStreamBase & stream, Funds & res );

namespace Resource
{
    const char * String( const int resourceType );

    const char * getDescription();

    int Rand( const bool includeGold );

    Funds CalculateEventResourceUpdate( const Funds & currentFunds, const Funds & eventFunds );

    // Returns index sprite objnrsrc.icn
    uint8_t GetIndexSprite( int resource );
    int FromIndexSprite( uint32_t index );

    // Return index sprite from resource.icn.
    uint32_t getIconIcnIndex( const int resourceType );

    int getResourceTypeFromIconIndex( const uint32_t index );

    class BoxSprite : protected fheroes2::Rect
    {
    public:
        BoxSprite( const Funds &, int32_t );

        const fheroes2::Rect & GetArea() const;
        void SetPos( int32_t, int32_t );
        void Redraw() const;

        const Funds rs;
    };

    // Applies the given function object 'fn' to every valid resource in the 'resources' set
    template <typename T, typename F, std::enable_if_t<std::is_integral_v<T> || std::is_enum_v<T>, bool> = true>
    void forEach( const T resources, const F & fn )
    {
        const auto forEachImpl = [&fn]( const auto res ) {
            constexpr int maxResourceIdBitNum = []() constexpr
            {
                static_assert( std::is_enum_v<decltype( Resource::ALL )> );
                using ResourceUnderlyingType = std::underlying_type_t<decltype( Resource::ALL )>;
                static_assert( std::numeric_limits<ResourceUnderlyingType>::radix == 2 );

                for ( int i = std::numeric_limits<ResourceUnderlyingType>::digits - 1; i >= 0; --i ) {
                    if ( ( Resource::ALL & ( static_cast<ResourceUnderlyingType>( 1 ) << i ) ) != 0 ) {
                        return i;
                    }
                }

                return -1;
            }
            ();

            using ResType = decltype( res );

            static_assert( std::numeric_limits<ResType>::radix == 2 && maxResourceIdBitNum >= 0 && maxResourceIdBitNum < std::numeric_limits<ResType>::digits );

            for ( int i = 0; i <= maxResourceIdBitNum; ++i ) {
                const ResType resItem = res & ( static_cast<ResType>( 1 ) << i );
                if ( resItem == 0 ) {
                    continue;
                }

                fn( resItem );
            }
        };

        if constexpr ( std::is_enum_v<decltype( resources )> ) {
            forEachImpl( static_cast<std::underlying_type_t<decltype( resources )>>( resources ) );
        }
        else {
            forEachImpl( resources );
        }
    }
}

#endif
