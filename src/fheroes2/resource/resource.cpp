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

#include "resource.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <sstream>
#include <tuple>
#include <utility>
#include <vector>

#include "agg_image.h"
#include "icn.h"
#include "image.h"
#include "logging.h"
#include "rand.h"
#include "screen.h"
#include "serialize.h"
#include "translations.h"
#include "ui_text.h"

namespace
{
    void RedrawResourceSprite( const fheroes2::Image & sf, const fheroes2::Point & pos, int32_t count, int32_t width, int32_t offset, int32_t value )
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        const fheroes2::Point dst_pt( pos.x + width / 2 + count * width, pos.y + offset );
        fheroes2::Blit( sf, display, dst_pt.x - sf.width() / 2, dst_pt.y - sf.height() );

        const fheroes2::Text text{ std::to_string( value ), fheroes2::FontType::smallWhite() };
        text.draw( dst_pt.x - text.width() / 2, dst_pt.y + 4, display );
    }
}

Funds::Funds( const int type, const uint32_t count )
{
    if ( count == 0 ) {
        // Nothing to add. Skip it.
        return;
    }

    switch ( type ) {
    case Resource::ORE:
        ore = count;
        break;
    case Resource::WOOD:
        wood = count;
        break;
    case Resource::MERCURY:
        mercury = count;
        break;
    case Resource::SULFUR:
        sulfur = count;
        break;
    case Resource::GEMS:
        gems = count;
        break;
    case Resource::CRYSTAL:
        crystal = count;
        break;
    case Resource::GOLD:
        gold = count;
        break;

    default:
        DEBUG_LOG( DBG_GAME, DBG_WARN, "Unknown resource is being added to Funds class. Ignore it." )
        break;
    }
}

int Resource::Rand( const bool includeGold )
{
    switch ( Rand::Get( 1, ( includeGold ? 7 : 6 ) ) ) {
    case 1:
        return Resource::WOOD;
    case 2:
        return Resource::MERCURY;
    case 3:
        return Resource::ORE;
    case 4:
        return Resource::SULFUR;
    case 5:
        return Resource::CRYSTAL;
    case 6:
        return Resource::GEMS;
    case 7:
        return Resource::GOLD;
    default:
        break;
    }

    return Resource::UNKNOWN;
}

int32_t * Funds::GetPtr( int rs )
{
    switch ( rs ) {
    case Resource::ORE:
        return &ore;
    case Resource::WOOD:
        return &wood;
    case Resource::MERCURY:
        return &mercury;
    case Resource::SULFUR:
        return &sulfur;
    case Resource::GEMS:
        return &gems;
    case Resource::CRYSTAL:
        return &crystal;
    case Resource::GOLD:
        return &gold;
    default:
        break;
    }
    return nullptr;
}

int32_t Funds::Get( const int type ) const
{
    switch ( type ) {
    case Resource::ORE:
        return ore;
    case Resource::WOOD:
        return wood;
    case Resource::MERCURY:
        return mercury;
    case Resource::SULFUR:
        return sulfur;
    case Resource::GEMS:
        return gems;
    case Resource::CRYSTAL:
        return crystal;
    case Resource::GOLD:
        return gold;
    default:
        break;
    }
    return 0;
}

Funds & Funds::operator=( const Cost & cost )
{
    wood = cost.wood;
    mercury = cost.mercury;
    ore = cost.ore;
    sulfur = cost.sulfur;
    crystal = cost.crystal;
    gems = cost.gems;
    gold = cost.gold;

    return *this;
}

Funds Funds::operator+( const Funds & pm ) const
{
    Funds res;

    res.wood = wood + pm.wood;
    res.mercury = mercury + pm.mercury;
    res.ore = ore + pm.ore;
    res.sulfur = sulfur + pm.sulfur;
    res.crystal = crystal + pm.crystal;
    res.gems = gems + pm.gems;
    res.gold = gold + pm.gold;

    return res;
}

Funds & Funds::operator+=( const Funds & pm )
{
    wood += pm.wood;
    mercury += pm.mercury;
    ore += pm.ore;
    sulfur += pm.sulfur;
    crystal += pm.crystal;
    gems += pm.gems;
    gold += pm.gold;

    return *this;
}

Funds Funds::operator-( const Funds & pm ) const
{
    Funds res;

    res.wood = wood - pm.wood;
    res.mercury = mercury - pm.mercury;
    res.ore = ore - pm.ore;
    res.sulfur = sulfur - pm.sulfur;
    res.crystal = crystal - pm.crystal;
    res.gems = gems - pm.gems;
    res.gold = gold - pm.gold;

    return res;
}

Funds Funds::operator/( const int32_t div ) const
{
    if ( div == 0 ) {
        assert( 0 );
        return {};
    }

    Funds res;

    res.wood = wood / div;
    res.mercury = mercury / div;
    res.ore = ore / div;
    res.sulfur = sulfur / div;
    res.crystal = crystal / div;
    res.gems = gems / div;
    res.gold = gold / div;

    return res;
}

Funds & Funds::operator-=( const Funds & pm )
{
    wood -= pm.wood;
    mercury -= pm.mercury;
    ore -= pm.ore;
    sulfur -= pm.sulfur;
    crystal -= pm.crystal;
    gems -= pm.gems;
    gold -= pm.gold;

    return *this;
}

Funds Funds::max( const Funds & other ) const
{
    Funds max;

    max.wood = std::max( wood, other.wood );
    max.mercury = std::max( mercury, other.mercury );
    max.ore = std::max( ore, other.ore );
    max.sulfur = std::max( sulfur, other.sulfur );
    max.crystal = std::max( crystal, other.crystal );
    max.gems = std::max( gems, other.gems );
    max.gold = std::max( gold, other.gold );

    return max;
}

int Funds::getLowestQuotient( const Funds & divisor ) const
{
    int result = ( divisor.gold ) ? gold / divisor.gold : gold;

    const auto divisionLambda = [&result]( int left, int right ) {
        if ( right > 0 ) {
            const int value = left / right;
            if ( value < result )
                result = value;
        }
    };

    divisionLambda( wood, divisor.wood );
    divisionLambda( ore, divisor.ore );
    divisionLambda( crystal, divisor.crystal );
    divisionLambda( gems, divisor.gems );
    divisionLambda( mercury, divisor.mercury );
    divisionLambda( sulfur, divisor.sulfur );

    return result;
}

Funds Funds::operator*( uint32_t mul ) const
{
    Funds res;

    res.wood = wood * mul;
    res.mercury = mercury * mul;
    res.ore = ore * mul;
    res.sulfur = sulfur * mul;
    res.crystal = crystal * mul;
    res.gems = gems * mul;
    res.gold = gold * mul;

    return res;
}

Funds & Funds::operator*=( uint32_t mul )
{
    wood *= mul;
    mercury *= mul;
    ore *= mul;
    sulfur *= mul;
    crystal *= mul;
    gems *= mul;
    gold *= mul;

    return *this;
}

Funds & Funds::operator/=( const int32_t div )
{
    if ( div == 0 ) {
        assert( 0 );
        return *this;
    }

    wood /= div;
    mercury /= div;
    ore /= div;
    sulfur /= div;
    crystal /= div;
    gems /= div;
    gold /= div;

    return *this;
}

bool Funds::operator==( const Funds & other ) const
{
    return std::tie( wood, mercury, ore, sulfur, crystal, gems, gold )
           == std::tie( other.wood, other.mercury, other.ore, other.sulfur, other.crystal, other.gems, other.gold );
}

bool Funds::operator>=( const Funds & other ) const
{
    return wood >= other.wood && mercury >= other.mercury && ore >= other.ore && sulfur >= other.sulfur && crystal >= other.crystal && gems >= other.gems
           && gold >= other.gold;
}

std::string Funds::String() const
{
    std::ostringstream os;
    os << "ore: " << ore << ", wood: " << wood << ", mercury: " << mercury << ", sulfur: " << sulfur << ", crystal: " << crystal << ", gems: " << gems
       << ", gold: " << gold;
    return os.str();
}

const char * Resource::String( const int resourceType )
{
    switch ( resourceType ) {
    case Resource::WOOD:
        return _( "Wood" );
    case Resource::MERCURY:
        return _( "Mercury" );
    case Resource::ORE:
        return _( "Ore" );
    case Resource::SULFUR:
        return _( "Sulfur" );
    case Resource::CRYSTAL:
        return _( "Crystal" );
    case Resource::GEMS:
        return _( "Gems" );
    case Resource::GOLD:
        return _( "Gold" );
    default:
        break;
    }

    return "Unknown";
}

const char * Resource::getDescription()
{
    return _( "There are seven resources in Heroes 2, used to build and improves castles, purchase troops and recruit heroes. Gold is the most common, required for "
              "virtually everything. Wood and ore are used for most buildings. Gems, Mercury, Sulfur and Crystal are rare magical resources used for the most "
              "powerful creatures and buildings." );
}

uint8_t Resource::GetIndexSprite( int resource )
{
    switch ( resource ) {
    case Resource::WOOD:
        return 1;
    case Resource::MERCURY:
        return 3;
    case Resource::ORE:
        return 5;
    case Resource::SULFUR:
        return 7;
    case Resource::CRYSTAL:
        return 9;
    case Resource::GEMS:
        return 11;
    case Resource::GOLD:
        return 13;
    default:
        DEBUG_LOG( DBG_GAME, DBG_WARN, "Unknown resource type." )
        break;
    }

    return 0;
}

int Resource::FromIndexSprite( uint32_t index )
{
    switch ( index ) {
    case 1:
        return WOOD;
    case 3:
        return MERCURY;
    case 5:
        return ORE;
    case 7:
        return SULFUR;
    case 9:
        return CRYSTAL;
    case 11:
        return GEMS;
    case 13:
        return GOLD;

    default:
        break;
    }

    return UNKNOWN;
}

uint32_t Resource::getIconIcnIndex( const int resourceType )
{
    switch ( resourceType ) {
    case Resource::WOOD:
        return 0;
    case Resource::MERCURY:
        return 1;
    case Resource::ORE:
        return 2;
    case Resource::SULFUR:
        return 3;
    case Resource::CRYSTAL:
        return 4;
    case Resource::GEMS:
        return 5;
    case Resource::GOLD:
        return 6;
    default:
        // You are passing not a single resource type or an invalid one. Fix it!
        assert( 0 );
        DEBUG_LOG( DBG_GAME, DBG_WARN, "unknown resource" )
        break;
    }

    return 0;
}

int Resource::getResourceTypeFromIconIndex( const uint32_t index )
{
    switch ( index ) {
    case 0:
        return WOOD;
    case 1:
        return MERCURY;
    case 2:
        return ORE;
    case 3:
        return SULFUR;
    case 4:
        return CRYSTAL;
    case 5:
        return GEMS;
    case 6:
        return GOLD;
    default:
        // ICN index is wrong!
        assert( 0 );
        break;
    }

    return UNKNOWN;
}

Funds Resource::CalculateEventResourceUpdate( const Funds & currentFunds, const Funds & eventFunds )
{
    Funds funds = currentFunds + eventFunds;
    funds.Trim();
    return funds - currentFunds;
}

int Funds::GetValidItems() const
{
    int rs = 0;

    if ( wood )
        rs |= Resource::WOOD;
    if ( ore )
        rs |= Resource::ORE;
    if ( mercury )
        rs |= Resource::MERCURY;
    if ( sulfur )
        rs |= Resource::SULFUR;
    if ( crystal )
        rs |= Resource::CRYSTAL;
    if ( gems )
        rs |= Resource::GEMS;
    if ( gold )
        rs |= Resource::GOLD;

    return rs;
}

uint32_t Funds::GetValidItemsCount() const
{
    uint32_t result = 0;

    if ( wood )
        ++result;
    if ( ore )
        ++result;
    if ( mercury )
        ++result;
    if ( sulfur )
        ++result;
    if ( crystal )
        ++result;
    if ( gems )
        ++result;
    if ( gold )
        ++result;

    return result;
}

void Funds::Trim()
{
    if ( wood < 0 )
        wood = 0;
    if ( ore < 0 )
        ore = 0;
    if ( mercury < 0 )
        mercury = 0;
    if ( sulfur < 0 )
        sulfur = 0;
    if ( crystal < 0 )
        crystal = 0;
    if ( gems < 0 )
        gems = 0;
    if ( gold < 0 )
        gold = 0;
}

std::pair<int, int32_t> Funds::getFirstValidResource() const
{
    if ( wood > 0 ) {
        return { Resource::WOOD, wood };
    }

    if ( ore > 0 ) {
        return { Resource::ORE, ore };
    }

    if ( mercury > 0 ) {
        return { Resource::MERCURY, mercury };
    }

    if ( sulfur > 0 ) {
        return { Resource::SULFUR, sulfur };
    }

    if ( crystal > 0 ) {
        return { Resource::CRYSTAL, crystal };
    }

    if ( gems > 0 ) {
        return { Resource::GEMS, gems };
    }

    if ( gold > 0 ) {
        return { Resource::GOLD, gold };
    }

    // We shouldn't reach this point. Make sure that you are calling this method for valid Funds.
    assert( 0 );
    return { Resource::UNKNOWN, 0 };
}

void Funds::Reset()
{
    wood = 0;
    ore = 0;
    mercury = 0;
    sulfur = 0;
    crystal = 0;
    gems = 0;
    gold = 0;
}

Resource::BoxSprite::BoxSprite( const Funds & f, int32_t width_ )
    : fheroes2::Rect( 0, 0, width_, 0 )
    , rs( f )
{
    const uint32_t count = rs.GetValidItemsCount();
    height = 4 > count ? 45 : ( 7 > count ? 90 : 135 );
}

const fheroes2::Rect & Resource::BoxSprite::GetArea() const
{
    return *this;
}

void Resource::BoxSprite::SetPos( int32_t px, int32_t py )
{
    x = px;
    y = py;
}

void Resource::BoxSprite::Redraw() const
{
    std::vector<std::pair<int32_t, uint32_t>> valueVsSprite;

    if ( rs.wood )
        valueVsSprite.emplace_back( rs.wood, 0 );

    if ( rs.ore )
        valueVsSprite.emplace_back( rs.ore, 2 );

    if ( rs.mercury )
        valueVsSprite.emplace_back( rs.mercury, 1 );

    if ( rs.sulfur )
        valueVsSprite.emplace_back( rs.sulfur, 3 );

    if ( rs.crystal )
        valueVsSprite.emplace_back( rs.crystal, 4 );

    if ( rs.gems )
        valueVsSprite.emplace_back( rs.gems, 5 );

    if ( rs.gold )
        valueVsSprite.emplace_back( rs.gold, 6 );

    uint32_t offsetY = 35;
    size_t id = 0;

    while ( valueVsSprite.size() - id > 2 ) {
        const uint32_t width_ = width / 3;
        const fheroes2::Sprite & res1 = fheroes2::AGG::GetICN( ICN::RESOURCE, valueVsSprite[id].second );
        const fheroes2::Sprite & res2 = fheroes2::AGG::GetICN( ICN::RESOURCE, valueVsSprite[id + 1].second );
        const fheroes2::Sprite & res3 = fheroes2::AGG::GetICN( ICN::RESOURCE, valueVsSprite[id + 2].second );

        RedrawResourceSprite( res1, { x, y }, 0, width_, offsetY, valueVsSprite[id].first );
        RedrawResourceSprite( res2, { x, y }, 1, width_, offsetY, valueVsSprite[id + 1].first );
        RedrawResourceSprite( res3, { x, y }, 2, width_, offsetY, valueVsSprite[id + 2].first );

        id += 3;
        offsetY += 45;
    }

    const bool isManyResources = valueVsSprite.size() > 2;

    if ( valueVsSprite.size() - id == 2 ) {
        const fheroes2::Sprite & res1 = fheroes2::AGG::GetICN( ICN::RESOURCE, valueVsSprite[id].second );
        const fheroes2::Sprite & res2 = fheroes2::AGG::GetICN( ICN::RESOURCE, valueVsSprite[id + 1].second );

        const uint32_t width_ = isManyResources ? width / 3 : width / 2;
        const int32_t offsetX = isManyResources ? width_ / 2 : 0;

        RedrawResourceSprite( res1, { x + offsetX, y }, 0, width_, offsetY, valueVsSprite[id].first );
        RedrawResourceSprite( res2, { x + offsetX, y }, 1, width_, offsetY, valueVsSprite[id + 1].first );
    }
    else if ( valueVsSprite.size() - id == 1 ) {
        const fheroes2::Sprite & res1 = fheroes2::AGG::GetICN( ICN::RESOURCE, valueVsSprite[id].second );

        const int32_t width_ = isManyResources ? width / 3 : width;

        RedrawResourceSprite( res1, { x, y }, isManyResources ? 1 : 0, width_, offsetY, valueVsSprite[id].first );
    }
}

OStreamBase & operator<<( OStreamBase & stream, const Funds & res )
{
    return stream << res.wood << res.mercury << res.ore << res.sulfur << res.crystal << res.gems << res.gold;
}

IStreamBase & operator>>( IStreamBase & stream, Funds & res )
{
    return stream >> res.wood >> res.mercury >> res.ore >> res.sulfur >> res.crystal >> res.gems >> res.gold;
}
