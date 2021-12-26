#if !defined( NO_NETWORK )

#include <iostream>

#include "NetworkHandshake.h"
#include "dialog.h"
#include "text.h"

using namespace fheroes2::Network;

namespace
{
    const int fheroes2port = 466;

    // These will have to not be global later on when we have real usage beyond just handshake
    ConnectionResourcesHandler gResourceHandlerClient;
    ConnectionResourcesHandler gResourceHandlerServer;

    // Helper to get the external IP address of this host by doing a HTTP GET to api.ipify.org and parse its result
    std::string getHostIpAddress()
    {
        static std::string ipAddress;

        if ( !ipAddress.empty() ) {
            return ipAddress;
        }

        try {
            asio::io_service io_service;
            std::string hostAddress = "api.ipify.org";

            // Get a list of endpoints corresponding to the server name.
            asio::ip::tcp::resolver resolver( io_service );
            asio::ip::tcp::resolver::query query( hostAddress, "http" );
            asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve( query );

            // Try each endpoint until we successfully establish a connection.
            asio::ip::tcp::socket socket( io_service );
            asio::connect( socket, endpoint_iterator );

            // Form the request.
            asio::streambuf request;
            std::ostream request_stream( &request );
            request_stream << "GET / HTTP/1.1\r\n";
            request_stream << "Host: " << hostAddress << "\r\n";
            request_stream << "Accept: */*\r\n";
            request_stream << "Connection: close\r\n\r\n";

            // Send the request.
            asio::write( socket, request );

            asio::streambuf response;
            std::ostringstream ss;

            // Read until EOF, writing data to output as we go.
            asio::error_code error;
            while ( asio::read( socket, response, asio::transfer_at_least( 1 ), error ) )
                ss << &response;

            std::string httpResponse = ss.str();

            if ( httpResponse.find( "200 OK" ) != std::string::npos ) {
                size_t pos = httpResponse.find_last_of( '\n' );
                if ( pos != std::string::npos ) {
                    std::string ipAddressStr = httpResponse.substr( pos + 1 );
                    asio::ip::make_address_v4( ipAddressStr ); // this will throw if ipAddressStr is not a correct ip address
                    ipAddress = ipAddressStr;
                }
            }
        }
        catch ( std::exception & e ) {
        }
        return ipAddress;
    }

}

namespace fheroes2
{
    namespace Network
    {
        // Open an handshake connection for a server.
        // Here, I purposefully create a server with 2 open connections to test the behavior
        // Also note that if there is a failure, I don't close the full gResourceHandlerServer instance, but I just delete a NetworkConnection instance.
        // This brings no benefits, except testing that the gResourceHandlerServer still behaves correctly.
        bool HandshakeServer( const std::string & serverName )
        {
            const int timeoutSeconds = 50;

            std::ostringstream name;
            name << "Server[" << serverName << "]";
            gResourceHandlerServer.init( name.str(), 2 ); // create server with 2 open connections, just to try

            NetworkConnection & server = *gResourceHandlerServer.getConnections()[0];

            std::atomic<int> signal = { 0 };
            server.acceptConnectionAsync( fheroes2port, signal, timeoutSeconds );

            std::ostringstream dialogMessage;
            dialogMessage << "Accepting client connections on port " << fheroes2port << " for the next " << timeoutSeconds << " seconds...";
            const int connectionResult = Dialog::MessageUntilSignal( "Connecting", dialogMessage.str(), signal, Font::BIG );

            if ( connectionResult == Dialog::CANCEL ) { // cancelled by user
                server.close();
                gResourceHandlerServer.getConnections().clear();
                Dialog::Message( "Handshake failed", "Connection cancelled", Font::BIG, Dialog::OK );
                return false;
            }
            else if ( connectionResult == Dialog::NO ) { // timed out
                server.close();
                gResourceHandlerServer.getConnections().clear();
                Dialog::Message( "Handshake failed", "Connection timed out", Font::BIG, Dialog::OK );
                return false;
            }

            IncomingGameMessage gameMessage;
            bool gotMessage = getNextGameMessage( server, gameMessage );

            if ( gotMessage && gameMessage.getMessageType() == GameMessageType::Handshake1 ) {
                std::string clientName( gameMessage.data().begin() + 2, gameMessage.data().end() );
                gResourceHandlerServer.setPlayerName( clientName );
                server.sendPayload( std::move( createHandshake2Message( serverName ) ) );
                gotMessage = getNextGameMessage( server, gameMessage );
                if ( gotMessage && gameMessage.getMessageType() == GameMessageType::Handshake3 ) {
                    std::ostringstream oss;
                    oss << "Server Handshake completed with player " << clientName;
                    Dialog::Message( "Handshake suceeded", oss.str(), Font::BIG, Dialog::OK );
                    return true;
                }
                else {
                    Dialog::Message( "Handshake failed", gotMessage ? "Incorrect message received from client" : "Timed out", Font::BIG, Dialog::OK );
                    server.close();
                    gResourceHandlerServer.getConnections().clear();
                    return false;
                }
            }
            else {
                Dialog::Message( "Handshake failed", gotMessage ? "Incorrect message received from client" : "Timed out", Font::BIG, Dialog::OK );
                server.close();
                gResourceHandlerServer.getConnections().clear();
                return false;
            }
        }

        // Open an handshake connection for a client.
        // If there is an error, we completely close the gResourceHandlerClient instance
        bool HandshakeClient( const std::string & clientName )
        {
            const int timeoutSeconds = 3;
            std::ostringstream ss;

            std::string ipAddress = getHostIpAddress();
            bool ok = Dialog::InputString( "IPv4 address to connect to", ipAddress, std::string(), 20 );
            if ( !ok ) {
                return false;
            }

            // due to the font, some dots are stored as either '<' or '>' so we correct this
            std::replace( ipAddress.begin(), ipAddress.end(), '<', '.' );
            std::replace( ipAddress.begin(), ipAddress.end(), '>', '.' );

            asio::error_code ec;
            asio::ip::address_v4 ip = asio::ip::make_address_v4( ipAddress, ec );

            if ( ec ) {
                ss << "Invalid IPv4: " << ipAddress;
                Dialog::Message( "Handshake failed", ss.str(), Font::BIG, Dialog::OK );
                return false;
            }

            asio::ip::tcp::endpoint endpoint( ip, fheroes2port );

            std::ostringstream name;
            name << "Client[" << clientName << "]";
            gResourceHandlerClient.init( name.str(), 1 );

            NetworkConnection & client = *gResourceHandlerClient.getConnections()[0];

            std::atomic<int> signal = { 0 };
            client.connectAsync( endpoint, signal, timeoutSeconds );

            std::ostringstream dialogMessage;
            dialogMessage << "Trying to connect to " << ipAddress << " on port " << fheroes2port << "...";
            const int connectionResult = Dialog::MessageUntilSignal( "Connecting", dialogMessage.str(), signal, Font::BIG );

            if ( connectionResult == Dialog::CANCEL ) { // cancelled by user
                gResourceHandlerClient.stop();
                Dialog::Message( "Handshake failed", "Connection cancelled", Font::BIG, Dialog::OK );
                return false;
            }
            else if ( connectionResult == Dialog::NO ) { // timed out
                gResourceHandlerClient.stop();
                Dialog::Message( "Handshake failed", "Connection timed out", Font::BIG, Dialog::OK );
                return false;
            }

            client.sendPayload( std::move( createHandshake1Message( clientName ) ) );

            IncomingGameMessage gameMessage;
            getNextGameMessage( client, gameMessage );

            if ( gameMessage.getMessageType() == GameMessageType::Handshake2 ) {
                std::string serverName( gameMessage.data().begin() + 2, gameMessage.data().end() );
                gResourceHandlerClient.setPlayerName( serverName );
                client.sendPayload( std::move( createHandshake3Message() ) );
                std::ostringstream oss;
                oss << "Client Handshake completed with player " << serverName;
                Dialog::Message( "Handshake suceeded", oss.str(), Font::BIG, Dialog::OK );
                return true;
            }
            else {
                gResourceHandlerClient.stop();
                Dialog::Message( "Handshake failed", "Incorrect message received from server", Font::BIG, Dialog::OK );
                return false;
            }
        }
    }
}

#endif // NO_NETWORK