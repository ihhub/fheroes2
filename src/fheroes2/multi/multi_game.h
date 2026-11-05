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

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "color.h"
#include "game_mode.h"
#include "multi_protocol.h"

namespace Multi
{
    class NetworkSession;
    class PublishedRoom;
    struct RoomGuest;

    // A network game is a Hot Seat game where some human players play on other computers. The host sends the game for
    // their turns and gets it back. Save files stay regular Hot Seat saves.

    struct NetworkGameStats
    {
        uint32_t turnsSent{ 0 };
        uint32_t turnsReceived{ 0 };
        uint32_t snapshotsReceived{ 0 };
        uint32_t fingerprintMismatches{ 0 };
        bool isEndedByHost{ false };
    };

    // In self-tests the AI plays every human player and errors are only logged.
    struct NetworkGameTestOptions
    {
        bool isEnabled{ false };

        // The host stops at the end of this day, 0 means never.
        uint32_t lastDay{ 0 };
    };

    // Loads a Hot Seat save and waits for the players of the other computers.
    fheroes2::GameMode hostGame( const std::string & filePath, const uint16_t port, const bool localOnly );

    // A game published on a lobby while the host waits for players. Texts are UTF-8.
    struct LobbyHosting
    {
        std::string lobbyAddress;
        std::string nickname;
        // Empty for the name of the map.
        std::string gameName;
        bool isPublic{ true };
        // No address is given to the lobby: the other players come through its relay.
        bool isRelayOnly{ false };
    };

    // Same as hostGame() for the game which is about to start. With a lobby, the game is published there too.
    fheroes2::GameMode hostCurrentGame( const uint16_t port, const bool localOnly, const std::optional<LobbyHosting> & lobby = {} );

    fheroes2::GameMode joinGame( const std::string & address, const uint16_t port );

    // The guests must be admitted in the session and the game loaded with the current player set. The human colors
    // of no guest are played on the host.
    void startHostGame( std::unique_ptr<NetworkSession> session, const std::vector<RoomGuest> & guests, const NetworkGameTestOptions testOptions = {} );

    // The guest then waits for the host to send a turn in waitForTurn().
    void startGuestGame( std::unique_ptr<NetworkSession> session, const NetworkGameTestOptions testOptions = {} );

    // Host only. The room of the game on a lobby stays until the end of the game.
    void keepPublishedRoom( std::unique_ptr<PublishedRoom> room );

    // Host only. The lobby must hear from the game at least every minute, also while it waits for the other computers.
    void updateLobby();

    // Tells the other computers that this one leaves and closes the connections. The host ends the game for everyone.
    void endGame();

    bool isNetworkGame();
    bool isGuest();

    // Host only. The color is played on a guest.
    bool isRemoteColor( const PlayerColor color );

    // Returns true while the map is shown read-only during the turn of another computer.
    bool isObservingOtherPlayers();

    // Returns the player shown in the hourglass of the status panel, the one playing while we wait.
    PlayerColor getPlayingColorShown( const PlayerColor currentColor );

    // With only one player on this computer the turn is announced over the map instead of hiding it.
    // Returns false if the game should do as usual.
    bool announceTurnOnMap( const PlayerColor color );

    // Returns nullptr if there is no network game.
    NetworkSession * getSession();

    bool isTestMode();

    // The human players played on this computer, and on the other ones.
    PlayerColorsSet getLocalHumanColors();
    PlayerColorsSet getRemoteHumanColors();

    // In self-tests the AI must play every human player again after loading a game.
    void applyTestAutoPlay();

    enum class HostWaitingCase
    {
        LOCAL,
        OPENING,
        OPEN,
        BEHIND_ANOTHER_NETWORK,
        FAILED,
        // Published on a lobby, then with other players and the chat.
        LOBBY,
        GUEST_IN
    };

    // Shows the waiting room of the host until Enter or Esc. Used by self-tests only.
    void showHostWaitingScreenForTest( const HostWaitingCase waitingCase );

    // Saves the current game as the content of a save file. Returns an error message, or an empty string on success.
    std::string saveGameData( std::vector<uint8_t> & data, uint64_t & fingerprint );

    // Returns an error message, or an empty string on success.
    std::string loadGameData( const std::vector<uint8_t> & data );

    const NetworkGameStats & getStats();

    // Used by the turn loop in Interface::AdventureMap::StartGame().

    // Returns true if a turn sent by another computer is loaded and the turn loop has not resumed it yet.
    bool isResumingTurn();

    // Should be called once the turn loop has found the current player.
    TurnStage finishResumingTurn();

    // Saves the game and sends it to the computer of the player (from the host), or back to the host. Returns
    // MULTI_WAIT_FOR_TURN, or MAIN_MENU on failure.
    fheroes2::GameMode handOverTurn( const PlayerColor color, const TurnStage stage );

    // On the host, at the start of a local turn, sends the game to look at to the guests.
    void onTurnStart( const PlayerColor color );

    // Host only, in the turn loop. Each guest gets its day, played at the same time.
    void startSimultaneousDay();

    // Host only. Returns true once, for the first day after the game was loaded from a save file.
    bool isFirstTurnOfLoadedGame();

    // Host only, before the turn of each player outside of simultaneous days: who plays which color may change there.
    void onTurnBoundary();

    // Host only. A guest is cut for a while, or left: the game waits for it.
    bool hasAbsentGuests();

    // Host only. The game goes on without the absent guests, the AI plays their players from their next turn.
    void abandonAbsentGuests();

    // Returns true in self-tests once the host played all the days it had to.
    bool isTestDayLimitReached();

    // Kept while the connection with that computer is cut. A guest sends to the other guests through the host.
    void sendToMachine( const MachineId machine, const Message & message );

    // To every other computer still in the game.
    void sendToOthers( const Message & message );

    // Applies the changes of the other computers during a simultaneous day. An AI player plans on the world of the start
    // of its turn, so they wait until the end of it.
    void processDayMessages( const bool isApplyingUpdates = true );

    // Called at each frame of the map during a local turn. Returns MAIN_MENU once the network game is over here, otherwise CANCEL.
    fheroes2::GameMode onAdventureMapFrame();

    // Returns MAIN_MENU once, at a moment the game can leave, if the network game is over for this computer.
    // The game goes on here if no other computer has players left.
    fheroes2::GameMode leaveIfNetworkGameOver();

    // Host only. Takes in the computers which come back, follows the ones which are cut or gone, and tells the guests.
    // Called at every frame.
    void updateGuests();

    // Host only. Sends the game to the computers which came back, as their turn, their day or a game to look at. Only
    // at a moment the game can be sent.
    void resyncReturningGuests();

    // Host only. Waits for the last changes of the guests before the end of a simultaneous day.
    void waitForOtherDaysClosed();

    // The colors a guest asks for when it joins the restarted game of the host: those it had.
    uint8_t getColorsToClaim();

    // After a restart of the host, the new session of the guest takes over once the game is back in MULTI_WAIT_FOR_TURN.
    // Until then the old session stays closed and the code still using it gives up.
    void replaceSessionAfterHostRestart( std::unique_ptr<NetworkSession> session );
    bool isSessionReplaced();

    // Updates the fog, radar, icons and focus after the world changed under the map.
    void refreshMap();

    // Shows the map read-only, with the actions of the other computers, until isWaiting() returns false.
    void observeUntil( const std::function<bool()> & isWaiting );

    // Host only. Watches the other computers play their players once the local ones are done.
    void waitForOtherDays();

    // Waits for a turn from the host (or for its own turn back on the host) and loads it. Returns START_GAME when the
    // game can go on.
    fheroes2::GameMode waitForTurn();

    // A wait for another computer: it gives up once that computer is gone, or came back after a restart.
    struct MachineWatch
    {
        MachineId machine{ hostMachine };
        uint32_t comebacks{ 0 };
    };

    MachineWatch watchMachine( const MachineId machine );
    bool isStillThere( const MachineWatch & watch );
}
