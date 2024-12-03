/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
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
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "ai_planner.h"
#include "army.h"
#include "army_troop.h"
#include "artifact.h"
#include "battle.h" // IWYU pragma: associated
#include "battle_arena.h"
#include "battle_army.h"
#include "campaign_savedata.h"
#include "captain.h"
#include "dialog.h"
#include "game.h"
#include "heroes.h"
#include "heroes_base.h"
#include "kingdom.h"
#include "logging.h"
#include "monster.h"
#include "players.h"
#include "rand.h"
#include "resource.h"
#include "settings.h"
#include "skill.h"
#include "spell.h"
#include "spell_storage.h"
#include "tools.h"
#include "translations.h"
#include "ui_dialog.h"
#include "world.h"

namespace
{
    std::vector<Artifact> planArtifactTransfer( const BagArtifacts & winnerBag, const BagArtifacts & loserBag )
    {
        std::vector<Artifact> artifacts;

        // Calculate how many free slots the winner has in his bag.
        size_t availableArtifactSlots = 0;
        for ( const Artifact & artifact : winnerBag ) {
            if ( !artifact.isValid() ) {
                ++availableArtifactSlots;
            }
        }

        for ( const Artifact & artifact : loserBag ) {
            if ( availableArtifactSlots == 0 ) {
                break;
            }
            if ( artifact.isValid() && artifact.GetID() != Artifact::MAGIC_BOOK && !artifact.isUltimate() ) {
                artifacts.push_back( artifact );
                --availableArtifactSlots;
            }
        }

        // One more pass to put all the ultimate artifacts at the end.
        for ( const Artifact & artifact : loserBag ) {
            if ( artifact.isUltimate() ) {
                artifacts.push_back( artifact );
            }
        }

        return artifacts;
    }

    void transferArtifacts( BagArtifacts & winnerBag, const std::vector<Artifact> & artifacts )
    {
        size_t artifactPos = 0;

        for ( Artifact & artifact : winnerBag ) {
            if ( artifact.isValid() ) {
                continue;
            }
            for ( ; artifactPos < artifacts.size(); ++artifactPos ) {
                if ( !artifacts[artifactPos].isUltimate() ) {
                    artifact = artifacts[artifactPos];
                    ++artifactPos;
                    break;
                }
            }
            if ( artifactPos >= artifacts.size() ) {
                break;
            }
        }
    }

    void clearArtifacts( BagArtifacts & bag )
    {
        for ( Artifact & artifact : bag ) {
            if ( artifact.isValid() && artifact.GetID() != Artifact::MAGIC_BOOK ) {
                artifact = Artifact::UNKNOWN;
            }
        }
    }

    uint32_t computeBattleSeed( const int32_t mapIndex, const uint32_t mapSeed, const Army & army1, const Army & army2 )
    {
        uint32_t seed = static_cast<uint32_t>( mapIndex ) + mapSeed;

        for ( size_t i = 0; i < army1.Size(); ++i ) {
            const Troop * troop = army1.GetTroop( i );
            if ( troop->isValid() ) {
                fheroes2::hashCombine( seed, troop->GetID() );
                fheroes2::hashCombine( seed, troop->GetCount() );
            }
            else {
                fheroes2::hashCombine( seed, 0 );
            }
        }

        for ( size_t i = 0; i < army2.Size(); ++i ) {
            const Troop * troop = army2.GetTroop( i );
            if ( troop->isValid() ) {
                fheroes2::hashCombine( seed, troop->GetID() );
                fheroes2::hashCombine( seed, troop->GetCount() );
            }
            else {
                fheroes2::hashCombine( seed, 0 );
            }
        }

        return seed;
    }

    uint32_t getBattleResult( const uint32_t army )
    {
        if ( army & Battle::RESULT_SURRENDER )
            return Battle::RESULT_SURRENDER;
        if ( army & Battle::RESULT_RETREAT )
            return Battle::RESULT_RETREAT;
        if ( army & Battle::RESULT_LOSS )
            return Battle::RESULT_LOSS;
        if ( army & Battle::RESULT_WINS )
            return Battle::RESULT_WINS;

        return 0;
    }

    void eagleEyeSkillAction( HeroBase & hero, const SpellStorage & spells, bool local, Rand::DeterministicRandomGenerator & randomGenerator )
    {
        if ( spells.empty() || !hero.HaveSpellBook() )
            return;

        SpellStorage new_spells;
        new_spells.reserve( 10 );

        const Skill::Secondary eagleeye( Skill::Secondary::EAGLE_EYE, hero.GetLevelSkill( Skill::Secondary::EAGLE_EYE ) );

        // filter spells
        for ( const Spell & sp : spells ) {
            if ( hero.HaveSpell( sp ) ) {
                continue;
            }

            switch ( eagleeye.Level() ) {
            case Skill::Level::BASIC:
                // 20%
                if ( 3 > sp.Level() && eagleeye.GetValue() >= randomGenerator.Get( 1, 100 ) )
                    new_spells.push_back( sp );
                break;
            case Skill::Level::ADVANCED:
                // 30%
                if ( 4 > sp.Level() && eagleeye.GetValue() >= randomGenerator.Get( 1, 100 ) )
                    new_spells.push_back( sp );
                break;
            case Skill::Level::EXPERT:
                // 40%
                if ( 5 > sp.Level() && eagleeye.GetValue() >= randomGenerator.Get( 1, 100 ) )
                    new_spells.push_back( sp );
                break;
            default:
                break;
            }
        }

        // add new spell
        if ( local ) {
            for ( const Spell & sp : new_spells ) {
                std::string msg = _( "Through eagle-eyed observation, %{name} is able to learn the magic spell %{spell}." );
                StringReplace( msg, "%{name}", hero.GetName() );
                StringReplace( msg, "%{spell}", sp.GetName() );
                Game::PlayPickupSound();

                const fheroes2::SpellDialogElement spellUI( sp, &hero );
                fheroes2::showStandardTextMessage( {}, std::move( msg ), Dialog::OK, { &spellUI } );
            }
        }

        hero.AppendSpellsToBook( new_spells, true );
    }

    void necromancySkillAction( HeroBase & hero, const uint32_t enemyTroopsKilled, const bool isControlHuman )
    {
        Army & army = hero.GetArmy();

        if ( 0 == enemyTroopsKilled || ( army.isFullHouse() && !army.HasMonster( Monster::SKELETON ) ) )
            return;

        const uint32_t necromancyPercent = GetNecromancyPercent( hero );

        uint32_t raiseCount = std::max( enemyTroopsKilled * necromancyPercent / 100, 1U );
        army.JoinTroop( Monster::SKELETON, raiseCount, false );

        if ( isControlHuman ) {
            Battle::Arena::DialogBattleNecromancy( raiseCount );
        }

        DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "raise: " << raiseCount << " skeletons" )
    }

    Kingdom * getKingdomOfCommander( const HeroBase * commander )
    {
        if ( commander == nullptr ) {
            return nullptr;
        }

        if ( commander->isHeroes() ) {
            const Heroes * hero = dynamic_cast<const Heroes *>( commander );
            assert( hero != nullptr );

            return &hero->GetKingdom();
        }

        if ( commander->isCaptain() ) {
            const Captain * captain = dynamic_cast<const Captain *>( commander );
            assert( captain != nullptr );

            return &world.GetKingdom( captain->GetColor() );
        }

        assert( 0 );
        return nullptr;
    }

    void restoreFundsOfCommandersKingdom( const HeroBase * commander, const Funds & initialFunds )
    {
        Kingdom * kingdom = getKingdomOfCommander( commander );
        assert( kingdom != nullptr );

        const Funds fundsDiff = kingdom->GetFunds() - initialFunds;
        assert( kingdom->AllowPayment( fundsDiff ) );

        kingdom->OddFundsResource( fundsDiff );

        assert( kingdom->GetFunds() == initialFunds );
    }
}

Battle::Result Battle::Loader( Army & army1, Army & army2, int32_t mapsindex )
{
    Result result;

    // Validate the arguments - check if battle should even load
    if ( !army1.isValid() || !army2.isValid() ) {
        // Check second army first so attacker would win by default
        if ( !army2.isValid() ) {
            result.army1 = RESULT_WINS;
            DEBUG_LOG( DBG_BATTLE, DBG_WARN, "Invalid battle detected! Index " << mapsindex << ", Army: " << army2.String() )
        }
        else {
            result.army2 = RESULT_WINS;
            DEBUG_LOG( DBG_BATTLE, DBG_WARN, "Invalid battle detected! Index " << mapsindex << ", Army: " << army1.String() )
        }
        return result;
    }

    HeroBase * commander1 = army1.GetCommander();
    if ( commander1 ) {
        commander1->ActionPreBattle();

        if ( army1.isControlAI() ) {
            AI::Planner::HeroesPreBattle( *commander1, true );
        }
    }

    const uint32_t initialSpellPoints1 = [commander1]() -> uint32_t {
        if ( commander1 == nullptr ) {
            return 0;
        }

        return commander1->GetSpellPoints();
    }();

    const Funds initialFunds1 = [commander1]() -> Funds {
        const Kingdom * kingdom = getKingdomOfCommander( commander1 );
        if ( kingdom == nullptr ) {
            return {};
        }

        return kingdom->GetFunds();
    }();

    HeroBase * commander2 = army2.GetCommander();
    if ( commander2 ) {
        commander2->ActionPreBattle();

        if ( army2.isControlAI() ) {
            AI::Planner::HeroesPreBattle( *commander2, false );
        }
    }

    const uint32_t initialSpellPoints2 = [commander2]() -> uint32_t {
        if ( commander2 == nullptr ) {
            return 0;
        }

        return commander2->GetSpellPoints();
    }();

    const Funds initialFunds2 = [commander2]() -> Funds {
        const Kingdom * kingdom = getKingdomOfCommander( commander2 );
        if ( kingdom == nullptr ) {
            return {};
        }

        return kingdom->GetFunds();
    }();

    const bool isHumanBattle = army1.isControlHuman() || army2.isControlHuman();

    const Settings & conf = Settings::Get();
    bool showBattle = !conf.BattleAutoResolve() && isHumanBattle;

#ifdef WITH_DEBUG
    if ( !showBattle ) {
        // The battle is always shown either in battle debugging mode ...
        if ( IS_DEBUG( DBG_BATTLE, DBG_TRACE ) ) {
            showBattle = true;
        }
        // ... or when any of the participating human players are controlled by AI
        else {
            const Player * player1 = Players::Get( army1.GetColor() );
            const Player * player2 = Players::Get( army2.GetColor() );

            if ( ( player1 != nullptr && player1->isAIAutoControlMode() ) || ( player2 != nullptr && player2->isAIAutoControlMode() ) ) {
                showBattle = true;
            }
        }
    }
#endif

    const uint32_t battleSeed = computeBattleSeed( mapsindex, world.GetMapSeed(), army1, army2 );

    while ( true ) {
        Rand::DeterministicRandomGenerator randomGenerator( battleSeed );
        Arena arena( army1, army2, mapsindex, showBattle, randomGenerator );

        DEBUG_LOG( DBG_BATTLE, DBG_INFO, "army1 " << army1.String() )
        DEBUG_LOG( DBG_BATTLE, DBG_INFO, "army2 " << army2.String() )

        while ( arena.BattleValid() ) {
            arena.Turns();
        }
        result = arena.GetResult();

        HeroBase * const winnerHero = ( result.army1 & RESULT_WINS ? commander1 : ( result.army2 & RESULT_WINS ? commander2 : nullptr ) );
        HeroBase * const loserHero = ( result.army1 & RESULT_LOSS ? commander1 : ( result.army2 & RESULT_LOSS ? commander2 : nullptr ) );
        const uint32_t lossResult = result.army1 & RESULT_LOSS ? result.army1 : result.army2;
        const bool loserAbandoned = !( ( RESULT_RETREAT | RESULT_SURRENDER ) & lossResult );

        const std::vector<Artifact> artifactsToTransfer = winnerHero && loserHero && loserAbandoned && winnerHero->isHeroes() && loserHero->isHeroes()
                                                              ? planArtifactTransfer( winnerHero->GetBagArtifacts(), loserHero->GetBagArtifacts() )
                                                              : std::vector<Artifact>();

        if ( showBattle ) {
            const bool clearMessageLog = ( result.army1 & ( RESULT_RETREAT | RESULT_SURRENDER ) ) || ( result.army2 & ( RESULT_RETREAT | RESULT_SURRENDER ) );
            arena.FadeArena( clearMessageLog );
        }

        if ( isHumanBattle && arena.DialogBattleSummary( result, artifactsToTransfer, !showBattle ) ) {
            // If dialog returns true we will restart battle in manual mode
            showBattle = true;

            // Reset the state of army commanders and the state of their kingdoms' finances (one of them could spend gold to surrender, and the other could accept this
            // gold). Please note that heroes can also surrender to castle captains.
            if ( commander1 ) {
                commander1->SetSpellPoints( initialSpellPoints1 );

                restoreFundsOfCommandersKingdom( commander1, initialFunds1 );
            }
            if ( commander2 ) {
                commander2->SetSpellPoints( initialSpellPoints2 );

                restoreFundsOfCommandersKingdom( commander2, initialFunds2 );
            }

            continue;
        }

        if ( loserHero != nullptr && loserAbandoned ) {
            // If a hero lost the battle and didn't flee or surrender, they lose all artifacts
            clearArtifacts( loserHero->GetBagArtifacts() );

            // If the other army also had a hero, some artifacts may be captured by them
            if ( winnerHero != nullptr ) {
                BagArtifacts & bag = winnerHero->GetBagArtifacts();

                transferArtifacts( bag, artifactsToTransfer );

                const auto assembledArtifacts = bag.assembleArtifactSetIfPossible();

                if ( winnerHero->isControlHuman() ) {
                    std::for_each( assembledArtifacts.begin(), assembledArtifacts.end(), Dialog::ArtifactSetAssembled );
                }
            }

            if ( loserHero->isControlAI() ) {
                const Heroes * loserAdventureHero = dynamic_cast<const Heroes *>( loserHero );
                if ( loserAdventureHero != nullptr && conf.isCampaignGameType() ) {
                    Campaign::CampaignSaveData::Get().setEnemyDefeatedAward( loserAdventureHero->GetID() );
                }
            }
        }

        arena.GetForce1().SyncArmyCount();
        arena.GetForce2().SyncArmyCount();

        if ( commander1 ) {
            commander1->ActionAfterBattle();
        }
        if ( commander2 ) {
            commander2->ActionAfterBattle();
        }

        if ( winnerHero && loserHero && winnerHero->GetLevelSkill( Skill::Secondary::EAGLE_EYE ) && loserHero->isHeroes() ) {
            eagleEyeSkillAction( *winnerHero, arena.GetUsedSpells(), winnerHero->isControlHuman(), randomGenerator );
        }

        if ( winnerHero && winnerHero->GetLevelSkill( Skill::Secondary::NECROMANCY ) ) {
            necromancySkillAction( *winnerHero, result.killed, winnerHero->isControlHuman() );
        }

        break;
    }

    DEBUG_LOG( DBG_BATTLE, DBG_INFO, "army1 " << army1.String() )
    DEBUG_LOG( DBG_BATTLE, DBG_INFO, "army2 " << army2.String() )

    army1.resetInvalidMonsters();
    army2.resetInvalidMonsters();

    // Reset the hero's army to the minimum army if the hero retreated or was defeated
    if ( commander1 && commander1->isHeroes() && ( !army1.isValid() || ( result.army1 & RESULT_RETREAT ) ) ) {
        army1.Reset( false );
    }
    if ( commander2 && commander2->isHeroes() && ( !army2.isValid() || ( result.army2 & RESULT_RETREAT ) ) ) {
        army2.Reset( false );
    }

    DEBUG_LOG( DBG_BATTLE, DBG_INFO, "army1: " << ( result.army1 & RESULT_WINS ? "wins" : "loss" ) << ", army2: " << ( result.army2 & RESULT_WINS ? "wins" : "loss" ) )

    return result;
}

uint32_t Battle::Result::AttackerResult() const
{
    return getBattleResult( army1 );
}

uint32_t Battle::Result::DefenderResult() const
{
    return getBattleResult( army2 );
}

uint32_t Battle::Result::GetExperienceAttacker() const
{
    return exp1;
}

uint32_t Battle::Result::GetExperienceDefender() const
{
    return exp2;
}

bool Battle::Result::AttackerWins() const
{
    return ( army1 & RESULT_WINS ) != 0;
}

bool Battle::Result::DefenderWins() const
{
    return ( army2 & RESULT_WINS ) != 0;
}
