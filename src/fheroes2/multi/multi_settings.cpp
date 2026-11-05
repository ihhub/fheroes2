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

#include "multi_settings.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
#include <string_view>

#include "system.h"

namespace
{
    std::string getSettingsFilePath()
    {
        return System::concatPath( System::GetConfigDirectory( "fheroes2" ), "multi.cfg" );
    }

    const char * toText( const bool value )
    {
        return value ? "on" : "off";
    }
}

Multi::NetworkSettings Multi::loadNetworkSettings()
{
    std::ifstream file( getSettingsFilePath() );

    NetworkSettings settings;
    std::string line;
    bool isFirstLine = true;

    while ( std::getline( file, line ) ) {
        if ( !line.empty() && line.back() == '\r' ) {
            line.pop_back();
        }

        // Notepad starts UTF-8 files with a byte order mark.
        if ( isFirstLine && line.rfind( "\xEF\xBB\xBF", 0 ) == 0 ) {
            line.erase( 0, 3 );
        }

        const size_t separator = line.find( " = " );

        // The first versions kept only the address on the first line.
        if ( separator == std::string::npos ) {
            if ( isFirstLine ) {
                settings.address = line;
            }
            isFirstLine = false;
            continue;
        }

        isFirstLine = false;

        const std::string_view key = std::string_view( line ).substr( 0, separator );
        const std::string value = line.substr( separator + 3 );

        if ( key == "address" ) {
            settings.address = value;
        }
        else if ( key == "simultaneous turns" ) {
            settings.isSimultaneous = ( value == "on" );
        }
        else if ( key == "nickname" ) {
            settings.nickname = value;
        }
        else if ( key == "lobby" ) {
            if ( !value.empty() && std::find( settings.lobbies.begin(), settings.lobbies.end(), value ) == settings.lobbies.end() ) {
                settings.lobbies.push_back( value );
            }
        }
        else if ( key == "always relay" ) {
            settings.isRelayAlways = ( value == "on" );
        }
        else if ( key == "public games" ) {
            settings.isPublic = ( value != "off" );
        }
        else if ( key == "shared lobby" ) {
            settings.isSharedLobbyUsed = ( value != "off" );
        }
        else if ( key == "machine key" ) {
            std::istringstream text( value );
            uint64_t machineKey = 0;
            if ( text >> std::hex >> machineKey ) {
                settings.machineKey = machineKey;
            }
        }
    }

    return settings;
}

void Multi::saveNetworkSettings( const NetworkSettings & settings )
{
    std::ofstream file( getSettingsFilePath(), std::ios::trunc );

    file << "address = " << settings.address << '\n';
    file << "simultaneous turns = " << toText( settings.isSimultaneous ) << '\n';
    file << "nickname = " << settings.nickname << '\n';
    for ( const std::string & lobby : settings.lobbies ) {
        file << "lobby = " << lobby << '\n';
    }
    file << "always relay = " << toText( settings.isRelayAlways ) << '\n';
    file << "public games = " << toText( settings.isPublic ) << '\n';
    if ( !settings.isSharedLobbyUsed ) {
        file << "shared lobby = off" << '\n';
    }
    if ( settings.machineKey != 0 ) {
        file << "machine key = " << std::hex << std::setw( 16 ) << std::setfill( '0' ) << settings.machineKey << '\n';
    }
}

std::vector<std::string> Multi::getLobbyAddresses( const NetworkSettings & settings )
{
    std::vector<std::string> addresses;
    if ( settings.isSharedLobbyUsed ) {
        addresses.emplace_back( sharedLobbyAddress );
    }

    for ( const std::string & lobby : settings.lobbies ) {
        if ( std::find( addresses.begin(), addresses.end(), lobby ) == addresses.end() ) {
            addresses.push_back( lobby );
        }
    }

    return addresses;
}

uint64_t Multi::getMachineKey()
{
    NetworkSettings settings = loadNetworkSettings();
    if ( settings.machineKey != 0 ) {
        return settings.machineKey;
    }

    std::random_device device;
    std::mt19937_64 generator( ( static_cast<uint64_t>( device() ) << 32 ) ^ device()
                               ^ static_cast<uint64_t>( std::chrono::steady_clock::now().time_since_epoch().count() ) );

    while ( settings.machineKey == 0 ) {
        settings.machineKey = generator();
    }

    saveNetworkSettings( settings );
    return settings.machineKey;
}
