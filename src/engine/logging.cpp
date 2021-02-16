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

#include "logging.h"
#include <ctime>

namespace
{
    int g_debug = DBG_ALL_WARN + DBG_ALL_INFO;
}

namespace Logging
{
#if defined( __SWITCH__ ) // Platforms which log to file
    std::ofstream logFile;
#endif

    const char * GetDebugOptionName( const int name )
    {
        if ( name & DBG_ENGINE )
            return "DBG_ENGINE";
        else if ( name & DBG_GAME )
            return "DBG_GAME";
        else if ( name & DBG_BATTLE )
            return "DBG_BATTLE";
        else if ( name & DBG_AI )
            return "DBG_AI";
        else if ( name & DBG_NETWORK )
            return "DBG_NETWORK";
        else if ( name & DBG_OTHER )
            return "DBG_OTHER";
        else if ( name & DBG_DEVEL )
            return "DBG_DEVEL";

        return "";
    }

    std::string GetTimeString()
    {
        time_t raw;
        std::time( &raw );
        struct tm * tmi = std::localtime( &raw );

        char buf[13] = {0};
        std::strftime( buf, sizeof( buf ) - 1, "%X", tmi );

        return std::string( buf );
    }

    void InitLog()
    {
#if defined( __SWITCH__ ) // Platforms which log to file
        logFile.open( "fheroes2.log", std::ofstream::out );
#endif
    }

    void SetDebugLevel( const int debugLevel )
    {
        g_debug = debugLevel;
    }
}

bool IS_DEBUG( const int name, const int level )
{
    return ( ( DBG_ENGINE & name ) && ( ( DBG_ENGINE & g_debug ) >> 2 ) >= level ) || ( ( DBG_GAME & name ) && ( ( DBG_GAME & g_debug ) >> 4 ) >= level )
           || ( ( DBG_BATTLE & name ) && ( ( DBG_BATTLE & g_debug ) >> 6 ) >= level ) || ( ( DBG_AI & name ) && ( ( DBG_AI & g_debug ) >> 8 ) >= level )
           || ( ( DBG_NETWORK & name ) && ( ( DBG_NETWORK & g_debug ) >> 10 ) >= level ) || ( ( DBG_DEVEL & name ) && ( ( DBG_DEVEL & g_debug ) >> 12 ) >= level );
}
