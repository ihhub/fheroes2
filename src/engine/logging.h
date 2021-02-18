/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2021                                                    *
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

#ifndef H2LOGGING_H
#define H2LOGGING_H

#include <iostream>
#include <sstream>

enum
{
    DBG_WARN = 0x0001,
    DBG_INFO = 0x0002,
    DBG_TRACE = 0x0003,

    DBG_ENGINE = 0x000C,
    DBG_GAME = 0x0030,
    DBG_BATTLE = 0x00C0,
    DBG_AI = 0x0300,
    DBG_NETWORK = 0x0C00,
    DBG_OTHER = 0x3000,
    DBG_DEVEL = 0xC000,

    DBG_ENGINE_WARN = 0x0004,
    DBG_GAME_WARN = 0x0010,
    DBG_BATTLE_WARN = 0x0040,
    DBG_AI_WARN = 0x0100,
    DBG_NETWORK_WARN = 0x0400,
    DBG_OTHER_WARN = 0x1000,

    DBG_ENGINE_INFO = 0x0008,
    DBG_GAME_INFO = 0x0020,
    DBG_BATTLE_INFO = 0x0080,
    DBG_AI_INFO = 0x0200,
    DBG_NETWORK_INFO = 0x0800,
    DBG_OTHER_INFO = 0x2000,

    DBG_ENGINE_TRACE = DBG_ENGINE,
    DBG_GAME_TRACE = DBG_GAME,
    DBG_BATTLE_TRACE = DBG_BATTLE,
    DBG_AI_TRACE = DBG_AI,
    DBG_NETWORK_TRACE = DBG_NETWORK,
    DBG_OTHER_TRACE = DBG_OTHER,

    DBG_ALL = DBG_ENGINE | DBG_GAME | DBG_BATTLE | DBG_AI | DBG_NETWORK | DBG_OTHER,

    DBG_ALL_WARN = DBG_ENGINE_WARN | DBG_GAME_WARN | DBG_BATTLE_WARN | DBG_AI_WARN | DBG_NETWORK_WARN | DBG_OTHER_WARN,
    DBG_ALL_INFO = DBG_ENGINE_INFO | DBG_GAME_INFO | DBG_BATTLE_INFO | DBG_AI_INFO | DBG_NETWORK_INFO | DBG_OTHER_INFO,
    DBG_ALL_TRACE = DBG_ENGINE_TRACE | DBG_GAME_TRACE | DBG_BATTLE_TRACE | DBG_AI_TRACE | DBG_NETWORK_TRACE | DBG_OTHER_TRACE
};

namespace Logging
{
    const char * GetDebugOptionName( const int name );

    std::string GetTimeString();

    // Initialize logging. Some systems require writing logging information into a file.
    void InitLog();

    void SetDebugLevel( const int debugLevel );
}

#if defined( ANDROID ) // Android has a specific logging function
#include <android/log.h>
namespace std
{
    static const char * android_endl = "\n";
}
#define endl android_endl
#define COUT( x )                                                                                                                                                        \
    {                                                                                                                                                                    \
        std::ostringstream osss;                                                                                                                                         \
        osss << x;                                                                                                                                                       \
        __android_log_print( ANDROID_LOG_INFO, "FHeroes2", "%s", osss.str().c_str() );                                                                                   \
    }

#elif defined( __SWITCH__ ) // Platforms which log to file
#include <fstream>

namespace Logging
{
    extern std::ofstream logFile;
}

#define COUT( x )                                                                                                                                                        \
    {                                                                                                                                                                    \
        Logging::logFile << x << std::endl;                                                                                                                              \
        Logging::logFile.flush();                                                                                                                                        \
    }
#else // Default: log to STDERR
#define COUT( x )                                                                                                                                                        \
    {                                                                                                                                                                    \
        std::cerr << x << std::endl;                                                                                                                                     \
    }
#endif

#define VERBOSE_LOG( x )                                                                                                                                                 \
    {                                                                                                                                                                    \
        COUT( Logging::GetTimeString() << ": [VERBOSE]\t" << __FUNCTION__ << ":  " << x );                                                                               \
    }
#define ERROR_LOG( x )                                                                                                                                                   \
    {                                                                                                                                                                    \
        COUT( Logging::GetTimeString() << ": [ERROR]\t" << __FUNCTION__ << ":  " << x );                                                                                 \
    }

#ifdef WITH_DEBUG
#define DEBUG_LOG( x, y, z )                                                                                                                                             \
    if ( IS_DEBUG( x, y ) ) {                                                                                                                                            \
        COUT( Logging::GetTimeString() << ": [" << Logging::GetDebugOptionName( x ) << "]\t" << __FUNCTION__ << ":  " << z );                                            \
    }
#else
#define DEBUG_LOG( x, y, z )
#endif
#define IS_DEVEL() IS_DEBUG( DBG_DEVEL, DBG_INFO )

bool IS_DEBUG( const int name, const int level );

#endif // H2LOGGING_H
