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
#include "kingdom.h"
#include "spell.h"
#include "tools.h"
#include "translations.h"

#include <cassert>

namespace fheroes2
{
    uint32_t getSpellDamage( const Spell & spell, const uint32_t spellPower, const HeroBase * hero )
    {
        assert( spellPower > 0 );

        uint32_t damage = spell.Damage() * spellPower;

        if ( hero == nullptr ) {
            return damage;
        }

        switch ( spell.GetID() ) {
        case Spell::COLDRAY:
        case Spell::COLDRING:
            if ( hero->hasArtifact( Artifact::EVERCOLD_ICICLE ) ) {
                damage += damage * Artifact( Artifact::EVERCOLD_ICICLE ).ExtraValue() / 100;
            }
            break;

        case Spell::FIREBALL:
        case Spell::FIREBLAST:
            if ( hero->hasArtifact( Artifact::EVERHOT_LAVA_ROCK ) ) {
                damage += damage * Artifact( Artifact::EVERHOT_LAVA_ROCK ).ExtraValue() / 100;
            }
            break;

        case Spell::LIGHTNINGBOLT:
        case Spell::CHAINLIGHTNING:
            if ( hero->hasArtifact( Artifact::LIGHTNING_ROD ) ) {
                damage += damage * Artifact( Artifact::LIGHTNING_ROD ).ExtraValue() / 100;
            }
            break;
        default:
            break;
        }

        return damage;
    }

    uint32_t getSummonMonsterCount( const Spell & spell, const uint32_t spellPower, const HeroBase * hero )
    {
        assert( spellPower > 0 );

        uint32_t monsterCount = spell.ExtraValue() * spellPower;
        if ( hero == nullptr ) {
            return monsterCount;
        }

        uint32_t artifactCount = hero->artifactCount( Artifact::BOOK_ELEMENTS );
        if ( artifactCount > 0 ) {
            monsterCount *= artifactCount * 2;
        }

        return monsterCount;
    }

    uint32_t getHPRestorePoints( const Spell & spell, const uint32_t spellPower, const HeroBase * hero )
    {
        (void)hero;

        assert( spellPower > 0 );

        return spell.Restore() * spellPower;
    }

    uint32_t getResurrectPoints( const Spell & spell, const uint32_t spellPower, const HeroBase * hero )
    {
        assert( spellPower > 0 );

        uint32_t resurrectionPoints = spell.Resurrect() * spellPower;
        if ( hero == nullptr ) {
            return resurrectionPoints;
        }

        uint32_t artifactCount = hero ? hero->artifactCount( Artifact::ANKH ) : 0;
        if ( artifactCount ) {
            resurrectionPoints *= artifactCount * 2;
        }

        return resurrectionPoints;
    }

    uint32_t getGuardianMonsterCount( const Spell & spell, const uint32_t spellPower, const HeroBase * hero )
    {
        (void)hero;

        assert( spellPower > 0 );

        return spell.ExtraValue() * spellPower;
    }

    const Castle * getNearestCastleTownGate( const Heroes & hero )
    {
        const Kingdom & kingdom = hero.GetKingdom();
        const KingdomCastles & castles = kingdom.GetCastles();

        const fheroes2::Point & heroPosition = hero.GetCenter();
        int32_t minDistance = -1;

        const Castle * nearestCastle = nullptr;

        for ( const Castle * castle : castles ) {
            if ( castle == nullptr ) {
                continue;
            }

            const fheroes2::Point & castlePosition = castle->GetCenter();
            const int32_t offsetX = heroPosition.x - castlePosition.x;
            const int32_t offsetY = heroPosition.y - castlePosition.y;
            const int32_t distance = offsetX * offsetX + offsetY * offsetY;
            if ( minDistance < 0 || distance < minDistance ) {
                minDistance = distance;
                nearestCastle = castle;
            }
        }

        return nearestCastle;
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
            StringReplace( description, "%{damage}", getSpellDamage( spell, hero->GetPower(), hero ) );

            return description;
        }

        if ( spell.isSummon() ) {
            const Monster monster( spell );
            if ( !monster.isValid() ) {
                // Did you add a new summoning spell but forgot to add corresponding monster?
                assert( 0 );
                return spell.GetDescription();
            }

            const uint32_t summonCount = getSummonMonsterCount( spell, hero->GetPower(), hero );

            description += "\n \n";
            description += _( "This spell summons\n%{count} %{monster}." );
            StringReplace( description, "%{count}", summonCount );
            StringReplace( description, "%{monster}", monster.GetPluralName( summonCount ) );

            return description;
        }

        if ( spell.isRestore() ) {
            description += "\n \n";
            description += _( "This spell restores %{hp} HP." );
            StringReplace( description, "%{hp}", getHPRestorePoints( spell, hero->GetPower(), hero ) );

            return description;
        }

        if ( spell.isResurrect() ) {
            description += "\n \n";
            description += _( "This spell restores %{hp} HP." );
            StringReplace( description, "%{hp}", getResurrectPoints( spell, hero->GetPower(), hero ) );

            return description;
        }

        if ( spell.isGuardianType() ) {
            const Monster monster( spell );
            if ( !monster.isValid() ) {
                // Did you add a new guardian spell but forgot to add corresponding monster?
                assert( 0 );
                return spell.GetDescription();
            }

            const uint32_t guardianCount = getGuardianMonsterCount( spell, hero->GetPower(), hero );

            description += "\n \n";
            description += _( "This spell summons %{count} %{monster} to guard the mine." );
            StringReplace( description, "%{count}", guardianCount );
            StringReplace( description, "%{monster}", monster.GetPluralName( guardianCount ) );

            return description;
        }

        if ( spell == Spell::TOWNGATE ) {
            if ( hero == nullptr ) {
                return description;
            }

            const Heroes * realHero = dynamic_cast<const Heroes *>( hero );
            if ( realHero == nullptr ) {
                return description;
            }

            const Castle * castle = getNearestCastleTownGate( *realHero );
            if ( castle == nullptr ) {
                return description;
            }

            description += "\n \n";

            description += _( "The nearest town is %{town}." );
            StringReplace( description, "%{town}", castle->GetName() );

            return description;
        }

        return description;
    }
}
