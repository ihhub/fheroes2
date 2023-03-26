/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
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
#include <cassert>
#include <iterator>

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

namespace Skill
{
    int SecondaryGetWeightSkillFromRace( int race, int skill );
    int SecondaryPriorityFromRace( int, const std::vector<int> &, uint32_t seed );

    const int secskills[]
        = { Secondary::PATHFINDING, Secondary::ARCHERY,   Secondary::LOGISTICS, Secondary::SCOUTING,   Secondary::DIPLOMACY, Secondary::NAVIGATION, Secondary::LEADERSHIP,
            Secondary::WISDOM,      Secondary::MYSTICISM, Secondary::LUCK,      Secondary::BALLISTICS, Secondary::EAGLEEYE,  Secondary::NECROMANCY, Secondary::ESTATES };
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
        res = _( "Your attack skill is a bonus added to each creature's attack skill." );
        if ( hero )
            hero->GetAttack( &ext );
        break;
    case DEFENSE:
        res = _( "Your defense skill is a bonus added to each creature's defense skill." );
        if ( hero )
            hero->GetDefense( &ext );
        break;
    case POWER:
        res = _( "Your spell power determines the length or power of a spell." );
        if ( hero )
            hero->GetPower( &ext );
        break;
    case KNOWLEDGE:
        res = _(
            "Your knowledge determines how many spell points your hero may have. Under normal circumstances, a hero is limited to 10 spell points per level of knowledge." );
        if ( hero )
            hero->GetKnowledge( &ext );
        break;
    default:
        // Are you sure that you are passing the correct skill type?
        assert( 0 );
        break;
    }

    if ( !ext.empty() ) {
        res.append( "\n \n" );
        res.append( _( "Current Modifiers:" ) );
        res.append( "\n \n" );
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
    const std::string levelStr = String( skill.Level() );
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
            v.push_back( EAGLEEYE );
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
    case EAGLEEYE:
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
        str = _( "%{skill} increases the damage done by range attacking creatures by %{count} percent." );
        break;
    }
    case LOGISTICS: {
        str = _( "%{skill} increases your hero's movement points by %{count} percent." );
        break;
    }
    case SCOUTING: {
        str = _n( "%{skill} increases your hero's viewable area by one square.", "%{skill} increases your hero's viewable area by %{count} squares.", count );
        break;
    }
    case DIPLOMACY:
        str = _( "%{skill} allows you to negotiate with monsters who are weaker than your group. " );
        switch ( Level() ) {
        case Level::BASIC:
        case Level::ADVANCED:
            str.append( _( "Approximately %{count} percent of the creatures may offer to join you." ) );
            break;
        case Level::EXPERT:
            str.append( _( "All of the creatures may offer to join you." ) );
            break;
        default:
            break;
        }
        break;
    case NAVIGATION: {
        str = _( "%{skill} increases your hero's movement points over water by %{count} percent." );
        break;
    }
    case LEADERSHIP: {
        str = _( "%{skill} increases your hero's troops morale by %{count}." );
        break;
    }
    case WISDOM: {
        switch ( Level() ) {
        case Level::BASIC:
            str = _( "%{skill} allows your hero to learn third level spells." );
            break;
        case Level::ADVANCED:
            str = _( "%{skill} allows your hero to learn fourth level spells." );
            break;
        case Level::EXPERT:
            str = _( "%{skill} allows your hero to learn fifth level spells." );
            break;
        default:
            break;
        }
        break;
    }
    case MYSTICISM: {
        str = _n( "%{skill} additionally regenerates one of your hero's spell points per day.",
                  "%{skill} additionally regenerates %{count} of your hero's spell points per day.", count );
        break;
    }
    case LUCK: {
        str = _( "%{skill} increases your hero's luck by %{count}." );
        break;
    }
    case BALLISTICS:
        switch ( Level() ) {
        case Level::BASIC:
            str = _( "%{skill} gives your hero's catapult shots a greater chance to hit and do damage to castle walls." );
            break;
        case Level::ADVANCED:
            str = _( "%{skill} gives your hero's catapult an extra shot, and each shot has a greater chance to hit and do damage to castle walls." );
            break;
        case Level::EXPERT:
            str = _( "%{skill} gives your hero's catapult an extra shot, and each shot automatically destroys any wall, except a fortified wall in a Knight castle." );
            break;
        default:
            break;
        }
        break;
    case EAGLEEYE:
        switch ( Level() ) {
        case Level::BASIC:
            str = _( "%{skill} gives your hero a %{count} percent chance to learn any given 1st or 2nd level enemy spell used against him in a combat." );
            break;
        case Level::ADVANCED:
            str = _( "%{skill} gives your hero a %{count} percent chance to learn any given 3rd level spell (or below) used against him in combat." );
            break;
        case Level::EXPERT:
            str = _( "%{skill} gives your hero a %{count} percent chance to learn any given 4th level spell (or below) used against him in combat." );
            break;
        default:
            break;
        }
        break;
    case NECROMANCY: {
        count += Skill::GetNecromancyPercent( hero ) - hero.GetSecondaryValues( Skill::Secondary::NECROMANCY );
        name = GetNameWithBonus( hero );
        str = _( "%{skill} allows %{count} percent of the creatures killed in combat to be brought back from the dead as Skeletons." );
        break;
    }
    case ESTATES:
        str = _( "Your hero produces %{count} gold pieces per day as tax revenue from estates." );
        break;
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
                AddSkill( Secondary( Secondary::EAGLEEYE, ptr->initial_secondary.eagleeye ) );
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

int Skill::SecondaryGetWeightSkillFromRace( int race, int skill )
{
    const stats_t * ptr = GameStatic::GetSkillStats( race );

    if ( ptr ) {
        if ( skill == Secondary::PATHFINDING )
            return ptr->mature_secondary.pathfinding;
        else if ( skill == Secondary::ARCHERY )
            return ptr->mature_secondary.archery;
        else if ( skill == Secondary::LOGISTICS )
            return ptr->mature_secondary.logistics;
        else if ( skill == Secondary::SCOUTING )
            return ptr->mature_secondary.scouting;
        else if ( skill == Secondary::DIPLOMACY )
            return ptr->mature_secondary.diplomacy;
        else if ( skill == Secondary::NAVIGATION )
            return ptr->mature_secondary.navigation;
        else if ( skill == Secondary::LEADERSHIP )
            return ptr->mature_secondary.leadership;
        else if ( skill == Secondary::WISDOM )
            return ptr->mature_secondary.wisdom;
        else if ( skill == Secondary::MYSTICISM )
            return ptr->mature_secondary.mysticism;
        else if ( skill == Secondary::LUCK )
            return ptr->mature_secondary.luck;
        else if ( skill == Secondary::BALLISTICS )
            return ptr->mature_secondary.ballistics;
        else if ( skill == Secondary::EAGLEEYE )
            return ptr->mature_secondary.eagleeye;
        else if ( skill == Secondary::NECROMANCY )
            return ptr->mature_secondary.necromancy;
        else if ( skill == Secondary::ESTATES )
            return ptr->mature_secondary.estates;
    }

    return 0;
}

int Skill::SecondaryPriorityFromRace( int race, const std::vector<int> & exclude, uint32_t seed )
{
    Rand::Queue parts( MAXSECONDARYSKILL );

    for ( auto skill : secskills )
        if ( exclude.end() == std::find( exclude.begin(), exclude.end(), skill ) )
            parts.Push( skill, SecondaryGetWeightSkillFromRace( race, skill ) );

    return parts.Size() ? parts.GetWithSeed( seed ) : Secondary::UNKNOWN;
}

/* select secondary skills for level up */
void Skill::SecSkills::FindSkillsForLevelUp( int race, uint32_t seedSkill1, uint32_t seedSkill2, Secondary & sec1, Secondary & sec2 ) const
{
    std::vector<int> exclude_skills;
    exclude_skills.reserve( MAXSECONDARYSKILL + HEROESMAXSKILL );

    // exclude for expert
    for ( const_iterator it = begin(); it != end(); ++it )
        if ( ( *it ).Level() == Level::EXPERT )
            exclude_skills.push_back( ( *it ).Skill() );

    // exclude is full, add other.
    if ( HEROESMAXSKILL <= Count() ) {
        std::copy_if( secskills, std::end( secskills ), std::back_inserter( exclude_skills ), [this]( int skill ) { return Level::NONE == GetLevel( skill ); } );
    }

    sec1.SetSkill( SecondaryPriorityFromRace( race, exclude_skills, seedSkill1 ) );

    if ( Secondary::UNKNOWN != sec1.Skill() ) {
        exclude_skills.push_back( sec1.Skill() );
        sec2.SetSkill( SecondaryPriorityFromRace( race, exclude_skills, seedSkill2 ) );

        sec1.SetLevel( GetLevel( sec1.Skill() ) );
        sec2.SetLevel( GetLevel( sec2.Skill() ) );

        sec1.NextLevel();
        sec2.NextLevel();
    }
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
