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

#ifndef SDLTHREAD_H
#define SDLTHREAD_H

#include <sys/time.h>
#include "SDL_thread.h"
#include "types.h"

namespace SDL
{

class Thread
{
public:
    Thread();
    ~Thread();
    Thread(const Thread &);

    Thread & operator= (const Thread &);

    void	Create(int (*)(void *), void *param = NULL);
    int		Wait(void);
    void	Kill(void);

    bool	IsRun(void) const;

    u32		GetID(void) const;

private:
    SDL_Thread *thread;
};

class Mutex
{
public:
    Mutex(bool init = false);
    Mutex(const Mutex &);
    ~Mutex();

    Mutex & operator= (const Mutex &);

    void Create(void);
    bool Lock(void) const;
    bool Unlock(void) const;

private:

    SDL_mutex *mutex;
};

class Timer
{
public:
    Timer();

    bool IsValid(void) const;

    void Run(u32, u32 (*)(u32, void *), void *param = NULL);
    void Remove(void);

private:
    SDL_TimerID id;
};

class Time
{
public:
    Time();

    void Start(void);
    void Stop(void);
    u32 Get(void) const;
    void Print(const char* header = NULL) const;

private:
    u32 tick1;
    u32 tick2;
};

}

#endif
