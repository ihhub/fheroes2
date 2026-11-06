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

#include "multi_websocket.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <string_view>
#include <utility>

#include "translations.h"

namespace
{
    constexpr std::string_view acceptGuid{ "258EAFA5-E914-47DA-95CA-C5AB0DC85B11" };

    // An answer of a lobby to the upgrade request is a few hundred bytes.
    constexpr size_t maxResponseSize{ 8192 };

    // Text frames (1) are refused: lobby messages are binary.
    namespace Opcode
    {
        constexpr uint8_t continuation{ 0 };
        constexpr uint8_t binary{ 2 };
        constexpr uint8_t close{ 8 };
        constexpr uint8_t ping{ 9 };
        constexpr uint8_t pong{ 10 };
    }

    uint32_t rotateLeft( const uint32_t value, const int bits )
    {
        return ( value << bits ) | ( value >> ( 32 - bits ) );
    }

    std::string toLower( std::string_view text )
    {
        std::string lower( text );
        std::transform( lower.begin(), lower.end(), lower.begin(), []( const unsigned char c ) { return static_cast<char>( std::tolower( c ) ); } );
        return lower;
    }

    std::string_view trim( std::string_view text )
    {
        while ( !text.empty() && ( text.front() == ' ' || text.front() == '\t' ) ) {
            text.remove_prefix( 1 );
        }
        while ( !text.empty() && ( text.back() == ' ' || text.back() == '\t' ) ) {
            text.remove_suffix( 1 );
        }
        return text;
    }

    bool hasToken( const std::string & header, const std::string_view token )
    {
        size_t start = 0;
        while ( start <= header.size() ) {
            const size_t end = std::min( header.find( ',', start ), header.size() );
            if ( trim( std::string_view( header ).substr( start, end - start ) ) == token ) {
                return true;
            }
            start = end + 1;
        }
        return false;
    }

    bool isHostName( const std::string_view host )
    {
        return !host.empty() && host.size() <= 253
               && std::all_of( host.begin(), host.end(), []( const char c ) { return std::isalnum( static_cast<unsigned char>( c ) ) || c == '.' || c == '-'; } );
    }

    bool isIPv6Text( const std::string_view host )
    {
        return host.size() >= 2 && host.size() <= 45 && host.find( ':' ) != std::string_view::npos
               && std::all_of( host.begin(), host.end(), []( const char c ) { return std::isxdigit( static_cast<unsigned char>( c ) ) || c == ':' || c == '.'; } );
    }

    std::optional<uint16_t> parsePortNumber( const std::string_view text )
    {
        uint32_t port = 0;
        const auto [end, error] = std::from_chars( text.data(), text.data() + text.size(), port );
        if ( error != std::errc() || end != text.data() + text.size() || port == 0 || port > 65535 ) {
            return {};
        }
        return static_cast<uint16_t>( port );
    }
}

std::array<uint8_t, 20> Multi::computeSha1( const std::string & text )
{
    std::array<uint32_t, 5> state{ 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0 };

    std::vector<uint8_t> data( text.begin(), text.end() );
    const uint64_t bitCount = static_cast<uint64_t>( data.size() ) * 8;
    data.push_back( 0x80 );
    while ( data.size() % 64 != 56 ) {
        data.push_back( 0 );
    }
    for ( int shift = 56; shift >= 0; shift -= 8 ) {
        data.push_back( static_cast<uint8_t>( bitCount >> shift ) );
    }

    std::array<uint32_t, 80> words{};

    for ( size_t chunk = 0; chunk < data.size(); chunk += 64 ) {
        for ( size_t i = 0; i < 16; ++i ) {
            const uint8_t * bytes = data.data() + chunk + i * 4;
            words[i] = ( static_cast<uint32_t>( bytes[0] ) << 24 ) | ( static_cast<uint32_t>( bytes[1] ) << 16 ) | ( static_cast<uint32_t>( bytes[2] ) << 8 ) | bytes[3];
        }
        for ( size_t i = 16; i < 80; ++i ) {
            words[i] = rotateLeft( words[i - 3] ^ words[i - 8] ^ words[i - 14] ^ words[i - 16], 1 );
        }

        uint32_t a = state[0];
        uint32_t b = state[1];
        uint32_t c = state[2];
        uint32_t d = state[3];
        uint32_t e = state[4];

        for ( size_t i = 0; i < 80; ++i ) {
            uint32_t f = 0;
            uint32_t k = 0;
            if ( i < 20 ) {
                f = ( b & c ) | ( ~b & d );
                k = 0x5A827999;
            }
            else if ( i < 40 ) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            }
            else if ( i < 60 ) {
                f = ( b & c ) | ( b & d ) | ( c & d );
                k = 0x8F1BBCDC;
            }
            else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }

            const uint32_t next = rotateLeft( a, 5 ) + f + e + k + words[i];
            e = d;
            d = c;
            c = rotateLeft( b, 30 );
            b = a;
            a = next;
        }

        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
        state[4] += e;
    }

    std::array<uint8_t, 20> digest{};
    for ( size_t i = 0; i < digest.size(); ++i ) {
        digest[i] = static_cast<uint8_t>( state[i / 4] >> ( 24 - ( i % 4 ) * 8 ) );
    }

    return digest;
}

std::string Multi::encodeBase64( const uint8_t * data, const size_t size )
{
    constexpr std::string_view characters{ "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/" };

    std::string text;
    text.reserve( ( size + 2 ) / 3 * 4 );

    for ( size_t i = 0; i < size; i += 3 ) {
        const size_t count = std::min<size_t>( 3, size - i );
        uint32_t group = static_cast<uint32_t>( data[i] ) << 16;
        if ( count > 1 ) {
            group |= static_cast<uint32_t>( data[i + 1] ) << 8;
        }
        if ( count > 2 ) {
            group |= data[i + 2];
        }

        text += characters[( group >> 18 ) & 0x3F];
        text += characters[( group >> 12 ) & 0x3F];
        text += count > 1 ? characters[( group >> 6 ) & 0x3F] : '=';
        text += count > 2 ? characters[group & 0x3F] : '=';
    }

    return text;
}

std::string Multi::computeWebSocketAccept( const std::string & key )
{
    const std::array<uint8_t, 20> digest = computeSha1( key + std::string( acceptGuid ) );
    return encodeBase64( digest.data(), digest.size() );
}

std::optional<Multi::LobbyAddress> Multi::parseLobbyAddress( const std::string & text )
{
    std::string_view rest = trim( text );

    LobbyAddress address;
    address.port = defaultLobbyPort;

    const std::string lowerCase = toLower( rest.substr( 0, 6 ) );
    if ( lowerCase.rfind( "ws://", 0 ) == 0 ) {
        rest.remove_prefix( 5 );
        address.port = 80;
    }
    else if ( lowerCase.rfind( "wss://", 0 ) == 0 ) {
        rest.remove_prefix( 6 );
        address.port = 443;
        address.isSecure = true;
    }

    const size_t pathStart = rest.find( '/' );
    const std::string_view authority = rest.substr( 0, pathStart );
    address.path = pathStart == std::string_view::npos ? "/lobby" : std::string( rest.substr( pathStart ) );

    if ( address.path.size() > 200 || !std::all_of( address.path.begin(), address.path.end(), []( const char c ) { return c > ' ' && c < 127; } ) ) {
        return {};
    }

    std::string_view port;
    if ( !authority.empty() && authority.front() == '[' ) {
        const size_t end = authority.find( ']' );
        if ( end == std::string_view::npos ) {
            return {};
        }
        address.host = std::string( authority.substr( 1, end - 1 ) );
        const std::string_view after = authority.substr( end + 1 );
        if ( !after.empty() ) {
            if ( after.front() != ':' ) {
                return {};
            }
            port = after.substr( 1 );
        }
        if ( !isIPv6Text( address.host ) ) {
            return {};
        }
    }
    else if ( std::count( authority.begin(), authority.end(), ':' ) > 1 ) {
        // An IPv6 address without port.
        address.host = std::string( authority );
        if ( !isIPv6Text( address.host ) ) {
            return {};
        }
    }
    else {
        const size_t colon = authority.find( ':' );
        address.host = std::string( authority.substr( 0, colon ) );
        if ( colon != std::string_view::npos ) {
            port = authority.substr( colon + 1 );
        }
        if ( !isHostName( address.host ) ) {
            return {};
        }
    }

    if ( !port.empty() || ( !authority.empty() && authority.back() == ':' ) ) {
        const std::optional<uint16_t> number = parsePortNumber( port );
        if ( !number ) {
            return {};
        }
        address.port = *number;
    }

    return address;
}

Multi::WebSocketClient::WebSocketClient( std::string host, const uint16_t port, std::string path, const size_t maxMessageSize )
    : _host( std::move( host ) )
    , _port( port )
    , _path( std::move( path ) )
    , _maxMessageSize( maxMessageSize )
    , _random( std::random_device()() )
{
    std::array<uint8_t, 16> key{};
    for ( uint8_t & byte : key ) {
        byte = static_cast<uint8_t>( _random() );
    }
    _key = encodeBase64( key.data(), key.size() );
}

std::vector<uint8_t> Multi::WebSocketClient::makeRequest()
{
    std::string host = _host.find( ':' ) == std::string::npos ? _host : "[" + _host + "]";
    if ( _port != 80 ) {
        host += ":" + std::to_string( _port );
    }

    const std::string request = "GET " + _path + " HTTP/1.1\r\nHost: " + host + "\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: " + _key
                                + "\r\nSec-WebSocket-Version: 13\r\nUser-Agent: fheroes2\r\n\r\n";

    return { request.begin(), request.end() };
}

bool Multi::WebSocketClient::_fail( std::string error )
{
    if ( _error.empty() ) {
        _error = std::move( error );
    }
    return false;
}

bool Multi::WebSocketClient::receive( const uint8_t * data, const size_t size )
{
    if ( !_error.empty() ) {
        return false;
    }
    if ( _isClosedByServer ) {
        return true;
    }

    // Drop what was already read before adding more.
    if ( _bufferStart > 0 ) {
        _buffer.erase( _buffer.begin(), _buffer.begin() + static_cast<std::ptrdiff_t>( _bufferStart ) );
        _bufferStart = 0;
    }
    _buffer.insert( _buffer.end(), data, data + size );

    if ( !_isOpen && !_readResponse() ) {
        return _error.empty();
    }

    return _readFrames();
}

bool Multi::WebSocketClient::_readResponse()
{
    const std::string_view received( reinterpret_cast<const char *>( _buffer.data() ), _buffer.size() );
    const size_t headerEnd = received.find( "\r\n\r\n" );

    if ( headerEnd == std::string_view::npos ) {
        if ( _buffer.size() > maxResponseSize ) {
            _fail( gettext_noop( "this address is not a lobby" ) );
        }
        return false;
    }

    const std::string_view header = received.substr( 0, headerEnd );
    if ( header.rfind( "HTTP/1.1 101", 0 ) != 0 ) {
        return _fail( gettext_noop( "this address is not a lobby" ) );
    }

    std::string upgrade;
    std::string connection;
    std::string accept;

    size_t lineStart = header.find( "\r\n" );
    while ( lineStart != std::string_view::npos ) {
        lineStart += 2;
        const size_t lineEnd = header.find( "\r\n", lineStart );
        const std::string_view line = header.substr( lineStart, lineEnd == std::string_view::npos ? std::string_view::npos : lineEnd - lineStart );
        lineStart = lineEnd;

        const size_t colon = line.find( ':' );
        if ( colon == std::string_view::npos ) {
            continue;
        }

        const std::string name = toLower( trim( line.substr( 0, colon ) ) );
        const std::string_view value = trim( line.substr( colon + 1 ) );
        if ( name == "upgrade" ) {
            upgrade = toLower( value );
        }
        else if ( name == "connection" ) {
            connection = toLower( value );
        }
        else if ( name == "sec-websocket-accept" ) {
            accept = std::string( value );
        }
    }

    if ( upgrade != "websocket" || !hasToken( connection, "upgrade" ) || accept != computeWebSocketAccept( _key ) ) {
        return _fail( gettext_noop( "this address is not a lobby" ) );
    }

    _isOpen = true;
    _bufferStart = headerEnd + 4;

    return true;
}

bool Multi::WebSocketClient::_readFrames()
{
    while ( !_isClosedByServer && _buffer.size() - _bufferStart >= 2 ) {
        const uint8_t * frame = _buffer.data() + _bufferStart;
        const size_t available = _buffer.size() - _bufferStart;

        const bool isFinal = ( frame[0] & 0x80 ) != 0;
        const uint8_t opcode = frame[0] & 0x0F;
        uint64_t length = frame[1] & 0x7F;
        size_t offset = 2;

        // No extension was agreed, and a server never masks its frames.
        if ( ( frame[0] & 0x70 ) != 0 || ( frame[1] & 0x80 ) != 0 ) {
            return _fail( gettext_noop( "the lobby sent wrong data" ) );
        }

        if ( length == 126 ) {
            if ( available < 4 ) {
                return true;
            }
            length = ( static_cast<uint64_t>( frame[2] ) << 8 ) | frame[3];
            offset = 4;
        }
        else if ( length == 127 ) {
            if ( available < 10 ) {
                return true;
            }
            length = 0;
            for ( size_t i = 2; i < 10; ++i ) {
                length = ( length << 8 ) | frame[i];
            }
            offset = 10;
        }

        const bool isControl = opcode >= Opcode::close;
        if ( isControl && ( !isFinal || length > 125 ) ) {
            return _fail( gettext_noop( "the lobby sent wrong data" ) );
        }

        // Checked before waiting for the data.
        if ( !isControl && length > _maxMessageSize - _fragments.size() ) {
            return _fail( gettext_noop( "the lobby sent a message which is too big" ) );
        }

        if ( available < offset + length ) {
            return true;
        }

        const uint8_t * payload = frame + offset;
        const size_t size = static_cast<size_t>( length );
        _bufferStart += offset + size;

        switch ( opcode ) {
        case Opcode::binary:
        case Opcode::continuation: {
            const bool isStart = ( opcode == Opcode::binary );
            if ( isStart == _isFragmented ) {
                return _fail( gettext_noop( "the lobby sent wrong data" ) );
            }

            _fragments.insert( _fragments.end(), payload, payload + size );
            _isFragmented = !isFinal;

            if ( isFinal ) {
                _messages.push_back( std::move( _fragments ) );
                _fragments = {};
            }
            break;
        }
        case Opcode::close:
            _isClosedByServer = true;
            _closeCode = size >= 2 ? static_cast<uint16_t>( ( payload[0] << 8 ) | payload[1] ) : 1005;
            if ( !_isCloseSent ) {
                const std::vector<uint8_t> reply = makeClose( size >= 2 ? _closeCode : 1000 );
                _replies.insert( _replies.end(), reply.begin(), reply.end() );
            }
            break;
        case Opcode::ping: {
            const std::vector<uint8_t> pong = _makeFrame( Opcode::pong, payload, size );
            _replies.insert( _replies.end(), pong.begin(), pong.end() );
            break;
        }
        case Opcode::pong:
            ++_pongs;
            break;
        default:
            // Text frames are never used by lobbies.
            return _fail( gettext_noop( "the lobby sent wrong data" ) );
        }
    }

    return true;
}

std::optional<std::vector<uint8_t>> Multi::WebSocketClient::takeMessage()
{
    if ( _messages.empty() ) {
        return {};
    }

    std::vector<uint8_t> message = std::move( _messages.front() );
    _messages.pop_front();

    return message;
}

std::vector<uint8_t> Multi::WebSocketClient::takeReplies()
{
    return std::exchange( _replies, {} );
}

uint32_t Multi::WebSocketClient::takePongs()
{
    return std::exchange( _pongs, 0 );
}

std::vector<uint8_t> Multi::WebSocketClient::_makeFrame( const uint8_t opcode, const uint8_t * data, const size_t size )
{
    std::vector<uint8_t> frame;
    frame.reserve( size + 14 );
    frame.push_back( static_cast<uint8_t>( 0x80 | opcode ) );

    if ( size < 126 ) {
        frame.push_back( static_cast<uint8_t>( 0x80 | size ) );
    }
    else if ( size < 0x10000 ) {
        frame.push_back( 0x80 | 126 );
        frame.push_back( static_cast<uint8_t>( size >> 8 ) );
        frame.push_back( static_cast<uint8_t>( size ) );
    }
    else {
        frame.push_back( 0x80 | 127 );
        for ( int shift = 56; shift >= 0; shift -= 8 ) {
            frame.push_back( static_cast<uint8_t>( static_cast<uint64_t>( size ) >> shift ) );
        }
    }

    std::array<uint8_t, 4> mask{};
    const uint32_t random = _random();
    for ( size_t i = 0; i < mask.size(); ++i ) {
        mask[i] = static_cast<uint8_t>( random >> ( i * 8 ) );
    }
    frame.insert( frame.end(), mask.begin(), mask.end() );

    for ( size_t i = 0; i < size; ++i ) {
        frame.push_back( data[i] ^ mask[i & 3] );
    }

    return frame;
}

std::vector<uint8_t> Multi::WebSocketClient::makeMessage( const uint8_t * data, const size_t size )
{
    return _makeFrame( Opcode::binary, data, size );
}

std::vector<uint8_t> Multi::WebSocketClient::makePing()
{
    return _makeFrame( Opcode::ping, nullptr, 0 );
}

std::vector<uint8_t> Multi::WebSocketClient::makeClose( const uint16_t code )
{
    _isCloseSent = true;
    const std::array<uint8_t, 2> payload{ static_cast<uint8_t>( code >> 8 ), static_cast<uint8_t>( code ) };
    return _makeFrame( Opcode::close, payload.data(), payload.size() );
}
