/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
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

#include "army_troop.h"

#include "army.h"
#include "color.h"
#include "heroes_base.h"
#include "resource.h"
#include "serialize.h"
#include "speed.h"

Troop::Troop()
    : Monster( Monster::UNKNOWN )
    , count( 0 )
{}

Troop::Troop( const Monster & m, uint32_t c )
    : Monster( m )
    , count( c )
{}

bool Troop::operator==( const Monster & m ) const
{
    return Monster::operator==( m );
}

bool Troop::isMonster( int mons ) const
{
    return GetID() == mons;
}

Monster Troop::GetMonster() const
{
    return Monster( id );
}

void Troop::Set( const Troop & t )
{
    SetMonster( t.GetMonster() );
    SetCount( t.GetCount() );
}

void Troop::Set( const Monster & m, uint32_t c )
{
    Set( Troop( m, c ) );
}

void Troop::SetMonster( const Monster & m )
{
    id = m.GetID();
}

void Troop::SetCount( uint32_t c )
{
    count = c;
}

void Troop::Reset()
{
    id = Monster::UNKNOWN;
    count = 0;
}

const char * Troop::GetName() const
{
    return Monster::GetPluralName( count );
}

uint32_t Troop::GetCount() const
{
    return count;
}

uint32_t Troop::GetHitPoints() const
{
    return Monster::GetHitPoints() * count;
}

uint32_t Troop::GetDamageMin() const
{
    return Monster::GetDamageMin() * count;
}

uint32_t Troop::GetDamageMax() const
{
    return Monster::GetDamageMax() * count;
}

double Troop::GetStrength() const
{
    return Monster::GetMonsterStrength() * count;
}

double Troop::GetStrengthWithBonus( int bonusAttack, int bonusDefense ) const
{
    return Monster::GetMonsterStrength( Monster::GetAttack() + bonusAttack, Monster::GetDefense() + bonusDefense ) * count;
}

bool Troop::isValid() const
{
    return Monster::isValid() && count;
}

bool Troop::isEmpty() const
{
    return !isValid();
}

payment_t Troop::GetTotalCost() const
{
    return GetCost() * count;
}

payment_t Troop::GetTotalUpgradeCost() const
{
    return GetUpgradeCost() * count;
}

bool Troop::isBattle() const
{
    return false;
}

bool Troop::isModes( uint32_t /* unused */ ) const
{
    return false;
}

std::string Troop::GetAttackString() const
{
    return std::to_string( GetAttack() );
}

std::string Troop::GetDefenseString() const
{
    return std::to_string( GetDefense() );
}

std::string Troop::GetShotString() const
{
    return std::to_string( GetShots() );
}

std::string Troop::GetSpeedString() const
{
    return GetSpeedString( GetSpeed() );
}

std::string Troop::GetSpeedString( uint32_t speed )
{
    std::string output( Speed::String( static_cast<int>( speed ) ) );
    output += " (";
    output += std::to_string( speed );
    output += ')';

    return output;
}

uint32_t Troop::GetHitPointsLeft() const
{
    return 0;
}

uint32_t Troop::GetSpeed() const
{
    return Monster::GetSpeed();
}

uint32_t Troop::GetAffectedDuration( uint32_t /* unused */ ) const
{
    return 0;
}

ArmyTroop::ArmyTroop( const Army * a )
    : army( a )
{}

ArmyTroop::ArmyTroop( const Army * a, const Troop & t )
    : Troop( t )
    , army( a )
{}

uint32_t ArmyTroop::GetAttack() const
{
    return Troop::GetAttack() + ( army && army->GetCommander() ? army->GetCommander()->GetAttack() : 0 );
}

uint32_t ArmyTroop::GetDefense() const
{
    return Troop::GetDefense() + ( army && army->GetCommander() ? army->GetCommander()->GetDefense() : 0 );
}

int ArmyTroop::GetColor() const
{
    return army ? army->GetColor() : Color::NONE;
}

int ArmyTroop::GetMorale() const
{
    return army && isAffectedByMorale() ? army->GetMorale() : Troop::GetMorale();
}

int ArmyTroop::GetLuck() const
{
    return army ? army->GetLuck() : Troop::GetLuck();
}

void ArmyTroop::SetArmy( const Army & a )
{
    army = &a;
}

const Army * ArmyTroop::GetArmy() const
{
    return army;
}

std::string ArmyTroop::GetAttackString() const
{
    if ( Troop::GetAttack() == GetAttack() )
        return std::to_string( Troop::GetAttack() );

    std::string output( std::to_string( Troop::GetAttack() ) );
    output += " (";
    output += std::to_string( GetAttack() );
    output += ')';

    return output;
}

std::string ArmyTroop::GetDefenseString() const
{
    if ( Troop::GetDefense() == GetDefense() )
        return std::to_string( Troop::GetDefense() );

    std::string output( std::to_string( Troop::GetDefense() ) );
    output += " (";
    output += std::to_string( GetDefense() );
    output += ')';

    return output;
}

StreamBase & operator<<( StreamBase & msg, const Troop & troop )
{
    return msg << troop.id << troop.count;
}

StreamBase & operator>>( StreamBase & msg, Troop & troop )
{
    return msg >> troop.id >> troop.count;
}
