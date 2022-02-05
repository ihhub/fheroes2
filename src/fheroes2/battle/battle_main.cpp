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
}

Battle::Result Battle::Loader( Army & army1, Army & army2, s32 mapsindex )
{
    Result result;

    // Validate the arguments - check if battle should even load
    if ( !army1.isValid() || !army2.isValid() ) {
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

    const uint32_t battleSeed = Settings::Get().ExtBattleDeterministicResult() ? computeBattleSeed( mapsindex, world.GetMapSeed(), army1, army2 )
                                                                               : Rand::Get( std::numeric_limits<uint32_t>::max() );

    bool isBattleOver = false;
    while ( !isBattleOver ) {
        Rand::DeterministicRandomGenerator randomGenerator( battleSeed );
        Arena arena( army1, army2, mapsindex, showBattle, randomGenerator );

        DEBUG_LOG( DBG_BATTLE, DBG_INFO, "army1 " << army1.String() );
        DEBUG_LOG( DBG_BATTLE, DBG_INFO, "army2 " << army2.String() );

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
            // fade arena
            const bool clearMessageLog = ( result.army1 & ( RESULT_RETREAT | RESULT_SURRENDER ) ) || ( result.army2 & ( RESULT_RETREAT | RESULT_SURRENDER ) );
            arena.FadeArena( clearMessageLog );
        }

        if ( isHumanBattle ) {
            if ( arena.DialogBattleSummary( result, artifactsToTransfer, !showBattle ) ) {
                // If dialog returns true we will restart battle in manual mode
                showBattle = true;

                // Reset army commander state
                if ( commander1 )
                    commander1->SetSpellPoints( initialSpellPoints1 );
                if ( commander2 )
                    commander2->SetSpellPoints( initialSpellPoints2 );
                continue;
            }
        }
        isBattleOver = true;

        if ( loserHero != nullptr && loserAbandoned ) {
            // if a hero lost the battle and didn't flee or surrender, they lose all artifacts
            clearArtifacts( loserHero->GetBagArtifacts() );

            // if the other army also had a hero, some artifacts may be captured by them
            if ( winnerHero != nullptr ) {
                transferArtifacts( winnerHero->GetBagArtifacts(), artifactsToTransfer );
            }
        }

        // save count troop
        arena.GetForce1().SyncArmyCount();
        arena.GetForce2().SyncArmyCount();

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
        if ( winnerHero && loserHero && winnerHero->GetLevelSkill( Skill::Secondary::EAGLEEYE ) && loserHero->isHeroes() )
            EagleEyeSkillAction( *winnerHero, arena.GetUsageSpells(), winnerHero->isControlHuman(), randomGenerator );

        // necromancy capability
        if ( winnerHero && winnerHero->GetLevelSkill( Skill::Secondary::NECROMANCY ) )
            NecromancySkillAction( *winnerHero, result.killed, winnerHero->isControlHuman(), arena );

        if ( winnerHero ) {
            const Heroes * kingdomHero = dynamic_cast<const Heroes *>( winnerHero );

            if ( kingdomHero ) {
                Kingdom & kingdom = kingdomHero->GetKingdom();
                kingdom.SetLastBattleWinHero( *kingdomHero );
            }
        }
    }

    DEBUG_LOG( DBG_BATTLE, DBG_INFO, "army1 " << army1.String() );
    DEBUG_LOG( DBG_BATTLE, DBG_INFO, "army2 " << army1.String() );

    // update army
    if ( commander1 && commander1->isHeroes() ) {
        army1.resetInvalidMonsters();
        // hard reset army
        if ( !army1.isValid() || ( result.army1 & RESULT_RETREAT ) )
            army1.Reset( false );
    }

    // update army
    if ( commander2 && commander2->isHeroes() ) {
        army2.resetInvalidMonsters();
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
    return getBattleResult( army1 );
}

u32 Battle::Result::DefenderResult( void ) const
{
    return getBattleResult( army2 );
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
