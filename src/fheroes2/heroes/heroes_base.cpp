/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2019 - 2022                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <algorithm>
#include <array>
#include <cassert>

#include "army.h"
#include "castle.h"
#include "heroes_base.h"
#include "kingdom.h"
#include "race.h"
#include "serialize.h"
#include "settings.h"
#include "spell_info.h"
#include "tools.h"
#include "translations.h"
#include "world.h"

HeroBase::HeroBase( const int type, const int race )
    : magic_point( 0 )
    , move_point( 0 )
{
    bag_artifacts.assign( HEROESMAXARTIFACT, Artifact::UNKNOWN );
    LoadDefaults( type, race );
}

void HeroBase::LoadDefaults( const int type, const int race )
{
    if ( Race::ALL & race ) {
        // fixed default primary skills
        Skill::Primary::LoadDefaults( type, race );

        // fixed default spell
        switch ( type ) {
        case HeroBase::CAPTAIN: {
            // force add spell book
            SpellBookActivate();

            const Spell spell = Skill::Primary::GetInitialSpell( race );
            if ( spell.isValid() )
                spell_book.Append( spell );
            break;
        }

        case HeroBase::HEROES: {
            const Spell spell = Skill::Primary::GetInitialSpell( race );
            if ( spell.isValid() ) {
                SpellBookActivate();
                spell_book.Append( spell );
            }
            break;
        }

        default:
            break;
        }
    }
}

HeroBase::HeroBase()
    : magic_point( 0 )
    , move_point( 0 )
{}

bool HeroBase::isCaptain() const
{
    return GetType() == CAPTAIN;
}

bool HeroBase::isHeroes() const
{
    return GetType() == HEROES;
}

uint32_t HeroBase::GetSpellPoints() const
{
    return magic_point;
}

void HeroBase::SetSpellPoints( const uint32_t points )
{
    magic_point = points;
}

bool HeroBase::HaveSpellPoints( const Spell & spell ) const
{
    return magic_point >= spell.SpellPoint( this );
}

void HeroBase::EditSpellBook()
{
    spell_book.Edit( *this );
}

Spell HeroBase::OpenSpellBook( const SpellBook::Filter filter, const bool canCastSpell, const std::function<void( const std::string & )> * statusCallback ) const
{
    return spell_book.Open( *this, filter, canCastSpell, statusCallback );
}

bool HeroBase::HaveSpellBook() const
{
    return hasArtifact( Artifact::MAGIC_BOOK );
}

std::vector<Spell> HeroBase::GetSpells( const int lvl ) const
{
    return spell_book.GetSpells( lvl );
}

bool HeroBase::HaveSpell( const Spell & spell, const bool skip_bag ) const
{
    return HaveSpellBook() && ( spell_book.isPresentSpell( spell ) || ( !skip_bag && bag_artifacts.ContainSpell( spell ) ) );
}

void HeroBase::AppendSpellToBook( const Spell & spell, const bool without_wisdom )
{
    if ( without_wisdom || CanLearnSpell( spell ) )
        spell_book.Append( spell );
}

void HeroBase::AppendSpellsToBook( const SpellStorage & spells, const bool without_wisdom )
{
    for ( const auto & spell : spells )
        AppendSpellToBook( spell, without_wisdom );
}

bool HeroBase::SpellBookActivate()
{
    return !HaveSpellBook() && bag_artifacts.PushArtifact( Artifact::MAGIC_BOOK );
}

const BagArtifacts & HeroBase::GetBagArtifacts() const
{
    return bag_artifacts;
}

BagArtifacts & HeroBase::GetBagArtifacts()
{
    return bag_artifacts;
}

bool HeroBase::hasArtifact( const Artifact & art ) const
{
    return bag_artifacts.isPresentArtifact( art );
}

int HeroBase::GetAttackModificator( std::string * strs ) const
{
    int result = 0;
    if ( strs == nullptr ) {
        result = GetBagArtifacts().getTotalArtifactEffectValue( fheroes2::ArtifactBonusType::ATTACK_SKILL );
    }
    else {
        result = GetBagArtifacts().getTotalArtifactEffectValue( fheroes2::ArtifactBonusType::ATTACK_SKILL, *strs );
    }

    // check castle modificator
    const Castle * castle = inCastle();

    if ( castle )
        result += castle->GetAttackModificator( strs );

    return result;
}

int HeroBase::GetDefenseModificator( std::string * strs ) const
{
    int result = 0;
    if ( strs == nullptr ) {
        result = GetBagArtifacts().getTotalArtifactEffectValue( fheroes2::ArtifactBonusType::DEFENCE_SKILL );
    }
    else {
        result = GetBagArtifacts().getTotalArtifactEffectValue( fheroes2::ArtifactBonusType::DEFENCE_SKILL, *strs );
    }

    // check castle modificator
    const Castle * castle = inCastle();

    if ( castle )
        result += castle->GetDefenseModificator( strs );

    return result;
}

int HeroBase::GetPowerModificator( std::string * strs ) const
{
    int result = 0;
    if ( strs == nullptr ) {
        result = GetBagArtifacts().getTotalArtifactEffectValue( fheroes2::ArtifactBonusType::SPELL_POWER_SKILL );
        result -= GetBagArtifacts().getTotalArtifactEffectValue( fheroes2::ArtifactCurseType::SPELL_POWER_SKILL );
    }
    else {
        result = GetBagArtifacts().getTotalArtifactEffectValue( fheroes2::ArtifactBonusType::SPELL_POWER_SKILL, *strs );
        result -= GetBagArtifacts().getTotalArtifactEffectValue( fheroes2::ArtifactCurseType::SPELL_POWER_SKILL, *strs );
    }

    // check castle modificator
    const Castle * castle = inCastle();

    if ( castle )
        result += castle->GetPowerModificator( strs );

    return result;
}

int HeroBase::GetKnowledgeModificator( std::string * strs ) const
{
    int result = 0;
    if ( strs == nullptr ) {
        result = GetBagArtifacts().getTotalArtifactEffectValue( fheroes2::ArtifactBonusType::KNOWLEDGE_SKILL );
    }
    else {
        result = GetBagArtifacts().getTotalArtifactEffectValue( fheroes2::ArtifactBonusType::KNOWLEDGE_SKILL, *strs );
    }

    // check castle modificator
    const Castle * castle = inCastle();

    if ( castle )
        result += castle->GetKnowledgeModificator( strs );

    return result;
}

int HeroBase::GetMoraleModificator( std::string * strs ) const
{
    int result = 0;
    if ( strs == nullptr ) {
        result = GetBagArtifacts().getTotalArtifactEffectValue( fheroes2::ArtifactBonusType::MORALE );
        if ( Modes( Heroes::SHIPMASTER ) ) {
            result += GetBagArtifacts().getTotalArtifactEffectValue( fheroes2::ArtifactBonusType::SEA_BATTLE_MORALE_BOOST );
        }

        result -= GetBagArtifacts().getTotalArtifactEffectValue( fheroes2::ArtifactCurseType::MORALE );
    }
    else {
        result = GetBagArtifacts().getTotalArtifactEffectValue( fheroes2::ArtifactBonusType::MORALE, *strs );
        if ( Modes( Heroes::SHIPMASTER ) ) {
            result += GetBagArtifacts().getTotalArtifactEffectValue( fheroes2::ArtifactBonusType::SEA_BATTLE_MORALE_BOOST, *strs );
        }

        result -= GetBagArtifacts().getTotalArtifactEffectValue( fheroes2::ArtifactCurseType::MORALE, *strs );
    }

    // check castle modificator
    const Castle * castle = inCastle();

    if ( castle )
        result += castle->GetMoraleModificator( strs );

    // army modificator
    result += GetArmy().GetMoraleModificator( strs );

    return result;
}

int HeroBase::GetLuckModificator( std::string * strs ) const
{
    int result = 0;
    if ( strs == nullptr ) {
        result = GetBagArtifacts().getTotalArtifactEffectValue( fheroes2::ArtifactBonusType::LUCK );
        if ( Modes( Heroes::SHIPMASTER ) ) {
            result += GetBagArtifacts().getTotalArtifactEffectValue( fheroes2::ArtifactBonusType::SEA_BATTLE_LUCK_BOOST );
        }
    }
    else {
        result = GetBagArtifacts().getTotalArtifactEffectValue( fheroes2::ArtifactBonusType::LUCK, *strs );
        if ( Modes( Heroes::SHIPMASTER ) ) {
            result += GetBagArtifacts().getTotalArtifactEffectValue( fheroes2::ArtifactBonusType::SEA_BATTLE_LUCK_BOOST, *strs );
        }
    }

    // check castle modificator
    const Castle * castle = inCastle();

    if ( castle )
        result += castle->GetLuckModificator( strs );

    // army modificator
    result += GetArmy().GetLuckModificator( strs );

    return result;
}

double HeroBase::GetMagicStrategicValue( const double armyStrength ) const
{
    const std::vector<Spell> & spells = GetSpells();
    const uint32_t currentSpellPoints = GetSpellPoints();
    const int spellPower = GetPower();

    double bestValue = 0;
    for ( const Spell & spell : spells ) {
        if ( spell.isCombat() ) {
            const int id = spell.GetID();

            const uint32_t spellCost = spell.SpellPoint();
            const uint32_t casts = spellCost ? std::min( 10U, currentSpellPoints / spellCost ) : 0;

            // use quadratic formula to diminish returns from subsequent spell casts, (up to x5 when spell has 10 uses)
            const double amountModifier = ( casts == 1 ) ? 1 : casts - ( 0.05 * casts * casts );

            if ( spell.isDamage() ) {
                // Benchmark for Lightning for 20 power * 20 knowledge (maximum uses) is 2500.0
                bestValue = std::max( bestValue, amountModifier * spell.Damage() * spellPower );
            }
            // These high impact spells can turn tide of battle
            else if ( spell.isResurrect() || spell.isMassActions() || id == Spell::BLIND || id == Spell::PARALYZE ) {
                bestValue = std::max( bestValue, armyStrength * 0.1 * amountModifier );
            }
            else if ( spell.isSummon() ) {
                bestValue = std::max( bestValue, Monster( spell ).GetMonsterStrength() * spell.ExtraValue() * spellPower * amountModifier );
            }
            else {
                bestValue = std::max( bestValue, armyStrength * 0.04 * amountModifier );
            }
        }
    }

    return bestValue;
}

bool HeroBase::CanCastSpell( const Spell & spell, std::string * res ) const
{
    if ( !HaveSpellBook() ) {
        if ( res ) {
            *res = _( "Spell book is not present." );
        }
        return false;
    }

    if ( !HaveSpell( spell ) ) {
        if ( res ) {
            *res = _( "The spell is not found." );
        }
        return false;
    }

    if ( !HaveSpellPoints( spell ) ) {
        if ( res ) {
            *res = _( "That spell costs %{mana} mana. You only have %{point} mana, so you can't cast the spell." );
        }
        return false;
    }

    if ( move_point < spell.MovePoint() ) {
        if ( res ) {
            *res = _( "Not enough move points." );
        }
        return false;
    }

    const Heroes * hero = dynamic_cast<const Heroes *>( this );
    if ( spell.isAdventure() && hero == nullptr ) {
        // How is it possible that a captain can access this spell?
        assert( 0 );
        if ( res != nullptr ) {
            *res = _( "Only heroes can cast this spell." );
        }
        return false;
    }

    if ( spell == Spell::TOWNGATE ) {
        const Castle * castle = fheroes2::getNearestCastleTownGate( *hero );
        if ( castle == nullptr ) {
            if ( res != nullptr ) {
                *res = _( "You do not currently own any town or castle, so you can't cast the spell." );
            }
            return false;
        }

        if ( castle->GetIndex() == hero->GetIndex() ) {
            if ( res != nullptr ) {
                *res = _( "This hero is already in a town, so you can't cast the spell." );
            }
            return false;
        }

        const Heroes * townGuest = castle->GetHeroes().Guest();
        if ( townGuest != nullptr ) {
            if ( res != nullptr ) {
                *res = _( "The nearest town is %{town}.\n \nThis town is occupied by your hero %{hero}." );
                StringReplace( *res, "%{town}", castle->GetName() );
                StringReplace( *res, "%{hero}", townGuest->GetName() );
            }
            return false;
        }
    }

    if ( res ) {
        res->clear();
    }
    return true;
}

void HeroBase::SpellCasted( const Spell & spell )
{
    // spell point cost
    magic_point -= ( spell.SpellPoint( this ) < magic_point ? spell.SpellPoint( this ) : magic_point );

    // move point cost
    if ( spell.MovePoint() )
        move_point -= ( spell.MovePoint() < move_point ? spell.MovePoint() : move_point );
}

bool HeroBase::CanTranscribeScroll( const Artifact & art ) const
{
    const Spell spell = art.GetSpell();

    if ( spell.isValid() && CanCastSpell( spell ) ) {
        const int learning = GetLevelSkill( Skill::Secondary::EAGLEEYE );

        return ( ( 3 < spell.Level() && Skill::Level::EXPERT == learning ) || ( 3 == spell.Level() && Skill::Level::ADVANCED <= learning )
                 || ( 3 > spell.Level() && Skill::Level::BASIC <= learning ) );
    }

    return false;
}

bool HeroBase::CanLearnSpell( const Spell & spell ) const
{
    const int wisdom = GetLevelSkill( Skill::Secondary::WISDOM );

    return ( ( 4 < spell.Level() && Skill::Level::EXPERT == wisdom ) || ( 4 == spell.Level() && Skill::Level::ADVANCED <= wisdom )
             || ( 3 == spell.Level() && Skill::Level::BASIC <= wisdom ) || 3 > spell.Level() );
}

void HeroBase::TranscribeScroll( const Artifact & art )
{
    const Spell spell = art.GetSpell();

    if ( spell.isValid() ) {
        // add spell
        spell_book.Append( spell );

        // remove art
        bag_artifacts.RemoveScroll( art );

        // reduce mp and resource
        SpellCasted( spell );
    }
}

/* pack hero base */
StreamBase & operator<<( StreamBase & msg, const HeroBase & hero )
{
    return msg << static_cast<const Skill::Primary &>( hero ) << static_cast<const MapPosition &>( hero ) <<
           // modes
           hero.modes <<
           // hero base
           hero.magic_point << hero.move_point << hero.spell_book << hero.bag_artifacts;
}

/* unpack hero base */
StreamBase & operator>>( StreamBase & msg, HeroBase & hero )
{
    msg >> static_cast<Skill::Primary &>( hero ) >> static_cast<MapPosition &>( hero ) >>
        // modes
        hero.modes >> hero.magic_point >> hero.move_point >> hero.spell_book >> hero.bag_artifacts;

    return msg;
}
