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

#include "multi_game_over.h"

#include "dialog.h"
#include "game.h"
#include "game_over.h"
#include "kingdom.h"
#include "logging.h"
#include "maps_fileinfo.h"
#include "multi_game.h"
#include "multi_machines.h"
#include "multi_network.h"
#include "players.h"
#include "settings.h"
#include "translations.h"
#include "ui_dialog.h"
#include "world.h"

namespace
{
    Multi::GameOverStats & gameOverStats()
    {
        static Multi::GameOverStats stats;
        return stats;
    }

    PlayerColorsSet getHumanColors()
    {
        return static_cast<PlayerColorsSet>( Multi::getLocalHumanColors() | Multi::getRemoteHumanColors() );
    }

    // With an alliance the whole side wins
    bool isWinnerAmong( const PlayerColorsSet colors, const PlayerColor winner, const uint32_t result )
    {
        PlayerColorsSet winners = static_cast<PlayerColorsSet>( winner );
        if ( result == GameOver::WINS_SIDE ) {
            winners |= Players::GetPlayerFriends( winner );
        }

        return ( colors & winners ) != 0;
    }

    // The dialogs expect a real player and a condition of this map, a received resutl may have neither
    bool isValidResult( const PlayerColor color, const uint32_t result )
    {
        if ( Color::Count( static_cast<PlayerColorsSet>( color ) ) != 1 || Players::Get( color ) == nullptr ) {
            return false;
        }

        if ( ( result & ( GameOver::WINS | GameOver::LOSS ) ) != result || ( result & ( result - 1 ) ) != 0 ) {
            return false;
        }

        const Maps::FileInfo & mapInfo = Settings::Get().getCurrentMapInfo();

        switch ( result ) {
        case GameOver::WINS_TOWN:
        case GameOver::LOSS_ENEMY_WINS_TOWN:
            return world.getCastleEntrance( mapInfo.WinsMapsPositionObject() ) != nullptr;
        case GameOver::LOSS_TOWN:
            return world.getCastleEntrance( mapInfo.LossMapsPositionObject() ) != nullptr;
        case GameOver::WINS_HERO:
            return world.GetHeroesCondWins() != nullptr;
        case GameOver::LOSS_HERO:
            return world.GetHeroesCondLoss() != nullptr;
        default:
            break;
        }

        return true;
    }

    // To every other computer, each one told if it has no player left.
    void sendResult( const PlayerColor color, const uint32_t result, const bool isGameOver )
    {
        Multi::NetworkSession * session = Multi::getSession();
        if ( session == nullptr || session->state() == Multi::NetworkSession::State::CLOSED ) {
            return;
        }

        Multi::GameResult gameResult;
        gameResult.color = static_cast<uint8_t>( color );
        gameResult.result = result;
        gameResult.isGameOver = isGameOver;

        for ( const Multi::MachineInfo & info : Multi::getMachineTable().machines ) {
            if ( info.id == Multi::getThisMachine() || info.state == Multi::MachineState::GONE ) {
                continue;
            }

            gameResult.isOverForReceiver = isGameOver || !Multi::hasPlayerInGame( info.playedColors );
            session->send( info.id, Multi::makeGameResult( gameResult ) );
        }

        ++gameOverStats().resultsSent;

        VERBOSE_LOG( "[MULTI] Result of " << Color::String( color ) << " sent: " << GameOver::GetString( result ) << ( isGameOver ? ", the game is over" : "" ) )

        if ( isGameOver ) {
            gameOverStats().isGameOver = true;
        }
    }

    // Ends the network game on this computer
    fheroes2::GameMode leaveGame( const fheroes2::GameMode mode )
    {
        gameOverStats().isGameOver = true;
        Multi::endGame();

        return mode;
    }
}

bool Multi::isResultShownHere( const PlayerColor color )
{
    if ( isTestMode() ) {
        return false;
    }

    return !isNetworkGame() || ( getSeatedColors() & color ) != 0;
}

bool Multi::isGameEndShown()
{
    return !isTestMode();
}

bool Multi::hasPlayerInGame( const PlayerColorsSet colors )
{
    for ( const PlayerColor color : PlayerColorsVector( colors ) ) {
        if ( world.GetKingdom( color ).isPlay() ) {
            return true;
        }
    }

    return false;
}

void Multi::onPlayerVanquished( const PlayerColor color )
{
    // The current player is handled by onPlayerLoss()
    if ( !isNetworkGame() || color == Settings::Get().CurrentColor() ) {
        return;
    }

    // Game over for everyone if no human player is left on any computer
    sendResult( color, GameOver::LOSS_ALL, !hasPlayerInGame( getHumanColors() ) );
}

void Multi::onPlayerLoss( const PlayerColor color, const uint32_t result, const bool isGameOver )
{
    // The end of the game may already have been sent with a vanquished player.
    const bool isGameOverAlreadySent = gameOverStats().isGameOver && isNetworkGame();

    if ( isGameOver ) {
        gameOverStats().isGameOver = true;
    }

    if ( !isNetworkGame() ) {
        return;
    }

    const bool isPlayerHere = ( getSeatedColors() & color ) != 0;

    if ( !isGameOverAlreadySent ) {
        sendResult( color, result, isGameOver );
    }

    if ( isGameOver && !isPlayerHere && !isTestMode() ) {
        fheroes2::showStandardTextMessage( _( "Network game" ), _( "The game is over." ), Dialog::OK );
    }
}

std::optional<fheroes2::GameMode> Multi::onPlayerVictory( const PlayerColor color, const uint32_t result )
{
    gameOverStats().isGameOver = true;

    const bool isWinnerHere = !isNetworkGame() || isWinnerAmong( getSeatedColors(), color, result );

    if ( isNetworkGame() ) {
        sendResult( color, result, true );
    }

    if ( isWinnerHere ) {
        // Self-tests skip the victory dialogs
        if ( isTestMode() ) {
            return fheroes2::GameMode::HIGHSCORES_STANDARD;
        }

        return {};
    }

    if ( !isTestMode() ) {
        Game::DialogPlayers( color, _( "Major Event!" ), _( "%{color} player has won the game." ) );
    }

    // No high score for the players of this computer
    GameOver::Result::Get().ResetResult();

    return leaveGame( fheroes2::GameMode::MAIN_MENU );
}

fheroes2::GameMode Multi::showGameResult( const Message & message )
{
    const std::optional<GameResult> gameResult = readGameResult( message );
    if ( !gameResult || !isValidResult( static_cast<PlayerColor>( gameResult->color ), gameResult->result ) ) {
        ERROR_LOG( "[MULTI] Invalid game result received" )
        return fheroes2::GameMode::MULTI_WAIT_FOR_TURN;
    }

    ++gameOverStats().resultsReceived;

    const PlayerColor color = static_cast<PlayerColor>( gameResult->color );
    const bool isPlayerHere = ( getSeatedColors() & color ) != 0;
    const bool isShown = !isTestMode();

    VERBOSE_LOG( "[MULTI] Result of " << Color::String( color ) << " received: " << GameOver::GetString( gameResult->result )
                                      << ( gameResult->isGameOver ? ", the game is over" : "" ) )

    if ( gameResult->result & GameOver::WINS ) {
        if ( isWinnerAmong( getSeatedColors(), color, gameResult->result ) ) {
            if ( isShown ) {
                GameOver::showVictory( gameResult->result );
            }

            // For the high scores
            GameOver::Result::Get().SetResult( gameResult->result );

            return leaveGame( fheroes2::GameMode::HIGHSCORES_STANDARD );
        }

        if ( isShown ) {
            Game::DialogPlayers( color, _( "Major Event!" ), _( "%{color} player has won the game." ) );
        }

        return leaveGame( fheroes2::GameMode::MAIN_MENU );
    }

    if ( isShown ) {
        if ( isPlayerHere ) {
            GameOver::showDefeat( gameResult->result, gameResult->isGameOver );
        }
        else if ( gameResult->result == GameOver::LOSS_ALL && !gameResult->isGameOver ) {
            Game::DialogPlayers( color, _( "Major Event!" ), _( "%{color} player has been vanquished!" ) );
        }
    }

    if ( gameResult->isGameOver ) {
        if ( !isPlayerHere && isShown ) {
            fheroes2::showStandardTextMessage( _( "Network game" ), _( "The game is over." ), Dialog::OK );
        }

        return leaveGame( fheroes2::GameMode::MAIN_MENU );
    }

    if ( gameResult->isOverForReceiver ) {
        // The host is needed by the other players.
        if ( !isGuest() ) {
            keepHostingWithoutPlayer();
            return fheroes2::GameMode::MULTI_WAIT_FOR_TURN;
        }

        return leaveWithoutPlayer();
    }

    return fheroes2::GameMode::MULTI_WAIT_FOR_TURN;
}

void Multi::keepHostingWithoutPlayer()
{
    if ( !isTestMode() ) {
        fheroes2::showStandardTextMessage( _( "Network game" ), _( "You have no player left in this game. Your computer keeps hosting it for the other players." ),
                                           Dialog::OK );
    }

    VERBOSE_LOG( "[MULTI] No player left here, the host keeps hosting the game" )
}

fheroes2::GameMode Multi::leaveWithoutPlayer()
{
    if ( !isTestMode() ) {
        fheroes2::showStandardTextMessage( _( "Network game" ), _( "You have no player left in this game. It goes on without you." ), Dialog::OK );
    }

    VERBOSE_LOG( "[MULTI] No player left here, leaving the game" )

    gameOverStats().isOutOfGame = true;
    endGame();

    return fheroes2::GameMode::MAIN_MENU;
}

void Multi::goOnAlone()
{
    VERBOSE_LOG( "[MULTI] No other computer has players left, the game goes on here" )

    gameOverStats().isPlayingAlone = true;
}

const Multi::GameOverStats & Multi::getGameOverStats()
{
    return gameOverStats();
}
