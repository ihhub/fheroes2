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

#include "multi_machines.h"

#include <sstream>
#include <utility>

#include "kingdom.h"
#include "multi_lobby_protocol.h"
#include "multi_payload.h"
#include "multi_text.h"
#include "players.h"
#include "serialize.h"
#include "world.h"

namespace
{
    Multi::MachineTable & machineTable()
    {
        static Multi::MachineTable table;
        return table;
    }

    Multi::MachineInfo * findMachineInTable( const Multi::MachineId machine )
    {
        for ( Multi::MachineInfo & info : machineTable().machines ) {
            if ( info.id == machine ) {
                return &info;
            }
        }

        return nullptr;
    }

    const char * stateName( const Multi::MachineState state )
    {
        switch ( state ) {
        case Multi::MachineState::PRESENT:
            return "present";
        case Multi::MachineState::LOST:
            return "lost";
        case Multi::MachineState::LEFT:
            return "left";
        case Multi::MachineState::GONE:
            return "gone";
        default:
            break;
        }

        return "unknown";
    }
}

Multi::Message Multi::makeMachineTable( const MachineTable & table )
{
    RWStreamBuf stream;
    stream.setBigendian( true );
    stream << table.receiver << static_cast<uint8_t>( table.machines.size() );

    for ( const MachineInfo & info : table.machines ) {
        stream << info.id << info.nickname << static_cast<uint8_t>( info.ownedColors ) << static_cast<uint8_t>( info.playedColors ) << static_cast<uint8_t>( info.state )
               << info.comebacks;
    }

    return { MessageType::MACHINES, toPayload( stream ) };
}

std::optional<Multi::MachineTable> Multi::readMachineTable( const Message & message )
{
    if ( message.type != MessageType::MACHINES ) {
        return {};
    }

    ROStreamBuf stream( message.payload );
    stream.setBigendian( true );

    MachineTable table;
    uint8_t count = 0;
    stream >> table.receiver >> count;

    if ( stream.fail() || count == 0 || count > maxGuests + 1 || table.receiver > maxMachineId ) {
        return {};
    }

    PlayerColorsSet owned = 0;
    PlayerColorsSet played = 0;
    bool isReceiverFound = false;

    for ( uint8_t i = 0; i < count; ++i ) {
        MachineInfo info;
        uint8_t ownedColors = 0;
        uint8_t playedColors = 0;
        uint8_t state = 0;

        stream >> info.id;
        if ( !readString( stream, info.nickname, 64 ) ) {
            return {};
        }
        stream >> ownedColors >> playedColors >> state >> info.comebacks;

        // The host first, then the guests in order. Each color is chosen and played by one computer at most.
        const bool isInOrder = table.machines.empty() ? ( info.id == hostMachine ) : ( info.id > table.machines.back().id );
        if ( stream.fail() || !isInOrder || info.id > maxMachineId || ( !info.nickname.empty() && !isValidNickname( info.nickname ) )
             || ( ownedColors & ~Color::allPlayerColors() ) != 0 || ( playedColors & ~Color::allPlayerColors() ) != 0 || ( owned & ownedColors ) != 0
             || ( played & playedColors ) != 0 || state > static_cast<uint8_t>( MachineState::GONE ) ) {
            return {};
        }

        info.ownedColors = ownedColors;
        info.playedColors = playedColors;
        info.state = static_cast<MachineState>( state );
        owned |= ownedColors;
        played |= playedColors;
        isReceiverFound = isReceiverFound || ( info.id == table.receiver );

        table.machines.push_back( std::move( info ) );
    }

    if ( stream.size() != 0 || !isReceiverFound || owned != played ) {
        return {};
    }

    return table;
}

Multi::MachineTable Multi::setMachineTable( MachineTable table )
{
    return std::exchange( machineTable(), std::move( table ) );
}

void Multi::clearMachineTable()
{
    machineTable() = {};
}

const Multi::MachineTable & Multi::getMachineTable()
{
    return machineTable();
}

Multi::MachineId Multi::getThisMachine()
{
    return machineTable().receiver;
}

const Multi::MachineInfo * Multi::findMachine( const MachineId machine )
{
    return findMachineInTable( machine );
}

bool Multi::setMachineState( const MachineId machine, const MachineState state )
{
    MachineInfo * info = findMachineInTable( machine );
    if ( info == nullptr || info->state == state ) {
        return false;
    }

    info->state = state;
    return true;
}

void Multi::addMachineComeback( const MachineId machine )
{
    if ( MachineInfo * info = findMachineInTable( machine ); info != nullptr ) {
        ++info->comebacks;
    }
}

bool Multi::updatePlayedColors()
{
    MachineTable & table = machineTable();
    if ( table.machines.empty() || table.machines.front().id != hostMachine ) {
        return false;
    }

    MachineInfo & host = table.machines.front();
    PlayerColorsSet hostColors = host.ownedColors;
    bool isChanged = false;

    for ( MachineInfo & info : table.machines ) {
        if ( info.id == hostMachine ) {
            continue;
        }

        const PlayerColorsSet colors = ( info.state == MachineState::GONE ) ? PlayerColorsSet{ 0 } : info.ownedColors;
        if ( info.state == MachineState::GONE ) {
            hostColors |= info.ownedColors;
        }

        if ( info.playedColors != colors ) {
            info.playedColors = colors;
            isChanged = true;
        }
    }

    if ( host.playedColors != hostColors ) {
        host.playedColors = hostColors;
        isChanged = true;
    }

    return isChanged;
}

std::optional<Multi::MachineId> Multi::getMachineOfColor( const PlayerColor color )
{
    for ( const MachineInfo & info : machineTable().machines ) {
        if ( ( info.playedColors & color ) != 0 ) {
            return info.id;
        }
    }

    return {};
}

PlayerColorsSet Multi::getColorsPlayedBy( const MachineId machine )
{
    const MachineInfo * info = findMachineInTable( machine );
    return info != nullptr ? info->playedColors : PlayerColorsSet{ 0 };
}

PlayerColorsSet Multi::getAllHumanColors()
{
    PlayerColorsSet colors = 0;
    for ( const MachineInfo & info : machineTable().machines ) {
        colors |= info.playedColors;
    }

    return colors;
}

PlayerColorsSet Multi::getSeatedColors()
{
    const MachineInfo * info = findMachineInTable( getThisMachine() );
    return info != nullptr ? static_cast<PlayerColorsSet>( info->ownedColors & info->playedColors ) : PlayerColorsSet{ 0 };
}

Multi::MachineSet Multi::getOtherMachines()
{
    MachineSet machines = 0;
    for ( const MachineInfo & info : machineTable().machines ) {
        if ( info.id != getThisMachine() && info.state != MachineState::GONE ) {
            machines |= machineBit( info.id );
        }
    }

    return machines;
}

Multi::MachineSet Multi::getMachinesOfColors( const PlayerColorsSet colors )
{
    MachineSet machines = 0;
    for ( const MachineInfo & info : machineTable().machines ) {
        if ( info.id != getThisMachine() && ( info.playedColors & colors ) != 0 ) {
            machines |= machineBit( info.id );
        }
    }

    return machines;
}

std::vector<Multi::MachineId> Multi::getGuestsWithPlayers()
{
    std::vector<MachineId> guests;

    for ( const MachineInfo & info : machineTable().machines ) {
        if ( info.id == hostMachine ) {
            continue;
        }

        for ( const PlayerColor color : PlayerColorsVector( info.playedColors ) ) {
            if ( world.GetKingdom( color ).isPlay() ) {
                guests.push_back( info.id );
                break;
            }
        }
    }

    return guests;
}

std::string Multi::getMachineName( const MachineId machine )
{
    const MachineInfo * info = findMachineInTable( machine );
    if ( info == nullptr ) {
        return {};
    }

    if ( !info->nickname.empty() ) {
        return fromUtf8( info->nickname );
    }

    const PlayerColorsSet colors = info->ownedColors != 0 ? info->ownedColors : info->playedColors;
    for ( const PlayerColor color : PlayerColorsVector( colors ) ) {
        if ( const Player * player = Players::Get( color ); player != nullptr ) {
            return player->GetName();
        }
    }

    return Color::String( PlayerColor::NONE );
}

std::string Multi::describeMachines()
{
    std::ostringstream os;
    const MachineTable & table = machineTable();

    os << "this computer " << static_cast<int>( table.receiver );

    for ( const MachineInfo & info : table.machines ) {
        os << "; " << static_cast<int>( info.id ) << " " << stateName( info.state ) << ", plays " << describeColors( info.playedColors );

        if ( info.ownedColors != info.playedColors ) {
            os << " (chose " << describeColors( info.ownedColors ) << ")";
        }
    }

    return os.str();
}

std::string Multi::describeColors( const PlayerColorsSet colors )
{
    std::string text;

    for ( const PlayerColor color : PlayerColorsVector( colors ) ) {
        if ( !text.empty() ) {
            text += ' ';
        }
        text += Color::String( color );
    }

    return text.empty() ? std::string( "none" ) : text;
}
