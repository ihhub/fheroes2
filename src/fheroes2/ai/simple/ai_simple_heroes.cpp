/********************************************************************************
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>               *
 *   All rights reserved.                                                       *
 *                                                                              *
 *   Part of the Free Heroes2 Engine:                                           *
 *   http://sourceforge.net/projects/fheroes2                                   *
 *                                                                              *
 *   Redistribution and use in source and binary forms, with or without         *
 *   modification, are permitted provided that the following conditions         *
 *   are met:                                                                   *
 *   - Redistributions may not be sold, nor may they be used in a               *
 *     commercial product or activity.                                          *
 *   - Redistributions of source code and/or in binary form must reproduce      *
 *     the above copyright notice, this list of conditions and the              *
 *     following disclaimer in the documentation and/or other materials         *
 *     provided with the distribution.                                          *
 *                                                                              *
 * THIS SOFTWARE IS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,   *
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS    *
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT     *
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,        *
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, *
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;  *
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,     *
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE         *
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,            *
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                           *
 *******************************************************************************/

#include <algorithm>
#include <functional>

#include "ai_simple.h"
#include "army.h"
#include "battle.h"
#include "castle.h"
#include "cursor.h"
#include "difficulty.h"
#include "game_interface.h"
#include "heroes.h"
#include "interface_gamearea.h"
#include "kingdom.h"
#include "luck.h"
#include "maps_tiles.h"
#include "morale.h"
#include "payment.h"
#include "race.h"
#include "settings.h"
#include "world.h"

namespace AI
{
    static const size_t HERO_MAX_SHEDULED_TASK = 7;
    static const uint32_t PATHFINDING_LIMIT = 30;

    bool Simple::HeroesSkipFog( void )
    {
        return false;
    }

    std::string Simple::HeroesString( const Heroes & hero )
    {
        std::ostringstream os;

        AIHero & ai_hero = GetHero( hero );
        Queue & task = ai_hero.sheduled_visit;

        os << "flags           : " << ( hero.Modes( AI::HERO_SCOUT ) ? "SCOUTER," : "" ) << ( hero.Modes( AI::HERO_HUNTER ) ? "HUNTER," : "" )
           << ( hero.Modes( AI::HERO_WAITING ) ? "WAITING," : "" ) << ( hero.Modes( AI::HERO_SKIP_TURN ) ? "STUPID" : "" ) << std::endl;

        os << "ai primary target: " << ai_hero.primary_target << std::endl << "ai sheduled visit: ";
        for ( Queue::const_iterator it = task.begin(); it != task.end(); ++it )
            os << it->first << "(" << MP2::StringObject( world.GetTiles( it->first ).GetObject() ) << "), ";
        os << std::endl;

        return os.str();
    }

    void Simple::HeroesPostLoad( Heroes & hero )
    {
        hero.SetModes( AI::HERO_HUNTER );
    }

    void Simple::HeroesLevelUp( Heroes & hero )
    {
        if ( 4 < hero.GetLevel() && !hero.Modes( AI::HERO_HUNTER ) )
            hero.SetModes( AI::HERO_HUNTER );

        if ( 9 < hero.GetLevel() && hero.Modes( AI::HERO_SCOUT ) )
            hero.ResetModes( AI::HERO_SCOUT );
    }

    void Simple::HeroesPreBattle( HeroBase & hero )
    {
        Castle * castle = world.GetCastle( hero.GetCenter() );
        if ( castle && hero.GetType() != HeroBase::CAPTAIN )
            hero.GetArmy().JoinTroops( castle->GetArmy() );
    }

    void Simple::HeroesAfterBattle( HeroBase & hero ) {}

    void Simple::HeroesClearTask( const Heroes & hero )
    {
        GetHero( hero ).ClearTasks();
    }

    bool AIHeroesValidObject2( const Heroes * hero, s32 index )
    {
        const Heroes & hero2 = *hero;
        return AI::HeroesValidObject( hero2, index );
    }

    // get priority object for AI independent of distance (1 day)
    bool AIHeroesPriorityObject( const Heroes & hero, s32 index )
    {
        Maps::Tiles & tile = world.GetTiles( index );

        if ( MP2::OBJ_CASTLE == tile.GetObject() ) {
            const Castle * castle = world.GetCastle( Maps::GetPoint( index ) );
            if ( castle ) {
                if ( hero.GetColor() == castle->GetColor() ) {
                    // maybe need join army
                    return hero.Modes( AI::HERO_HUNTER ) && castle->GetArmy().isValid() && !hero.isVisited( world.GetTiles( castle->GetIndex() ) );
                }
                else if ( !hero.isFriends( castle->GetColor() ) )
                    return AI::HeroesValidObject( hero, index );
            }
        }
        else if ( MP2::OBJ_HEROES == tile.GetObject() ) {
            // kill enemy hero
            const Heroes * hero2 = tile.GetHeroes();
            return hero2 && !hero.isFriends( hero2->GetColor() ) && AI::HeroesValidObject( hero, index );
        }

        switch ( tile.GetObject() ) {
        case MP2::OBJ_MONSTER:
        case MP2::OBJ_SAWMILL:
        case MP2::OBJ_MINES:
        case MP2::OBJ_ALCHEMYLAB:

        case MP2::OBJ_ARTIFACT:
        case MP2::OBJ_RESOURCE:
        case MP2::OBJ_CAMPFIRE:
        case MP2::OBJ_TREASURECHEST:

            return AI::HeroesValidObject( hero, index );

        default:
            break;
        }

        return false;
    }

    s32 FindUncharteredTerritory( Heroes & hero, u32 scoute )
    {
        Maps::Indexes v = Maps::GetAroundIndexes( hero.GetIndex(), scoute, true );
        Maps::Indexes res;

        v.resize( std::distance( v.begin(), std::remove_if( v.begin(), v.end(), std::ptr_fun( &Maps::TileIsUnderProtection ) ) ) );

        for ( MapsIndexes::const_reverse_iterator it = v.rbegin(); it != v.rend() && res.size() < 4; ++it ) {
            // find fogs
            if ( world.GetTiles( *it ).isFog( hero.GetColor() ) && world.GetTiles( *it ).isPassable( Direction::CENTER, hero.isShipMaster(), true )
                 && hero.GetPath().Calculate( *it ) )
                res.push_back( *it );
        }

        const s32 result = res.size() ? *Rand::Get( res ) : -1;

        if ( 0 <= result ) {
            DEBUG( DBG_AI, DBG_INFO, Color::String( hero.GetColor() ) << ", hero: " << hero.GetName() << ", added task: " << result );
        }

        return result;
    }

    s32 GetRandomHeroesPosition( Heroes & hero, u32 scoute )
    {
        Maps::Indexes v = Maps::GetAroundIndexes( hero.GetIndex(), scoute, true );
        Maps::Indexes res;

        v.resize( std::distance( v.begin(), std::remove_if( v.begin(), v.end(), std::ptr_fun( &Maps::TileIsUnderProtection ) ) ) );

        for ( MapsIndexes::const_reverse_iterator it = v.rbegin(); it != v.rend() && res.size() < 4; ++it ) {
            if ( world.GetTiles( *it ).isPassable( Direction::CENTER, hero.isShipMaster(), true ) && hero.GetPath().Calculate( *it ) )
                res.push_back( *it );
        }

        const s32 result = res.size() ? *Rand::Get( res ) : -1;

        if ( 0 <= result ) {
            DEBUG( DBG_AI, DBG_INFO, Color::String( hero.GetColor() ) << ", hero: " << hero.GetName() << ", added task: " << result );
        }

        return result;
    }

    void Simple::HeroesAddedRescueTask( Heroes & hero )
    {
        AIHero & ai_hero = GetHero( hero );
        Queue & task = ai_hero.sheduled_visit;

        DEBUG( DBG_AI, DBG_TRACE, hero.GetName() );

        u32 scoute = hero.GetScoute();

        switch ( Settings::Get().GameDifficulty() ) {
        case Difficulty::NORMAL:
            scoute += 2;
            break;
        case Difficulty::HARD:
            scoute += 3;
            break;
        case Difficulty::EXPERT:
            scoute += 4;
            break;
        case Difficulty::IMPOSSIBLE:
            scoute += 6;
            break;
        default:
            break;
        }

        // find unchartered territory
        s32 index = FindUncharteredTerritory( hero, scoute );
        const Maps::Tiles & tile = world.GetTiles( hero.GetIndex() );

        if ( index < 0 ) {
            // check teleports
            if ( MP2::OBJ_STONELIGHTS == tile.GetObject( false ) || MP2::OBJ_WHIRLPOOL == tile.GetObject( false ) ) {
                AI::HeroesAction( hero, hero.GetIndex() );
            }
            else {
                // random
                index = GetRandomHeroesPosition( hero, scoute );
            }
        }

        if ( 0 <= index )
            task.push_back( IndexDistance( index, Maps::GetApproximateDistance( hero.GetIndex(), index ) * 100 ) );
    }

    void Simple::HeroesAddedTask( Heroes & hero )
    {
        AIHero & ai_hero = GetHero( hero );
        AIKingdom & ai_kingdom = GetKingdom( hero.GetColor() );

        Queue & task = ai_hero.sheduled_visit;
        IndexObjectMap & ai_objects = ai_kingdom.scans;

        // load minimal distance tasks
        std::vector<IndexDistance> objs;

        for ( std::map<s32, int>::const_iterator it = ai_objects.begin(); it != ai_objects.end(); ++it ) {
            const Maps::Tiles & tile = world.GetTiles( ( *it ).first );

            if ( hero.isShipMaster() ) {
                if ( MP2::OBJ_COAST != tile.GetObject() && !tile.isWater() )
                    continue;

                // check previous positions
                if ( MP2::OBJ_COAST == ( *it ).second && hero.isVisited( world.GetTiles( ( *it ).first ) ) )
                    continue;
            }
            else {
                if ( tile.isWater() && MP2::OBJ_BOAT != tile.GetObject() )
                    continue;
            }

            const uint32_t heuristic = Maps::GetApproximateDistance( hero.GetIndex(), ( *it ).first );
            if ( heuristic < PATHFINDING_LIMIT && AI::HeroesValidObject( hero, ( *it ).first ) ) {
                objs.push_back( IndexDistance( ( *it ).first, heuristic ) );
            }
        }

        DEBUG( DBG_AI, DBG_INFO, Color::String( hero.GetColor() ) << ", hero: " << hero.GetName() << ", task prepare: " << objs.size() );

        std::sort( objs.begin(), objs.end(), IndexDistance::Shortest );

        for ( std::vector<IndexDistance>::const_iterator it = objs.begin(); it != objs.end(); ++it ) {
            if ( task.size() >= HERO_MAX_SHEDULED_TASK )
                break;
            const int positionIndex = ( *it ).first;
            const uint32_t distance = hero.GetPath().Calculate( positionIndex, PATHFINDING_LIMIT );

            if ( distance ) {
                DEBUG( DBG_AI, DBG_INFO,
                       Color::String( hero.GetColor() ) << ", hero: " << hero.GetName() << ", added task: " << MP2::StringObject( ai_objects[positionIndex] )
                                                        << ", index: " << positionIndex << ", distance: " << distance );

                task.push_back( IndexDistance( positionIndex, distance ) );
                ai_objects.erase( ( *it ).first );
            }
            else {
                DEBUG( DBG_AI, DBG_TRACE,
                       Color::String( hero.GetColor() ) << ", hero: " << hero.GetName() << ", impossible: " << MP2::StringObject( ai_objects[positionIndex] )
                                                        << ", index: " << positionIndex << ", distance: " << distance );
            }
        }

        if ( task.empty() )
            HeroesAddedRescueTask( hero );
    }

    void Simple::HeroesActionNewPosition( Heroes & hero ) {}

    bool Simple::HeroesGetTask( Heroes & hero )
    {
        std::vector<s32> results;
        results.reserve( 5 );

        const Settings & conf = Settings::Get();
        AIHero & ai_hero = GetHero( hero );
        AIKingdom & ai_kingdom = GetKingdom( hero.GetColor() );

        Queue & task = ai_hero.sheduled_visit;
        IndexObjectMap & ai_objects = ai_kingdom.scans;

        const int heroIndex = hero.GetIndex();

        const u8 objs1[] = {MP2::OBJ_ARTIFACT, MP2::OBJ_RESOURCE, MP2::OBJ_CAMPFIRE, MP2::OBJ_TREASURECHEST, 0};
        const u8 objs2[] = {MP2::OBJ_SAWMILL, MP2::OBJ_MINES, MP2::OBJ_ALCHEMYLAB, 0};
        const u8 objs3[] = {MP2::OBJ_CASTLE, MP2::OBJ_HEROES, MP2::OBJ_MONSTER, 0};

        // rescan path
        hero.RescanPath();

        Castle * castle = hero.inCastle();
        // if hero in castle
        if ( castle ) {
            DEBUG( DBG_AI, DBG_TRACE, hero.GetName() << ", in castle" );

            castle->RecruitAllMonsters();
            hero.GetArmy().UpgradeTroops( *castle );

            // recruit army
            if ( hero.Modes( AI::HERO_HUNTER ) )
                hero.GetArmy().JoinStrongestFromArmy( castle->GetArmy() );
            else if ( hero.Modes( AI::HERO_SCOUT ) )
                hero.GetArmy().KeepOnlyWeakestTroops( castle->GetArmy() );

            DEBUG( DBG_AI, DBG_TRACE, hero.GetName() << ", " << hero.GetArmy().String() );
        }

        // patrol task
        if ( hero.Modes( Heroes::PATROL ) ) {
            DEBUG( DBG_AI, DBG_TRACE, hero.GetName() << ", is in patrol mode" );

            // goto patrol center
            if ( hero.GetCenterPatrol() != hero.GetCenter() && hero.GetPath().Calculate( Maps::GetIndexFromAbsPoint( hero.GetCenterPatrol() ) ) )
                return true;

            // scan enemy hero
            if ( hero.GetSquarePatrol() ) {
                const Maps::Indexes & mapIndices
                    = Maps::ScanAroundObject( Maps::GetIndexFromAbsPoint( hero.GetCenterPatrol() ), hero.GetSquarePatrol(), MP2::OBJ_HEROES );
                for ( MapsIndexes::const_iterator it = mapIndices.begin(); it != mapIndices.end(); ++it ) {
                    const Heroes * enemy = world.GetTiles( *it ).GetHeroes();
                    if ( enemy && !enemy->isFriends( hero.GetColor() ) ) {
                        if ( hero.GetPath().Calculate( enemy->GetIndex() ) ) {
                            DEBUG( DBG_AI, DBG_TRACE, hero.GetName() << ", enemy found: " << enemy->GetName() );
                            return true;
                        }
                    }
                }
            }

            // can pickup objects
            if ( conf.ExtHeroPatrolAllowPickup() ) {
                const Maps::Indexes & mapIndices = Maps::ScanAroundObjects( heroIndex, hero.GetSquarePatrol(), objs1 );
                for ( MapsIndexes::const_iterator it = mapIndices.begin(); it != mapIndices.end(); ++it )
                    if ( AI::HeroesValidObject( hero, *it ) && hero.GetPath().Calculate( *it ) ) {
                        ai_objects.erase( *it );

                        DEBUG( DBG_AI, DBG_TRACE, hero.GetName() << ": object found: " << MP2::StringObject( world.GetTiles( *it ).GetObject() ) << "(" << *it << ")" );
                        return true;
                    }
            }

            hero.SetModes( AI::HERO_SKIP_TURN );
            return false;
        }

        if ( ai_hero.fix_loop > 3 ) {
            DEBUG( DBG_AI, DBG_TRACE, hero.GetName() << ": hero can't find a task, break loop" );
            hero.SetModes( hero.Modes( AI::HERO_WAITING ) ? AI::HERO_SKIP_TURN : AI::HERO_WAITING );
            return false;
        }

        // primary target
        if ( Maps::isValidAbsIndex( ai_hero.primary_target ) ) {
            if ( heroIndex == ai_hero.primary_target ) {
                ai_hero.primary_target = -1;
                hero.GetPath().Reset();
                DEBUG( DBG_AI, DBG_TRACE, hero.GetName() << ", reset path" );
            }
            else {
                DEBUG( DBG_AI, DBG_TRACE,
                       hero.GetName() << ", primary target: " << ai_hero.primary_target << ", "
                                      << MP2::StringObject( world.GetTiles( ai_hero.primary_target ).GetObject() ) );

                const Castle * castle = NULL;

                if ( NULL != ( castle = world.GetCastle( Maps::GetPoint( ai_hero.primary_target ) ) ) && NULL != castle->GetHeroes().Guest()
                     && castle->isFriends( hero.GetColor() ) ) {
                    hero.SetModes( AI::HERO_WAITING );
                    DEBUG( DBG_AI, DBG_TRACE, hero.GetName() << ", castle busy.." );
                }

                // make path
                if ( ai_hero.primary_target != hero.GetPath().GetDestinationIndex() && !hero.GetPath().Calculate( ai_hero.primary_target ) ) {
                    DEBUG( DBG_AI, DBG_TRACE, hero.GetName() << ", path unknown, primary target reset" );
                    ai_hero.primary_target = -1;
                }
            }

            if ( hero.GetPath().isValid() )
                return true;
        }

        // scan heroes and castle
        const Maps::Indexes & enemies = Maps::ScanAroundObjects( heroIndex, hero.GetScoute(), objs3 );

        for ( MapsIndexes::const_iterator it = enemies.begin(); it != enemies.end(); ++it )
            if ( AIHeroesPriorityObject( hero, *it ) && hero.GetPath().Calculate( *it ) ) {
                DEBUG( DBG_AI, DBG_TRACE, hero.GetName() << ", set primary target: " << MP2::StringObject( world.GetTiles( *it ).GetObject() ) << "(" << *it << ")" );

                ai_hero.primary_target = *it;
                return true;
            }

        // check destination
        if ( hero.GetPath().isValid() ) {
            if ( !AI::HeroesValidObject( hero, hero.GetPath().GetDestinationIndex() ) )
                hero.GetPath().Reset();
            else if ( hero.GetPath().size() < 5 ) {
                DEBUG( DBG_AI, DBG_TRACE, hero.GetName() << ", continue on short path" );
                ai_hero.fix_loop++;
                return true;
            }
        }

        // scan 2x2 pickup objects
        Maps::Indexes pickups = Maps::ScanAroundObjects( heroIndex, 2, objs1 );
        // scan 3x3 capture objects
        const Maps::Indexes & captures = Maps::ScanAroundObjects( heroIndex, 3, objs2 );
        if ( captures.size() )
            pickups.insert( pickups.end(), captures.begin(), captures.end() );

        if ( pickups.size() ) {
            hero.GetPath().Reset();

            for ( MapsIndexes::const_iterator it = pickups.begin(); it != pickups.end(); ++it )
                if ( AI::HeroesValidObject( hero, *it ) ) {
                    const uint32_t dist = hero.GetPath().Calculate( *it );

                    if ( dist != 0 ) {
                        task.push_front( IndexDistance( *it, dist ) );
                        DEBUG( DBG_AI, DBG_TRACE, hero.GetName() << ", object found: " << MP2::StringObject( world.GetTiles( *it ).GetObject() ) << "(" << *it << ")" );
                    }
                }
        }

        if ( !pickups.empty() && hero.GetPath().isValid() ) {
            DEBUG( DBG_AI, DBG_TRACE, hero.GetName() << ", continue" );
            ai_hero.fix_loop++;
            return true;
        }

        if ( task.empty() ) {
            // get task from kingdom
            DEBUG( DBG_AI, DBG_TRACE, hero.GetName() << ", empty task" );
            HeroesAddedTask( hero );
        }

        task.sort( IndexDistance::Shortest );

        // find passable index
        while ( task.size() ) {
            const int index = task.front().first;

            if ( HeroesValidObject( hero, index ) ) {
                DEBUG( DBG_AI, DBG_TRACE, hero.GetName() << ", looking for: " << MP2::StringObject( world.GetTiles( index ).GetObject() ) << "(" << index << ")" );
                if ( hero.GetPath().Calculate( index, PATHFINDING_LIMIT ) )
                    break;

                DEBUG( DBG_AI, DBG_TRACE, hero.GetName() << " say: unable to get object: " << index << ", remove task..." );
            }

            task.pop_front();
        }

        // success
        if ( task.size() ) {
            const s32 & index = task.front().first;
            DEBUG( DBG_AI, DBG_TRACE, hero.GetName() << " go to: " << index );

            if ( !hero.GetPath().isValid() )
                hero.GetPath().Calculate( index );

            ai_objects.erase( index );
            task.pop_front();

            DEBUG( DBG_AI, DBG_TRACE, hero.GetName() << ", route: " << hero.GetPath().String() );
            return true;
        }
        else if ( hero.Modes( AI::HERO_WAITING ) ) {
            hero.GetPath().Reset();
            DEBUG( DBG_AI, DBG_TRACE, hero.GetName() << " say: unknown task, help me please.." );

            hero.ResetModes( AI::HERO_WAITING );
            hero.SetModes( AI::HERO_SKIP_TURN );
        }
        else {
            DEBUG( DBG_AI, DBG_TRACE, hero.GetName() << " say: waiting..." );
            hero.SetModes( AI::HERO_WAITING );
        }

        return false;
    }

    void Simple::HeroTurn( Heroes & hero )
    {
        DEBUG( DBG_AI, DBG_TRACE,
               hero.GetName() << ", start: " << ( hero.Modes( Heroes::SHIPMASTER ) ? "SHIPMASTER," : "" ) << ( hero.Modes( AI::HERO_SCOUT ) ? "SCOUTER," : "" )
                              << ( hero.Modes( AI::HERO_HUNTER ) ? "HUNTER," : "" ) << ( hero.Modes( Heroes::PATROL ) ? "PATROL," : "" )
                              << ( hero.Modes( AI::HERO_WAITING ) ? "WAITING," : "" ) << ( hero.Modes( AI::HERO_SKIP_TURN ) ? "STUPID" : "" ) );

        Interface::StatusWindow & status = Interface::Basic::Get().GetStatusWindow();

        while ( hero.MayStillMove() && !hero.Modes( AI::HERO_WAITING | AI::HERO_SKIP_TURN ) ) {
            // turn indicator
            status.RedrawTurnProgress( 3 );

            // get task for heroes
            HeroesGetTask( hero );

            // turn indicator
            status.RedrawTurnProgress( 5 );

            // heroes AI turn
            HeroesMove( hero );

            // turn indicator
            status.RedrawTurnProgress( 7 );
        }

        DEBUG( DBG_AI, DBG_TRACE, hero.GetName() << ", end" );
    }

    bool Simple::HeroesScheduledVisit( const Kingdom & kingdom, s32 index )
    {
        for ( KingdomHeroes::const_iterator it = kingdom.GetHeroes().begin(); it != kingdom.GetHeroes().end(); ++it ) {
            AIHero & ai_hero = GetHero( **it );
            Queue & task = ai_hero.sheduled_visit;
            if ( task.isPresent( index ) )
                return true;
        }
        return false;
    }

    bool Simple::IsPriorityAndNotVisitAndNotPresent( const std::pair<s32, int> & indexObj, const Heroes * hero )
    {
        AIHero & ai_hero = GetHero( *hero );

        return !HeroesScheduledVisit( hero->GetKingdom(), indexObj.first ) && AIHeroesPriorityObject( *hero, indexObj.first );
    }

    void Simple::HeroesTurnEnd( Heroes * hero )
    {
        if ( hero ) {
            AIHero & ai_hero = GetHero( *hero );
            AIKingdom & ai_kingdom = GetKingdom( hero->GetColor() );
            Queue & task = ai_hero.sheduled_visit;
            IndexObjectMap & ai_objects = ai_kingdom.scans;

            if ( hero->Modes( AI::HERO_WAITING | AI::HERO_SKIP_TURN ) ) {
                ai_hero.Reset();
                hero->ResetModes( AI::HERO_WAITING | AI::HERO_SKIP_TURN );
            }

            std::vector<int> validObjects;
            for ( IndexObjectMap::iterator it = ai_objects.begin(); it != ai_objects.end(); ++it ) {
                if ( IsPriorityAndNotVisitAndNotPresent( *it, hero ) ) {
                    validObjects.push_back( it->first );
                    DEBUG( DBG_AI, DBG_TRACE, hero->GetName() << ", adding priority object: " << MP2::StringObject( ( *it ).second ) << ", index: " << ( *it ).first );
                }
            }

            for ( std::vector<int>::iterator it = validObjects.begin(); it != validObjects.end(); ++it ) {
                task.push_front( IndexDistance( *it, Maps::GetApproximateDistance( hero->GetIndex(), *it ) * 100 ) );
                ai_objects.erase( *it );
            }
        }
    }

    void Simple::HeroesSetHunterWithTarget( Heroes * hero, s32 dst )
    {
        if ( hero ) {
            AIHero & ai_hero = GetHero( *hero );

            hero->SetModes( AI::HERO_HUNTER );

            if ( 0 > ai_hero.primary_target ) {
                ai_hero.primary_target = dst;
            }
        }
    }

    void Simple::HeroesCaptureNearestTown( Heroes * hero )
    {
        if ( hero ) {
            AIHero & ai_hero = GetHero( *hero );

            if ( 0 > ai_hero.primary_target ) {
                const Maps::Indexes & castles = Maps::GetObjectPositions( hero->GetIndex(), MP2::OBJ_CASTLE, true );

                for ( MapsIndexes::const_iterator it = castles.begin(); it != castles.end(); ++it ) {
                    const Castle * castle = world.GetCastle( Maps::GetPoint( *it ) );

                    if ( castle )
                        DEBUG( DBG_AI, DBG_TRACE, hero->GetName() << ", to castle: " << castle->GetName() );

                    if ( castle && hero->GetArmy().isStrongerThan( castle->GetArmy() ) ) {
                        ai_hero.primary_target = *it;

                        DEBUG( DBG_AI, DBG_INFO, Color::String( hero->GetColor() ) << ", Hero " << hero->GetName() << " set primary target: " << *it );
                        break;
                    }
                }
            }
        }
    }
}
