/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2022 - 2025                                             *
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

#include "thread.h"

#include <cassert>
#include <memory>

#if defined( __EMSCRIPTEN__ ) && !defined( __EMSCRIPTEN_PTHREADS__ )
namespace
{
    class MutexUnlocker
    {
    public:
        explicit MutexUnlocker( std::mutex & mutex )
            : _mutex( mutex )
        {
            _mutex.unlock();
        }

        MutexUnlocker( const MutexUnlocker & ) = delete;

        ~MutexUnlocker()
        {
            _mutex.lock();
        }

        MutexUnlocker & operator=( const MutexUnlocker & ) = delete;

    private:
        std::mutex & _mutex;
    };
}
#endif

namespace MultiThreading
{
    void AsyncManager::createWorker()
    {
#if !defined( __EMSCRIPTEN__ ) || defined( __EMSCRIPTEN_PTHREADS__ )
        if ( !_worker ) {
            _runFlag = true;
            _worker = std::make_unique<std::thread>( AsyncManager::_workerThread, this );

            {
                std::unique_lock<std::mutex> lock( _mutex );

                _masterNotification.wait( lock, [this] { return !_runFlag; } );
            }
        }
#endif
    }

    void AsyncManager::stopWorker()
    {
#if defined( __EMSCRIPTEN__ ) && !defined( __EMSCRIPTEN_PTHREADS__ )
        assert( !_worker );
#else
        if ( _worker ) {
            {
                const std::scoped_lock<std::mutex> lock( _mutex );

                _exitFlag = true;
                _runFlag = true;
            }

            _workerNotification.notify_all();

            _worker->join();
            _worker.reset();
        }
#endif
    }

    void AsyncManager::notifyWorker()
    {
        _runFlag = true;

#if defined( __EMSCRIPTEN__ ) && !defined( __EMSCRIPTEN_PTHREADS__ )
        assert( !_exitFlag );

        while ( _runFlag ) {
            const bool moreTasks = prepareTask();
            if ( !moreTasks ) {
                _runFlag = false;
            }

            {
                // In accordance with the contract, the _mutex should NOT be acquired while
                // calling the executeTask() - even if its acquisition is in fact a snake oil.
                MutexUnlocker unlocker( _mutex );

                executeTask();
            }
        }
#else
        _workerNotification.notify_all();
#endif
    }

    void AsyncManager::_workerThread( AsyncManager * manager )
    {
        assert( manager != nullptr );

        {
            const std::scoped_lock<std::mutex> lock( manager->_mutex );

            manager->_runFlag = false;
        }

        manager->_masterNotification.notify_one();

        while ( !manager->_exitFlag ) {
            {
                std::unique_lock<std::mutex> lock( manager->_mutex );

                manager->_workerNotification.wait( lock, [manager] { return manager->_runFlag; } );
            }

            if ( manager->_exitFlag ) {
                break;
            }

            {
                const std::scoped_lock<std::mutex> lock( manager->_mutex );

                const bool moreTasks = manager->prepareTask();
                if ( !moreTasks ) {
                    manager->_runFlag = false;
                }
            }

            manager->executeTask();
        }
    }
}
