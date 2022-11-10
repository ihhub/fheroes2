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

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

namespace MultiThreading
{
    class AsyncManager
    {
    public:
        AsyncManager() = default;
        AsyncManager( const AsyncManager & ) = delete;

        virtual ~AsyncManager() = default;

        AsyncManager & operator=( const AsyncManager & ) = delete;

        // Create the worker thread if it doesn't exist yet. Both createWorker() and stopWorker() are not
        // designed to be executed concurrently.
        void createWorker();

        // Stop and join the worker thread. This cannot be done in the destructor (directly or indirectly) due
        // to the potential race on the vptr since this class has virtual methods that could be called from the
        // worker thread.
        void stopWorker();

    protected:
        std::mutex _mutex;

        // Notify the worker thread about a new task. The _mutex should be acquired while calling this method.
        void notifyWorker();

        // Prepare a task which requires mutex lock. Returns true if more tasks are available.
        virtual bool prepareTask() = 0;

        // Task execution is done in non-thread safe mode! No mutex lock for any means of synchronizations are
        // done for this call.
        virtual void executeTask() = 0;

    private:
        std::unique_ptr<std::thread> _worker;

        std::condition_variable _masterNotification;
        std::condition_variable _workerNotification;

        std::atomic<bool> _exitFlag{ false };
        bool _runFlag{ false };

        static void _workerThread( AsyncManager * manager );
    };
}
