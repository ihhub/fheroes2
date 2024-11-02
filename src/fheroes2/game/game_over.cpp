/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "game_over.h"

#include <cassert>
#include <cstddef>
#include <utility>
#include <vector>

#include "artifact.h"
#include "audio.h"
#include "audio_manager.h"
#include "campaign_savedata.h"
#include "campaign_scenariodata.h"
#include "castle.h"
#include "color.h"
#include "dialog.h"
#include "game.h"
#include "game_video.h"
#include "game_video_type.h"
#include "heroes.h"
#include "highscores.h"
#include "kingdom.h"
#include "maps_fileinfo.h"
#include "monster.h"
#include "mus.h"
#include "players.h"
#include "serialize.h"
#include "settings.h"
#include "tools.h"
#include "translations.h"
#include "ui_dialog.h"
#include "ui_text.h"
#include "world.h"

namespace
{
    void DialogWins( uint32_t cond )
    {
        std::string body;

        const Settings & conf = Settings::Get();
        if ( conf.isCampaignGameType() ) {
            const Campaign::ScenarioVictoryCondition victoryCondition = Campaign::getCurrentScenarioVictoryCondition();
            if ( victoryCondition == Campaign::ScenarioVictoryCondition::CAPTURE_DRAGON_CITY ) {
                body = _( "Dragon city has fallen! You are now the Master of the Dragons." );
            }
        }

        if ( body.empty() ) {
            switch ( cond ) {
            case GameOver::WINS_ALL:
                break;

            case GameOver::WINS_TOWN: {
                body = _( "You captured %{name}!\nYou are victorious." );

                const Castle * town = world.getCastleEntrance( conf.getCurrentMapInfo().WinsMapsPositionObject() );
                assert( town != nullptr );

                if ( town ) {
                    StringReplace( body, "%{name}", town->GetName() );
                }

                break;
            }

            case GameOver::WINS_HERO: {
                body = _( "You have captured the enemy hero %{name}!\nYour quest is complete." );

                const Heroes * hero = world.GetHeroesCondWins();
                assert( hero != nullptr );

                if ( hero ) {
                    StringReplace( body, "%{name}", hero->GetName() );
                }

                break;
            }

            case GameOver::WINS_ARTIFACT:
                body = _( "You have found the %{name}.\nYour quest is complete." );

                if ( conf.getCurrentMapInfo().WinsFindUltimateArtifact() ) {
                    StringReplace( body, "%{name}", _( "Ultimate Artifact" ) );
                }
                else {
                    const Artifact art = conf.getCurrentMapInfo().WinsFindArtifactID();
                    StringReplace( body, "%{name}", art.GetName() );
                }

                break;

            case GameOver::WINS_SIDE:
                body = _( "The enemy is beaten.\nYour side has triumphed!" );
                break;

            case GameOver::WINS_GOLD:
                body = _( "You have built up over %{count} gold in your treasury.\nAll enemies bow before your wealth and power." );
                StringReplace( body, "%{count}", conf.getCurrentMapInfo().getWinningGoldAccumulationValue() );
                break;

            default:
                break;
            }
        }

        if ( !body.empty() ) {
            AudioManager::PlayMusic( MUS::VICTORY, Music::PlaybackMode::PLAY_ONCE );

            fheroes2::showStandardTextMessage( _( "Victory!" ), body, Dialog::OK );
        }
    }

    void DialogLoss( uint32_t cond )
    {
        const Settings & conf = Settings::Get();
        std::string body;

        switch ( cond ) {
        case GameOver::LOSS_ENEMY_WINS_TOWN: {
            body = _( "The enemy has captured %{name}!\nThey are triumphant." );

            const Castle * town = world.getCastleEntrance( conf.getCurrentMapInfo().WinsMapsPositionObject() );
            assert( town != nullptr );

            if ( town ) {
                StringReplace( body, "%{name}", town->GetName() );
            }

            break;
        }

        case GameOver::LOSS_ENEMY_WINS_GOLD:
            body = _( "The enemy has built up over %{count} gold in his treasury.\nYou must bow done in defeat before his wealth and power." );
            StringReplace( body, "%{count}", conf.getCurrentMapInfo().getWinningGoldAccumulationValue() );
            break;

        case GameOver::LOSS_ALL:
            body = _( "You have been eliminated from the game!!!" );
            break;

        case GameOver::LOSS_TOWN: {
            body = _( "The enemy has captured %{name}!\nThey are triumphant." );

            const Castle * town = world.getCastleEntrance( conf.getCurrentMapInfo().LossMapsPositionObject() );
            assert( town != nullptr );

            if ( town ) {
                StringReplace( body, "%{name}", town->GetName() );
            }

            break;
        }

        case GameOver::LOSS_HERO: {
            body = _( "You have lost the hero %{name}.\nYour quest is over." );

            const Heroes * hero = world.GetHeroesCondLoss();
            assert( hero != nullptr );

            if ( hero ) {
                StringReplace( body, "%{name}", hero->GetName() );
            }

            break;
        }

        case GameOver::LOSS_TIME:
            body = _( "You have failed to complete your quest in time.\nAll is lost." );
            break;

        default:
            break;
        }

        if ( !body.empty() ) {
            AudioManager::PlayMusic( MUS::LOSTGAME, Music::PlaybackMode::PLAY_ONCE );

            fheroes2::showStandardTextMessage( _( "Defeat!" ), body, Dialog::OK );
        }
    }

    Video::Subtitle standardGameResults()
    {
        // Get data for ratings text.
        const int32_t difficulty = static_cast<int32_t>( Game::GetRating() );
        const int32_t baseScore = static_cast<int32_t>( Game::getGameOverScoreFactor() );
        const int32_t score = difficulty * baseScore / 100;

        // Make ratings text as a subtitle for WIN.SMK.
        fheroes2::MultiFontText ratingText;

        std::string textBody = _( "Congratulations!\n\nDays: %{days}\n" );
        StringReplace( textBody, "%{days}", static_cast<int32_t>( world.CountDay() ) );
        ratingText.add( { textBody, fheroes2::FontType::normalWhite() } );

        textBody = _( "Base score: %{score}\nDifficulty: %{difficulty}\n\n" );
        StringReplace( textBody, "%{score}", baseScore );
        StringReplace( textBody, "%{difficulty}", difficulty );
        ratingText.add( { textBody, fheroes2::FontType::smallWhite() } );

        textBody = _( "Score: %{score}\n\nRating:\n%{rating}" );
        StringReplace( textBody, "%{score}", score );
        StringReplace( textBody, "%{rating}", fheroes2::HighScoreDataContainer::getMonsterByRating( score ).GetName() );
        ratingText.add( { textBody, fheroes2::FontType::normalWhite() } );

        // Show results from the 5th second until end (forever) and set maximum width to 140 to fit the black area.
        // Set subtitles top-center position (475,110) to render results over the black rectangle of burned picture in WIN.SMK video.
        return { ratingText, 5000, UINT32_MAX, { 475, 110 }, 140 };
    }
}

const char * GameOver::GetString( uint32_t cond )
{
    switch ( cond ) {
    case WINS_ALL:
        return _( "Defeat all enemy heroes and capture all enemy towns and castles." );
    case WINS_TOWN:
        return _( "Capture a specific town." );
    case WINS_HERO:
        return _( "Defeat a specific hero." );
    case WINS_ARTIFACT:
        return _( "Find a specific artifact." );
    case WINS_SIDE:
        return _( "Your side defeats the opposing side." );
    case WINS_GOLD:
        return _( "Accumulate a large amount of gold." );
    case LOSS_ALL:
        return _( "Lose all your heroes and towns." );
    case LOSS_TOWN:
        return _( "Lose a specific town." );
    case LOSS_HERO:
        return _( "Lose a specific hero." );
    case LOSS_TIME:
        return _( "Run out of time. (Fail to win by a certain point.)" );
    default:
        break;
    }

    return "None";
}

std::string GameOver::GetActualDescription( uint32_t cond )
{
    const Settings & conf = Settings::Get();
    std::string msg;

    if ( WINS_ALL == cond ) {
        msg = GetString( WINS_ALL );
    }
    else if ( cond == WINS_SIDE ) {
        const Player * currentPlayer = Settings::Get().GetPlayers().GetCurrent();
        assert( currentPlayer != nullptr );

        const int currentColor = currentPlayer->GetColor();
        const int friendColors = currentPlayer->GetFriends();

        const auto makeListOfPlayers = []( const int colors ) {
            std::pair<std::string, size_t> result{ {}, 0 };

            for ( const int col : Colors( colors ) ) {
                const Player * player = Players::Get( col );
                assert( player != nullptr );

                ++result.second;

                if ( result.second > 1 ) {
                    result.first += ", ";
                }

                result.first += player->GetName();
            }

            return result;
        };

        const auto [alliesList, alliesCount] = makeListOfPlayers( friendColors & ~currentColor );
        const auto [enemiesList, enemiesCount] = makeListOfPlayers( Game::GetKingdomColors() & ~friendColors );

        assert( enemiesCount > 0 );

        if ( alliesCount == 0 ) {
            msg = _n( "You must defeat the enemy %{enemies}.", "You must defeat the enemy alliance of %{enemies}.", enemiesCount );
            StringReplace( msg, "%{enemies}", enemiesList );
        }
        else {
            msg = _n( "The alliance consisting of %{allies} and you must defeat the enemy %{enemies}.",
                      "The alliance consisting of %{allies} and you must defeat the enemy alliance of %{enemies}.", enemiesCount );
            StringReplace( msg, "%{allies}", alliesList );
            StringReplace( msg, "%{enemies}", enemiesList );
        }
    }
    else if ( WINS_TOWN & cond ) {
        const Castle * town = world.getCastleEntrance( conf.getCurrentMapInfo().WinsMapsPositionObject() );
        assert( town != nullptr );

        if ( town ) {
            msg = town->isCastle() ? _( "Capture the castle '%{name}'." ) : _( "Capture the town '%{name}'." );
            StringReplace( msg, "%{name}", town->GetName() );
        }
    }
    else if ( WINS_HERO & cond ) {
        const Heroes * hero = world.GetHeroesCondWins();
        assert( hero != nullptr );

        if ( hero ) {
            msg = _( "Defeat the hero '%{name}'." );
            StringReplace( msg, "%{name}", hero->GetName() );
        }
    }
    else if ( WINS_ARTIFACT & cond ) {
        if ( conf.getCurrentMapInfo().WinsFindUltimateArtifact() ) {
            msg = _( "Find the ultimate artifact." );
        }
        else {
            const Artifact art = conf.getCurrentMapInfo().WinsFindArtifactID();

            msg = _( "Find the '%{name}' artifact." );
            StringReplace( msg, "%{name}", art.GetName() );
        }
    }
    else if ( WINS_GOLD & cond ) {
        msg = _( "Accumulate %{count} gold." );
        StringReplace( msg, "%{count}", conf.getCurrentMapInfo().getWinningGoldAccumulationValue() );
    }

    if ( WINS_ALL != cond && ( WINS_ALL & cond ) ) {
        msg.append( _( ", or you may win by defeating all enemy heroes and capturing all enemy towns and castles." ) );
    }

    if ( LOSS_ALL == cond ) {
        msg = GetString( LOSS_ALL );
    }
    else if ( LOSS_TOWN & cond ) {
        const Castle * town = world.getCastleEntrance( conf.getCurrentMapInfo().LossMapsPositionObject() );
        assert( town != nullptr );

        if ( town ) {
            msg = town->isCastle() ? _( "Lose the castle '%{name}'." ) : _( "Lose the town '%{name}'." );
            StringReplace( msg, "%{name}", town->GetName() );
        }
    }
    else if ( LOSS_HERO & cond ) {
        const Heroes * hero = world.GetHeroesCondLoss();
        assert( hero != nullptr );

        if ( hero ) {
            msg = _( "Lose the hero: %{name}." );
            StringReplace( msg, "%{name}", hero->GetName() );
        }
    }
    else if ( LOSS_TIME & cond ) {
        const uint32_t dayCount = conf.getCurrentMapInfo().LossCountDays() - 1;
        const uint32_t month = dayCount / ( numOfDaysPerWeek * numOfWeeksPerMonth );
        const uint32_t week = ( dayCount - month * ( numOfDaysPerWeek * numOfWeeksPerMonth ) ) / numOfDaysPerWeek;
        const uint32_t day = dayCount % numOfDaysPerWeek;

        msg = _( "Fail to win by the end of month %{month}, week %{week}, day %{day}." );
        StringReplace( msg, "%{day}", day + 1 );
        StringReplace( msg, "%{week}", week + 1 );
        StringReplace( msg, "%{month}", month + 1 );
    }

    return msg;
}

GameOver::Result & GameOver::Result::Get()
{
    static Result gresult;
    return gresult;
}

GameOver::Result::Result()
    : colors( 0 )
    , result( 0 )
{}

void GameOver::Result::Reset()
{
    colors = Game::GetKingdomColors();
    result = GameOver::COND_NONE;
}

fheroes2::GameMode GameOver::Result::checkGameOver()
{
    fheroes2::GameMode res = fheroes2::GameMode::CANCEL;

    const Settings & conf = Settings::Get();
    const int humanColors = Players::HumanColors();
    const int currentColor = conf.CurrentColor();
    const bool isSinglePlayer = ( Color::Count( humanColors ) == 1 );
    // Remembers whether the current player was considered active at the time of calling this function
    const bool isCurrentPlayerWasActive = ( currentColor & colors );

    int activeHumanColors = 0;

    for ( const int color : Colors( colors ) ) {
        if ( !world.GetKingdom( color ).isPlay() ) {
            // This notification should always be displayed for the AI players. For human players, this should only be displayed in a multiplayer game for a
            // human player who is not currently active - in all other cases, the "you have been eliminated" dialog should be displayed.
            if ( !( color & humanColors ) || ( !isSinglePlayer && color != currentColor ) ) {
                Game::DialogPlayers( color, _( "Major Event!" ), _( "%{color} player has been vanquished!" ) );
            }

            colors &= ( ~color );
        }
        else if ( color & humanColors ) {
            ++activeHumanColors;
        }
    }

    if ( isSinglePlayer ) {
        assert( activeHumanColors <= 1 );

        const Kingdom & kingdom = world.GetKingdom( humanColors );

#if defined( WITH_DEBUG )
        const Player * player = Players::Get( humanColors );
        assert( player != nullptr );

        const bool isAIAutoControlMode = player->isAIAutoControlMode();
#else
        const bool isAIAutoControlMode = false;
#endif

        if ( kingdom.isControlHuman() || isAIAutoControlMode ) {
            result = world.CheckKingdomWins( kingdom );

            if ( result != GameOver::COND_NONE ) {
                DialogWins( result );

                if ( conf.isCampaignGameType() ) {
                    res = fheroes2::GameMode::COMPLETE_CAMPAIGN_SCENARIO;
                }
                else {
                    AudioManager::ResetAudio();

                    Video::ShowVideo( "WIN.SMK", Video::VideoAction::WAIT_FOR_USER_INPUT, { standardGameResults() }, true );

                    // AudioManager::PlayMusic is run here in order to start playing before displaying the high score.
                    AudioManager::PlayMusicAsync( MUS::VICTORY, Music::PlaybackMode::REWIND_AND_PLAY_INFINITE );

                    res = fheroes2::GameMode::HIGHSCORES_STANDARD;
                }
            }
            else {
                // If the player's kingdom has been vanquished, he loses regardless of other conditions
                if ( !kingdom.isPlay() ) {
                    result = GameOver::LOSS_ALL;
                }
                else {
                    result = world.CheckKingdomLoss( kingdom );
                }

                if ( result != GameOver::COND_NONE ) {
                    // Don't show the loss dialog if player's kingdom has been vanquished due to the expired countdown of days since the loss of the last town.
                    // This case was already handled at the end of the Interface::AdventureMap::HumanTurn().
                    if ( !( result == GameOver::LOSS_ALL && kingdom.GetCastles().empty() && kingdom.GetLostTownDays() == 0 ) ) {
                        DialogLoss( result );
                    }

                    AudioManager::ResetAudio();
                    Video::ShowVideo( "LOSE.SMK", Video::VideoAction::LOOP_VIDEO );

                    res = fheroes2::GameMode::MAIN_MENU;
                }
            }
        }
    }
    else {
        result = [currentColor, activeHumanColors]() -> uint32_t {
            // If all human-controlled players have been vanquished, then the game is over
            if ( activeHumanColors == 0 ) {
                return GameOver::LOSS_ALL;
            }

            // Do not perform other checks if there is no active kingdom at the moment
            if ( !( currentColor & Color::ALL ) ) {
                return GameOver::COND_NONE;
            }

            const Kingdom & kingdom = world.GetKingdom( currentColor );

#if defined( WITH_DEBUG )
            const Player * player = Players::Get( currentColor );
            assert( player != nullptr );

            const bool isAIAutoControlMode = player->isAIAutoControlMode();
#else
            const bool isAIAutoControlMode = false;
#endif

            // Check the win/loss conditions for human-controlled players only
            if ( !kingdom.isControlHuman() && !isAIAutoControlMode ) {
                return GameOver::COND_NONE;
            }

            // If the player's kingdom has been vanquished, this particular player loses regardless of other conditions
            if ( !kingdom.isPlay() ) {
                return GameOver::LOSS_ALL;
            }

            uint32_t condition = world.CheckKingdomWins( kingdom );

            if ( condition != GameOver::COND_NONE ) {
                return condition;
            }

            condition = world.CheckKingdomLoss( kingdom );

            // LOSS_TOWN is currently not supported in multiplayer
            if ( condition == GameOver::LOSS_HERO || condition == GameOver::LOSS_TIME || ( condition & GameOver::LOSS_ENEMY_WINS ) ) {
                return condition;
            }

            return GameOver::COND_NONE;
        }();

        if ( result & GameOver::WINS ) {
            DialogWins( result );

            AudioManager::ResetAudio();
            Video::ShowVideo( "WIN.SMK", Video::VideoAction::WAIT_FOR_USER_INPUT, { standardGameResults() }, true );

            res = fheroes2::GameMode::HIGHSCORES_STANDARD;
        }
        else if ( result & GameOver::LOSS ) {
            const bool showLossDialog = [currentColor, isCurrentPlayerWasActive, this]() {
                // We shouldn't show the loss notification dialog if there is no active kingdom at the moment
                if ( !( currentColor & Color::ALL ) ) {
                    return false;
                }

                const Kingdom & kingdom = world.GetKingdom( currentColor );

#if defined( WITH_DEBUG )
                const Player * player = Players::Get( currentColor );
                assert( player != nullptr );

                const bool isAIAutoControlMode = player->isAIAutoControlMode();
#else
                const bool isAIAutoControlMode = false;
#endif

                // We shouldn't show the loss notification dialog to AI players
                if ( !kingdom.isControlHuman() && !isAIAutoControlMode ) {
                    return false;
                }

                // Don't show the loss dialog if player's kingdom has been vanquished due to the expired countdown of days since the loss of the last town.
                // This case was already handled at the end of the Interface::AdventureMap::HumanTurn().
                if ( result == GameOver::LOSS_ALL && kingdom.GetCastles().empty() && kingdom.GetLostTownDays() == 0 ) {
                    return false;
                }

                // LOSS_ALL fulfillment by itself is not a reason to end the multiplayer game, and in this case, this function can be called multiple times.
                // Show the loss notification dialog just once.
                if ( !isCurrentPlayerWasActive ) {
                    return false;
                }

                return true;
            }();

            // LOSS_ALL fulfillment by itself is not a reason to end the multiplayer game, unless all human-controlled players are vanquished
            const bool endGame = ( result != GameOver::LOSS_ALL || activeHumanColors == 0 );

            if ( showLossDialog ) {
                DialogLoss( result );
            }

            if ( endGame ) {
                AudioManager::ResetAudio();
                Video::ShowVideo( "LOSE.SMK", Video::VideoAction::LOOP_VIDEO );

                res = fheroes2::GameMode::MAIN_MENU;
            }
        }
    }

    return res;
}

OStreamBase & GameOver::operator<<( OStreamBase & stream, const Result & res )
{
    return stream << res.colors << res.result;
}

IStreamBase & GameOver::operator>>( IStreamBase & stream, Result & res )
{
    return stream >> res.colors >> res.result;
}
