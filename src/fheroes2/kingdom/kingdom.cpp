/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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
#include <cassert>

#include "ai.h"
#include "battle.h"
#include "campaign_data.h"
#include "campaign_savedata.h"
#include "color.h"
#include "difficulty.h"
#include "game.h"
#include "game_interface.h"
#include "game_static.h"
#include "kingdom.h"
#include "logging.h"
#include "players.h"
#include "profit.h"
#include "race.h"
#include "save_format_version.h"
#include "serialize.h"
#include "settings.h"
#include "tools.h"
#include "translations.h"
#include "visit.h"
#include "world.h"

#include <cassert>

bool HeroesStrongestArmy( const Heroes * h1, const Heroes * h2 )
{
    return h1 && h2 && h2->GetArmy().isStrongerThan( h1->GetArmy() );
}

Kingdom::Kingdom()
    : color( Color::NONE )
    , _lastBattleWinHeroID( 0 )
    , lost_town_days( 0 )
    , visited_tents_colors( 0 )
    , _topItemInKingdomView( -1 )
{
    heroes_cond_loss.reserve( 4 );
}

void Kingdom::Init( int clr )
{
    clear();
    color = clr;

    if ( Color::ALL & color ) {
        heroes.reserve( GetMaxHeroes() );
        castles.reserve( 15 );
        resource = _getKingdomStartingResources( Game::getDifficulty() );
    }
    else {
        DEBUG_LOG( DBG_GAME, DBG_WARN, "Kingdom: unknown player: " << Color::String( color ) << "(" << static_cast<int>( color ) << ")" );
    }
}

void Kingdom::clear( void )
{
    modes = 0;

    color = Color::NONE;
    visited_tents_colors = 0;
    lost_town_days = Game::GetLostTownDays() + 1;

    heroes.clear();
    castles.clear();
    visit_object.clear();

    recruits.Reset();

    heroes_cond_loss.clear();
    puzzle_maps.reset();
}

int Kingdom::GetControl( void ) const
{
    return Players::GetPlayerControl( color );
}

int Kingdom::GetColor( void ) const
{
    return color;
}

int Kingdom::GetRace( void ) const
{
    return Players::GetPlayerRace( GetColor() );
}

bool Kingdom::isLoss( void ) const
{
    return castles.empty() && heroes.empty();
}

bool Kingdom::isPlay( void ) const
{
    return Players::GetPlayerInGame( color );
}

void Kingdom::LossPostActions( void )
{
    if ( isPlay() ) {
        Players::SetPlayerInGame( color, false );

        // Heroes::SetFreeman() calls Kingdom::RemoveHeroes(), which eventually calls heroes.erase()
        while ( !heroes.empty() ) {
            Heroes * hero = heroes.back();

            assert( hero->GetColor() == GetColor() );

            hero->SetFreeman( static_cast<int>( Battle::RESULT_LOSS ) );
        }

        if ( !castles.empty() ) {
            castles.ChangeColors( GetColor(), Color::NONE );
            castles.clear();
        }

        world.ResetCapturedObjects( GetColor() );
    }
}

void Kingdom::ActionBeforeTurn()
{
    if ( isControlHuman() ) {
        // Recalculate the existing paths of heroes if the kingdom is controlled by a human
        std::for_each( heroes.begin(), heroes.end(), []( Heroes * hero ) { hero->calculatePath( -1 ); } );
    }
    else {
        // Reset the paths of heroes if the kingdom is controlled by AI, because it uses a
        // special pathfinder implementation and revises its goals every turn
        std::for_each( heroes.begin(), heroes.end(), []( Heroes * hero ) { hero->GetPath().Reset(); } );
    }
}

void Kingdom::ActionNewDay( void )
{
    // countdown of days since the loss of the last town, first day isn't counted
    if ( world.CountDay() > 1 && castles.empty() && lost_town_days > 0 ) {
        --lost_town_days;
    }

    // check the conditions of the loss
    if ( isLoss() || 0 == lost_town_days ) {
        LossPostActions();
        return;
    }

    // modes
    ResetModes( IDENTIFYHERO );

    // skip the income for the first day
    if ( world.CountDay() > 1 ) {
        // income
        AddFundsResource( GetIncome() );

        // handle resource bonus campaign awards
        if ( isControlHuman() && Settings::Get().isCampaignGameType() ) {
            const std::vector<Campaign::CampaignAwardData> campaignAwards = Campaign::CampaignSaveData::Get().getObtainedCampaignAwards();

            for ( size_t i = 0; i < campaignAwards.size(); ++i ) {
                if ( campaignAwards[i]._type != Campaign::CampaignAwardData::TYPE_RESOURCE_BONUS )
                    continue;

                AddFundsResource( Funds( campaignAwards[i]._subType, campaignAwards[i]._amount ) );
            }
        }
    }

    // check event day AI
    EventsDate events = world.GetEventsDate( GetColor() );
    for ( EventsDate::const_iterator it = events.begin(); it != events.end(); ++it )
        AddFundsResource( ( *it ).resource );

    // remove day visit object
    visit_object.remove_if( Visit::isDayLife );
}

void Kingdom::ActionNewWeek( void )
{
    // skip the first week
    if ( world.CountWeek() > 1 ) {
        // debug a gift
        if ( IS_DEVEL() && isControlHuman() ) {
            Funds gift( 20, 20, 10, 10, 10, 10, 5000 );
            DEBUG_LOG( DBG_GAME, DBG_INFO, "debug gift: " << gift.String() );
            resource += gift;
        }
    }

    // remove week visit object
    visit_object.remove_if( Visit::isWeekLife );

    // Heroes who surrendered on Sunday should still be available for hire next week
    if ( world.CountDay() - recruits.getSurrenderDayOfHero1() > 1 ) {
        recruits.SetHero1( nullptr );
    }
    if ( world.CountDay() - recruits.getSurrenderDayOfHero2() > 1 ) {
        recruits.SetHero2( nullptr );
    }

    // Settle a new set of recruits
    GetRecruits();
}

void Kingdom::ActionNewMonth( void )
{
    // remove month visit object
    visit_object.remove_if( Visit::isMonthLife );
}

void Kingdom::AddHeroes( Heroes * hero )
{
    if ( hero ) {
        if ( heroes.end() == std::find( heroes.begin(), heroes.end(), hero ) )
            heroes.push_back( hero );

        const Player * player = Settings::Get().GetPlayers().GetCurrent();
        if ( player && player->isColor( GetColor() ) && player->isControlHuman() )
            Interface::Basic::Get().GetIconsPanel().ResetIcons( ICON_HEROES );

        AI::Get().HeroesAdd( *hero );
    }
}

void Kingdom::RemoveHeroes( const Heroes * hero )
{
    if ( hero ) {
        if ( !heroes.empty() ) {
            auto it = std::find( heroes.begin(), heroes.end(), hero );
            assert( it != heroes.end() );
            if ( it != heroes.end() ) {
                heroes.erase( it );
            }
        }

        Player * player = Players::Get( GetColor() );

        if ( player && player->GetFocus().GetHeroes() == hero ) {
            player->GetFocus().Reset();
        }

        assert( hero != nullptr );

        AI::Get().HeroesRemove( *hero );
    }

    if ( isLoss() )
        LossPostActions();
}

void Kingdom::AddCastle( const Castle * castle )
{
    if ( castle ) {
        if ( castles.end() == std::find( castles.begin(), castles.end(), castle ) )
            castles.push_back( const_cast<Castle *>( castle ) );

        const Player * player = Settings::Get().GetPlayers().GetCurrent();
        if ( player && player->isColor( GetColor() ) )
            Interface::Basic::Get().GetIconsPanel().ResetIcons( ICON_CASTLES );

        AI::Get().CastleAdd( *castle );
    }

    lost_town_days = Game::GetLostTownDays() + 1;
}

void Kingdom::RemoveCastle( const Castle * castle )
{
    if ( castle ) {
        if ( !castles.empty() ) {
            auto it = std::find( castles.begin(), castles.end(), castle );
            assert( it != castles.end() );
            if ( it != castles.end() ) {
                castles.erase( it );
            }
        }

        Player * player = Players::Get( GetColor() );

        if ( player && player->GetFocus().GetCastle() == castle ) {
            player->GetFocus().Reset();
        }

        assert( castle != nullptr );

        AI::Get().CastleRemove( *castle );
    }

    if ( isLoss() )
        LossPostActions();
}

u32 Kingdom::GetCountCastle( void ) const
{
    return static_cast<uint32_t>( std::count_if( castles.begin(), castles.end(), Castle::PredicateIsCastle ) );
}

u32 Kingdom::GetCountTown( void ) const
{
    return static_cast<uint32_t>( std::count_if( castles.begin(), castles.end(), Castle::PredicateIsTown ) );
}

u32 Kingdom::GetCountMarketplace( void ) const
{
    return static_cast<uint32_t>(
        std::count_if( castles.begin(), castles.end(), []( const Castle * castle ) { return Castle::PredicateIsBuildBuilding( castle, BUILD_MARKETPLACE ); } ) );
}

u32 Kingdom::GetCountNecromancyShrineBuild( void ) const
{
    return static_cast<uint32_t>( std::count_if( castles.begin(), castles.end(), []( const Castle * castle ) { return castle->isNecromancyShrineBuild(); } ) );
}

u32 Kingdom::GetCountBuilding( u32 build ) const
{
    return static_cast<uint32_t>( std::count_if( castles.begin(), castles.end(), [build]( const Castle * castle ) { return castle->isBuild( build ); } ) );
}

uint32_t Kingdom::GetCountThievesGuild() const
{
    return static_cast<uint32_t>(
        std::count_if( castles.begin(), castles.end(), []( const Castle * castle ) { return Castle::PredicateIsBuildBuilding( castle, BUILD_THIEVESGUILD ); } ) );
}

uint32_t Kingdom::GetCountArtifacts() const
{
    uint32_t result = 0;
    for ( const Heroes * hero : heroes )
        result += hero->GetCountArtifacts();
    return result;
}

bool Kingdom::AllowPayment( const Funds & funds ) const
{
    return ( resource.wood >= funds.wood || 0 == funds.wood ) && ( resource.mercury >= funds.mercury || 0 == funds.mercury )
           && ( resource.ore >= funds.ore || 0 == funds.ore ) && ( resource.sulfur >= funds.sulfur || 0 == funds.sulfur )
           && ( resource.crystal >= funds.crystal || 0 == funds.crystal ) && ( resource.gems >= funds.gems || 0 == funds.gems )
           && ( resource.gold >= funds.gold || 0 == funds.gold );
}

/* is visited cell */
bool Kingdom::isVisited( const Maps::Tiles & tile ) const
{
    return isVisited( tile.GetIndex(), tile.GetObject( false ) );
}

bool Kingdom::isVisited( s32 index, const MP2::MapObjectType objectType ) const
{
    std::list<IndexObject>::const_iterator it = std::find_if( visit_object.begin(), visit_object.end(), [index]( const IndexObject & v ) { return v.isIndex( index ); } );
    return visit_object.end() != it && ( *it ).isObject( objectType );
}

/* return true if object visited */
bool Kingdom::isVisited( const MP2::MapObjectType objectType ) const
{
    return std::any_of( visit_object.begin(), visit_object.end(), [objectType]( const IndexObject & v ) { return v.isObject( objectType ); } );
}

uint32_t Kingdom::CountVisitedObjects( const MP2::MapObjectType objectType ) const
{
    // Safe to downcast as we don't deal with gigantic amount of data.
    return static_cast<uint32_t>( std::count_if( visit_object.begin(), visit_object.end(), [objectType]( const IndexObject & v ) { return v.isObject( objectType ); } ) );
}

/* set visited cell */
void Kingdom::SetVisited( s32 index, const MP2::MapObjectType objectType = MP2::OBJ_ZERO )
{
    if ( !isVisited( index, objectType ) && objectType != MP2::OBJ_ZERO )
        visit_object.push_front( IndexObject( index, objectType ) );
}

bool Kingdom::isValidKingdomObject( const Maps::Tiles & tile, const MP2::MapObjectType objectType ) const
{
    if ( !MP2::isActionObject( objectType ) && objectType != MP2::OBJ_COAST )
        return false;

    if ( isVisited( tile.GetIndex(), objectType ) )
        return false;

    // Check castle first to ignore guest hero (tile with both Castle and Hero)
    if ( tile.GetObject( false ) == MP2::OBJ_CASTLE ) {
        const int tileColor = tile.QuantityColor();
        if ( Players::isFriends( color, tileColor ) ) {
            // false only if alliance castles can't be visited
            return color == tileColor;
        }
        return true;
    }

    // Hero object can overlay other objects when standing on top of it: force check with GetObject( true )
    if ( objectType == MP2::OBJ_HEROES ) {
        const Heroes * hero = tile.GetHeroes();
        return hero && ( color == hero->GetColor() || !Players::isFriends( color, hero->GetColor() ) );
    }

    if ( MP2::isCaptureObject( objectType ) )
        return !Players::isFriends( color, tile.QuantityColor() );

    if ( MP2::isQuantityObject( objectType ) )
        return tile.QuantityIsValid();

    return true;
}

bool Kingdom::HeroesMayStillMove( void ) const
{
    return std::any_of( heroes.begin(), heroes.end(), []( const Heroes * hero ) { return hero->MayStillMove( false, false ); } );
}

void Kingdom::AddFundsResource( const Funds & funds )
{
    resource = resource + funds;
    resource.Trim();
}

void Kingdom::OddFundsResource( const Funds & funds )
{
    resource = resource - funds;
    resource.Trim();
}

u32 Kingdom::GetLostTownDays( void ) const
{
    return lost_town_days;
}

const Recruits & Kingdom::GetRecruits()
{
    // In the first week, it is necessary to offer one native hero (or a hero given as a campaign award)
    const bool offerNativeHero = world.CountWeek() < 2 && recruits.GetID1() == Heroes::UNKNOWN && recruits.GetID2() == Heroes::UNKNOWN;
    // Special hero given as a campaign award
    const Heroes * specialHireableHero = nullptr;

    if ( isControlHuman() && Settings::Get().isCampaignGameType() && offerNativeHero ) {
        const std::vector<Campaign::CampaignAwardData> obtainedAwards = Campaign::CampaignSaveData::Get().getObtainedCampaignAwards();

        for ( const auto & obtainedAward : obtainedAwards ) {
            if ( obtainedAward._type != Campaign::CampaignAwardData::TYPE_HIREABLE_HERO ) {
                continue;
            }

            const Heroes * hero = world.GetHeroes( obtainedAward._subType );

            if ( hero && hero->isFreeman() ) {
                specialHireableHero = hero;
            }
        }
    }

    if ( recruits.GetID1() == Heroes::UNKNOWN || ( recruits.GetHero1() && !recruits.GetHero1()->isFreeman() ) ) {
        if ( specialHireableHero ) {
            recruits.SetHero1( specialHireableHero );
        }
        else {
            recruits.SetHero1( world.GetFreemanHeroes( offerNativeHero ? GetRace() : Race::NONE, recruits.GetID2() ) );
        }
    }

    if ( recruits.GetID2() == Heroes::UNKNOWN || ( recruits.GetHero2() && !recruits.GetHero2()->isFreeman() ) ) {
        recruits.SetHero2( world.GetFreemanHeroes( Race::NONE, recruits.GetID1() ) );
    }

    assert( recruits.GetID1() != recruits.GetID2() && recruits.GetID1() != Heroes::UNKNOWN && recruits.GetID2() != Heroes::UNKNOWN );

    return recruits;
}

Puzzle & Kingdom::PuzzleMaps( void )
{
    return puzzle_maps;
}

void Kingdom::SetVisitTravelersTent( int col )
{
    // visited_tents_color is a bitfield
    visited_tents_colors |= ( 1 << col );
}

bool Kingdom::IsVisitTravelersTent( int col ) const
{
    // visited_tents_color is a bitfield
    return ( visited_tents_colors & ( 1 << col ) ) != 0;
}

bool Kingdom::AllowRecruitHero( bool check_payment, int level ) const
{
    return ( heroes.size() < GetMaxHeroes() ) && ( !check_payment || AllowPayment( PaymentConditions::RecruitHero( level ) ) );
}

void Kingdom::ApplyPlayWithStartingHero( void )
{
    if ( !isPlay() || castles.empty() )
        return;

    bool foundHeroes = false;

    for ( KingdomCastles::const_iterator it = castles.begin(); it != castles.end(); ++it ) {
        const Castle * castle = *it;
        if ( castle == nullptr )
            continue;

        // check manual set hero (castle position + point(0, 1))?
        const fheroes2::Point & cp = castle->GetCenter();
        Heroes * hero = world.GetTiles( cp.x, cp.y + 1 ).GetHeroes();

        // and move manual set hero to castle
        if ( hero && hero->GetColor() == GetColor() ) {
            const bool patrol = hero->Modes( Heroes::PATROL );
            if ( hero->isValid() ) {
                hero->Move2Dest( Maps::GetIndexFromAbsPoint( cp ) );
            }
            else {
                hero->SetFreeman( 0 );
                hero->Recruit( *castle );
            }

            if ( patrol ) {
                hero->SetModes( Heroes::PATROL );
                hero->SetCenterPatrol( cp );
            }
            foundHeroes = true;
        }
    }

    if ( !foundHeroes && Settings::Get().GameStartWithHeroes() ) {
        // get first castle
        const Castle * first = castles.GetFirstCastle();
        if ( nullptr == first )
            first = castles.front();

        Heroes * hero = world.GetFreemanHeroes( first->GetRace() );
        if ( hero && AllowRecruitHero( false, 0 ) )
            hero->Recruit( *first );
    }
}

u32 Kingdom::GetMaxHeroes( void )
{
    return GameStatic::GetKingdomMaxHeroes();
}

Funds Kingdom::GetIncome( int type /* INCOME_ALL */ ) const
{
    Funds totalIncome;

    if ( INCOME_CAPTURED & type ) {
        // captured object
        const int resources[8]
            = { Resource::WOOD, Resource::ORE, Resource::MERCURY, Resource::SULFUR, Resource::CRYSTAL, Resource::GEMS, Resource::GOLD, Resource::UNKNOWN };

        for ( u32 index = 0; resources[index] != Resource::UNKNOWN; ++index )
            totalIncome += ProfitConditions::FromMine( resources[index] ) * world.CountCapturedMines( resources[index], GetColor() );
    }

    if ( INCOME_CASTLES & type ) {
        // castles
        for ( KingdomCastles::const_iterator it = castles.begin(); it != castles.end(); ++it ) {
            const Castle & castle = **it;

            // castle or town profit
            totalIncome += ProfitConditions::FromBuilding( ( castle.isCastle() ? BUILD_CASTLE : BUILD_TENT ), 0 );

            // statue
            if ( castle.isBuild( BUILD_STATUE ) )
                totalIncome += ProfitConditions::FromBuilding( BUILD_STATUE, 0 );

            // dungeon for warlock
            if ( castle.isBuild( BUILD_SPEC ) && Race::WRLK == castle.GetRace() )
                totalIncome += ProfitConditions::FromBuilding( BUILD_SPEC, Race::WRLK );
        }
    }

    if ( INCOME_ARTIFACTS & type ) {
        // find artifacts
        const std::array<int, 10> artifacts
            = { Artifact::GOLDEN_GOOSE,         Artifact::ENDLESS_SACK_GOLD,    Artifact::ENDLESS_BAG_GOLD,   Artifact::ENDLESS_PURSE_GOLD,
                Artifact::ENDLESS_POUCH_SULFUR, Artifact::ENDLESS_VIAL_MERCURY, Artifact::ENDLESS_POUCH_GEMS, Artifact::ENDLESS_CORD_WOOD,
                Artifact::ENDLESS_CART_ORE,     Artifact::ENDLESS_POUCH_CRYSTAL };

        for ( const Heroes * hero : heroes ) {
            for ( const int art : artifacts )
                totalIncome += ProfitConditions::FromArtifact( art ) * hero->artifactCount( Artifact( art ) );
            // TAX_LIEN
            totalIncome -= ProfitConditions::FromArtifact( Artifact::TAX_LIEN ) * hero->artifactCount( Artifact( Artifact::TAX_LIEN ) );
        }
    }

    if ( INCOME_HEROSKILLS & type ) {
        // estates skill bonus
        for ( KingdomHeroes::const_iterator ith = heroes.begin(); ith != heroes.end(); ++ith )
            totalIncome.gold += ( **ith ).GetSecondaryValues( Skill::Secondary::ESTATES );
    }

    if ( isControlAI() ) {
        totalIncome.gold = static_cast<int32_t>( totalIncome.gold * Difficulty::GetGoldIncomeBonus( Game::getDifficulty() ) );
    }

    return totalIncome;
}

Heroes * Kingdom::GetBestHero()
{
    return !heroes.empty() ? *std::max_element( heroes.begin(), heroes.end(), HeroesStrongestArmy ) : nullptr;
}

Monster Kingdom::GetStrongestMonster() const
{
    Monster monster( Monster::UNKNOWN );
    for ( const Heroes * hero : heroes ) {
        const Monster currentMonster = hero->GetArmy().GetStrongestMonster();
        if ( currentMonster.GetMonsterStrength() > monster.GetMonsterStrength() ) {
            monster = currentMonster;
        }
    }
    for ( const Castle * castle : castles ) {
        const Monster currentMonster = castle->GetArmy().GetStrongestMonster();
        if ( currentMonster.GetMonsterStrength() > monster.GetMonsterStrength() ) {
            monster = currentMonster;
        }
    }
    return monster;
}

double Kingdom::GetArmiesStrength( void ) const
{
    double res = 0;

    for ( KingdomHeroes::const_iterator ith = heroes.begin(); ith != heroes.end(); ++ith )
        res += ( **ith ).GetArmy().GetStrength();

    for ( KingdomCastles::const_iterator itc = castles.begin(); itc != castles.end(); ++itc )
        res += ( **itc ).GetArmy().GetStrength();

    return res;
}

void Kingdoms::Init( void )
{
    const Colors colors( Settings::Get().GetPlayers().GetColors() );

    clear();

    for ( Colors::const_iterator it = colors.begin(); it != colors.end(); ++it )
        GetKingdom( *it ).Init( *it );
}

void Kingdoms::clear( void )
{
    for ( Kingdom & kingdom : kingdoms )
        kingdom.clear();
}

void Kingdoms::ApplyPlayWithStartingHero( void )
{
    for ( Kingdom & kingdom : kingdoms )
        if ( kingdom.isPlay() )
            kingdom.ApplyPlayWithStartingHero();
}

const Kingdom & Kingdoms::GetKingdom( int color ) const
{
    switch ( color ) {
    case Color::BLUE:
        return kingdoms[0];
    case Color::GREEN:
        return kingdoms[1];
    case Color::RED:
        return kingdoms[2];
    case Color::YELLOW:
        return kingdoms[3];
    case Color::ORANGE:
        return kingdoms[4];
    case Color::PURPLE:
        return kingdoms[5];
    default:
        break;
    }

    return kingdoms[6];
}

Kingdom & Kingdoms::GetKingdom( int color )
{
    switch ( color ) {
    case Color::BLUE:
        return kingdoms[0];
    case Color::GREEN:
        return kingdoms[1];
    case Color::RED:
        return kingdoms[2];
    case Color::YELLOW:
        return kingdoms[3];
    case Color::ORANGE:
        return kingdoms[4];
    case Color::PURPLE:
        return kingdoms[5];
    default:
        break;
    }

    return kingdoms[6];
}

void Kingdom::SetLastBattleWinHero( const Heroes & hero )
{
    _lastBattleWinHeroID = hero.GetID();
}

Heroes * Kingdom::GetLastBattleWinHero() const
{
    return Heroes::UNKNOWN != _lastBattleWinHeroID ? world.GetHeroes( _lastBattleWinHeroID ) : nullptr;
}

void Kingdom::appendSurrenderedHero( const Heroes & hero )
{
    recruits.appendSurrenderedHero( hero, world.CountDay() );
}

void Kingdoms::NewDay( void )
{
    for ( Kingdom & kingdom : kingdoms )
        if ( kingdom.isPlay() )
            kingdom.ActionNewDay();
}

void Kingdoms::NewWeek( void )
{
    for ( Kingdom & kingdom : kingdoms )
        if ( kingdom.isPlay() )
            kingdom.ActionNewWeek();
}

void Kingdoms::NewMonth( void )
{
    for ( Kingdom & kingdom : kingdoms )
        if ( kingdom.isPlay() )
            kingdom.ActionNewMonth();
}

int Kingdoms::GetNotLossColors( void ) const
{
    int result = 0;
    for ( const Kingdom & kingdom : kingdoms )
        if ( kingdom.GetColor() && !kingdom.isLoss() )
            result |= kingdom.GetColor();
    return result;
}

int Kingdoms::FindWins( int cond ) const
{
    for ( const Kingdom & kingdom : kingdoms )
        if ( kingdom.GetColor() && world.KingdomIsWins( kingdom, cond ) )
            return kingdom.GetColor();
    return 0;
}

void Kingdoms::AddHeroes( const AllHeroes & heroes )
{
    for ( AllHeroes::const_iterator it = heroes.begin(); it != heroes.end(); ++it )
        // skip gray color
        if ( ( *it )->GetColor() )
            GetKingdom( ( *it )->GetColor() ).AddHeroes( *it );
}

void Kingdoms::AddCastles( const AllCastles & castles )
{
    for ( const Castle * castle : castles ) {
        // skip gray color
        if ( castle->GetColor() )
            GetKingdom( castle->GetColor() ).AddCastle( castle );
    }
}

void Kingdoms::AddTributeEvents( CapturedObjects & captureobj, const uint32_t day, const MP2::MapObjectType objectType )
{
    for ( Kingdom & kingdom : kingdoms ) {
        if ( kingdom.isPlay() ) {
            const int color = kingdom.GetColor();
            Funds funds;
            int objectCount = 0;

            captureobj.tributeCapturedObjects( color, objectType, funds, objectCount );
            if ( objectCount == 0 ) {
                continue;
            }

            kingdom.AddFundsResource( funds );

            // for show dialogs
            if ( funds.GetValidItemsCount() && kingdom.isControlHuman() ) {
                EventDate event;

                event.computer = true;
                event.first = day;
                event.colors = color;
                event.resource = funds;

                if ( objectCount > 1 ) {
                    event.title = std::to_string( objectCount );
                    event.title += ' ';
                    event.title += MP2::StringObject( objectType, objectCount );
                }
                else {
                    event.title = MP2::StringObject( objectType );
                }

                world.AddEventDate( event );
            }
        }
    }
}

// Check if tile is visible from any crystal ball of any hero
bool Kingdom::IsTileVisibleFromCrystalBall( const int32_t dest ) const
{
    for ( const Heroes * hero : heroes ) {
        if ( hero->hasArtifact( Artifact::CRYSTAL_BALL ) ) {
            const uint32_t crystalBallDistance = hero->GetVisionsDistance();
            if ( Maps::GetApproximateDistance( hero->GetIndex(), dest ) <= crystalBallDistance ) {
                return true;
            }
        }
    }
    return false;
}

cost_t Kingdom::_getKingdomStartingResources( const int difficulty ) const
{
    if ( isControlAI() )
        return { 10000, 30, 10, 30, 10, 10, 10 };

    switch ( difficulty ) {
    case Difficulty::EASY:
        return { 10000, 30, 10, 30, 10, 10, 10 };
    case Difficulty::NORMAL:
        return { 7500, 20, 5, 20, 5, 5, 5 };
    case Difficulty::HARD:
        return { 5000, 10, 2, 10, 2, 2, 2 };
    case Difficulty::EXPERT:
        return { 2500, 5, 0, 5, 0, 0, 0 };
    case Difficulty::IMPOSSIBLE:
        return { 0, 0, 0, 0, 0, 0, 0 };
    default:
        // Did you add a new difficulty level?
        assert( 0 );
        break;
    }

    return { 7500, 20, 5, 20, 5, 5, 5 };
}

StreamBase & operator<<( StreamBase & msg, const Kingdom & kingdom )
{
    return msg << kingdom.modes << kingdom.color << kingdom.resource << kingdom.lost_town_days << kingdom.castles << kingdom.heroes << kingdom.recruits
               << kingdom.visit_object << kingdom.puzzle_maps << kingdom.visited_tents_colors << kingdom.heroes_cond_loss << kingdom._lastBattleWinHeroID
               << kingdom._topItemInKingdomView;
}

StreamBase & operator>>( StreamBase & msg, Kingdom & kingdom )
{
    msg >> kingdom.modes >> kingdom.color >> kingdom.resource >> kingdom.lost_town_days >> kingdom.castles >> kingdom.heroes >> kingdom.recruits;

    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_0912_RELEASE, "Remove the check below." );
    if ( Game::GetLoadVersion() < FORMAT_VERSION_0912_RELEASE ) {
        int heroId;
        uint32_t heroSurrenderDay;

        msg >> heroId >> heroSurrenderDay;

        if ( heroId != Heroes::UNKNOWN && heroSurrenderDay > 0 ) {
            kingdom.recruits.SetHero2Tmp( world.GetHeroes( heroId ), heroSurrenderDay );
        }
    }

    msg >> kingdom.visit_object >> kingdom.puzzle_maps >> kingdom.visited_tents_colors >> kingdom.heroes_cond_loss >> kingdom._lastBattleWinHeroID;

    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_097_RELEASE, "Remove the check below." );
    if ( Game::GetLoadVersion() >= FORMAT_VERSION_097_RELEASE ) {
        msg >> kingdom._topItemInKingdomView;
    }
    else {
        kingdom._topItemInKingdomView = -1;
    }

    return msg;
}

StreamBase & operator<<( StreamBase & msg, const Kingdoms & obj )
{
    msg << Kingdoms::_size;
    for ( const Kingdom & kingdom : obj.kingdoms )
        msg << kingdom;

    return msg;
}

StreamBase & operator>>( StreamBase & msg, Kingdoms & obj )
{
    u32 kingdomscount = 0;
    msg >> kingdomscount;

    if ( kingdomscount <= Kingdoms::_size ) {
        for ( u32 i = 0; i < kingdomscount; ++i )
            msg >> obj.kingdoms[i];
    }

    return msg;
}
