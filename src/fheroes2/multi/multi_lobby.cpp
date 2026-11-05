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

#include "multi_lobby.h"

#include <algorithm>
#include <chrono>
#include <utility>

#include "logging.h"
#include "multi_lobby_protocol.h"
#include "multi_network.h"
#include "multi_settings.h"
#include "translations.h"

namespace
{
    constexpr std::chrono::seconds directAttemptTimeout{ 3 };

    std::string describeSessionError( const Multi::NetworkSession & session )
    {
        if ( session.state() != Multi::NetworkSession::State::CLOSED ) {
            return gettext_noop( "cancelled" );
        }

        return session.error().empty() ? gettext_noop( "connection closed" ) : session.error();
    }
}

std::optional<Multi::Message> Multi::waitForMessage( NetworkSession & session, const KeepWaiting & keepWaiting )
{
    while ( true ) {
        session.update();

        if ( std::optional<Message> message = session.receive(); message ) {
            return message;
        }
        if ( session.state() == NetworkSession::State::CLOSED || !keepWaiting() ) {
            return {};
        }
    }
}

std::optional<Multi::HandshakeInfo> Multi::readNewcomerHandshake( NetworkSession & session, const MachineId newcomer, const Message & firstMessage, std::string & error )
{
    std::optional<HandshakeInfo> remote = readHandshake( firstMessage );

    const HandshakeInfo local = getLocalHandshakeInfo();
    const HandshakeResult result = remote ? checkHandshake( local, *remote ) : HandshakeResult::INVALID_MESSAGE;

    if ( result != HandshakeResult::ACCEPTED ) {
        refuseNewcomer( session, newcomer, result );
        error = std::string( gettext_noop( "handshake refused" ) ) + ": " + describeHandshakeResult( result, local, remote.value_or( HandshakeInfo{} ) );
        return {};
    }

    return remote;
}

void Multi::acceptNewcomer( NetworkSession & session, const MachineId newcomer, const MachineId machine )
{
    session.send( newcomer, makeHandshakeReply( HandshakeResult::ACCEPTED, getLocalHandshakeInfo() ) );
    session.admit( newcomer, machine );
}

void Multi::refuseNewcomer( NetworkSession & session, const MachineId newcomer, const HandshakeResult result )
{
    session.send( newcomer, makeHandshakeReply( result, getLocalHandshakeInfo() ) );
    session.closeLink( newcomer );
}

std::string Multi::acceptPlayer( NetworkSession & session, const KeepWaiting & keepWaiting, const MachineId machine )
{
    while ( true ) {
        session.update();

        if ( const std::optional<NetworkSession::Newcomer> newcomer = session.takeNewcomer(); newcomer ) {
            std::string error;
            if ( !readNewcomerHandshake( session, newcomer->machine, newcomer->firstMessage, error ) ) {
                return error;
            }

            acceptNewcomer( session, newcomer->machine, machine );
            return {};
        }

        if ( session.state() == NetworkSession::State::CLOSED || !keepWaiting() ) {
            return std::string( gettext_noop( "no player joined" ) ) + ": " + describeSessionError( session );
        }
    }
}

std::string Multi::connectToHost( NetworkSession & session, const std::string & address, const uint16_t port, const KeepWaiting & keepWaiting )
{
    session.join( address, port );

    while ( session.state() == NetworkSession::State::CONNECTING ) {
        session.update();

        if ( !keepWaiting() ) {
            return gettext_noop( "cancelled" );
        }
    }

    if ( session.state() != NetworkSession::State::CONNECTED ) {
        return std::string( gettext_noop( "unable to connect" ) ) + ": " + describeSessionError( session );
    }

    return {};
}

std::string Multi::joinHost( NetworkSession & session, const std::string & address, const uint16_t port, const KeepWaiting & keepWaiting, const uint8_t claimedColors )
{
    if ( std::string error = connectToHost( session, address, port, keepWaiting ); !error.empty() ) {
        return error;
    }

    return checkHostHandshake( session, keepWaiting, claimedColors );
}

std::unique_ptr<Multi::NetworkSession> Multi::joinRoomDirectly( const JoinPlan & plan, const KeepWaiting & keepWaiting, std::string & error )
{
    for ( const JoinCandidate & candidate : plan.candidates ) {
        const char * kind = candidate.kind == JoinCandidateKind::LOCAL ? "local" : candidate.kind == JoinCandidateKind::IPV6 ? "IPv6" : "IPv4";

        // A host which cannot be reached this way usually does not answer at all: a few seconds are enough.
        const auto deadline = std::chrono::steady_clock::now() + directAttemptTimeout;
        bool isCancelled = false;
        const KeepWaiting keepTrying = [&keepWaiting, &isCancelled, deadline]() {
            isCancelled = !keepWaiting();
            return !isCancelled && std::chrono::steady_clock::now() < deadline;
        };

        auto session = std::make_unique<NetworkSession>();
        const std::string connectError = connectToHost( *session, candidate.address, candidate.port, keepTrying );
        if ( isCancelled ) {
            error = gettext_noop( "cancelled" );
            return {};
        }
        if ( !connectError.empty() ) {
            VERBOSE_LOG( "[MULTI] No direct connection to the host (" << kind << "): " << connectError )
            continue;
        }

        if ( std::string handshakeError = checkHostHandshake( *session, keepWaiting ); !handshakeError.empty() ) {
            error = std::move( handshakeError );
            return {};
        }

        VERBOSE_LOG( "[MULTI] Joined the host with a direct connection (" << kind << ")" )
        return session;
    }

    error = gettext_noop( "the host cannot be reached directly" );
    return {};
}

std::unique_ptr<Multi::NetworkSession> Multi::joinRoom( const JoinPlan & plan, const std::string & relayAddress, const bool isRelayOnly, const KeepWaiting & keepWaiting,
                                                        std::string & error )
{
    if ( !isRelayOnly && !plan.candidates.empty() ) {
        if ( std::unique_ptr<NetworkSession> session = joinRoomDirectly( plan, keepWaiting, error ); session ) {
            return session;
        }

        if ( error == gettext_noop( "cancelled" ) ) {
            return {};
        }

        VERBOSE_LOG( "[MULTI] No direct connection to the host (" << error << "), trying the relay of the lobby" )
    }

    return joinRoomThroughRelay( plan, relayAddress, keepWaiting, error );
}

std::unique_ptr<Multi::NetworkSession> Multi::joinRoomThroughRelay( const JoinPlan & plan, const std::string & relayAddress, const KeepWaiting & keepWaiting,
                                                                    std::string & error )
{
    if ( std::all_of( plan.relayTicket.begin(), plan.relayTicket.end(), []( const uint8_t byte ) { return byte == 0; } ) ) {
        error = gettext_noop( "the lobby has no relay" );
        return {};
    }

    auto session = std::make_unique<NetworkSession>();
    session->joinThroughRelay( relayAddress, plan.room.roomId, plan.relayTicket );

    while ( session->state() == NetworkSession::State::CONNECTING ) {
        session->update();

        if ( !keepWaiting() ) {
            error = gettext_noop( "cancelled" );
            return {};
        }
    }

    if ( session->state() != NetworkSession::State::CONNECTED ) {
        error = describeSessionError( *session );
        return {};
    }

    if ( std::string handshakeError = checkHostHandshake( *session, keepWaiting ); !handshakeError.empty() ) {
        error = std::move( handshakeError );
        return {};
    }

    VERBOSE_LOG( "[MULTI] Joined the host through the relay of the lobby" )
    return session;
}

std::string Multi::checkHostHandshake( NetworkSession & session, const KeepWaiting & keepWaiting, const uint8_t claimedColors )
{
    HandshakeInfo local = getLocalHandshakeInfo();
    local.machineKey = getMachineKey();
    local.nickname = loadNetworkSettings().nickname;
    local.claimedColors = claimedColors;
    session.send( hostMachine, makeHandshake( local ) );

    const std::optional<Message> replyMessage = waitForMessage( session, keepWaiting );
    if ( !replyMessage ) {
        return std::string( gettext_noop( "no answer from the host" ) ) + ": " + describeSessionError( session );
    }

    const std::optional<HandshakeReply> reply = readHandshakeReply( *replyMessage );
    if ( !reply ) {
        return gettext_noop( "invalid answer from the host" );
    }
    if ( reply->result != HandshakeResult::ACCEPTED ) {
        return std::string( gettext_noop( "handshake refused" ) ) + ": " + describeHandshakeResult( reply->result, local, reply->host );
    }

    // Check the handshake on our side too.
    if ( const HandshakeResult result = checkHandshake( local, reply->host ); result != HandshakeResult::ACCEPTED ) {
        return std::string( gettext_noop( "handshake refused" ) ) + ": " + describeHandshakeResult( result, local, reply->host );
    }

    return {};
}
