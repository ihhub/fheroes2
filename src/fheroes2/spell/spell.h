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
#ifndef H2SPELL_H
#define H2SPELL_H

#include <cstdint>

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

        // These constants are placeholders for a random spell of the corresponding level and should not be added to the spell book.
        RANDOM,
        RANDOM1,
        RANDOM2,
        RANDOM3,
        RANDOM4,
        RANDOM5,

        // This spell is exclusively a built-in monster spell and should not be added to the spell book.
        PETRIFY,

        // IMPORTANT! Put all new spells above this line.
        SPELL_COUNT
    };

    Spell( const int32_t spellId = NONE )
        : id( ( spellId < 0 || spellId >= SPELL_COUNT ) ? NONE : spellId )
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

    const char * GetName() const;
    const char * GetDescription() const;

    // Returns the number of spell points consumed/required by this spell
    uint32_t spellPoints( const HeroBase * hero = nullptr ) const;

    // Returns the number of movement points consumed by this spell
    uint32_t movePoints() const;

    // Returns the minimum number of movement points required to cast this spell
    uint32_t minMovePoints() const;

    // Returns the value of the spell using the provided context
    double getStrategicValue( double armyStrength, uint32_t currentSpellPoints, int spellPower ) const;

    int Level() const;
    uint32_t Damage() const;
    uint32_t Restore() const;
    uint32_t Resurrect() const;

    uint32_t ExtraValue() const;

    bool isValid() const
    {
        return id != Spell::NONE;
    }

    bool isLevel( int lvl ) const
    {
        return Level() == lvl;
    }

    bool isCombat() const;

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

    bool isMindInfluence() const;
    bool isUndeadOnly() const;
    bool isAliveOnly() const;
    bool isSummon() const;
    bool isEffectDispel() const;
    bool isApplyWithoutFocusObject() const;
    bool isApplyToAnyTroops() const;
    bool isApplyToFriends() const;
    bool isApplyToEnemies() const;
    bool isMassActions() const;
    bool isRaceCompatible( int race ) const;

    bool isFire() const
    {
        return id == FIREBALL || id == FIREBLAST;
    }

    bool isCold() const
    {
        return id == COLDRAY || id == COLDRING;
    }

    bool isBuiltinOnly() const
    {
        return id == PETRIFY;
    }

    bool isGuardianType() const;

    /* return index sprite spells.icn */
    uint32_t IndexSprite() const;

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
