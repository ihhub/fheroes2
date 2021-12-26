#if !defined( NO_NETWORK )

#pragma once

#include <string>
#include <vector>

namespace fheroes2
{
    namespace Network
    {
        class ConnectionManager;

        // A type of game message that can be sent from one endpoint to the other
        enum class GameMessageType : uint16_t
        {
            Invalid,
            Handshake1,
            Handshake2,
            Handshake3,
            LastMessageType,
        };

        //--- Outgoing game messages ---

        // Game message data that is ready to be sent over the network
        using OutgoingGameMessage = std::vector<uint8_t>;

        OutgoingGameMessage createHandshake1Message( const std::string & playerName );
        OutgoingGameMessage createHandshake2Message( const std::string & playerName );
        OutgoingGameMessage createHandshake3Message();

        //--- Incoming game messages ---

        // Game message that we received from the network
        class IncomingGameMessage
        {
        public:
            IncomingGameMessage();
            IncomingGameMessage( std::vector<uint8_t> && payload );

            GameMessageType getMessageType() const
            {
                return _messageType;
            }

            const std::vector<uint8_t> & data() const
            {
                return _data;
            }

        private:
            GameMessageType _messageType;
            std::vector<uint8_t> _data;
        };

        // these methods are small wrappers around NetworkConnection so instead of extracting a payload we extract a IncomingGameMessage
        bool getNextGameMessage( NetworkConnection & connection, IncomingGameMessage & gameMessage, int timeoutMilliseconds = 10 );
        bool getNextGameMessageInstant( NetworkConnection & connection, IncomingGameMessage & gameMessage );

    }
}

#endif