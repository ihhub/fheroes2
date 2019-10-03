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
#ifndef H2SPELL_H
#define H2SPELL_H

#include "payment.h"
#include "gamedefs.h"

class HeroBase;
class StreamBase;
class StreamBase;

class Spell
{
public:
    enum type_t
    {
	NONE		= 0,
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

	STONE
    };

    Spell(int = NONE);

    bool operator< (const Spell &) const;
    bool operator== (const Spell &) const;
    bool operator!= (const Spell &) const;

    int operator() () const;
    int GetID() const;

    const char* GetName() const;
    const char* GetDescription() const;

    u32 SpellPoint(const HeroBase* hero = NULL) const;
    u32 MovePoint() const;
    int Level() const;
    u32 Damage() const;
    u32 Restore() const;
    u32 Resurrect() const;

    u32 ExtraValue() const;
    payment_t GetCost() const;
    
    bool isValid() const;
    bool isLevel(int) const;
    bool isCombat() const;
    bool isAdventure() const;
    bool isDamage() const;
    bool isRestore() const;
    bool isResurrect() const;
    bool isMindInfluence() const;
    bool isUndeadOnly() const;
    bool isALiveOnly() const;
    bool isSummon() const;
    bool isApplyWithoutFocusObject() const;
    bool isApplyToAnyTroops() const;
    bool isApplyToFriends() const;
    bool isApplyToEnemies() const;
    bool isMassActions() const;
    bool isRaceCompatible(int race) const;

    /* return index sprite spells.icn */
    u32 IndexSprite() const;
    /* return index in spellinl.icn */
    u32 InlIndexSprite() const;

    static Spell RandCombat(int lvl);
    static Spell RandAdventure(int lvl);
    static Spell Rand(int lvl, bool adv);

    static void UpdateStats(const std::string &);
    static u32 CalculateDimensionDoorDistance(u32 current_sp, u32 total_hp);

private:
    friend StreamBase & operator<< (StreamBase &, const Spell &);
    friend StreamBase & operator>> (StreamBase &, Spell &);

    int id;
};

StreamBase & operator<< (StreamBase &, const Spell &);
StreamBase & operator>> (StreamBase &, Spell &);

#endif

