#if !defined( NO_NETWORK )

#pragma once

#include <atomic>
#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow" // required because asio will trigger this warning on macOS builds
#include "asio.hpp"
#pragma GCC diagnostic pop

// asio will import some window header that define macros for GetObject, this can conflict with fheroes2 methods
#ifdef GetObject
#undef GetObject
#endif

namespace fheroes2
{
    namespace Network
    {
        // This class is the low-level class that manages a connection through a socket and writing/reading data.
        class NetworkConnection : public std::enable_shared_from_this<NetworkConnection>
        {
        public:
            NetworkConnection( const std::string & name, asio::io_context & io_context );

            NetworkConnection( const NetworkConnection & ) = delete;
            NetworkConnection & operator=( const NetworkConnection & ) = delete;
            NetworkConnection( NetworkConnection && ) = delete;
            NetworkConnection & operator=( NetworkConnection && ) = delete;

            bool isConnected() const
            {
                return _isConnected;
            }

            // Connect to a server
            bool connectAsync( const asio::ip::tcp::endpoint endpoint, std::atomic<int> & signal, int timeoutSeconds = 5 );

            // Wait for connection from clients
            bool acceptConnectionAsync( const int port, std::atomic<int> & signal, int timeoutSeconds = 50 );

            // Closes the connection
            void close();

            // Asynchronously sends a payload to the endpoint. This is thread-safe.
            bool sendPayload( std::vector<uint8_t> && payload );

            // Get next payload if one is in the queue
            bool getNextPayloadInstant( std::vector<uint8_t> & payload );

            // Block and get next payload, or aborts if we exceed the timeout
            bool getNextPayload( std::vector<uint8_t> & payload, int timeoutMilliseconds = 10 );

            ~NetworkConnection();

        private:
            void do_read_header();
            void do_read_body( const int payloadSize );
            void do_write();
            void try_get_payload( std::atomic<int> & signal, std::vector<uint8_t> & payload, std::chrono::steady_clock::time_point expirationTime );

            // used for debug only
            const std::string _name;

            asio::io_context & _ioContext;
            asio::ip::tcp::socket _socket;

            std::atomic<bool> _isConnected;
            std::atomic<bool> _isConnecting;

            //--- connection part

            // the pending timer that we have for timeout when connecting.
            // It needs to be visible at class level so we can cancel it when the class is destroyed, otherwise we will keep waiting on it.
            asio::steady_timer _timeoutForConnecting;

            //--- reading part

            // all read operations will go through _readStrand so they will be executed sequentially
            asio::io_service::strand _readStrand;

            // temporary buffers where the data from socket are stored during reading:
            std::array<uint8_t, 6> _readMessageHeader;
            std::vector<uint8_t> _readMessageData;

            // permanent buffer where the messages read from socket are stored
            std::deque<std::vector<uint8_t>> _messagesToRead;

            //--- writing part

            // all write operations will go through _writeStrand so they will be executed sequentially
            asio::io_service::strand _writeStrand;

            // tells whether or not there is a current write on the socket. This is always accessed through _writeStrand so no need to be atomic<bool>
            bool _isWriting;

            // type of data we store before writing the message to socket: it's a 6 bytes header and a message
            using MessageData = std::pair<std::array<uint8_t, 6>, std::vector<uint8_t>>;
            // we also write a footer to the socket but it's always the same
            const std::array<uint8_t, 2> _messageFooter = { 0xFA, 0xAF };

            // buffer where the messages to write to socket are stored
            std::deque<MessageData> _messagesToWrite;
        };
    }
}

#endif