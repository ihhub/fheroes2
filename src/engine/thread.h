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

#pragma once

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>

namespace MultiThreading
{
    class AsyncManager
    {
    public:
        AsyncManager() = default;

        AsyncManager( const AsyncManager & ) = delete;
        AsyncManager( AsyncManager && ) = delete;

        virtual ~AsyncManager();

        AsyncManager & operator=( const AsyncManager & ) = delete;
        AsyncManager & operator=( AsyncManager && ) = delete;

    protected:
        std::mutex _mutex;

        void _createThreadIfNeeded();

        void notifyThread();

        // Prepare a task which requires mutex lock. Returns true if more tasks are available.
        virtual bool prepareTask() = 0;

        // Task execution is done in non-thread safe mode! No mutex lock for any means of synchronizations are done for this call.
        virtual void executeTask() = 0;

    private:
        std::unique_ptr<std::thread> _worker;

        std::condition_variable _masterNotification;
        std::condition_variable _workerNotification;

        uint8_t _exitFlag{ 0 };
        uint8_t _runFlag{ 1 };

        static void _workerThread( AsyncManager * manager );
    };
}
