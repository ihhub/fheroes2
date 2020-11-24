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

#include "captain.h"
#include "agg.h"
#include "castle.h"
#include "interface_icons.h"
#include "luck.h"
#include "morale.h"
#include "race.h"

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
            return fheroes2::Point( 43, 9 );
        case Race::BARB:
            return fheroes2::Point( 42, 8 );
        case Race::SORC:
            return fheroes2::Point( 43, 9 );
        case Race::WRLK:
            return fheroes2::Point( 41, 9 );
        case Race::WZRD:
            return fheroes2::Point( 42, 10 );
        case Race::NECR:
            return fheroes2::Point( 42, 9 );
        default:
            return fheroes2::Point();
        }
    }
}

Captain::Captain( Castle & cstl )
    : HeroBase( HeroBase::CAPTAIN, cstl.GetRace() )
    , home( cstl )
{
    SetCenter( home.GetCenter() );
}

bool Captain::isValid( void ) const
{
    return home.isBuild( BUILD_CAPTAIN );
}

int Captain::GetAttack( void ) const
{
    return attack + GetAttackModificator( NULL );
}

int Captain::GetDefense( void ) const
{
    return defense + GetDefenseModificator( NULL );
}

int Captain::GetPower( void ) const
{
    const int finalPower = power + GetPowerModificator( NULL );
    return finalPower < 1 ? 1 : ( finalPower > 255 ? 255 : finalPower );
}

int Captain::GetKnowledge( void ) const
{
    return knowledge + GetKnowledgeModificator( NULL );
}

int Captain::GetMorale( void ) const
{
    int result = Morale::NORMAL;

    // global modificator
    result += GetMoraleModificator( NULL );

    // result
    if ( result < Morale::AWFUL )
        return Morale::TREASON;
    else if ( result < Morale::POOR )
        return Morale::AWFUL;
    else if ( result < Morale::NORMAL )
        return Morale::POOR;
    else if ( result < Morale::GOOD )
        return Morale::NORMAL;
    else if ( result < Morale::GREAT )
        return Morale::GOOD;
    else if ( result < Morale::BLOOD )
        return Morale::GREAT;

    return Morale::BLOOD;
}

int Captain::GetLuck( void ) const
{
    int result = Luck::NORMAL;

    // global modificator
    result += GetLuckModificator( NULL );

    // result
    if ( result < Luck::AWFUL )
        return Luck::CURSED;
    else if ( result < Luck::BAD )
        return Luck::AWFUL;
    else if ( result < Luck::NORMAL )
        return Luck::BAD;
    else if ( result < Luck::GOOD )
        return Luck::NORMAL;
    else if ( result < Luck::GREAT )
        return Luck::GOOD;
    else if ( result < Luck::IRISH )
        return Luck::GREAT;

    return Luck::IRISH;
}

int Captain::GetRace( void ) const
{
    return home.GetRace();
}

int Captain::GetColor( void ) const
{
    return home.GetColor();
}

const std::string & Captain::GetName( void ) const
{
    return home.GetName();
}

int Captain::GetType( void ) const
{
    return HeroBase::CAPTAIN;
}

int Captain::GetLevelSkill( int ) const
{
    return 0;
}

u32 Captain::GetSecondaryValues( int ) const
{
    return 0;
}

const Army & Captain::GetArmy( void ) const
{
    return home.GetArmy();
}

Army & Captain::GetArmy( void )
{
    return home.GetArmy();
}

u32 Captain::GetMaxSpellPoints( void ) const
{
    return knowledge * 10;
}

int Captain::GetControl( void ) const
{
    return home.GetControl();
}

s32 Captain::GetIndex( void ) const
{
    return home.GetIndex();
}

void Captain::ActionAfterBattle( void )
{
    SetSpellPoints( GetMaxSpellPoints() );
}

void Captain::ActionPreBattle( void )
{
    SetSpellPoints( GetMaxSpellPoints() );
}

const Castle * Captain::inCastle( void ) const
{
    return &home;
}

fheroes2::Image Captain::GetPortrait( int type ) const
{
    switch ( type ) {
    case PORT_BIG: {
        const int portraitIcnId = GetPortraitIcnId( GetRace() );
        if ( portraitIcnId < 0 )
            return fheroes2::Image();

        fheroes2::Image portait = fheroes2::AGG::GetICN( portraitIcnId, 0 );
        const fheroes2::Image & flag = fheroes2::AGG::GetICN( ICN::GetFlagIcnId( GetColor() ), 0 );

        const fheroes2::Point & offset = GetFlagOffset( GetRace() );
        fheroes2::Blit( flag, portait, offset.x, offset.y );
        return portait;
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
    }

    return fheroes2::Image();
}

void Captain::PortraitRedraw( s32 px, s32 py, int type, fheroes2::Image & dstsf ) const
{
    if ( !isValid() )
        return;

    const fheroes2::Image & port = GetPortrait( type );
    if ( PORT_SMALL != type ) { // a normal portrait in a castle or in battle
        fheroes2::Blit( port, dstsf, px, py );
        return;
    }

    const int iconWidth = Interface::IconsBar::GetItemWidth();
    const int iconHeight = Interface::IconsBar::GetItemHeight();
    const int barWidth = 7;

    fheroes2::Image blackBG( iconWidth, iconHeight );
    blackBG.fill( 0 );
    fheroes2::Image blueBG( barWidth, iconHeight );
    blueBG.fill( fheroes2::GetColorId( 15, 30, 120 ) );

    // background
    fheroes2::Blit( blackBG, dstsf, px, py );

    // mobility is always 0
    fheroes2::Blit( blueBG, dstsf, px, py );

    // portrait
    fheroes2::Blit( port, dstsf, px + barWidth + 1, py );

    // spell points
    fheroes2::Blit( blueBG, dstsf, px + barWidth + port.width() + 2, py );
    const fheroes2::Sprite & mana = fheroes2::AGG::GetICN( ICN::MANA, GetMaxSpellPoints() );
    fheroes2::Blit( mana, dstsf, px + barWidth + port.width() + 2, py + mana.y() );
}
