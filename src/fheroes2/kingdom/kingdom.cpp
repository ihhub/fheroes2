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

#include "kingdom.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "ai.h"
#include "army.h"
#include "artifact.h"
#include "artifact_info.h"
#include "battle.h"
#include "campaign_data.h"
#include "campaign_savedata.h"
#include "color.h"
#include "difficulty.h"
#include "game.h"
#include "game_interface.h"
#include "game_io.h"
#include "game_static.h"
#include "interface_icons.h"
#include "logging.h"
#include "maps.h"
#include "maps_fileinfo.h"
#include "maps_tiles.h"
#include "maps_tiles_helper.h"
#include "math_base.h"
#include "payment.h"
#include "players.h"
#include "profit.h"
#include "race.h"
#include "route.h"
#include "save_format_version.h"
#include "serialize.h"
#include "settings.h"
#include "skill.h"
#include "visit.h"
#include "world.h"

namespace
{
    int32_t getHandicapIncomePercentage( const Player::HandicapStatus handicapStatus )
    {
        switch ( handicapStatus ) {
        case Player::HandicapStatus::NONE:
            return 100;
        case Player::HandicapStatus::MILD:
            return 85;
        case Player::HandicapStatus::SEVERE:
            return 70;
        default:
            // Did you add a new handicap status? Add the logic above!
            break;
        }

        return 100;
    }

    Funds getHandicapDependentIncome( const Funds & original, const Player::HandicapStatus handicapStatus )
    {
        const int32_t handicapPercentage = getHandicapIncomePercentage( handicapStatus );
        Funds corrected( original );
        corrected.wood = std::min( corrected.wood, ( corrected.wood * handicapPercentage + 99 ) / 100 );
        corrected.mercury = std::min( corrected.mercury, ( corrected.mercury * handicapPercentage + 99 ) / 100 );
        corrected.ore = std::min( corrected.ore, ( corrected.ore * handicapPercentage + 99 ) / 100 );
        corrected.sulfur = std::min( corrected.sulfur, ( corrected.sulfur * handicapPercentage + 99 ) / 100 );
        corrected.crystal = std::min( corrected.crystal, ( corrected.crystal * handicapPercentage + 99 ) / 100 );
        corrected.gems = std::min( corrected.gems, ( corrected.gems * handicapPercentage + 99 ) / 100 );
        corrected.gold = std::min( corrected.gold, ( corrected.gold * handicapPercentage + 99 ) / 100 );

        return corrected;
    }
}

bool HeroesStrongestArmy( const Heroes * h1, const Heroes * h2 )
{
    return h1 && h2 && h2->GetArmy().isStrongerThan( h1->GetArmy() );
}

Kingdom::Kingdom()
    : color( Color::NONE )
    , _lastBattleWinHeroID( 0 )
    , lost_town_days( 0 )
    , visited_tents_colors( 0 )
    , _topCastleInKingdomView( -1 )
    , _topHeroInKingdomView( -1 )
{
    // Do nothing.
}

void Kingdom::Init( int clr )
{
    clear();
    color = clr;

    if ( Color::ALL & color ) {
        heroes.reserve( GetMaxHeroes() );
        castles.reserve( 15 );

        // Difficulty calculation is different for campaigns. Difficulty affects only on starting resources for human players.
        const Settings & configuration = Settings::Get();
        const int difficultyLevel = ( configuration.isCampaignGameType() ? configuration.CurrentFileInfo().difficulty : configuration.GameDifficulty() );

        resource = _getKingdomStartingResources( difficultyLevel );

        // Some human players can have handicap for resources.
        const Player * player = Players::Get( color );
        assert( player != nullptr );
        resource = getHandicapDependentIncome( resource, player->getHandicapStatus() );
    }
    else {
        DEBUG_LOG( DBG_GAME, DBG_WARN, "Kingdom: unknown player: " << Color::String( color ) << "(" << static_cast<int>( color ) << ")" )
    }
}

void Kingdom::clear()
{
    modes = 0;

    color = Color::NONE;
    visited_tents_colors = 0;
    lost_town_days = Game::GetLostTownDays() + 1;

    heroes.clear();
    castles.clear();
    visit_object.clear();

    recruits.Reset();

    puzzle_maps.reset();
}

int Kingdom::GetControl() const
{
    return Players::GetPlayerControl( color );
}

int Kingdom::GetColor() const
{
    return color;
}

int Kingdom::GetRace() const
{
    return Players::GetPlayerRace( GetColor() );
}

bool Kingdom::isLoss() const
{
    return castles.empty() && heroes.empty();
}

bool Kingdom::isPlay() const
{
    return Players::GetPlayerInGame( color );
}

void Kingdom::LossPostActions()
{
    if ( isPlay() ) {
        Players::SetPlayerInGame( color, false );

        // Heroes::Dismiss() calls Kingdom::RemoveHero(), which eventually calls heroes.erase()
        while ( !heroes.empty() ) {
            Heroes * hero = heroes.back();

            assert( hero->GetColor() == GetColor() );

            hero->Dismiss( static_cast<int>( Battle::RESULT_LOSS ) );
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

void Kingdom::ActionNewDay()
{
    // Clear the visited objects with a lifetime of one day, even if this kingdom has already been vanquished
    visit_object.remove_if( Visit::isDayLife );

    if ( !isPlay() ) {
        return;
    }

    // Countdown of days since the loss of the last town, first day isn't counted
    if ( world.CountDay() > 1 && castles.empty() && lost_town_days > 0 ) {
        --lost_town_days;
    }

    // Check the basic conditions of losing the game
    if ( isLoss() || 0 == lost_town_days ) {
        LossPostActions();

        return;
    }

    // Reset the effect of the "Identify Hero" spell
    ResetModes( IDENTIFYHERO );
}

void Kingdom::ActionNewDayResourceUpdate( const std::function<void( const EventDate & event, const Funds & funds )> & displayEventDialog )
{
    // Skip the income for the first day
    if ( world.CountDay() > 1 ) {
        AddFundsResource( GetIncome() );

        // Resource bonuses from campaign awards
        if ( isControlHuman() && Settings::Get().isCampaignGameType() ) {
            const std::vector<Campaign::CampaignAwardData> campaignAwards = Campaign::CampaignSaveData::Get().getObtainedCampaignAwards();

            for ( size_t i = 0; i < campaignAwards.size(); ++i ) {
                if ( campaignAwards[i]._type != Campaign::CampaignAwardData::TYPE_RESOURCE_BONUS ) {
                    continue;
                }

                AddFundsResource( Funds( campaignAwards[i]._subType, campaignAwards[i]._amount ) );
            }
        }
    }

    const bool isAIPlayer = ( GetControl() == CONTROL_AI );

    // Resources from events
    const EventsDate events = world.GetEventsDate( GetColor() );
    for ( const EventDate & event : events ) {
        if ( isAIPlayer && !event.isApplicableForAIPlayers ) {
            continue;
        }

        const Funds fundsUpdate = Resource::CalculateEventResourceUpdate( GetFunds(), event.resource );
        AddFundsResource( fundsUpdate );
        if ( displayEventDialog )
            displayEventDialog( event, fundsUpdate );
    }
}

void Kingdom::ActionNewWeek()
{
    // Clear the visited objects with a lifetime of one week, even if this kingdom has already been vanquished
    visit_object.remove_if( Visit::isWeekLife );

    if ( !isPlay() ) {
        return;
    }

    // Skip the first week
    if ( world.CountWeek() > 1 ) {
        // Additional gift in debug mode
        if ( IS_DEVEL() && isControlHuman() ) {
            Funds gift( 20, 20, 10, 10, 10, 10, 5000 );
            DEBUG_LOG( DBG_GAME, DBG_INFO, "debug gift: " << gift.String() )
            resource += gift;
        }
    }

    // Settle a new set of recruits
    GetRecruits();
}

void Kingdom::ActionNewMonth()
{
    // Clear the visited objects with a lifetime of one month, even if this kingdom has already been vanquished
    visit_object.remove_if( Visit::isMonthLife );
}

void Kingdom::AddHero( Heroes * hero )
{
    if ( hero == nullptr ) {
        // Why are you adding an empty hero?
        assert( 0 );
        return;
    }

    if ( heroes.end() == std::find( heroes.begin(), heroes.end(), hero ) ) {
        heroes.push_back( hero );
    }

    AI::Get().HeroesAdd( *hero );
}

void Kingdom::RemoveHero( const Heroes * hero )
{
    if ( hero != nullptr ) {
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
    else {
        // Why are trying to delete a non existing hero?
        assert( 0 );
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
            Interface::AdventureMap::Get().GetIconsPanel().ResetIcons( ICON_CASTLES );

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

uint32_t Kingdom::GetCountCastle() const
{
    return static_cast<uint32_t>( std::count_if( castles.begin(), castles.end(), Castle::PredicateIsCastle ) );
}

uint32_t Kingdom::GetCountTown() const
{
    return static_cast<uint32_t>( std::count_if( castles.begin(), castles.end(), Castle::PredicateIsTown ) );
}

uint32_t Kingdom::GetCountMarketplace() const
{
    return static_cast<uint32_t>(
        std::count_if( castles.begin(), castles.end(), []( const Castle * castle ) { return Castle::PredicateIsBuildBuilding( castle, BUILD_MARKETPLACE ); } ) );
}

uint32_t Kingdom::GetCountNecromancyShrineBuild() const
{
    return static_cast<uint32_t>( std::count_if( castles.begin(), castles.end(), []( const Castle * castle ) { return castle->isNecromancyShrineBuild(); } ) );
}

uint32_t Kingdom::GetCountBuilding( uint32_t build ) const
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

bool Kingdom::isVisited( const Maps::Tiles & tile ) const
{
    return isVisited( tile.GetIndex(), tile.GetObject( false ) );
}

bool Kingdom::isVisited( int32_t index, const MP2::MapObjectType objectType ) const
{
    std::list<IndexObject>::const_iterator it = std::find_if( visit_object.begin(), visit_object.end(), [index]( const IndexObject & v ) { return v.isIndex( index ); } );
    return visit_object.end() != it && ( *it ).isObject( objectType );
}

bool Kingdom::isVisited( const MP2::MapObjectType objectType ) const
{
    return std::any_of( visit_object.begin(), visit_object.end(), [objectType]( const IndexObject & v ) { return v.isObject( objectType ); } );
}

uint32_t Kingdom::CountVisitedObjects( const MP2::MapObjectType objectType ) const
{
    // Safe to downcast as we don't deal with gigantic amount of data.
    return static_cast<uint32_t>( std::count_if( visit_object.begin(), visit_object.end(), [objectType]( const IndexObject & v ) { return v.isObject( objectType ); } ) );
}

void Kingdom::SetVisited( int32_t index, const MP2::MapObjectType objectType )
{
    if ( !isVisited( index, objectType ) && objectType != MP2::OBJ_NONE )
        visit_object.emplace_front( index, objectType );
}

bool Kingdom::isValidKingdomObject( const Maps::Tiles & tile, const MP2::MapObjectType objectType ) const
{
    if ( !MP2::isActionObject( objectType ) )
        return false;

    if ( isVisited( tile.GetIndex(), objectType ) )
        return false;

    // Check castle first to ignore guest hero (tile with both Castle and Hero)
    if ( tile.GetObject( false ) == MP2::OBJ_CASTLE ) {
        const int tileColor = getColorFromTile( tile );

        // Castle can only be visited if it either belongs to this kingdom or is an enemy castle (in the latter case, an attack may occur)
        return color == tileColor || !Players::isFriends( color, tileColor );
    }

    // Hero object can overlay other objects when standing on top of it: force check with GetObject( true )
    if ( objectType == MP2::OBJ_HEROES ) {
        const Heroes * hero = tile.getHero();

        // Hero can only be met if he either belongs to this kingdom or is an enemy hero (in the latter case, an attack will occur)
        return hero && ( color == hero->GetColor() || !Players::isFriends( color, hero->GetColor() ) );
    }

    if ( MP2::isCaptureObject( objectType ) )
        return !Players::isFriends( color, getColorFromTile( tile ) );

    if ( MP2::isValuableResourceObject( objectType ) )
        return doesTileContainValuableItems( tile );

    return true;
}

bool Kingdom::opponentsCanRecruitMoreHeroes() const
{
    for ( int opponentColor : Players::getInPlayOpponents( GetColor() ) ) {
        if ( world.GetKingdom( opponentColor ).canRecruitHeroes() )
            return true;
    }
    return false;
}

bool Kingdom::opponentsHaveHeroes() const
{
    for ( int opponentColor : Players::getInPlayOpponents( GetColor() ) ) {
        if ( world.GetKingdom( opponentColor ).hasHeroes() )
            return true;
    }
    return false;
}

bool Kingdom::HeroesMayStillMove() const
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

uint32_t Kingdom::GetLostTownDays() const
{
    return lost_town_days;
}

const Recruits & Kingdom::GetRecruits()
{
    // In the first week, it is necessary to offer one native hero (or a hero given as a campaign award)
    const bool offerNativeHero = world.CountWeek() < 2 && recruits.GetID1() == Heroes::UNKNOWN && recruits.GetID2() == Heroes::UNKNOWN;
    // Special hero given as a campaign award
    Heroes * specialHireableHero = nullptr;

    if ( isControlHuman() && Settings::Get().isCampaignGameType() && offerNativeHero ) {
        const std::vector<Campaign::CampaignAwardData> obtainedAwards = Campaign::CampaignSaveData::Get().getObtainedCampaignAwards();

        for ( const auto & obtainedAward : obtainedAwards ) {
            if ( obtainedAward._type != Campaign::CampaignAwardData::TYPE_HIREABLE_HERO ) {
                continue;
            }

            Heroes * hero = world.GetHeroes( obtainedAward._subType );

            if ( hero && hero->isAvailableForHire() ) {
                specialHireableHero = hero;
            }
        }
    }

    if ( recruits.GetID1() == Heroes::UNKNOWN || ( recruits.GetHero1() && !recruits.GetHero1()->isAvailableForHire() ) ) {
        if ( specialHireableHero ) {
            recruits.SetHero1( specialHireableHero );
        }
        else {
            recruits.SetHero1( world.GetHeroForHire( offerNativeHero ? GetRace() : Race::NONE, recruits.GetID2() ) );
        }
    }

    if ( recruits.GetID2() == Heroes::UNKNOWN || ( recruits.GetHero2() && !recruits.GetHero2()->isAvailableForHire() ) ) {
        recruits.SetHero2( world.GetHeroForHire( Race::NONE, recruits.GetID1() ) );
    }

    assert( recruits.GetID1() != recruits.GetID2() && recruits.GetID1() != Heroes::UNKNOWN && recruits.GetID2() != Heroes::UNKNOWN );

    return recruits;
}

Recruits & Kingdom::GetCurrentRecruits()
{
    return recruits;
}

Puzzle & Kingdom::PuzzleMaps()
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

bool Kingdom::AllowRecruitHero( bool check_payment ) const
{
    return ( heroes.size() < GetMaxHeroes() ) && ( !check_payment || AllowPayment( PaymentConditions::RecruitHero() ) );
}

void Kingdom::ApplyPlayWithStartingHero()
{
    if ( !isPlay() || castles.empty() )
        return;

    bool foundHeroes = false;

    for ( const Castle * castle : castles ) {
        if ( castle == nullptr )
            continue;

        // check manual set hero (castle position + point(0, 1))?
        const fheroes2::Point & cp = castle->GetCenter();
        Heroes * hero = world.GetTiles( cp.x, cp.y + 1 ).getHero();

        // and move manual set hero to castle
        if ( hero && hero->GetColor() == GetColor() ) {
            const bool patrol = hero->Modes( Heroes::PATROL );
            if ( hero->isValid() ) {
                hero->Move2Dest( Maps::GetIndexFromAbsPoint( cp ) );
            }
            else {
                hero->Dismiss( 0 );
                hero->Recruit( *castle );
            }

            if ( patrol ) {
                hero->SetModes( Heroes::PATROL );
                hero->SetPatrolCenter( cp );
            }
            foundHeroes = true;
        }
    }

    if ( !foundHeroes && Settings::Get().GameStartWithHeroes() ) {
        // get first castle
        const Castle * first = castles.GetFirstCastle();
        if ( nullptr == first )
            first = castles.front();

        Heroes * hero = world.GetHeroForHire( first->GetRace() );
        if ( hero && AllowRecruitHero( false ) )
            hero->Recruit( *first );
    }
}

uint32_t Kingdom::GetMaxHeroes()
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

        for ( uint32_t index = 0; resources[index] != Resource::UNKNOWN; ++index )
            totalIncome += ProfitConditions::FromMine( resources[index] ) * world.CountCapturedMines( resources[index], GetColor() );
    }

    if ( INCOME_CASTLES & type ) {
        // castles
        for ( const Castle * castle : castles ) {
            assert( castle != nullptr );

            // castle or town profit
            totalIncome += ProfitConditions::FromBuilding( ( castle->isCastle() ? BUILD_CASTLE : BUILD_TENT ), 0 );

            // statue
            if ( castle->isBuild( BUILD_STATUE ) )
                totalIncome += ProfitConditions::FromBuilding( BUILD_STATUE, 0 );

            // dungeon for warlock
            if ( castle->isBuild( BUILD_SPEC ) && Race::WRLK == castle->GetRace() )
                totalIncome += ProfitConditions::FromBuilding( BUILD_SPEC, Race::WRLK );
        }
    }

    if ( INCOME_ARTIFACTS & type ) {
        for ( const Heroes * hero : heroes ) {
            const BagArtifacts & bag = hero->GetBagArtifacts();
            for ( const Artifact & artifact : bag ) {
                totalIncome += ProfitConditions::FromArtifact( artifact.GetID() );
            }
        }
    }

    if ( INCOME_HERO_SKILLS & type ) {
        // estates skill bonus
        for ( const Heroes * hero : heroes ) {
            assert( hero != nullptr );
            totalIncome.gold += hero->GetSecondaryValues( Skill::Secondary::ESTATES );
        }
    }

    if ( ( type & INCOME_CAMPAIGN_BONUS ) && Settings::Get().isCampaignGameType() ) {
        const std::vector<Campaign::CampaignAwardData> awards = Campaign::CampaignSaveData::Get().getObtainedCampaignAwards();
        for ( const Campaign::CampaignAwardData & award : awards ) {
            if ( award._type != Campaign::CampaignAwardData::TYPE_RESOURCE_BONUS ) {
                continue;
            }

            totalIncome += Funds( award._subType, award._amount );
        }
    }

    if ( isControlAI() && totalIncome.gold > 0 ) {
        const int32_t bonusGold = static_cast<int32_t>( totalIncome.gold * Difficulty::getGoldIncomeBonusForAI( Game::getDifficulty() ) );

        totalIncome.gold += bonusGold;
    }

    // Some human players can have handicap for resources.
    const Player * player = Players::Get( color );
    assert( player != nullptr );
    return getHandicapDependentIncome( totalIncome, player->getHandicapStatus() );
}

Heroes * Kingdom::GetBestHero() const
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

double Kingdom::GetArmiesStrength() const
{
    double res = 0;

    for ( const Heroes * hero : heroes ) {
        assert( hero != nullptr );
        res += hero->GetArmy().GetStrength();
    }

    for ( const Castle * castle : castles ) {
        assert( castle != nullptr );
        res += castle->GetArmy().GetStrength();
    }

    return res;
}

void Kingdoms::Init()
{
    const Colors colors( Settings::Get().GetPlayers().GetColors() );

    clear();

    for ( Colors::const_iterator it = colors.begin(); it != colors.end(); ++it )
        GetKingdom( *it ).Init( *it );
}

void Kingdoms::clear()
{
    for ( Kingdom & kingdom : kingdoms )
        kingdom.clear();
}

void Kingdoms::ApplyPlayWithStartingHero()
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

void Kingdom::appendSurrenderedHero( Heroes & hero )
{
    recruits.appendSurrenderedHero( hero, world.CountDay() );
}

void Kingdoms::NewDay()
{
    for ( Kingdom & kingdom : kingdoms ) {
        kingdom.ActionNewDay();
    }
}

void Kingdoms::NewWeek()
{
    for ( Kingdom & kingdom : kingdoms ) {
        kingdom.ActionNewWeek();
    }
}

void Kingdoms::NewMonth()
{
    for ( Kingdom & kingdom : kingdoms ) {
        kingdom.ActionNewMonth();
    }
}

int Kingdoms::GetNotLossColors() const
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
    for ( Heroes * hero : heroes ) {
        assert( hero != nullptr );

        if ( hero->GetColor() != Color::NONE ) {
            GetKingdom( hero->GetColor() ).AddHero( hero );
        }
    }
}

void Kingdoms::AddCastles( const AllCastles & castles )
{
    for ( const Castle * castle : castles ) {
        // skip gray color
        if ( castle->GetColor() )
            GetKingdom( castle->GetColor() ).AddCastle( castle );
    }
}

std::set<Heroes *> Kingdoms::resetRecruits()
{
    std::set<Heroes *> remainingRecruits;

    for ( Kingdom & kingdom : kingdoms ) {
        Recruits & recruits = kingdom.GetCurrentRecruits();

        // Heroes who surrendered on Sunday should still be available for recruitment next week in
        // the same kingdom, provided that this kingdom is still playable
        if ( !kingdom.isPlay() || world.CountDay() - recruits.getSurrenderDayOfHero1() > 1 ) {
            recruits.SetHero1( nullptr );
        }
        else {
            Heroes * hero = recruits.GetHero1();

            if ( hero ) {
                remainingRecruits.insert( hero );
            }
        }
        if ( !kingdom.isPlay() || world.CountDay() - recruits.getSurrenderDayOfHero2() > 1 ) {
            recruits.SetHero2( nullptr );
        }
        else {
            Heroes * hero = recruits.GetHero2();

            if ( hero ) {
                remainingRecruits.insert( hero );
            }
        }
    }

    return remainingRecruits;
}

bool Kingdom::IsTileVisibleFromCrystalBall( const int32_t dest ) const
{
    for ( const Heroes * hero : heroes ) {
        assert( hero != nullptr );

        if ( hero->GetBagArtifacts().isArtifactBonusPresent( fheroes2::ArtifactBonusType::VIEW_MONSTER_INFORMATION ) ) {
            const uint32_t crystalBallDistance = hero->GetVisionsDistance();

            if ( Maps::GetStraightLineDistance( hero->GetIndex(), dest ) <= crystalBallDistance ) {
                return true;
            }
        }
    }

    return false;
}

cost_t Kingdom::_getKingdomStartingResources( const int difficulty ) const
{
    if ( isControlAI() ) {
        switch ( difficulty ) {
        case Difficulty::EASY:
        case Difficulty::NORMAL:
            return { 7500, 20, 5, 20, 5, 5, 5 };
        case Difficulty::HARD:
        case Difficulty::EXPERT:
        case Difficulty::IMPOSSIBLE:
            return { 10000, 30, 10, 30, 10, 10, 10 };
        default:
            // Did you add a new difficulty level?
            assert( 0 );
            break;
        }

        return { 10000, 30, 10, 30, 10, 10, 10 };
    }

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
               << kingdom.visit_object << kingdom.puzzle_maps << kingdom.visited_tents_colors << kingdom._lastBattleWinHeroID << kingdom._topCastleInKingdomView
               << kingdom._topHeroInKingdomView;
}

StreamBase & operator>>( StreamBase & msg, Kingdom & kingdom )
{
    msg >> kingdom.modes >> kingdom.color >> kingdom.resource >> kingdom.lost_town_days >> kingdom.castles >> kingdom.heroes >> kingdom.recruits >> kingdom.visit_object
        >> kingdom.puzzle_maps >> kingdom.visited_tents_colors >> kingdom._lastBattleWinHeroID;

    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_1010_RELEASE, "Remove the logic below." );
    if ( Game::GetVersionOfCurrentSaveFile() < FORMAT_VERSION_1010_RELEASE ) {
        ++kingdom._lastBattleWinHeroID;
    }

    return msg >> kingdom._topCastleInKingdomView >> kingdom._topHeroInKingdomView;
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
    uint32_t kingdomscount = 0;
    msg >> kingdomscount;

    if ( kingdomscount <= Kingdoms::_size ) {
        for ( uint32_t i = 0; i < kingdomscount; ++i )
            msg >> obj.kingdoms[i];
    }

    return msg;
}
