/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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
#include <cstdint>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "ai.h"
#include "army.h"
#include "army_troop.h"
#include "artifact.h"
#include "audio_manager.h"
#include "battle.h"
#include "castle.h"
#include "color.h"
#include "dialog.h"
#include "direction.h"
#include "game.h"
#include "game_delays.h"
#include "game_hotkeys.h"
#include "game_interface.h"
#include "game_static.h"
#include "heroes.h"
#include "interface_base.h"
#include "interface_gamearea.h"
#include "kingdom.h"
#include "localevent.h"
#include "logging.h"
#include "m82.h"
#include "maps.h"
#include "maps_objects.h"
#include "maps_tiles.h"
#include "maps_tiles_helper.h"
#include "math_base.h"
#include "monster.h"
#include "mp2.h"
#include "payment.h"
#include "players.h"
#include "puzzle.h"
#include "race.h"
#include "rand.h"
#include "resource.h"
#include "route.h"
#include "screen.h"
#include "settings.h"
#include "skill.h"
#include "spell.h"
#include "spell_info.h"
#include "translations.h"
#include "ui_dialog.h"
#include "visit.h"
#include "world.h"

namespace
{
    uint32_t AIGetAllianceColors()
    {
        // accumulate colors
        uint32_t colors = 0;

        if ( Settings::Get().GameType() & Game::TYPE_HOTSEAT ) {
            const Colors vcolors( Players::HumanColors() );

            for ( const int color : vcolors ) {
                const Player * player = Players::Get( color );
                if ( player )
                    colors |= player->GetFriends();
            }
        }
        else {
            const Player * player = Players::Get( Players::HumanColors() );
            if ( player )
                colors = player->GetFriends();
        }

        return colors;
    }

    // Never cache the value of this function as it depends on hero's path and location.
    bool AIHeroesShowAnimation( const Heroes & hero, const uint32_t colors )
    {
        if ( Settings::Get().AIMoveSpeed() == 0 ) {
            return false;
        }

        if ( colors == 0 ) {
            return false;
        }

        const int32_t indexFrom = hero.GetIndex();
        if ( !Maps::isValidAbsIndex( indexFrom ) ) {
            return false;
        }

        // Show AI hero animation if he is visible (or barely visible) for human player.
        if ( world.GetTiles( indexFrom ).getFogDirection() != DIRECTION_ALL ) {
            return true;
        }

        const Route::Path & path = hero.GetPath();
        // Show AI hero animation if any of the tiles next to the first tile in the path is visible for human player.
        if ( path.isValid() && ( world.GetTiles( path.front().GetIndex() ).getFogDirection() != DIRECTION_ALL ) ) {
            return true;
        }

        return false;
    }

    int AISelectSkillFromArena( const Heroes & hero )
    {
        switch ( hero.GetRace() ) {
        case Race::KNGT:
            if ( hero.GetDefense() < 5 ) {
                return Skill::Primary::DEFENSE;
            }
            if ( hero.GetAttack() < 5 ) {
                return Skill::Primary::ATTACK;
            }
            if ( hero.GetPower() < 3 ) {
                return Skill::Primary::POWER;
            }
            break;

        case Race::BARB:
            if ( hero.GetAttack() < 5 ) {
                return Skill::Primary::ATTACK;
            }
            if ( hero.GetDefense() < 5 ) {
                return Skill::Primary::DEFENSE;
            }
            if ( hero.GetPower() < 3 ) {
                return Skill::Primary::POWER;
            }
            break;

        case Race::SORC:
        case Race::WZRD:
            if ( hero.GetPower() < 5 ) {
                return Skill::Primary::POWER;
            }
            if ( hero.GetDefense() < 3 ) {
                return Skill::Primary::DEFENSE;
            }
            if ( hero.GetAttack() < 3 ) {
                return Skill::Primary::ATTACK;
            }
            break;

        case Race::WRLK:
        case Race::NECR:
            if ( hero.GetPower() < 5 ) {
                return Skill::Primary::POWER;
            }
            if ( hero.GetAttack() < 3 ) {
                return Skill::Primary::ATTACK;
            }
            if ( hero.GetDefense() < 3 ) {
                return Skill::Primary::DEFENSE;
            }
            break;

        default:
            break;
        }

        switch ( Rand::Get( 1, 3 ) ) {
        case 1:
            return Skill::Primary::ATTACK;
        case 2:
            return Skill::Primary::DEFENSE;
        case 3:
            return Skill::Primary::POWER;
        default:
            assert( 0 );
            break;
        }

        return Skill::Primary::UNKNOWN;
    }

    bool canMonsterJoinHero( const Troop & troop, Heroes & hero )
    {
        if ( hero.GetArmy().HasMonster( troop.GetID() ) ) {
            return true;
        }

        if ( troop.GetStrength() < hero.getAIMinimumJoiningArmyStrength() ) {
            // No use to hire such a weak troop.
            return false;
        }

        if ( !hero.GetArmy().mergeWeakestTroopsIfNeeded() ) {
            // The army has no slots and we cannot rearrange it.
            return false;
        }

        return true;
    }

    void AIBattleLose( Heroes & hero, const Battle::Result & res, bool attacker, const fheroes2::Point * centerOn = nullptr, const bool playSound = false )
    {
        const uint32_t reason = attacker ? res.AttackerResult() : res.DefenderResult();

        if ( AIHeroesShowAnimation( hero, AIGetAllianceColors() ) ) {
            if ( centerOn != nullptr ) {
                Interface::AdventureMap::Get().getGameArea().SetCenter( *centerOn );
            }

            if ( playSound ) {
                AudioManager::PlaySound( M82::KILLFADE );
            }
            hero.FadeOut();
        }

        hero.SetFreeman( reason );
    }

    void AIMeeting( Heroes & left, Heroes & right )
    {
        left.markHeroMeeting( right.GetID() );
        right.markHeroMeeting( left.GetID() );

        // A hero with a higher role must receive an army and artifacts from another hero.
        // In case of the same roles a more powerful hero will receive all benefits.
        bool rightToLeft = true;
        if ( left.getAIRole() < right.getAIRole() ) {
            rightToLeft = false;
        }
        else if ( left.getAIRole() == right.getAIRole() ) {
            rightToLeft = right.getStatsValue() < left.getStatsValue();
        }

        Heroes & giver = rightToLeft ? right : left;
        Heroes & taker = rightToLeft ? left : right;

        // TODO: do not transfer the whole army from one hero to another. Add logic to leave a fast unit for Scout and Courier. Also 3-5 monsters are better than
        // having 1 Peasant in one stack which leads to an instant death if the hero is attacked by an opponent.
        taker.GetArmy().JoinStrongestFromArmy( giver.GetArmy() );

        taker.GetBagArtifacts().exchangeArtifacts( giver.GetBagArtifacts(), taker, giver );
    }

    void AIToCastle( Heroes & hero, const int32_t dstIndex )
    {
        Castle * castle = world.getCastleEntrance( Maps::GetPoint( dstIndex ) );
        if ( castle == nullptr ) {
            // This should never happen
            assert( 0 );

            DEBUG_LOG( DBG_AI, DBG_WARN,
                       hero.GetName() << " is trying to visit the castle on tile " << dstIndex << ", but there is no entrance to the castle on this tile" )
            return;
        }

        if ( hero.GetColor() == castle->GetColor() ) {
            assert( hero.GetIndex() == dstIndex );

            DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() << " visits " << castle->GetName() )

            castle->MageGuildEducateHero( hero );
            hero.SetVisited( dstIndex );

            return;
        }

        if ( hero.isFriends( castle->GetColor() ) ) {
            // This should never happen - hero should not be able to visit an allied castle
            assert( 0 );

            DEBUG_LOG( DBG_AI, DBG_WARN, hero.GetName() << " is not allowed to visit the allied castle " << castle->GetName() )
            return;
        }

        auto captureCastle = [&hero, dstIndex, castle]() {
            castle->GetKingdom().RemoveCastle( castle );
            hero.GetKingdom().AddCastle( castle );
            world.CaptureObject( dstIndex, hero.GetColor() );

            castle->Scout();
        };

        Army & army = castle->GetActualArmy();

        // Hero is standing in front of the castle, which means that there must be an enemy army in the castle
        if ( hero.GetIndex() != dstIndex ) {
            assert( army.isValid() && army.GetColor() != hero.GetColor() );

            DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() << " attacks the enemy castle " << castle->GetName() )

            Heroes * defender = castle->GetHero();
            const bool playVanishingHeroSound = defender != nullptr && defender->isControlHuman();

            castle->ActionPreBattle();

            Battle::Result res = Battle::Loader( hero.GetArmy(), army, dstIndex );

            castle->ActionAfterBattle( res.AttackerWins() );

            // The defender was defeated
            if ( !res.DefenderWins() && defender ) {
                AIBattleLose( *defender, res, false, nullptr, playVanishingHeroSound );
            }

            // The attacker was defeated
            if ( !res.AttackerWins() ) {
                AIBattleLose( hero, res, true, &( castle->GetCenter() ), playVanishingHeroSound );
            }

            // The attacker won
            if ( res.AttackerWins() ) {
                captureCastle();

                hero.IncreaseExperience( res.GetExperienceAttacker() );
            }
            // The defender won
            else if ( res.DefenderWins() && defender ) {
                defender->IncreaseExperience( res.GetExperienceDefender() );
            }

            return;
        }

        // If hero is already in the castle, then there must be his own army in the castle
        assert( army.isValid() && army.GetColor() == hero.GetColor() );

        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() << " captures the enemy castle " << castle->GetName() )

        captureCastle();

        castle->MageGuildEducateHero( hero );
        hero.SetVisited( dstIndex );
    }

    void AIToHeroes( Heroes & hero, const int32_t dstIndex )
    {
        Heroes * otherHero = world.GetTiles( dstIndex ).GetHeroes();
        if ( otherHero == nullptr ) {
            // This should never happen
            assert( 0 );

            DEBUG_LOG( DBG_AI, DBG_WARN, hero.GetName() << " is trying to meet the hero on tile " << dstIndex << ", but there is no hero on this tile" )
            return;
        }

        if ( hero.GetColor() == otherHero->GetColor() ) {
            DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() << " meets " << otherHero->GetName() )

            AIMeeting( hero, *otherHero );

            return;
        }

        if ( hero.isFriends( otherHero->GetColor() ) ) {
            // This should never happen - hero should not be able to meet an allied hero
            assert( 0 );

            DEBUG_LOG( DBG_AI, DBG_WARN, hero.GetName() << " is not allowed to meet the allied hero " << otherHero->GetName() )
            return;
        }

        if ( otherHero->inCastle() ) {
            AIToCastle( hero, dstIndex );

            return;
        }

        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() << " attacks " << otherHero->GetName() )

        const bool playVanishingHeroSound = otherHero->isControlHuman();

        Battle::Result res = Battle::Loader( hero.GetArmy(), otherHero->GetArmy(), dstIndex );

        // The defender was defeated
        if ( !res.DefenderWins() ) {
            AIBattleLose( *otherHero, res, false, nullptr, playVanishingHeroSound );
        }

        // The attacker was defeated
        if ( !res.AttackerWins() ) {
            AIBattleLose( hero, res, true, &( otherHero->GetCenter() ), playVanishingHeroSound );
        }

        // The attacker won
        if ( res.AttackerWins() ) {
            hero.IncreaseExperience( res.GetExperienceAttacker() );
        }
        // The defender won
        else if ( res.DefenderWins() ) {
            otherHero->IncreaseExperience( res.GetExperienceDefender() );
        }
    }

    void AIToMonster( Heroes & hero, int32_t dst_index )
    {
        bool destroy = false;
        Maps::Tiles & tile = world.GetTiles( dst_index );
        Troop troop = getTroopFromTile( tile );

        const NeutralMonsterJoiningCondition join = Army::GetJoinSolution( hero, tile, troop );

        if ( join.reason == NeutralMonsterJoiningCondition::Reason::Alliance ) {
            if ( hero.GetArmy().CanJoinTroop( troop ) ) {
                DEBUG_LOG( DBG_AI, DBG_INFO, troop.GetName() << " join " << hero.GetName() << " as a part of alliance." )
                hero.GetArmy().JoinTroop( troop );
            }
            else {
                DEBUG_LOG( DBG_AI, DBG_INFO, troop.GetName() << " unblock way for " << hero.GetName() << " as a part of alliance." )
            }

            destroy = true;
        }
        else if ( join.reason == NeutralMonsterJoiningCondition::Reason::Bane ) {
            DEBUG_LOG( DBG_AI, DBG_INFO, troop.GetName() << " run away from " << hero.GetName() << " as a part of bane." )
            destroy = true;
        }
        else if ( join.reason == NeutralMonsterJoiningCondition::Reason::Free ) {
            // This condition must already be met if a group of monsters wants to join
            assert( hero.GetArmy().CanJoinTroop( troop ) );

            DEBUG_LOG( DBG_AI, DBG_INFO, troop.GetName() << " join " << hero.GetName() << "." )
            hero.GetArmy().JoinTroop( troop );
            destroy = true;
        }
        else if ( join.reason == NeutralMonsterJoiningCondition::Reason::ForMoney ) {
            const int32_t joiningCost = troop.GetTotalCost().gold;

            // These conditions must already be met if a group of monsters wants to join
            assert( hero.GetArmy().CanJoinTroop( troop ) && hero.GetKingdom().AllowPayment( payment_t( Resource::GOLD, joiningCost ) ) );

            DEBUG_LOG( DBG_AI, DBG_INFO, join.monsterCount << " " << troop.GetName() << " join " << hero.GetName() << " for " << joiningCost << " gold." )
            hero.GetArmy().JoinTroop( troop.GetMonster(), join.monsterCount, false );
            hero.GetKingdom().OddFundsResource( Funds( Resource::GOLD, joiningCost ) );
            destroy = true;
        }
        else if ( join.reason == NeutralMonsterJoiningCondition::Reason::RunAway ) {
            // TODO: AI should still chase monsters which it can defeat without losses to get extra experience.
            DEBUG_LOG( DBG_AI, DBG_INFO, troop.GetName() << " run away from " << hero.GetName() << "." )
            destroy = true;
        }

        // Fight
        if ( !destroy ) {
            DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() << " attacks monster " << troop.GetName() )

            Army army( tile );

            Battle::Result res = Battle::Loader( hero.GetArmy(), army, dst_index );

            if ( res.AttackerWins() ) {
                hero.IncreaseExperience( res.GetExperienceAttacker() );

                destroy = true;
            }
            else {
                AIBattleLose( hero, res, true );

                // The army can include both the original monsters and their upgraded version
                const uint32_t monstersLeft = army.getTotalCount();
#ifndef NDEBUG
                const Monster originalMonster = troop.GetMonster();
                const Monster upgradedMonster = originalMonster.GetUpgrade();

                if ( upgradedMonster == originalMonster ) {
                    assert( monstersLeft == army.GetCountMonsters( originalMonster ) );
                }
                else {
                    assert( monstersLeft == army.GetCountMonsters( originalMonster ) + army.GetCountMonsters( upgradedMonster ) );
                }
#endif

                if ( monstersLeft > 0 ) {
                    setMonsterCountOnTile( tile, monstersLeft );

                    if ( Maps::isMonsterOnTileJoinConditionFree( tile ) ) {
                        Maps::setMonsterOnTileJoinCondition( tile, Monster::JOIN_CONDITION_MONEY );
                    }
                }
                else {
                    destroy = true;
                }
            }
        }

        if ( destroy ) {
            setMonsterCountOnTile( tile, 0 );

            tile.RemoveObjectSprite();
            tile.setAsEmpty();
        }
    }

    void AIToPickupResource( Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        Maps::Tiles & tile = world.GetTiles( dst_index );

        if ( objectType != MP2::OBJ_BOTTLE ) {
            hero.GetKingdom().AddFundsResource( getFundsFromTile( tile ) );
        }

        tile.RemoveObjectSprite();
        resetObjectInfoOnTile( tile );
        hero.GetPath().Reset();

        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() << " pickup small resource" )
    }

    void AIToTreasureChest( Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        Maps::Tiles & tile = world.GetTiles( dst_index );
        const Funds funds = getFundsFromTile( tile );

        assert( funds.gold > 0 || funds.GetValidItemsCount() == 0 );
        uint32_t gold = funds.gold;

        Kingdom & kingdom = hero.GetKingdom();

        if ( tile.isWater() ) {
            if ( gold ) {
                const Artifact & art = getArtifactFromTile( tile );

                if ( art.isValid() && !hero.PickupArtifact( art ) )
                    gold = GoldInsteadArtifact( objectType );
            }
        }
        else {
            const Artifact & art = getArtifactFromTile( tile );

            if ( gold ) {
                const uint32_t experience = gold > 500 ? gold - 500 : 500;
                const Heroes::Role role = hero.getAIRole();
                const int32_t kingdomGold = kingdom.GetFunds().gold;

                uint32_t chance = 0;
                if ( role == Heroes::Role::SCOUT || role == Heroes::Role::COURIER ) {
                    // These roles usually don't choose experience. Make AI rich!
                    if ( kingdomGold > 10000 && experience >= 1500 ) {
                        chance = 10;
                    }
                }
                else if ( role == Heroes::Role::CHAMPION ) {
                    // If AI is extremely low on gold consider taking it
                    if ( kingdomGold < 3000 ) {
                        // Safeguard the calculation since we're working with unsigned values
                        chance = ( std::max( experience, 500U ) - 500 ) / 15;
                    }
                    else {
                        // Otherwise Champion always picks experience
                        chance = 100;
                    }
                }
                else if ( kingdomGold < 3000 ) {
                    chance = ( role == Heroes::Role::FIGHTER && experience >= 1500 ) ? 10 : 0;
                }
                else {
                    uint32_t value = ( experience > 500 ) ? experience - 500 : 0;
                    if ( role == Heroes::Role::FIGHTER ) {
                        value += 500;
                    }
                    chance = value / 15; // 33% for every 500 experience
                }

                if ( chance ) {
                    const uint32_t randomRoll = Rand::Get( 1, 100 );
                    if ( randomRoll <= chance ) {
                        gold = 0;
                        hero.IncreaseExperience( experience );
                    }
                }
            }
            else if ( art.isValid() && !hero.PickupArtifact( art ) ) {
                gold = GoldInsteadArtifact( objectType );
            }
        }

        if ( gold ) {
            kingdom.AddFundsResource( Funds( Resource::GOLD, gold ) );
        }

        tile.RemoveObjectSprite();
        resetObjectInfoOnTile( tile );

        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() )
    }

    void AIToCaptureObject( Heroes & hero, const MP2::MapObjectType objectType, const int32_t dstIndex )
    {
        Maps::Tiles & tile = world.GetTiles( dstIndex );

        if ( !hero.isFriends( getColorFromTile( tile ) ) ) {
            auto removeObjectProtection = [&tile]() {
                // Clear any metadata related to spells
                if ( tile.GetObject( false ) == MP2::OBJ_MINES ) {
                    removeMineSpellFromTile( tile );
                }
            };

            auto captureObject = [&hero, &tile, &removeObjectProtection]() {
                removeObjectProtection();

                setColorOnTile( tile, hero.GetColor() );
            };

            if ( tile.isCaptureObjectProtected() ) {
                Army army( tile );

                Battle::Result result = Battle::Loader( hero.GetArmy(), army, dstIndex );

                if ( result.AttackerWins() ) {
                    hero.IncreaseExperience( result.GetExperienceAttacker() );

                    captureObject();
                }
                else {
                    // The army should include only the original monsters
                    const uint32_t monstersLeft = army.getTotalCount();
                    assert( monstersLeft == army.GetCountMonsters( getMonsterFromTile( tile ) ) );

                    Troop & troop = world.GetCapturedObject( dstIndex ).GetTroop();
                    troop.SetCount( monstersLeft );

                    // If all the guards are defeated, but the hero has lost the battle,
                    // just remove the protection from the object
                    if ( monstersLeft == 0 ) {
                        removeObjectProtection();
                    }

                    AIBattleLose( hero, result, true );
                }
            }
            else {
                captureObject();
            }
        }

#ifndef WITH_DEBUG
        (void)objectType;
#endif

        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() << " object: " << MP2::StringObject( objectType ) )
    }

    void AIToObjectResource( Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        Maps::Tiles & tile = world.GetTiles( dst_index );
        hero.GetKingdom().AddFundsResource( getFundsFromTile( tile ) );

        if ( MP2::isCaptureObject( objectType ) )
            AIToCaptureObject( hero, objectType, dst_index );

        resetObjectInfoOnTile( tile );
        hero.setVisitedForAllies( dst_index );

        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() )
    }

    void AIToSkeleton( Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        Maps::Tiles & tile = world.GetTiles( dst_index );

        if ( doesTileContainValuableItems( tile ) ) {
            const Artifact & art = getArtifactFromTile( tile );

            if ( !hero.PickupArtifact( art ) ) {
                uint32_t gold = GoldInsteadArtifact( objectType );
                hero.GetKingdom().AddFundsResource( Funds( Resource::GOLD, gold ) );
            }

            resetObjectInfoOnTile( tile );
        }

        hero.SetVisitedWideTile( dst_index, objectType, Visit::GLOBAL );

        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() )
    }

    void AIToWagon( Heroes & hero, int32_t dst_index )
    {
        Maps::Tiles & tile = world.GetTiles( dst_index );

        if ( doesTileContainValuableItems( tile ) ) {
            const Artifact & art = getArtifactFromTile( tile );

            if ( art.isValid() )
                hero.PickupArtifact( art );
            else
                hero.GetKingdom().AddFundsResource( getFundsFromTile( tile ) );

            resetObjectInfoOnTile( tile );
        }

        hero.SetVisited( dst_index, Visit::GLOBAL );

        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() )
    }

    void AIToFlotSam( const Heroes & hero, int32_t dst_index )
    {
        Maps::Tiles & tile = world.GetTiles( dst_index );

        hero.GetKingdom().AddFundsResource( getFundsFromTile( tile ) );
        tile.RemoveObjectSprite();
        resetObjectInfoOnTile( tile );

        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() )
    }

    void AIToSign( Heroes & hero, int32_t dst_index )
    {
        hero.SetVisited( dst_index, Visit::GLOBAL );
        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() )
    }

    void AIToObservationTower( Heroes & hero, int32_t dst_index )
    {
        Maps::ClearFog( dst_index, GameStatic::getFogDiscoveryDistance( GameStatic::FogDiscoveryType::OBSERVATION_TOWER ), hero.GetColor() );
        hero.SetVisited( dst_index, Visit::GLOBAL );
        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() )
    }

    void AIToMagellanMaps( Heroes & hero, int32_t dst_index )
    {
        const Funds payment( Resource::GOLD, 1000 );
        Kingdom & kingdom = hero.GetKingdom();

        if ( !hero.isObjectTypeVisited( MP2::OBJ_MAGELLANS_MAPS, Visit::GLOBAL ) && kingdom.AllowPayment( payment ) ) {
            hero.SetVisited( dst_index, Visit::GLOBAL );
            hero.setVisitedForAllies( dst_index );
            world.ActionForMagellanMaps( hero.GetColor() );

            if ( Players::isFriends( hero.GetColor(), Players::HumanColors() ) ) {
                // Fully update fog directions if AI player is an ally.
                Interface::GameArea::updateMapFogDirections();
            }

            kingdom.OddFundsResource( payment );
        }

        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() )
    }

    void AIToTeleports( Heroes & hero, const int32_t startIndex )
    {
        assert( hero.GetPath().empty() );

        const int32_t indexTo = world.NextTeleport( startIndex );
        if ( startIndex == indexTo ) {
            DEBUG_LOG( DBG_AI, DBG_WARN, "AI hero " << hero.GetName() << " has nowhere to go through stone liths." )
            return;
        }

        assert( world.GetTiles( indexTo ).GetObject() != MP2::OBJ_HEROES );

        if ( AIHeroesShowAnimation( hero, AIGetAllianceColors() ) ) {
            // AI-controlled hero cannot activate Stone Liths from the same tile, but should move to this tile from some
            // other tile first, so there is no need to re-center the game area on the hero before his disappearance
            hero.FadeOut();
        }

        hero.Move2Dest( indexTo );
        hero.GetPath().Reset();

        if ( AIHeroesShowAnimation( hero, AIGetAllianceColors() ) ) {
            Interface::AdventureMap::Get().getGameArea().SetCenter( hero.GetCenter() );
            hero.FadeIn();
        }

        hero.ActionNewPosition( false );

        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() )
    }

    void AIWhirlpoolTroopLoseEffect( Heroes & hero )
    {
        Army & heroArmy = hero.GetArmy();

        // Arrange the hero's army for the passage of the whirlpool first
        heroArmy.ArrangeForWhirlpool();

        Troop * weakestTroop = heroArmy.GetWeakestTroop();
        assert( weakestTroop != nullptr );
        if ( weakestTroop == nullptr ) {
            return;
        }

        // Whirlpool effect affects heroes only with more than one creature in more than one slot
        if ( heroArmy.GetOccupiedSlotCount() == 1 && weakestTroop->GetCount() == 1 ) {
            return;
        }

        if ( 1 == Rand::Get( 1, 3 ) ) {
            if ( weakestTroop->GetCount() == 1 ) {
                DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() << " lost " << weakestTroop->GetCount() << " " << weakestTroop->GetName() << " in the whirlpool" )

                weakestTroop->Reset();
            }
            else {
                const uint32_t newCount = Monster::GetCountFromHitPoints( weakestTroop->GetID(), weakestTroop->GetHitPoints()
                                                                                                     - weakestTroop->GetHitPoints() * Game::GetWhirlpoolPercent() / 100 );

                DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() << " lost " << weakestTroop->GetCount() - newCount << " " << weakestTroop->GetName() << " in the whirlpool" )

                weakestTroop->SetCount( newCount );
            }
        }
    }

    void AIToWhirlpools( Heroes & hero, const int32_t startIndex )
    {
        assert( hero.GetPath().empty() );

        const int32_t indexTo = world.NextWhirlpool( startIndex );
        if ( startIndex == indexTo ) {
            DEBUG_LOG( DBG_AI, DBG_WARN, "AI hero " << hero.GetName() << " has nowhere to go through the whirlpool." )
            return;
        }

        if ( AIHeroesShowAnimation( hero, AIGetAllianceColors() ) ) {
            // AI-controlled hero cannot activate Whirlpool from the same tile, but should move to this tile from some
            // other tile first, so there is no need to re-center the game area on the hero before his disappearance
            hero.FadeOut();
        }

        hero.Move2Dest( indexTo );
        hero.GetPath().Reset();

        AIWhirlpoolTroopLoseEffect( hero );

        if ( AIHeroesShowAnimation( hero, AIGetAllianceColors() ) ) {
            Interface::AdventureMap::Get().getGameArea().SetCenter( hero.GetCenter() );
            hero.FadeIn();
        }

        hero.ActionNewPosition( false );

        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() )
    }

    void AIToPrimarySkillObject( Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        const Maps::Tiles & tile = world.GetTiles( dst_index );

        int skill = Skill::Primary::UNKNOWN;

        switch ( objectType ) {
        case MP2::OBJ_FORT:
            skill = Skill::Primary::DEFENSE;
            break;
        case MP2::OBJ_MERCENARY_CAMP:
            skill = Skill::Primary::ATTACK;
            break;
        case MP2::OBJ_WITCH_DOCTORS_HUT:
            skill = Skill::Primary::KNOWLEDGE;
            break;
        case MP2::OBJ_STANDING_STONES:
            skill = Skill::Primary::POWER;
            break;
        case MP2::OBJ_ARENA:
            skill = AISelectSkillFromArena( hero );
            break;
        default:
            break;
        }

        if ( ( MP2::OBJ_ARENA == objectType && !hero.isObjectTypeVisited( objectType ) ) || !hero.isVisited( tile ) ) {
            // increase skill
            hero.IncreasePrimarySkill( skill );
            hero.SetVisited( dst_index );

            // fix double action tile
            hero.SetVisitedWideTile( dst_index, objectType );
        }

        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() )
    }

    void AIToExperienceObject( Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        const Maps::Tiles & tile = world.GetTiles( dst_index );

        uint32_t exp = 0;

        switch ( objectType ) {
        case MP2::OBJ_GAZEBO:
            exp = 1000;
            break;
        default:
            break;
        }

        // check already visited
        if ( !hero.isVisited( tile ) && exp ) {
            hero.SetVisited( dst_index );
            hero.IncreaseExperience( exp );
        }

        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() )
    }

    void AIToWitchsHut( Heroes & hero, int32_t dst_index )
    {
        const Skill::Secondary & skill = getSecondarySkillFromWitchsHut( world.GetTiles( dst_index ) );

        // check full
        if ( skill.isValid() && !hero.HasMaxSecondarySkill() && !hero.HasSecondarySkill( skill.Skill() ) )
            hero.LearnSkill( skill );

        hero.SetVisited( dst_index );
        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() )
    }

    void AIToShrine( Heroes & hero, int32_t dst_index )
    {
        const Spell & spell = getSpellFromTile( world.GetTiles( dst_index ) );
        assert( spell.isValid() );

        if ( hero.HaveSpellBook() && !hero.HaveSpell( spell, true ) &&
             // check valid level spell and wisdom skill
             !( 3 == spell.Level() && Skill::Level::NONE == hero.GetLevelSkill( Skill::Secondary::WISDOM ) ) ) {
            hero.AppendSpellToBook( spell );
        }

        // All heroes will know which spell is here.
        hero.SetVisited( dst_index, Visit::GLOBAL );

        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() )
    }

    void AIToGoodLuckObject( Heroes & hero, int32_t dst_index )
    {
        hero.SetVisited( dst_index );
        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() )
    }

    void AIToGoodMoraleObject( Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        uint32_t move = 0;

        switch ( objectType ) {
        case MP2::OBJ_OASIS:
            move = GameStatic::getMovementPointBonus( MP2::OBJ_OASIS );
            break;
        case MP2::OBJ_WATERING_HOLE:
            move = GameStatic::getMovementPointBonus( MP2::OBJ_WATERING_HOLE );
            break;
        default:
            break;
        }

        // check already visited
        if ( !hero.isObjectTypeVisited( objectType ) ) {
            // modify morale
            hero.SetVisited( dst_index );
            if ( move )
                hero.IncreaseMovePoints( move );

            // fix double action tile
            hero.SetVisitedWideTile( dst_index, objectType );
        }

        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() )
    }

    void AIToMagicWell( Heroes & hero, int32_t dst_index )
    {
        const uint32_t max = hero.GetMaxSpellPoints();

        if ( hero.GetSpellPoints() < max && !hero.isObjectTypeVisited( MP2::OBJ_MAGIC_WELL ) ) {
            hero.SetSpellPoints( max );
        }

        hero.SetVisited( dst_index );

        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() )
    }

    void AIToArtesianSpring( Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        const uint32_t max = hero.GetMaxSpellPoints();

        if ( !world.isAnyKingdomVisited( objectType, dst_index ) && hero.GetSpellPoints() < max * 2 ) {
            hero.SetSpellPoints( max * 2 );
        }

        hero.SetVisitedWideTile( dst_index, objectType, Visit::GLOBAL );

        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() )
    }

    void AIToXanadu( Heroes & hero, int32_t dst_index )
    {
        const Maps::Tiles & tile = world.GetTiles( dst_index );

        if ( !hero.isVisited( tile ) && GameStatic::isHeroWorthyToVisitXanadu( hero ) ) {
            hero.IncreasePrimarySkill( Skill::Primary::ATTACK );
            hero.IncreasePrimarySkill( Skill::Primary::DEFENSE );
            hero.IncreasePrimarySkill( Skill::Primary::KNOWLEDGE );
            hero.IncreasePrimarySkill( Skill::Primary::POWER );
            hero.SetVisited( dst_index );
        }

        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() )
    }

    void AIToEvent( Heroes & hero, int32_t dst_index )
    {
        // check event maps
        MapEvent * event_maps = world.GetMapEvent( Maps::GetPoint( dst_index ) );

        if ( event_maps && event_maps->isAllow( hero.GetColor() ) && event_maps->computer ) {
            if ( event_maps->resources.GetValidItemsCount() )
                hero.GetKingdom().AddFundsResource( event_maps->resources );
            if ( event_maps->artifact.isValid() )
                hero.PickupArtifact( event_maps->artifact );
            event_maps->SetVisited( hero.GetColor() );

            if ( event_maps->cancel ) {
                hero.SetMapsObject( MP2::OBJ_NONE );
                world.RemoveMapObject( event_maps );
            }
        }

        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() )
    }

    void AIToUpgradeArmyObject( Heroes & hero, const MP2::MapObjectType objectType, int32_t /*dst_index*/ )
    {
        switch ( objectType ) {
        case MP2::OBJ_HILL_FORT:
            if ( hero.GetArmy().HasMonster( Monster::DWARF ) )
                hero.GetArmy().UpgradeMonsters( Monster::DWARF );
            if ( hero.GetArmy().HasMonster( Monster::ORC ) )
                hero.GetArmy().UpgradeMonsters( Monster::ORC );
            if ( hero.GetArmy().HasMonster( Monster::OGRE ) )
                hero.GetArmy().UpgradeMonsters( Monster::OGRE );
            break;

        case MP2::OBJ_FREEMANS_FOUNDRY:
            if ( hero.GetArmy().HasMonster( Monster::PIKEMAN ) )
                hero.GetArmy().UpgradeMonsters( Monster::PIKEMAN );
            if ( hero.GetArmy().HasMonster( Monster::SWORDSMAN ) )
                hero.GetArmy().UpgradeMonsters( Monster::SWORDSMAN );
            if ( hero.GetArmy().HasMonster( Monster::IRON_GOLEM ) )
                hero.GetArmy().UpgradeMonsters( Monster::IRON_GOLEM );
            break;

        default:
            break;
        }

        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() )
    }

    void AIToPoorMoraleObject( Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        Maps::Tiles & tile = world.GetTiles( dst_index );
        const Funds funds = getFundsFromTile( tile );
        assert( funds.gold > 0 || funds.GetValidItemsCount() == 0 );

        uint32_t gold = funds.gold;
        bool complete = false;

        if ( gold ) {
            Army army( tile );

            Battle::Result res = Battle::Loader( hero.GetArmy(), army, dst_index );
            if ( res.AttackerWins() ) {
                hero.IncreaseExperience( res.GetExperienceAttacker() );
                complete = true;

                Artifact art;
                if ( isArtifactObject( objectType ) ) {
                    art = getArtifactFromTile( tile );
                }

                if ( art.isValid() && !hero.PickupArtifact( art ) ) {
                    gold = GoldInsteadArtifact( objectType );
                }

                hero.GetKingdom().AddFundsResource( Funds( Resource::GOLD, gold ) );
            }
            else {
                AIBattleLose( hero, res, true );
            }
        }

        if ( complete )
            resetObjectInfoOnTile( tile );
        else if ( 0 == gold && !hero.isObjectTypeVisited( objectType ) ) {
            // modify morale
            hero.SetVisited( dst_index );
            hero.SetVisited( dst_index, Visit::GLOBAL );
        }

        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() )
    }

    void AIToPyramid( Heroes & hero, int32_t dst_index )
    {
        Maps::Tiles & tile = world.GetTiles( dst_index );
        const Spell & spell = getSpellFromTile( tile );

        if ( spell.isValid() ) {
            // battle
            Army army( tile );

            Battle::Result res = Battle::Loader( hero.GetArmy(), army, dst_index );

            if ( res.AttackerWins() ) {
                hero.IncreaseExperience( res.GetExperienceAttacker() );

                // check magic book
                if ( hero.HaveSpellBook() &&
                     // check skill level for wisdom
                     Skill::Level::EXPERT == hero.GetLevelSkill( Skill::Secondary::WISDOM ) ) {
                    hero.AppendSpellToBook( spell );
                }

                resetObjectInfoOnTile( tile );
                hero.SetVisited( dst_index, Visit::GLOBAL );
            }
            else {
                AIBattleLose( hero, res, true );
            }
        }
        else {
            hero.SetVisited( dst_index, Visit::LOCAL );
            hero.SetVisited( dst_index, Visit::GLOBAL );
        }

        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() )
    }

    void AIToObelisk( Heroes & hero, const Maps::Tiles & tile )
    {
        if ( !hero.isVisited( tile, Visit::GLOBAL ) ) {
            hero.SetVisited( tile.GetIndex(), Visit::GLOBAL );
            Kingdom & kingdom = hero.GetKingdom();
            kingdom.PuzzleMaps().Update( kingdom.CountVisitedObjects( MP2::OBJ_OBELISK ), world.CountObeliskOnMaps() );
            // TODO: once any piece of the puzzle is open and the area is discovered and a hero has nothing to do make him dig around this area.
            // TODO: once all pieces are open add a special condition to make AI to dig the Ultimate artifact.
            // TODO: do not dig for an Ultimate artifact if the winning condition of the map is to find this artifact.
        }

        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() )
    }

    void AIToTreeKnowledge( Heroes & hero, int32_t dst_index )
    {
        const Maps::Tiles & tile = world.GetTiles( dst_index );

        if ( !hero.isVisited( tile ) ) {
            const Funds & payment = getTreeOfKnowledgeRequirement( tile );

            if ( 0 == payment.GetValidItemsCount() || hero.GetKingdom().AllowPayment( payment ) ) {
                const int level = hero.GetLevel();
                assert( level > 0 );
                const uint32_t experience = Heroes::GetExperienceFromLevel( level ) - Heroes::GetExperienceFromLevel( level - 1 );

                hero.GetKingdom().OddFundsResource( payment );
                hero.SetVisited( dst_index );
                hero.IncreaseExperience( experience );
            }
        }

        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() )
    }

    void AIToDaemonCave( Heroes & hero, int32_t dst_index )
    {
        Maps::Tiles & tile = world.GetTiles( dst_index );

        if ( doesTileContainValuableItems( tile ) ) {
            Army army( tile );

            Battle::Result res = Battle::Loader( hero.GetArmy(), army, dst_index );
            if ( res.AttackerWins() ) {
                hero.IncreaseExperience( res.GetExperienceAttacker() );
                // Daemon Cave always gives 2500 Gold after a battle
                hero.GetKingdom().AddFundsResource( Funds( Resource::GOLD, 2500 ) );
            }
            else {
                AIBattleLose( hero, res, true );
            }

            resetObjectInfoOnTile( tile );
        }

        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() )
    }

    void AIToDwellingJoinMonster( Heroes & hero, int32_t dst_index )
    {
        Maps::Tiles & tile = world.GetTiles( dst_index );
        const Troop & troop = getTroopFromTile( tile );
        if ( !troop.isValid() ) {
            return;
        }

        if ( !canMonsterJoinHero( troop, hero ) ) {
            return;
        }

        if ( !hero.GetArmy().JoinTroop( troop ) ) {
            // How is it possible that an army has free slots but monsters cannot join it?
            assert( 0 );
            return;
        }

        setMonsterCountOnTile( tile, 0 );

        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() )
    }

    void AIToDwellingRecruitMonster( Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        Maps::Tiles & tile = world.GetTiles( dst_index );
        const Troop & troop = getTroopFromTile( tile );
        if ( !troop.isValid() ) {
            return;
        }

        Kingdom & kingdom = hero.GetKingdom();
        const payment_t singleMonsterCost = troop.GetCost();

        uint32_t recruitTroopCount = kingdom.GetFunds().getLowestQuotient( singleMonsterCost );
        if ( recruitTroopCount <= 0 ) {
            // We do not have resources to hire even a single creature.
            return;
        }

        const uint32_t availableTroopCount = troop.GetCount();
        if ( recruitTroopCount > availableTroopCount ) {
            recruitTroopCount = availableTroopCount;
        }

        const Troop troopToHire{ troop.GetID(), recruitTroopCount };

        if ( !canMonsterJoinHero( troopToHire, hero ) ) {
            return;
        }

        if ( !hero.GetArmy().JoinTroop( troopToHire ) ) {
            // How is it possible that an army has free slots but monsters cannot join it?
            assert( 0 );
            return;
        }

        setMonsterCountOnTile( tile, availableTroopCount - recruitTroopCount );
        kingdom.OddFundsResource( troopToHire.GetTotalCost() );

        // Remove genie lamp sprite if no genies are available to hire.
        if ( MP2::OBJ_GENIE_LAMP == objectType && ( availableTroopCount == recruitTroopCount ) ) {
            tile.RemoveObjectSprite();
            tile.setAsEmpty();
        }

        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() )
    }

    void AIToDwellingBattleMonster( Heroes & hero, const MP2::MapObjectType objectType, const int32_t tileIndex )
    {
        Maps::Tiles & tile = world.GetTiles( tileIndex );
        bool recruitmentAllowed = true;

        if ( getColorFromTile( tile ) == Color::NONE ) {
            Army army( tile );

            const Battle::Result res = Battle::Loader( hero.GetArmy(), army, tileIndex );
            if ( res.AttackerWins() ) {
                hero.IncreaseExperience( res.GetExperienceAttacker() );

                // Set ownership of the dwelling to a Neutral (gray) player so that any player can recruit troops without a fight.
                setColorOnTile( tile, Color::UNUSED );
                tile.SetObjectPassable( true );
            }
            else {
                AIBattleLose( hero, res, true );

                recruitmentAllowed = false;
            }
        }

        if ( recruitmentAllowed ) {
            const Troop troop = getTroopFromTile( tile );

            if ( troop.isValid() ) {
                AIToDwellingRecruitMonster( hero, objectType, tileIndex );
            }

            hero.SetVisited( tileIndex, Visit::GLOBAL );
        }

        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() << ", object: " << MP2::StringObject( objectType ) )
    }

    void AIToStables( Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        // check already visited
        if ( !hero.isObjectTypeVisited( objectType ) ) {
            hero.SetVisited( dst_index );
            hero.IncreaseMovePoints( GameStatic::getMovementPointBonus( MP2::OBJ_STABLES ) );
        }

        if ( hero.GetArmy().HasMonster( Monster::CAVALRY ) )
            hero.GetArmy().UpgradeMonsters( Monster::CAVALRY );

        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() )
    }

    void AIToAbandonedMine( Heroes & hero, const int32_t dstIndex )
    {
        Maps::Tiles & tile = world.GetTiles( dstIndex );

        Army army( tile );

        Battle::Result result = Battle::Loader( hero.GetArmy(), army, dstIndex );

        if ( result.AttackerWins() ) {
            hero.IncreaseExperience( result.GetExperienceAttacker() );

            Maps::Tiles::RestoreAbandonedMine( tile, Resource::GOLD );
            hero.SetMapsObject( MP2::OBJ_MINES );
            setColorOnTile( tile, hero.GetColor() );
        }
        else {
            AIBattleLose( hero, result, true );
        }

        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() )
    }

    void AIToBarrier( const Heroes & hero, int32_t dst_index )
    {
        Maps::Tiles & tile = world.GetTiles( dst_index );
        const Kingdom & kingdom = hero.GetKingdom();

        if ( kingdom.IsVisitTravelersTent( getColorFromTile( tile ) ) ) {
            tile.RemoveObjectSprite();
            tile.setAsEmpty();
        }

        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() )
    }

    void AIToTravellersTent( const Heroes & hero, int32_t dst_index )
    {
        const Maps::Tiles & tile = world.GetTiles( dst_index );
        Kingdom & kingdom = hero.GetKingdom();

        kingdom.SetVisitTravelersTent( getColorFromTile( tile ) );

        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() )
    }

    void AIToShipwreckSurvivor( Heroes & hero, const MP2::MapObjectType objectType, int32_t dst_index )
    {
        Maps::Tiles & tile = world.GetTiles( dst_index );

        if ( hero.IsFullBagArtifacts() )
            hero.GetKingdom().AddFundsResource( Funds( Resource::GOLD, GoldInsteadArtifact( objectType ) ) );
        else
            hero.PickupArtifact( getArtifactFromTile( tile ) );

        tile.RemoveObjectSprite();
        resetObjectInfoOnTile( tile );

        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() )
    }

    void AIToArtifact( Heroes & hero, int32_t dst_index )
    {
        Maps::Tiles & tile = world.GetTiles( dst_index );

        if ( !hero.IsFullBagArtifacts() ) {
            const Maps::ArtifactCaptureCondition condition = getArtifactCaptureCondition( tile );
            const Artifact art = getArtifactFromTile( tile );

            bool result = false;

            if ( condition == Maps::ArtifactCaptureCondition::PAY_2000_GOLD || condition == Maps::ArtifactCaptureCondition::PAY_2500_GOLD_AND_3_RESOURCES
                 || condition == Maps::ArtifactCaptureCondition::PAY_3000_GOLD_AND_5_RESOURCES ) {
                const Funds payment = getArtifactResourceRequirement( tile );

                if ( hero.GetKingdom().AllowPayment( payment ) ) {
                    result = true;
                    hero.GetKingdom().OddFundsResource( payment );
                }
            }
            else if ( condition == Maps::ArtifactCaptureCondition::HAVE_WISDOM_SKILL || condition == Maps::ArtifactCaptureCondition::HAVE_LEADERSHIP_SKILL ) {
                // TODO: do we need to check this condition?
                result = true;
            }
            else if ( condition >= Maps::ArtifactCaptureCondition::FIGHT_50_ROGUES && condition <= Maps::ArtifactCaptureCondition::FIGHT_1_BONE_DRAGON ) {
                Army army( tile );

                // new battle
                Battle::Result res = Battle::Loader( hero.GetArmy(), army, dst_index );
                if ( res.AttackerWins() ) {
                    hero.IncreaseExperience( res.GetExperienceAttacker() );
                    result = true;
                }
                else {
                    AIBattleLose( hero, res, true );
                }
            }
            else {
                result = true;
            }

            if ( result && hero.PickupArtifact( art ) ) {
                tile.RemoveObjectSprite();
                resetObjectInfoOnTile( tile );
            }
        }

        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() )
    }

    void AIToBoat( Heroes & hero, const int32_t dst_index )
    {
        if ( hero.isShipMaster() )
            return;

        hero.setLastGroundRegion( world.GetTiles( hero.GetIndex() ).GetRegion() );

        const fheroes2::Point offset( Maps::GetPoint( dst_index ) - hero.GetCenter() );

        if ( AIHeroesShowAnimation( hero, AIGetAllianceColors() ) ) {
            Interface::AdventureMap::Get().getGameArea().SetCenter( hero.GetCenter() );
            hero.FadeOut( offset );
        }

        hero.setDirection( world.GetTiles( dst_index ).getBoatDirection() );
        hero.ResetMovePoints();
        hero.Move2Dest( dst_index );
        hero.SetMapsObject( MP2::OBJ_NONE );
        world.GetTiles( dst_index ).resetObjectSprite();
        hero.SetShipMaster( true );
        hero.GetPath().Reset();

        AI::Get().HeroesClearTask( hero );

        world.GetTiles( dst_index ).resetBoatOwnerColor();

        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() )
    }

    void AIToCoast( Heroes & hero, const int32_t dst_index )
    {
        if ( !hero.isShipMaster() )
            return;

        const int fromIndex = hero.GetIndex();
        Maps::Tiles & from = world.GetTiles( fromIndex );

        // Calculate the offset before making the action.
        const fheroes2::Point prevPos = hero.GetCenter();
        const fheroes2::Point offset( Maps::GetPoint( dst_index ) - prevPos );

        hero.ResetMovePoints();
        hero.Move2Dest( dst_index );
        from.setBoat( Maps::GetDirection( fromIndex, dst_index ), hero.GetColor() );
        hero.SetShipMaster( false );
        hero.GetPath().Reset();

        if ( AIHeroesShowAnimation( hero, AIGetAllianceColors() ) ) {
            Interface::AdventureMap::Get().getGameArea().SetCenter( prevPos );
            hero.FadeIn( offset );
        }

        hero.ActionNewPosition( true );

        AI::Get().HeroesClearTask( hero );

        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() )
    }

    void AIToJail( const Heroes & hero, const int32_t tileIndex )
    {
        const Kingdom & kingdom = hero.GetKingdom();

        if ( kingdom.GetHeroes().size() < Kingdom::GetMaxHeroes() ) {
            Maps::Tiles & tile = world.GetTiles( tileIndex );

            tile.RemoveObjectSprite();
            tile.setAsEmpty();

            Heroes * prisoner = world.FromJailHeroes( tileIndex );

            if ( prisoner ) {
                prisoner->Recruit( hero.GetColor(), Maps::GetPoint( tileIndex ) );
            }
        }
    }

    void AIToHutMagi( Heroes & hero, const MP2::MapObjectType objectType, const int32_t tileIndex )
    {
        if ( !hero.isObjectTypeVisited( objectType, Visit::GLOBAL ) ) {
            hero.SetVisited( tileIndex, Visit::GLOBAL );

            const MapsIndexes eyeMagiIndexes = Maps::GetObjectPositions( MP2::OBJ_EYE_OF_MAGI, true );
            for ( const int32_t index : eyeMagiIndexes ) {
                Maps::ClearFog( index, GameStatic::getFogDiscoveryDistance( GameStatic::FogDiscoveryType::MAGI_EYES ), hero.GetColor() );
            }
        }
    }

    void AIToAlchemistTower( Heroes & hero )
    {
        BagArtifacts & bag = hero.GetBagArtifacts();
        const uint32_t cursed = static_cast<uint32_t>( std::count_if( bag.begin(), bag.end(), []( const Artifact & art ) { return art.containsCurses(); } ) );
        if ( cursed == 0 ) {
            return;
        }

        const payment_t payment = PaymentConditions::ForAlchemist();

        if ( hero.GetKingdom().AllowPayment( payment ) ) {
            hero.GetKingdom().OddFundsResource( payment );

            for ( Artifact & artifact : bag ) {
                if ( artifact.containsCurses() ) {
                    artifact = Artifact::UNKNOWN;
                }
            }
        }

        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() << " visited Alchemist Tower to remove " << cursed << " artifacts." )
    }

    void AIToSirens( Heroes & hero, const MP2::MapObjectType objectType, const int32_t objectIndex )
    {
        if ( hero.isObjectTypeVisited( objectType ) ) {
            return;
        }

        const uint32_t experience = hero.GetArmy().ActionToSirens();
        hero.IncreaseExperience( experience );

        hero.SetVisited( objectIndex );

        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() << " visited Sirens and got " << experience << " experience." )
    }
}

namespace AI
{
    void HeroesAction( Heroes & hero, const int32_t dst_index )
    {
        const Heroes::AIHeroMeetingUpdater heroMeetingUpdater( hero );

        const Maps::Tiles & tile = world.GetTiles( dst_index );
        const MP2::MapObjectType objectType = tile.GetObject( dst_index != hero.GetIndex() );

        const bool isActionObject = MP2::isActionObject( objectType, hero.isShipMaster() );
        if ( isActionObject )
            hero.SetModes( Heroes::ACTION );

        switch ( objectType ) {
        case MP2::OBJ_BOAT:
            AIToBoat( hero, dst_index );
            break;
        case MP2::OBJ_COAST:
            // Coast is not an action object by definition but we need to do hero's animation.
            AIToCoast( hero, dst_index );
            break;

        case MP2::OBJ_MONSTER:
            AIToMonster( hero, dst_index );
            break;
        case MP2::OBJ_HEROES:
            AIToHeroes( hero, dst_index );
            break;
        case MP2::OBJ_CASTLE:
            AIToCastle( hero, dst_index );
            break;

        // pickup object
        case MP2::OBJ_BOTTLE:
        case MP2::OBJ_CAMPFIRE:
        case MP2::OBJ_RESOURCE:
            AIToPickupResource( hero, objectType, dst_index );
            break;

        case MP2::OBJ_SEA_CHEST:
        case MP2::OBJ_TREASURE_CHEST:
            AIToTreasureChest( hero, objectType, dst_index );
            break;
        case MP2::OBJ_ARTIFACT:
            AIToArtifact( hero, dst_index );
            break;

        case MP2::OBJ_LEAN_TO:
        case MP2::OBJ_MAGIC_GARDEN:
        case MP2::OBJ_WINDMILL:
        case MP2::OBJ_WATER_WHEEL:
            AIToObjectResource( hero, objectType, dst_index );
            break;

        case MP2::OBJ_WAGON:
            AIToWagon( hero, dst_index );
            break;
        case MP2::OBJ_SKELETON:
            AIToSkeleton( hero, objectType, dst_index );
            break;
        case MP2::OBJ_FLOTSAM:
            AIToFlotSam( hero, dst_index );
            break;

        case MP2::OBJ_ALCHEMIST_LAB:
        case MP2::OBJ_MINES:
        case MP2::OBJ_SAWMILL:
        case MP2::OBJ_LIGHTHOUSE:
            AIToCaptureObject( hero, objectType, dst_index );
            break;

        case MP2::OBJ_ABANDONED_MINE:
            AIToAbandonedMine( hero, dst_index );
            break;

        case MP2::OBJ_SHIPWRECK_SURVIVOR:
            AIToShipwreckSurvivor( hero, objectType, dst_index );
            break;

        // event
        case MP2::OBJ_EVENT:
            AIToEvent( hero, dst_index );
            break;

        case MP2::OBJ_SIGN:
            AIToSign( hero, dst_index );
            break;

        // increase view
        case MP2::OBJ_OBSERVATION_TOWER:
            AIToObservationTower( hero, dst_index );
            break;
        case MP2::OBJ_MAGELLANS_MAPS:
            AIToMagellanMaps( hero, dst_index );
            break;

        // teleports
        case MP2::OBJ_STONE_LITHS:
            AIToTeleports( hero, dst_index );
            break;
        case MP2::OBJ_WHIRLPOOL:
            AIToWhirlpools( hero, dst_index );
            break;

        // primary skill modification
        case MP2::OBJ_FORT:
        case MP2::OBJ_MERCENARY_CAMP:
        case MP2::OBJ_WITCH_DOCTORS_HUT:
        case MP2::OBJ_STANDING_STONES:
            AIToPrimarySkillObject( hero, objectType, dst_index );
            break;

        // experience modification
        case MP2::OBJ_GAZEBO:
            AIToExperienceObject( hero, objectType, dst_index );
            break;

        // Witch's hut
        case MP2::OBJ_WITCHS_HUT:
            AIToWitchsHut( hero, dst_index );
            break;

        // shrine circle
        case MP2::OBJ_SHRINE_FIRST_CIRCLE:
        case MP2::OBJ_SHRINE_SECOND_CIRCLE:
        case MP2::OBJ_SHRINE_THIRD_CIRCLE:
            AIToShrine( hero, dst_index );
            break;

        // luck modification
        case MP2::OBJ_FOUNTAIN:
        case MP2::OBJ_FAERIE_RING:
        case MP2::OBJ_IDOL:
        case MP2::OBJ_MERMAID:
            AIToGoodLuckObject( hero, dst_index );
            break;

        // morale modification
        case MP2::OBJ_OASIS:
        case MP2::OBJ_TEMPLE:
        case MP2::OBJ_WATERING_HOLE:
        case MP2::OBJ_BUOY:
            AIToGoodMoraleObject( hero, objectType, dst_index );
            break;

        case MP2::OBJ_OBELISK:
            AIToObelisk( hero, tile );
            break;

        // magic point
        case MP2::OBJ_ARTESIAN_SPRING:
            AIToArtesianSpring( hero, objectType, dst_index );
            break;
        case MP2::OBJ_MAGIC_WELL:
            AIToMagicWell( hero, dst_index );
            break;

        // increase skill
        case MP2::OBJ_XANADU:
            AIToXanadu( hero, dst_index );
            break;

        case MP2::OBJ_HILL_FORT:
        case MP2::OBJ_FREEMANS_FOUNDRY:
            AIToUpgradeArmyObject( hero, objectType, dst_index );
            break;

        case MP2::OBJ_SHIPWRECK:
        case MP2::OBJ_GRAVEYARD:
        case MP2::OBJ_DERELICT_SHIP:
            AIToPoorMoraleObject( hero, objectType, dst_index );
            break;

        case MP2::OBJ_PYRAMID:
            AIToPyramid( hero, dst_index );
            break;
        case MP2::OBJ_DAEMON_CAVE:
            AIToDaemonCave( hero, dst_index );
            break;

        case MP2::OBJ_TREE_OF_KNOWLEDGE:
            AIToTreeKnowledge( hero, dst_index );
            break;

        // accept army
        case MP2::OBJ_WATCH_TOWER:
        case MP2::OBJ_EXCAVATION:
        case MP2::OBJ_CAVE:
        case MP2::OBJ_TREE_HOUSE:
        case MP2::OBJ_ARCHER_HOUSE:
        case MP2::OBJ_GOBLIN_HUT:
        case MP2::OBJ_DWARF_COTTAGE:
        case MP2::OBJ_HALFLING_HOLE:
        case MP2::OBJ_PEASANT_HUT:
            AIToDwellingJoinMonster( hero, dst_index );
            break;

        // recruit army
        case MP2::OBJ_RUINS:
        case MP2::OBJ_TREE_CITY:
        case MP2::OBJ_WAGON_CAMP:
        case MP2::OBJ_DESERT_TENT:
        // loyalty version objects
        case MP2::OBJ_WATER_ALTAR:
        case MP2::OBJ_AIR_ALTAR:
        case MP2::OBJ_FIRE_ALTAR:
        case MP2::OBJ_EARTH_ALTAR:
        case MP2::OBJ_BARROW_MOUNDS:
            AIToDwellingRecruitMonster( hero, objectType, dst_index );
            break;

        // recruit army (battle)
        case MP2::OBJ_DRAGON_CITY:
        case MP2::OBJ_CITY_OF_DEAD:
        case MP2::OBJ_TROLL_BRIDGE:
            AIToDwellingBattleMonster( hero, objectType, dst_index );
            break;

        // recruit genie
        case MP2::OBJ_GENIE_LAMP:
            AIToDwellingRecruitMonster( hero, objectType, dst_index );
            break;

        case MP2::OBJ_STABLES:
            AIToStables( hero, objectType, dst_index );
            break;
        case MP2::OBJ_ARENA:
            AIToPrimarySkillObject( hero, objectType, dst_index );
            break;

        case MP2::OBJ_BARRIER:
            AIToBarrier( hero, dst_index );
            break;
        case MP2::OBJ_TRAVELLER_TENT:
            AIToTravellersTent( hero, dst_index );
            break;

        case MP2::OBJ_JAIL:
            AIToJail( hero, dst_index );
            break;
        case MP2::OBJ_HUT_OF_MAGI:
            AIToHutMagi( hero, objectType, dst_index );
            break;
        case MP2::OBJ_ALCHEMIST_TOWER:
            AIToAlchemistTower( hero );
            break;

        // AI has no advantage or knowledge to use these objects
        case MP2::OBJ_ORACLE:
        case MP2::OBJ_TRADING_POST:
        case MP2::OBJ_EYE_OF_MAGI:
        case MP2::OBJ_SPHINX:
            break;
        case MP2::OBJ_SIRENS:
            // AI must have some action even if it goes on this object by mistake.
            AIToSirens( hero, objectType, dst_index );
            break;
        default:
            // AI should know what to do with this type of action object! Please add logic for it.
            assert( !isActionObject );
            break;
        }

        if ( MP2::isNeedStayFront( objectType ) )
            hero.GetPath().Reset();

        // ignore empty tiles
        if ( isActionObject )
            AI::Get().HeroesActionComplete( hero, dst_index, objectType );
    }

    void HeroesMove( Heroes & hero )
    {
        const Route::Path & path = hero.GetPath();

        if ( path.isValid() ) {
            hero.SetMove( true );

            Interface::AdventureMap & adventureMapInterface = Interface::AdventureMap::Get();
            Interface::GameArea & gameArea = adventureMapInterface.getGameArea();

            const Settings & conf = Settings::Get();

            const uint32_t colors = AIGetAllianceColors();
            bool recenterNeeded = true;

            int heroAnimationFrameCount = 0;
            fheroes2::Point heroAnimationOffset;
            int heroAnimationSpriteId = 0;

            const bool hideAIMovements = ( conf.AIMoveSpeed() == 0 );
            const bool noMovementAnimation = ( conf.AIMoveSpeed() == 10 );

            const std::vector<Game::DelayType> delayTypes = { Game::CURRENT_AI_DELAY, Game::MAPS_DELAY };

            fheroes2::Display & display = fheroes2::Display::instance();

            LocalEvent & le = LocalEvent::Get();
            while ( le.HandleEvents( !hideAIMovements && Game::isDelayNeeded( delayTypes ) ) ) {
#if defined( WITH_DEBUG )
                if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_TRANSFER_CONTROL_TO_AI ) && Players::Get( hero.GetColor() )->isAIAutoControlMode() ) {
                    if ( fheroes2::showStandardTextMessage( _( "Warning" ),
                                                            _( "Do you want to regain control from AI? The effect will take place only on the next turn." ),
                                                            Dialog::YES | Dialog::NO )
                         == Dialog::YES ) {
                        Players::Get( hero.GetColor() )->setAIAutoControlMode( false );
                        continue;
                    }
                }
#endif

                if ( hero.isFreeman() || !hero.isMoveEnabled() ) {
                    break;
                }

                if ( hideAIMovements || !AIHeroesShowAnimation( hero, colors ) ) {
                    hero.Move( true );
                    recenterNeeded = true;

                    // Render a frame only if there is a need to show one.
                    if ( Game::validateAnimationDelay( Game::MAPS_DELAY ) ) {
                        // Update Adventure Map objects' animation.
                        Game::updateAdventureMapAnimationIndex();

                        adventureMapInterface.redraw( Interface::REDRAW_GAMEAREA );

                        // If this assertion blows up it means that we are holding a RedrawLocker lock for rendering which should not happen.
                        assert( adventureMapInterface.getRedrawMask() == 0 );

                        display.render();
                    }
                }
                else if ( Game::validateAnimationDelay( Game::CURRENT_AI_DELAY ) ) {
                    // re-center in case hero appears from the fog
                    if ( recenterNeeded ) {
                        gameArea.SetCenter( hero.GetCenter() );
                        recenterNeeded = false;
                    }

                    bool resetHeroSprite = false;
                    if ( heroAnimationFrameCount > 0 ) {
                        const int32_t heroMovementSkipValue = Game::AIHeroAnimSkip();

                        gameArea.ShiftCenter( { heroAnimationOffset.x * heroMovementSkipValue, heroAnimationOffset.y * heroMovementSkipValue } );
                        gameArea.SetRedraw();
                        heroAnimationFrameCount -= heroMovementSkipValue;
                        if ( ( heroAnimationFrameCount & 0x3 ) == 0 ) { // % 4
                            hero.SetSpriteIndex( heroAnimationSpriteId );

                            if ( heroAnimationFrameCount == 0 )
                                resetHeroSprite = true;
                            else
                                ++heroAnimationSpriteId;
                        }
                        const int offsetStep = ( ( 4 - ( heroAnimationFrameCount & 0x3 ) ) & 0x3 ); // % 4
                        hero.SetOffset( { heroAnimationOffset.x * offsetStep, heroAnimationOffset.y * offsetStep } );
                    }

                    if ( heroAnimationFrameCount == 0 ) {
                        if ( resetHeroSprite ) {
                            hero.SetSpriteIndex( heroAnimationSpriteId - 1 );
                        }

                        if ( hero.Move( noMovementAnimation ) ) {
                            if ( AIHeroesShowAnimation( hero, colors ) ) {
                                gameArea.SetCenter( hero.GetCenter() );
                            }
                        }
                        else {
                            const fheroes2::Point movement( hero.MovementDirection() );
                            if ( movement != fheroes2::Point() ) { // don't waste resources for no movement
                                const int32_t heroMovementSkipValue = Game::AIHeroAnimSkip();

                                heroAnimationOffset = movement;
                                gameArea.ShiftCenter( movement );
                                heroAnimationFrameCount = 32 - heroMovementSkipValue;
                                heroAnimationSpriteId = hero.GetSpriteIndex();
                                if ( heroMovementSkipValue < 4 ) {
                                    hero.SetSpriteIndex( heroAnimationSpriteId - 1 );
                                    hero.SetOffset( { heroAnimationOffset.x * heroMovementSkipValue, heroAnimationOffset.y * heroMovementSkipValue } );
                                }
                                else {
                                    ++heroAnimationSpriteId;
                                }
                            }
                        }
                    }

                    if ( Game::validateAnimationDelay( Game::MAPS_DELAY ) ) {
                        // Update Adventure Map objects' animation.
                        Game::updateAdventureMapAnimationIndex();
                    }

                    adventureMapInterface.redraw( Interface::REDRAW_GAMEAREA );

                    // If this assertion blows up it means that we are holding a RedrawLocker lock for rendering which should not happen.
                    assert( adventureMapInterface.getRedrawMask() == 0 );

                    display.render();
                }
            }

            hero.SetMove( false );
        }
        else if ( !path.empty() && path.GetFrontDirection() == Direction::UNKNOWN ) {
            const Route::Step & step = path.front();
            const int32_t targetIndex = step.GetIndex();

            if ( step.GetFrom() != targetIndex && world.GetTiles( targetIndex ).GetObject() == MP2::OBJ_CASTLE ) {
                HeroesCastTownPortal( hero, targetIndex );
            }
            else if ( MP2::isActionObject( hero.GetMapsObject(), hero.isShipMaster() ) ) {
                // use the action object hero is standing on (Stone Liths)
                hero.Action( hero.GetIndex() );
            }
        }
    }

    void HeroesCastDimensionDoor( Heroes & hero, const int32_t targetIndex )
    {
        if ( !Maps::isValidAbsIndex( targetIndex ) || hero.isShipMaster() != world.GetTiles( targetIndex ).isWater() ) {
            return;
        }

        const Spell dimensionDoor( Spell::DIMENSIONDOOR );
        if ( !hero.CanCastSpell( dimensionDoor ) ) {
            // How is it even possible to call this function if the hero does not have this spell?
            assert( 0 );
            return;
        }

#if defined( WITH_DEBUG )
        if ( Players::Get( hero.GetColor() )->isAIAutoControlMode() ) {
            LocalEvent::Get().HandleEvents( false );
            if ( HotKeyPressEvent( Game::HotKeyEvent::WORLD_TRANSFER_CONTROL_TO_AI )
                 && fheroes2::showStandardTextMessage( _( "Warning" ), _( "Do you want to regain control from AI? The effect will take place only on the next turn." ),
                                                       Dialog::YES | Dialog::NO )
                        == Dialog::YES ) {
                Players::Get( hero.GetColor() )->setAIAutoControlMode( false );
            }
        }
#endif

        if ( AIHeroesShowAnimation( hero, AIGetAllianceColors() ) ) {
            Interface::AdventureMap::Get().getGameArea().SetCenter( hero.GetCenter() );
            hero.FadeOut();
        }

        hero.Move2Dest( targetIndex );
        hero.SpellCasted( dimensionDoor );
        hero.GetPath().Reset();

        if ( AIHeroesShowAnimation( hero, AIGetAllianceColors() ) ) {
            Interface::AdventureMap::Get().getGameArea().SetCenter( hero.GetCenter() );
            hero.FadeIn();
        }

        hero.ActionNewPosition( false );
    }

    void HeroesCastTownPortal( Heroes & hero, const int32_t targetIndex )
    {
        if ( !Maps::isValidAbsIndex( targetIndex ) || hero.isShipMaster() ) {
            return;
        }

        Spell spellToUse( Spell::TOWNPORTAL );

        // check if we can cast Town Gate instead
        const Spell townGate( Spell::TOWNGATE );
        const Castle * nearestCastle = fheroes2::getNearestCastleTownGate( hero );
        if ( nearestCastle && nearestCastle->GetIndex() == targetIndex && hero.HaveSpell( townGate ) ) {
            spellToUse = townGate;
        }

        if ( !hero.CanCastSpell( spellToUse ) ) {
            return;
        }

        if ( AIHeroesShowAnimation( hero, AIGetAllianceColors() ) ) {
            Interface::AdventureMap::Get().getGameArea().SetCenter( hero.GetCenter() );
            hero.FadeOut();
        }

        hero.Move2Dest( targetIndex );
        hero.SpellCasted( spellToUse );
        hero.GetPath().Reset();

        if ( AIHeroesShowAnimation( hero, AIGetAllianceColors() ) ) {
            Interface::AdventureMap::Get().getGameArea().SetCenter( hero.GetCenter() );
            hero.FadeIn();
        }

        AI::Get().HeroesActionComplete( hero, targetIndex, hero.GetMapsObject() );
    }

    bool HeroesCastAdventureSpell( Heroes & hero, const Spell & spell )
    {
        if ( !hero.CanCastSpell( spell ) )
            return false;

        hero.SpellCasted( spell );

        return true;
    }
}
