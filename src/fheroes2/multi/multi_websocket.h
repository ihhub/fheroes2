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

#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <random>
#include <string>
#include <vector>

// Client side of WebSocket connections (RFC 6455), used to reach a lobby server. Nothing here touches the network: the
// connection gives the bytes it receives and sends the bytes it is given.
namespace Multi
{
    std::array<uint8_t, 20> computeSha1( const std::string & text );

    std::string encodeBase64( const uint8_t * data, const size_t size );

    // The key a server must answer to the given request key.
    std::string computeWebSocketAccept( const std::string & key );

    // ws://host[:port][/path], wss://... or host[:port][/path]. The default path is /lobby, the default port 80 with ws://,
    // 443 with wss:// and the port of lobbies otherwise.
    struct LobbyAddress
    {
        // Without brackets for an IPv6 address.
        std::string host;
        uint16_t port{ 0 };
        std::string path;
        // Not supported yet.
        bool isSecure{ false };
    };

    constexpr uint16_t defaultLobbyPort{ 47220 };

    // Returns nothing for a wrong address.
    std::optional<LobbyAddress> parseLobbyAddress( const std::string & text );

    class WebSocketClient
    {
    public:
        WebSocketClient( std::string host, const uint16_t port, std::string path, const size_t maxMessageSize );

        // The upgrade request, to send first.
        std::vector<uint8_t> makeRequest();

        // Adds received bytes. Returns false if the connection must be closed, error() tells why.
        bool receive( const uint8_t * data, const size_t size );

        // The server accepted the upgrade.
        bool isOpen() const
        {
            return _isOpen;
        }

        // A whole binary message received, as it was sent.
        std::optional<std::vector<uint8_t>> takeMessage();

        // Bytes to send without waiting: answers to pings, answer to a close.
        std::vector<uint8_t> takeReplies();

        // A binary message as one frame, masked as a client must.
        std::vector<uint8_t> makeMessage( const uint8_t * data, const size_t size );
        std::vector<uint8_t> makePing();
        std::vector<uint8_t> makeClose( const uint16_t code );

        // Returns the number of pongs received since the last call.
        uint32_t takePongs();

        // The server closed the connection, with this code.
        bool isClosedByServer() const
        {
            return _isClosedByServer;
        }

        uint16_t closeCode() const
        {
            return _closeCode;
        }

        const std::string & error() const
        {
            return _error;
        }

    private:
        bool _fail( std::string error );
        bool _readResponse();
        bool _readFrames();
        std::vector<uint8_t> _makeFrame( const uint8_t opcode, const uint8_t * data, const size_t size );

        std::string _host;
        uint16_t _port{ 0 };
        std::string _path;
        size_t _maxMessageSize{ 0 };
        std::string _key;
        std::mt19937 _random;

        std::vector<uint8_t> _buffer;
        size_t _bufferStart{ 0 };
        bool _isOpen{ false };
        bool _isClosedByServer{ false };
        bool _isCloseSent{ false };
        uint16_t _closeCode{ 0 };

        std::vector<uint8_t> _fragments;
        bool _isFragmented{ false };
        std::deque<std::vector<uint8_t>> _messages;
        std::vector<uint8_t> _replies;
        uint32_t _pongs{ 0 };

        std::string _error;
    };
}
