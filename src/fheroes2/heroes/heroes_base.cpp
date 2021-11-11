/***************************************************************************
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <algorithm>
#include <array>

#include "army.h"
#include "castle.h"
#include "heroes_base.h"
#include "kingdom.h"
#include "race.h"
#include "serialize.h"
#include "settings.h"
#include "translations.h"
#include "world.h"

template <std::size_t size>
int ArtifactsModifiersResult( int type, const std::array<uint8_t, size> & arts, const HeroBase & base, std::string * strs )
{
    int result = 0;

    for ( const Artifact art : arts ) {
        if ( art.isValid() ) {
            const uint32_t acount = base.artifactCount( art );
            if ( acount ) {
                int32_t mod = art.ExtraValue();

                switch ( art.GetID() ) {
                case Artifact::SWORD_BREAKER:
                    if ( type == MDF_ATTACK )
                        mod = 1;
                    break;
                // power
                case Artifact::BROACH_SHIELDING:
                    if ( type == MDF_POWER )
                        mod = -2;
                    break;
                // morale/luck
                case Artifact::BATTLE_GARB:
                    if ( type == MDF_MORALE || type == MDF_LUCK )
                        mod = 10;
                    break;
                case Artifact::MASTHEAD:
                    if ( type == MDF_MORALE || type == MDF_LUCK )
                        mod = base.Modes( Heroes::SHIPMASTER ) ? art.ExtraValue() : 0;
                    break;
                // morale
                case Artifact::FIZBIN_MISFORTUNE:
                    if ( type == MDF_MORALE )
                        mod = -static_cast<s32>( art.ExtraValue() );
                    break;
                default:
                    break;
                }

                result += mod * acount;

                if ( strs && mod ) {
                    strs->append( art.GetName() );
                    StringAppendModifiers( *strs, mod );
                    strs->append( "\n" );
                }
            }
        }
    }

    return result;
}

int ArtifactsModifiersAttack( const HeroBase & base, std::string * strs )
{
    const std::array<uint8_t, 14> arts
        = { Artifact::SPIKED_HELM,   Artifact::THUNDER_MACE,      Artifact::GIANT_FLAIL,     Artifact::SWORD_BREAKER,  Artifact::SPIKED_SHIELD,
            Artifact::POWER_AXE,     Artifact::LEGENDARY_SCEPTER, Artifact::DRAGON_SWORD,    Artifact::ULTIMATE_CROWN, Artifact::BATTLE_GARB,
            Artifact::SWORD_ANDURAN, Artifact::HOLY_HAMMER,       Artifact::ULTIMATE_SHIELD, Artifact::ULTIMATE_SWORD };

    return ArtifactsModifiersResult( MDF_ATTACK, arts, base, strs );
}

int ArtifactsModifiersDefense( const HeroBase & base, std::string * strs )
{
    const std::array<uint8_t, 13> arts
        = { Artifact::SPIKED_HELM,       Artifact::ARMORED_GAUNTLETS,  Artifact::DEFENDER_HELM,  Artifact::SPIKED_SHIELD, Artifact::STEALTH_SHIELD,
            Artifact::LEGENDARY_SCEPTER, Artifact::DIVINE_BREASTPLATE, Artifact::ULTIMATE_CROWN, Artifact::SWORD_BREAKER, Artifact::BREASTPLATE_ANDURAN,
            Artifact::BATTLE_GARB,       Artifact::ULTIMATE_SHIELD,    Artifact::ULTIMATE_CLOAK };

    return ArtifactsModifiersResult( MDF_DEFENSE, arts, base, strs );
}

int ArtifactsModifiersPower( const HeroBase & base, std::string * strs )
{
    const std::array<uint8_t, 15> arts
        = { Artifact::WHITE_PEARL,    Artifact::BLACK_PEARL,    Artifact::CASTER_BRACELET, Artifact::MAGE_RING,       Artifact::LEGENDARY_SCEPTER,
            Artifact::WITCHES_BROACH, Artifact::ARM_MARTYR,     Artifact::ULTIMATE_CROWN,  Artifact::ARCANE_NECKLACE, Artifact::BATTLE_GARB,
            Artifact::STAFF_WIZARDRY, Artifact::HELMET_ANDURAN, Artifact::ULTIMATE_STAFF,  Artifact::ULTIMATE_WAND,   Artifact::BROACH_SHIELDING };

    return ArtifactsModifiersResult( MDF_POWER, arts, base, strs );
}

int ArtifactsModifiersKnowledge( const HeroBase & base, std::string * strs )
{
    const std::array<uint8_t, 10> arts
        = { Artifact::WHITE_PEARL,     Artifact::BLACK_PEARL,       Artifact::MINOR_SCROLL,   Artifact::MAJOR_SCROLL,   Artifact::SUPERIOR_SCROLL,
            Artifact::FOREMOST_SCROLL, Artifact::LEGENDARY_SCEPTER, Artifact::ULTIMATE_CROWN, Artifact::ULTIMATE_STAFF, Artifact::ULTIMATE_BOOK };

    return ArtifactsModifiersResult( MDF_KNOWLEDGE, arts, base, strs );
}

int ArtifactsModifiersMorale( const HeroBase & base, std::string * strs )
{
    const std::array<uint8_t, 7> arts = { Artifact::MEDAL_VALOR, Artifact::MEDAL_COURAGE, Artifact::MEDAL_HONOR,      Artifact::MEDAL_DISTINCTION,
                                          Artifact::BATTLE_GARB, Artifact::MASTHEAD,      Artifact::FIZBIN_MISFORTUNE };

    return ArtifactsModifiersResult( MDF_MORALE, arts, base, strs );
}

int ArtifactsModifiersLuck( const HeroBase & base, std::string * strs )
{
    const std::array<uint8_t, 6> arts
        = { Artifact::RABBIT_FOOT, Artifact::GOLDEN_HORSESHOE, Artifact::GAMBLER_LUCKY_COIN, Artifact::FOUR_LEAF_CLOVER, Artifact::BATTLE_GARB, Artifact::MASTHEAD };

    return ArtifactsModifiersResult( MDF_LUCK, arts, base, strs );
}

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

uint32_t HeroBase::artifactCount( const Artifact & art ) const
{
    bool unique = true;

    switch ( art.Type() ) {
    case 1: // morale/luck arifacts
        unique = true;
        break;
    case 2:
        unique = Settings::Get().ExtWorldUseUniqueArtifactsRS();
        break; /* resource affecting arts. */
    case 3:
        unique = Settings::Get().ExtWorldUseUniqueArtifactsPS();
        break; /* primary/mp/sp arts. */
    case 4:
        unique = Settings::Get().ExtWorldUseUniqueArtifactsSS();
        break; /* sec. skills arts. */
    default:
        break;
    }

    if ( unique ) {
        return bag_artifacts.isPresentArtifact( art ) ? 1 : 0;
    }
    else {
        return bag_artifacts.Count( art );
    }
}

bool HeroBase::hasArtifact( const Artifact & art ) const
{
    return bag_artifacts.isPresentArtifact( art );
}

int HeroBase::GetAttackModificator( std::string * strs ) const
{
    int result = ArtifactsModifiersAttack( *this, strs );

    // check castle modificator
    const Castle * castle = inCastle();

    if ( castle )
        result += castle->GetAttackModificator( strs );

    return result;
}

int HeroBase::GetDefenseModificator( std::string * strs ) const
{
    int result = ArtifactsModifiersDefense( *this, strs );

    // check castle modificator
    const Castle * castle = inCastle();

    if ( castle )
        result += castle->GetDefenseModificator( strs );

    return result;
}

int HeroBase::GetPowerModificator( std::string * strs ) const
{
    int result = ArtifactsModifiersPower( *this, strs );

    // check castle modificator
    const Castle * castle = inCastle();

    if ( castle )
        result += castle->GetPowerModificator( strs );

    return result;
}

int HeroBase::GetKnowledgeModificator( std::string * strs ) const
{
    int result = ArtifactsModifiersKnowledge( *this, strs );

    // check castle modificator
    const Castle * castle = inCastle();

    if ( castle )
        result += castle->GetKnowledgeModificator( strs );

    return result;
}

int HeroBase::GetMoraleModificator( std::string * strs ) const
{
    int result = ArtifactsModifiersMorale( *this, strs );

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
    int result = ArtifactsModifiersLuck( *this, strs );

    // check castle modificator
    const Castle * castle = inCastle();

    if ( castle )
        result += castle->GetLuckModificator( strs );

    // army modificator
    result += GetArmy().GetLuckModificator( strs );

    return result;
}

double HeroBase::GetSpellcastStrength( const double armyLimit ) const
{
    const std::vector<Spell> & spells = GetSpells();
    const uint32_t currentSpellPoints = GetSpellPoints();
    const int spellPower = GetPower();

    double bestValue = 0;
    for ( const Spell & spell : spells ) {
        if ( spell.isCombat() && spell.SpellPoint() <= currentSpellPoints ) {
            const int id = spell.GetID();

            // High impact spells can turn tide of battle, otherwise look for damage spells
            if ( spell.isSummon() ) {
                bestValue = std::max( bestValue, Monster( spell ).GetMonsterStrength() * spell.ExtraValue() * spellPower );
            }
            else if ( spell.isDamage() ) {
                // Benchmark for Lightning for 20 power * 20 knowledge (200 spell points) is 2500.0
                bestValue = std::max( bestValue, spell.Damage() / 2.0 * spellPower * sqrt( currentSpellPoints / 2 ) );
            }
            else if ( spell.isResurrect() || id == Spell::BLIND || id == Spell::PARALYZE ) {
                bestValue = std::max( bestValue, armyLimit * 0.5 );
            }
            else {
                bestValue = std::max( bestValue, armyLimit * 0.2 );
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

bool HeroBase::CanTeachSpell( const Spell & spell ) const
{
    const int learning = GetLevelSkill( Skill::Secondary::EAGLEEYE );

    return ( ( 4 == spell.Level() && Skill::Level::EXPERT == learning ) || ( 3 == spell.Level() && Skill::Level::ADVANCED <= learning )
             || ( 3 > spell.Level() && Skill::Level::BASIC <= learning ) );
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
