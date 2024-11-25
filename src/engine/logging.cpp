/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2024                                             *
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

#include <array>
#include <cassert>
#include <ctime>

#if defined( _WIN32 )
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#if defined( MACOS_APP_BUNDLE )
#include <syslog.h>
#endif

#include "logging.h"
#include "system.h"

namespace
{
    int debugLevel = DBG_ALL_WARN;
    bool textSupportMode = false;

#if defined( _WIN32 )
    // Sets the Windows console codepage to the system codepage
    class ConsoleCPSwitcher
    {
    public:
        ConsoleCPSwitcher()
            : _consoleOutputCP( GetConsoleOutputCP() )
        {
            if ( _consoleOutputCP > 0 ) {
                SetConsoleOutputCP( GetACP() );
            }
        }

        ConsoleCPSwitcher( const ConsoleCPSwitcher & ) = delete;

        ~ConsoleCPSwitcher()
        {
            if ( _consoleOutputCP > 0 ) {
                SetConsoleOutputCP( _consoleOutputCP );
            }
        }

        ConsoleCPSwitcher & operator=( const ConsoleCPSwitcher & ) = delete;

    private:
        const UINT _consoleOutputCP;
    };

    const ConsoleCPSwitcher consoleCPSwitcher;
#endif
}

namespace Logging
{
#if defined( TARGET_NINTENDO_SWITCH ) || defined( _WIN32 )
    std::ofstream logFile;
    // This mutex protects operations with logFile
    std::mutex logMutex;
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
        const tm tmi = System::GetTM( std::time( nullptr ) );

        std::array<char, 256> buf;

        const size_t writtenBytes = std::strftime( buf.data(), buf.size(), "%d.%m.%Y %H:%M:%S", &tmi );
        if ( writtenBytes == 0 ) {
            assert( 0 );
            return "<TIMESTAMP ERROR>";
        }

        return std::string( buf.data() );
    }

    void InitLog()
    {
#if defined( TARGET_NINTENDO_SWITCH )
        const std::scoped_lock<std::mutex> lock( logMutex );

        logFile.open( "fheroes2.log", std::ofstream::out );
#elif defined( _WIN32 )
        const std::scoped_lock<std::mutex> lock( logMutex );

        const std::string configDir = System::GetConfigDirectory( "fheroes2" );

        System::MakeDirectory( configDir );

        logFile.open( System::concatPath( configDir, "fheroes2.log" ), std::ofstream::out );
#elif defined( MACOS_APP_BUNDLE )
        openlog( "fheroes2", LOG_CONS | LOG_NDELAY, LOG_USER );

        setlogmask( LOG_UPTO( LOG_WARNING ) );
#endif
    }

    void setDebugLevel( const int level )
    {
        debugLevel = level;
    }

    int getDebugLevel()
    {
        return debugLevel;
    }

    void setTextSupportMode( const bool enableTextSupportMode )
    {
        textSupportMode = enableTextSupportMode;
    }

    bool isTextSupportModeEnabled()
    {
        return textSupportMode;
    }
}

bool IS_DEBUG( const int name, const int level )
{
    return ( ( DBG_ENGINE & name ) && ( ( DBG_ENGINE & debugLevel ) >> 2 ) >= level ) || ( ( DBG_GAME & name ) && ( ( DBG_GAME & debugLevel ) >> 4 ) >= level )
           || ( ( DBG_BATTLE & name ) && ( ( DBG_BATTLE & debugLevel ) >> 6 ) >= level ) || ( ( DBG_AI & name ) && ( ( DBG_AI & debugLevel ) >> 8 ) >= level )
           || ( ( DBG_NETWORK & name ) && ( ( DBG_NETWORK & debugLevel ) >> 10 ) >= level ) || ( ( DBG_DEVEL & name ) && ( ( DBG_DEVEL & debugLevel ) >> 12 ) >= level );
}
