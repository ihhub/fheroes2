/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
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

#include "system.h"
#include "thread.h"

using namespace SDL;

Thread::Thread() : thread(NULL)
{
}

Thread::~Thread()
{
    Kill();
}

Thread::Thread(const Thread &) : thread(NULL)
{
}

Thread & Thread::operator= (const Thread &)
{
    return *this;
}

void Thread::Create(int (*fn)(void *), void *param)
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
    thread = SDL_CreateThread(fn, "", param);
#else
    thread = SDL_CreateThread(fn, param);
#endif
}

int Thread::Wait(void)
{
    int status = 0;
    if(thread) SDL_WaitThread(thread, &status);
    thread = NULL;
    return status;
}

void Thread::Kill(void)
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
#else
    if(thread) SDL_KillThread(thread);
    thread = NULL;
#endif
}

bool Thread::IsRun(void) const
{
    return GetID();
}

u32 Thread::GetID(void) const
{
    return thread ? SDL_GetThreadID(thread) : 0;
}

Mutex::Mutex(bool init) : mutex(init ? SDL_CreateMutex() : NULL)
{
}

Mutex::Mutex(const Mutex &) : mutex(NULL)
{
}

Mutex::~Mutex()
{
    if(mutex) SDL_DestroyMutex(mutex);
}

Mutex & Mutex::operator= (const Mutex &)
{
    return *this;
}

void Mutex::Create(void)
{
    if(mutex) SDL_DestroyMutex(mutex);
    mutex = SDL_CreateMutex();
}

bool Mutex::Lock(void) const
{
    return mutex ? 0 == SDL_mutexP(mutex) : false;
}

bool Mutex::Unlock(void) const
{
    return mutex ? 0 == SDL_mutexV(mutex) : false;
}

Timer::Timer() : id(0)
{
}

void Timer::Run(u32 interval, u32 (*fn)(u32, void *), void *param)
{
    if(id) Remove();

    id = SDL_AddTimer(interval, fn, param);
}

void Timer::Remove(void)
{
    if(id)
    {
	SDL_RemoveTimer(id);
	id = 0;
    }
}

bool Timer::IsValid(void) const
{
    return id;
}

Time::Time()
{
}

void Time::Start(void)
{
    tick2 = tick1 = SDL_GetTicks();
}

void Time::Stop(void)
{
    tick2 = SDL_GetTicks();
}

u32 Time::Get(void) const
{
    return tick2 > tick1 ? tick2 - tick1 : 0;
}

void Time::Print(const char* header) const
{
    ERROR((header ? header : "time: ") << Get() << " ms");
}
