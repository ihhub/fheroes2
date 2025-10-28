/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2025                                             *
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
    bool isArtifactSuitableForTransfer( const Artifact & art )
    {
        return art.isValid() && art.GetID() != Artifact::MAGIC_BOOK;
    }

    std::vector<Artifact *> planArtifactTransfer( const BagArtifacts & winnerBag, BagArtifacts & loserBag )
    {
        size_t availableArtifactSlots = std::count_if( winnerBag.begin(), winnerBag.end(), []( const Artifact & art ) { return !art.isValid(); } );

        std::vector<Artifact *> artifacts;
        artifacts.reserve( availableArtifactSlots );

        std::vector<Artifact *> ultimateArtifacts;
        ultimateArtifacts.reserve( availableArtifactSlots );

        for ( Artifact & art : loserBag ) {
            if ( availableArtifactSlots == 0 ) {
                break;
            }

            if ( !isArtifactSuitableForTransfer( art ) ) {
                continue;
            }

            if ( art.isUltimate() ) {
                ultimateArtifacts.push_back( &art );
            }
            else {
                artifacts.push_back( &art );
            }

            --availableArtifactSlots;
        }

        // Put all the ultimate artifacts at the end.
        artifacts.insert( artifacts.end(), ultimateArtifacts.begin(), ultimateArtifacts.end() );

        return artifacts;
    }

    std::vector<Artifact> getArtifactsToTransfer( const BagArtifacts & winnerBag, BagArtifacts & loserBag )
    {
        std::vector<Artifact> artifacts;
        artifacts.reserve( BagArtifacts::maxCapacity );

        for ( const Artifact * art : planArtifactTransfer( winnerBag, loserBag ) ) {
            assert( art != nullptr && isArtifactSuitableForTransfer( *art ) );

            artifacts.push_back( *art );
        }

        return artifacts;
    }

    void transferArtifacts( BagArtifacts & winnerBag, BagArtifacts & loserBag )
    {
        for ( Artifact * art : planArtifactTransfer( winnerBag, loserBag ) ) {
            assert( art != nullptr && isArtifactSuitableForTransfer( *art ) );

            // Ultimate artifacts never go to the winner.
            if ( !art->isUltimate() ) {
                if ( !winnerBag.PushArtifact( *art ) ) {
                    assert( 0 );
                }
            }

            *art = Artifact::UNKNOWN;
        }
    }

    uint32_t computeBattleSeed( const int32_t mapIndex, const uint32_t mapSeed, const Army & attackingArmy, const Army & defendingArmy )
    {
        uint32_t seed = static_cast<uint32_t>( mapIndex ) + mapSeed;

        for ( size_t i = 0; i < attackingArmy.Size(); ++i ) {
            const Troop * troop = attackingArmy.GetTroop( i );
            if ( troop->isValid() ) {
                Rand::combineSeedWithValueHash( seed, troop->GetID() );
                Rand::combineSeedWithValueHash( seed, troop->GetCount() );
            }
            else {
                Rand::combineSeedWithValueHash( seed, 0 );
            }
        }

        for ( size_t i = 0; i < defendingArmy.Size(); ++i ) {
            const Troop * troop = defendingArmy.GetTroop( i );
            if ( troop->isValid() ) {
                Rand::combineSeedWithValueHash( seed, troop->GetID() );
                Rand::combineSeedWithValueHash( seed, troop->GetCount() );
            }
            else {
                Rand::combineSeedWithValueHash( seed, 0 );
            }
        }

        return seed;
    }

    uint32_t getBattleResult( const uint32_t army )
    {
        if ( army & Battle::RESULT_SURRENDER ) {
            return Battle::RESULT_SURRENDER;
        }
        if ( army & Battle::RESULT_RETREAT ) {
            return Battle::RESULT_RETREAT;
        }
        if ( army & Battle::RESULT_LOSS ) {
            return Battle::RESULT_LOSS;
        }
        if ( army & Battle::RESULT_WINS ) {
            return Battle::RESULT_WINS;
        }

        return 0;
    }

    void eagleEyeSkillAction( HeroBase & hero, const SpellStorage & spells, bool local, Rand::PCG32 & randomGenerator )
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
                if ( 3 > sp.Level() && eagleeye.GetValue() >= Rand::GetWithGen( 1, 100, randomGenerator ) )
                    new_spells.push_back( sp );
                break;
            case Skill::Level::ADVANCED:
                // 30%
                if ( 4 > sp.Level() && eagleeye.GetValue() >= Rand::GetWithGen( 1, 100, randomGenerator ) )
                    new_spells.push_back( sp );
                break;
            case Skill::Level::EXPERT:
                // 40%
                if ( 5 > sp.Level() && eagleeye.GetValue() >= Rand::GetWithGen( 1, 100, randomGenerator ) )
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

Battle::Result Battle::Loader( Army & attackingArmy, Army & defendingArmy, const int32_t tileIndex )
{
    Result result;

    // Validate the arguments - check if battle should even load
    if ( !attackingArmy.isValid() || !defendingArmy.isValid() ) {
        // Check second army first so attacker would win by default
        if ( !defendingArmy.isValid() ) {
            result.attacker = RESULT_WINS;
            DEBUG_LOG( DBG_BATTLE, DBG_WARN, "Invalid battle detected! Index " << tileIndex << ", Army: " << defendingArmy.String() )
        }
        else {
            result.defender = RESULT_WINS;
            DEBUG_LOG( DBG_BATTLE, DBG_WARN, "Invalid battle detected! Index " << tileIndex << ", Army: " << attackingArmy.String() )
        }
        return result;
    }

    HeroBase * attackingArmyCommander = attackingArmy.GetCommander();
    if ( attackingArmyCommander ) {
        attackingArmyCommander->ActionPreBattle();

        if ( attackingArmy.isControlAI() ) {
            AI::Planner::HeroesPreBattle( *attackingArmyCommander, true );
        }
    }

    const uint32_t attackingArmyCommanderInitialSpellPoints = [attackingArmyCommander]() -> uint32_t {
        if ( attackingArmyCommander == nullptr ) {
            return 0;
        }

        return attackingArmyCommander->GetSpellPoints();
    }();

    const Funds attackingKingdomInitialFunds = [attackingArmyCommander]() -> Funds {
        const Kingdom * kingdom = getKingdomOfCommander( attackingArmyCommander );
        if ( kingdom == nullptr ) {
            return {};
        }

        return kingdom->GetFunds();
    }();

    HeroBase * defendingArmyCommander = defendingArmy.GetCommander();
    if ( defendingArmyCommander ) {
        defendingArmyCommander->ActionPreBattle();

        if ( defendingArmy.isControlAI() ) {
            AI::Planner::HeroesPreBattle( *defendingArmyCommander, false );
        }
    }

    const uint32_t defendingArmyCommanderInitialSpellPoints = [defendingArmyCommander]() -> uint32_t {
        if ( defendingArmyCommander == nullptr ) {
            return 0;
        }

        return defendingArmyCommander->GetSpellPoints();
    }();

    const Funds defendingKingdomInitialFunds = [defendingArmyCommander]() -> Funds {
        const Kingdom * kingdom = getKingdomOfCommander( defendingArmyCommander );
        if ( kingdom == nullptr ) {
            return {};
        }

        return kingdom->GetFunds();
    }();

    const bool isHumanBattle = attackingArmy.isControlHuman() || defendingArmy.isControlHuman();

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
            const Player * attackingPlayer = Players::Get( attackingArmy.GetColor() );
            const Player * defendingPlayer = Players::Get( defendingArmy.GetColor() );

            if ( ( attackingPlayer != nullptr && attackingPlayer->isAIAutoControlMode() ) || ( defendingPlayer != nullptr && defendingPlayer->isAIAutoControlMode() ) ) {
                showBattle = true;
            }
        }
    }
#endif

    const uint32_t battleSeed = computeBattleSeed( tileIndex, world.GetMapSeed(), attackingArmy, defendingArmy );

    while ( true ) {
        Rand::PCG32 randomGenerator( battleSeed );
        Arena arena( attackingArmy, defendingArmy, tileIndex, showBattle, randomGenerator );

        DEBUG_LOG( DBG_BATTLE, DBG_INFO, "attacking army: " << attackingArmy.String() )
        DEBUG_LOG( DBG_BATTLE, DBG_INFO, "defending army: " << defendingArmy.String() )

        while ( arena.BattleValid() ) {
            arena.Turns();
        }
        result = arena.GetResult();

        HeroBase * const winnerHero = ( result.attacker & RESULT_WINS ? attackingArmyCommander : ( result.defender & RESULT_WINS ? defendingArmyCommander : nullptr ) );
        HeroBase * const loserHero = ( result.attacker & RESULT_LOSS ? attackingArmyCommander : ( result.defender & RESULT_LOSS ? defendingArmyCommander : nullptr ) );

        const bool isLoserHeroAbandoned = !( ( result.attacker & RESULT_LOSS ? result.attacker : result.defender ) & ( RESULT_RETREAT | RESULT_SURRENDER ) );
        const bool shouldTransferArtifacts = ( winnerHero != nullptr && loserHero != nullptr && winnerHero->isHeroes() && loserHero->isHeroes() && isLoserHeroAbandoned );

        if ( showBattle ) {
            const bool clearMessageLog = ( result.attacker & ( RESULT_RETREAT | RESULT_SURRENDER ) ) || ( result.defender & ( RESULT_RETREAT | RESULT_SURRENDER ) );
            arena.FadeArena( clearMessageLog );
        }

        if ( isHumanBattle
             && arena.DialogBattleSummary( result,
                                           shouldTransferArtifacts ? getArtifactsToTransfer( winnerHero->GetBagArtifacts(), loserHero->GetBagArtifacts() )
                                                                   : std::vector<Artifact>{},
                                           !showBattle ) ) {
            // If dialog returns true we will restart battle in manual mode
            showBattle = true;

            // Reset the state of army commanders and the state of their kingdoms' finances (one of them could spend gold to surrender, and the other could accept this
            // gold). Please note that heroes can also surrender to castle captains.
            if ( attackingArmyCommander ) {
                attackingArmyCommander->SetSpellPoints( attackingArmyCommanderInitialSpellPoints );

                restoreFundsOfCommandersKingdom( attackingArmyCommander, attackingKingdomInitialFunds );
            }
            if ( defendingArmyCommander ) {
                defendingArmyCommander->SetSpellPoints( defendingArmyCommanderInitialSpellPoints );

                restoreFundsOfCommandersKingdom( defendingArmyCommander, defendingKingdomInitialFunds );
            }

            continue;
        }

        if ( loserHero != nullptr && isLoserHeroAbandoned ) {
            // If the losing hero did not escape or surrender, and the winning army also has a hero, then the winning hero can capture some artifacts
            if ( winnerHero != nullptr && shouldTransferArtifacts ) {
                BagArtifacts & winnerBag = winnerHero->GetBagArtifacts();

                transferArtifacts( winnerBag, loserHero->GetBagArtifacts() );

                const auto assembledArtifacts = winnerBag.assembleArtifactSetIfPossible();

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

        arena.getAttackingForce().syncOriginalArmy();
        arena.getDefendingForce().syncOriginalArmy();

        if ( attackingArmyCommander ) {
            attackingArmyCommander->ActionAfterBattle();
        }
        if ( defendingArmyCommander ) {
            defendingArmyCommander->ActionAfterBattle();
        }

        if ( winnerHero && loserHero && winnerHero->GetLevelSkill( Skill::Secondary::EAGLE_EYE ) && loserHero->isHeroes() ) {
            eagleEyeSkillAction( *winnerHero, arena.GetUsedSpells(), winnerHero->isControlHuman(), randomGenerator );
        }

        if ( winnerHero && winnerHero->GetLevelSkill( Skill::Secondary::NECROMANCY ) ) {
            necromancySkillAction( *winnerHero, result.numOfDeadUnitsForNecromancy, winnerHero->isControlHuman() );
        }

        break;
    }

    DEBUG_LOG( DBG_BATTLE, DBG_INFO, "attacking army: " << attackingArmy.String() )
    DEBUG_LOG( DBG_BATTLE, DBG_INFO, "defending army: " << defendingArmy.String() )

    attackingArmy.resetInvalidMonsters();
    defendingArmy.resetInvalidMonsters();

    DEBUG_LOG( DBG_BATTLE, DBG_INFO,
               "attacker: " << ( result.attacker & RESULT_WINS ? "wins" : "loss" ) << ", defender: " << ( result.defender & RESULT_WINS ? "wins" : "loss" ) )

    return result;
}

uint32_t Battle::Result::getAttackerResult() const
{
    return getBattleResult( attacker );
}

uint32_t Battle::Result::getDefenderResult() const
{
    return getBattleResult( defender );
}

bool Battle::Result::isAttackerWin() const
{
    return ( attacker & RESULT_WINS ) != 0;
}

bool Battle::Result::isDefenderWin() const
{
    return ( defender & RESULT_WINS ) != 0;
}
