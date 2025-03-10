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

#include "captain.h"

#include <cassert>

#include "agg_image.h"
#include "artifact.h"
#include "artifact_info.h"
#include "castle.h"
#include "icn.h"
#include "interface_icons.h"
#include "luck.h"
#include "math_base.h"
#include "morale.h"
#include "race.h"
#include "spell_book.h"

class Army;

namespace
{
    int GetPortraitIcnId( int race )
    {
        switch ( race ) {
        case Race::KNGT:
            return ICN::PORT0090;
        case Race::BARB:
            return ICN::PORT0091;
        case Race::SORC:
            return ICN::PORT0092;
        case Race::WRLK:
            return ICN::PORT0093;
        case Race::WZRD:
            return ICN::PORT0094;
        case Race::NECR:
            return ICN::PORT0095;
        default:
            return -1;
        }
    }

    fheroes2::Point GetFlagOffset( int race )
    {
        switch ( race ) {
        case Race::KNGT:
            return { 43, 9 };
        case Race::BARB:
            return { 42, 8 };
        case Race::SORC:
            return { 43, 9 };
        case Race::WRLK:
            return { 41, 9 };
        case Race::WZRD:
            return { 42, 10 };
        case Race::NECR:
            return { 42, 9 };
        default:
            return {};
        }
    }
}

Captain::Captain( Castle & castle )
    : HeroBase( HeroBase::CAPTAIN, castle.GetRace() )
    , home( castle )
{
    SetCenter( home.GetCenter() );
}

bool Captain::isValid() const
{
    return home.isBuild( BUILD_CAPTAIN );
}

int Captain::GetAttack() const
{
    return attack + GetAttackModificator( nullptr );
}

int Captain::GetDefense() const
{
    return defense + GetDefenseModificator( nullptr );
}

int Captain::GetPower() const
{
    const int finalPower = power + GetPowerModificator( nullptr );
    return finalPower < 1 ? 1 : ( finalPower > 255 ? 255 : finalPower );
}

int Captain::GetKnowledge() const
{
    return knowledge + GetKnowledgeModificator( nullptr );
}

int Captain::GetMorale() const
{
    int result = Morale::NORMAL;

    // global modificator
    result += GetMoraleModificator( nullptr );

    // A special artifact ability presence must be the last check.
    const Artifact maxMoraleArtifact = bag_artifacts.getFirstArtifactWithBonus( fheroes2::ArtifactBonusType::MAXIMUM_MORALE );
    if ( maxMoraleArtifact.isValid() ) {
        result = Morale::BLOOD;
    }

    return Morale::Normalize( result );
}

int Captain::GetLuck() const
{
    int result = Luck::NORMAL;

    // global modificator
    result += GetLuckModificator( nullptr );

    // A special artifact ability presence must be the last check.
    const Artifact maxLuckArtifact = bag_artifacts.getFirstArtifactWithBonus( fheroes2::ArtifactBonusType::MAXIMUM_LUCK );
    if ( maxLuckArtifact.isValid() ) {
        result = Luck::IRISH;
    }

    return Luck::Normalize( result );
}

int Captain::GetRace() const
{
    return home.GetRace();
}

int Captain::GetColor() const
{
    return home.GetColor();
}

const std::string & Captain::GetName() const
{
    return home.GetName();
}

int Captain::GetType() const
{
    return HeroBase::CAPTAIN;
}

const Army & Captain::GetArmy() const
{
    return home.GetArmy();
}

Army & Captain::GetArmy()
{
    return home.GetArmy();
}

int Captain::GetControl() const
{
    return home.GetControl();
}

void Captain::ActionAfterBattle()
{
    SetSpellPoints( GetMaxSpellPoints() );
}

void Captain::ActionPreBattle()
{
    spell_book.resetState();
}

const Castle * Captain::inCastle() const
{
    return &home;
}

fheroes2::Sprite Captain::GetPortrait( const PortraitType type ) const
{
    switch ( type ) {
    case PORT_BIG: {
        const int portraitIcnId = GetPortraitIcnId( GetRace() );
        if ( portraitIcnId < 0 )
            return fheroes2::Image();

        fheroes2::Sprite portrait = fheroes2::AGG::GetICN( portraitIcnId, 0 );
        const fheroes2::Image & flag = fheroes2::AGG::GetICN( ICN::getFlagIcnId( GetColor() ), 0 );

        const fheroes2::Point & offset = GetFlagOffset( GetRace() );
        fheroes2::Blit( flag, portrait, offset.x, offset.y );
        return portrait;
    }
    case PORT_MEDIUM:
    case PORT_SMALL:
        switch ( GetRace() ) {
        case Race::KNGT:
            return fheroes2::AGG::GetICN( ICN::MINICAPT, 0 );
        case Race::BARB:
            return fheroes2::AGG::GetICN( ICN::MINICAPT, 1 );
        case Race::SORC:
            return fheroes2::AGG::GetICN( ICN::MINICAPT, 2 );
        case Race::WRLK:
            return fheroes2::AGG::GetICN( ICN::MINICAPT, 3 );
        case Race::WZRD:
            return fheroes2::AGG::GetICN( ICN::MINICAPT, 4 );
        case Race::NECR:
            return fheroes2::AGG::GetICN( ICN::MINICAPT, 5 );
        default:
            break;
        }
        break;
    default:
        break;
    }

    // We shouldn't even reach this code!
    assert( 0 );
    return fheroes2::AGG::GetICN( -1, 0 );
}

void Captain::PortraitRedraw( const int32_t px, const int32_t py, const PortraitType type, fheroes2::Image & dstsf ) const
{
    if ( !isValid() )
        return;

    const fheroes2::Sprite & port = GetPortrait( type );
    if ( PORT_SMALL != type ) { // a normal portrait in a castle or in battle
        fheroes2::Blit( port, dstsf, px, py );
        return;
    }

    const fheroes2::Sprite & mana = fheroes2::AGG::GetICN( ICN::MANA, GetManaIndexSprite() );

    const int iconWidth = Interface::IconsBar::getItemWidth();
    const int iconHeight = Interface::IconsBar::getItemHeight();
    const int barWidth = 7;

    // background
    fheroes2::Fill( dstsf, px, py, iconWidth, iconHeight, 0 );

    // mobility is always 0
    const uint8_t blueColor = fheroes2::GetColorId( 15, 30, 120 );
    fheroes2::Fill( dstsf, px, py, barWidth, iconHeight, blueColor );

    // portrait
    fheroes2::Blit( port, dstsf, px + barWidth + 1, py );

    // spell points
    fheroes2::Fill( dstsf, px + barWidth + port.width() + 2, py, barWidth, iconHeight, blueColor );
    fheroes2::Blit( mana, dstsf, px + barWidth + port.width() + 2, py + mana.y() );
}

int Captain::GetManaIndexSprite() const
{
    // valid range (0 - 25)
    const int r = GetMaxSpellPoints() / 5;

    return 25 >= r ? r : 25;
}
