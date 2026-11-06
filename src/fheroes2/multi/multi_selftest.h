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

#include <optional>

namespace Multi
{
    // Runs the self-test asked by the command line and returns the exit code, or an empty value if there is none.
    // Results go to the log file.
    //
    //   --multi-selftest-save <save file>
    //       Save, reload, same fingerprint.
    //   --multi-selftest-save-parts <save or map file> [more files]
    //       Splits the world into parts and writes it back. Same bytes, broken data refused.
    //   --multi-selftest-contact <save or map file> [more files]
    //       Checks the contact computation. Game unchanged, stable, fast, wider than the moves of every hero.
    //   --multi-selftest-contact-cases <map file>
    //       Places heroes in known situations and checks the contact verdict.
    //   --multi-selftest-protocol
    //       Message encoding and handshake checks.
    //   --multi-selftest-lobby-protocol <server/lobby/test/vectors.txt>
    //       Lobby messages written by the lobby server are read and written back the same, wrong ones are refused.
    //   --multi-selftest-websocket
    //       WebSocket client without network: SHA-1, lobby addresses, frames, wrong answers.
    //   --multi-selftest-lobby-client <address of a lobby server on this computer>
    //       Publishes a room, finds it in the list, closes it, then tries wrong addresses.
    //   --multi-selftest-port-mapping <box | description address of a fake box> <open | failed>
    //       Opens a port on the internet box, then closes it.
    //   --multi-selftest-host <port>
    //   --multi-selftest-join <address> <port>
    //       Handshake and a few MB of data between two instances.
    //   --multi-selftest-game-host <hot seat save file | map file> <port> <days> [human players, 2 by default]
    //   --multi-selftest-game-join <address> <port>
    //       Network game played by the AI for a few days. A map file starts a new game.
    //   --multi-selftest-battle-host <hot seat save file> <port>
    //   --multi-selftest-battle-join <address> <port>
    //       Battle between a hero of each computer, then the defender levels up on the guest.
    //   --multi-selftest-gameover-host <hot seat save file> <port>
    //   --multi-selftest-gameover-join <address> <port>
    //       The guest players are vanquished. The guest leaves, the host goes on alone.
    //   --multi-selftest-dialogs <hot seat save file>
    //       Network dialogs one by one for screenshots (each waits for a click).
    //
    // Options at the end of network self-tests:
    //   --multi-watch-actions   logs what each move of a human player changes
    //   --multi-reserve         each step of a human hero reserves its area with the host (classic turns)
    //   --multi-simultaneous    (host) the human players of both computers play their day at the same time
    //   --multi-test-jumps      the first hero of each human jumps like Dimension Door at the start of its turn
    //   --multi-test-toggle     (guest) simultaneous turns off on day 3, back on on day 6
    //   --multi-test-attacks    heroes jump next to a hero of the other computer and attack it, both at once every third day
    //   --multi-test-spectate   watches the battles of an ally on the other computer, every other one left and joined again
    //   --multi-test-lobby <address>   the host publishes its game on this lobby, the guest finds it in the list and joins it
    //   --multi-test-relay      (guest, with --multi-test-lobby) joins through the relay of the lobby, never directly
    //   --multi-test-lobby-game <name>   name of the game on the lobby, "Self-test" by default
    std::optional<int> runSelfTestIfRequested( const int argc, const char * const * argv );
}
