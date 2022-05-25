/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
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

#ifndef H2ARMYTROOP_H
#define H2ARMYTROOP_H

#include <string>

#include "monster.h"

class Army;

class Troop : public Monster
{
public:
    Troop();
    Troop( const Monster &, uint32_t );

    bool operator==( const Monster & ) const;

    void Set( const Troop & );
    void Set( const Monster &, uint32_t );
    void SetMonster( const Monster & );
    void SetCount( uint32_t );
    void Reset( void );

    bool isMonster( int ) const;
    const char * GetName( void ) const;
    virtual uint32_t GetCount( void ) const;
    uint32_t GetHitPoints( void ) const;
    Monster GetMonster( void ) const;

    uint32_t GetDamageMin( void ) const;
    uint32_t GetDamageMax( void ) const;

    payment_t GetTotalCost() const;
    payment_t GetTotalUpgradeCost() const;

    virtual bool isValid( void ) const;
    virtual bool isEmpty( void ) const;
    virtual bool isBattle( void ) const;
    virtual bool isModes( uint32_t ) const;
    virtual std::string GetAttackString( void ) const;
    virtual std::string GetDefenseString( void ) const;
    virtual std::string GetShotString( void ) const;
    virtual std::string GetSpeedString( void ) const;
    virtual uint32_t GetHitPointsLeft( void ) const;
    virtual uint32_t GetSpeed( void ) const;
    virtual uint32_t GetAffectedDuration( uint32_t ) const;
    virtual double GetStrength() const;
    virtual double GetStrengthWithBonus( int bonusAttack, int bonusDefense ) const;

protected:
    friend StreamBase & operator<<( StreamBase &, const Troop & );
    friend StreamBase & operator>>( StreamBase &, Troop & );

    uint32_t count;
};

StreamBase & operator<<( StreamBase &, const Troop & );
StreamBase & operator>>( StreamBase &, Troop & );

class ArmyTroop : public Troop
{
public:
    explicit ArmyTroop( const Army * );
    ArmyTroop( const Army *, const Troop & );

    ArmyTroop & operator=( const Troop & ) = delete;

    uint32_t GetAttack( void ) const override;
    uint32_t GetDefense( void ) const override;
    int GetColor( void ) const override;
    int GetMorale( void ) const override;
    int GetLuck( void ) const override;

    void SetArmy( const Army & );
    const Army * GetArmy( void ) const;

    std::string GetAttackString( void ) const override;
    std::string GetDefenseString( void ) const override;

protected:
    const Army * army;
};

#endif
