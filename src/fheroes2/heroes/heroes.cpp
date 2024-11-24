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

#include "heroes.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
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
#include "world.h"

namespace
{
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

    int ObjectVisitedModifiersResult( const std::vector<MP2::MapObjectType> & objectTypes, const Heroes & hero, std::string * strs )
    {
        int result = 0;

        for ( const MP2::MapObjectType objectType : objectTypes ) {
            if ( hero.isObjectTypeVisited( objectType ) ) {
                result += GameStatic::ObjectVisitedModifiers( objectType );

                if ( strs ) {
                    switch ( objectType ) {
                    case MP2::OBJ_GRAVEYARD:
                    case MP2::OBJ_NON_ACTION_GRAVEYARD:
                    case MP2::OBJ_SHIPWRECK:
                    case MP2::OBJ_NON_ACTION_SHIPWRECK:
                    case MP2::OBJ_DERELICT_SHIP:
                    case MP2::OBJ_NON_ACTION_DERELICT_SHIP: {
                        std::string modRobber = _( "shipAndGraveyard|%{object} robber" );
                        StringReplace( modRobber, "%{object}", MP2::StringObject( objectType ) );
                        strs->append( modRobber );
                        break;
                    }
                    case MP2::OBJ_PYRAMID:
                    case MP2::OBJ_NON_ACTION_PYRAMID: {
                        std::string modRaided = _( "pyramid|%{object} raided" );
                        StringReplace( modRaided, "%{object}", MP2::StringObject( objectType ) );
                        strs->append( modRaided );
                        break;
                    }
                    default:
                        strs->append( MP2::StringObject( objectType ) );
                        break;
                    }

                    fheroes2::appendModifierToString( *strs, GameStatic::ObjectVisitedModifiers( objectType ) );
                    strs->append( "\n" );
                }
            }
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

const char * Heroes::GetName( int heroid )
{
    assert( heroid >= UNKNOWN && heroid < HEROES_COUNT );

    const std::array<const char *, HEROES_COUNT> names
        = { // Unknown / uninitialized hero.
            "Unknown",

            // Knight heroes from The Succession Wars.
            gettext_noop( "Lord Kilburn" ), gettext_noop( "Sir Gallant" ), gettext_noop( "Ector" ), gettext_noop( "Gwenneth" ), gettext_noop( "Tyro" ),
            gettext_noop( "Ambrose" ), gettext_noop( "Ruby" ), gettext_noop( "Maximus" ), gettext_noop( "Dimitry" ),

            // Barbarian heroes from The Succession Wars.
            gettext_noop( "Thundax" ), gettext_noop( "Fineous" ), gettext_noop( "Jojosh" ), gettext_noop( "Crag Hack" ), gettext_noop( "Jezebel" ),
            gettext_noop( "Jaclyn" ), gettext_noop( "Ergon" ), gettext_noop( "Tsabu" ), gettext_noop( "Atlas" ),

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
            gettext_noop( "Solmyr" ), gettext_noop( "Dainwin" ), gettext_noop( "Mog" ), gettext_noop( "Uncle Ivan" ), gettext_noop( "Joseph" ),
            gettext_noop( "Gallavant" ), _( "Elderian" ), gettext_noop( "Ceallach" ), gettext_noop( "Drakonia" ), gettext_noop( "Martine" ), gettext_noop( "Jarkonas" ),

            // Debug hero. Should not be used anywhere outside the development!
            "Debug Hero" };

    return _( names[heroid] );
}

Heroes::Heroes()
    : experience( 0 )
    , army( this )
    , _id( UNKNOWN )
    , portrait( UNKNOWN )
    , _race( Race::NONE )
    , _objectTypeUnderHero( MP2::OBJ_NONE )
    , path( *this )
    , direction( Direction::RIGHT )
    , sprite_index( 18 )
    , _patrolDistance( 0 )
    , _alphaValue( 255 )
    , _attackedMonsterTileIndex( -1 )
    , _aiRole( Role::HUNTER )
{
    // Do nothing.
}

Heroes::Heroes( const int heroID, const int race, const uint32_t additionalExperience )
    : Heroes( heroID, race )
{
    IncreaseExperience( additionalExperience, true );
}

Heroes::Heroes( int heroid, int rc )
    : HeroBase( HeroBase::HEROES, rc )
    , ColorBase( Color::NONE )
    , experience( GetStartingXp() )
    , secondary_skills( rc )
    , army( this )
    , _id( heroid )
    , portrait( heroid )
    , _race( rc )
    , _objectTypeUnderHero( MP2::OBJ_NONE )
    , path( *this )
    , direction( Direction::RIGHT )
    , sprite_index( 18 )
    , _patrolDistance( 0 )
    , _alphaValue( 255 )
    , _attackedMonsterTileIndex( -1 )
    , _aiRole( Role::HUNTER )
{
    name = _( Heroes::GetName( heroid ) );

    army.Reset( true );

    // Add to debug hero a lot of stuff.
    if ( _id == DEBUG_HERO ) {
        army.Clean();
        army.JoinTroop( Monster::BLACK_DRAGON, 2, false );
        army.JoinTroop( Monster::RED_DRAGON, 3, false );

        secondary_skills = Skill::SecSkills();
        secondary_skills.AddSkill( Skill::Secondary( Skill::Secondary::PATHFINDING, Skill::Level::ADVANCED ) );
        secondary_skills.AddSkill( Skill::Secondary( Skill::Secondary::LOGISTICS, Skill::Level::ADVANCED ) );
        secondary_skills.AddSkill( Skill::Secondary( Skill::Secondary::SCOUTING, Skill::Level::BASIC ) );
        secondary_skills.AddSkill( Skill::Secondary( Skill::Secondary::MYSTICISM, Skill::Level::BASIC ) );

        PickupArtifact( Artifact::STEALTH_SHIELD );
        PickupArtifact( Artifact::DRAGON_SWORD );
        PickupArtifact( Artifact::NOMAD_BOOTS_MOBILITY );
        PickupArtifact( Artifact::TRAVELER_BOOTS_MOBILITY );
        PickupArtifact( Artifact::TRUE_COMPASS_MOBILITY );

        experience = 777;
        magic_point = 120;

        // This hero has all the spells in his spell book
        for ( const int spellId : Spell::getAllSpellIdsSuitableForSpellBook() ) {
            AppendSpellToBook( Spell( spellId ), true );
        }
    }

    if ( !magic_point ) {
        SetSpellPoints( GetMaxSpellPoints() );
    }
    move_point = GetMaxMovePoints();
}

void Heroes::LoadFromMP2( const int32_t mapIndex, const int colorType, const int raceType, const bool isInJail, const std::vector<uint8_t> & data )
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
    secondary_skills = Skill::SecSkills( _race );

    // Reset the army to default
    army.Reset( true );

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

        army.Assign( troops, std::end( troops ) );

        // On some maps, customized heroes don't have an army, give them a minimal army
        if ( !army.isValid() ) {
            army.Reset( false );
        }
    }
    else {
        dataStream.skip( 15 );
    }

    const bool doesHeroHaveCustomPortrait = ( dataStream.get() != 0 );
    if ( doesHeroHaveCustomPortrait ) {
        SetModes( CUSTOM );

        // Portrait sprite index. It should be increased by 1 as in the original game hero IDs start from 0.
        portrait = dataStream.get() + 1;

        if ( !isValidId( portrait ) ) {
            DEBUG_LOG( DBG_GAME, DBG_WARN, "Invalid MP2 file format: incorrect custom portrait ID: " << portrait )
            portrait = _id;
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
    experience = dataStream.getLE32();

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

        secondary_skills = Skill::SecSkills();

        for ( const Skill::Secondary & skill : secs ) {
            if ( skill.isValid() ) {
                // The original map editor does not check presence of similar skills even those which have different levels.
                // We need to check whether the existing skill has a lower level before updating it.
                const auto * existingSkill = secondary_skills.FindSkill( skill.Skill() );
                if ( existingSkill == nullptr || ( existingSkill->Level() < skill.Level() ) ) {
                    secondary_skills.AddSkill( skill );
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
            name = std::move( temp );
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
            LevelUp( doesHeroHaveCustomSecondarySkills, true );
        }
    }

    SetSpellPoints( GetMaxSpellPoints() );
    move_point = GetMaxMovePoints();

    DEBUG_LOG( DBG_GAME, DBG_INFO, name << ", color: " << Color::String( GetColor() ) << ", race: " << Race::String( _race ) )
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

    if ( !Maps::loadHeroArmy( army, heroMetadata ) && !isEditor ) {
        // Reset the army to default
        army.Reset( true );
    }

    // Hero's portrait.
    if ( heroMetadata.customPortrait > 0 ) {
        SetModes( CUSTOM );

        assert( isValidId( heroMetadata.customPortrait ) );

        // Portrait sprite index.
        portrait = heroMetadata.customPortrait;
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

        secondary_skills = {};

        for ( size_t i = 0; i < heroMetadata.secondarySkill.size(); ++i ) {
            secondary_skills.AddSkill( Skill::Secondary{ heroMetadata.secondarySkill[i], heroMetadata.secondarySkillLevel[i] } );
        }
    }
    else if ( !isEditor ) {
        // Reset secondary skills to defaults
        secondary_skills = Skill::SecSkills( _race );
    }

    // For Editor we need to fill all the rest of the 8 skills with the empty ones.
    if ( isEditor ) {
        GetSecondarySkills().FillMax( Skill::Secondary() );
    }

    if ( isEditor || !heroMetadata.customName.empty() ) {
        SetModes( CUSTOM );
        name = heroMetadata.customName;
    }

    if ( heroMetadata.isOnPatrol ) {
        SetModes( PATROL );

        _patrolCenter = GetCenter();
        _patrolDistance = heroMetadata.patrolRadius;
    }

    // Hero's experience.
    if ( heroMetadata.customExperience > -1 ) {
        experience = heroMetadata.customExperience;
    }
    else if ( isEditor ) {
        // There is no way to set "default experience" condition, so we will consider 'UINT32_MAX' as it.
        experience = UINT32_MAX;
    }

    // Level up if needed.
    if ( !isEditor ) {
        const int16_t level = heroMetadata.customLevel > -1 ? heroMetadata.customLevel : static_cast<int16_t>( GetLevel() );
        if ( level > 1 ) {
            SetModes( CUSTOM );

            for ( int16_t i = 1; i < level; ++i ) {
                LevelUp( doesHeroHaveCustomSecondarySkills, true );
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
            magic_point = UINT32_MAX;
        }
        else {
            magic_point = GetMaxSpellPoints();
        }
    }
    else {
        magic_point = static_cast<uint32_t>( heroMetadata.magicPoints );
    }

    move_point = GetMaxMovePoints();
}

Maps::Map_Format::HeroMetadata Heroes::getHeroMetadata() const
{
    Maps::Map_Format::HeroMetadata heroMetadata;

    Maps::saveHeroArmy( army, heroMetadata );

    // Hero's portrait.
    heroMetadata.customPortrait = portrait;

    // Hero's artifacts.
    const size_t artifactCount = bag_artifacts.size();
    assert( artifactCount == heroMetadata.artifactMetadata.size() );
    for ( size_t i = 0; i < artifactCount; ++i ) {
        heroMetadata.artifact[i] = bag_artifacts[i].GetID();
        // The spell scroll may contain a spell.

        if ( heroMetadata.artifact[i] == Artifact::SPELL_SCROLL ) {
            const int32_t artifactSpellId = bag_artifacts[i].getSpellId();

            assert( artifactSpellId != Spell::NONE );

            heroMetadata.artifactMetadata[i] = artifactSpellId;
        }
    }

    // Hero's spells.
    const size_t bookSize = spell_book.size();
    heroMetadata.availableSpells.reserve( bookSize );
    for ( size_t i = 0; i < bookSize; ++i ) {
        heroMetadata.availableSpells.push_back( spell_book[i].GetID() );
    }

    // Hero's secondary skills.
    const std::vector<Skill::Secondary> & skills = secondary_skills.ToVector();
    const size_t skillsSize = skills.size();
    assert( heroMetadata.secondarySkill.size() == skillsSize && heroMetadata.secondarySkillLevel.size() == skillsSize );
    for ( size_t i = 0; i < skillsSize; ++i ) {
        heroMetadata.secondarySkill[i] = static_cast<int8_t>( skills[i].Skill() );
        heroMetadata.secondarySkillLevel[i] = static_cast<uint8_t>( skills[i].Level() );
    }

    // Hero's name.
    heroMetadata.customName = name;

    // Patrol mode.
    heroMetadata.isOnPatrol = Modes( PATROL );
    heroMetadata.patrolRadius = static_cast<uint8_t>( _patrolDistance );

    // Hero's experience.
    heroMetadata.customExperience = ( experience == UINT32_MAX ) ? -1 : static_cast<int32_t>( experience );

    // Primary Skill base values.
    heroMetadata.customAttack = static_cast<int16_t>( attack );
    heroMetadata.customDefense = static_cast<int16_t>( defense );
    heroMetadata.customSpellPower = static_cast<int16_t>( power );
    heroMetadata.customKnowledge = static_cast<int16_t>( knowledge );

    // Hero's spell points.
    heroMetadata.magicPoints = ( magic_point == UINT32_MAX ) ? static_cast<int16_t>( -1 ) : static_cast<int16_t>( magic_point );

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
    return name;
}

int Heroes::GetColor() const
{
    return ColorBase::GetColor();
}

int Heroes::GetType() const
{
    return HeroBase::HEROES;
}

const Army & Heroes::GetArmy() const
{
    return army;
}

Army & Heroes::GetArmy()
{
    return army;
}

int Heroes::GetMobilityIndexSprite() const
{
    // valid range (0 - 25)
    int index = CanMove() ? ( move_point + 50 ) / 100 : 0;
    return 25 >= index ? index : 25;
}

int Heroes::GetManaIndexSprite() const
{
    // Add 2 to round values.
    const int value = ( GetSpellPoints() + 2 ) / 5;
    return value >= 25 ? 25 : value;
}

int Heroes::getStatsValue() const
{
    // experience and artifacts don't matter here, only natural stats
    return getTotalPrimarySkillLevel() + secondary_skills.GetTotalLevel();
}

double Heroes::getRecruitValue() const
{
    return army.GetStrength() + ( ( bag_artifacts.getArtifactValue() * 10.0 + getStatsValue() ) * SKILL_VALUE );
}

double Heroes::getMeetingValue( const Heroes & receivingHero ) const
{
    // TODO: add logic to check artifacts with curses and those which are invaluable for a hero.

    // Magic Book is not transferable.
    const uint32_t artCount = bag_artifacts.CountArtifacts() - bag_artifacts.Count( Artifact::MAGIC_BOOK );
    const uint32_t canFit = BagArtifacts::maxCapacity - receivingHero.bag_artifacts.CountArtifacts();

    double artifactValue = bag_artifacts.getArtifactValue() * 5.0;
    if ( artCount > canFit ) {
        artifactValue = canFit * ( artifactValue / artCount );
    }

    // TODO: leaving only one monster in an army is very risky. Add logic to find out which part of the army would be useful to get.
    return receivingHero.army.getReinforcementValue( army ) + artifactValue * SKILL_VALUE;
}

int Heroes::GetAttack() const
{
    return GetAttack( nullptr );
}

int Heroes::GetAttack( std::string * strs ) const
{
    int result = attack + GetAttackModificator( strs );
    return std::clamp( result, 0, 255 );
}

int Heroes::GetDefense() const
{
    return GetDefense( nullptr );
}

int Heroes::GetDefense( std::string * strs ) const
{
    int result = defense + GetDefenseModificator( strs );
    return std::clamp( result, 0, 255 );
}

int Heroes::GetPower() const
{
    return GetPower( nullptr );
}

int Heroes::GetPower( std::string * strs ) const
{
    const int result = power + GetPowerModificator( strs );
    return std::clamp( result, 1, 255 );
}

int Heroes::GetKnowledge() const
{
    return GetKnowledge( nullptr );
}

int Heroes::GetKnowledge( std::string * strs ) const
{
    int result = knowledge + GetKnowledgeModificator( strs );
    return std::clamp( result, 0, 255 );
}

void Heroes::IncreasePrimarySkill( int skill )
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

uint32_t Heroes::GetMaxMovePoints() const
{
    return GetMaxMovePoints( isShipMaster() );
}

uint32_t Heroes::GetMaxMovePoints( const bool onWater ) const
{
    uint32_t result = 0;

    if ( onWater ) {
        // Initial mobility on water does not depend on the composition of the army
        result = 1500;

        // Influence of Navigation skill
        result = UpdateMovementPoints( result, Skill::Secondary::NAVIGATION );

        // Artifact bonuses
        result += GetBagArtifacts().getTotalArtifactEffectValue( fheroes2::ArtifactBonusType::SEA_MOBILITY );

        // Bonuses from captured lighthouses
        result += 500 * world.CountCapturedObject( MP2::OBJ_LIGHTHOUSE, GetColor() );
    }
    else {
        // Initial mobility on land depends on the speed of the slowest army unit
        const Troop * troop = army.GetSlowestTroop();
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
        result = UpdateMovementPoints( result, Skill::Secondary::LOGISTICS );

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
    return GetMoraleWithModificators( nullptr );
}

int Heroes::GetMoraleWithModificators( std::string * strs ) const
{
    int result = Morale::NORMAL;

    // bonus leadership
    result += Skill::GetLeadershipModifiers( GetLevelSkill( Skill::Secondary::LEADERSHIP ), strs );

    // object visited
    const std::vector<MP2::MapObjectType> objectTypes{ MP2::OBJ_BUOY,      MP2::OBJ_OASIS,         MP2::OBJ_WATERING_HOLE, MP2::OBJ_TEMPLE,
                                                       MP2::OBJ_GRAVEYARD, MP2::OBJ_DERELICT_SHIP, MP2::OBJ_SHIPWRECK };
    result += ObjectVisitedModifiersResult( objectTypes, *this, strs );

    // bonus artifact
    result += GetMoraleModificator( strs );

    // A special artifact ability presence must be the last check.
    const Artifact maxMoraleArtifact = bag_artifacts.getFirstArtifactWithBonus( fheroes2::ArtifactBonusType::MAXIMUM_MORALE );
    if ( maxMoraleArtifact.isValid() ) {
        if ( strs != nullptr ) {
            *strs += maxMoraleArtifact.GetName();
            *strs += _( " gives you maximum morale" );
        }
        result = Morale::BLOOD;
    }
    else if ( strs != nullptr && !strs->empty() && strs->back() == '\n' ) {
        // Remove the possible empty line at the end of the string.
        strs->pop_back();
    }

    return Morale::Normalize( result );
}

int Heroes::GetLuck() const
{
    return GetLuckWithModificators( nullptr );
}

int Heroes::GetLuckWithModificators( std::string * strs ) const
{
    int result = Luck::NORMAL;

    // bonus luck
    result += Skill::GetLuckModifiers( GetLevelSkill( Skill::Secondary::LUCK ), strs );

    // object visited
    const std::vector<MP2::MapObjectType> objectTypes{ MP2::OBJ_MERMAID, MP2::OBJ_FAERIE_RING, MP2::OBJ_FOUNTAIN, MP2::OBJ_IDOL, MP2::OBJ_PYRAMID };
    result += ObjectVisitedModifiersResult( objectTypes, *this, strs );

    // bonus artifact
    result += GetLuckModificator( strs );

    const Artifact maxLuckArtifact = bag_artifacts.getFirstArtifactWithBonus( fheroes2::ArtifactBonusType::MAXIMUM_LUCK );
    if ( maxLuckArtifact.isValid() ) {
        if ( strs != nullptr ) {
            *strs += maxLuckArtifact.GetName();
            *strs += _( " gives you maximum luck" );
        }
        result = Luck::IRISH;
    }
    else if ( strs != nullptr && !strs->empty() && strs->back() == '\n' ) {
        // Remove the possible empty line at the end of the string.
        strs->pop_back();
    }

    return Luck::Normalize( result );
}

bool Heroes::Recruit( const int col, const fheroes2::Point & pt )
{
    if ( GetColor() != Color::NONE ) {
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
        move_point = GetMaxMovePoints();
    }

    if ( !army.isValid() ) {
        army.Reset( false );
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

    if ( castle.GetLevelMageGuild() ) {
        castle.MageGuildEducateHero( *this );
    }

    SetVisited( GetIndex() );
    return true;
}

void Heroes::ActionNewDay()
{
    move_point = GetMaxMovePoints();

    if ( world.CountDay() > 1 ) {
        ReplenishSpellPoints();
    }

    visit_object.remove_if( Visit::isDayLife );

    ResetModes( SAVEMP );
}

void Heroes::ActionNewWeek()
{
    visit_object.remove_if( Visit::isWeekLife );
}

void Heroes::ActionNewMonth()
{
    visit_object.remove_if( Visit::isMonthLife );
}

void Heroes::ActionAfterBattle()
{
    visit_object.remove_if( Visit::isBattleLife );

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

void Heroes::ReplenishSpellPoints()
{
    const uint32_t maxp = GetMaxSpellPoints();
    uint32_t curr = GetSpellPoints();

    // spell points may be doubled in artesian spring, leave as is
    if ( curr >= maxp ) {
        return;
    }

    const Castle * castle = inCastle();

    // in castle?
    if ( castle && castle->GetLevelMageGuild() ) {
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
        dstIdx = path.GetDestinationIndex();
    }

    if ( !path.isValidForMovement() ) {
        path.Reset();
    }

    if ( dstIdx < 0 ) {
        return;
    }

    path.setPath( world.getPath( *this, dstIdx ) );

    if ( !path.isValidForMovement() ) {
        path.Reset();
    }
}

const Castle * Heroes::inCastle() const
{
    return inCastleMutable();
}

Castle * Heroes::inCastleMutable() const
{
    if ( GetColor() == Color::NONE ) {
        return nullptr;
    }

    Castle * castle = world.getCastleEntrance( GetCenter() );
    return castle && castle->GetHero() == this ? castle : nullptr;
}

bool Heroes::isVisited( const Maps::Tile & tile, Visit::Type type ) const
{
    const int32_t index = tile.GetIndex();
    const MP2::MapObjectType objectType = tile.getMainObjectType( false );

    if ( Visit::GLOBAL == type ) {
        return GetKingdom().isVisited( index, objectType );
    }

    return visit_object.end() != std::find( visit_object.begin(), visit_object.end(), IndexObject( index, objectType ) );
}

bool Heroes::isObjectTypeVisited( const MP2::MapObjectType objectType, Visit::Type type ) const
{
    if ( Visit::GLOBAL == type ) {
        return GetKingdom().isVisited( objectType );
    }

    return std::any_of( visit_object.begin(), visit_object.end(), [objectType]( const IndexObject & v ) { return v.isObject( objectType ); } );
}

void Heroes::SetVisited( int32_t index, Visit::Type type )
{
    const Maps::Tile & tile = world.getTile( index );
    const MP2::MapObjectType objectType = tile.getMainObjectType( false );

    if ( Visit::GLOBAL == type ) {
        GetKingdom().SetVisited( index, objectType );
    }
    else if ( !isVisited( tile ) && MP2::OBJ_NONE != objectType ) {
        visit_object.emplace_front( index, objectType );
    }
}

void Heroes::setVisitedForAllies( const int32_t tileIndex ) const
{
    const Maps::Tile & tile = world.getTile( tileIndex );
    const MP2::MapObjectType objectType = tile.getMainObjectType( false );

    // Set visited to all allies as well.
    const Colors friendColors( Players::GetPlayerFriends( GetColor() ) );
    for ( const int friendColor : friendColors ) {
        world.GetKingdom( friendColor ).SetVisited( tileIndex, objectType );
    }
}

void Heroes::SetVisitedWideTile( int32_t index, const MP2::MapObjectType objectType, Visit::Type type )
{
    const Maps::Tile & tile = world.getTile( index );
    const uint32_t uid = tile.getMainObjectPart()._uid;
    int wide = 0;

    switch ( objectType ) {
    case MP2::OBJ_SKELETON:
    case MP2::OBJ_OASIS:
    case MP2::OBJ_STANDING_STONES:
    case MP2::OBJ_ARTESIAN_SPRING:
        wide = 2;
        break;
    case MP2::OBJ_WATERING_HOLE:
        wide = 4;
        break;
    default:
        break;
    }

    if ( tile.getMainObjectType( false ) == objectType && wide ) {
        for ( int32_t ii = tile.GetIndex() - ( wide - 1 ); ii <= tile.GetIndex() + ( wide - 1 ); ++ii )
            if ( Maps::isValidAbsIndex( ii ) && world.getTile( ii ).getMainObjectPart()._uid == uid )
                SetVisited( ii, type );
    }
}

void Heroes::markHeroMeeting( int heroID )
{
    if ( isValidId( heroID ) && !hasMetWithHero( heroID ) ) {
        visit_object.emplace_front( heroID, MP2::OBJ_HERO );
    }
}

void Heroes::unmarkHeroMeeting()
{
    const VecHeroes & heroes = GetKingdom().GetHeroes();
    for ( Heroes * hero : heroes ) {
        if ( hero == nullptr || hero == this ) {
            continue;
        }

        hero->visit_object.remove( IndexObject( _id, MP2::OBJ_HERO ) );
        visit_object.remove( IndexObject( hero->_id, MP2::OBJ_HERO ) );
    }
}

bool Heroes::hasMetWithHero( int heroID ) const
{
    return visit_object.end() != std::find( visit_object.begin(), visit_object.end(), IndexObject( heroID, MP2::OBJ_HERO ) );
}

bool Heroes::isLosingGame() const
{
    return GetKingdom().isLosingGame();
}

bool Heroes::isAction() const
{
    return Modes( ACTION );
}

void Heroes::ResetAction()
{
    ResetModes( ACTION );
}

bool Heroes::PickupArtifact( const Artifact & art )
{
    if ( !art.isValid() ) {
        return false;
    }

    if ( !bag_artifacts.PushArtifact( art ) ) {
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

    const auto assembledArtifacts = bag_artifacts.assembleArtifactSetIfPossible();

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
    int oldLevel = GetLevelFromExperience( experience );
    int newLevel = GetLevelFromExperience( experience + amount );

    const uint32_t updatedExperience = experience + amount;

    for ( int level = oldLevel; level < newLevel - 1; ++level ) {
        experience = GetExperienceFromLevel( level );
        LevelUp( false, autoselect );
    }

    experience = updatedExperience;

    if ( newLevel > oldLevel ) {
        LevelUp( false, autoselect );
    }
}

int Heroes::GetLevelFromExperience( uint32_t exp )
{
    for ( int lvl = 1; lvl < 255; ++lvl )
        if ( exp < GetExperienceFromLevel( lvl ) )
            return lvl;

    return 0;
}

uint32_t Heroes::GetExperienceFromLevel( int lvl )
{
    switch ( lvl ) {
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

    const uint32_t l1 = GetExperienceFromLevel( lvl - 1 );
    return ( l1 + static_cast<uint32_t>( round( ( l1 - GetExperienceFromLevel( lvl - 2 ) ) * 1.2 / 100 ) * 100 ) );
}

bool Heroes::BuySpellBook( const Castle * castle )
{
    if ( HaveSpellBook() || Color::NONE == GetColor() ) {
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

        if ( castle ) {
            castle->MageGuildEducateHero( *this );
        }

        return true;
    }

    return false;
}

bool Heroes::isMoveEnabled() const
{
    return Modes( ENABLEMOVE ) && path.isValidForMovement() && path.hasAllowedSteps();
}

bool Heroes::CanMove() const
{
    const Maps::Tile & tile = world.getTile( GetIndex() );
    return move_point >= ( tile.isRoad() ? Maps::Ground::roadPenalty : Maps::Ground::GetPenalty( tile, GetLevelSkill( Skill::Secondary::PATHFINDING ) ) );
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
    switch ( direction ) {
    case Direction::TOP:
        sprite_index = 0;
        break;
    case Direction::BOTTOM:
        sprite_index = 36;
        break;
    case Direction::TOP_RIGHT:
    case Direction::TOP_LEFT:
        sprite_index = 9;
        break;
    case Direction::BOTTOM_RIGHT:
    case Direction::BOTTOM_LEFT:
        sprite_index = 27;
        break;
    case Direction::RIGHT:
    case Direction::LEFT:
        sprite_index = 18;
        break;
    default:
        break;
    }
}

bool Heroes::isShipMaster() const
{
    return Modes( SHIPMASTER );
}

void Heroes::SetShipMaster( bool f )
{
    f ? SetModes( SHIPMASTER ) : ResetModes( SHIPMASTER );
}

bool Heroes::HasSecondarySkill( int skill ) const
{
    return Skill::Level::NONE != secondary_skills.GetLevel( skill );
}

uint32_t Heroes::GetSecondarySkillValue( int skill ) const
{
    return secondary_skills.GetValue( skill );
}

bool Heroes::HasMaxSecondarySkill() const
{
    return maxNumOfSecSkills <= secondary_skills.Count();
}

int Heroes::GetLevelSkill( int skill ) const
{
    return secondary_skills.GetLevel( skill );
}

void Heroes::LearnSkill( const Skill::Secondary & skill )
{
    if ( skill.isValid() )
        secondary_skills.AddSkill( skill );
}

void Heroes::Scout( const int tileIndex ) const
{
    // We should not scout for the NONE color player.
    assert( GetColor() != Color::NONE );

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

int Heroes::GetScoutingDistance() const
{
    return static_cast<int>( GetBagArtifacts().getTotalArtifactEffectValue( fheroes2::ArtifactBonusType::AREA_REVEAL_DISTANCE )
                             + GameStatic::getFogDiscoveryDistance( GameStatic::FogDiscoveryType::HEROES ) + GetSecondarySkillValue( Skill::Secondary::SCOUTING ) );
}

fheroes2::Rect Heroes::GetScoutRoi() const
{
    const int32_t scoutRange = GetScoutingDistance();
    const fheroes2::Point heroPosition = GetCenter();

    return { heroPosition.x - scoutRange, heroPosition.y - scoutRange, 2 * scoutRange + 1, 2 * scoutRange + 1 };
}

uint32_t Heroes::UpdateMovementPoints( const uint32_t movePoints, const int skill ) const
{
    const int level = GetLevelSkill( skill );
    if ( level == Skill::Level::NONE )
        return movePoints;

    const uint32_t skillValue = GetSecondarySkillValue( skill );

    if ( skillValue == 33 ) {
        return movePoints * 4 / 3;
    }
    else if ( skillValue == 66 ) {
        return movePoints * 5 / 3;
    }

    return movePoints + skillValue * movePoints / 100;
}

uint32_t Heroes::GetVisionsDistance() const
{
    return 8;
}

int Heroes::getNumOfTravelDays( int32_t dstIdx ) const
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

void Heroes::LevelUp( bool skipsecondary, bool autoselect )
{
    const HeroSeedsForLevelUp seeds = GetSeedsForLevelUp();

    // level up primary skill
    const int primarySkill = Skill::Primary::LevelUp( _race, GetLevel(), seeds.seedPrimarySkill );

    DEBUG_LOG( DBG_GAME, DBG_INFO, "for " << GetName() << ", up " << Skill::Primary::String( primarySkill ) )

    if ( !skipsecondary ) {
        LevelUpSecondarySkill( seeds, primarySkill, autoselect );
    }
}

void Heroes::LevelUpSecondarySkill( const HeroSeedsForLevelUp & seeds, int primary, bool autoselect )
{
    const auto [sec1, sec2] = secondary_skills.FindSkillsForLevelUp( _race, seeds.seedSecondarySkill1, seeds.seedSecondarySkill2 );

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
        const int result = Dialog::LevelUpSelectSkill( name, primary, sec1, sec2, *this );

        if ( Skill::Secondary::UNKNOWN != result ) {
            selected = ( result == sec2.Skill() ) ? sec2 : sec1;
        }
    }

    if ( selected.isValid() ) {
        DEBUG_LOG( DBG_GAME, DBG_INFO, GetName() << ", selected: " << Skill::Secondary::String( selected.Skill() ) )
        Skill::Secondary * secs = secondary_skills.FindSkill( selected.Skill() );

        if ( secs ) {
            secs->NextLevel();
        }
        else {
            secondary_skills.AddSkill( Skill::Secondary( selected.Skill(), Skill::Level::BASIC ) );
        }

        // Campaign-only heroes get additional experience immediately upon their creation, even while still neutral.
        // We should not try to scout the area around such heroes.
        if ( selected.Skill() == Skill::Secondary::SCOUTING && GetColor() != Color::NONE ) {
            Scout( GetIndex() );
            if ( isControlHuman() ) {
                ScoutRadar();
            }
        }
    }
}

void Heroes::ApplyPenaltyMovement( uint32_t penalty )
{
    if ( move_point >= penalty )
        move_point -= penalty;
    else
        move_point = 0;
}

bool Heroes::MayStillMove( const bool ignorePath, const bool ignoreSleeper ) const
{
    if ( !isActive() ) {
        return false;
    }

    if ( !ignoreSleeper && Modes( SLEEPER ) ) {
        return false;
    }

    if ( !ignorePath && path.isValidForMovement() ) {
        return path.hasAllowedSteps();
    }

    return CanMove();
}

bool Heroes::MayCastAdventureSpells() const
{
    return isValid() && GetColor() != Color::NONE;
}

bool Heroes::isValid() const
{
    return isValidId( _id );
}

bool Heroes::isActive() const
{
    return isValid() && ( GetColor() & Color::ALL ) && !Modes( JAIL );
}

bool Heroes::isAvailableForHire() const
{
    return isValid() && GetColor() == Color::NONE && !Modes( JAIL );
}

void Heroes::Dismiss( int reason )
{
    if ( isAvailableForHire() ) {
        return;
    }

    // if not surrendering, reset army
    if ( ( reason & Battle::RESULT_SURRENDER ) == 0 ) {
        army.Reset( true );
    }

    const int heroColor = GetColor();
    Kingdom & kingdom = GetKingdom();

    if ( heroColor != Color::NONE ) {
        kingdom.RemoveHero( this );
    }
    SetColor( Color::NONE );

    world.getTile( GetIndex() ).setHero( nullptr );
    SetIndex( -1 );

    modes = 0;

    path.Hide();
    path.Reset();

    SetMove( false );

    SetModes( ACTION );

    if ( ( Battle::RESULT_RETREAT | Battle::RESULT_SURRENDER ) & reason ) {
        SetModes( SAVEMP );

        if ( heroColor != Color::NONE ) {
            kingdom.appendSurrenderedHero( *this );
        }
    }
}

int Heroes::GetControl() const
{
    return GetKingdom().GetControl();
}

uint32_t Heroes::GetStartingXp()
{
    return Rand::Get( 40, 90 );
}

void Heroes::ActionPreBattle()
{
    spell_book.resetState();
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
            MapsIndexes::const_iterator it = std::find( targets.begin(), targets.end(), GetPath().GetDestinationIndex() );

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

void Heroes::Move2Dest( const int32_t dstIndex )
{
    const int32_t currentIndex = GetIndex();

    if ( dstIndex == currentIndex ) {
        return;
    }

    world.getTile( currentIndex ).setHero( nullptr );
    SetIndex( dstIndex );
    world.getTile( dstIndex ).setHero( this );
}

const fheroes2::Sprite & Heroes::GetPortrait( int id, int type )
{
    if ( isValidId( id ) )
        switch ( type ) {
        case PORT_BIG:
            return fheroes2::AGG::GetICN( ICN::getHeroPortraitIcnId( id ), 0 );
        case PORT_MEDIUM: {
            // Original ICN::PORTMEDI sprites are badly rendered. Instead of them we're getting high quality ICN:PORT00xx file and resize it to a smaller image.
            // TODO: find a better way to store these images, ideally in agg_image.cpp file.
            static std::map<int, fheroes2::Sprite> mediumSizePortrait;
            auto iter = mediumSizePortrait.find( id );
            if ( iter != mediumSizePortrait.end() ) {
                return iter->second;
            }

            const fheroes2::Sprite & original = fheroes2::AGG::GetICN( ICN::getHeroPortraitIcnId( id ), 0 );
            fheroes2::Sprite output( 50, 47 );
            fheroes2::Resize( original, output );

            return mediumSizePortrait.try_emplace( id, std::move( output ) ).first->second;
        }
        case PORT_SMALL:
            if ( id == Heroes::DEBUG_HERO ) {
                return fheroes2::AGG::GetICN( ICN::MINIPORT, BRAX - 1 );
            }

            // Since hero IDs start from 1 we have to deduct 1 from the ID.
            return fheroes2::AGG::GetICN( ICN::MINIPORT, id - 1 );
        default:
            break;
        }

    return fheroes2::AGG::GetICN( ICN::UNKNOWN, 0 );
}

void Heroes::PortraitRedraw( const int32_t px, const int32_t py, const PortraitType type, fheroes2::Image & dstsf ) const
{
    const fheroes2::Sprite & port = GetPortrait( portrait, type );
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
            const fheroes2::Sprite & mana = fheroes2::AGG::GetICN( ICN::MANA, GetManaIndexSprite() );

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

    os << "name            : " << name << " (" << Race::String( _race ) << ")" << std::endl
       << "color           : " << Color::String( GetColor() ) << std::endl
       << "experience      : " << experience << std::endl
       << "level           : " << GetLevel() << std::endl
       << "magic points    : " << GetSpellPoints() << " / " << GetMaxSpellPoints() << std::endl
       << "position x, y   : " << GetCenter().x << ", " << GetCenter().y << std::endl
       << "move points     : " << move_point << " / " << GetMaxMovePoints() << std::endl
       << "direction       : " << Direction::String( direction ) << std::endl
       << "index sprite    : " << sprite_index << std::endl
       << "in castle       : " << ( inCastle() ? "true" : "false" ) << std::endl
       << "save object     : " << MP2::StringObject( world.getTile( GetIndex() ).getMainObjectType( false ) ) << std::endl
       << "flags           : " << ( Modes( SHIPMASTER ) ? "SHIPMASTER," : "" ) << ( Modes( CUSTOM ) ? "CUSTOM," : "" ) << ( Modes( PATROL ) ? "PATROL" : "" )
       << std::endl;

    if ( Modes( PATROL ) ) {
        os << "patrol zone     : center: (" << _patrolCenter.x << ", " << _patrolCenter.y << "), distance " << _patrolDistance << std::endl;
    }

    if ( !visit_object.empty() ) {
        os << "visit objects   : ";
        for ( const auto & info : visit_object ) {
            os << MP2::StringObject( info.second ) << "(" << info.first << "), ";
        }

        os << std::endl;
    }

    if ( isControlAI() ) {
        os << "skills          : " << secondary_skills.String() << std::endl
           << "artifacts       : " << bag_artifacts.String() << std::endl
           << "spell book      : " << ( HaveSpellBook() ? spell_book.String() : "disabled" ) << std::endl
           << "army dump       : " << army.String() << std::endl
           << "ai role         : " << GetHeroRoleString( *this ) << std::endl;
    }

    return os.str();
}

Heroes::HeroSeedsForLevelUp Heroes::GetSeedsForLevelUp() const
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
    fheroes2::hashCombine( hash, _id );
    fheroes2::hashCombine( hash, _race );
    fheroes2::hashCombine( hash, attack );
    fheroes2::hashCombine( hash, defense );
    fheroes2::hashCombine( hash, power );
    fheroes2::hashCombine( hash, knowledge );
    for ( int skillId = Skill::Secondary::PATHFINDING; skillId <= Skill::Secondary::ESTATES; ++skillId ) {
        fheroes2::hashCombine( hash, GetLevelSkill( skillId ) );
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

Heroes * AllHeroes::Get( const int hid ) const
{
    if ( !Heroes::isValidId( hid ) ) {
        return nullptr;
    }

    assert( hid >= 0 && static_cast<size_t>( hid ) < _heroes.size() && _heroes[hid] );

    return _heroes[hid].get();
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

void AllHeroes::Scout( int colors ) const
{
    for ( const Heroes * hero : *this ) {
        assert( hero != nullptr );

        if ( !( hero->GetColor() & colors ) ) {
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

void AllHeroes::NewMonth() const
{
    std::for_each( begin(), end(), []( Heroes * hero ) {
        assert( hero != nullptr );

        hero->ActionNewMonth();
    } );
}

Heroes * AllHeroes::FromJail( int32_t index ) const
{
    for ( Heroes * hero : *this ) {
        assert( hero != nullptr );

        if ( hero->Modes( Heroes::JAIL ) && index == hero->GetIndex() ) {
            return hero;
        }
    }

    return nullptr;
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
    return stream << hero.name << col << hero.experience << hero.secondary_skills << hero.army << hero._id << hero.portrait << hero._race << hero._objectTypeUnderHero
                  << hero.path << hero.direction << hero.sprite_index << hero._patrolCenter << hero._patrolDistance << hero.visit_object << hero._lastGroundRegion;
}

IStreamBase & operator>>( IStreamBase & stream, Heroes & hero )
{
    HeroBase & base = hero;
    ColorBase & col = hero;

    // HeroBase
    stream >> base;

    // Heroes
    stream >> hero.name >> col >> hero.experience >> hero.secondary_skills >> hero.army >> hero._id >> hero.portrait >> hero._race;

    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_PRE1_1100_RELEASE, "Remove the logic below." );
    if ( Game::GetVersionOfCurrentSaveFile() < FORMAT_VERSION_PRE1_1100_RELEASE ) {
        // Before FORMAT_VERSION_PRE1_1100_RELEASE we did not check that a custom hero name is empty set inside the original map.
        // This leads to assertion rise while rendering text. Also, it is incorrect to have a hero with no name.
        if ( hero.name.empty() ) {
            hero.name = Heroes::GetName( hero._id );
        }
    }

    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_1010_RELEASE, "Remove the logic below." );
    if ( Game::GetVersionOfCurrentSaveFile() < FORMAT_VERSION_1010_RELEASE ) {
        // Before FORMAT_VERSION_1010_RELEASE Heroes::UNKNOWN was 72.
        if ( hero._id == 72 ) {
            hero._id = Heroes::UNKNOWN;
            hero.portrait = Heroes::UNKNOWN;
        }
        else {
            ++hero._id;
            ++hero.portrait;
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

    stream >> hero.path >> hero.direction >> hero.sprite_index;

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

    stream >> hero._patrolDistance >> hero.visit_object >> hero._lastGroundRegion;

    hero.army.SetCommander( &hero );

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
