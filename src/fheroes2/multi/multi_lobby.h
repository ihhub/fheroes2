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

#include "multi_lobby_protocol.h"
#include "multi_protocol.h"

namespace Multi
{
    class NetworkSession;

    // Returns false to stop waiting.
    using KeepWaiting = std::function<bool()>;

    // Returns an empty value if the session is closed or the waiting was cancelled.
    std::optional<Message> waitForMessage( NetworkSession & session, const KeepWaiting & keepWaiting );

    // Host. Reads the handshake of a new computer (see NetworkSession::takeNewcomer()). Another version is refused at
    // once, with the reason in the error. The caller then answers with acceptNewcomer() or refuseNewcomer().
    std::optional<HandshakeInfo> readNewcomerHandshake( NetworkSession & session, const MachineId newcomer, const Message & firstMessage, std::string & error );

    // Host. The new computer becomes this guest.
    void acceptNewcomer( NetworkSession & session, const MachineId newcomer, const MachineId machine );

    // Host. Tells the new computer why and drops it.
    void refuseNewcomer( NetworkSession & session, const MachineId newcomer, const HandshakeResult result );

    // Host, self-tests. Waits for a computer, checks the handshake and makes it this guest. Returns an error message, or an
    // empty string on success.
    std::string acceptPlayer( NetworkSession & session, const KeepWaiting & keepWaiting, const MachineId machine );

    // Returns an error message, or an empty string on success.
    std::string connectToHost( NetworkSession & session, const std::string & address, const uint16_t port, const KeepWaiting & keepWaiting );

    // Returns an error message, or an empty string on success. The claimed colors are those this computer played before
    // the host was restarted.
    std::string checkHostHandshake( NetworkSession & session, const KeepWaiting & keepWaiting, const uint8_t claimedColors = 0 );

    // Connects to the host and checks the handshake. Returns an error message, or an empty string on success.
    std::string joinHost( NetworkSession & session, const std::string & address, const uint16_t port, const KeepWaiting & keepWaiting, const uint8_t claimedColors = 0 );

    // Tries the direct addresses given by a lobby one after another, then checks the handshake. Returns the session, or
    // nothing with the reason.
    std::unique_ptr<NetworkSession> joinRoomDirectly( const JoinPlan & plan, const KeepWaiting & keepWaiting, std::string & error );

    // The addresses given by the lobby first, unless the player always goes through the relay, then the relay. The
    // handshake is done.
    std::unique_ptr<NetworkSession> joinRoom( const JoinPlan & plan, const std::string & relayAddress, const bool isRelayOnly, const KeepWaiting & keepWaiting,
                                              std::string & error );

    // Joins through the relay of the lobby, then checks the handshake. Returns the session, or nothing with the reason.
    std::unique_ptr<NetworkSession> joinRoomThroughRelay( const JoinPlan & plan, const std::string & relayAddress, const KeepWaiting & keepWaiting, std::string & error );
}
