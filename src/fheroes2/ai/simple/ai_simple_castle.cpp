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

#include "ai_simple.h"
#include "castle.h"
#include "game.h"
#include "heroes.h"
#include "kingdom.h"
#include "race.h"
#include "world.h"

namespace AI
{
    bool BuildIfAvailable( Castle & castle, int building )
    {
        if ( !castle.isBuild( building ) )
            return castle.BuyBuilding( building );
        return false;
    }

    bool BuildIfEnoughResources( Castle & castle, int building, u32 minimumMultiplicator )
    {
        if ( minimumMultiplicator < 1 || minimumMultiplicator > 99 ) // can't be that we need more than 100 times resources
            return false;

        const Kingdom & kingdom = castle.GetKingdom();
        if ( kingdom.GetFunds() >= PaymentConditions::BuyBuilding( castle.GetRace(), building ) * minimumMultiplicator )
            return BuildIfAvailable( castle, building );
        return false;
    }

    u32 GetResourceMultiplier( Castle & castle, u32 min, u32 max )
    {
        return castle.isCapital() ? 1 : Rand::Get( min, max );
    }

    void AICastleDefense( Castle & castle )
    {
        castle.RecruitAllMonster(); // buy monsters at first place
        const bool doesArmyExist = castle.GetActualArmy().GetCount() > 0;

        if ( castle.isCastle() && doesArmyExist ) {
            BuildIfAvailable( castle, BUILD_LEFTTURRET );
            BuildIfAvailable( castle, BUILD_RIGHTTURRET );
            BuildIfAvailable( castle, BUILD_MOAT );
            if ( NULL == castle.GetHeroes().Guest() )
                BuildIfAvailable( castle, BUILD_CAPTAIN );

            if ( castle.GetRace() == Race::KNGT ) {
                BuildIfAvailable( castle, BUILD_SPEC ); // fortification
                BuildIfAvailable( castle, BUILD_TAVERN );
            }
            else if ( ( ( Race::SORC | Race::BARB | Race::NECR ) & castle.GetRace() ) == 0 ) {
                BuildIfAvailable( castle, BUILD_SPEC ); // Rainbow, Colliseum or Storm
            }
        }
    }

    void AICastleDevelopment( Castle & castle )
    {
        const Kingdom & kingdom = castle.GetKingdom();

        if ( castle.isCastle() ) {
            if ( world.LastDay() ) // 7th day of week
                BuildIfAvailable( castle, BUILD_WELL );
            BuildIfAvailable( castle, BUILD_STATUE );
            if ( Race::WRLK == castle.GetRace() )
                BuildIfAvailable( castle, BUILD_SPEC ); // Dungeon
            BuildIfAvailable( castle, DWELLING_UPGRADE7 );
            BuildIfAvailable( castle, DWELLING_UPGRADE6 );
            BuildIfAvailable( castle, DWELLING_MONSTER6 );
            BuildIfAvailable( castle, DWELLING_UPGRADE5 );
            BuildIfAvailable( castle, DWELLING_MONSTER5 );
            BuildIfAvailable( castle, DWELLING_UPGRADE4 );
            BuildIfAvailable( castle, DWELLING_MONSTER4 );
            BuildIfEnoughResources( castle, DWELLING_UPGRADE3, GetResourceMultiplier( castle, 2, 3 ) );
            BuildIfEnoughResources( castle, DWELLING_MONSTER3, GetResourceMultiplier( castle, 2, 3 ) );
            BuildIfEnoughResources( castle, DWELLING_UPGRADE2, GetResourceMultiplier( castle, 3, 4 ) );
            BuildIfEnoughResources( castle, DWELLING_MONSTER2, GetResourceMultiplier( castle, 3, 4 ) );
            BuildIfEnoughResources( castle, DWELLING_MONSTER1, GetResourceMultiplier( castle, 4, 5 ) );
            if ( Race::KNGT == castle.GetRace() ) {
                BuildIfEnoughResources( castle, BUILD_TAVERN, GetResourceMultiplier( castle, 2, 3 ) ); // needed for Armory
                BuildIfEnoughResources( castle, BUILD_WELL, GetResourceMultiplier( castle, 3, 4 ) ); // needed for Blacksmith
            }
            else if ( Race::SORC == castle.GetRace() ) {
                BuildIfEnoughResources( castle, BUILD_MAGEGUILD1, GetResourceMultiplier( castle, 2, 3 ) ); // needed for Stonehenge
                BuildIfEnoughResources( castle, BUILD_WELL, GetResourceMultiplier( castle, 3, 4 ) ); // needed Upg. Cottage
                BuildIfEnoughResources( castle, BUILD_TAVERN, GetResourceMultiplier( castle, 3, 4 ) ); // needed for Cottage
            }
            else if ( Race::NECR == castle.GetRace() ) {
                BuildIfEnoughResources( castle, BUILD_MAGEGUILD2, GetResourceMultiplier( castle, 2, 3 ) ); // needed for Upg. Mausoleum
                BuildIfEnoughResources( castle, BUILD_MAGEGUILD1, GetResourceMultiplier( castle, 2, 3 ) ); // needed for Mausoleum
                BuildIfEnoughResources( castle, BUILD_THIEVESGUILD, GetResourceMultiplier( castle, 3, 4 ) ); // needed for Mansion
            }
            else if ( Race::WZRD == castle.GetRace() ) {
                BuildIfEnoughResources( castle, BUILD_SPEC, GetResourceMultiplier( castle, 2, 3 ) ); // Library needed for Upg. Ivory Tower
                BuildIfEnoughResources( castle, BUILD_MAGEGUILD1, GetResourceMultiplier( castle, 2, 3 ) ); // needed for Ivory Tower
                BuildIfEnoughResources( castle, BUILD_WELL, GetResourceMultiplier( castle, 3, 4 ) ); // needed for Upg. Foundry
            }
            if ( world.LastDay() )
                BuildIfEnoughResources( castle, BUILD_WEL2, 5 );
            BuildIfAvailable( castle, BUILD_MAGEGUILD1 );
            BuildIfEnoughResources( castle, BUILD_LEFTTURRET, 5 );
            BuildIfEnoughResources( castle, BUILD_RIGHTTURRET, 5 );
            BuildIfEnoughResources( castle, BUILD_MOAT, 10 );
            BuildIfEnoughResources( castle, BUILD_WELL, 5 );
            BuildIfEnoughResources( castle, BUILD_MAGEGUILD2, 5 );
            BuildIfEnoughResources( castle, BUILD_MAGEGUILD3, 5 );
            BuildIfEnoughResources( castle, BUILD_MAGEGUILD4, 5 );
            BuildIfEnoughResources( castle, BUILD_MAGEGUILD5, 5 );
            BuildIfEnoughResources( castle, BUILD_SPEC, 10 );
        }
        else {
            // Build castle only monday or tuesday or for capital or when we have in 5-10 times more resources than needed (fair point)
            if ( castle.isCapital() || 3 > world.GetDay()
                 || kingdom.GetFunds() >= ( PaymentConditions::BuyBuilding( castle.GetRace(), BUILD_CASTLE ) * Rand::Get( 5, 10 ) ) )
                castle.BuyBuilding( BUILD_CASTLE );
        }

        if ( world.LastDay() ) // last day so buy monster
            castle.RecruitAllMonster();
    }

    void Simple::CastleTurn( Castle & castle )
    {
        // skip neutral town
        if ( castle.GetColor() == Color::NONE )
            return;

        s32 range = Game::GetViewDistance( castle.isCastle() ? Game::VIEW_CASTLE : Game::VIEW_TOWN );
        const Heroes * enemy = NULL;

        // find enemy hero
        const Point & castleCenter = castle.GetCenter();
        for ( s32 y = -range; y <= range; ++y ) {
            for ( s32 x = -range; x <= range; ++x ) {
                if ( !y && !x )
                    continue;

                if ( Maps::isValidAbsPoint( castleCenter.x + x, castleCenter.y + y ) ) {
                    const Maps::Tiles & tile = world.GetTiles( Maps::GetIndexFromAbsPoint( castleCenter.x + x, castleCenter.y + y ) );

                    if ( MP2::OBJ_HEROES == tile.GetObject() )
                        enemy = tile.GetHeroes();

                    if ( enemy && castle.GetColor() == enemy->GetColor() )
                        enemy = NULL;

                    if ( enemy )
                        break;
                }
            }
        }

        enemy ? AI::AICastleDefense( castle ) : AI::AICastleDevelopment( castle );

        Kingdom & kingdom = castle.GetKingdom();
        Heroes * hero = castle.GetHeroes().Guest();
        const bool canRecruit = castle.isCastle() && !hero && kingdom.GetHeroes().size() < Kingdom::GetMaxHeroes();

        // part II
        if ( enemy && castle.GetArmy().isValid() && castle.GetArmy().isStrongerThan( enemy->GetArmy() ) ) {
            if ( canRecruit ) {
                Recruits & rec = kingdom.GetRecruits();

                if ( rec.GetHero1() )
                    hero = castle.RecruitHero( rec.GetHero1() );
                else if ( rec.GetHero2() )
                    hero = castle.RecruitHero( rec.GetHero2() );
            }

            if ( hero )
                hero->SetModes( AI::HEROES_HUNTER );
        }

        // part III
        AIKingdom & ai = GetKingdom( castle.GetColor() );
        if ( ai.capital != &castle && castle.GetArmy().isValid() && !hero && 2 < castle.GetArmy().GetCount() && 150 < castle.GetArmy().GetHitPoints() && canRecruit ) {
            Recruits & rec = kingdom.GetRecruits();

            if ( rec.GetHero1() )
                hero = castle.RecruitHero( rec.GetHero1() );
            else if ( rec.GetHero2() )
                hero = castle.RecruitHero( rec.GetHero2() );

            if ( hero )
                hero->SetModes( AI::HEROES_HUNTER | AI::HEROES_SCOUTER );
        }
    }

    void Simple::CastlePreBattle( Castle & castle )
    {
        Heroes * hero = castle.GetHeroes().GuardFirst();
        if ( hero && castle.GetArmy().isValid() )
            hero->GetArmy().JoinStrongestFromArmy( castle.GetArmy() );
    }

    void Simple::CastleRemove( const Castle & castle )
    {
        AIKingdom & ai = GetKingdom( castle.GetColor() );

        if ( ai.capital == &castle ) {
            ai.capital->ResetModes( Castle::CAPITAL );
            ai.capital = NULL;
        }
    }
}
