/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2026                                                    *
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

#ifdef TARGET_NINTENDO_3DS

#include <memory>

#include <3ds.h>
#include <3ds/synchronization.h>

template <class T, int MaxThreads = 10>
class ThreadLocal
{
public:
    explicit ThreadLocal( std::function<T()> factory )
        : factory_( std::move( factory ) )
    {
        LightLock_Init( &lock_ );
    }

    T & get()
    {
        LightLock_Lock( &lock_ );
        // Note: main thread is created by the console and is nullptr
        Thread self = threadGetCurrent();

        for ( auto & s : slots_ ) {
            if ( s.used && s.thread == self ) {
                assert( s.value );
                LightLock_Unlock( &lock_ );
                return *s.value;
            }

            if ( !s.used ) {
                s.used = true;
                s.thread = self;
                s.value.reset( new T( factory_() ) );
                LightLock_Unlock( &lock_ );
                return *s.value;
            }
        }

        // Unlikely, we use less threads
        LightLock_Unlock( &lock_ );
        svcBreak( USERBREAK_PANIC );
        __builtin_unreachable();
    }

private:
    struct Slot
    {
        bool used = false;
        Thread thread = nullptr;
        std::unique_ptr<T> value = nullptr;
    };

    std::function<T()> factory_;
    std::array<Slot, MaxThreads> slots_;
    LightLock lock_;
};
#endif
