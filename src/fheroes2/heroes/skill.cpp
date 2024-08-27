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

#include "skill.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <unordered_set>

#include "artifact.h"
#include "artifact_info.h"
#include "game_static.h"
#include "gamedefs.h"
#include "heroes.h"
#include "heroes_base.h"
#include "kingdom.h"
#include "race.h"
#include "rand.h"
#include "serialize.h"
#include "skill_static.h"
#include "tools.h"
#include "translations.h"
#include "world.h"

namespace
{
    constexpr std::array<int, MAXSECONDARYSKILL> allSecondarySkills{ Skill::Secondary::PATHFINDING, Skill::Secondary::ARCHERY,    Skill::Secondary::LOGISTICS,
                                                                     Skill::Secondary::SCOUTING,    Skill::Secondary::DIPLOMACY,  Skill::Secondary::NAVIGATION,
                                                                     Skill::Secondary::LEADERSHIP,  Skill::Secondary::WISDOM,     Skill::Secondary::MYSTICISM,
                                                                     Skill::Secondary::LUCK,        Skill::Secondary::BALLISTICS, Skill::Secondary::EAGLE_EYE,
                                                                     Skill::Secondary::NECROMANCY,  Skill::Secondary::ESTATES };
    static_assert( !allSecondarySkills.empty() && allSecondarySkills.back() != 0, "All existing secondary skills must be present in this array" );

    int SecondaryGetWeightSkillFromRace( const int race, const int skill )
    {
        const Skill::stats_t * ptr = GameStatic::GetSkillStats( race );
        if ( ptr == nullptr ) {
            return 0;
        }

        switch ( skill ) {
        case Skill::Secondary::PATHFINDING:
            return ptr->mature_secondary.pathfinding;
        case Skill::Secondary::ARCHERY:
            return ptr->mature_secondary.archery;
        case Skill::Secondary::LOGISTICS:
            return ptr->mature_secondary.logistics;
        case Skill::Secondary::SCOUTING:
            return ptr->mature_secondary.scouting;
        case Skill::Secondary::DIPLOMACY:
            return ptr->mature_secondary.diplomacy;
        case Skill::Secondary::NAVIGATION:
            return ptr->mature_secondary.navigation;
        case Skill::Secondary::LEADERSHIP:
            return ptr->mature_secondary.leadership;
        case Skill::Secondary::WISDOM:
            return ptr->mature_secondary.wisdom;
        case Skill::Secondary::MYSTICISM:
            return ptr->mature_secondary.mysticism;
        case Skill::Secondary::LUCK:
            return ptr->mature_secondary.luck;
        case Skill::Secondary::BALLISTICS:
            return ptr->mature_secondary.ballistics;
        case Skill::Secondary::EAGLE_EYE:
            return ptr->mature_secondary.eagleeye;
        case Skill::Secondary::NECROMANCY:
            return ptr->mature_secondary.necromancy;
        case Skill::Secondary::ESTATES:
            return ptr->mature_secondary.estates;
        default:
            assert( 0 );
            break;
        }

        return 0;
    }

    int SecondaryPriorityFromRace( const int race, const std::unordered_set<int> & blacklist, const uint32_t seed )
    {
        Rand::Queue parts( MAXSECONDARYSKILL );

        for ( auto skill : allSecondarySkills ) {
            if ( blacklist.find( skill ) != blacklist.end() ) {
                continue;
            }

            parts.Push( skill, SecondaryGetWeightSkillFromRace( race, skill ) );
        }

        if ( parts.Size() == 0 ) {
            return Skill::Secondary::UNKNOWN;
        }

        return parts.GetWithSeed( seed );
    }
}

uint32_t Skill::Secondary::GetValues() const
{
    const values_t * val = GameStatic::GetSkillValues( Skill() );

    if ( val )
        switch ( Level() ) {
        case Level::BASIC:
            return val->values.basic;
        case Level::ADVANCED:
            return val->values.advanced;
        case Level::EXPERT:
            return val->values.expert;
        default:
            break;
        }

    return 0;
}

Skill::Primary::Primary()
    : attack( 0 )
    , defense( 0 )
    , power( 0 )
    , knowledge( 0 )
{}

void Skill::Primary::LoadDefaults( int type, int race )
{
    const stats_t * ptr = GameStatic::GetSkillStats( race );

    if ( ptr )
        switch ( type ) {
        case HeroBase::CAPTAIN:
            attack = ptr->captain_primary.attack;
            defense = ptr->captain_primary.defense;
            power = ptr->captain_primary.power;
            knowledge = ptr->captain_primary.knowledge;
            break;

        case HeroBase::HEROES:
            attack = ptr->initial_primary.attack;
            defense = ptr->initial_primary.defense;
            power = ptr->initial_primary.power;
            knowledge = ptr->initial_primary.knowledge;
            break;

        default:
            break;
        }
}

int Skill::Primary::GetInitialSpell( int race )
{
    const stats_t * ptr = GameStatic::GetSkillStats( race );
    return ptr ? ptr->initial_spell : 0;
}

int Skill::Primary::getHeroDefaultSkillValue( const int skill, const int race )
{
    if ( const stats_t * ptr = GameStatic::GetSkillStats( race ) ) {
        switch ( skill ) {
        case ATTACK:
            return ptr->initial_primary.attack;
        case DEFENSE:
            return ptr->initial_primary.defense;
        case POWER:
            return ptr->initial_primary.power;
        case KNOWLEDGE:
            return ptr->initial_primary.knowledge;
        default:
            // Are you sure that you are passing the correct skill type?
            assert( 0 );
            break;
        }
    }

    return skill == POWER ? 1 : 0;
}

int Skill::Primary::LevelUp( int race, int level, uint32_t seed )
{
    Rand::Queue percents( MAXPRIMARYSKILL );

    const stats_t * ptr = GameStatic::GetSkillStats( race );
    if ( ptr ) {
        if ( ptr->over_level > level ) {
            percents.Push( ATTACK, ptr->mature_primary_under.attack );
            percents.Push( DEFENSE, ptr->mature_primary_under.defense );
            percents.Push( POWER, ptr->mature_primary_under.power );
            percents.Push( KNOWLEDGE, ptr->mature_primary_under.knowledge );
        }
        else {
            percents.Push( ATTACK, ptr->mature_primary_over.attack );
            percents.Push( DEFENSE, ptr->mature_primary_over.defense );
            percents.Push( POWER, ptr->mature_primary_over.power );
            percents.Push( KNOWLEDGE, ptr->mature_primary_over.knowledge );
        }
    }

    int result = percents.Size() ? percents.GetWithSeed( seed ) : UNKNOWN;

    switch ( result ) {
    case ATTACK:
        ++attack;
        break;
    case DEFENSE:
        ++defense;
        break;
    case POWER:
        ++power;
        break;
    case KNOWLEDGE:
        ++knowledge;
        break;
    default:
        break;
    }

    return result;
}

const char * Skill::Primary::String( const int skillType )
{
    switch ( skillType ) {
    case ATTACK:
        return _( "Attack Skill" );
    case DEFENSE:
        return _( "Defense Skill" );
    case POWER:
        return _( "Spell Power" );
    case KNOWLEDGE:
        return _( "Knowledge" );
    default:
        // Are you sure that you are passing the correct skill type?
        assert( 0 );
        break;
    }

    return "Unknown";
}

std::string Skill::Primary::StringDescription( int skill, const Heroes * hero )
{
    std::string res;
    std::string ext;

    switch ( skill ) {
    case ATTACK:
        res = _( "The hero's attack skill is a bonus added to each unit's attack skill." );
        if ( hero )
            hero->GetAttack( &ext );
        break;
    case DEFENSE:
        res = _( "The hero's defense skill is a bonus added to each unit's defense skill." );
        if ( hero )
            hero->GetDefense( &ext );
        break;
    case POWER:
        res = _( "The hero's spell power determines the duration or power of a spell." );
        if ( hero )
            hero->GetPower( &ext );
        break;
    case KNOWLEDGE:
        res = _(
            "The hero's knowledge determines how many spell points the hero may have. Under normal circumstances, a hero is limited to 10 spell points per level of knowledge." );
        if ( hero )
            hero->GetKnowledge( &ext );
        break;
    default:
        // Are you sure that you are passing the correct skill type?
        assert( 0 );
        break;
    }

    if ( !ext.empty() ) {
        res.append( "\n\n" );
        res.append( _( "Current Modifiers:" ) );
        res.append( "\n\n" );
        res.append( ext );
    }

    return res;
}

const char * Skill::Level::String( int level )
{
    switch ( level ) {
    case BASIC:
        return _( "skill|Basic" );
    case ADVANCED:
        return _( "skill|Advanced" );
    case EXPERT:
        return _( "skill|Expert" );
    default:
        // Are you sure that you are passing the correct skill level?
        assert( 0 );
        break;
    }

    return "None";
}

std::string Skill::Level::StringWithBonus( const Heroes & hero, const Secondary & skill )
{
    std::string levelStr = String( skill.Level() );
    if ( skill.Skill() == Skill::Secondary::NECROMANCY && Skill::GetNecromancyBonus( hero ) > 0 ) {
        return levelStr + "+" + std::to_string( Skill::GetNecromancyBonus( hero ) );
    }
    return levelStr;
}

Skill::Secondary::Secondary()
    : std::pair<int, int>( UNKNOWN, Level::NONE )
{}

Skill::Secondary::Secondary( int skill, int level )
    : std::pair<int, int>( UNKNOWN, Level::NONE )
{
    SetSkill( skill );
    SetLevel( level );
}

void Skill::Secondary::Reset()
{
    first = UNKNOWN;
    second = Level::NONE;
}

void Skill::Secondary::Set( const Secondary & skill )
{
    first = skill.first;
    second = skill.second;
}

void Skill::Secondary::SetSkill( int skill )
{
    first = ( skill >= UNKNOWN && skill <= ESTATES ) ? skill : UNKNOWN;
}

void Skill::Secondary::SetLevel( int level )
{
    second = ( level >= Level::NONE && level <= Level::EXPERT ) ? level : Level::NONE;
}

void Skill::Secondary::NextLevel()
{
    switch ( second ) {
    case Level::NONE:
        second = Level::BASIC;
        break;
    case Level::BASIC:
        second = Level::ADVANCED;
        break;
    case Level::ADVANCED:
        second = Level::EXPERT;
        break;
    default:
        break;
    }
}

bool Skill::Secondary::isValid() const
{
    return Skill() != UNKNOWN && Level() != Level::NONE;
}

int Skill::Secondary::RandForWitchsHut()
{
    const Skill::secondary_t * sec = GameStatic::GetSkillForWitchsHut();
    std::vector<int> v;

    if ( sec ) {
        v.reserve( 14 );

        if ( sec->archery )
            v.push_back( ARCHERY );
        if ( sec->ballistics )
            v.push_back( BALLISTICS );
        if ( sec->diplomacy )
            v.push_back( DIPLOMACY );
        if ( sec->eagleeye )
            v.push_back( EAGLE_EYE );
        if ( sec->estates )
            v.push_back( ESTATES );
        if ( sec->leadership )
            v.push_back( LEADERSHIP );
        if ( sec->logistics )
            v.push_back( LOGISTICS );
        if ( sec->luck )
            v.push_back( LUCK );
        if ( sec->mysticism )
            v.push_back( MYSTICISM );
        if ( sec->navigation )
            v.push_back( NAVIGATION );
        if ( sec->necromancy )
            v.push_back( NECROMANCY );
        if ( sec->pathfinding )
            v.push_back( PATHFINDING );
        if ( sec->scouting )
            v.push_back( SCOUTING );
        if ( sec->wisdom )
            v.push_back( WISDOM );
    }

    return v.empty() ? UNKNOWN : Rand::Get( v );
}

int Skill::Secondary::GetIndexSprite1() const
{
    return ( Skill() > UNKNOWN && Skill() <= ESTATES ) ? Skill() : 0;
}

int Skill::Secondary::GetIndexSprite2() const
{
    return ( Skill() > UNKNOWN && Skill() <= ESTATES ) ? Skill() - 1 : 0xFF;
}

const char * Skill::Secondary::String( int skill )
{
    switch ( skill ) {
    case PATHFINDING:
        return _( "Pathfinding" );
    case ARCHERY:
        return _( "Archery" );
    case LOGISTICS:
        return _( "Logistics" );
    case SCOUTING:
        return _( "Scouting" );
    case DIPLOMACY:
        return _( "Diplomacy" );
    case NAVIGATION:
        return _( "Navigation" );
    case LEADERSHIP:
        return _( "Leadership" );
    case WISDOM:
        return _( "Wisdom" );
    case MYSTICISM:
        return _( "Mysticism" );
    case LUCK:
        return _( "Luck" );
    case BALLISTICS:
        return _( "Ballistics" );
    case EAGLE_EYE:
        return _( "Eagle Eye" );
    case NECROMANCY:
        return _( "Necromancy" );
    case ESTATES:
        return _( "Estates" );
    default:
        // Are you sure that you are passing the correct secondary skill type?
        assert( 0 );
        break;
    }

    return "Unknown";
}

std::string Skill::Secondary::GetName() const
{
    const char * name_skill[]
        = { _( "Basic Pathfinding" ),  _( "Advanced Pathfinding" ), _( "Expert Pathfinding" ),  _( "Basic Archery" ),      _( "Advanced Archery" ),
            _( "Expert Archery" ),     _( "Basic Logistics" ),      _( "Advanced Logistics" ),  _( "Expert Logistics" ),   _( "Basic Scouting" ),
            _( "Advanced Scouting" ),  _( "Expert Scouting" ),      _( "Basic Diplomacy" ),     _( "Advanced Diplomacy" ), _( "Expert Diplomacy" ),
            _( "Basic Navigation" ),   _( "Advanced Navigation" ),  _( "Expert Navigation" ),   _( "Basic Leadership" ),   _( "Advanced Leadership" ),
            _( "Expert Leadership" ),  _( "Basic Wisdom" ),         _( "Advanced Wisdom" ),     _( "Expert Wisdom" ),      _( "Basic Mysticism" ),
            _( "Advanced Mysticism" ), _( "Expert Mysticism" ),     _( "Basic Luck" ),          _( "Advanced Luck" ),      _( "Expert Luck" ),
            _( "Basic Ballistics" ),   _( "Advanced Ballistics" ),  _( "Expert Ballistics" ),   _( "Basic Eagle Eye" ),    _( "Advanced Eagle Eye" ),
            _( "Expert Eagle Eye" ),   _( "Basic Necromancy" ),     _( "Advanced Necromancy" ), _( "Expert Necromancy" ),  _( "Basic Estates" ),
            _( "Advanced Estates" ),   _( "Expert Estates" ) };

    return isValid() ? name_skill[( Level() - 1 ) + ( Skill() - 1 ) * 3] : "unknown";
}

std::string Skill::Secondary::GetNameWithBonus( const Heroes & hero ) const
{
    if ( Skill() == NECROMANCY && Skill::GetNecromancyBonus( hero ) > 0 ) {
        return GetName() + " (+" + std::to_string( Skill::GetNecromancyBonus( hero ) ) + ')';
    }
    return GetName();
}

std::string Skill::Secondary::GetDescription( const Heroes & hero ) const
{
    uint32_t count = GetValues();
    std::string name = GetName();
    std::string str = "unknown";

    switch ( Skill() ) {
    case PATHFINDING:
        switch ( Level() ) {
        case Level::BASIC:
        case Level::ADVANCED:
            str = _( "%{skill} reduces the movement penalty for rough terrain by %{count} percent." );
            break;
        case Level::EXPERT:
            str = _( "%{skill} eliminates the movement penalty for rough terrain." );
            break;
        default:
            break;
        }
        break;
    case ARCHERY: {
        str = _(
            "%{skill} increases the damage done by the hero's range attacking creatures by %{count} percent, and eliminates the %{penalty} percent penalty when shooting past obstacles (e.g. castle walls)." );
        StringReplace( str, "%{penalty}", GameStatic::getCastleWallRangedPenalty() );
        break;
    }
    case LOGISTICS: {
        str = _( "%{skill} increases the hero's movement points by %{count} percent." );
        break;
    }
    case SCOUTING: {
        str = _n( "%{skill} increases the hero's viewable area by one square.", "%{skill} increases the hero's viewable area by %{count} squares.", count );
        break;
    }
    case DIPLOMACY: {
        str = _( "%{skill} allows the hero to negotiate with monsters who are weaker than their army, and reduces the cost of surrender." );
        str += "\n\n";
        switch ( Level() ) {
        case Level::BASIC:
        case Level::ADVANCED:
            str.append( _( "Approximately %{count} percent of the creatures may offer to join the hero." ) );
            break;
        case Level::EXPERT:
            str.append( _( "All of the creatures may offer to join the hero." ) );
            break;
        default:
            break;
        }
        str += "\n\n";
        str.append( _( "The cost of surrender is reduced to %{percent} percent of the total cost of troops in the army." ) );
        StringReplace( str, "%{percent}", GetDiplomacySurrenderCostDiscount( Level() ) );
        break;
    }
    case NAVIGATION: {
        str = _( "%{skill} increases the hero's movement points over water by %{count} percent." );
        break;
    }
    case LEADERSHIP: {
        str = _( "%{skill} increases the hero's troops morale by %{count}." );
        break;
    }
    case WISDOM: {
        switch ( Level() ) {
        case Level::BASIC:
            str = _( "%{skill} allows the hero to learn third level spells." );
            break;
        case Level::ADVANCED:
            str = _( "%{skill} allows the hero to learn fourth level spells." );
            break;
        case Level::EXPERT:
            str = _( "%{skill} allows the hero to learn fifth level spells." );
            break;
        default:
            break;
        }
        break;
    }
    case MYSTICISM: {
        str = _n( "%{skill} regenerates one additional spell point per day to the hero.", "%{skill} regenerates %{count} additional spell points per day to the hero.",
                  count );
        break;
    }
    case LUCK: {
        str = _( "%{skill} increases the hero's luck by %{count}." );
        break;
    }
    case BALLISTICS: {
        switch ( Level() ) {
        case Level::BASIC:
            str = _( "%{skill} gives the hero's catapult shots a greater chance to hit and do damage to castle walls." );
            break;
        case Level::ADVANCED:
            str = _( "%{skill} gives the hero's catapult an extra shot, and each shot has a greater chance to hit and do damage to castle walls." );
            break;
        case Level::EXPERT:
            str = _( "%{skill} gives the hero's catapult an extra shot, and each shot automatically destroys any wall, except a fortified wall in a Knight castle." );
            break;
        default:
            break;
        }
        break;
    }
    case EAGLE_EYE: {
        switch ( Level() ) {
        case Level::BASIC:
            str = _( "%{skill} gives the hero a %{count} percent chance to learn any given 1st or 2nd level spell that was cast by an enemy during combat." );
            break;
        case Level::ADVANCED:
            str = _( "%{skill} gives the hero a %{count} percent chance to learn any given 3rd level spell (or below) that was cast by an enemy during combat." );
            break;
        case Level::EXPERT:
            str = _( "%{skill} gives the hero a %{count} percent chance to learn any given 4th level spell (or below) that was cast by an enemy during combat." );
            break;
        default:
            break;
        }
        break;
    }
    case NECROMANCY: {
        count += Skill::GetNecromancyPercent( hero ) - hero.GetSecondaryValues( Skill::Secondary::NECROMANCY );
        name = GetNameWithBonus( hero );
        str = _( "%{skill} allows %{count} percent of the creatures killed in combat to be brought back from the dead as Skeletons." );
        break;
    }
    case ESTATES: {
        str = _( "The hero produces %{count} gold pieces per day as tax revenue from estates." );
        break;
    }
    default:
        // Are you sure that you are passing the correct secondary skill type?
        assert( 0 );
        break;
    }

    StringReplace( str, "%{skill}", name );
    StringReplace( str, "%{count}", count );

    return str;
}

Skill::SecSkills::SecSkills()
{
    reserve( HEROESMAXSKILL );
}

Skill::SecSkills::SecSkills( int race )
{
    reserve( HEROESMAXSKILL );

    if ( race & Race::ALL ) {
        const stats_t * ptr = GameStatic::GetSkillStats( race );

        if ( ptr ) {
            if ( ptr->initial_secondary.archery )
                AddSkill( Secondary( Secondary::ARCHERY, ptr->initial_secondary.archery ) );
            if ( ptr->initial_secondary.diplomacy )
                AddSkill( Secondary( Secondary::DIPLOMACY, ptr->initial_secondary.diplomacy ) );
            if ( ptr->initial_secondary.eagleeye )
                AddSkill( Secondary( Secondary::EAGLE_EYE, ptr->initial_secondary.eagleeye ) );
            if ( ptr->initial_secondary.estates )
                AddSkill( Secondary( Secondary::ESTATES, ptr->initial_secondary.estates ) );
            if ( ptr->initial_secondary.logistics )
                AddSkill( Secondary( Secondary::LOGISTICS, ptr->initial_secondary.logistics ) );
            if ( ptr->initial_secondary.luck )
                AddSkill( Secondary( Secondary::LUCK, ptr->initial_secondary.luck ) );
            if ( ptr->initial_secondary.mysticism )
                AddSkill( Secondary( Secondary::MYSTICISM, ptr->initial_secondary.mysticism ) );
            if ( ptr->initial_secondary.pathfinding )
                AddSkill( Secondary( Secondary::PATHFINDING, ptr->initial_secondary.pathfinding ) );
            if ( ptr->initial_secondary.leadership )
                AddSkill( Secondary( Secondary::LEADERSHIP, ptr->initial_secondary.leadership ) );
            if ( ptr->initial_secondary.ballistics )
                AddSkill( Secondary( Secondary::BALLISTICS, ptr->initial_secondary.ballistics ) );
            if ( ptr->initial_secondary.navigation )
                AddSkill( Secondary( Secondary::NAVIGATION, ptr->initial_secondary.navigation ) );
            if ( ptr->initial_secondary.scouting )
                AddSkill( Secondary( Secondary::SCOUTING, ptr->initial_secondary.scouting ) );
            if ( ptr->initial_secondary.necromancy )
                AddSkill( Secondary( Secondary::NECROMANCY, ptr->initial_secondary.necromancy ) );
            if ( ptr->initial_secondary.wisdom )
                AddSkill( Secondary( Secondary::WISDOM, ptr->initial_secondary.wisdom ) );
        }
    }
}

int Skill::SecSkills::GetLevel( int skill ) const
{
    const_iterator it = std::find_if( begin(), end(), [skill]( const Secondary & v ) { return v.isSkill( skill ); } );

    return it == end() ? Level::NONE : ( *it ).Level();
}

uint32_t Skill::SecSkills::GetValues( int skill ) const
{
    const_iterator it = std::find_if( begin(), end(), [skill]( const Secondary & v ) { return v.isSkill( skill ); } );

    return it == end() ? 0 : ( *it ).GetValues();
}

int Skill::SecSkills::Count() const
{
    return static_cast<int>( std::count_if( begin(), end(), []( const Secondary & v ) { return v.isValid(); } ) ); // it's safe to cast as number is small
}

int Skill::SecSkills::GetTotalLevel() const
{
    int result = 0;
    for ( const Skill::Secondary & skill : *this ) {
        if ( skill.isValid() ) {
            result += skill.Level();
        }
    }
    return result;
}

void Skill::SecSkills::AddSkill( const Skill::Secondary & skill )
{
    if ( skill.isValid() ) {
        const int skillValue = skill.Skill();
        iterator it = std::find_if( begin(), end(), [skillValue]( const Secondary & v ) { return v.isSkill( skillValue ); } );
        if ( it != end() )
            ( *it ).SetLevel( skill.Level() );
        else {
            it = std::find_if( begin(), end(), []( const Secondary & v ) { return !v.isValid(); } );
            if ( it != end() )
                ( *it ).Set( skill );
            else if ( size() < HEROESMAXSKILL )
                push_back( skill );
        }
    }
}

Skill::Secondary * Skill::SecSkills::FindSkill( int skill )
{
    iterator it = std::find_if( begin(), end(), [skill]( const Secondary & v ) { return v.isSkill( skill ); } );
    return it != end() ? &( *it ) : nullptr;
}

std::vector<Skill::Secondary> & Skill::SecSkills::ToVector()
{
    std::vector<Secondary> & v = *this;
    return v;
}

const std::vector<Skill::Secondary> & Skill::SecSkills::ToVector() const
{
    const std::vector<Secondary> & v = *this;
    return v;
}

std::string Skill::SecSkills::String() const
{
    std::string output;

    for ( const_iterator it = begin(); it != end(); ++it ) {
        output += it->GetName();
        output += ", ";
    }

    return output;
}

void Skill::SecSkills::FillMax( const Skill::Secondary & skill )
{
    if ( size() < HEROESMAXSKILL )
        resize( HEROESMAXSKILL, skill );
}

std::pair<Skill::Secondary, Skill::Secondary> Skill::SecSkills::FindSkillsForLevelUp( const int race, const uint32_t firstSkillSeed,
                                                                                      uint32_t const secondSkillSeed ) const
{
    std::unordered_set<int> blacklist;
    blacklist.reserve( MAXSECONDARYSKILL + HEROESMAXSKILL );

    for ( const Secondary & skill : *this ) {
        if ( skill.Level() != Level::EXPERT ) {
            continue;
        }

        blacklist.insert( skill.Skill() );
    }

    if ( Count() >= HEROESMAXSKILL ) {
        for ( const int skill : allSecondarySkills ) {
            if ( GetLevel( skill ) != Level::NONE ) {
                continue;
            }

            blacklist.insert( skill );
        }
    }

    // Wisdom should be offered to the heroes of "magic" classes on a mandatory basis at least once every three level-ups, regardless of its probability in accordance
    // with the class parameters
    const bool isWisdomPriority = [this, race, &blacklist = std::as_const( blacklist )]() {
        if ( !Race::isMagicalRace( race ) ) {
            return false;
        }

        if ( GetTotalLevel() % 3 != 0 ) {
            return false;
        }

        if ( blacklist.find( Skill::Secondary::WISDOM ) != blacklist.end() ) {
            return false;
        }

        return true;
    }();

    const auto levelUpSingleSkill = [this]( const int skill ) -> Secondary {
        if ( skill == Secondary::UNKNOWN ) {
            return {};
        }

        Secondary result{ skill, GetLevel( skill ) };
        assert( result.Level() != Level::EXPERT );

        result.NextLevel();

        return result;
    };

    std::pair<Secondary, Secondary> result;

    result.first = levelUpSingleSkill( isWisdomPriority ? Skill::Secondary::WISDOM : SecondaryPriorityFromRace( race, blacklist, firstSkillSeed ) );
    if ( result.first.Skill() == Secondary::UNKNOWN ) {
        return result;
    }

    blacklist.insert( result.first.Skill() );

    result.second = levelUpSingleSkill( SecondaryPriorityFromRace( race, blacklist, secondSkillSeed ) );
    assert( result.first.Skill() != result.second.Skill() );

    return result;
}

void StringAppendModifiers( std::string & str, int value )
{
    if ( value < 0 )
        str.append( " " ); // '-' present
    else if ( value > 0 )
        str.append( " +" );

    str.append( std::to_string( value ) );
}

int Skill::GetLeadershipModifiers( int level, std::string * strs = nullptr )
{
    Secondary skill( Secondary::LEADERSHIP, level );

    if ( skill.GetValues() && strs ) {
        strs->append( skill.GetName() );
        StringAppendModifiers( *strs, skill.GetValues() );
        strs->append( "\n" );
    }

    return skill.GetValues();
}

int Skill::GetLuckModifiers( int level, std::string * strs = nullptr )
{
    Secondary skill( Secondary::LUCK, level );

    if ( skill.GetValues() && strs ) {
        strs->append( skill.GetName() );
        StringAppendModifiers( *strs, skill.GetValues() );
        strs->append( "\n" );
    }

    return skill.GetValues();
}

uint32_t Skill::GetNecromancyBonus( const HeroBase & hero )
{
    const uint32_t shrineCount = world.GetKingdom( hero.GetColor() ).GetCountNecromancyShrineBuild();
    const uint32_t artifactEffect = hero.GetBagArtifacts().isArtifactBonusPresent( fheroes2::ArtifactBonusType::NECROMANCY_SKILL ) ? 1 : 0;
    // cap bonus at 7
    return std::min( 7u, shrineCount + artifactEffect );
}

uint32_t Skill::GetNecromancyPercent( const HeroBase & hero )
{
    uint32_t percent = hero.GetSecondaryValues( Skill::Secondary::NECROMANCY );
    percent += 10 * GetNecromancyBonus( hero );
    // cap at 100% bonus
    return std::min( percent, 100u );
}

uint32_t Skill::GetDiplomacySurrenderCostDiscount( const int level )
{
    switch ( level ) {
    case Level::BASIC:
        return 40;
    case Level::ADVANCED:
        return 30;
    case Level::EXPERT:
        return 20;
    default:
        return 0;
    }
}

StreamBase & Skill::operator<<( StreamBase & msg, const Primary & skill )
{
    return msg << skill.attack << skill.defense << skill.knowledge << skill.power;
}

StreamBase & Skill::operator>>( StreamBase & msg, Primary & skill )
{
    return msg >> skill.attack >> skill.defense >> skill.knowledge >> skill.power;
}

StreamBase & Skill::operator>>( StreamBase & sb, Secondary & st )
{
    return sb >> st.first >> st.second;
}

StreamBase & Skill::operator<<( StreamBase & sb, const SecSkills & ss )
{
    const std::vector<Secondary> & v = ss;
    return sb << v;
}

StreamBase & Skill::operator>>( StreamBase & sb, SecSkills & ss )
{
    std::vector<Secondary> & v = ss;
    sb >> v;

    if ( v.size() > HEROESMAXSKILL )
        v.resize( HEROESMAXSKILL );
    return sb;
}
