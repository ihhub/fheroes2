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
#ifndef H2SPELL_H
#define H2SPELL_H

#include <cstdint>
#include <vector>

class IStreamBase;
class OStreamBase;

class HeroBase;

class Spell
{
public:
    enum : int32_t
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

    // Returns the weight of this spell for a specific race.
    // See https://handbookhmm.ru/kakim-obrazom-zaklinaniya-popadayut-v-magicheskuyu-gildiyu.html for details.
    uint32_t weightForRace( const int race ) const;

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

    // Returns the index of the spell sprite in SPELLS.ICN
    uint32_t IndexSprite() const;

    static Spell Rand( const int level, const bool isAdventure );
    static Spell RandCombat( const int level );
    static Spell RandAdventure( const int level );

    // Returns the IDs of all spells of a given level that are suitable for the spell book (i.e. no placeholders or exclusive
    // built-in spells for monsters are returned). If 'spellLevel' is less than 1, suitable spells of all levels are returned.
    static std::vector<int> getAllSpellIdsSuitableForSpellBook( const int spellLevel = -1 );

    static int32_t CalculateDimensionDoorDistance();

private:
    friend OStreamBase & operator<<( OStreamBase & stream, const Spell & spell );
    friend IStreamBase & operator>>( IStreamBase & stream, Spell & spell );

    int id;
};

#endif
