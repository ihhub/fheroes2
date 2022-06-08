/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2022                                                    *
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

namespace MultiThreading
{
    AsyncManager::~AsyncManager()
    {
        if ( _worker ) {
            {
                std::lock_guard<std::mutex> guard( _mutex );

                _exitFlag = 1;
                _runFlag = 1;
                _workerNotification.notify_all();
            }

            _worker->join();
            _worker.reset();
        }
    }

    void AsyncManager::_createThreadIfNeeded()
    {
        if ( !_worker ) {
            _runFlag = 1;
            _worker = std::make_unique<std::thread>( AsyncManager::_workerThread, this );

            std::unique_lock<std::mutex> mutexLock( _mutex );
            _masterNotification.wait( mutexLock, [this] { return _runFlag == 0; } );
        }
    }

    void AsyncManager::_workerThread( AsyncManager * manager )
    {
        assert( manager != nullptr );

        {
            std::lock_guard<std::mutex> guard( manager->_mutex );
            manager->_runFlag = 0;
            manager->_masterNotification.notify_one();
        }

        while ( manager->_exitFlag == 0 ) {
            {
                std::unique_lock<std::mutex> mutexLock( manager->_mutex );
                manager->_workerNotification.wait( mutexLock, [manager] { return manager->_runFlag == 1; } );
            }

            if ( manager->_exitFlag )
                break;

            {
                std::lock_guard<std::mutex> guard( manager->_mutex );

                const bool moreTasks = manager->prepareTask();
                if ( !moreTasks ) {
                    manager->_runFlag = 0;
                }
            }

            manager->executeTask();
        }
    }

    void AsyncManager::notifyThread()
    {
        _runFlag = 1;
        _workerNotification.notify_all();
    }
}
