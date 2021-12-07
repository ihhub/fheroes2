/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2021                                                    *
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

#include "spell_info.h"
#include "heroes.h"
#include "spell.h"
#include "tools.h"
#include "translations.h"

#include <cassert>

namespace fheroes2
{
    uint32_t getSpellDamage( const Spell & spell, const HeroBase * hero )
    {
        if ( hero == nullptr ) {
            return spell.Damage();
        }

        uint32_t damage = spell.Damage() * hero->GetPower();

        switch ( spell.GetID() ) {
        case Spell::COLDRAY:
        case Spell::COLDRING:
            // +50%
            if ( hero->hasArtifact( Artifact::EVERCOLD_ICICLE ) )
                damage += damage * Artifact( Artifact::EVERCOLD_ICICLE ).ExtraValue() / 100;
            break;

        case Spell::FIREBALL:
        case Spell::FIREBLAST:
            // +50%
            if ( hero->hasArtifact( Artifact::EVERHOT_LAVA_ROCK ) )
                damage += damage * Artifact( Artifact::EVERHOT_LAVA_ROCK ).ExtraValue() / 100;
            break;

        case Spell::LIGHTNINGBOLT:
            // +50%
            if ( hero->hasArtifact( Artifact::LIGHTNING_ROD ) )
                damage += damage * Artifact( Artifact::LIGHTNING_ROD ).ExtraValue() / 100;
            break;

        case Spell::CHAINLIGHTNING:
            // +50%
            if ( hero->hasArtifact( Artifact::LIGHTNING_ROD ) )
                damage += damage * Artifact( Artifact::LIGHTNING_ROD ).ExtraValue() / 100;
            break;
        default:
            break;
        }

        return damage;
    }

    uint32_t getSummonMonsterCount( const Spell & spell, const HeroBase * hero )
    {
        if ( hero == nullptr ) {
            return spell.ExtraValue();
        }

        uint32_t monsterCount = spell.ExtraValue() * hero->GetPower();
        uint32_t artifactCount = hero->artifactCount( Artifact::BOOK_ELEMENTS );
        if ( artifactCount > 0 ) {
            monsterCount *= artifactCount * 2;
        }

        return monsterCount;
    }

    uint32_t getHPRestorePoints( const Spell & spell, const HeroBase * hero )
    {
        if ( hero == nullptr ) {
            return spell.Restore();
        }

        return spell.Restore() * hero->GetPower();
    }

    uint32_t getResurrectPoints( const Spell & spell, const HeroBase * hero )
    {
        if ( hero == nullptr ) {
            return spell.Resurrect();
        }

        uint32_t resurrectionPoints = spell.Resurrect() * hero->GetPower();;

        uint32_t artifactCount = hero ? hero->artifactCount( Artifact::ANKH ) : 0;
        if ( artifactCount ) {
            resurrectionPoints *= artifactCount * 2;
        }

        return resurrectionPoints;
    }

    std::string getSpellDescription( const Spell & spell, const HeroBase * hero )
    {
        if ( hero == nullptr ) {
            return spell.GetDescription();
        }

        std::string description = spell.GetDescription();

        if ( spell.isDamage() ) {
            description += "\n \n";
            description += _( "This spell does %{damage} points of damage." );
            StringReplace( description, "%{damage}", getSpellDamage( spell, hero ) );

            return description;
        }

        if ( spell.isSummon() ) {
            const Monster monster( spell );

            if ( !monster.isValid() ) {
                // Did you add a new summoning spell but forgot to add corresponding monster?
                assert( 0 );
                return spell.GetDescription();
            }

            const uint32_t summonCount = getSummonMonsterCount( spell, hero );

            description += "\n \n";
            description += _( "This spell summons\n%{count} %{monster}." );
            StringReplace( description, "%{count}", summonCount );
            StringReplace( description, "%{monster}", monster.GetPluralName( summonCount ) );

            return description;
        }

        if ( spell.isRestore() ) {
            description += "\n \n";
            description += _( "This spell restores %{hp} HP." );
            StringReplace( description, "%{hp}", getHPRestorePoints( spell, hero ) );

            return description;
        }

        if ( spell.isResurrect() ) {
            description += "\n \n";
            description += _( "This spell restores %{hp} HP." );
            StringReplace( description, "%{hp}", getResurrectPoints( spell, hero ) );

            return description;
        }

        return description;
    }
}
