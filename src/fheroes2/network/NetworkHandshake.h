#if !defined( NO_NETWORK )

#include <iostream>

#include "ConnectionManager.h"
#include "NetworkGameMessage.h"

namespace fheroes2
{
    namespace Network
    {
        // Manages the lifecycle of all io and network objects necessary for a server or a client
        // The difference with between Client and Server version is the number of connections we manage (only one for client)
        class ConnectionResourcesHandler
        {
        public:

            std::string getPlayerName() const
            {
                return _playerName;
            }

            void setPlayerName( const std::string & name )
            {
                _playerName = name;
            }

            std::vector<std::shared_ptr<NetworkConnection>>&  getConnections()
            {
                return _connections;
            }

            void init( const std::string & name, const int nbConnections )
            {
                assert( nbConnections > 0 );
                stop();

                const int nbThreads = nbConnections + 1;

                _io_context.reset( new asio::io_context() );
                _work_guard.reset( new asio::executor_work_guard<asio::io_context::executor_type>( _io_context->get_executor() ) );
                for ( int i = 0; i < nbThreads; ++i ) {
                    _io_context_threads.emplace_back( new std::thread( [this]() { _io_context->run(); } ) );
                }

                for ( int i = 0; i < nbConnections; ++i ) {
                    std::ostringstream stream;
                    stream << name << '_' << i;
                    _connections.emplace_back( std::make_shared<NetworkConnection>( stream.str(), *_io_context ) );
                }
            }

            ~ConnectionResourcesHandler()
            {
                stop();
            }

            void stop()
            {
                _playerName.clear();

                if ( _work_guard ) {
                    // delete work guard, so threads calling _io_context->run() can finish executing if they have no work left
                    _work_guard.reset();
                }

                for ( auto & c : _connections ) {
                    // closes all connections
                    c->close();
                }
                _connections.clear();

                for ( auto & t : _io_context_threads ) {
                    // allow all pending work to complete, this includes calls made through closeAfterAllPendingWrites()
                    t->join();
                }
                _io_context_threads.clear();

                // _io_context is deleted last
                _io_context.reset();
            }

        private:
            std::unique_ptr<asio::io_context> _io_context = nullptr;
            std::vector<std::unique_ptr<std::thread>> _io_context_threads;
            std::unique_ptr<asio::executor_work_guard<asio::io_context::executor_type>> _work_guard = nullptr;

            std::vector<std::shared_ptr<NetworkConnection>> _connections;
            std::string _playerName;
        };


        bool HandshakeServer( const std::string & serverName );
        bool HandshakeClient( const std::string & clientName );
    }



}


#endif // NO_NETWORK