/***************************************************************************
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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
#include <memory>

#include "ai.h"
#include "army.h"
#include "artifact.h"
#include "battle_arena.h"
#include "battle_army.h"
#include "dialog.h"
#include "game.h"
#include "heroes_base.h"
#include "kingdom.h"
#include "logging.h"
#include "settings.h"
#include "skill.h"
#include "tools.h"
#include "translations.h"
#include "world.h"

namespace Battle
{
    void EagleEyeSkillAction( HeroBase &, const SpellStorage &, bool, const Rand::DeterministicRandomGenerator & randomGenerator );
    void NecromancySkillAction( HeroBase & hero, const uint32_t, const bool isControlHuman, const Battle::Arena & arena );
}

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

    void transferArtifacts( BagArtifacts & winnerBag, BagArtifacts & loserBag, const std::vector<Artifact> & artifacts )
    {
        // Clear loser's artifact bag.
        for ( Artifact & artifact : loserBag ) {
            if ( artifact.isValid() && artifact.GetID() != Artifact::MAGIC_BOOK ) {
                artifact = Artifact::UNKNOWN;
            }
        }

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
}

Battle::Result Battle::Loader( Army & army1, Army & army2, s32 mapsindex )
{
    // Validate the arguments - check if battle should even load
    if ( !army1.isValid() || !army2.isValid() ) {
        Result result;
        // Check second army first so attacker would win by default
        if ( !army2.isValid() ) {
            result.army1 = RESULT_WINS;
            DEBUG_LOG( DBG_BATTLE, DBG_WARN, "Invalid battle detected! Index " << mapsindex << ", Army: " << army2.String() );
        }
        else {
            result.army2 = RESULT_WINS;
            DEBUG_LOG( DBG_BATTLE, DBG_WARN, "Invalid battle detected! Index " << mapsindex << ", Army: " << army1.String() );
        }
        return result;
    }

    // pre battle army1
    HeroBase * commander1 = army1.GetCommander();
    uint32_t initialSpellPoints1 = 0;

    if ( commander1 ) {
        commander1->ActionPreBattle();

        if ( army1.isControlAI() ) {
            AI::Get().HeroesPreBattle( *commander1, true );
        }

        initialSpellPoints1 = commander1->GetSpellPoints();
    }

    // pre battle army2
    HeroBase * commander2 = army2.GetCommander();
    uint32_t initialSpellPoints2 = 0;

    if ( commander2 ) {
        commander2->ActionPreBattle();

        if ( army2.isControlAI() ) {
            AI::Get().HeroesPreBattle( *commander2, false );
        }

        initialSpellPoints2 = commander2->GetSpellPoints();
    }

    const bool isHumanBattle = army1.isControlHuman() || army2.isControlHuman();
    bool showBattle = !Settings::Get().BattleAutoResolve() && isHumanBattle;

#ifdef WITH_DEBUG
    if ( IS_DEBUG( DBG_BATTLE, DBG_TRACE ) )
        showBattle = true;
#endif

    const size_t battleDeterministicSeed = static_cast<size_t>( mapsindex ) + static_cast<size_t>( world.GetMapSeed() );
    const size_t battlePureRandomSeed = Rand::Get( std::numeric_limits<uint32_t>::max() );
    const size_t battleSeed = Settings::Get().ExtBattleDeterministicResult() ? battleDeterministicSeed : battlePureRandomSeed;
    Rand::DeterministicRandomGenerator randomGenerator( battleSeed );

    std::unique_ptr<Arena> arena( new Arena( army1, army2, mapsindex, showBattle, randomGenerator ) );

    DEBUG_LOG( DBG_BATTLE, DBG_INFO, "army1 " << army1.String() );
    DEBUG_LOG( DBG_BATTLE, DBG_INFO, "army2 " << army2.String() );

    while ( arena->BattleValid() ) {
        arena->Turns();
    }

    Result result = arena->GetResult();

    HeroBase * hero_wins = ( result.army1 & RESULT_WINS ? commander1 : ( result.army2 & RESULT_WINS ? commander2 : nullptr ) );
    HeroBase * hero_loss = ( result.army1 & RESULT_LOSS ? commander1 : ( result.army2 & RESULT_LOSS ? commander2 : nullptr ) );
    u32 loss_result = result.army1 & RESULT_LOSS ? result.army1 : result.army2;

    std::vector<Artifact> artifactsToTransfer;
    if ( ( hero_wins && hero_loss && !( ( RESULT_RETREAT | RESULT_SURRENDER ) & loss_result ) && hero_wins->isHeroes() && hero_loss->isHeroes() ) ) {
        artifactsToTransfer = planArtifactTransfer( hero_wins->GetBagArtifacts(), hero_loss->GetBagArtifacts() );
    }

    bool battleSummaryShown = false;
    // Check if it was an auto battle
    if ( isHumanBattle && !showBattle ) {
        if ( arena->DialogBattleSummary( result, artifactsToTransfer, true ) ) {
            // If dialog returns true we will restart battle in manual mode
            showBattle = true;

            // Reset army commander state
            if ( commander1 )
                commander1->SetSpellPoints( initialSpellPoints1 );
            if ( commander2 )
                commander2->SetSpellPoints( initialSpellPoints2 );

            // Have to destroy old Arena instance first
            arena.reset();

            // reset random seed
            randomGenerator.UpdateSeed( battleSeed );

            arena = std::unique_ptr<Arena>( new Arena( army1, army2, mapsindex, true, randomGenerator ) );

            while ( arena->BattleValid() ) {
                arena->Turns();
            }

            // Override the result
            result = arena->GetResult();
            hero_wins = ( result.army1 & RESULT_WINS ? commander1 : ( result.army2 & RESULT_WINS ? commander2 : nullptr ) );
            hero_loss = ( result.army1 & RESULT_LOSS ? commander1 : ( result.army2 & RESULT_LOSS ? commander2 : nullptr ) );
            loss_result = result.army1 & RESULT_LOSS ? result.army1 : result.army2;

            if ( ( hero_wins && hero_loss && !( ( RESULT_RETREAT | RESULT_SURRENDER ) & loss_result ) && hero_wins->isHeroes() && hero_loss->isHeroes() ) ) {
                artifactsToTransfer = planArtifactTransfer( hero_wins->GetBagArtifacts(), hero_loss->GetBagArtifacts() );
            }
        }
        else {
            battleSummaryShown = true;
        }
    }

    if ( showBattle ) {
        // fade arena
        const bool clearMessageLog
            = ( result.army1 & RESULT_RETREAT ) || ( result.army2 & RESULT_RETREAT ) || ( result.army1 & RESULT_SURRENDER ) || ( result.army2 & RESULT_SURRENDER );
        arena->FadeArena( clearMessageLog );
    }

    // final summary dialog
    if ( isHumanBattle && !battleSummaryShown ) {
        arena->DialogBattleSummary( result, artifactsToTransfer, false );
    }

    transferArtifacts( hero_wins->GetBagArtifacts(), hero_loss->GetBagArtifacts(), artifactsToTransfer );

    // save count troop
    arena->GetForce1().SyncArmyCount();
    arena->GetForce2().SyncArmyCount();

    // after battle army1
    if ( commander1 ) {
        if ( army1.isControlAI() )
            AI::Get().HeroesAfterBattle( *commander1, true );
        else
            commander1->ActionAfterBattle();
    }

    // after battle army2
    if ( commander2 ) {
        if ( army2.isControlAI() )
            AI::Get().HeroesAfterBattle( *commander2, false );
        else
            commander2->ActionAfterBattle();
    }

    // eagle eye capability
    if ( hero_wins && hero_loss && hero_wins->GetLevelSkill( Skill::Secondary::EAGLEEYE ) && hero_loss->isHeroes() )
        EagleEyeSkillAction( *hero_wins, arena->GetUsageSpells(), hero_wins->isControlHuman(), randomGenerator );

    // necromancy capability
    if ( hero_wins && hero_wins->GetLevelSkill( Skill::Secondary::NECROMANCY ) )
        NecromancySkillAction( *hero_wins, result.killed, hero_wins->isControlHuman(), *arena );

    if ( hero_wins ) {
        Heroes * kingdomHero = dynamic_cast<Heroes *>( hero_wins );

        if ( kingdomHero ) {
            Kingdom & kingdom = kingdomHero->GetKingdom();
            kingdom.SetLastBattleWinHero( *kingdomHero );
        }
    }

    DEBUG_LOG( DBG_BATTLE, DBG_INFO, "army1 " << army1.String() );
    DEBUG_LOG( DBG_BATTLE, DBG_INFO, "army2 " << army1.String() );

    // update army
    if ( commander1 && commander1->isHeroes() ) {
        // hard reset army
        if ( !army1.isValid() || ( result.army1 & RESULT_RETREAT ) )
            army1.Reset( false );
    }

    // update army
    if ( commander2 && commander2->isHeroes() ) {
        // hard reset army
        if ( !army2.isValid() || ( result.army2 & RESULT_RETREAT ) )
            army2.Reset( false );
    }

    DEBUG_LOG( DBG_BATTLE, DBG_INFO, "army1: " << ( result.army1 & RESULT_WINS ? "wins" : "loss" ) << ", army2: " << ( result.army2 & RESULT_WINS ? "wins" : "loss" ) );

    return result;
}

void Battle::EagleEyeSkillAction( HeroBase & hero, const SpellStorage & spells, bool local, const Rand::DeterministicRandomGenerator & randomGenerator )
{
    if ( spells.empty() || !hero.HaveSpellBook() )
        return;

    SpellStorage new_spells;
    new_spells.reserve( 10 );

    const Skill::Secondary eagleeye( Skill::Secondary::EAGLEEYE, hero.GetLevelSkill( Skill::Secondary::EAGLEEYE ) );

    // filter spells
    for ( SpellStorage::const_iterator it = spells.begin(); it != spells.end(); ++it ) {
        const Spell & sp = *it;
        if ( !hero.HaveSpell( sp ) ) {
            switch ( eagleeye.Level() ) {
            case Skill::Level::BASIC:
                // 20%
                if ( 3 > sp.Level() && eagleeye.GetValues() >= randomGenerator.Get( 1, 100 ) )
                    new_spells.push_back( sp );
                break;
            case Skill::Level::ADVANCED:
                // 30%
                if ( 4 > sp.Level() && eagleeye.GetValues() >= randomGenerator.Get( 1, 100 ) )
                    new_spells.push_back( sp );
                break;
            case Skill::Level::EXPERT:
                // 40%
                if ( 5 > sp.Level() && eagleeye.GetValues() >= randomGenerator.Get( 1, 100 ) )
                    new_spells.push_back( sp );
                break;
            default:
                break;
            }
        }
    }

    // add new spell
    for ( SpellStorage::const_iterator it = new_spells.begin(); it != new_spells.end(); ++it ) {
        const Spell & sp = *it;
        if ( local ) {
            std::string msg = _( "Through eagle-eyed observation, %{name} is able to learn the magic spell %{spell}." );
            StringReplace( msg, "%{name}", hero.GetName() );
            StringReplace( msg, "%{spell}", sp.GetName() );
            Game::PlayPickupSound();
            Dialog::SpellInfo( "", msg, sp );
        }
    }

    hero.AppendSpellsToBook( new_spells, true );
}

void Battle::NecromancySkillAction( HeroBase & hero, const uint32_t enemyTroopsKilled, const bool isControlHuman, const Battle::Arena & arena )
{
    Army & army = hero.GetArmy();

    if ( 0 == enemyTroopsKilled || ( army.isFullHouse() && !army.HasMonster( Monster::SKELETON ) ) )
        return;

    const uint32_t necromancyPercent = GetNecromancyPercent( hero );
    const uint32_t raisedMonsterType = Monster::SKELETON;

    const Monster mons( Monster::SKELETON );
    uint32_t raiseCount = Monster::GetCountFromHitPoints( raisedMonsterType, mons.GetHitPoints() * enemyTroopsKilled * necromancyPercent / 100 );
    if ( raiseCount == 0u )
        raiseCount = 1;
    army.JoinTroop( mons, raiseCount );

    if ( isControlHuman )
        arena.DialogBattleNecromancy( raiseCount, raisedMonsterType );

    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "raise: " << raiseCount << mons.GetMultiName() );
}

u32 Battle::Result::AttackerResult( void ) const
{
    if ( RESULT_SURRENDER & army1 )
        return RESULT_SURRENDER;
    else if ( RESULT_RETREAT & army1 )
        return RESULT_RETREAT;
    else if ( RESULT_LOSS & army1 )
        return RESULT_LOSS;
    else if ( RESULT_WINS & army1 )
        return RESULT_WINS;

    return 0;
}

u32 Battle::Result::DefenderResult( void ) const
{
    if ( RESULT_SURRENDER & army2 )
        return RESULT_SURRENDER;
    else if ( RESULT_RETREAT & army2 )
        return RESULT_RETREAT;
    else if ( RESULT_LOSS & army2 )
        return RESULT_LOSS;
    else if ( RESULT_WINS & army2 )
        return RESULT_WINS;

    return 0;
}

u32 Battle::Result::GetExperienceAttacker( void ) const
{
    return exp1;
}

u32 Battle::Result::GetExperienceDefender( void ) const
{
    return exp2;
}

bool Battle::Result::AttackerWins( void ) const
{
    return ( army1 & RESULT_WINS ) != 0;
}

bool Battle::Result::DefenderWins( void ) const
{
    return ( army2 & RESULT_WINS ) != 0;
}
