/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2026                                                    *
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

#include "multi_hero.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "ai_planner.h"
#include "artifact.h"
#include "audio_manager.h"
#include "battle_arena.h"
#include "dialog.h"
#include "game.h"
#include "heroes.h"
#include "heroes_base.h"
#include "localevent.h"
#include "logging.h"
#include "m82.h"
#include "math_base.h"
#include "multi_connection.h"
#include "multi_game.h"
#include "multi_machines.h"
#include "multi_network.h"
#include "multi_notice.h"
#include "multi_simultaneous.h"
#include "multi_spectator.h"
#include "screen.h"
#include "skill.h"
#include "spell.h"
#include "tools.h"
#include "translations.h"
#include "ui_dialog.h"
#include "ui_text.h"
#include "ui_window.h"
#include "world.h"

namespace
{
    Multi::HeroEventStats & heroEventStats()
    {
        static Multi::HeroEventStats stats;
        return stats;
    }

    Multi::NetworkSession * getConnectedSession()
    {
        Multi::NetworkSession * session = Multi::getSession();

        if ( session == nullptr || session->state() == Multi::NetworkSession::State::CLOSED ) {
            return nullptr;
        }

        return session;
    }

    bool isValidSecondarySkill( const int32_t skill, const int32_t level )
    {
        if ( skill == Skill::Secondary::UNKNOWN ) {
            return level == Skill::Level::NONE;
        }

        return skill > Skill::Secondary::UNKNOWN && skill <= Skill::Secondary::ESTATES && level >= Skill::Level::BASIC && level <= Skill::Level::EXPERT;
    }

    // Returns nullptr unless the hero is played on this computer
    Heroes * getLocalHero( const uint32_t heroId )
    {
        Heroes * hero = world.GetHeroes( static_cast<int>( heroId ) );

        if ( hero == nullptr || ( Multi::getLocalHumanColors() & hero->GetColor() ) == 0 ) {
            return nullptr;
        }

        return hero;
    }

    // To the computer of the hero's player.
    void sendNotice( const HeroBase & hero, Multi::HeroNotice notice )
    {
        Multi::NetworkSession * session = getConnectedSession();
        const std::optional<Multi::MachineId> machine = Multi::getMachineOfColor( hero.GetColor() );
        if ( session == nullptr || !machine ) {
            return;
        }

        const Heroes * adventureHero = dynamic_cast<const Heroes *>( &hero );
        if ( adventureHero == nullptr ) {
            return;
        }

        notice.heroId = static_cast<uint32_t>( adventureHero->GetID() );

        session->send( *machine, Multi::makeHeroNotice( notice ) );
        ++heroEventStats().noticesSent;
    }
}

bool Multi::isPlayedHere( const HeroBase & hero )
{
    if ( !hero.isControlHuman() ) {
        return false;
    }

    return !isNetworkGame() || ( getRemoteHumanColors() & hero.GetColor() ) == 0;
}

bool Multi::isPlayedOnOtherComputer( const HeroBase & hero )
{
    return isNetworkGame() && ( getRemoteHumanColors() & hero.GetColor() ) != 0;
}

Skill::Secondary Multi::chooseSecondarySkillOnOtherComputer( const Heroes & hero, const int primarySkill, const Skill::Secondary & first,
                                                             const Skill::Secondary & second )
{
    const bool isChoice = first.isValid() && second.isValid();
    const Skill::Secondary onlySkill = first.isValid() ? first : second;

    NetworkSession * session = getConnectedSession();
    const std::optional<MachineId> machine = getMachineOfColor( hero.GetColor() );
    if ( session == nullptr || !machine ) {
        return isChoice ? AI::Planner::pickSecondarySkill( hero, first, second ) : onlySkill;
    }

    const MachineWatch watch = watchMachine( *machine );

    HeroLevelUp levelUp;
    levelUp.heroId = static_cast<uint32_t>( hero.GetID() );
    levelUp.primarySkill = primarySkill;
    levelUp.firstSkill = first.Skill();
    levelUp.firstSkillLevel = first.Level();
    levelUp.secondSkill = second.Skill();
    levelUp.secondSkillLevel = second.Level();

    session->send( *machine, makeHeroLevelUp( levelUp ) );
    ++heroEventStats().levelUpsSent;

    // Nothing to choose, the other player is only told
    if ( !isChoice ) {
        return onlySkill;
    }

    ++heroEventStats().choicesAsked;

    VERBOSE_LOG( "[MULTI] " << hero.GetName() << " gained a level, waiting for the other player to pick a skill" )

    std::string waitText = _( "%{name} is choosing a skill for their hero..." );
    StringReplace( waitText, "%{name}", getMachineName( *machine ) );
    const MapWait wait( waitText );

    LocalEvent & le = LocalEvent::Get();

    // An answer already received is used even if the connection is closed. A new turn or a battle means no answer.
    const MachineId player = *machine;
    const auto isAwaited = [player]( const Message & message ) {
        switch ( message.type ) {
        case MessageType::HERO_LEVEL_UP_CHOICE:
            return message.sender == player;
        case MessageType::SIMULTANEOUS_TURNS:
        case MessageType::SPECTATE_START:
        case MessageType::SPECTATE_ACTIONS:
        case MessageType::SPECTATE_END:
            return true;
        case MessageType::GAME_END:
        case MessageType::TURN_STATE:
        case MessageType::GAME_SNAPSHOT:
        case MessageType::DAY_START:
        case MessageType::DAY_END:
        case MessageType::BATTLE_START:
            return message.sender == player || message.sender == hostMachine;
        default:
            return false;
        }
    };

    while ( true ) {
        le.HandleEvents();

        session->update();

        const std::optional<Message> message = session->receive( isAwaited );
        if ( !message ) {
            if ( !keepConnection() || !isStillThere( watch ) ) {
                break;
            }

            continue;
        }

        if ( message->type == MessageType::SIMULTANEOUS_TURNS ) {
            onSimultaneousTurnsMessage( *message );
            continue;
        }

        if ( message->type == MessageType::SPECTATE_START || message->type == MessageType::SPECTATE_ACTIONS || message->type == MessageType::SPECTATE_END ) {
            onSpectatedMessage( *message );
            continue;
        }

        if ( message->type != MessageType::HERO_LEVEL_UP_CHOICE ) {
            // The main loop processes it later
            if ( message->type != MessageType::GAME_END ) {
                ERROR_LOG( "[MULTI] Unexpected message " << static_cast<uint32_t>( message->type ) << " while waiting for a skill" )
            }

            session->putBack( *message );
            break;
        }

        const std::optional<int32_t> skill = readHeroLevelUpChoice( *message );
        if ( skill && *skill == first.Skill() ) {
            ++heroEventStats().choicesReceived;
            return first;
        }
        if ( skill && *skill == second.Skill() ) {
            ++heroEventStats().choicesReceived;
            return second;
        }

        ERROR_LOG( "[MULTI] The other player picked a skill that was not offered" )
        break;
    }

    ERROR_LOG( "[MULTI] No skill picked by the other player, the AI picks it" )

    return AI::Planner::pickSecondarySkill( hero, first, second );
}

void Multi::notifyRaisedSkeletons( const HeroBase & hero, const uint32_t count )
{
    if ( !isPlayedOnOtherComputer( hero ) || count == 0 ) {
        return;
    }

    HeroNotice notice;
    notice.kind = HeroNotice::Kind::RAISED_SKELETONS;
    notice.values.push_back( static_cast<int32_t>( count ) );

    sendNotice( hero, std::move( notice ) );
}

void Multi::notifyLearnedSpells( const HeroBase & hero, const std::vector<Spell> & spells )
{
    if ( !isPlayedOnOtherComputer( hero ) || spells.empty() ) {
        return;
    }

    HeroNotice notice;
    notice.kind = HeroNotice::Kind::LEARNED_SPELLS;

    for ( const Spell & spell : spells ) {
        notice.values.push_back( spell.GetID() );
    }

    sendNotice( hero, std::move( notice ) );
}

void Multi::notifyAssembledArtifacts( const HeroBase & hero, const std::set<ArtifactSetData> & artifactSets )
{
    if ( !isPlayedOnOtherComputer( hero ) ) {
        return;
    }

    for ( const ArtifactSetData & artifactSet : artifactSets ) {
        HeroNotice notice;
        notice.kind = HeroNotice::Kind::ASSEMBLED_ARTIFACT;
        notice.values.push_back( artifactSet._assembledArtifactID );
        notice.text = artifactSet._assembleMessage;

        sendNotice( hero, std::move( notice ) );
    }
}

void Multi::answerHeroLevelUp( const Message & message )
{
    const std::optional<HeroLevelUp> levelUp = readHeroLevelUp( message );

    const bool isValid = levelUp && levelUp->primarySkill >= Skill::Primary::ATTACK && levelUp->primarySkill <= Skill::Primary::KNOWLEDGE
                         && isValidSecondarySkill( levelUp->firstSkill, levelUp->firstSkillLevel )
                         && isValidSecondarySkill( levelUp->secondSkill, levelUp->secondSkillLevel );

    const Skill::Secondary first = isValid ? Skill::Secondary( levelUp->firstSkill, levelUp->firstSkillLevel ) : Skill::Secondary();
    const Skill::Secondary second = isValid ? Skill::Secondary( levelUp->secondSkill, levelUp->secondSkillLevel ) : Skill::Secondary();
    const bool isChoice = first.isValid() && second.isValid();

    Heroes * hero = isValid ? getLocalHero( levelUp->heroId ) : nullptr;

    Skill::Secondary selected;

    if ( !levelUp || hero == nullptr ) {
        ERROR_LOG( "[MULTI] Invalid level up received" )
    }
    else {
        ++heroEventStats().levelUpsReceived;

        if ( isTestMode() || hero->isControlAI() ) {
            selected = isChoice ? AI::Planner::pickSecondarySkill( *hero, first, second ) : ( first.isValid() ? first : second );
        }
        else {
            AudioManager::PlaySound( M82::NWHEROLV );
            const int result = Dialog::LevelUpSelectSkill( hero->GetName(), levelUp->primarySkill, first, second, *hero );

            if ( result != Skill::Secondary::UNKNOWN ) {
                selected = ( result == second.Skill() ) ? second : first;
            }
        }

        VERBOSE_LOG( "[MULTI] " << hero->GetName() << " gained a level, skill picked: " << Skill::Secondary::String( selected.Skill() ) )

        // The real game is on the other computer. This copy is updated only for the next level.
        hero->LearnSkill( selected );
    }

    // The other computer waits only if there is a choice
    if ( isChoice ) {
        if ( NetworkSession * session = getConnectedSession(); session != nullptr ) {
            session->send( message.sender, makeHeroLevelUpChoice( selected.Skill() ) );
        }
    }
}

void Multi::showHeroNotice( const Message & message )
{
    const std::optional<HeroNotice> notice = readHeroNotice( message );
    const Heroes * hero = notice ? getLocalHero( notice->heroId ) : nullptr;

    if ( !notice || hero == nullptr ) {
        ERROR_LOG( "[MULTI] Invalid hero notice received" )
        return;
    }

    ++heroEventStats().noticesReceived;

    VERBOSE_LOG( "[MULTI] Notice " << static_cast<int>( notice->kind ) << " received for " << hero->GetName() )

    if ( isTestMode() ) {
        return;
    }

    switch ( notice->kind ) {
    case HeroNotice::Kind::RAISED_SKELETONS:
        if ( notice->values.size() == 1 && notice->values.front() > 0 ) {
            Battle::Arena::DialogBattleNecromancy( static_cast<uint32_t>( notice->values.front() ) );
        }
        break;
    case HeroNotice::Kind::LEARNED_SPELLS:
        for ( const int32_t spellId : notice->values ) {
            const Spell spell( spellId );
            if ( !spell.isValid() ) {
                continue;
            }

            std::string text = _( "Through eagle-eyed observation, %{name} is able to learn the magic spell %{spell}." );
            StringReplace( text, "%{name}", hero->GetName() );
            StringReplace( text, "%{spell}", spell.GetName() );
            Game::PlayPickupSound();

            const fheroes2::SpellDialogElement spellUI( spell, hero );
            fheroes2::showStandardTextMessage( {}, std::move( text ), Dialog::OK, { &spellUI } );
        }
        break;
    case HeroNotice::Kind::ASSEMBLED_ARTIFACT:
        if ( notice->values.size() == 1 && Artifact( notice->values.front() ).isValid() && !notice->text.empty() ) {
            Dialog::ArtifactSetAssembled( ArtifactSetData( notice->values.front(), notice->text ) );
        }
        break;
    default:
        break;
    }
}

const Multi::HeroEventStats & Multi::getHeroEventStats()
{
    return heroEventStats();
}
