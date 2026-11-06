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

#include "multi_crash_report.h"

#if defined( _WIN32 ) && defined( _M_X64 )
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <array>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <string>

#include <windows.h>

#include "logging.h"

namespace
{
    // The addresses of a stack are numbers, Windows reads them through pointers.
    template <typename Pointer>
    Pointer toPointer( const DWORD64 address )
    {
        static_assert( sizeof( Pointer ) == sizeof( address ) );

        Pointer pointer = nullptr;
        std::memcpy( static_cast<void *>( &pointer ), &address, sizeof( pointer ) );
        return pointer;
    }

    std::string describeAddress( const DWORD64 address )
    {
        HMODULE module = nullptr;
        if ( !GetModuleHandleExA( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, toPointer<LPCSTR>( address ), &module ) ) {
            std::ostringstream os;
            os << "0x" << std::hex << address;
            return os.str();
        }

        std::array<char, MAX_PATH> path{};
        GetModuleFileNameA( module, path.data(), MAX_PATH );

        std::string name( path.data() );
        if ( const size_t slash = name.find_last_of( "\\/" ); slash != std::string::npos ) {
            name.erase( 0, slash + 1 );
        }

        std::ostringstream os;
        os << name << "+0x" << std::hex << ( address - reinterpret_cast<DWORD64>( module ) );
        return os.str();
    }

    std::string describeStack( CONTEXT context )
    {
        std::ostringstream os;

        for ( int frame = 0; frame < 32 && context.Rip != 0; ++frame ) {
            os << ' ' << describeAddress( context.Rip );

            DWORD64 imageBase = 0;
            PRUNTIME_FUNCTION function = RtlLookupFunctionEntry( context.Rip, &imageBase, nullptr );

            if ( function == nullptr ) {
                // For a leaf function the return address is on top of the stack.
                context.Rip = *toPointer<const DWORD64 *>( context.Rsp );
                context.Rsp += sizeof( DWORD64 );
                continue;
            }

            void * handlerData = nullptr;
            DWORD64 establisherFrame = 0;
            RtlVirtualUnwind( UNW_FLAG_NHANDLER, imageBase, context.Rip, function, &context, &handlerData, &establisherFrame, nullptr );
        }

        return os.str();
    }

    LONG WINAPI writeCrashReport( EXCEPTION_POINTERS * info )
    {
        ERROR_LOG( "[MULTI] Crash: exception 0x" << std::hex << info->ExceptionRecord->ExceptionCode << std::dec << ", frames:" << describeStack( *info->ContextRecord ) )

        return EXCEPTION_CONTINUE_SEARCH;
    }

    // A failed assertion or an uncaught C++ exception ends up in abort().
    void writeAbortReport( int /* signal */ )
    {
        CONTEXT context{};
        RtlCaptureContext( &context );

        ERROR_LOG( "[MULTI] Crash: abort, frames:" << describeStack( context ) )
    }
}

void Multi::installCrashReport()
{
    static bool isInstalled = false;

    if ( !isInstalled ) {
        SetUnhandledExceptionFilter( writeCrashReport );
        std::signal( SIGABRT, writeAbortReport );
        isInstalled = true;
    }
}

#else

void Multi::installCrashReport()
{
    // Windows x64 only
}

#endif
