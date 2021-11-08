/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
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

#include <string>

#include "math_base.h"
#include "types.h"

class StreamBase;
class ResourceCount;

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

class Funds
{
public:
    Funds();
    Funds( const int32_t _ore, const int32_t _wood, const int32_t _mercury, const int32_t _sulfur, const int32_t _crystal, const int32_t _gems, const int32_t _gold );
    Funds( const int rs, const uint32_t count );
    explicit Funds( const ResourceCount & rs );

    Funds operator+( const Funds & pm ) const;
    Funds operator*( uint32_t mul ) const;
    Funds operator-( const Funds & pm ) const;
    Funds & operator+=( const Funds & pm );
    Funds & operator*=( uint32_t mul );
    Funds & operator-=( const Funds & pm );

    int32_t Get( const int rs ) const;
    int32_t * GetPtr( const int rs );

    bool operator>=( const Funds & pm ) const;

    int getLowestQuotient( const Funds & divisor ) const;
    int GetValidItems() const;
    u32 GetValidItemsCount() const;

    void Trim(); // set all values to be >= 0

    void Reset();
    std::string String() const;

    int32_t wood;
    int32_t mercury;
    int32_t ore;
    int32_t sulfur;
    int32_t crystal;
    int32_t gems;
    int32_t gold;
};

StreamBase & operator<<( StreamBase &, const Funds & );
StreamBase & operator>>( StreamBase &, Funds & );

namespace Resource
{
    const char * String( const int resource );
    int Rand( const bool includeGold );

    /* return index sprite objnrsrc.icn */
    uint32_t GetIndexSprite( const int resource );
    int FromIndexSprite( const uint32_t index );

    /* return index sprite resource.icn */
    uint32_t GetIndexSprite2( const int resource );
    int FromIndexSprite2( const uint32_t index );

    class BoxSprite : protected fheroes2::Rect
    {
    public:
        BoxSprite( const Funds &, const int32_t width_ );

        const fheroes2::Rect & GetArea( void ) const;
        void SetPos( const int32_t px, const int32_t py );
        void Redraw() const;

        const Funds rs;
    };
}

#endif
