/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2020 - 2022                                             *
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

#include <cassert>

#include "agg.h"
#include "ai_normal.h"
#include "game_interface.h"
#include "ground.h"
#include "logging.h"
#include "mus.h"
#include "world.h"

namespace
{
    const double fighterStrengthMultiplier = 3;

    void setHeroRoles( KingdomHeroes & heroes )
    {
        if ( heroes.empty() ) {
            // No heroes exist.
            return;
        }

        if ( heroes.size() == 1 ) {
            // A single hero has no roles.
            heroes[0]->setAIRole( Heroes::Role::HUNTER );
            return;
        }

        // Set hero's roles. First calculate each hero strength and sort it in descending order.
        std::vector<std::pair<double, Heroes *>> heroStrength;
        for ( Heroes * hero : heroes ) {
            heroStrength.emplace_back( hero->GetArmy().GetStrength(), hero );
        }

        std::sort( heroStrength.begin(), heroStrength.end(),
                   []( const std::pair<double, Heroes *> & first, const std::pair<double, Heroes *> & second ) { return first.first > second.first; } );

        const double medianStrength = heroStrength[heroStrength.size() / 2].first;

        for ( std::pair<double, Heroes *> & hero : heroStrength ) {
            if ( hero.first > medianStrength * fighterStrengthMultiplier ) {
                hero.second->setAIRole( Heroes::Role::FIGHTER );
            }
            else {
                hero.second->setAIRole( Heroes::Role::HUNTER );
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
        Heroes * firstRecruit = rec.GetHero1();
        Heroes * secondRecruit = rec.GetHero2();
        if ( firstRecruit && secondRecruit && secondRecruit->getRecruitValue() > firstRecruit->getRecruitValue() ) {
            recruit = castle.RecruitHero( secondRecruit );
        }
        else {
            recruit = castle.RecruitHero( firstRecruit );
        }

        if ( recruit && buyArmy ) {
            CastleTurn( castle, underThreat );
            ReinforceHeroInCastle( *recruit, castle, kingdom.GetFunds() );
        }

        return recruit != nullptr;
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
                    DEBUG_LOG( DBG_AI, DBG_TRACE, "Region " << regionID << " safety factor is " << _regions[regionID].safetyFactor );
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
            return left.buildingValue > right.buildingValue;
        } );

        return sortedCastleList;
    }

    std::set<int> Normal::findCastlesInDanger( const KingdomCastles & castles, const std::vector<std::pair<int, const Army *>> & enemyArmies, int myColor )
    {
        const uint32_t threatDistanceLimit = 2500; // 25 tiles, roughly how much maxed out hero can move in a turn
        std::set<int> castlesInDanger;

        for ( const std::pair<int, const Army *> & enemy : enemyArmies ) {
            if ( enemy.second == nullptr )
                continue;

            const double attackerStrength = enemy.second->GetStrength();

            for ( const Castle * castle : castles ) {
                if ( !castle )
                    continue;

                const int castleIndex = castle->GetIndex();
                // skip precise distance check if army is too far to be a threat
                if ( Maps::GetApproximateDistance( enemy.first, castleIndex ) * Maps::Ground::roadPenalty > threatDistanceLimit )
                    continue;

                const double defenders = castle->GetArmy().GetStrength();

                const double attackerThreat = attackerStrength - defenders;
                if ( attackerThreat > 0 ) {
                    const uint32_t dist = _pathfinder.getDistance( enemy.first, castleIndex, myColor, attackerStrength );
                    if ( dist && dist < threatDistanceLimit ) {
                        // castle is under threat
                        castlesInDanger.insert( castleIndex );
                    }
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
        status.RedrawTurnProgress( 0 );

        AGG::PlayMusic( MUS::COMPUTER_TURN, true, true );

        KingdomHeroes & heroes = kingdom.GetHeroes();
        const KingdomCastles & castles = kingdom.GetCastles();

        DEBUG_LOG( DBG_AI, DBG_INFO, Color::String( myColor ) << " starts the turn: " << castles.size() << " castles, " << heroes.size() << " heroes" );
        DEBUG_LOG( DBG_AI, DBG_TRACE, "Funds: " << kingdom.GetFunds().String() );


        uint8_t purchasedHeroesCount;

        do
        {
            purchasedHeroesCount = 0;
            
            // Step 1. Scan visible map (based on game difficulty), add goals and threats
            std::vector<std::pair<int, const Army *>> enemyArmies;

            const int mapSize = world.w() * world.h();
            _mapObjects.clear();
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
                if ( objectType != MP2::OBJ_COAST )
                    stats.validObjects.emplace_back( idx, objectType );

                if ( !tile.isFog( myColor ) ) {
                    _mapObjects.emplace_back( idx, objectType );

                    const int tileColor = tile.QuantityColor();
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
                        else if ( !Players::isFriends( myColor, hero->GetColor() ) ) {
                            const Army & heroArmy = hero->GetArmy();
                            enemyArmies.emplace_back( idx, &heroArmy );

                            const double heroThreat = heroArmy.GetStrength();
                            if ( stats.highestThreat < heroThreat ) {
                                stats.highestThreat = heroThreat;
                            }
                        }
                        // check object underneath the hero as well (maybe a castle)
                        objectType = tile.GetObject( false );
                    }

                    if ( objectType == MP2::OBJ_CASTLE ) {
                        if ( myColor == tileColor || Players::isFriends( myColor, tileColor ) ) {
                            ++stats.friendlyCastles;
                        }
                        else if ( tileColor != Color::NONE ) {
                            ++stats.enemyCastles;

                            const Castle * castle = world.getCastleEntrance( Maps::GetPoint( idx ) );
                            if ( !castle )
                                continue;

                            const Army & castleArmy = castle->GetArmy();
                            enemyArmies.emplace_back( idx, &castleArmy );

                            const double castleThreat = castleArmy.GetStrength();
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
                else {
                    ++stats.fogCount;
                }
            }

            evaluateRegionSafety();

            DEBUG_LOG( DBG_AI, DBG_TRACE, Color::String( myColor ) << " found " << _mapObjects.size() << " valid objects" );

            status.RedrawTurnProgress( 1 );

            // Step 2. Update AI variables and recalculate resource budget
            int32_t availableHeroCount = 0;

            for ( Heroes * hero : heroes ) {
                _combinedHeroStrength += hero->GetArmy().GetStrength();
                if ( !hero->Modes( Heroes::PATROL ) )
                    ++availableHeroCount;
            }

            const std::set<int> castlesInDanger = findCastlesInDanger( castles, enemyArmies, myColor );

            // Step 3. Do some hero stuff.

            // If a hero is standing in a castle most likely he has nothing to do so let's try to give him more army.
            for ( Heroes * hero : heroes ) {
                HeroesActionComplete( *hero );
            }

            setHeroRoles( heroes );

            status.RedrawTurnProgress( 6 );

            // Step 4. Buy new heroes, adjust roles, sort heroes based on priority or strength
            std::vector<AICastle> sortedCastleList = getSortedCastleList( castles, castlesInDanger );

            purchasedHeroesCount = purchaseNewHeroes(heroes, sortedCastleList, castlesInDanger, availableHeroCount);

            status.RedrawTurnProgress( 7 );

            // Step 5. Move newly hired heroes if any.
            setHeroRoles( heroes );

            HeroesTurn( heroes );

            status.RedrawTurnProgress( 9 );

            // sync up castle list (if conquered new ones during the turn)
            if ( castles.size() != sortedCastleList.size() ) {
                evaluateRegionSafety();
                sortedCastleList = getSortedCastleList( castles, castlesInDanger );
            }

            // Step 6. Castle development according to kingdom budget
            for ( const AICastle & entry : sortedCastleList ) {
                if ( entry.castle != nullptr ) {
                    CastleTurn( *entry.castle, entry.underThreat );
                }
            }

        } while(purchasedHeroesCount != 0); // If we have bought new heroes during this turn let's repeat the whole process. 
    }

	uint8_t Normal::purchaseNewHeroes(VecHeroes &heroes, const std::vector<AICastle> &sortedCastleList, const std::set<int> &castlesInDanger, int32_t availableHeroCount)
	{
    const bool slowEarlyGame = world.CountDay() < 5 && sortedCastleList.size() == 1;
    int32_t heroLimit = world.w() / Maps::SMALL + 1;
    const bool moreTasksForHeroes = HeroesTurn( heroes );

    if ( _personality == EXPLORER )
        ++heroLimit;
    if ( slowEarlyGame )
        heroLimit = 2;

 		if ( availableHeroCount < heroLimit ) {
            Castle * recruitmentCastle = nullptr;
            double bestArmyAvailable = -1.0;

            // search for best castle to recruit hero from
            for ( const AICastle & entry : sortedCastleList ) {
                Castle * castle = entry.castle;
                if ( castle && castle->isCastle() ) {
                    const Heroes * hero = castle->GetHeroes().Guest();
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
            if ( recruitmentCastle && recruitHero( *recruitmentCastle, !slowEarlyGame, false ) ) {
                ++availableHeroCount;
                return true;
            }
        }
        return false;
	  }

}
