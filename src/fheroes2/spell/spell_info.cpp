/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2024                                             *
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

#include <algorithm>
#include <cassert>
#include <vector>

#include "artifact.h"
#include "artifact_info.h"
#include "castle.h"
#include "color.h"
#include "direction.h"
#include "heroes.h"
#include "heroes_base.h"
#include "kingdom.h"
#include "maps.h"
#include "maps_fileinfo.h"
#include "maps_tiles.h"
#include "math_base.h"
#include "monster.h"
#include "mp2.h"
#include "settings.h"
#include "spell.h"
#include "tools.h"
#include "translations.h"
#include "world.h"

namespace
{
    void updateSpellDescription( const Spell & spell, std::string & description )
    {
        const uint32_t spellExtraValue = spell.ExtraValue();
        switch ( spellExtraValue ) {
        case 1:
            StringReplace( description, "%{count}", _( "one" ) );
            break;
        case 2:
            StringReplace( description, "%{count}", _( "two" ) );
            break;
        default:
            StringReplace( description, "%{count}", spellExtraValue );
            break;
        }
    }
}

namespace fheroes2
{
    uint32_t getSpellDamage( const Spell & spell, const uint32_t spellPower, const HeroBase * hero )
    {
        assert( spellPower > 0 );

        uint32_t damage = spell.Damage() * spellPower;

        if ( hero == nullptr ) {
            return damage;
        }

        ArtifactBonusType type = ArtifactBonusType::NONE;

        switch ( spell.GetID() ) {
        case Spell::COLDRAY:
        case Spell::COLDRING:
            type = ArtifactBonusType::COLD_SPELL_EXTRA_EFFECTIVENESS_PERCENT;
            break;

        case Spell::FIREBALL:
        case Spell::FIREBLAST:
            type = ArtifactBonusType::FIRE_SPELL_EXTRA_EFFECTIVENESS_PERCENT;
            break;

        case Spell::LIGHTNINGBOLT:
        case Spell::CHAINLIGHTNING:
            type = ArtifactBonusType::LIGHTNING_SPELL_EXTRA_EFFECTIVENESS_PERCENT;
            break;
        default:
            break;
        }

        if ( type == ArtifactBonusType::NONE ) {
            return damage;
        }

        const std::vector<int32_t> extraDamagePercentage = hero->GetBagArtifacts().getTotalArtifactMultipliedPercent( type );
        for ( const int32_t value : extraDamagePercentage ) {
            damage = damage * ( 100 + value ) / 100;
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

        const std::vector<int32_t> summonSpellExtraEffectPercent
            = hero->GetBagArtifacts().getTotalArtifactMultipliedPercent( ArtifactBonusType::SUMMONING_SPELL_EXTRA_EFFECTIVENESS_PERCENT );

        for ( const int32_t value : summonSpellExtraEffectPercent ) {
            monsterCount = monsterCount * ( 100 + value ) / 100;
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

        const std::vector<int32_t> extraSpellEffectivenessPercent
            = hero->GetBagArtifacts().getTotalArtifactMultipliedPercent( ArtifactBonusType::RESURRECT_SPELL_EXTRA_EFFECTIVENESS_PERCENT );

        for ( const int32_t value : extraSpellEffectivenessPercent ) {
            resurrectionPoints = resurrectionPoints * ( 100 + value ) / 100;
        }

        return resurrectionPoints;
    }

    uint32_t getGuardianMonsterCount( const Spell & spell, const uint32_t spellPower, const HeroBase * hero )
    {
        (void)hero;

        assert( spellPower > 0 );

        return spell.ExtraValue() * spellPower;
    }

    uint32_t getHypnotizeMonsterHPPoints( const Spell & spell, const uint32_t spellPower, const HeroBase * hero )
    {
        assert( spell == Spell::HYPNOTIZE );
        assert( spellPower > 0 );

        uint32_t hpPoints = spell.ExtraValue() * spellPower;

        if ( hero != nullptr ) {
            const std::vector<int32_t> extraEffectiveness
                = hero->GetBagArtifacts().getTotalArtifactMultipliedPercent( fheroes2::ArtifactBonusType::HYPNOTIZE_SPELL_EXTRA_EFFECTIVENESS_PERCENT );
            for ( const int32_t value : extraEffectiveness ) {
                hpPoints = hpPoints * ( 100 + value ) / 100;
            }
        }

        return hpPoints;
    }

    const Castle * getNearestCastleTownGate( const Heroes & hero )
    {
        const Kingdom & kingdom = hero.GetKingdom();
        const VecCastles & castles = kingdom.GetCastles();

        const Point & heroPosition = hero.GetCenter();
        int32_t minDistance = -1;

        const Castle * nearestCastle = nullptr;

        for ( const Castle * castle : castles ) {
            if ( castle == nullptr ) {
                continue;
            }

            const Point & castlePosition = castle->GetCenter();
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
        std::string description = spell.GetDescription();
        updateSpellDescription( spell, description );

        if ( hero == nullptr ) {
            return description;
        }

        if ( spell.isDamage() ) {
            description += "\n\n";
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

            description += "\n\n";
            description += _( "This spell summons\n%{count} %{monster}." );
            StringReplace( description, "%{count}", summonCount );
            StringReplace( description, "%{monster}", monster.GetPluralName( summonCount ) );

            return description;
        }

        if ( spell.isRestore() ) {
            description += "\n\n";
            description += _( "This spell restores %{hp} HP." );
            StringReplace( description, "%{hp}", getHPRestorePoints( spell, hero->GetPower(), hero ) );

            return description;
        }

        if ( spell.isResurrect() ) {
            description += "\n\n";
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

            description += "\n\n";
            description += _( "This spell summons %{count} %{monster} to guard the mine." );
            StringReplace( description, "%{count}", guardianCount );
            StringReplace( description, "%{monster}", monster.GetPluralName( guardianCount ) );

            return description;
        }

        if ( spell == Spell::TOWNGATE ) {
            const Heroes * realHero = dynamic_cast<const Heroes *>( hero );
            if ( realHero == nullptr ) {
                return description;
            }

            const Castle * castle = getNearestCastleTownGate( *realHero );
            if ( castle == nullptr ) {
                return description;
            }

            description += "\n\n";

            description += _( "The nearest town is %{town}." );
            StringReplace( description, "%{town}", castle->GetName() );

            const Heroes * townHero = castle->GetHero();
            if ( townHero != nullptr ) {
                description += "\n\n";
                std::string extraLine = _( "This town is occupied by your hero %{hero}." );
                StringReplace( extraLine, "%{town}", castle->GetName() );
                StringReplace( extraLine, "%{hero}", townHero->GetName() );

                description += extraLine;
            }

            return description;
        }

        if ( spell == Spell::HYPNOTIZE ) {
            description += "\n\n";
            description += _( "This spell controls up to\n%{hp} HP." );
            StringReplace( description, "%{hp}", getHypnotizeMonsterHPPoints( spell, hero->GetPower(), hero ) );

            return description;
        }

        return description;
    }

    int32_t getPossibleBoatPosition( const Heroes & hero )
    {
        if ( !hero.MayCastAdventureSpells() ) {
            return -1;
        }

        const int32_t heroTileIdx = hero.GetIndex();
        const int heroTilePassability = world.getTile( heroTileIdx ).GetPassable();

        std::vector<int32_t> possibleBoatPositions;
        possibleBoatPositions.reserve( 8 );

        for ( const int32_t tileIdx : Maps::getAroundIndexes( heroTileIdx ) ) {
            const int direction = Maps::GetDirection( heroTileIdx, tileIdx );
            assert( direction != Direction::UNKNOWN && direction != Direction::CENTER );

            if ( ( heroTilePassability & direction ) == 0 ) {
                continue;
            }

            const Maps::Tile & tile = world.getTile( tileIdx );
            if ( !tile.isWater() || tile.getMainObjectType() != MP2::OBJ_NONE ) {
                continue;
            }

            possibleBoatPositions.emplace_back( tileIdx );
        }

        const auto iter = std::min_element( possibleBoatPositions.begin(), possibleBoatPositions.end(),
                                            [heroPoint = Maps::GetPoint( heroTileIdx )]( const int32_t left, const int32_t right ) {
                                                const fheroes2::Point leftPoint = Maps::GetPoint( left );
                                                const fheroes2::Point rightPoint = Maps::GetPoint( right );

                                                const int32_t leftDiffX = leftPoint.x - heroPoint.x;
                                                const int32_t leftDiffY = leftPoint.y - heroPoint.y;
                                                const int32_t rightDiffX = rightPoint.x - heroPoint.x;
                                                const int32_t rightDiffY = rightPoint.y - heroPoint.y;

                                                return ( leftDiffX * leftDiffX + leftDiffY * leftDiffY ) < ( rightDiffX * rightDiffX + rightDiffY * rightDiffY );
                                            } );
        if ( iter == possibleBoatPositions.end() ) {
            return -1;
        }

        return *iter;
    }

    int32_t getSummonableBoat( const Heroes & hero )
    {
        const int32_t center = hero.GetIndex();
        const int heroColor = hero.GetColor();

        const bool isResurrectionMap = ( Settings::Get().getCurrentMapInfo().version == GameVersion::RESURRECTION );

        for ( const int32_t boatSource : Maps::GetObjectPositions( center, MP2::OBJ_BOAT, false ) ) {
            assert( Maps::isValidAbsIndex( boatSource ) );

            // In the original game, AI could not use the Summon Boat spell at all, and many of the original maps (including the maps of the original campaign) were
            // created with this in mind. In fheroes2, however, the AI is able to use this spell. To mitigate the impact of this on the gameplay of the original maps,
            // AI is prohibited from summoning "neutral" boats (i.e. boats placed on the map by the map creator and not yet used by anyone) on these maps.
            if ( [&hero, heroColor, isResurrectionMap, boatSource]() {
                     const int boatColor = world.getTile( boatSource ).getBoatOwnerColor();

                     // Boats belonging to the hero's kingdom can always be summoned
                     if ( boatColor == heroColor ) {
                         return false;
                     }

                     // Non-neutral boats (belonging to any other kingdom) can never be summoned
                     if ( boatColor != Color::NONE ) {
                         return true;
                     }

                     // On Resurrection maps, neutral boats can be summoned by both human and AI players
                     if ( isResurrectionMap ) {
                         return false;
                     }

                     // On original HoMM2 maps, neutral boats can only be summoned by human players
                     return hero.isControlAI();
                 }() ) {
                continue;
            }

            const uint32_t distance = Maps::GetStraightLineDistance( boatSource, center );
            if ( distance > 1 ) {
                return boatSource;
            }
        }

        return -1;
    }

    bool isHeroNearWater( const Heroes & hero )
    {
        const MapsIndexes tilesAround = Maps::getAroundIndexes( hero.GetIndex() );
        return std::any_of( tilesAround.begin(), tilesAround.end(), []( const int32_t tileId ) { return world.getTile( tileId ).isWater(); } );
    }
}
