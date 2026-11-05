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

#include "multi_port_mapping.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <chrono>
#include <functional>
#include <optional>
#include <system_error>
#include <utility>
#include <vector>

#if defined( WITH_NETWORK )

// This file and multi_network.cpp are the only files which include asio.
#if defined( _WIN32 )
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#endif
#define ASIO_NO_DEPRECATED

#if !__has_include( <asio.hpp> )
#error "asio is missing: install the standalone asio headers (script/windows/install_packages.bat with Visual Studio)"
#endif

#include <asio.hpp>

#include "logging.h"
#include "multi_network.h"
#include "translations.h"

namespace
{
    // Boxes usually answer the search within a second or two.
    constexpr std::chrono::milliseconds searchTime{ 2500 };
    // Once a box has answered the other answers come right after it.
    constexpr std::chrono::milliseconds searchTimeAfterAnswer{ 400 };
    constexpr std::chrono::seconds requestTimeout{ 4 };
    constexpr std::chrono::milliseconds closingTimeout{ 1500 };

    // A description or an answer bigger than this cannot come from a box.
    constexpr size_t maxResponseSize{ size_t{ 256 } * 1024 };

    const std::array<const char *, 4> searchTargets{ "urn:schemas-upnp-org:device:InternetGatewayDevice:1", "urn:schemas-upnp-org:device:InternetGatewayDevice:2",
                                                     "urn:schemas-upnp-org:service:WANIPConnection:1", "urn:schemas-upnp-org:service:WANPPPConnection:1" };

    // Services are listed in order of preference.
    const std::array<const char *, 3> connectionServices{ "urn:schemas-upnp-org:service:WANIPConnection:2", "urn:schemas-upnp-org:service:WANIPConnection:1",
                                                          "urn:schemas-upnp-org:service:WANPPPConnection:1" };

    std::string toLower( std::string text )
    {
        std::transform( text.begin(), text.end(), text.begin(), []( const unsigned char c ) { return static_cast<char>( std::tolower( c ) ); } );
        return text;
    }

    std::string trim( const std::string & text )
    {
        const size_t begin = text.find_first_not_of( " \t\r\n" );
        if ( begin == std::string::npos ) {
            return {};
        }

        return text.substr( begin, text.find_last_not_of( " \t\r\n" ) - begin + 1 );
    }

    struct Url
    {
        std::string host;
        uint16_t port{ 80 };
        std::string path{ "/" };
    };

    std::optional<Url> parseUrl( const std::string & text )
    {
        const std::string scheme = "http://";
        if ( text.size() <= scheme.size() || toLower( text.substr( 0, scheme.size() ) ) != scheme ) {
            return {};
        }

        const size_t pathBegin = text.find( '/', scheme.size() );
        const std::string hostAndPort = text.substr( scheme.size(), pathBegin == std::string::npos ? std::string::npos : pathBegin - scheme.size() );

        Url url;
        if ( pathBegin != std::string::npos ) {
            url.path = text.substr( pathBegin );
        }

        const size_t colon = hostAndPort.find( ':' );
        url.host = hostAndPort.substr( 0, colon );

        if ( colon != std::string::npos ) {
            const std::string portText = hostAndPort.substr( colon + 1 );

            int port = 0;
            const auto [end, error] = std::from_chars( portText.data(), portText.data() + portText.size(), port );
            if ( error != std::errc() || end != portText.data() + portText.size() || port < 1 || port > 65535 ) {
                return {};
            }

            url.port = static_cast<uint16_t>( port );
        }

        if ( url.host.empty() ) {
            return {};
        }

        return url;
    }

    // Resolves an address given by the box, which can be absolute or relative to the base.
    std::optional<Url> resolveUrl( const Url & base, const std::string & text )
    {
        if ( toLower( text.substr( 0, 7 ) ) == "http://" ) {
            return parseUrl( text );
        }

        Url url = base;
        url.path = ( !text.empty() && text.front() == '/' ) ? text : "/" + text;
        return url;
    }

    // Returns the value of a header in an HTTP or search answer.
    std::string findHeader( const std::string & response, const std::string & name )
    {
        const std::string lowerName = toLower( name );

        size_t lineBegin = 0;
        while ( lineBegin < response.size() ) {
            size_t lineEnd = response.find( "\r\n", lineBegin );
            if ( lineEnd == std::string::npos ) {
                lineEnd = response.size();
            }

            // Headers end with an empty line.
            if ( lineEnd == lineBegin ) {
                break;
            }

            const std::string line = response.substr( lineBegin, lineEnd - lineBegin );
            const size_t colon = line.find( ':' );
            if ( colon != std::string::npos && toLower( trim( line.substr( 0, colon ) ) ) == lowerName ) {
                return trim( line.substr( colon + 1 ) );
            }

            lineBegin = lineEnd + 2;
        }

        return {};
    }

    // Returns the text of the first <tag> element, with or without a namespace prefix.
    std::optional<std::string> findElement( const std::string & xml, const std::string & tag )
    {
        for ( size_t position = xml.find( tag ); position != std::string::npos; position = xml.find( tag, position + 1 ) ) {
            const size_t contentBegin = position + tag.size() + 1;
            if ( position == 0 || contentBegin > xml.size() || xml[position + tag.size()] != '>' ) {
                continue;
            }

            const size_t open = xml.rfind( '<', position - 1 );
            if ( open == std::string::npos || xml[open + 1] == '/' ) {
                continue;
            }

            // The prefix is either empty or something like "u:".
            const std::string prefix = xml.substr( open + 1, position - open - 1 );
            if ( !prefix.empty() && ( prefix.back() != ':' || prefix.find_first_of( " /<>" ) != std::string::npos ) ) {
                continue;
            }

            const size_t contentEnd = xml.find( std::string( "</" ).append( prefix ).append( tag ).append( ">" ), contentBegin );
            if ( contentEnd == std::string::npos ) {
                return {};
            }

            return trim( xml.substr( contentBegin, contentEnd - contentBegin ) );
        }

        return {};
    }

    struct HttpResponse
    {
        int status{ 0 };
        std::string body;
    };

    std::optional<HttpResponse> parseHttpResponse( const std::string & text )
    {
        // "HTTP/1.1 200 OK"
        if ( text.size() < 12 || text.compare( 0, 5, "HTTP/" ) != 0 ) {
            return {};
        }

        const size_t space = text.find( ' ' );
        if ( space == std::string::npos || space + 4 > text.size() ) {
            return {};
        }

        HttpResponse response;
        const auto [end, error] = std::from_chars( text.data() + space + 1, text.data() + space + 4, response.status );
        if ( error != std::errc() || end != text.data() + space + 4 ) {
            return {};
        }

        const size_t headersEnd = text.find( "\r\n\r\n" );
        if ( headersEnd == std::string::npos ) {
            return {};
        }

        const std::string body = text.substr( headersEnd + 4 );

        if ( toLower( findHeader( text, "Transfer-Encoding" ) ).find( "chunked" ) == std::string::npos ) {
            response.body = body;
            return response;
        }

        // In a chunked body each chunk follows its size in hexadecimal, size 0 ends the body.
        size_t position = 0;
        while ( position < body.size() ) {
            const size_t sizeEnd = body.find( "\r\n", position );
            if ( sizeEnd == std::string::npos ) {
                return {};
            }

            size_t chunkSize = 0;
            const auto [sizeParsed, sizeError] = std::from_chars( body.data() + position, body.data() + sizeEnd, chunkSize, 16 );
            if ( sizeError != std::errc() || sizeParsed == body.data() + position ) {
                return {};
            }

            if ( chunkSize == 0 ) {
                return response;
            }

            const size_t chunkBegin = sizeEnd + 2;
            if ( chunkSize > body.size() - chunkBegin ) {
                return {};
            }

            response.body.append( body, chunkBegin, chunkSize );
            position = chunkBegin + chunkSize + 2;
        }

        return {};
    }

    std::string makeSoapRequest( const Url & url, const std::string & service, const std::string & action, const std::string & arguments )
    {
        const std::string body = "<?xml version=\"1.0\"?>\r\n"
                                 "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
                                 "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body><u:"
                                 + action + " xmlns:u=\"" + service + "\">" + arguments + "</u:" + action + "></s:Body></s:Envelope>\r\n";

        return "POST " + url.path + " HTTP/1.1\r\nHost: " + url.host + ":" + std::to_string( url.port )
               + "\r\nContent-Type: text/xml; charset=\"utf-8\"\r\nSOAPAction: \"" + service + "#" + action + "\"\r\nContent-Length: " + std::to_string( body.size() )
               + "\r\nConnection: close\r\n\r\n" + body;
    }

    // Home networks and addresses shared between customers, unreachable from the internet.
    bool isPrivateAddress( const asio::ip::address_v4 & address )
    {
        const asio::ip::address_v4::bytes_type bytes = address.to_bytes();

        return bytes[0] == 10 || ( bytes[0] == 172 && ( bytes[1] & 0xF0 ) == 16 ) || ( bytes[0] == 192 && bytes[1] == 168 )
               || ( bytes[0] == 100 && ( bytes[1] & 0xC0 ) == 64 ) || bytes[0] == 0 || address.is_loopback();
    }

    struct SearchSocket
    {
        explicit SearchSocket( asio::io_context & io )
            : socket( io )
        {}

        std::array<char, 2048> buffer{};
        asio::ip::udp::endpoint sender;
        asio::ip::udp::socket socket;
    };
}

struct Multi::PortMapping::Impl
{
    // Buffers must be declared before the I/O objects. Pending operations still use them while the I/O objects are destroyed.
    std::string request;
    std::string response;
    std::array<char, 4096> readBuffer{};

    asio::io_context io;
    std::vector<std::unique_ptr<SearchSocket>> searchSockets;
    asio::ip::tcp::socket tcp{ io };
    asio::steady_timer timer{ io };

    State state{ State::IDLE };
    uint16_t port{ 0 };

    std::vector<std::string> locations;
    size_t nextLocation{ 0 };
    bool isSearching{ false };

    Url controlUrl;
    std::string service;
    std::string localAddress;
    bool isMapped{ false };
    bool isTemporaryLease{ false };

    std::string externalAddress;
    bool isBehindAnotherNetwork{ false };
    std::string error;

    void fail( const std::string & reason )
    {
        if ( state != State::WORKING ) {
            return;
        }

        VERBOSE_LOG( "[MULTI] The internet box did not open the port: " << reason )

        error = reason;
        state = State::FAILED;
        stopSearch();
        timer.cancel();
    }

    void stopSearch()
    {
        isSearching = false;

        for ( const std::unique_ptr<SearchSocket> & searchSocket : searchSockets ) {
            std::error_code ignored;
            searchSocket->socket.close( ignored );
        }
    }

    // The box answers only on its own network, so search on all of them.
    void search()
    {
        isSearching = true;

        std::vector<std::string> addresses = getLocalAddresses();

        // There is no box behind Tailscale addresses.
        addresses.erase( std::remove_if( addresses.begin(), addresses.end(), []( const std::string & text ) { return text.rfind( "100.", 0 ) == 0; } ), addresses.end() );
        addresses.emplace_back( "0.0.0.0" );

        const asio::ip::udp::endpoint multicast( asio::ip::make_address_v4( "239.255.255.250" ), 1900 );

        for ( const std::string & address : addresses ) {
            std::error_code errorCode;
            const asio::ip::address_v4 localAddressV4 = asio::ip::make_address_v4( address, errorCode );
            if ( errorCode ) {
                continue;
            }

            auto searchSocket = std::make_unique<SearchSocket>( io );
            searchSocket->socket.open( asio::ip::udp::v4(), errorCode );
            if ( !errorCode ) {
                searchSocket->socket.bind( asio::ip::udp::endpoint( localAddressV4, 0 ), errorCode );
            }
            if ( errorCode ) {
                continue;
            }

            for ( const char * target : searchTargets ) {
                const std::string message
                    = std::string( "M-SEARCH * HTTP/1.1\r\nHOST: 239.255.255.250:1900\r\nMAN: \"ssdp:discover\"\r\nMX: 2\r\nST: " ) + target + "\r\n\r\n";
                searchSocket->socket.send_to( asio::buffer( message ), multicast, 0, errorCode );
            }

            receiveAnswer( *searchSocket );
            searchSockets.push_back( std::move( searchSocket ) );
        }

        timer.expires_after( searchTime );
        timer.async_wait( [this]( const std::error_code & errorCode ) {
            if ( !errorCode ) {
                endSearch();
            }
        } );
    }

    void receiveAnswer( SearchSocket & searchSocket )
    {
        searchSocket.socket.async_receive_from( asio::buffer( searchSocket.buffer ), searchSocket.sender,
                                                [this, &searchSocket]( const std::error_code & errorCode, const size_t size ) {
                                                    if ( errorCode || !isSearching ) {
                                                        return;
                                                    }

                                                    onAnswer( std::string( searchSocket.buffer.data(), size ) );
                                                    receiveAnswer( searchSocket );
                                                } );
    }

    void onAnswer( const std::string & answer )
    {
        if ( answer.compare( 0, 7, "HTTP/1." ) != 0 || answer.find( " 200" ) != 8 ) {
            return;
        }

        const std::string target = findHeader( answer, "ST" );
        const std::string location = findHeader( answer, "LOCATION" );

        const bool isGateway = target.find( "InternetGatewayDevice" ) != std::string::npos || target.find( "WANIPConnection" ) != std::string::npos
                               || target.find( "WANPPPConnection" ) != std::string::npos;

        if ( !isGateway || location.empty() || std::find( locations.begin(), locations.end(), location ) != locations.end() ) {
            return;
        }

        locations.push_back( location );

        // The first box has answered, the search ends soon.
        if ( locations.size() == 1 ) {
            timer.expires_after( searchTimeAfterAnswer );
            timer.async_wait( [this]( const std::error_code & errorCode ) {
                if ( !errorCode ) {
                    endSearch();
                }
            } );
        }
    }

    void endSearch()
    {
        stopSearch();

        if ( locations.empty() ) {
            fail( gettext_noop( "no internet box answered" ) );
            return;
        }

        readNextDescription();
    }

    void readNextDescription()
    {
        std::optional<Url> url;
        while ( !url ) {
            if ( nextLocation >= locations.size() ) {
                fail( gettext_noop( "UPnP is off or missing in the box" ) );
                return;
            }

            url = parseUrl( locations[nextLocation++] );
        }

        request = "GET " + url->path + " HTTP/1.1\r\nHost: " + url->host + ":" + std::to_string( url->port ) + "\r\nConnection: close\r\n\r\n";

        sendRequest( *url, [this, base = *url]( const std::optional<HttpResponse> & answer ) {
            if ( !answer || answer->status != 200 ) {
                readNextDescription();
                return;
            }

            if ( !findConnectionService( base, answer->body ) ) {
                readNextDescription();
                return;
            }

            addMapping( false );
        } );
    }

    bool findConnectionService( const Url & location, const std::string & description )
    {
        Url base = location;
        if ( const std::optional<std::string> urlBase = findElement( description, "URLBase" ); urlBase && !urlBase->empty() ) {
            if ( const std::optional<Url> parsed = parseUrl( *urlBase ); parsed ) {
                base = *parsed;
            }
        }

        for ( const char * wanted : connectionServices ) {
            size_t position = 0;
            while ( ( position = description.find( "<serviceType>", position ) ) != std::string::npos ) {
                const size_t blockEnd = description.find( "</service>", position );
                const std::string block = description.substr( position, blockEnd == std::string::npos ? std::string::npos : blockEnd - position );
                position += 1;

                const std::optional<std::string> type = findElement( block, "serviceType" );
                const std::optional<std::string> control = findElement( block, "controlURL" );
                if ( !type || *type != wanted || !control ) {
                    continue;
                }

                const std::optional<Url> url = resolveUrl( base, *control );
                if ( !url ) {
                    continue;
                }

                controlUrl = *url;
                service = wanted;
                return true;
            }
        }

        return false;
    }

    std::string mappingArguments( const bool isTemporary ) const
    {
        const std::string portText = std::to_string( port );

        return "<NewRemoteHost></NewRemoteHost><NewExternalPort>" + portText + "</NewExternalPort><NewProtocol>TCP</NewProtocol><NewInternalPort>" + portText
               + "</NewInternalPort><NewInternalClient>" + localAddress
               + "</NewInternalClient><NewEnabled>1</NewEnabled><NewPortMappingDescription>fheroes2</NewPortMappingDescription><NewLeaseDuration>"
               + ( isTemporary ? "86400" : "0" ) + "</NewLeaseDuration>";
    }

    // Unlimited lease, or one day if the box refuses it.
    void addMapping( const bool isTemporary )
    {
        isTemporaryLease = isTemporary;
        request = makeSoapRequest( controlUrl, service, "AddPortMapping", mappingArguments( isTemporary ) );

        sendRequest( controlUrl, [this]( const std::optional<HttpResponse> & answer ) { onMappingAnswer( answer ); } );
    }

    void onMappingAnswer( const std::optional<HttpResponse> & answer )
    {
        if ( answer && answer->status == 200 ) {
            isMapped = true;
            VERBOSE_LOG( "[MULTI] Port " << port << " of the internet box forwarded to " << localAddress )
            readExternalAddress();
            return;
        }

        const std::string code = answer ? findElement( answer->body, "errorCode" ).value_or( std::to_string( answer->status ) ) : std::string();

        // Error 718 means the port is already forwarded to another computer.
        if ( code == "718" ) {
            fail( gettext_noop( "the port is already used by another computer" ) );
            return;
        }

        if ( !isTemporaryLease && answer ) {
            addMapping( true );
            return;
        }

        fail( answer ? std::string( gettext_noop( "the box refused" ) ) + ": " + code : std::string( gettext_noop( "the box does not answer" ) ) );
    }

    void readExternalAddress()
    {
        request = makeSoapRequest( controlUrl, service, "GetExternalIPAddress", "" );

        sendRequest( controlUrl, [this]( const std::optional<HttpResponse> & answer ) { onExternalAddress( answer ); } );
    }

    void onExternalAddress( const std::optional<HttpResponse> & answer )
    {
        if ( answer && answer->status == 200 ) {
            std::error_code errorCode;
            const asio::ip::address_v4 address = asio::ip::make_address_v4( findElement( answer->body, "NewExternalIPAddress" ).value_or( "" ), errorCode );
            if ( !errorCode ) {
                externalAddress = address.to_string();
                isBehindAnotherNetwork = isPrivateAddress( address );
            }
        }

        if ( externalAddress.empty() ) {
            VERBOSE_LOG( "[MULTI] Internet address of the box unknown" )
        }
        else {
            VERBOSE_LOG( "[MULTI] Internet address of the box " << ( isBehindAnotherNetwork ? "private" : "found" ) )
        }

        state = State::OPEN;
    }

    // Sends 'request' and calls done() with the answer, or with nothing on error or timeout.
    void sendRequest( const Url & url, std::function<void( const std::optional<HttpResponse> & )> done )
    {
        std::error_code errorCode;
        const asio::ip::address address = asio::ip::make_address( url.host, errorCode );
        if ( errorCode ) {
            done( {} );
            return;
        }

        std::error_code ignored;
        tcp.close( ignored );
        response.clear();

        auto finish = std::make_shared<std::function<void( const std::optional<HttpResponse> & )>>( std::move( done ) );

        timer.expires_after( requestTimeout );
        timer.async_wait( [this]( const std::error_code & timerError ) {
            if ( !timerError ) {
                std::error_code closeError;
                tcp.close( closeError );
            }
        } );

        tcp.async_connect( asio::ip::tcp::endpoint( address, url.port ), [this, finish]( const std::error_code & connectError ) {
            if ( connectError ) {
                timer.cancel();
                ( *finish )( {} );
                return;
            }

            std::error_code endpointError;
            const asio::ip::tcp::endpoint local = tcp.local_endpoint( endpointError );
            if ( !endpointError ) {
                localAddress = local.address().to_string();
            }

            asio::async_write( tcp, asio::buffer( request ), [this, finish]( const std::error_code & writeError, const size_t /* written */ ) {
                if ( writeError ) {
                    timer.cancel();
                    ( *finish )( {} );
                    return;
                }

                readResponse( finish );
            } );
        } );
    }

    void readResponse( const std::shared_ptr<std::function<void( const std::optional<HttpResponse> & )>> & finish )
    {
        tcp.async_read_some( asio::buffer( readBuffer ), [this, finish]( const std::error_code & readError, const size_t size ) {
            response.append( readBuffer.data(), size );

            if ( !readError && response.size() <= maxResponseSize ) {
                readResponse( finish );
                return;
            }

            timer.cancel();

            std::error_code ignored;
            tcp.close( ignored );

            if ( readError != asio::error::eof || response.size() > maxResponseSize ) {
                ( *finish )( {} );
                return;
            }

            ( *finish )( parseHttpResponse( response ) );
        } );
    }

    void start( const uint16_t mappedPort )
    {
        port = mappedPort;
        state = State::WORKING;
        VERBOSE_LOG( "[MULTI] Asking the internet box to open port " << port )
    }

    // Waits for the answer of the box only a moment.
    void removeMapping()
    {
        if ( !isMapped ) {
            return;
        }

        isMapped = false;
        state = State::WORKING;

        request = makeSoapRequest( controlUrl, service, "DeletePortMapping",
                                   "<NewRemoteHost></NewRemoteHost><NewExternalPort>" + std::to_string( port ) + "</NewExternalPort><NewProtocol>TCP</NewProtocol>" );

        bool isDone = false;
        sendRequest( controlUrl, [this, &isDone]( const std::optional<HttpResponse> & answer ) {
            isDone = true;
            onMappingRemoved( answer );
        } );

        io.restart();

        const auto deadline = std::chrono::steady_clock::now() + closingTimeout;
        while ( !isDone && std::chrono::steady_clock::now() < deadline ) {
            io.run_for( std::chrono::milliseconds( 50 ) );
        }
    }

    void onMappingRemoved( const std::optional<HttpResponse> & answer ) const
    {
        if ( answer && answer->status == 200 ) {
            VERBOSE_LOG( "[MULTI] Port " << port << " of the internet box closed" )
        }
        else {
            VERBOSE_LOG( "[MULTI] Port " << port << " of the internet box maybe left open" )
        }
    }
};

Multi::PortMapping::PortMapping()
    : _impl( std::make_unique<Impl>() )
{}

Multi::PortMapping::~PortMapping()
{
    try {
        _impl->removeMapping();
    }
    catch ( ... ) {
        // Nothing may leave a destructor. The port may then stay open in the box until the box restarts.
        _impl.reset();
    }
}

void Multi::PortMapping::start( const uint16_t port )
{
    _impl->start( port );
    _impl->search();
}

void Multi::PortMapping::startWithDescription( const uint16_t port, const std::string & descriptionUrl )
{
    _impl->start( port );
    _impl->locations.push_back( descriptionUrl );
    _impl->readNextDescription();
}

void Multi::PortMapping::update()
{
    if ( _impl->state != State::WORKING ) {
        return;
    }

    try {
        _impl->io.poll();
    }
    catch ( const std::exception & ex ) {
        _impl->fail( ex.what() );
    }
}

Multi::PortMapping::State Multi::PortMapping::state() const
{
    return _impl->state;
}

const std::string & Multi::PortMapping::externalAddress() const
{
    return _impl->externalAddress;
}

bool Multi::PortMapping::isBehindAnotherNetwork() const
{
    return _impl->isBehindAnotherNetwork;
}

const std::string & Multi::PortMapping::error() const
{
    return _impl->error;
}

#else

#include "translations.h"

// A build without network games, see multi_network.cpp.
struct Multi::PortMapping::Impl
{
    State state{ State::IDLE };
    std::string externalAddress;
    std::string error{ gettext_noop( "network games are not available in this version of fheroes2" ) };
};

Multi::PortMapping::PortMapping()
    : _impl( std::make_unique<Impl>() )
{}

Multi::PortMapping::~PortMapping() = default;

void Multi::PortMapping::start( const uint16_t /* port */ )
{
    _impl->state = State::FAILED;
}

void Multi::PortMapping::startWithDescription( const uint16_t /* port */, const std::string & /* descriptionUrl */ )
{
    _impl->state = State::FAILED;
}

void Multi::PortMapping::update()
{
    // Do nothing.
}

Multi::PortMapping::State Multi::PortMapping::state() const
{
    return _impl->state;
}

const std::string & Multi::PortMapping::externalAddress() const
{
    return _impl->externalAddress;
}

bool Multi::PortMapping::isBehindAnotherNetwork() const
{
    return false;
}

const std::string & Multi::PortMapping::error() const
{
    return _impl->error;
}

#endif
