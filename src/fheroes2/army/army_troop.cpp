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

#include "army_troop.h"

#include <cassert>

#include "army.h"
#include "color.h"
#include "heroes_base.h"
#include "resource.h"
#include "serialize.h"
#include "speed.h"

Troop::Troop()
    : Monster( Monster::UNKNOWN )
    , _count( 0 )
{}

Troop::Troop( const Monster & mons, const uint32_t count )
    : Monster( mons )
    , _count( count )
{}

bool Troop::operator==( const Monster & mons ) const
{
    return Monster::operator==( mons );
}

bool Troop::isMonster( const int mons ) const
{
    return GetID() == mons;
}

Monster Troop::GetMonster() const
{
    return Monster( id );
}

void Troop::Set( const Troop & troop )
{
    SetMonster( troop.GetMonster() );
    SetCount( troop.GetCount() );
}

void Troop::Set( const Monster & mons, const uint32_t count )
{
    Set( Troop( mons, count ) );
}

void Troop::SetMonster( const Monster & mons )
{
    id = mons.GetID();
}

void Troop::SetCount( const uint32_t count )
{
    _count = count;
}

void Troop::Reset()
{
    id = Monster::UNKNOWN;
    _count = 0;
}

const char * Troop::GetName() const
{
    return Monster::GetPluralName( _count );
}

uint32_t Troop::GetCount() const
{
    return _count;
}

uint32_t Troop::GetHitPoints() const
{
    return Monster::GetHitPoints() * _count;
}

uint32_t Troop::GetDamageMin() const
{
    return Monster::GetDamageMin() * _count;
}

uint32_t Troop::GetDamageMax() const
{
    return Monster::GetDamageMax() * _count;
}

double Troop::GetStrength() const
{
    return Monster::GetMonsterStrength() * _count;
}

double Troop::GetStrengthWithBonus( const int bonusAttack, const int bonusDefense ) const
{
    assert( bonusAttack >= 0 && bonusDefense >= 0 );

    return Monster::GetMonsterStrength( Monster::GetAttack() + bonusAttack, Monster::GetDefense() + bonusDefense ) * _count;
}

bool Troop::isValid() const
{
    return Monster::isValid() && _count;
}

bool Troop::isEmpty() const
{
    return !isValid();
}

Funds Troop::GetTotalCost() const
{
    return GetCost() * _count;
}

Funds Troop::GetTotalUpgradeCost() const
{
    return GetUpgradeCost() * _count;
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

std::string Troop::GetSpeedString( const uint32_t speed )
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

ArmyTroop::ArmyTroop( const Army * army )
    : _army( army )
{}

ArmyTroop::ArmyTroop( const Army * army, const Troop & troop )
    : Troop( troop )
    , _army( army )
{}

uint32_t ArmyTroop::GetAttack() const
{
    return Troop::GetAttack() + ( _army && _army->GetCommander() ? _army->GetCommander()->GetAttack() : 0 );
}

uint32_t ArmyTroop::GetDefense() const
{
    return Troop::GetDefense() + ( _army && _army->GetCommander() ? _army->GetCommander()->GetDefense() : 0 );
}

int ArmyTroop::GetColor() const
{
    return _army ? _army->GetColor() : Color::NONE;
}

int ArmyTroop::GetMorale() const
{
    return _army && isAffectedByMorale() ? _army->GetMorale() : Troop::GetMorale();
}

int ArmyTroop::GetLuck() const
{
    return _army ? _army->GetLuck() : Troop::GetLuck();
}

void ArmyTroop::SetArmy( const Army & army )
{
    _army = &army;
}

const Army * ArmyTroop::GetArmy() const
{
    return _army;
}

std::string ArmyTroop::GetAttackString() const
{
    if ( Troop::GetAttack() == GetAttack() ) {
        return std::to_string( Troop::GetAttack() );
    }

    std::string output( std::to_string( Troop::GetAttack() ) );
    output += " (";
    output += std::to_string( GetAttack() );
    output += ')';

    return output;
}

std::string ArmyTroop::GetDefenseString() const
{
    if ( Troop::GetDefense() == GetDefense() ) {
        return std::to_string( Troop::GetDefense() );
    }

    std::string output( std::to_string( Troop::GetDefense() ) );
    output += " (";
    output += std::to_string( GetDefense() );
    output += ')';

    return output;
}

OStreamBase & operator<<( OStreamBase & stream, const Troop & troop )
{
    return stream << troop.id << troop._count;
}

IStreamBase & operator>>( IStreamBase & stream, Troop & troop )
{
    return stream >> troop.id >> troop._count;
}
