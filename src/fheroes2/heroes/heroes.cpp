/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2025                                             *
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

#include "heroes.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <utility>

#include "agg_image.h"
#include "ai_planner.h"
#include "army_troop.h"
#include "artifact.h"
#include "artifact_info.h"
#include "audio_manager.h"
#include "battle.h"
#include "castle.h"
#include "dialog.h"
#include "direction.h"
#include "game_io.h"
#include "game_static.h"
#include "ground.h"
#include "icn.h"
#include "image.h"
#include "kingdom.h"
#include "logging.h"
#include "luck.h"
#include "m82.h"
#include "map_format_helper.h"
#include "map_format_info.h"
#include "map_object_info.h"
#include "maps.h"
#include "maps_fileinfo.h"
#include "maps_tiles.h"
#include "monster.h"
#include "morale.h"
#include "mp2.h"
#include "payment.h"
#include "players.h"
#include "race.h"
#include "rand.h"
#include "resource.h"
#include "save_format_version.h"
#include "serialize.h"
#include "settings.h"
#include "speed.h"
#include "spell_book.h"
#include "tools.h"
#include "translations.h"
#include "ui_dialog.h"
#include "ui_font.h"
#include "world.h"

namespace
{
    const std::array<const char *, Heroes::HEROES_COUNT> defaultHeroNames = {
        // Unknown / uninitialized hero.
        "Unknown",

        // Knight heroes from The Succession Wars.
        gettext_noop( "Lord Kilburn" ), gettext_noop( "Sir Gallant" ), gettext_noop( "Ector" ), gettext_noop( "Gwenneth" ), gettext_noop( "Tyro" ),
        gettext_noop( "Ambrose" ), gettext_noop( "Ruby" ), gettext_noop( "Maximus" ), gettext_noop( "Dimitry" ),

        // Barbarian heroes from The Succession Wars.
        gettext_noop( "Thundax" ), gettext_noop( "Fineous" ), gettext_noop( "Jojosh" ), gettext_noop( "Crag Hack" ), gettext_noop( "Jezebel" ), gettext_noop( "Jaclyn" ),
        gettext_noop( "Ergon" ), gettext_noop( "Tsabu" ), gettext_noop( "Atlas" ),

        // Sorceress heroes from The Succession Wars.
        gettext_noop( "Astra" ), gettext_noop( "Natasha" ), gettext_noop( "Troyan" ), gettext_noop( "Vatawna" ), gettext_noop( "Rebecca" ), gettext_noop( "Gem" ),
        gettext_noop( "Ariel" ), gettext_noop( "Carlawn" ), gettext_noop( "Luna" ),

        // Warlock heroes from The Succession Wars.
        gettext_noop( "Arie" ), gettext_noop( "Alamar" ), gettext_noop( "Vesper" ), gettext_noop( "Crodo" ), gettext_noop( "Barok" ), gettext_noop( "Kastore" ),
        gettext_noop( "Agar" ), gettext_noop( "Falagar" ), gettext_noop( "Wrathmont" ),

        // Wizard heroes from The Succession Wars.
        gettext_noop( "Myra" ), gettext_noop( "Flint" ), gettext_noop( "Dawn" ), gettext_noop( "Halon" ), gettext_noop( "Myrini" ), gettext_noop( "Wilfrey" ),
        gettext_noop( "Sarakin" ), gettext_noop( "Kalindra" ), gettext_noop( "Mandigal" ),

        // Necromancer heroes from The Succession Wars.
        gettext_noop( "Zom" ), gettext_noop( "Darlana" ), gettext_noop( "Zam" ), gettext_noop( "Ranloo" ), gettext_noop( "Charity" ), gettext_noop( "Rialdo" ),
        gettext_noop( "Roxana" ), gettext_noop( "Sandro" ), gettext_noop( "Celia" ),

        // The Succession Wars campaign heroes.
        gettext_noop( "Roland" ), gettext_noop( "Lord Corlagon" ), gettext_noop( "Sister Eliza" ), gettext_noop( "Archibald" ), gettext_noop( "Lord Halton" ),
        gettext_noop( "Brother Brax" ),

        // The Price of Loyalty expansion heroes.
        gettext_noop( "Solmyr" ), gettext_noop( "Dainwin" ), gettext_noop( "Mog" ), gettext_noop( "Uncle Ivan" ), gettext_noop( "Joseph" ), gettext_noop( "Gallavant" ),
        gettext_noop( "Elderian" ), gettext_noop( "Ceallach" ), gettext_noop( "Drakonia" ), gettext_noop( "Martine" ), gettext_noop( "Jarkonas" ),

        // Debug hero. Should not be used anywhere outside the development!
        "Debug Hero" };

    std::pair<int, int> getHeroIdRangeForRace( const int race )
    {
        switch ( race ) {
        case Race::KNGT:
            return { Heroes::LORDKILBURN, Heroes::DIMITRY };
        case Race::BARB:
            return { Heroes::THUNDAX, Heroes::ATLAS };
        case Race::SORC:
            return { Heroes::ASTRA, Heroes::LUNA };
        case Race::WRLK:
            return { Heroes::ARIE, Heroes::WRATHMONT };
        case Race::WZRD:
            return { Heroes::MYRA, Heroes::MANDIGAL };
        case Race::NECR:
            return { Heroes::ZOM, Heroes::CELIA };
        default:
            break;
        }

        return { Heroes::LORDKILBURN, Heroes::CELIA };
    }

    int getObjectMoraleModifiers( const std::set<MP2::MapObjectType> & objectTypes, std::string * output )
    {
        int result = 0;

        for ( const MP2::MapObjectType objectType : objectTypes ) {
            const int32_t modifier = GameStatic::getObjectMoraleEffect( objectType );
            if ( modifier == 0 ) {
                continue;
            }

            result += modifier;

            if ( output == nullptr ) {
                continue;
            }

            switch ( objectType ) {
            case MP2::OBJ_DERELICT_SHIP:
            case MP2::OBJ_GRAVEYARD:
            case MP2::OBJ_SHIPWRECK: {
                std::string modRobber = _( "shipAndGraveyard|%{object} robber" );
                StringReplace( modRobber, "%{object}", MP2::StringObject( objectType ) );
                output->append( modRobber );
                break;
            }
            default:
                output->append( MP2::StringObject( objectType ) );
                break;
            }

            fheroes2::appendModifierToString( *output, modifier );
            output->append( "\n" );
        }

        return result;
    }

    int getObjectLuckModifiers( const std::set<MP2::MapObjectType> & objectTypes, std::string * output )
    {
        int result = 0;

        for ( const MP2::MapObjectType objectType : objectTypes ) {
            const int32_t modifier = GameStatic::getObjectLuckEffect( objectType );
            if ( modifier == 0 ) {
                continue;
            }

            result += modifier;

            if ( output == nullptr ) {
                continue;
            }

            if ( objectType == MP2::OBJ_PYRAMID ) {
                std::string modRaided = _( "pyramid|%{object} raided" );
                StringReplace( modRaided, "%{object}", MP2::StringObject( objectType ) );
                output->append( modRaided );
            }
            else {
                output->append( MP2::StringObject( objectType ) );
            }

            fheroes2::appendModifierToString( *output, modifier );
            output->append( "\n" );
        }

        return result;
    }

    std::string GetHeroRoleString( const Heroes & hero )
    {
        switch ( hero.getAIRole() ) {
        case Heroes::Role::SCOUT:
            return "Scout";
        case Heroes::Role::COURIER:
            return "Courier";
        case Heroes::Role::HUNTER:
            return "Hunter";
        case Heroes::Role::FIGHTER:
            return "Fighter";
        case Heroes::Role::CHAMPION:
            return "Champion";
        default:
            // Did you add a new AI hero role? Add the appropriate logic for it!
            assert( 0 );
            break;
        }

        return "Unknown";
    }
}

const char * Heroes::getDefaultName( const int heroId )
{
    assert( heroId >= UNKNOWN && heroId < HEROES_COUNT );

    return _( defaultHeroNames[heroId] );
}

Heroes::Heroes( const int heroId, const int race )
    : HeroBase( HeroBase::HEROES, race )
    , ColorBase( PlayerColor::NONE )
    , _experience( _getStartingXp() )
    , _name( getDefaultName( heroId ) )
    , _secondarySkills( race )
    , _id( heroId )
    , _portrait( heroId )
    , _race( race )
{
    _army.Reset( true );

    // Add to debug hero a lot of stuff.
    if ( _id == DEBUG_HERO ) {
        _army.Clean();
        _army.JoinTroop( Monster::BLACK_DRAGON, 2, false );
        _army.JoinTroop( Monster::RED_DRAGON, 3, false );

        _secondarySkills = Skill::SecSkills();
        _secondarySkills.AddSkill( Skill::Secondary( Skill::Secondary::PATHFINDING, Skill::Level::ADVANCED ) );
        _secondarySkills.AddSkill( Skill::Secondary( Skill::Secondary::LOGISTICS, Skill::Level::ADVANCED ) );
        _secondarySkills.AddSkill( Skill::Secondary( Skill::Secondary::SCOUTING, Skill::Level::BASIC ) );
        _secondarySkills.AddSkill( Skill::Secondary( Skill::Secondary::MYSTICISM, Skill::Level::BASIC ) );

        PickupArtifact( Artifact::STEALTH_SHIELD );
        PickupArtifact( Artifact::DRAGON_SWORD );
        PickupArtifact( Artifact::NOMAD_BOOTS_MOBILITY );
        PickupArtifact( Artifact::TRAVELER_BOOTS_MOBILITY );
        PickupArtifact( Artifact::TRUE_COMPASS_MOBILITY );

        _experience = 777;
        _spellPoints = 120;

        // This hero has all the spells in his spell book
        for ( const int spellId : Spell::getAllSpellIdsSuitableForSpellBook() ) {
            AppendSpellToBook( Spell( spellId ), true );
        }
    }

    if ( !_spellPoints ) {
        SetSpellPoints( GetMaxSpellPoints() );
    }
    _movePoints = GetMaxMovePoints();
}

void Heroes::LoadFromMP2( const int32_t mapIndex, const PlayerColor colorType, const int raceType, const bool isInJail, const std::vector<uint8_t> & data )
{
    assert( data.size() == MP2::MP2_HEROES_STRUCTURE_SIZE );

    // Structure containing information about a hero.
    //
    // - uint8_t (1 byte)
    //     Unknown / unused. TODO: find out what this byte is for.
    //
    // - uint8_t (1 byte)
    //     Does the hero have custom army set by map creator?
    //
    // - uint8_t (1 byte)
    //    Custom monster type in army slot 1.
    //
    // - uint8_t (1 byte)
    //    Custom monster type in army slot 2.
    //
    // - uint8_t (1 byte)
    //    Custom monster type in army slot 3.
    //
    // - uint8_t (1 byte)
    //    Custom monster type in army slot 4.
    //
    // - uint8_t (1 byte)
    //    Custom monster type in army slot 5.
    //
    // - uint16_t (2 bytes)
    //    The number of custom monsters in army slot 1.
    //
    // - uint16_t (2 bytes)
    //    The number of custom monsters in army slot 2.
    //
    // - uint16_t (2 bytes)
    //    The number of custom monsters in army slot 3.
    //
    // - uint16_t (2 bytes)
    //    The number of custom monsters in army slot 4.
    //
    // - uint16_t (2 bytes)
    //    The number of custom monsters in army slot 5.
    //
    // - uint8_t (1 byte)
    //     Does the hero have a custom portrait?
    //
    // - uint8_t (1 byte)
    //     Custom portrait ID.
    //
    // - uint8_t (1 byte)
    //     Custom first artifact ID.
    //
    // - uint8_t (1 byte)
    //     Custom second artifact ID.
    //
    // - uint8_t (1 byte)
    //     Custom third artifact ID.
    //
    // - uint8_t (1 byte)
    //     Unknown / unused. TODO: find out what this byte is for.
    //
    // - uint32_t (4 bytes)
    //    Experience.
    //
    // - uint8_t (1 byte)
    //     Does the hero have custom secondary skills?
    //
    // - uint8_t (1 byte)
    //     Custom secondary skill type at slot 1.
    //
    // - uint8_t (1 byte)
    //     Custom secondary skill type at slot 2.
    //
    // - uint8_t (1 byte)
    //     Custom secondary skill type at slot 3.
    //
    // - uint8_t (1 byte)
    //     Custom secondary skill type at slot 4.
    //
    // - uint8_t (1 byte)
    //     Custom secondary skill type at slot 5.
    //
    // - uint8_t (1 byte)
    //     Custom secondary skill type at slot 6.
    //
    // - uint8_t (1 byte)
    //     Custom secondary skill type at slot 7.
    //
    // - uint8_t (1 byte)
    //     Custom secondary skill type at slot 8.
    //
    // - uint8_t (1 byte)
    //     Custom secondary skill level at slot 1.
    //
    // - uint8_t (1 byte)
    //     Custom secondary skill level at slot 2.
    //
    // - uint8_t (1 byte)
    //     Custom secondary skill level at slot 3.
    //
    // - uint8_t (1 byte)
    //     Custom secondary skill level at slot 4.
    //
    // - uint8_t (1 byte)
    //     Custom secondary skill level at slot 5.
    //
    // - uint8_t (1 byte)
    //     Custom secondary skill level at slot 6.
    //
    // - uint8_t (1 byte)
    //     Custom secondary skill level at slot 7.
    //
    // - uint8_t (1 byte)
    //     Custom secondary skill level at slot 8.
    //
    // - uint8_t (1 byte)
    //     Unknown / unused. TODO: find out what this byte is for.
    //
    // - uint8_t (1 byte)
    //     Does the hero have a custom name?
    //
    // - string of 13 bytes
    //    Null terminated string of custom hero name.
    //
    // - uint8_t (1 byte)
    //     Is AI hero on patrol?
    //
    // - uint8_t (1 byte)
    //     Patrol distance of this hero, if this is an AI hero placed on the map, or the race of this hero, if this hero is in Jail.
    //
    // - unused 15 bytes
    //    Always zeros.

    modes = 0;

    if ( isInJail ) {
        SetModes( JAIL );
    }

    SetIndex( mapIndex );
    SetColor( colorType );

    // The hero's race can be changed if the portrait that was specified using the map editor does not match the desired race of this hero, or if there are no free heroes
    // of the desired race
    if ( _race != raceType ) {
        SetModes( CUSTOM );

        _race = raceType;
    }

    //
    // Campaign heroes have a non-standard amount of experience by default, so if they are used on the map, then we have to reset their properties to the default values
    // corresponding to their race. The same must be done if the hero's race has been changed.
    //

    // Clear the initial spell
    SpellBookDeactivate();

    // Reset primary skills and initial spell to defaults
    HeroBase::LoadDefaults( HeroBase::HEROES, _race );

    // Reset secondary skills to defaults
    _secondarySkills = Skill::SecSkills( _race );

    // Reset the army to default
    _army.Reset( true );

    ROStreamBuf dataStream( data );

    // Skip first unused byte.
    dataStream.skip( 1 );

    const bool doesHeroHaveCustomArmy = ( dataStream.get() != 0 );
    if ( doesHeroHaveCustomArmy ) {
        Troop troops[5];

        // Set monster types.
        for ( Troop & troop : troops ) {
            // Monster IDs in the MP2 format start from 0, while in the engine they start from 1 due to presence of UNKNOWN monster type.
            troop.SetMonster( dataStream.get() + 1 );
        }

        // Set monster count.
        for ( Troop & troop : troops ) {
            troop.SetCount( dataStream.getLE16() );
        }

        _army.Assign( troops, std::end( troops ) );

        // On some maps, customized heroes don't have an army, give them a minimal army
        if ( !_army.isValid() ) {
            _army.Reset( false );
        }
    }
    else {
        dataStream.skip( 15 );
    }

    const bool doesHeroHaveCustomPortrait = ( dataStream.get() != 0 );
    if ( doesHeroHaveCustomPortrait ) {
        SetModes( CUSTOM );

        // Portrait sprite index. It should be increased by 1 as in the original game hero IDs start from 0.
        _portrait = dataStream.get() + 1;

        if ( !isValidId( _portrait ) ) {
            DEBUG_LOG( DBG_GAME, DBG_WARN, "Invalid MP2 file format: incorrect custom portrait ID: " << _portrait )
            _portrait = _id;
        }
    }
    else {
        dataStream.skip( 1 );
    }

    const auto addInitialArtifact = [this]( const Artifact & art ) {
        // Perhaps the hero already has a spell book because of his race
        if ( art == Artifact::MAGIC_BOOK && HaveSpellBook() ) {
            return;
        }

        PickupArtifact( art );
    };

    // 3 artifacts. Artifact IDs are by value 1 bigger than in the original game.
    addInitialArtifact( Artifact( dataStream.get() + 1 ) );
    addInitialArtifact( Artifact( dataStream.get() + 1 ) );
    addInitialArtifact( Artifact( dataStream.get() + 1 ) );

    // Skip unused byte.
    dataStream.skip( 1 );

    // Get hero's experience.
    _experience = dataStream.getLE32();

    const bool doesHeroHaveCustomSecondarySkills = ( dataStream.get() != 0 );
    if ( doesHeroHaveCustomSecondarySkills ) {
        SetModes( CUSTOM );

        std::array<Skill::Secondary, 8> secs;

        for ( Skill::Secondary & skill : secs ) {
            // Secondary Skill IDs in the MP2 format start from 0, while in the engine they start from 1 due to presence of UNKNOWN skill type.
            skill.SetSkill( dataStream.get() + 1 );
        }

        for ( Skill::Secondary & skill : secs ) {
            skill.SetLevel( dataStream.get() );
        }

        _secondarySkills = Skill::SecSkills();

        for ( const Skill::Secondary & skill : secs ) {
            if ( skill.isValid() ) {
                // The original map editor does not check presence of similar skills even those which have different levels.
                // We need to check whether the existing skill has a lower level before updating it.
                const auto * existingSkill = _secondarySkills.FindSkill( skill.Skill() );
                if ( existingSkill == nullptr || ( existingSkill->Level() < skill.Level() ) ) {
                    _secondarySkills.AddSkill( skill );
                }
            }
        }
    }
    else {
        dataStream.skip( 16 );
    }

    // Skip unused byte.
    dataStream.skip( 1 );

    const bool doesHeroHaveCustomName = ( dataStream.get() != 0 );
    if ( doesHeroHaveCustomName ) {
        // An empty name can be set in the original Editor which is wrong.
        std::string temp = dataStream.getString( 13 );
        if ( !temp.empty() ) {
            SetModes( CUSTOM );
            _name = std::move( temp );
        }
    }
    else {
        dataStream.skip( 13 );
    }

    const bool doesAIHeroSetOnPatrol = ( dataStream.get() != 0 );
    if ( doesAIHeroSetOnPatrol ) {
        SetModes( PATROL );

        _patrolCenter = GetCenter();
        _patrolDistance = dataStream.get();
    }
    else {
        dataStream.skip( 1 );
    }

    // TODO: remove this temporary assertion
    assert( _objectTypeUnderHero == MP2::OBJ_NONE );

    // Level up if needed
    const int level = GetLevel();
    if ( level > 1 ) {
        SetModes( CUSTOM );

        for ( int i = 1; i < level; ++i ) {
            _levelUp( doesHeroHaveCustomSecondarySkills, true );
        }
    }

    SetSpellPoints( GetMaxSpellPoints() );
    _movePoints = GetMaxMovePoints();

    DEBUG_LOG( DBG_GAME, DBG_INFO, _name << ", color: " << Color::String( GetColor() ) << ", race: " << Race::String( _race ) )
}

void Heroes::applyHeroMetadata( const Maps::Map_Format::HeroMetadata & heroMetadata, const bool isInJail, const bool isEditor )
{
    modes = 0;

    if ( isInJail ) {
        SetModes( JAIL );
    }

    if ( _race != heroMetadata.race ) {
        SetModes( CUSTOM );

        _race = heroMetadata.race;
    }

    if ( !isEditor ) {
        // Reset primary skills and initial spell to defaults.
        HeroBase::LoadDefaults( HeroBase::HEROES, _race );
    }

    if ( !Maps::loadHeroArmy( _army, heroMetadata ) && !isEditor ) {
        // Reset the army to default
        _army.Reset( true );
    }

    // Hero's portrait.
    if ( heroMetadata.customPortrait > 0 ) {
        SetModes( CUSTOM );

        assert( isValidId( heroMetadata.customPortrait ) );

        // Portrait sprite index.
        _portrait = heroMetadata.customPortrait;
    }

    const bool doesHeroHaveCustomArtifacts
        = isEditor || std::any_of( heroMetadata.artifact.begin(), heroMetadata.artifact.end(), []( const int32_t artifact ) { return artifact != 0; } );
    if ( doesHeroHaveCustomArtifacts ) {
        // Clear the initial spells and a possible spellBook.
        SpellBookDeactivate();

        const size_t artifactCount = heroMetadata.artifact.size();
        assert( artifactCount == 14 );
        for ( size_t i = 0; i < artifactCount; ++i ) {
            Artifact art( heroMetadata.artifact[i] );
            if ( !art.isValid() ) {
                continue;
            }

            if ( heroMetadata.artifact[i] == Artifact::SPELL_SCROLL ) {
                assert( heroMetadata.artifactMetadata[i] != Spell::NONE );

                art.SetSpell( heroMetadata.artifactMetadata[i] );
            }

            if ( heroMetadata.artifact[i] == Artifact::MAGIC_BOOK ) {
                SpellBookActivate();

                // Add spells to the spell book.
                for ( const int32_t spellId : heroMetadata.availableSpells ) {
                    if ( spellId == Spell::NONE ) {
                        continue;
                    }

                    AppendSpellToBook( spellId, true );
                }
            }
            else {
                PickupArtifact( art );
            }
        }
    }

    const bool doesHeroHaveCustomSecondarySkills
        = std::any_of( heroMetadata.secondarySkill.begin(), heroMetadata.secondarySkill.end(), []( const int32_t skill ) { return skill != 0; } );
    if ( doesHeroHaveCustomSecondarySkills ) {
        SetModes( CUSTOM );

        _secondarySkills = {};

        for ( size_t i = 0; i < heroMetadata.secondarySkill.size(); ++i ) {
            _secondarySkills.AddSkill( Skill::Secondary{ heroMetadata.secondarySkill[i], heroMetadata.secondarySkillLevel[i] } );
        }
    }
    else if ( !isEditor ) {
        // Reset secondary skills to defaults
        _secondarySkills = Skill::SecSkills( _race );
    }

    // For Editor we need to fill all the rest of the 8 skills with the empty ones.
    if ( isEditor ) {
        GetSecondarySkills().FillMax( Skill::Secondary() );
    }

    if ( isEditor || !heroMetadata.customName.empty() ) {
        SetModes( CUSTOM );
        _name = heroMetadata.customName;
    }

    if ( heroMetadata.isOnPatrol ) {
        SetModes( PATROL );

        _patrolCenter = GetCenter();
        _patrolDistance = heroMetadata.patrolRadius;
    }

    // Hero's experience.
    if ( heroMetadata.customExperience > -1 ) {
        _experience = heroMetadata.customExperience;
    }
    else if ( isEditor ) {
        // There is no way to set "default experience" condition, so we will consider 'UINT32_MAX' as it.
        _experience = UINT32_MAX;
    }

    // Level up if needed.
    if ( !isEditor ) {
        const int16_t level = heroMetadata.customLevel > -1 ? heroMetadata.customLevel : static_cast<int16_t>( GetLevel() );
        if ( level > 1 ) {
            SetModes( CUSTOM );

            for ( int16_t i = 1; i < level; ++i ) {
                _levelUp( doesHeroHaveCustomSecondarySkills, true );
            }
        }
    }

    // Apply custom primary Skill values.
    if ( isEditor || heroMetadata.customAttack > -1 ) {
        attack = heroMetadata.customAttack;
    }
    if ( isEditor || heroMetadata.customDefense > -1 ) {
        defense = heroMetadata.customDefense;
    }
    if ( isEditor || heroMetadata.customSpellPower > -1 ) {
        power = heroMetadata.customSpellPower;
    }
    if ( isEditor || heroMetadata.customKnowledge > -1 ) {
        knowledge = heroMetadata.customKnowledge;
    }

    if ( heroMetadata.magicPoints < 0 ) {
        // Default Spell points.
        if ( isEditor ) {
            // There is no way to set "default spell points" condition, so we will consider 'UINT32_MAX' as it.
            _spellPoints = UINT32_MAX;
        }
        else {
            _spellPoints = GetMaxSpellPoints();
        }
    }
    else {
        _spellPoints = static_cast<uint32_t>( heroMetadata.magicPoints );
    }

    _movePoints = GetMaxMovePoints();
}

Maps::Map_Format::HeroMetadata Heroes::getHeroMetadata() const
{
    Maps::Map_Format::HeroMetadata heroMetadata;

    Maps::saveHeroArmy( _army, heroMetadata );

    // Hero's portrait.
    heroMetadata.customPortrait = _portrait;

    // Hero's artifacts.
    const size_t artifactCount = _bagArtifacts.size();
    assert( artifactCount == heroMetadata.artifactMetadata.size() );
    for ( size_t i = 0; i < artifactCount; ++i ) {
        heroMetadata.artifact[i] = _bagArtifacts[i].GetID();
        // The spell scroll may contain a spell.

        if ( heroMetadata.artifact[i] == Artifact::SPELL_SCROLL ) {
            const int32_t artifactSpellId = _bagArtifacts[i].getSpellId();

            assert( artifactSpellId != Spell::NONE );

            heroMetadata.artifactMetadata[i] = artifactSpellId;
        }
    }

    // Hero's spells.
    const size_t bookSize = _spellBook.size();
    heroMetadata.availableSpells.reserve( bookSize );
    for ( size_t i = 0; i < bookSize; ++i ) {
        heroMetadata.availableSpells.push_back( _spellBook[i].GetID() );
    }

    // Hero's secondary skills.
    const std::vector<Skill::Secondary> & skills = _secondarySkills.ToVector();
    const size_t skillsSize = skills.size();
    assert( heroMetadata.secondarySkill.size() == skillsSize && heroMetadata.secondarySkillLevel.size() == skillsSize );
    for ( size_t i = 0; i < skillsSize; ++i ) {
        heroMetadata.secondarySkill[i] = static_cast<int8_t>( skills[i].Skill() );
        heroMetadata.secondarySkillLevel[i] = static_cast<uint8_t>( skills[i].Level() );
    }

    // Hero's name.
    heroMetadata.customName = _name;

    // Patrol mode.
    heroMetadata.isOnPatrol = Modes( PATROL );
    heroMetadata.patrolRadius = static_cast<uint8_t>( _patrolDistance );

    // Hero's experience.
    heroMetadata.customExperience = ( _experience == UINT32_MAX ) ? -1 : static_cast<int32_t>( _experience );

    // Primary Skill base values.
    heroMetadata.customAttack = static_cast<int16_t>( attack );
    heroMetadata.customDefense = static_cast<int16_t>( defense );
    heroMetadata.customSpellPower = static_cast<int16_t>( power );
    heroMetadata.customKnowledge = static_cast<int16_t>( knowledge );

    // Hero's spell points.
    heroMetadata.magicPoints = ( _spellPoints == UINT32_MAX ) ? static_cast<int16_t>( -1 ) : static_cast<int16_t>( _spellPoints );

    // Hero's race.
    heroMetadata.race = static_cast<uint8_t>( _race );

    return heroMetadata;
}

int Heroes::GetRace() const
{
    return _race;
}

const std::string & Heroes::GetName() const
{
    return _name;
}

PlayerColor Heroes::GetColor() const
{
    return ColorBase::GetColor();
}

int Heroes::GetType() const
{
    return HeroBase::HEROES;
}

const Army & Heroes::GetArmy() const
{
    return _army;
}

Army & Heroes::GetArmy()
{
    return _army;
}

uint32_t Heroes::GetMobilityIndexSprite() const
{
    if ( !CanMove() ) {
        return 0;
    }

    const uint32_t value = ( _movePoints + 50U ) / 100U;

    // Valid sprite index range is (0 - 25).
    return std::min( value, 25U );
}

int Heroes::getStatsValue() const
{
    // experience and artifacts don't matter here, only natural stats
    return getTotalPrimarySkillLevel() + _secondarySkills.GetTotalLevel();
}

double Heroes::getRecruitValue() const
{
    return _army.GetStrength() + ( ( _bagArtifacts.getArtifactValue() * 10.0 + getStatsValue() ) * SKILL_VALUE );
}

double Heroes::getMeetingValue( const Heroes & receivingHero ) const
{
    // TODO: add logic to check artifacts with curses and those which are invaluable for a hero.

    // Magic Book is not transferable.
    const uint32_t artCount = _bagArtifacts.CountArtifacts() - _bagArtifacts.Count( Artifact::MAGIC_BOOK );
    const uint32_t canFit = BagArtifacts::maxCapacity - receivingHero._bagArtifacts.CountArtifacts();

    double artifactValue = _bagArtifacts.getArtifactValue() * 5.0;
    if ( artCount > canFit ) {
        artifactValue = canFit * ( artifactValue / artCount );
    }

    // TODO: leaving only one monster in an army is very risky. Add logic to find out which part of the army would be useful to get.
    return receivingHero._army.getReinforcementValue( _army ) + artifactValue * SKILL_VALUE;
}

int Heroes::GetAttack() const
{
    return GetAttack( nullptr );
}

int Heroes::GetAttack( std::string * text ) const
{
    const int result = attack + GetAttackModificator( text );
    return std::clamp( result, 0, 255 );
}

int Heroes::GetDefense() const
{
    return GetDefense( nullptr );
}

int Heroes::GetDefense( std::string * text ) const
{
    const int result = defense + GetDefenseModificator( text );
    return std::clamp( result, 0, 255 );
}

int Heroes::GetPower() const
{
    return GetPower( nullptr );
}

int Heroes::GetPower( std::string * text ) const
{
    const int result = power + GetPowerModificator( text );
    return std::clamp( result, 1, 255 );
}

int Heroes::GetKnowledge() const
{
    return GetKnowledge( nullptr );
}

int Heroes::GetKnowledge( std::string * text ) const
{
    const int result = knowledge + GetKnowledgeModificator( text );
    return std::clamp( result, 0, 255 );
}

void Heroes::IncreasePrimarySkill( const int skill )
{
    switch ( skill ) {
    case Skill::Primary::ATTACK:
        ++attack;
        break;
    case Skill::Primary::DEFENSE:
        ++defense;
        break;
    case Skill::Primary::POWER:
        ++power;
        break;
    case Skill::Primary::KNOWLEDGE:
        ++knowledge;
        break;
    default:
        break;
    }
}

uint32_t Heroes::GetMaxSpellPoints() const
{
    return 10 * GetKnowledge();
}

uint32_t Heroes::GetMaxMovePoints( const bool onWater ) const
{
    uint32_t result = 0;

    if ( onWater ) {
        // Initial mobility on water does not depend on the composition of the army
        result = 1500;

        // Influence of Navigation skill
        result = _updateMovementPoints( result, Skill::Secondary::NAVIGATION );

        // Artifact bonuses
        result += GetBagArtifacts().getTotalArtifactEffectValue( fheroes2::ArtifactBonusType::SEA_MOBILITY );

        // Bonuses from captured lighthouses
        result += 500 * world.CountCapturedObject( MP2::OBJ_LIGHTHOUSE, GetColor() );
    }
    else {
        // Initial mobility on land depends on the speed of the slowest army unit
        const Troop * troop = _army.GetSlowestTroop();
        if ( troop ) {
            switch ( troop->GetSpeed() ) {
            case Speed::VERYSLOW:
                result = 1000;
                break;
            case Speed::SLOW:
                result = 1100;
                break;
            case Speed::AVERAGE:
                result = 1200;
                break;
            case Speed::FAST:
                result = 1300;
                break;
            case Speed::VERYFAST:
                result = 1400;
                break;
            case Speed::ULTRAFAST:
                result = 1500;
                break;
            default:
                assert( 0 );
                break;
            }
        }

        // Influence of Logistics skill
        result = _updateMovementPoints( result, Skill::Secondary::LOGISTICS );

        // Artifact bonuses
        result += GetBagArtifacts().getTotalArtifactEffectValue( fheroes2::ArtifactBonusType::LAND_MOBILITY );

        // Bonuses from visited objects
        if ( isObjectTypeVisited( MP2::OBJ_STABLES ) ) {
            result += GameStatic::getMovementPointBonus( MP2::OBJ_STABLES );
        }
    }

    return result;
}

int Heroes::GetMorale() const
{
    return getMoraleWithModifiers( nullptr );
}

int Heroes::getMoraleWithModifiers( std::string * text ) const
{
    int result = Morale::NORMAL;

    // bonus leadership
    result += Skill::getLeadershipModifiers( GetLevelSkill( Skill::Secondary::LEADERSHIP ), text );

    result += getObjectMoraleModifiers( getAllVisitedObjectTypes(), text );

    // bonus artifact
    result += GetMoraleModificator( text );

    // A special artifact ability presence must be the last check.
    const Artifact maxMoraleArtifact = _bagArtifacts.getFirstArtifactWithBonus( fheroes2::ArtifactBonusType::MAXIMUM_MORALE );
    if ( maxMoraleArtifact.isValid() ) {
        if ( text != nullptr ) {
            *text += maxMoraleArtifact.GetName();
            *text += _( " gives you maximum morale" );
        }

        return Morale::BLOOD;
    }

    if ( text != nullptr && !text->empty() && text->back() == '\n' ) {
        // Remove the possible empty line at the end of the string.
        text->pop_back();
    }

    return Morale::Normalize( result );
}

int Heroes::GetLuck() const
{
    return getLuckWithModifiers( nullptr );
}

int Heroes::getLuckWithModifiers( std::string * text ) const
{
    int result = Luck::NORMAL;

    // bonus luck
    result += Skill::getLuckModifiers( GetLevelSkill( Skill::Secondary::LUCK ), text );

    // object visited
    result += getObjectLuckModifiers( getAllVisitedObjectTypes(), text );

    // bonus artifact
    result += GetLuckModificator( text );

    const Artifact maxLuckArtifact = _bagArtifacts.getFirstArtifactWithBonus( fheroes2::ArtifactBonusType::MAXIMUM_LUCK );
    if ( maxLuckArtifact.isValid() ) {
        if ( text != nullptr ) {
            *text += maxLuckArtifact.GetName();
            *text += _( " gives you maximum luck" );
        }

        return Luck::IRISH;
    }

    if ( text != nullptr && !text->empty() && text->back() == '\n' ) {
        // Remove the possible empty line at the end of the string.
        text->pop_back();
    }

    return Luck::Normalize( result );
}

bool Heroes::Recruit( const PlayerColor col, const fheroes2::Point & pt )
{
    if ( GetColor() != PlayerColor::NONE ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, "hero has already been hired by some kingdom" )

        return false;
    }

    Kingdom & kingdom = world.GetKingdom( col );

    if ( !kingdom.AllowRecruitHero( false ) ) {
        return false;
    }

    ResetModes( RECRUIT );
    ResetModes( JAIL );

    SetColor( col );

    SetCenter( pt );
    setDirection( Direction::RIGHT );

    if ( !Modes( SAVEMP ) ) {
        _movePoints = GetMaxMovePoints();
    }

    if ( !_army.isValid() ) {
        _army.Reset( false );
    }

    world.getTile( pt.x, pt.y ).setHero( this );

    kingdom.AddHero( this );
    // Update the set of recruits in the kingdom
    kingdom.GetRecruits();

    // After recruiting a hero we reveal map in hero scout area.
    Scout( GetIndex() );
    if ( isControlHuman() ) {
        // And the radar image map for human player.
        ScoutRadar();
    }

    return true;
}

bool Heroes::Recruit( const Castle & castle )
{
    if ( !Recruit( castle.GetColor(), castle.GetCenter() ) ) {
        return false;
    }

    castle.trainHeroInMageGuild( *this );

    SetVisited( GetIndex() );

    return true;
}

void Heroes::ActionNewDay()
{
    _movePoints = GetMaxMovePoints();

    if ( world.CountDay() > 1 ) {
        _replenishSpellPoints();
    }

    _visitedObjects.remove_if( Visit::isDayLife );

    ResetModes( SAVEMP );
}

void Heroes::ActionNewWeek()
{
    _visitedObjects.remove_if( Visit::isWeekLife );
}

void Heroes::ActionAfterBattle()
{
    _visitedObjects.remove_if( Visit::isBattleLife );

    SetModes( ACTION );
}

uint32_t Heroes::getDailyRestoredSpellPoints() const
{
    uint32_t points = GameStatic::GetHeroesRestoreSpellPointsPerDay();

    // Spell points from artifacts.
    points += static_cast<uint32_t>( GetBagArtifacts().getTotalArtifactEffectValue( fheroes2::ArtifactBonusType::SPELL_POINTS_DAILY_GENERATION ) );

    points += GetSecondarySkillValue( Skill::Secondary::MYSTICISM );

    return points;
}

void Heroes::_replenishSpellPoints()
{
    const uint32_t maxp = GetMaxSpellPoints();
    uint32_t curr = GetSpellPoints();

    // Spell points can be doubled in Artesian Spring, leave them as they are
    if ( curr >= maxp ) {
        return;
    }

    const Castle * castle = inCastle();

    if ( castle && castle->GetLevelMageGuild() > 0 ) {
        SetSpellPoints( maxp );
    }
    else {
        curr += getDailyRestoredSpellPoints();

        SetSpellPoints( std::min( curr, maxp ) );
    }
}

void Heroes::calculatePath( int32_t dstIdx )
{
    if ( dstIdx < 0 ) {
        // Recalculating an existing path
        dstIdx = _path.GetDestinationIndex();
    }

    if ( !_path.isValidForMovement() ) {
        _path.Reset();
    }

    if ( dstIdx < 0 ) {
        return;
    }

    _path.setPath( world.getPath( *this, dstIdx ) );

    if ( !_path.isValidForMovement() ) {
        _path.Reset();
    }
}

const Castle * Heroes::inCastle() const
{
    return inCastleMutable();
}

Castle * Heroes::inCastleMutable() const
{
    if ( GetColor() == PlayerColor::NONE ) {
        return nullptr;
    }

    Castle * castle = world.getCastleEntrance( GetCenter() );
    return ( castle && castle->GetHero() == this ) ? castle : nullptr;
}

bool Heroes::isVisited( const Maps::Tile & tile, const Visit::Type type /* = Visit::LOCAL */ ) const
{
    const int32_t index = tile.GetIndex();
    const MP2::MapObjectType objectType = tile.getMainObjectType( false );

    if ( Visit::GLOBAL == type ) {
        return GetKingdom().isVisited( index, objectType );
    }

    return _visitedObjects.end() != std::find( _visitedObjects.begin(), _visitedObjects.end(), IndexObject( index, objectType ) );
}

bool Heroes::isObjectTypeVisited( const MP2::MapObjectType objectType, const Visit::Type type /* = Visit::LOCAL */ ) const
{
    if ( Visit::GLOBAL == type ) {
        return GetKingdom().isVisited( objectType );
    }

    return std::any_of( _visitedObjects.begin(), _visitedObjects.end(), [objectType]( const IndexObject & v ) { return v.isObject( objectType ); } );
}

std::set<MP2::MapObjectType> Heroes::getAllVisitedObjectTypes() const
{
    std::set<MP2::MapObjectType> objectTypes;

    for ( const auto & object : _visitedObjects ) {
        objectTypes.emplace( object.second );
    }

    return objectTypes;
}

void Heroes::SetVisited( const int32_t tileIndex, const Visit::Type type /* = Visit::LOCAL */ )
{
    const Maps::Tile & tile = world.getTile( tileIndex );

    const MP2::MapObjectType objectType = tile.getMainObjectType( false );
    if ( !MP2::isOffGameActionObject( objectType ) ) {
        // Something is wrong as how are going to visit a non-action object?!
        assert( 0 );
        return;
    }

    const uint32_t objectUID = tile.getMainObjectPart()._uid;

    if ( Visit::GLOBAL == type ) {
        GetKingdom().SetVisited( tileIndex, objectType );
    }
    else if ( !isVisited( tile ) ) {
        _visitedObjects.emplace_front( tileIndex, objectType );
    }

    // An object could be bigger than 1 tile so we need to check all its tiles.
    constexpr int32_t searchDist = []() constexpr {
        constexpr int32_t max = std::max( Maps::maxActionObjectDimensions.width, Maps::maxActionObjectDimensions.height );
        static_assert( max > 0 );

        return max - 1;
    }();

    for ( const int32_t index : Maps::getAroundIndexes( tileIndex, searchDist ) ) {
        const Maps::Tile & currentTile = world.getTile( index );
        if ( currentTile.getMainObjectType( false ) != objectType || currentTile.getMainObjectPart()._uid != objectUID ) {
            continue;
        }

        if ( Visit::GLOBAL == type ) {
            GetKingdom().SetVisited( index, objectType );
        }
        else if ( !isVisited( currentTile ) ) {
            _visitedObjects.emplace_front( index, objectType );
        }
    }
}

void Heroes::setVisitedForAllies( const int32_t tileIndex ) const
{
    const Maps::Tile & tile = world.getTile( tileIndex );

    const MP2::MapObjectType objectType = tile.getMainObjectType( false );
    if ( !MP2::isOffGameActionObject( objectType ) ) {
        // Something is wrong as how are going to visit a non-action object?!
        assert( 0 );
        return;
    }

    const uint32_t objectUID = tile.getMainObjectPart()._uid;

    // Set visited to all allies as well.
    const PlayerColorsVector friendColors( Players::GetPlayerFriends( GetColor() ) );
    for ( const PlayerColor friendColor : friendColors ) {
        world.GetKingdom( friendColor ).SetVisited( tileIndex, objectType );
    }

    // An object could be bigger than 1 tile so we need to check all its tiles.
    constexpr int32_t searchDist = []() constexpr {
        constexpr int32_t max = std::max( Maps::maxActionObjectDimensions.width, Maps::maxActionObjectDimensions.height );
        static_assert( max > 0 );

        return max - 1;
    }();

    for ( const int32_t index : Maps::getAroundIndexes( tileIndex, searchDist ) ) {
        const Maps::Tile & currentTile = world.getTile( index );
        if ( currentTile.getMainObjectType( false ) != objectType || currentTile.getMainObjectPart()._uid != objectUID ) {
            continue;
        }

        for ( const PlayerColor friendColor : friendColors ) {
            world.GetKingdom( friendColor ).SetVisited( index, objectType );
        }
    }
}

void Heroes::markHeroMeeting( const int heroId )
{
    if ( isValidId( heroId ) && !hasMetWithHero( heroId ) ) {
        _visitedObjects.emplace_front( heroId, MP2::OBJ_HERO );
    }
}

void Heroes::unmarkHeroMeeting()
{
    const VecHeroes & heroes = GetKingdom().GetHeroes();
    for ( Heroes * hero : heroes ) {
        if ( hero == nullptr || hero == this ) {
            continue;
        }

        hero->_visitedObjects.remove( IndexObject( _id, MP2::OBJ_HERO ) );
        _visitedObjects.remove( IndexObject( hero->_id, MP2::OBJ_HERO ) );
    }
}

bool Heroes::hasMetWithHero( const int heroId ) const
{
    return _visitedObjects.end() != std::find( _visitedObjects.begin(), _visitedObjects.end(), IndexObject( heroId, MP2::OBJ_HERO ) );
}

bool Heroes::isLosingGame() const
{
    return GetKingdom().isLosingGame();
}

bool Heroes::PickupArtifact( const Artifact & art )
{
    if ( !art.isValid() ) {
        return false;
    }

    if ( !_bagArtifacts.PushArtifact( art ) ) {
        if ( isControlHuman() ) {
            if ( art.GetID() == Artifact::MAGIC_BOOK ) {
                if ( HaveSpellBook() ) {
                    fheroes2::showStandardTextMessage( art.GetName(), _( "You cannot have multiple spell books." ), Dialog::OK );
                }
                else {
                    // In theory, there should be no other reason not to pick up the artifact
                    assert( IsFullBagArtifacts() );

                    fheroes2::showStandardTextMessage(
                        art.GetName(),
                        _( "You must purchase a spell book to use the mage guild, but you currently have no room for a spell book. Try giving one of your artifacts to another hero." ),
                        Dialog::OK );
                }
            }
            else {
                // In theory, there should be no other reason not to pick up the artifact
                assert( IsFullBagArtifacts() );

                fheroes2::showStandardTextMessage( art.GetName(), _( "You cannot pick up this artifact, you already have a full load!" ), Dialog::OK );
            }
        }

        return false;
    }

    const auto assembledArtifacts = _bagArtifacts.assembleArtifactSetIfPossible();

    if ( isControlHuman() ) {
        std::for_each( assembledArtifacts.begin(), assembledArtifacts.end(), Dialog::ArtifactSetAssembled );
    }

    // If the hero is in jail and gets an artifact assigned using the map editor, then there is no need to scout the area
    if ( Modes( JAIL ) ) {
        return true;
    }

    const auto scout = [this]( const int32_t artifactID ) {
        const std::vector<fheroes2::ArtifactBonus> & bonuses = fheroes2::getArtifactData( artifactID ).bonuses;
        if ( std::find( bonuses.begin(), bonuses.end(), fheroes2::ArtifactBonus( fheroes2::ArtifactBonusType::AREA_REVEAL_DISTANCE ) ) == bonuses.end() ) {
            return false;
        }

        Scout( GetIndex() );
        if ( isControlHuman() ) {
            ScoutRadar();
        }

        return true;
    };

    // Check the picked up artifact for a bonus to the scouting area.
    if ( scout( art.GetID() ) ) {
        return true;
    }

    // If there were artifacts assembled, check them for a bonus to the scouting area.
    for ( const ArtifactSetData & assembledArtifact : assembledArtifacts ) {
        if ( scout( assembledArtifact._assembledArtifactID ) ) {
            return true;
        }
    }

    return true;
}

void Heroes::IncreaseExperience( const uint32_t amount, const bool autoselect /* = false */ )
{
    const int oldLevel = GetLevelFromExperience( _experience );
    const int newLevel = GetLevelFromExperience( _experience + amount );

    const uint32_t updatedExperience = _experience + amount;

    for ( int level = oldLevel; level < newLevel - 1; ++level ) {
        _experience = GetExperienceFromLevel( level );
        _levelUp( false, autoselect );
    }

    _experience = updatedExperience;

    if ( newLevel > oldLevel ) {
        _levelUp( false, autoselect );
    }
}

int Heroes::GetLevelFromExperience( const uint32_t experience )
{
    for ( int lvl = 1; lvl < 255; ++lvl ) {
        if ( experience < GetExperienceFromLevel( lvl ) ) {
            return lvl;
        }
    }

    return 0;
}

uint32_t Heroes::GetExperienceFromLevel( const int level )
{
    switch ( level ) {
    case 0:
        return 0;
    case 1:
        return 1000;
    case 2:
        return 2000;
    case 3:
        return 3200;
    case 4:
        return 4500;
    case 5:
        return 6000;
    case 6:
        return 7700;
    case 7:
        return 9000;
    case 8:
        return 11000;
    case 9:
        return 13200;
    case 10:
        return 15500;
    case 11:
        return 18500;
    case 12:
        return 22100;
    case 13:
        return 26400;
    case 14:
        return 31600;
    case 15:
        return 37800;
    case 16:
        return 45300;
    case 17:
        return 54200;
    case 18:
        return 65000;
    case 19:
        return 78000;
    case 20:
        return 93600;
    case 21:
        return 112300;
    case 22:
        return 134700;
    case 23:
        return 161600;
    case 24:
        return 193900;
    case 25:
        return 232700;
    case 26:
        return 279300;
    case 27:
        return 335200;
    case 28:
        return 402300;
    case 29:
        return 482800;
    case 30:
        return 579400;
    case 31:
        return 695300;
    case 32:
        return 834400;
    case 33:
        return 1001300;
    case 34:
        return 1201600;
    case 35:
        return 1442000;
    case 36:
        return 1730500;
    case 37:
        return 2076700;
    case 38:
        return 2492100;
    case 39:
        return 2990600;

    default:
        break;
    }

    const uint32_t l1 = GetExperienceFromLevel( level - 1 );
    return ( l1 + static_cast<uint32_t>( round( ( l1 - GetExperienceFromLevel( level - 2 ) ) * 1.2 / 100 ) * 100 ) );
}

uint32_t Heroes::getExperienceMaxValue()
{
    return 2990600;
}

bool Heroes::BuySpellBook( const Castle & castle )
{
    if ( HaveSpellBook() || PlayerColor::NONE == GetColor() || castle.GetLevelMageGuild() < 1 ) {
        return false;
    }

    const Funds payment = PaymentConditions::BuySpellBook();
    Kingdom & kingdom = GetKingdom();

    std::string header = _( "To cast spells, you must first buy a spell book for %{gold} gold." );
    StringReplace( header, "%{gold}", payment.gold );

    if ( !kingdom.AllowPayment( payment ) ) {
        if ( isControlHuman() ) {
            header.append( " " );
            header.append( _( "Unfortunately, you seem to be a little short of cash at the moment." ) );

            const fheroes2::ArtifactDialogElement artifactUI( Artifact::MAGIC_BOOK );
            fheroes2::showStandardTextMessage( GetName(), std::move( header ), Dialog::OK, { &artifactUI } );
        }
        return false;
    }

    if ( isControlHuman() ) {
        header.append( " " );
        header.append( _( "Do you wish to buy one?" ) );

        const fheroes2::ArtifactDialogElement artifactUI( Artifact::MAGIC_BOOK );
        if ( fheroes2::showStandardTextMessage( GetName(), std::move( header ), Dialog::YES | Dialog::NO, { &artifactUI } ) == Dialog::NO ) {
            return false;
        }
    }

    if ( SpellBookActivate() ) {
        kingdom.OddFundsResource( payment );

        castle.trainHeroInMageGuild( *this );

        return true;
    }

    return false;
}

bool Heroes::isMoveEnabled() const
{
    return Modes( ENABLEMOVE ) && _path.isValidForMovement() && _path.hasAllowedSteps();
}

bool Heroes::CanMove() const
{
    const Maps::Tile & tile = world.getTile( GetIndex() );
    return _movePoints >= ( tile.isRoad() ? Maps::Ground::roadPenalty : Maps::Ground::GetPenalty( tile, GetLevelSkill( Skill::Secondary::PATHFINDING ) ) );
}

void Heroes::SetMove( const bool enable )
{
    if ( enable ) {
        if ( Modes( ENABLEMOVE ) ) {
            return;
        }

        ResetModes( SLEEPER );

        if ( isControlAI() ) {
            AI::Planner::Get().HeroesBeginMovement( *this );
        }

        SetModes( ENABLEMOVE );
    }
    else {
        if ( !Modes( ENABLEMOVE ) ) {
            return;
        }

        ResetModes( ENABLEMOVE );

        // Reset the hero sprite
        resetHeroSprite();
    }
}

void Heroes::resetHeroSprite()
{
    switch ( _direction ) {
    case Direction::TOP:
        _spriteIndex = 0;
        break;
    case Direction::BOTTOM:
        _spriteIndex = 36;
        break;
    case Direction::TOP_RIGHT:
    case Direction::TOP_LEFT:
        _spriteIndex = 9;
        break;
    case Direction::BOTTOM_RIGHT:
    case Direction::BOTTOM_LEFT:
        _spriteIndex = 27;
        break;
    case Direction::RIGHT:
    case Direction::LEFT:
        _spriteIndex = 18;
        break;
    default:
        break;
    }
}

void Heroes::SetShipMaster( const bool isShipMaster )
{
    isShipMaster ? SetModes( SHIPMASTER ) : ResetModes( SHIPMASTER );
}

bool Heroes::HasSecondarySkill( const int skill ) const
{
    return Skill::Level::NONE != _secondarySkills.GetLevel( skill );
}

uint32_t Heroes::GetSecondarySkillValue( const int skill ) const
{
    return _secondarySkills.GetValue( skill );
}

bool Heroes::HasMaxSecondarySkill() const
{
    return maxNumOfSecSkills <= _secondarySkills.Count();
}

int Heroes::GetLevelSkill( const int skill ) const
{
    return _secondarySkills.GetLevel( skill );
}

void Heroes::LearnSkill( const Skill::Secondary & skill )
{
    if ( skill.isValid() ) {
        _secondarySkills.AddSkill( skill );
    }
}

void Heroes::Scout( const int tileIndex ) const
{
    // We should not scout for the NONE color player.
    assert( GetColor() != PlayerColor::NONE );

    Maps::ClearFog( tileIndex, GetScoutingDistance(), GetColor() );

#if defined( WITH_DEBUG )
    const Player * player = Players::Get( GetColor() );
    assert( player != nullptr );

    // If player gave control to AI we need to update the radar image after every 'ClearFog()' call as in this mode we don't
    // do any optimizations.
    if ( player->isAIAutoControlMode() ) {
        // We redraw the radar map fully as there is no need to make a code for rendering optimizations for AI debug tracking.
        // As AI don't waste time for thinking between hero moves we don't need to force radar update in other places.
        ScoutRadar();
    }
#endif
}

int32_t Heroes::GetScoutingDistance() const
{
    return GetBagArtifacts().getTotalArtifactEffectValue( fheroes2::ArtifactBonusType::AREA_REVEAL_DISTANCE )
           + GameStatic::getFogDiscoveryDistance( GameStatic::FogDiscoveryType::HEROES ) + static_cast<int32_t>( GetSecondarySkillValue( Skill::Secondary::SCOUTING ) );
}

fheroes2::Rect Heroes::GetScoutRoi() const
{
    const int32_t scoutRange = GetScoutingDistance();
    const fheroes2::Point heroPosition = GetCenter();

    return { heroPosition.x - scoutRange, heroPosition.y - scoutRange, 2 * scoutRange + 1, 2 * scoutRange + 1 };
}

uint32_t Heroes::_updateMovementPoints( const uint32_t movePoints, const int skill ) const
{
    const int level = GetLevelSkill( skill );
    if ( level == Skill::Level::NONE ) {
        return movePoints;
    }

    const uint32_t skillValue = GetSecondarySkillValue( skill );

    if ( skillValue == 33 ) {
        return movePoints * 4 / 3;
    }
    if ( skillValue == 66 ) {
        return movePoints * 5 / 3;
    }

    return movePoints + skillValue * movePoints / 100;
}

uint32_t Heroes::GetVisionsDistance()
{
    return 8;
}

int Heroes::getNumOfTravelDays( const int32_t dstIdx ) const
{
    assert( Maps::isValidAbsIndex( dstIdx ) );

    const uint32_t maxMovePoints = GetMaxMovePoints();
    const std::list<Route::Step> routePath = world.getPath( *this, dstIdx );

    if ( routePath.empty() ) {
        return 0;
    }

    uint32_t movePoints = GetMovePoints();
    int days = 1;

    for ( const Route::Step & step : routePath ) {
        const uint32_t stepPenalty = step.GetPenalty();

        if ( movePoints >= stepPenalty ) {
            // This movement takes place on the same day
            movePoints -= stepPenalty;
        }
        else {
            // This movement takes place at the beginning of a new day: start with max
            // movement points, don't carry leftovers from the previous day
            assert( maxMovePoints >= stepPenalty );

            movePoints = maxMovePoints - stepPenalty;
            ++days;

            // Stop at 8 days
            if ( days >= 8 ) {
                break;
            }
        }
    }

    // Return no more than 8 days
    assert( days <= 8 );

    return days;
}

void Heroes::_levelUp( const bool skipSecondary, const bool autoselect /* = false */ )
{
    const HeroSeedsForLevelUp seeds = _getSeedsForLevelUp();

    // level up primary skill
    const int primarySkill = Skill::Primary::LevelUp( _race, GetLevel(), seeds.seedPrimarySkill );

    DEBUG_LOG( DBG_GAME, DBG_INFO, "for " << GetName() << ", up " << Skill::Primary::String( primarySkill ) )

    if ( !skipSecondary ) {
        _levelUpSecondarySkill( seeds, primarySkill, autoselect );
    }
}

void Heroes::_levelUpSecondarySkill( const HeroSeedsForLevelUp & seeds, const int primary, const bool autoselect /* = false */ )
{
    const auto [sec1, sec2] = _secondarySkills.FindSkillsForLevelUp( _race, seeds.seedSecondarySkill1, seeds.seedSecondarySkill2 );

    if ( sec1.isValid() && sec2.isValid() ) {
        DEBUG_LOG( DBG_GAME, DBG_INFO, GetName() << " select " << Skill::Secondary::String( sec1.Skill() ) << " or " << Skill::Secondary::String( sec2.Skill() ) )
    }
    else if ( sec1.isValid() ) {
        DEBUG_LOG( DBG_GAME, DBG_INFO, GetName() << " select " << Skill::Secondary::String( sec1.Skill() ) )
    }
    else if ( sec2.isValid() ) {
        DEBUG_LOG( DBG_GAME, DBG_INFO, GetName() << " select " << Skill::Secondary::String( sec2.Skill() ) )
    }

    Skill::Secondary selected;

    if ( autoselect ) {
        if ( sec1.isValid() && sec2.isValid() ) {
            selected = Rand::GetWithSeed( 0, 1, seeds.seedSecondarySkillRandomChoose ) ? sec1 : sec2;
        }
        else {
            selected = sec1.isValid() ? sec1 : sec2;
        }
    }
    else if ( isControlAI() ) {
        selected = AI::Planner::pickSecondarySkill( *this, sec1, sec2 );
    }
    else {
        AudioManager::PlaySound( M82::NWHEROLV );
        const int result = Dialog::LevelUpSelectSkill( _name, primary, sec1, sec2, *this );

        if ( Skill::Secondary::UNKNOWN != result ) {
            selected = ( result == sec2.Skill() ) ? sec2 : sec1;
        }
    }

    if ( selected.isValid() ) {
        DEBUG_LOG( DBG_GAME, DBG_INFO, GetName() << ", selected: " << Skill::Secondary::String( selected.Skill() ) )
        Skill::Secondary * secs = _secondarySkills.FindSkill( selected.Skill() );

        if ( secs ) {
            secs->NextLevel();
        }
        else {
            _secondarySkills.AddSkill( Skill::Secondary( selected.Skill(), Skill::Level::BASIC ) );
        }

        // Campaign-only heroes get additional experience immediately upon their creation, even while still neutral.
        // We should not try to scout the area around such heroes.
        if ( selected.Skill() == Skill::Secondary::SCOUTING && GetColor() != PlayerColor::NONE ) {
            Scout( GetIndex() );
            if ( isControlHuman() ) {
                ScoutRadar();
            }
        }
    }
}

bool Heroes::MayStillMove( const bool ignorePath, const bool ignoreSleeper ) const
{
    if ( !isActive() ) {
        return false;
    }

    if ( !ignoreSleeper && Modes( SLEEPER ) ) {
        return false;
    }

    if ( !ignorePath && _path.isValidForMovement() ) {
        return _path.hasAllowedSteps();
    }

    return CanMove();
}

bool Heroes::MayCastAdventureSpells() const
{
    return isValid() && GetColor() != PlayerColor::NONE;
}

bool Heroes::isValid() const
{
    return isValidId( _id );
}

bool Heroes::isActive() const
{
    return isValid() && ( Color::allPlayerColors() & GetColor() ) && !Modes( JAIL );
}

bool Heroes::isAvailableForHire() const
{
    return isValid() && GetColor() == PlayerColor::NONE && !Modes( JAIL );
}

void Heroes::Dismiss( const int reason )
{
    if ( isAvailableForHire() ) {
        return;
    }

    // Reset army to default state for hero's race if hero did not surrender in battle.
    if ( ( reason & Battle::RESULT_SURRENDER ) == 0 ) {
        _army.Reset( true );
    }

    const PlayerColor heroColor = GetColor();
    Kingdom & kingdom = GetKingdom();

    if ( heroColor != PlayerColor::NONE ) {
        kingdom.RemoveHero( this );
    }

    SetColor( PlayerColor::NONE );

    world.getTile( GetIndex() ).setHero( nullptr );
    SetIndex( -1 );

    modes = 0;

    _path.Hide();
    _path.Reset();

    SetMove( false );

    SetModes( ACTION );

    if ( ( Battle::RESULT_RETREAT | Battle::RESULT_SURRENDER ) & reason ) {
        SetModes( SAVEMP );

        if ( heroColor != PlayerColor::NONE ) {
            kingdom.appendSurrenderedHero( *this );
        }
    }
}

int Heroes::GetControl() const
{
    return GetKingdom().GetControl();
}

uint32_t Heroes::_getStartingXp()
{
    return Rand::Get( 40, 90 );
}

void Heroes::ActionPreBattle()
{
    _spellBook.resetState();
}

void Heroes::ActionNewPosition( const bool allowMonsterAttack )
{
    if ( allowMonsterAttack ) {
        // scan for monsters around
        const MapsIndexes targets = Maps::getMonstersProtectingTile( GetIndex(), false );

        if ( !targets.empty() ) {
            SetMove( false );
            ShowPath( false );

            // first fight the monsters on the destination tile (if any)
            const MapsIndexes::const_iterator it = std::find( targets.cbegin(), targets.cend(), GetPath().GetDestinationIndex() );

            if ( it != targets.end() ) {
                Action( *it );
            }
            // otherwise fight the monsters on the first adjacent tile
            else {
                Action( targets.front() );
            }
        }
    }

    if ( isControlAI() ) {
        AI::Planner::Get().HeroesActionNewPosition( *this );
    }

    ResetModes( VISIONS );
}

void Heroes::Move2Dest( const int32_t destinationIndex )
{
    const int32_t currentIndex = GetIndex();

    if ( destinationIndex == currentIndex ) {
        return;
    }

    world.getTile( currentIndex ).setHero( nullptr );
    SetIndex( destinationIndex );
    world.getTile( destinationIndex ).setHero( this );
}

const fheroes2::Sprite & Heroes::GetPortrait( const int heroId, const int portraitType )
{
    if ( isValidId( heroId ) )
        switch ( portraitType ) {
        case PORT_BIG:
            return fheroes2::AGG::GetICN( ICN::getHeroPortraitIcnId( heroId ), 0 );
        case PORT_MEDIUM: {
            // Original ICN::PORTMEDI sprites are badly rendered. Instead of them we're getting high quality ICN:PORT00xx file and resize it to a smaller image.
            // TODO: find a better way to store these images, ideally in agg_image.cpp file.
            static std::map<int, fheroes2::Sprite> mediumSizePortrait;
            auto iter = mediumSizePortrait.find( heroId );
            if ( iter != mediumSizePortrait.end() ) {
                return iter->second;
            }

            const fheroes2::Sprite & original = fheroes2::AGG::GetICN( ICN::getHeroPortraitIcnId( heroId ), 0 );
            fheroes2::Sprite output( 50, 47 );
            fheroes2::Resize( original, output );

            return mediumSizePortrait.try_emplace( heroId, std::move( output ) ).first->second;
        }
        case PORT_SMALL:
            if ( heroId == Heroes::DEBUG_HERO ) {
                return fheroes2::AGG::GetICN( ICN::MINIPORT, BRAX - 1 );
            }

            // Since hero IDs start from 1 we have to deduct 1 from the ID.
            return fheroes2::AGG::GetICN( ICN::MINIPORT, heroId - 1 );
        default:
            break;
        }

    return fheroes2::AGG::GetICN( ICN::UNKNOWN, 0 );
}

void Heroes::PortraitRedraw( const int32_t px, const int32_t py, const PortraitType type, fheroes2::Image & dstsf ) const
{
    const fheroes2::Sprite & port = GetPortrait( _portrait, type );
    fheroes2::Point mp;

    if ( !port.empty() ) {
        if ( PORT_BIG == type ) {
            fheroes2::Copy( port, 0, 0, dstsf, px, py, port.width(), port.height() );
            mp.y = 2;
            mp.x = port.width() - 12;
        }
        else if ( PORT_MEDIUM == type ) {
            fheroes2::Copy( port, 0, 0, dstsf, px, py, port.width(), port.height() );
            mp.x = port.width() - 10;
        }
        else if ( PORT_SMALL == type ) {
            const fheroes2::Sprite & background = fheroes2::AGG::GetICN( ICN::PORTXTRA, 0 );
            const fheroes2::Sprite & mobility = fheroes2::AGG::GetICN( ICN::MOBILITY, GetMobilityIndexSprite() );
            const fheroes2::Sprite & mana = fheroes2::AGG::GetICN( ICN::MANA, getManaIndexSprite() );

            const int barw = 7;

            // Draw background.
            fheroes2::Blit( background, dstsf, px, py );

            // Draw mobility.
            fheroes2::Copy( mobility, 0, 0, dstsf, px, py + mobility.y(), mobility.width(), mobility.height() );

            // Draw hero's portrait.
            fheroes2::Copy( port, 0, 0, dstsf, px + barw + 1, py, port.width(), port.height() );

            // Draw mana.
            fheroes2::Copy( mana, 0, 0, dstsf, px + barw + port.width() + 2, py + mana.y(), mana.width(), mana.height() );

            mp.x = 35;
        }
    }

    if ( isControlAI() ) {
        // AI heroes should not have any UI indicators for their statuses.
        return;
    }

    if ( Modes( Heroes::SLEEPER ) ) {
        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::MISC4, 14 );
        fheroes2::Image sleeperBG( sprite.width() - 4, sprite.height() - 4 );
        sleeperBG.fill( 0 );

        fheroes2::Blit( sleeperBG, dstsf, px + mp.x + 3, py + mp.y - 1 );
        fheroes2::Blit( sprite, dstsf, px + mp.x + 1, py + mp.y - 3 );
    }
}

std::string Heroes::String() const
{
    std::ostringstream os;

    os << "name            : " << _name << " (" << Race::String( _race ) << ")" << std::endl
       << "color           : " << Color::String( GetColor() ) << std::endl
       << "experience      : " << _experience << std::endl
       << "level           : " << GetLevel() << std::endl
       << "magic points    : " << GetSpellPoints() << " / " << GetMaxSpellPoints() << std::endl
       << "morale, luck    : " << GetMorale() << ", " << GetLuck() << std ::endl
       << "position x, y   : " << GetCenter().x << ", " << GetCenter().y << std::endl
       << "move points     : " << _movePoints << " / " << GetMaxMovePoints() << std::endl
       << "direction       : " << Direction::String( _direction ) << std::endl
       << "index sprite    : " << _spriteIndex << std::endl
       << "in castle       : " << ( inCastle() ? "true" : "false" ) << std::endl
       << "save object     : " << MP2::StringObject( world.getTile( GetIndex() ).getMainObjectType( false ) ) << std::endl
       << "flags           : " << ( Modes( SHIPMASTER ) ? "SHIPMASTER," : "" ) << ( Modes( CUSTOM ) ? "CUSTOM," : "" ) << ( Modes( PATROL ) ? "PATROL" : "" )
       << std::endl;

    if ( Modes( PATROL ) ) {
        os << "patrol zone     : center: (" << _patrolCenter.x << ", " << _patrolCenter.y << "), distance " << _patrolDistance << std::endl;
    }

    if ( !_visitedObjects.empty() ) {
        os << "visit objects   : ";
        for ( const auto & info : _visitedObjects ) {
            os << MP2::StringObject( info.second ) << "(" << info.first << "), ";
        }

        os << std::endl;
    }

    if ( isControlAI() ) {
        os << "skills          : " << _secondarySkills.String() << std::endl
           << "artifacts       : " << _bagArtifacts.String() << std::endl
           << "spell book      : " << ( HaveSpellBook() ? _spellBook.String() : "disabled" ) << std::endl
           << "army dump       : " << _army.String() << std::endl
           << "ai role         : " << GetHeroRoleString( *this ) << std::endl;
    }

    return os.str();
}

Heroes::HeroSeedsForLevelUp Heroes::_getSeedsForLevelUp() const
{
    /* We generate seeds based on the hero and global world map seed
     * The idea is that, we want the skill selection to be randomized at each map restart,
     * but deterministic for a given hero.
     * We also want the available skills to change depending on current skills/stats of the hero,
     * to avoid giving out the same skills/stats at each level up. We can't use the level field for this, as it
     * doesn't change when we level up several levels at once.
     * We also need to generate different seeds for each possible call to the random number generator,
     * in order to avoid always drawing the same random number at level-up: otherwise this
     * would mean that for all possible games, the 2nd secondary
     * skill would always be the same once the 1st one is selected.
     * */

    uint32_t hash = world.GetMapSeed();
    Rand::combineSeedWithValueHash( hash, _id );
    Rand::combineSeedWithValueHash( hash, _race );
    Rand::combineSeedWithValueHash( hash, attack );
    Rand::combineSeedWithValueHash( hash, defense );
    Rand::combineSeedWithValueHash( hash, power );
    Rand::combineSeedWithValueHash( hash, knowledge );
    for ( int skillId = Skill::Secondary::PATHFINDING; skillId <= Skill::Secondary::ESTATES; ++skillId ) {
        Rand::combineSeedWithValueHash( hash, GetLevelSkill( skillId ) );
    }

    HeroSeedsForLevelUp seeds;
    seeds.seedPrimarySkill = hash;
    seeds.seedSecondarySkill1 = hash + 1;
    seeds.seedSecondarySkill2 = hash + 2;
    seeds.seedSecondarySkillRandomChoose = hash + 3;
    return seeds;
}

double Heroes::getAIMinimumJoiningArmyStrength() const
{
    assert( isControlAI() );

    double strengthThreshold = 0.05;

    switch ( getAIRole() ) {
    case Heroes::Role::SCOUT:
        strengthThreshold = 0.01;
        break;
    case Heroes::Role::COURIER:
        strengthThreshold = 0.015;
        break;
    case Heroes::Role::HUNTER:
        strengthThreshold = 0.02;
        break;
    case Heroes::Role::FIGHTER:
        strengthThreshold = 0.025;
        break;
    case Heroes::Role::CHAMPION:
        strengthThreshold = 0.03;
        break;
    default:
        // Did you add a new AI hero role? Add the logic above!
        assert( 0 );
        break;
    }

    return strengthThreshold * Troops( GetArmy().getTroops() ).GetStrength();
}

void AllHeroes::Init()
{
    Clear();

    _heroes.reserve( Heroes::HEROES_COUNT );

    _heroes.emplace_back( std::make_unique<Heroes>( Heroes::UNKNOWN, Race::KNGT ) );

    for ( const int race : std::array<int, 6>{ Race::KNGT, Race::BARB, Race::SORC, Race::WRLK, Race::WZRD, Race::NECR } ) {
        const auto [minHeroId, maxHeroId] = getHeroIdRangeForRace( race );
        assert( minHeroId <= maxHeroId );

        for ( int hid = minHeroId; hid <= maxHeroId; ++hid ) {
            _heroes.emplace_back( std::make_unique<Heroes>( hid, race ) );
        }
    }

    // SW campaign
    _heroes.emplace_back( std::make_unique<Heroes>( Heroes::ROLAND, Race::WZRD, 5000 ) );
    _heroes.emplace_back( std::make_unique<Heroes>( Heroes::CORLAGON, Race::KNGT, 5000 ) );
    _heroes.emplace_back( std::make_unique<Heroes>( Heroes::ELIZA, Race::SORC, 5000 ) );
    _heroes.emplace_back( std::make_unique<Heroes>( Heroes::ARCHIBALD, Race::WRLK, 5000 ) );
    _heroes.emplace_back( std::make_unique<Heroes>( Heroes::HALTON, Race::KNGT, 5000 ) );
    _heroes.emplace_back( std::make_unique<Heroes>( Heroes::BRAX, Race::NECR, 5000 ) );

    // PoL
    const GameVersion version = Settings::Get().getCurrentMapInfo().version;
    if ( version == GameVersion::PRICE_OF_LOYALTY || version == GameVersion::RESURRECTION ) {
        _heroes.emplace_back( std::make_unique<Heroes>( Heroes::SOLMYR, Race::WZRD, 5000 ) );
        _heroes.emplace_back( std::make_unique<Heroes>( Heroes::DAINWIN, Race::WRLK, 5000 ) );
        _heroes.emplace_back( std::make_unique<Heroes>( Heroes::MOG, Race::NECR, 5000 ) );
        _heroes.emplace_back( std::make_unique<Heroes>( Heroes::UNCLEIVAN, Race::BARB, 5000 ) );
        _heroes.emplace_back( std::make_unique<Heroes>( Heroes::JOSEPH, Race::WZRD, 5000 ) );
        _heroes.emplace_back( std::make_unique<Heroes>( Heroes::GALLAVANT, Race::KNGT, 5000 ) );
        _heroes.emplace_back( std::make_unique<Heroes>( Heroes::ELDERIAN, Race::WRLK, 5000 ) );
        _heroes.emplace_back( std::make_unique<Heroes>( Heroes::CEALLACH, Race::KNGT, 5000 ) );
        _heroes.emplace_back( std::make_unique<Heroes>( Heroes::DRAKONIA, Race::WZRD, 5000 ) );
        _heroes.emplace_back( std::make_unique<Heroes>( Heroes::MARTINE, Race::SORC, 5000 ) );
        _heroes.emplace_back( std::make_unique<Heroes>( Heroes::JARKONAS, Race::BARB, 5000 ) );
    }
    else {
        // for non-PoL maps, just add unknown heroes instead in place of the PoL-specific ones
        for ( int i = Heroes::SOLMYR; i <= Heroes::JARKONAS; ++i ) {
            _heroes.emplace_back( std::make_unique<Heroes>( Heroes::UNKNOWN, Race::KNGT ) );
        }
    }

    if ( IS_DEVEL() ) {
        _heroes.emplace_back( std::make_unique<Heroes>( Heroes::DEBUG_HERO, Race::WRLK ) );
    }
    else {
        _heroes.emplace_back( std::make_unique<Heroes>( Heroes::UNKNOWN, Race::KNGT ) );
    }

    assert( _heroes.size() == Heroes::HEROES_COUNT );
}

Heroes * AllHeroes::GetHeroForHire( const int race, const int heroIDToIgnore ) const
{
    const std::set<int> customHeroesPortraits = [this]() {
        std::set<int> result;

        for ( const Heroes * hero : *this ) {
            assert( hero != nullptr );

            if ( !hero->Modes( Heroes::CUSTOM ) ) {
                continue;
            }

            result.insert( hero->getPortraitId() );
        }

        return result;
    }();

    std::vector<int> heroesForHire;
    heroesForHire.reserve( Heroes::HEROES_COUNT - 2 );

    const auto fillHeroesForHire = [this, heroIDToIgnore, &customHeroesPortraits, &heroesForHire]( const int raceFilter, const bool avoidCustomHeroes ) {
        const auto [minHeroId, maxHeroId] = getHeroIdRangeForRace( Race::NONE );

        for ( const Heroes * hero : *this ) {
            assert( hero != nullptr );

            // Only regular (non-campaign) heroes are available for hire
            if ( hero->GetID() > maxHeroId ) {
                continue;
            }

            if ( hero->GetID() == heroIDToIgnore ) {
                continue;
            }

            if ( raceFilter != Race::NONE && hero->GetRace() != raceFilter ) {
                continue;
            }

            if ( !hero->isAvailableForHire() ) {
                continue;
            }

            if ( avoidCustomHeroes && customHeroesPortraits.find( hero->getPortraitId() ) != customHeroesPortraits.end() ) {
                continue;
            }

            heroesForHire.push_back( hero->GetID() );
        }
    };

    // First, try to find a free hero of the specified race (avoiding customized heroes, as well as heroes with non-unique portraits)
    fillHeroesForHire( race, true );

    // If no suitable heroes were found, then try to find a free hero of any race (avoiding customized heroes, as well as heroes with non-unique portraits)
    if ( heroesForHire.empty() && race != Race::NONE ) {
        fillHeroesForHire( Race::NONE, true );
    }

    // No suitable heroes were found, any free hero will do
    if ( heroesForHire.empty() ) {
        fillHeroesForHire( Race::NONE, false );
    }

    // All the heroes are busy
    if ( heroesForHire.empty() ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, "no hero found for hire, all the heroes are busy." )
        return nullptr;
    }

    // Try to avoid heroes who are already available for recruitment in any kingdom
    std::vector<int> heroesForHireNotRecruits = heroesForHire;

    heroesForHireNotRecruits.erase( std::remove_if( heroesForHireNotRecruits.begin(), heroesForHireNotRecruits.end(),
                                                    [this]( const int heroID ) {
                                                        assert( heroID >= 0 && static_cast<size_t>( heroID ) < _heroes.size() && _heroes[heroID] );

                                                        return _heroes[heroID]->Modes( Heroes::RECRUIT );
                                                    } ),
                                    heroesForHireNotRecruits.end() );

    // If there are no heroes who are not yet available for recruitment, then allow heroes to be available for recruitment in several kingdoms at the same time
    const int heroID = heroesForHireNotRecruits.empty() ? Rand::Get( heroesForHire ) : Rand::Get( heroesForHireNotRecruits );
    assert( heroID >= 0 && static_cast<size_t>( heroID ) < _heroes.size() && _heroes[heroID] );

    return _heroes[heroID].get();
}

Heroes * AllHeroes::Get( const int heroId ) const
{
    if ( !Heroes::isValidId( heroId ) ) {
        return nullptr;
    }

    assert( heroId >= 0 && static_cast<size_t>( heroId ) < _heroes.size() && _heroes[heroId] );

    return _heroes[heroId].get();
}

Heroes * AllHeroes::Get( const fheroes2::Point & center ) const
{
    for ( Heroes * hero : *this ) {
        assert( hero != nullptr );

        if ( hero->isPosition( center ) ) {
            return hero;
        }
    }

    return nullptr;
}

void AllHeroes::Scout( PlayerColorsSet colors ) const
{
    for ( const Heroes * hero : *this ) {
        assert( hero != nullptr );

        if ( ( colors & hero->GetColor() ) == 0 ) {
            continue;
        }

        hero->Scout( hero->GetIndex() );
    }
}

void AllHeroes::ResetModes( const uint32_t modes ) const
{
    std::for_each( begin(), end(), [modes]( Heroes * hero ) {
        assert( hero != nullptr );

        hero->ResetModes( modes );
    } );
}

void AllHeroes::NewDay() const
{
    std::for_each( begin(), end(), []( Heroes * hero ) {
        assert( hero != nullptr );

        hero->ActionNewDay();
    } );
}

void AllHeroes::NewWeek() const
{
    std::for_each( begin(), end(), []( Heroes * hero ) {
        assert( hero != nullptr );

        hero->ActionNewWeek();
    } );
}

Heroes * AllHeroes::FromJail( const int32_t tileIndex ) const
{
    for ( Heroes * hero : *this ) {
        assert( hero != nullptr );

        if ( hero->Modes( Heroes::JAIL ) && tileIndex == hero->GetIndex() ) {
            return hero;
        }
    }

    return nullptr;
}

void Heroes::fixFrenchCharactersInName()
{
    fheroes2::fixFrenchCharactersForMP2Map( _name );
}

OStreamBase & operator<<( OStreamBase & stream, const VecHeroes & heroes )
{
    stream.put32( static_cast<uint32_t>( heroes.size() ) );

    std::for_each( heroes.begin(), heroes.end(), [&stream]( const Heroes * hero ) {
        assert( hero != nullptr );

        stream << hero->GetID();
    } );

    return stream;
}

IStreamBase & operator>>( IStreamBase & stream, VecHeroes & heroes )
{
    const uint32_t size = stream.get32();

    heroes.clear();
    heroes.reserve( size );

    for ( uint32_t i = 0; i < size; ++i ) {
        int32_t hid{ -1 };
        stream >> hid;

        static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_1010_RELEASE, "Remove the logic below." );
        if ( Game::GetVersionOfCurrentSaveFile() < FORMAT_VERSION_1010_RELEASE ) {
            // UNKNOWN was 72 before FORMAT_VERSION_1010_RELEASE. UNKNOWN hero shouldn't exist!
            if ( hid == 72 || !Heroes::isValidId( hid + 1 ) ) {
                // Most likely the save file is corrupted.
                stream.setFail();

                continue;
            }

            Heroes * hero = world.GetHeroes( hid + 1 );
            if ( hero == nullptr ) {
                // Most likely the save file is corrupted.
                stream.setFail();

                continue;
            }

            heroes.emplace_back( hero );
        }
        else {
            if ( !Heroes::isValidId( hid ) ) {
                // Most likely the save file is corrupted.
                stream.setFail();

                continue;
            }

            Heroes * hero = world.GetHeroes( hid );
            if ( hero == nullptr ) {
                // Most likely the save file is corrupted.
                stream.setFail();

                continue;
            }

            heroes.emplace_back( hero );
        }
    }

    return stream;
}

OStreamBase & operator<<( OStreamBase & stream, const Heroes & hero )
{
    const HeroBase & base = hero;
    const ColorBase & col = hero;

    // HeroBase
    stream << base;

    // Heroes
    return stream << hero._name << col << hero._experience << hero._secondarySkills << hero._army << hero._id << hero._portrait << hero._race << hero._objectTypeUnderHero
                  << hero._path << hero._direction << hero._spriteIndex << hero._patrolCenter << hero._patrolDistance << hero._visitedObjects << hero._lastGroundRegion;
}

IStreamBase & operator>>( IStreamBase & stream, Heroes & hero )
{
    HeroBase & base = hero;
    ColorBase & col = hero;

    // HeroBase
    stream >> base;

    // Heroes
    stream >> hero._name >> col >> hero._experience >> hero._secondarySkills >> hero._army >> hero._id >> hero._portrait >> hero._race;

    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_PRE1_1100_RELEASE, "Remove the logic below." );
    if ( Game::GetVersionOfCurrentSaveFile() < FORMAT_VERSION_PRE1_1100_RELEASE ) {
        // Before FORMAT_VERSION_PRE1_1100_RELEASE we did not check that a custom hero name is empty set inside the original map.
        // This leads to assertion rise while rendering text. Also, it is incorrect to have a hero with no name.
        if ( hero._name.empty() ) {
            hero._name = Heroes::getDefaultName( hero._id );
        }
    }

    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_1010_RELEASE, "Remove the logic below." );
    if ( Game::GetVersionOfCurrentSaveFile() < FORMAT_VERSION_1010_RELEASE ) {
        // Before FORMAT_VERSION_1010_RELEASE Heroes::UNKNOWN was 72.
        if ( hero._id == 72 ) {
            hero._id = Heroes::UNKNOWN;
            hero._portrait = Heroes::UNKNOWN;
        }
        else {
            ++hero._id;
            ++hero._portrait;
        }
    }

    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_PRE3_1100_RELEASE, "Remove the logic below." );
    if ( Game::GetVersionOfCurrentSaveFile() < FORMAT_VERSION_PRE3_1100_RELEASE ) {
        static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_PRE1_1009_RELEASE, "Remove the logic below." );
        if ( Game::GetVersionOfCurrentSaveFile() < FORMAT_VERSION_PRE1_1009_RELEASE ) {
            int temp = 0;
            stream >> temp;

            hero._objectTypeUnderHero = static_cast<MP2::MapObjectType>( temp );
        }
        else {
            uint8_t temp = 0;

            stream >> temp;

            hero._objectTypeUnderHero = static_cast<MP2::MapObjectType>( temp );
        }
    }
    else {
        stream >> hero._objectTypeUnderHero;
    }

    stream >> hero._path >> hero._direction >> hero._spriteIndex;

    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_1010_RELEASE, "Remove the logic below." );
    if ( Game::GetVersionOfCurrentSaveFile() < FORMAT_VERSION_1010_RELEASE ) {
        int16_t patrolX = 0;
        int16_t patrolY = 0;

        stream >> patrolX >> patrolY;
        hero._patrolCenter = fheroes2::Point( patrolX, patrolY );
    }
    else {
        stream >> hero._patrolCenter;
    }

    stream >> hero._patrolDistance >> hero._visitedObjects >> hero._lastGroundRegion;

    hero._army.SetCommander( &hero );

    return stream;
}

OStreamBase & operator<<( OStreamBase & stream, const AllHeroes & heroes )
{
    stream.put32( static_cast<uint32_t>( heroes._heroes.size() ) );

    std::for_each( heroes.begin(), heroes.end(), [&stream]( const Heroes * hero ) {
        assert( hero != nullptr );

        stream << *hero;
    } );

    return stream;
}

IStreamBase & operator>>( IStreamBase & stream, AllHeroes & heroes )
{
    std::vector<std::unique_ptr<Heroes>> & heroesRef = heroes._heroes;

    const uint32_t size = stream.get32();

    heroesRef.clear();
    heroesRef.reserve( size );

    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_1010_RELEASE, "Remove the logic below." );
    if ( Game::GetVersionOfCurrentSaveFile() < FORMAT_VERSION_1010_RELEASE ) {
        // Before FORMAT_VERSION_1010_RELEASE UNKNOWN hero was last while now it is first.
        // In order to preserve the original order of heroes we have to do the below trick.
        if ( size > 0 ) {
            heroesRef.emplace_back( std::make_unique<Heroes>() );

            for ( uint32_t i = 1; i < size; ++i ) {
                auto hero = std::make_unique<Heroes>();
                stream >> *hero;

                heroesRef.emplace_back( std::move( hero ) );
            }

            stream >> *( heroesRef[0] );
        }
    }
    else {
        for ( uint32_t i = 0; i < size; ++i ) {
            auto hero = std::make_unique<Heroes>();
            stream >> *hero;

            heroesRef.emplace_back( std::move( hero ) );
        }
    }

    return stream;
}
