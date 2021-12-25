#if !defined( NO_NETWORK )

#include <iostream>

#include "ConnectionManager.h"
#include "NetworkGameMessage.h"

namespace fheroes2
{
    namespace Network
    {
        // Manages the lifecycle of all io and network objects necessary for a client or a server
        class ConnectionResourcesHandler
        {
        public:
            NetworkConnection * getNetworkConnection()
            {
                return _networkConnection.get();
            }

            std::string getPlayerName() const
            {
                return _playerName;
            }

            void setPlayerName( std::string name )
            {
                _playerName = name;
            }

            void init( const std::string & name, const int nbThreads )
            {
                assert( nbThreads > 0 );
                stop();

                _io_context.reset( new asio::io_context() );
                _work_guard.reset( new asio::executor_work_guard<asio::io_context::executor_type>( _io_context->get_executor() ) );
                for ( int i = 0; i < nbThreads; ++i ) {
                    _io_context_threads.emplace_back( new std::thread( [this]() { _io_context->run(); } ) );
                }
                _networkConnection.reset( new NetworkConnection( name, *_io_context ) );
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

                if ( _networkConnection ) {
                    // ask networkConnection to disconnect (this is async so it will be effective later)
                    _networkConnection->closeAfterAllPendingWrites();
                }

                for ( auto & t : _io_context_threads ) {
                    // allow all pending work to complete, this includes calls made through closeAfterAllPendingWrites()
                    t->join();
                }
                _io_context_threads.clear();

                _networkConnection.reset();

                // _io_context is deleted last
                _io_context.reset();
            }

        private:
            std::unique_ptr<asio::io_context> _io_context = nullptr;
            std::vector<std::unique_ptr<std::thread>> _io_context_threads;
            std::unique_ptr<asio::executor_work_guard<asio::io_context::executor_type>> _work_guard = nullptr;
            std::unique_ptr<NetworkConnection> _networkConnection = nullptr;
            std::string _playerName;
        };

        bool HandshakeServer( const std::string & serverName );
        bool HandshakeClient( const std::string & clientName );
    }

}


#endif // NO_NETWORK