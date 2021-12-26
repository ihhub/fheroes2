#ifndef NO_NETWORK

#include <cassert>
#include <chrono>
#include <cstdint>
#include <iostream>

#include "ConnectionManager.h"
#include "logging.h"

namespace
{
    std::array<uint8_t, 6> MakeHeader( const std::vector<uint8_t> & payload )
    {
        // header magic values
        std::array<uint8_t, 6> header = { 0xFE, 0xEF };

        // encode the size of the message
        const uint32_t totalSize = 2 + 4 + static_cast<uint32_t>( payload.size() ) + 2; // header + size + payload + footer
        header[2] = ( totalSize & 0xFF );
        header[3] = ( ( totalSize >> 8 ) & 0xFF );
        header[4] = ( ( totalSize >> 16 ) & 0xFF );
        header[5] = ( ( totalSize >> 24 ) & 0xFF );
        return header;
    }
}

using namespace fheroes2::Network;

NetworkConnection::NetworkConnection( const std::string & name, asio::io_context & io_context )
    : _name( name )
    , _ioContext( io_context )
    , _socket( io_context )
    , _isConnected( false )
    , _isConnecting( false )
    , _timeoutForConnecting( io_context )
    , _readStrand( io_context )
    , _readMessageHeader()
    , _readMessageData()
    , _messagesToRead()
    , _writeStrand( io_context )
    , _isWriting( false )
    , _messagesToWrite()
{}

bool NetworkConnection::connectAsync( const asio::ip::tcp::endpoint endpoint, std::atomic<int> & signal, int timeoutSeconds /*= 5*/ )
{
    signal = 0;

    if ( _isConnected || _isConnecting ) {
        return false;
    }

    _isConnecting = true;

    _timeoutForConnecting.expires_after( std::chrono::seconds( timeoutSeconds ) );
    auto self = shared_from_this();
    _timeoutForConnecting.async_wait( _readStrand.wrap( [self, this, &signal]( asio::error_code ) {
        if ( !_isConnected ) {
            _socket.cancel();
            signal = -1;
            DEBUG_LOG( DBG_NETWORK, DBG_INFO, _name << " Timed out on waiting for connections" );
        }
        else {
            DEBUG_LOG( DBG_NETWORK, DBG_INFO, _name << " Timeout expired but already connected : no effect" );
        }
    } ) );

    DEBUG_LOG( DBG_NETWORK, DBG_INFO, _name << " : Connecting to " << endpoint << "..." );

    _socket.async_connect( endpoint, _readStrand.wrap( [self, this, &signal]( const asio::error_code & ec ) {
        if ( !ec ) {
            _readStrand.post( [self, this]() { do_read_header(); } );
            _isConnected = true;
            signal = 1;
        }
        else {
            signal = -1;
            DEBUG_LOG( DBG_NETWORK, DBG_INFO, _name << " : Connection failed with error " << ec.message() );
        }
        _isConnecting = false;
        _timeoutForConnecting.cancel();
    } ) );

    return true;
}

bool NetworkConnection::acceptConnectionAsync( const int port, std::atomic<int> & signal, int timeoutSeconds /*= 50*/ )
{
    signal = 0;

    if ( _isConnected || _isConnecting ) {
        return false;
    }

    _isConnecting = true;

    std::shared_ptr<asio::ip::tcp::acceptor> acceptor = std::make_shared<asio::ip::tcp::acceptor>( _ioContext, asio::ip::tcp::endpoint( asio::ip::tcp::v4(), port ) );

    _timeoutForConnecting.expires_after( std::chrono::seconds( timeoutSeconds ) );
    auto self = shared_from_this();
    _timeoutForConnecting.async_wait( _readStrand.wrap( [self, this, acceptor, &signal]( asio::error_code ) {
        if ( !_isConnected ) {
            acceptor->cancel();
            signal = -1;
            DEBUG_LOG( DBG_NETWORK, DBG_INFO, _name << " Timed out on waiting for connections" );
        }
        else {
            DEBUG_LOG( DBG_NETWORK, DBG_INFO, _name << " Timeout expired but already connected : no effect" );
        }
    } ) );

    DEBUG_LOG( DBG_NETWORK, DBG_INFO, _name << " : Accepting connections on port " << port << "..." );
    acceptor->async_accept( _socket, _readStrand.wrap( [self, this, acceptor, &signal]( const asio::error_code & ec ) {
        if ( !ec ) {
            _readStrand.post( [self, this]() { do_read_header(); } );
            _isConnected = true;
            signal = 1;
        }
        else {
            signal = -1;
            DEBUG_LOG( DBG_NETWORK, DBG_INFO, _name << " : Acceptor failed with error " << ec.message() );
        }
        _isConnecting = false;
        _timeoutForConnecting.cancel();
    } ) );

    return true;
}

void NetworkConnection::close()
{
    _timeoutForConnecting.cancel();
    _isConnected = false;
}

// asynchronously sends a payload to the endpoint. This is thread-safe.
bool NetworkConnection::sendPayload( std::vector<uint8_t> && payload )
{
    if ( !_isConnected ) {
        return false;
    }
    auto self = shared_from_this();
    // Ideally we would use payload = std::move( payload ) to avoid copy but this is C++14
    _writeStrand.post( [self, this, payload]() mutable {
        MessageData messageData( MakeHeader( payload ), std::move( payload ) );
        _messagesToWrite.emplace_back( std::move( messageData ) );
        do_write();
    } );

    return true;
}

bool NetworkConnection::getNextPayload( std::vector<uint8_t> & payload, int timeoutMilliseconds /*= 10*/ )
{
    if ( !_isConnected ) {
        return false;
    }

    std::atomic<int> signal = { -1 };
    const std::chrono::high_resolution_clock::time_point expirationTime = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds( timeoutMilliseconds );
    auto self = shared_from_this();
    _readStrand.post( [self, this, &signal, &payload, &expirationTime]() { try_get_payload( signal, payload, expirationTime ); } );

    while ( signal == -1 ) {
    }

    if ( signal == 1 ) {
        DEBUG_LOG( DBG_NETWORK, DBG_INFO, _name << "  : Extracted message of size " << payload.size() + 8 );
    }
    else {
        DEBUG_LOG( DBG_NETWORK, DBG_INFO, _name << " : Extracting message timed out" );
    }

    return signal == 1;
}

void NetworkConnection::try_get_payload( std::atomic<int> & signal, std::vector<uint8_t> & payload, std::chrono::high_resolution_clock::time_point expirationTime )
{
    if ( !_messagesToRead.empty() ) {
        payload = std::move( _messagesToRead.front() );
        _messagesToRead.pop_front();
        signal = 1; // signal that we found some value
    }
    else if ( std::chrono::high_resolution_clock::now() >= expirationTime || !_isConnected ) {
        signal = 0; // no time left, signal that we abort
    }
    else {
        auto self = shared_from_this();
        // Post a task that will retry. Potentially another task is already pending that will add an item.
        _readStrand.post( [self, this, &signal, &payload, expirationTime]() { try_get_payload( signal, payload, expirationTime ); } );
    }
}

bool NetworkConnection::getNextPayloadInstant( std::vector<uint8_t> & payload )
{
    if ( !_isConnected ) {
        return false;
    }

    std::atomic<int> signal = { -1 };
    auto self = shared_from_this();
    _readStrand.post( [self, this, &signal, &payload]() {
        if ( !_messagesToRead.empty() ) {
            payload = std::move( _messagesToRead.front() );
            _messagesToRead.pop_front();
            signal = 1; // signal that we found some value
        }
        else {
            signal = 0;
        }
    } );

    while ( signal == -1 ) {
    }

    if ( signal == 1 ) {
        DEBUG_LOG( DBG_NETWORK, DBG_INFO, _name << "  : Extracted message of size " << payload.size() + 8 );
    }

    return signal == 1;
}

void NetworkConnection::do_read_header()
{
    auto self = shared_from_this();
    asio::async_read( _socket, asio::buffer( _readMessageHeader ), _readStrand.wrap( [self, this]( asio::error_code ec, std::size_t /*length*/ ) {
        if ( !ec ) {
            if ( _readMessageHeader[0] != 0xFE || _readMessageHeader[1] != 0xEF ) // check header magic values
            {
                DEBUG_LOG( DBG_NETWORK, DBG_INFO, _name << " : Received invalid header, disconnecting" );
                close();
            }

            // read message size info
            int messageSize = _readMessageHeader[2];
            messageSize |= _readMessageHeader[3] << 8;
            messageSize |= _readMessageHeader[4] << 16;
            messageSize |= _readMessageHeader[5] << 24;

            const int payloadSize = messageSize - 8;
            do_read_body( payloadSize );
        }
        else {
            DEBUG_LOG( DBG_NETWORK, DBG_INFO, _name << " : Received error code in do_read_header " << ec.message() );
            close();
        }
    } ) );
}

void NetworkConnection::do_read_body( const int payloadSize )
{
    // resize to have enough data for payload + footer
    _readMessageData.resize( payloadSize + 2 );
    auto self = shared_from_this();
    asio::async_read( _socket, asio::buffer( _readMessageData ), _readStrand.wrap( [self, this, payloadSize]( asio::error_code ec, std::size_t /*length*/ ) {
        if ( !ec ) {
            if ( _readMessageData[payloadSize] != 0xFA || _readMessageData[payloadSize + 1] != 0xAF ) // check footer magic values
            {
                DEBUG_LOG( DBG_NETWORK, DBG_INFO, _name << " : Received invalid footer, disconnecting" );
                close();
            }

            DEBUG_LOG( DBG_NETWORK, DBG_INFO, _name << " : Read message of size " << payloadSize + 8 );

            _readMessageData.resize( _readMessageData.size() - 2 );
            _messagesToRead.emplace_back( std::move( _readMessageData ) );

            do_read_header();
        }
        else {
            DEBUG_LOG( DBG_NETWORK, DBG_INFO, _name << " : Received error code in do_read_body " << ec.message() );
            close();
        }
    } ) );
}

void NetworkConnection::do_write()
{
    if ( _messagesToWrite.empty() || _isWriting ) {
        return;
    }

    _isWriting = true;

    const MessageData & firstMessage = _messagesToWrite.front();

    std::array<asio::const_buffer, 3> message{ asio::buffer( firstMessage.first ), asio::buffer( firstMessage.second ), asio::buffer( _messageFooter ) };
    auto self = shared_from_this();
    asio::async_write( _socket, message, _writeStrand.wrap( [self, this]( asio::error_code ec, std::size_t length ) {
        (void)length;
        _isWriting = false;
        if ( !ec ) {
            DEBUG_LOG( DBG_NETWORK, DBG_INFO, _name << " : Wrote " << length << " bytes" );
            _messagesToWrite.pop_front();
            do_write();
        }
        else {
            DEBUG_LOG( DBG_NETWORK, DBG_INFO, _name << " : Received error code in async_write " << ec.message() );
            close();
        }
    } ) );
}

NetworkConnection::~NetworkConnection()
{
    close();
    DEBUG_LOG( DBG_NETWORK, DBG_INFO, _name << " : instance destroyed" );
}

#endif