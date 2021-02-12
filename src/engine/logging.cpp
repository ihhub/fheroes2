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

static int debug = DEFAULT_DEBUG;

#if defined( __SWITCH__ ) // Platforms which log to file
std::ofstream log_file;
#endif

void InitLog( int debug_setting )
{
    debug = debug_setting;
#if defined( __SWITCH__ ) // Platforms which log to file
    log_file.open( "fheroes2.log", std::ofstream::out );
#endif
}

std::string GetLogTime()
{
    time_t raw;
    struct tm * tmi;
    char buf[13] = {0};

    std::time( &raw );
    tmi = std::localtime( &raw );

    std::strftime( buf, sizeof( buf ) - 1, "%X", tmi );

    return std::string( buf );
}

bool IS_DEBUG( int name, int level )
{
    return ( ( DBG_ENGINE & name ) && ( ( DBG_ENGINE & debug ) >> 2 ) >= level ) || ( ( DBG_GAME & name ) && ( ( DBG_GAME & debug ) >> 4 ) >= level )
           || ( ( DBG_BATTLE & name ) && ( ( DBG_BATTLE & debug ) >> 6 ) >= level ) || ( ( DBG_AI & name ) && ( ( DBG_AI & debug ) >> 8 ) >= level )
           || ( ( DBG_NETWORK & name ) && ( ( DBG_NETWORK & debug ) >> 10 ) >= level ) || ( ( DBG_DEVEL & name ) && ( ( DBG_DEVEL & debug ) >> 12 ) >= level );
}

const char * StringDebug( int name )
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
