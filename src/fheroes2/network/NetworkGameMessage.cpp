#if !defined( NO_NETWORK )

#include <iterator>

#include "ConnectionManager.h"
#include "NetworkGameMessage.h"

using namespace fheroes2::Network;

namespace
{
    OutgoingGameMessage createMessage( const GameMessageType message, const std::vector<uint8_t> & payload )
    {
        const uint16_t messageTypeInt = static_cast<uint16_t>( message );
        OutgoingGameMessage result( payload.size() + 2 );
        result[0] = messageTypeInt & 0xFF;
        result[1] = ( messageTypeInt << 8 ) & 0xFF;
        std::copy( payload.begin(), payload.end(), result.begin() + 2 );
        return result;
    }

    OutgoingGameMessage createMessage( const GameMessageType message )
    {
        return createMessage( message, std::vector<uint8_t>( 0 ) );
    }
}

namespace fheroes2
{
    namespace Network
    {
        //--- Outgoing game messages ---

        OutgoingGameMessage createHandshake1Message( const std::string & playerName )
        {
            std::vector<uint8_t> data;
            std::copy( playerName.begin(), playerName.end(), std::back_inserter( data ) );
            return createMessage( GameMessageType::Handshake1, data );
        }

        OutgoingGameMessage createHandshake2Message( const std::string & playerName )
        {
            std::vector<uint8_t> data;
            std::copy( playerName.begin(), playerName.end(), std::back_inserter( data ) );
            return createMessage( GameMessageType::Handshake2, data );
        }

        OutgoingGameMessage createHandshake3Message()
        {
            return createMessage( GameMessageType::Handshake3 );
        }

        //--- Incoming game messages ---

        IncomingGameMessage::IncomingGameMessage()
            : _messageType( GameMessageType::Invalid )
        {}

        IncomingGameMessage::IncomingGameMessage( std::vector<uint8_t> && payload )
            : _data( std::move( payload ) )
        {
            if ( _data.size() < 2 ) {
                _messageType = GameMessageType::Invalid;
            }
            else {
                uint16_t messageTypeInt = _data[0] | ( static_cast<uint16_t>( _data[1] ) << 8 );
                if ( messageTypeInt >= static_cast<uint16_t>( GameMessageType::LastMessageType ) ) {
                    messageTypeInt = static_cast<uint16_t>( GameMessageType::Invalid );
                }
                _messageType = static_cast<GameMessageType>( messageTypeInt );
            }
        }

        bool getNextGameMessage( NetworkConnection & connection, IncomingGameMessage & gameMessage, int timeoutMilliseconds /*= 10*/ )
        {
            std::vector<uint8_t> payload;
            bool result = connection.getNextPayload( payload, timeoutMilliseconds );
            gameMessage = std::move( payload );
            return result;
        }

        bool getNextGameMessageInstant( NetworkConnection & connection, IncomingGameMessage & gameMessage )
        {
            std::vector<uint8_t> payload;
            bool result = connection.getNextPayloadInstant( payload );
            gameMessage = std::move( payload );
            return result;
        }
    }
}

#endif