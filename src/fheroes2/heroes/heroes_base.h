/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2025                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Josh Matthews  <josh@joshmatthews.net>          *
 *   Copyright (C) 2008 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "artifact.h"
#include "bitmodes.h"
#include "players.h"
#include "position.h"
#include "skill.h"
#include "spell.h"
#include "spell_book.h"
#include "spell_storage.h"

class IStreamBase;
class OStreamBase;

enum class PlayerColor : uint8_t;

namespace fheroes2
{
    class Image;
}

class Army;
class Castle;

enum
{
    MDF_NONE,
    MDF_ATTACK,
    MDF_DEFENSE,
    MDF_POWER,
    MDF_KNOWLEDGE,
    MDF_MORALE,
    MDF_LUCK
};

enum PortraitType
{
    PORT_BIG = 1,
    PORT_MEDIUM = 2,
    PORT_SMALL = 3
};

class HeroBase : public Skill::Primary, public MapPosition, public BitModes, public Control
{
public:
    HeroBase( const int type, const int race );
    HeroBase() = default;

    ~HeroBase() override = default;

    enum
    {
        UNDEFINED,
        CAPTAIN,
        HEROES
    };

    virtual const std::string & GetName() const = 0;
    virtual PlayerColor GetColor() const = 0;
    int GetControl() const override = 0;
    virtual bool isValid() const = 0;

    virtual const Army & GetArmy() const = 0;
    virtual Army & GetArmy() = 0;

    virtual uint32_t GetMaxSpellPoints() const = 0;

    virtual int GetLevelSkill( int skill ) const = 0;
    virtual uint32_t GetSecondarySkillValue( int skill ) const = 0;

    virtual void ActionAfterBattle() = 0;
    virtual void ActionPreBattle() = 0;

    virtual const Castle * inCastle() const = 0;
    virtual void PortraitRedraw( const int32_t px, const int32_t py, const PortraitType type, fheroes2::Image & dstsf ) const = 0;

    virtual int GetType() const = 0;

    virtual bool isCaptain() const;
    virtual bool isHeroes() const;

    int GetAttackModificator( std::string * = nullptr ) const;
    int GetDefenseModificator( std::string * = nullptr ) const;
    int GetPowerModificator( std::string * = nullptr ) const;
    int GetKnowledgeModificator( std::string * = nullptr ) const;
    int GetMoraleModificator( std::string * = nullptr ) const;
    int GetLuckModificator( std::string * = nullptr ) const;
    double GetMagicStrategicValue( const double armyStrength ) const;

    uint32_t GetSpellPoints() const
    {
        return _spellPoints;
    }

    // Returns the relative height of mana column near hero's portrait in heroes panel. Returned value will be in range [0; 25].
    uint32_t getManaIndexSprite() const;

    bool HaveSpellPoints( const Spell & spell ) const;
    bool haveMovePoints( const Spell & spell ) const;
    bool CanCastSpell( const Spell & spell, std::string * res = nullptr ) const;
    bool CanLearnSpell( const Spell & spell ) const;
    void SpellCasted( const Spell & spell );
    void SetSpellPoints( const uint32_t points )
    {
        _spellPoints = points;
    }

    bool isPotentSpellcaster() const;

    // Returns all spells that the hero can cast (including spells from the spell book and spell scrolls)
    SpellStorage getAllSpells() const;

    const SpellStorage & getMagicBookSpells() const
    {
        return _spellBook;
    }

    void EditSpellBook();
    Spell OpenSpellBook( const SpellBook::Filter filter, const bool canCastSpell, const bool restorePreviousState,
                         const std::function<void( const std::string & )> & statusCallback ) const;

    bool HaveSpellBook() const
    {
        return hasArtifact( Artifact::MAGIC_BOOK );
    }

    bool HaveSpell( const Spell & spell, const bool skipBag = false ) const;
    void AppendSpellToBook( const Spell &, const bool withoutWisdom = false );
    void AppendSpellsToBook( const SpellStorage &, const bool withoutWisdom = false );

    // Adds the spell book to the artifact bag if it is not already there. Returns true if the spell book was actually added to the artifact bag, otherwise returns false.
    bool SpellBookActivate();
    // Removes the spell book artifact from the artifact bag, if it is there, and removes all spells from the hero's spell book.
    void SpellBookDeactivate();

    BagArtifacts & GetBagArtifacts()
    {
        return _bagArtifacts;
    }

    const BagArtifacts & GetBagArtifacts() const
    {
        return _bagArtifacts;
    }

    bool hasArtifact( const Artifact & art ) const;

    void LoadDefaults( const int type, const int race );

protected:
    friend OStreamBase & operator<<( OStreamBase & stream, const HeroBase & hero );
    friend IStreamBase & operator>>( IStreamBase & stream, HeroBase & hero );

    uint32_t _spellPoints{ 0 };
    uint32_t _movePoints{ 0 };

    SpellBook _spellBook;
    BagArtifacts _bagArtifacts;
};

OStreamBase & operator<<( OStreamBase & stream, const HeroBase & hero );
IStreamBase & operator>>( IStreamBase & stream, HeroBase & hero );
