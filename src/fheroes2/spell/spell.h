/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
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
#ifndef H2SPELL_H
#define H2SPELL_H

#include "payment.h"

#define DEFAULT_SPELL_DURATION 3

class HeroBase;
class StreamBase;

class Spell
{
public:
    enum type_t : int32_t
    {
        NONE = 0,
        FIREBALL,
        FIREBLAST,
        LIGHTNINGBOLT,
        CHAINLIGHTNING,
        TELEPORT,
        CURE,
        MASSCURE,
        RESURRECT,
        RESURRECTTRUE,
        HASTE,
        MASSHASTE,
        SLOW,
        MASSSLOW,
        BLIND,
        BLESS,
        MASSBLESS,
        STONESKIN,
        STEELSKIN,
        CURSE,
        MASSCURSE,
        HOLYWORD,
        HOLYSHOUT,
        ANTIMAGIC,
        DISPEL,
        MASSDISPEL,
        ARROW,
        BERSERKER,
        ARMAGEDDON,
        ELEMENTALSTORM,
        METEORSHOWER,
        PARALYZE,
        HYPNOTIZE,
        COLDRAY,
        COLDRING,
        DISRUPTINGRAY,
        DEATHRIPPLE,
        DEATHWAVE,
        DRAGONSLAYER,
        BLOODLUST,
        ANIMATEDEAD,
        MIRRORIMAGE,
        SHIELD,
        MASSSHIELD,
        SUMMONEELEMENT,
        SUMMONAELEMENT,
        SUMMONFELEMENT,
        SUMMONWELEMENT,
        EARTHQUAKE,
        VIEWMINES,
        VIEWRESOURCES,
        VIEWARTIFACTS,
        VIEWTOWNS,
        VIEWHEROES,
        VIEWALL,
        IDENTIFYHERO,
        SUMMONBOAT,
        DIMENSIONDOOR,
        TOWNGATE,
        TOWNPORTAL,
        VISIONS,
        HAUNT,
        SETEGUARDIAN,
        SETAGUARDIAN,
        SETFGUARDIAN,
        SETWGUARDIAN,

        RANDOM,
        RANDOM1,
        RANDOM2,
        RANDOM3,
        RANDOM4,
        RANDOM5,

        STONE,

        // IMPORTANT! Put all new spells above this line.
        SPELL_COUNT
    };

    Spell( const int s = NONE )
        : id( s > STONE ? NONE : s )
    {
        // Do nothing.
    }

    bool operator<( const Spell & s ) const
    {
        return id < s.id;
    }

    bool operator==( const Spell & s ) const
    {
        return s.id == id;
    }

    bool operator!=( const Spell & s ) const
    {
        return s.id != id;
    }

    int GetID() const
    {
        return id;
    }

    const char * GetName( void ) const;
    const char * GetDescription( void ) const;

    // Returns the number of spell points consumed/required by this spell
    uint32_t spellPoints( const HeroBase * hero = nullptr ) const;
    // Returns the number of movement points consumed by this spell
    uint32_t movePoints() const;
    // Returns the minimum number of movement points required to cast this spell
    uint32_t minMovePoints() const;
    int Level( void ) const;
    u32 Damage( void ) const;
    u32 Restore( void ) const;
    u32 Resurrect( void ) const;

    u32 ExtraValue( void ) const;

    bool isValid() const
    {
        return id != Spell::NONE;
    }

    bool isLevel( int lvl ) const
    {
        return Level() == lvl;
    }

    bool isCombat( void ) const;

    bool isAdventure() const
    {
        return !isCombat();
    }

    bool isDamage() const
    {
        return Damage() != 0;
    }

    bool isSingleTarget() const;

    bool isRestore() const
    {
        return Restore() != 0;
    }

    bool isResurrect() const
    {
        return Resurrect() != 0;
    }

    bool isMindInfluence( void ) const;
    bool isUndeadOnly( void ) const;
    bool isALiveOnly( void ) const;
    bool isSummon( void ) const;
    bool isEffectDispel() const;
    bool isApplyWithoutFocusObject( void ) const;
    bool isApplyToAnyTroops( void ) const;
    bool isApplyToFriends( void ) const;
    bool isApplyToEnemies( void ) const;
    bool isMassActions( void ) const;
    bool isRaceCompatible( int race ) const;

    bool isFire() const
    {
        return id == FIREBALL || id == FIREBLAST;
    }

    bool isCold() const
    {
        return id == COLDRAY || id == COLDRING;
    }

    bool isGuardianType() const;

    /* return index sprite spells.icn */
    u32 IndexSprite( void ) const;

    static Spell RandCombat( int lvl );
    static Spell RandAdventure( int lvl );
    static Spell Rand( int lvl, bool adv );

    static int32_t CalculateDimensionDoorDistance();

private:
    friend StreamBase & operator<<( StreamBase &, const Spell & );
    friend StreamBase & operator>>( StreamBase &, Spell & );

    int id;
};

StreamBase & operator<<( StreamBase &, const Spell & );
StreamBase & operator>>( StreamBase &, Spell & );

#endif
