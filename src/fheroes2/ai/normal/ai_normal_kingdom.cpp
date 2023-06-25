/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2023                                             *
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
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <ostream>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "ai.h"
#include "ai_normal.h"
#include "army.h"
#include "army_troop.h"
#include "audio.h"
#include "audio_manager.h"
#include "castle.h"
#include "color.h"
#include "game_interface.h"
#include "ground.h"
#include "heroes.h"
#include "heroes_recruits.h"
#include "interface_status.h"
#include "kingdom.h"
#include "logging.h"
#include "maps.h"
#include "maps_tiles.h"
#include "maps_tiles_helper.h"
#include "mp2.h"
#include "mus.h"
#include "pairs.h"
#include "players.h"
#include "resource.h"
#include "skill.h"
#include "spell.h"
#include "world.h"
#include "world_pathfinding.h"
#include "world_regions.h"

namespace
{
    const double fighterStrengthMultiplier = 3;

    struct HeroValue
    {
        Heroes * hero = nullptr;
        double strength = 0.0;
        int stats = 0;

        HeroValue( Heroes * inHero, double inStrength, int inStats )
            : hero( inHero )
            , strength( inStrength )
            , stats( inStats )
        {
            // Do nothing.
        }
    };

    void setHeroRoles( KingdomHeroes & heroes )
    {
        if ( heroes.empty() ) {
            // No heroes exist.
            return;
        }

        const Heroes * valuableHero = world.GetHeroesCondWins();

        if ( heroes.size() == 1 ) {
            if ( valuableHero != nullptr && valuableHero == heroes[0] ) {
                heroes[0]->setAIRole( Heroes::Role::CHAMPION );
            }
            else {
                // A single hero has no roles.
                heroes[0]->setAIRole( Heroes::Role::HUNTER );
            }

            return;
        }

        // Set hero's roles. First calculate each hero strength and sort it in descending order.
        std::vector<HeroValue> heroList;
        for ( Heroes * hero : heroes ) {
            // AI heroes set on patrol mode can only be fighters; ignore them otherwise
            if ( hero->Modes( Heroes::PATROL ) ) {
                hero->setAIRole( Heroes::Role::FIGHTER );
            }
            else {
                heroList.emplace_back( hero, hero->GetArmy().GetStrength(), hero->getStatsValue() );
            }
        }

        if ( heroList.empty() ) {
            // No more heroes.
            return;
        }

        // If there's plenty of heroes we can assign special roles
        if ( heroList.size() > 3 ) {
            std::sort( heroList.begin(), heroList.end(), []( const HeroValue & first, const HeroValue & second ) { return first.stats > second.stats; } );

            if ( valuableHero == nullptr ) {
                heroList.front().hero->setAIRole( Heroes::Role::CHAMPION );
                heroList.erase( heroList.begin() );
            }

            // Assign the courier role and remove them so they aren't counted towards the median strength
            heroList.back().hero->setAIRole( Heroes::Role::COURIER );
            heroList.pop_back();

            if ( heroList.size() > 2 ) {
                // We still have a plenty of heroes. In this case lets create a Scout hero to uncover the fog.
                heroList.back().hero->setAIRole( Heroes::Role::SCOUT );
                heroList.pop_back();
            }
        }

        assert( !heroList.empty() );

        std::sort( heroList.begin(), heroList.end(), []( const HeroValue & first, const HeroValue & second ) { return first.strength > second.strength; } );

        const double medianStrength = heroList[heroList.size() / 2].strength;

        for ( HeroValue & object : heroList ) {
            if ( valuableHero != nullptr && object.hero == valuableHero ) {
                object.hero->setAIRole( Heroes::Role::CHAMPION );
                continue;
            }

            if ( object.strength > medianStrength * fighterStrengthMultiplier ) {
                object.hero->setAIRole( Heroes::Role::FIGHTER );
            }
            else {
                object.hero->setAIRole( Heroes::Role::HUNTER );
            }
        }
    }
}

namespace AI
{
    bool Normal::recruitHero( Castle & castle, bool buyArmy, bool underThreat )
    {
        Kingdom & kingdom = castle.GetKingdom();
        const Recruits & rec = kingdom.GetRecruits();

        Heroes * recruit = nullptr;

        // Re-hiring a hero related to any of the WINS_HERO or LOSS_HERO conditions is not allowed
        const auto heroesToIgnore = std::make_pair( world.GetHeroesCondWins(), world.GetHeroesCondLoss() );

        auto useIfPossible = [&heroesToIgnore]( Heroes * hero ) -> Heroes * {
            if ( std::apply( [hero]( const auto... heroToIgnore ) { return ( ( hero == heroToIgnore ) || ... ); }, heroesToIgnore ) ) {
                return nullptr;
            }

            return hero;
        };

        Heroes * firstRecruit = useIfPossible( rec.GetHero1() );
        Heroes * secondRecruit = useIfPossible( rec.GetHero2() );

        if ( firstRecruit && secondRecruit ) {
            if ( secondRecruit->getRecruitValue() > firstRecruit->getRecruitValue() ) {
                recruit = castle.RecruitHero( secondRecruit );
            }
            else {
                recruit = castle.RecruitHero( firstRecruit );
            }
        }
        else if ( firstRecruit ) {
            recruit = castle.RecruitHero( firstRecruit );
        }
        else if ( secondRecruit ) {
            recruit = castle.RecruitHero( secondRecruit );
        }

        if ( recruit && buyArmy ) {
            CastleTurn( castle, underThreat );
            reinforceHeroInCastle( *recruit, castle, kingdom.GetFunds() );
        }

        return recruit != nullptr;
    }

    void Normal::reinforceHeroInCastle( Heroes & hero, Castle & castle, const Funds & budget )
    {
        const Heroes::AIHeroMeetingUpdater heroMeetingUpdater( hero );

        if ( !hero.HaveSpellBook() && castle.GetLevelMageGuild() > 0 && !hero.IsFullBagArtifacts() ) {
            // this call will check if AI kingdom have enough resources to buy book
            hero.BuySpellBook( &castle );
        }

        Army & heroArmy = hero.GetArmy();
        Army & garrison = castle.GetArmy();

        // Merge all troops in the castle to have the best army.
        heroArmy.JoinStrongestFromArmy( garrison );

        // Upgrade troops and try to merge them again.
        heroArmy.UpgradeTroops( castle );
        garrison.UpgradeTroops( castle );
        heroArmy.JoinStrongestFromArmy( garrison );

        // Recruit more troops and also merge them.
        castle.recruitBestAvailable( budget );
        heroArmy.JoinStrongestFromArmy( garrison );

        const int32_t castleIndex = castle.GetIndex();

        if ( isPriorityTask( castleIndex ) ) {
            double heroStrength = heroArmy.GetStrength();
            double castleStrength = garrison.GetStrength();

            auto defenseTask = _priorityTargets.find( castleIndex );
            assert( defenseTask != _priorityTargets.end() );

            if ( heroStrength + castleStrength > defenseTask->second.threatLevel * AI::ARMY_ADVANTAGE_MEDIUM ) {
                // The enemy army is so-so. Feed some monsters to the castle and continue exploring the map.
                garrison.MergeSameMonsterTroops();

                const double minStrength = defenseTask->second.threatLevel * AI::ARMY_ADVANTAGE_DESPERATE;

                while ( castleStrength < minStrength ) {
                    Troop * weakestTroop = heroArmy.GetWeakestTroop();
                    assert( weakestTroop != nullptr );

                    const uint32_t initialCount = weakestTroop->GetCount();
                    const double weakestTroopStrength = weakestTroop->GetStrength();

                    uint32_t count = std::max( initialCount / 2, 1U );
                    if ( weakestTroopStrength < heroStrength * 0.1 ) {
                        count = initialCount;
                    }

                    if ( heroArmy.GetOccupiedSlotCount() == 1 ) {
                        // This is the last slot. Make sure to still leave some army for the hero.
                        if ( initialCount == 1 ) {
                            break;
                        }

                        if ( count == initialCount ) {
                            --count;
                        }
                    }

                    const double singleMonsterStrength = weakestTroopStrength / initialCount;
                    const double strengthDifference = ( minStrength - castleStrength );

                    assert( count > 0 );

                    if ( singleMonsterStrength * ( count - 1 ) > strengthDifference ) {
                        // We are giving too much army to the castle. Adjust it.
                        count = static_cast<uint32_t>( strengthDifference / singleMonsterStrength );
                        if ( count * singleMonsterStrength < strengthDifference ) {
                            ++count;
                        }
                    }

                    assert( count <= initialCount && count > 0 );

                    if ( !garrison.JoinTroop( weakestTroop->GetID(), count, false ) ) {
                        // TODO: there could be more monsters in the hero's army but for now we ignore them.
                        break;
                    }

                    if ( count == initialCount ) {
                        weakestTroop->Reset();
                    }
                    else {
                        weakestTroop->SetCount( initialCount - count );
                    }

                    heroStrength = heroArmy.GetStrength();
                    castleStrength = garrison.GetStrength();
                }

                if ( castleStrength < minStrength ) {
                    // Failed to secure the castle. Rearrange the army back.
                    heroArmy.JoinStrongestFromArmy( garrison );
                }
                else {
                    _priorityTargets.erase( defenseTask );
                }
            }
        }

        const uint32_t regionID = world.GetTiles( castleIndex ).GetRegion();
        // check if we should leave some troops in the garrison
        // TODO: amount of troops left could depend on region's safetyFactor
        if ( castle.isCastle() && _regions[regionID].safetyFactor <= 100 && !garrison.isValid() ) {
            const Heroes::Role heroRole = hero.getAIRole();
            const bool isFigtherHero = ( heroRole == Heroes::Role::FIGHTER || heroRole == Heroes::Role::CHAMPION );

            bool onlyHalf = false;
            Troop * unitToSwap = heroArmy.GetSlowestTroop();
            if ( unitToSwap ) {
                // We need to compare a strength of troops excluding hero's stats.
                const double troopsStrength = Troops( heroArmy.getTroops() ).GetStrength();

                const double significanceRatio = isFigtherHero ? 20.0 : 10.0;
                if ( unitToSwap->GetStrength() > troopsStrength / significanceRatio ) {
                    Troop * weakest = heroArmy.GetWeakestTroop();

                    assert( weakest != nullptr );
                    if ( weakest ) {
                        unitToSwap = weakest;
                        if ( weakest->GetStrength() > troopsStrength / significanceRatio ) {
                            if ( isFigtherHero ) {
                                // if it's an important hero and all troops are significant - keep the army
                                unitToSwap = nullptr;
                            }
                            else {
                                onlyHalf = true;
                            }
                        }
                    }
                }
            }

            if ( unitToSwap ) {
                const uint32_t count = unitToSwap->GetCount();
                const uint32_t toMove = onlyHalf ? count / 2 : count;
                if ( garrison.JoinTroop( unitToSwap->GetMonster(), toMove, true ) ) {
                    if ( !onlyHalf ) {
                        unitToSwap->Reset();
                    }
                    else {
                        unitToSwap->SetCount( count - toMove );
                    }
                }
            }
        }

        OptimizeTroopsOrder( heroArmy );

        if ( garrison.GetOccupiedSlotCount() == 1 ) {
            garrison.splitWeakestTroopsIfPossible();
        }
        else {
            OptimizeTroopsOrder( garrison );
        }
    }

    void Normal::evaluateRegionSafety()
    {
        std::vector<std::pair<size_t, int>> regionsToCheck;
        size_t lastPositive = 0;
        for ( size_t regionID = 0; regionID < _regions.size(); ++regionID ) {
            RegionStats & stats = _regions[regionID];

            if ( ( stats.friendlyCastles && stats.enemyCastles ) || ( stats.highestThreat > 0 && !stats.enemyCastles ) ) {
                // contested space OR enemy heroes invaded our region
                // TODO: assess army strength to get more accurate reading
                stats.safetyFactor = -50;
                stats.evaluated = true;
                regionsToCheck.emplace_back( regionID, -50 );
            }
            else if ( stats.enemyCastles ) {
                // straight up enemy territory
                stats.safetyFactor = -100;
                stats.evaluated = true;
                regionsToCheck.emplace_back( regionID, -100 );
            }
            else if ( stats.friendlyCastles ) {
                // our protected castle
                stats.safetyFactor = 100;
                stats.evaluated = true;
                regionsToCheck.emplace_back( regionID, 100 );
                ++lastPositive;
            }
            else {
                stats.safetyFactor = 0;
                stats.evaluated = false;
            }
        }
        std::sort( regionsToCheck.begin(), regionsToCheck.end(),
                   []( const std::pair<size_t, int> & left, const std::pair<size_t, int> & right ) { return left.second > right.second; } );

        size_t currentEntry = 0;
        size_t batchStart = 0;
        size_t batchEnd = lastPositive + 1;
        while ( currentEntry < regionsToCheck.size() ) {
            const MapRegion & region = world.getRegion( regionsToCheck[currentEntry].first );

            for ( uint32_t secondaryID : region._neighbours ) {
                RegionStats & adjacentStats = _regions[secondaryID];
                if ( !adjacentStats.evaluated ) {
                    adjacentStats.evaluated = true;
                    regionsToCheck.emplace_back( secondaryID, adjacentStats.safetyFactor );
                }

                if ( adjacentStats.safetyFactor != 0 ) {
                    // losing precision due to integer division is intentional here, values should be reduced to 0 eventually
                    regionsToCheck[currentEntry].second += adjacentStats.safetyFactor / static_cast<int>( region.getNeighboursCount() );
                }
            }
            // no neighbours means it is an island; they are usually safer (or more dangerous to explore) due to boat movement penalties
            if ( region.getNeighboursCount() == 0 )
                regionsToCheck[currentEntry].second = regionsToCheck[currentEntry].second * 3 / 2;

            if ( currentEntry == batchEnd - 1 ) {
                // Apply the calculated value in batches
                for ( size_t idx = batchStart; idx < batchEnd; ++idx ) {
                    const size_t regionID = regionsToCheck[idx].first;
                    _regions[regionID].safetyFactor = regionsToCheck[idx].second;
                    DEBUG_LOG( DBG_AI, DBG_TRACE, "Region " << regionID << " safety factor is " << _regions[regionID].safetyFactor )
                }
                batchStart = batchEnd;
                batchEnd = regionsToCheck.size();
            }
            ++currentEntry;
        }
    }

    std::vector<AICastle> Normal::getSortedCastleList( const KingdomCastles & castles, const std::set<int> & castlesInDanger )
    {
        std::vector<AICastle> sortedCastleList;
        for ( Castle * castle : castles ) {
            if ( !castle )
                continue;

            const int32_t castleIndex = castle->GetIndex();
            const uint32_t regionID = world.GetTiles( castleIndex ).GetRegion();
            sortedCastleList.emplace_back( castle, castlesInDanger.count( castleIndex ) > 0, _regions[regionID].safetyFactor, castle->getBuildingValue() );
        }

        std::sort( sortedCastleList.begin(), sortedCastleList.end(), []( const AICastle & left, const AICastle & right ) {
            if ( !left.underThreat && !right.underThreat ) {
                return left.safetyFactor > right.safetyFactor;
            }

            if ( left.underThreat && !right.underThreat ) {
                return true;
            }

            if ( !left.underThreat && right.underThreat ) {
                return false;
            }

            // We have a building value of a castle and its safety factor. The higher safety factor the lower priority to defend the castle.
            // Since we compare 2 castles we need to use safety factor of the opposite castle.
            return left.buildingValue * right.safetyFactor > right.buildingValue * left.safetyFactor;
        } );

        return sortedCastleList;
    }

    std::set<int> Normal::findCastlesInDanger( const KingdomCastles & castles, const std::vector<EnemyArmy> & enemyArmies, int myColor )
    {
        const uint32_t threatDistanceLimit = 3000; // 30 tiles, roughly how much maxed out hero can move in a turn
        std::set<int> castlesInDanger;

        for ( const EnemyArmy & enemyArmy : enemyArmies ) {
            for ( const Castle * castle : castles ) {
                if ( castle == nullptr ) {
                    // How is it even possible? Check the logic!
                    assert( 0 );
                    continue;
                }

                const int castleIndex = castle->GetIndex();
                // skip precise distance check if army is too far to be a threat
                if ( Maps::GetApproximateDistance( enemyArmy.index, castleIndex ) * Maps::Ground::roadPenalty > threatDistanceLimit ) {
                    continue;
                }

                // TODO: if a hero (even a weak one) is blocking the path then the function call below will return 0.
                // TODO: For example if a friendly hero stands just below the castle entrance then the distance will be 0.
                // TODO: Which is not the case as an enemy hero can be very powerful to kill the hero and capture the castle.
                const uint32_t dist = _pathfinder.getDistance( enemyArmy.index, castleIndex, myColor, enemyArmy.strength );
                if ( dist == 0 || dist >= threatDistanceLimit ) {
                    continue;
                }

                uint32_t daysToReach = ( dist + enemyArmy.movePoints - 1 ) / enemyArmy.movePoints;
                if ( daysToReach > 3 ) {
                    // It is too far away. Ignore it.
                    continue;
                }

                double enemyStrength = enemyArmy.strength;

                --daysToReach;
                while ( daysToReach > 0 ) {
                    // Each day reduces enemy strength by 50%. If an enemy is too far away then there is no reason to panic.
                    enemyStrength /= 2;
                    --daysToReach;
                }

                const double defenders = castle->GetArmy().GetStrength();
                const double attackerThreat = enemyStrength - defenders;
                if ( attackerThreat < 0.1 ) {
                    continue;
                }

                // The castle is under threat.
                castlesInDanger.insert( castleIndex );

                auto attackTask = _priorityTargets.find( enemyArmy.index );
                if ( attackTask == _priorityTargets.end() ) {
                    _priorityTargets[enemyArmy.index] = { PriorityTaskType::ATTACK, enemyArmy.strength, castleIndex };
                }
                else {
                    attackTask->second.secondaryTaskTileId.insert( castleIndex );
                }

                auto defenseTask = _priorityTargets.find( castleIndex );
                if ( defenseTask == _priorityTargets.end() ) {
                    _priorityTargets[castleIndex] = { PriorityTaskType::DEFEND, attackerThreat, enemyArmy.index };
                }
                else if ( defenseTask->second.threatLevel < attackerThreat ) {
                    defenseTask->second.secondaryTaskTileId.insert( defenseTask->first );
                    defenseTask->second.threatLevel = attackerThreat;
                }
                else {
                    defenseTask->second.secondaryTaskTileId.insert( enemyArmy.index );
                }
            }
        }

        return castlesInDanger;
    }

    void Normal::KingdomTurn( Kingdom & kingdom )
    {
        const int myColor = kingdom.GetColor();

        if ( kingdom.isLoss() || myColor == Color::NONE ) {
            kingdom.LossPostActions();
            return;
        }

        // reset indicator
        Interface::StatusWindow & status = Interface::Basic::Get().GetStatusWindow();
        status.DrawAITurnProgress( 0 );

        AudioManager::PlayMusicAsync( MUS::COMPUTER_TURN, Music::PlaybackMode::RESUME_AND_PLAY_INFINITE );

        KingdomHeroes & heroes = kingdom.GetHeroes();
        const KingdomCastles & castles = kingdom.GetCastles();

        // Clear the cache of neutral monsters as their strength might have changed.
        _neutralMonsterStrengthCache.clear();

        DEBUG_LOG( DBG_AI, DBG_INFO, Color::String( myColor ) << " starts the turn: " << castles.size() << " castles, " << heroes.size() << " heroes" )
        DEBUG_LOG( DBG_AI, DBG_INFO, "Funds: " << kingdom.GetFunds().String() )

        // Step 1. Scan visible map (based on game difficulty), add goals and threats
        bool underViewSpell = false;
        int32_t availableHeroCount = 0;
        Heroes * bestHeroToViewAll = nullptr;

        for ( Heroes * hero : heroes ) {
            hero->ResetModes( Heroes::SLEEPER );

            const double strength = hero->GetArmy().GetStrength();
            _combinedHeroStrength += strength;
            if ( !hero->Modes( Heroes::PATROL ) )
                ++availableHeroCount;

            if ( hero->HaveSpell( Spell::VIEWALL ) && ( !bestHeroToViewAll || hero->HasSecondarySkill( Skill::Secondary::MYSTICISM ) ) ) {
                bestHeroToViewAll = hero;
            }
        }

        if ( bestHeroToViewAll && HeroesCastAdventureSpell( *bestHeroToViewAll, Spell::VIEWALL ) ) {
            underViewSpell = true;
        }

        std::vector<EnemyArmy> enemyArmies;

        const int mapSize = world.w() * world.h();
        _priorityTargets.clear();
        _mapActionObjects.clear();
        _regions.clear();
        _regions.resize( world.getRegionCount() );

        for ( int idx = 0; idx < mapSize; ++idx ) {
            const Maps::Tiles & tile = world.GetTiles( idx );
            MP2::MapObjectType objectType = tile.GetObject();

            const uint32_t regionID = tile.GetRegion();
            if ( regionID >= _regions.size() ) {
                // shouldn't be possible, assert
                assert( regionID < _regions.size() );
                continue;
            }

            RegionStats & stats = _regions[regionID];
            if ( !underViewSpell && tile.isFog( myColor ) ) {
                ++stats.fogCount;
                continue;
            }

            if ( !MP2::isActionObject( objectType ) ) {
                continue;
            }

            _mapActionObjects.emplace_back( idx, objectType );

            if ( objectType == MP2::OBJ_HEROES ) {
                const Heroes * hero = tile.GetHeroes();
                if ( !hero )
                    continue;

                if ( hero->GetColor() == myColor && !hero->Modes( Heroes::PATROL ) ) {
                    ++stats.friendlyHeroes;

                    const int wisdomLevel = hero->GetLevelSkill( Skill::Secondary::WISDOM );
                    if ( wisdomLevel + 2 > stats.spellLevel )
                        stats.spellLevel = wisdomLevel + 2;
                }
                else if ( !Players::isFriends( myColor, hero->GetColor() ) && ( !hero->Modes( Heroes::PATROL ) || hero->GetPatrolDistance() != 0 ) ) {
                    const double heroThreat = hero->GetArmy().GetStrength();

                    enemyArmies.emplace_back( idx, heroThreat, hero->GetMaxMovePoints() );
                    if ( stats.highestThreat < heroThreat ) {
                        stats.highestThreat = heroThreat;
                    }
                }
                // check object underneath the hero as well (maybe a castle)
                objectType = tile.GetObject( false );
            }

            if ( objectType == MP2::OBJ_CASTLE ) {
                const int tileColor = getColorFromTile( tile );
                if ( myColor == tileColor || Players::isFriends( myColor, tileColor ) ) {
                    ++stats.friendlyCastles;
                }
                else if ( tileColor != Color::NONE ) {
                    ++stats.enemyCastles;

                    const Castle * castle = world.getCastleEntrance( Maps::GetPoint( idx ) );
                    if ( !castle )
                        continue;

                    if ( !castle->isCastle() && !castle->AllowBuyBuilding( BUILD_CASTLE ) ) {
                        // If it is just a town with no possibility to build a castle then there is no way to hire heroes which can be a threat.
                        continue;
                    }

                    const double castleThreat = castle->GetArmy().GetStrength();
                    // 1500 is slightly more than a fresh hero's maximum move points hired in a castle.
                    enemyArmies.emplace_back( idx, castleThreat, 1500 );

                    if ( stats.highestThreat < castleThreat ) {
                        stats.highestThreat = castleThreat;
                    }
                }
            }
            else if ( objectType == MP2::OBJ_MONSTER ) {
                stats.averageMonster += Army( tile ).GetStrength();
                ++stats.monsterCount;
            }
        }

        evaluateRegionSafety();

        updateKingdomBudget( kingdom );

        DEBUG_LOG( DBG_AI, DBG_TRACE, Color::String( myColor ) << " found " << _mapActionObjects.size() << " valid objects" )

        uint32_t progressStatus = 1;
        status.DrawAITurnProgress( progressStatus );

        std::vector<AICastle> sortedCastleList;
        std::set<int> castlesInDanger;
        while ( true ) {
            // Step 2. Do some hero stuff.
            // If a hero is standing in a castle most likely he has nothing to do so let's try to give him more army.
            for ( Heroes * hero : heroes ) {
                HeroesActionComplete( *hero, hero->GetIndex(), MP2::OBJ_NONE );
            }

            // Step 3. Reassign heroes roles
            setHeroRoles( heroes );

            castlesInDanger = findCastlesInDanger( castles, enemyArmies, myColor );
            for ( Heroes * hero : heroes ) {
                if ( hero->GetMapsObject() == MP2::OBJ_CASTLE && _priorityTargets.find( hero->GetIndex() ) != _priorityTargets.end() ) {
                    // If a hero is in a castle and it is in danger then the hero is very weak to defend it.
                    // Therefore let's make him stay in the castle.
                    // TODO: allow the hero to still do some actions but always return to the castle at the end of the turn.

                    HeroesActionComplete( *hero, hero->GetIndex(), hero->GetMapsObject() );
                }
            }

            sortedCastleList = getSortedCastleList( castles, castlesInDanger );

            const uint32_t startProgressValue = progressStatus;
            const uint32_t endProgressValue = ( progressStatus == 1 ) ? 8 : std::max( progressStatus + 1U, 9U );

            bool moreTaskForHeroes = HeroesTurn( heroes, startProgressValue, endProgressValue );

            if ( progressStatus == 1 ) {
                progressStatus = 8;
                status.DrawAITurnProgress( progressStatus );
            }

            // Step 4. Buy new heroes, adjust roles, sort heroes based on priority or strength
            if ( purchaseNewHeroes( sortedCastleList, castlesInDanger, availableHeroCount, moreTaskForHeroes ) ) {
                assert( !heroes.empty() && heroes.back() != nullptr );
                updateMapActionObjectCache( heroes.back()->GetIndex() );
                ++availableHeroCount;
                continue;
            }

            if ( !moreTaskForHeroes && world.LastDay() ) {
                // Heroes have nothing to do. In this case it is wise to move heroes to castles especially if it is the last day of a week.
                // So for the next day a hero will have a maximum amount of spell points as well as new troops.
                for ( const Castle * castle : castles ) {
                    if ( castle->GetHero() == nullptr ) {
                        const auto [dummy, inserted] = _priorityTargets.try_emplace( castle->GetIndex(), PriorityTaskType::REINFORCE, 0 );
                        if ( inserted ) {
                            moreTaskForHeroes = true;
                        }
                    }
                }

                if ( moreTaskForHeroes ) {
                    continue;
                }
            }

            break;
        }

        status.DrawAITurnProgress( 9 );

        // sync up castle list (if conquered new ones during the turn)
        if ( castles.size() != sortedCastleList.size() ) {
            evaluateRegionSafety();
            sortedCastleList = getSortedCastleList( castles, castlesInDanger );
        }

        // Step 5. Castle development according to kingdom budget
        for ( const AICastle & entry : sortedCastleList ) {
            if ( entry.castle != nullptr ) {
                CastleTurn( *entry.castle, entry.underThreat );
            }
        }

        status.DrawAITurnProgress( 10 );
    }

    bool Normal::purchaseNewHeroes( const std::vector<AICastle> & sortedCastleList, const std::set<int> & castlesInDanger, const int32_t availableHeroCount,
                                    const bool moreTasksForHeroes )
    {
        const bool slowEarlyGame = world.CountDay() < 5 && sortedCastleList.size() == 1;
        int32_t heroLimit = world.w() / Maps::SMALL + 1;

        if ( _personality == EXPLORER )
            ++heroLimit;
        if ( slowEarlyGame )
            heroLimit = 2;

        if ( availableHeroCount >= heroLimit ) {
            return false;
        }

        Castle * recruitmentCastle = nullptr;
        double bestArmyAvailable = -1.0;

        // search for best castle to recruit hero from
        for ( const AICastle & entry : sortedCastleList ) {
            Castle * castle = entry.castle;
            if ( castle && castle->isCastle() ) {
                const Heroes * hero = castle->GetHero();
                const int mapIndex = castle->GetIndex();

                // Make sure there is no hero in castle already and we're not under threat while having other heroes.
                if ( hero != nullptr || ( availableHeroCount > 0 && castlesInDanger.find( mapIndex ) != castlesInDanger.end() ) )
                    continue;

                const uint32_t regionID = world.GetTiles( mapIndex ).GetRegion();
                const int heroesInRegion = _regions[regionID].friendlyHeroes;

                if ( heroesInRegion > 1 )
                    continue;

                const size_t neighboursCount = world.getRegion( regionID ).getNeighboursCount();

                // don't buy another hero if there's nothing to do or castle is on an island
                if ( heroesInRegion > 0 && ( !moreTasksForHeroes || ( sortedCastleList.size() > 1 && neighboursCount == 0 ) ) ) {
                    continue;
                }

                const double availableArmy = castle->getArmyRecruitmentValue();

                if ( recruitmentCastle == nullptr || availableArmy > bestArmyAvailable ) {
                    recruitmentCastle = castle;
                    bestArmyAvailable = availableArmy;
                }
            }
        }

        // target found, buy hero
        return recruitmentCastle && recruitHero( *recruitmentCastle, !slowEarlyGame, false );
    }
}
