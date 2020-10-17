/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2020                                                    *
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

#include "agg.h"
#include "ai_normal.h"
#include "game_interface.h"
#include "kingdom.h"
#include "mus.h"
#include "world.h"

namespace AI
{
    bool IsValidKingdomObject( const Kingdom & kingdom, const Maps::Tiles & tile, int objectID )
    {
        const int kingdomColor = kingdom.GetColor();
        if ( tile.isFog( kingdomColor ) || ( !MP2::isGroundObject( objectID ) && objectID != MP2::OBJ_COAST ) )
            return false;

        if ( kingdom.isVisited( tile.GetIndex(), objectID ) )
            return false;

        // Check castle first to ignore guest hero (tile with both Castle and Hero)
        if ( tile.GetObject( false ) == MP2::OBJ_CASTLE ) {
            const int tileColor = tile.QuantityColor();
            if ( !Settings::Get().ExtUnionsAllowCastleVisiting() && Players::isFriends( kingdomColor, tileColor ) ) {
                // false only if alliance castles can't be visited
                return kingdomColor == tileColor;
            }
            return true;
        }

        // Hero object can overlay other objects when standing on top of it: force check with GetObject( true )
        if ( objectID == MP2::OBJ_HEROES ) {
            const Heroes * hero = tile.GetHeroes();
            return hero && !Players::isFriends( kingdomColor, hero->GetColor() );
        }

        if ( MP2::isCaptureObject( objectID ) )
            return !Players::isFriends( kingdomColor, tile.QuantityColor() );

        if ( MP2::isQuantityObject( objectID ) )
            return tile.QuantityIsValid();

        return true;
    }

    void Normal::KingdomTurn( Kingdom & kingdom )
    {
        const int color = kingdom.GetColor();

        if ( kingdom.isLoss() || color == Color::NONE ) {
            kingdom.LossPostActions();
            return;
        }

        // reset indicator
        Interface::StatusWindow & status = Interface::Basic::Get().GetStatusWindow();
        status.RedrawTurnProgress( 0 );

        AGG::PlayMusic( MUS::COMPUTER_TURN );
        KingdomHeroes & heroes = kingdom.GetHeroes();
        KingdomCastles & castles = kingdom.GetCastles();

        DEBUG( DBG_AI, DBG_INFO, Color::String( color ) << " starts the turn: " << castles.size() << " castles, " << heroes.size() << " heroes" );
        DEBUG( DBG_AI, DBG_TRACE, "Funds: " << kingdom.GetFunds().String() );

        // Step 1. Scan visible map (based on game difficulty), add goals and threats
        std::vector<std::pair<int, const Army *> > enemyArmies;

        const int mapSize = world.w() * world.h();
        _mapObjects.clear();

        for ( int idx = 0; idx < mapSize; ++idx ) {
            const Maps::Tiles & tile = world.GetTiles( idx );
            int objectID = tile.GetObject();

            if ( !IsValidKingdomObject( kingdom, tile, objectID ) )
                continue;

            _mapObjects.emplace_back( idx, objectID );

            const int color = tile.QuantityColor();
            if ( objectID == MP2::OBJ_HEROES ) {
                const Heroes * enemy = tile.GetHeroes();
                if ( enemy && !Players::isFriends( color, enemy->GetColor() ) ) {
                    enemyArmies.emplace_back( idx, &enemy->GetArmy() );
                }
            }
            else if ( objectID == MP2::OBJ_CASTLE && color != Color::NONE && !Players::isFriends( color, color ) ) {
                const Castle * castle = world.GetCastle( Maps::GetPoint( idx ) );
                if ( castle )
                    enemyArmies.emplace_back( idx, &castle->GetArmy() );
            }
        }

        DEBUG( DBG_AI, DBG_TRACE, Color::String( color ) << " found " << _mapObjects.size() << " valid objects" );

        status.RedrawTurnProgress( 1 );

        // Step 2. Update AI variables and recalculate resource budget
        const bool slowEarlyGame = world.CountDay() < 5 && castles.size() == 1;

        double combinedHeroStrength = 0;
        for ( auto it = heroes.begin(); it != heroes.end(); ++it ) {
            if ( *it ) {
                combinedHeroStrength += ( *it )->GetArmy().GetStrength();
            }
        }

        const uint32_t threatDistanceLimit = 2500; // 25 tiles, roughly how much maxed out hero can move in a turn
        std::vector<int> castlesInDanger;

        if ( !enemyArmies.empty() ) { // only if at least one enemy is nearby
            for ( size_t idx = 0; idx < castles.size(); ++idx ) {
                const Castle * castle = castles[idx];
                if ( castle ) {
                    const int castleIndex = castle->GetIndex();
                    const double defenders = castle->GetArmy().GetStrength();

                    for ( auto enemy = enemyArmies.begin(); enemy != enemyArmies.end(); ++enemy ) {
                        if ( enemy->second ) {
                            const double attackerStrength = enemy->second->GetStrength();
                            const double attackerThreat = attackerStrength - defenders;
                            if ( attackerThreat > 0 ) {
                                const uint32_t dist = _pathfinder.getDistance( castleIndex, enemy->first, attackerStrength, color );
                                if ( dist && dist < threatDistanceLimit ) {
                                    // castle is under threat
                                    castlesInDanger.push_back( castleIndex );
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }

        size_t heroLimit = Maps::XLARGE > world.w() ? ( Maps::LARGE > world.w() ? 2 : 3 ) : 4;
        if ( _personality == EXPLORER )
            heroLimit++;
        if ( slowEarlyGame )
            heroLimit = 2;

        // Step 3. Buy new heroes, adjust roles, sort heroes based on priority or strength

        // GetFirstCastle might return NULL if there's only towns with a tent
        Castle * castle = castles.GetFirstCastle();

        if ( castle && heroes.size() < heroLimit ) {
            Recruits & rec = kingdom.GetRecruits();

            Heroes * hero = castle->GetHeroes().Guest();
            if ( !hero ) {
                hero = castle->RecruitHero( rec.GetHero1() );

                if ( !slowEarlyGame && hero )
                    ReinforceHeroInCastle( *hero, *castle, kingdom.GetFunds() );
            }
        }

        // Copy hero list and sort (original list may be altered during the turn)
        VecHeroes sortedHeroList = heroes;
        std::sort( sortedHeroList.begin(), sortedHeroList.end(), []( const Heroes * left, const Heroes * right ) {
            if ( left && right )
                return left->GetArmy().GetStrength() < right->GetArmy().GetStrength();
            return right == NULL;
        } );

        status.RedrawTurnProgress( 2 );

        // Step 4. Move heroes until they have nothing to do (HERO_WAITING or HERO_MOVED state)
        size_t heroesMovedCount = 0;
        for ( auto it = sortedHeroList.begin(); it != sortedHeroList.end(); ++it ) {
            if ( *it ) {
                HeroTurn( **it );

                if ( ( *it )->Modes( HERO_MOVED ) ) {
                    ++heroesMovedCount;
                    status.RedrawTurnProgress( 2 + ( 7 * heroesMovedCount / sortedHeroList.size() ) );
                }
            }
        }

        // Step 5. Repeat process (maybe there was a path unlocked by a stronger hero)
        for ( auto it = sortedHeroList.begin(); it != sortedHeroList.end(); ++it ) {
            if ( *it && !( *it )->Modes( HERO_MOVED ) ) {
                HeroTurn( **it );
                ++heroesMovedCount;
                status.RedrawTurnProgress( 2 + ( 7 * heroesMovedCount / sortedHeroList.size() ) );
            }
        }

        status.RedrawTurnProgress( 9 );

        // Step 6. Castle development according to kingdom budget
        for ( KingdomCastles::iterator it = castles.begin(); it != castles.end(); ++it ) {
            if ( *it ) {
                CastleTurn( **it, std::find( castlesInDanger.begin(), castlesInDanger.end(), ( *it )->GetIndex() ) != castlesInDanger.end() );
            }
        }
    }
}
