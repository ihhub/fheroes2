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

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "color.h"
#include "multi_protocol.h"

// The computers of a network game and the human colors each one plays. The host keeps the table and sends it to every
// guest (MACHINES) at the start and whenever it changes.
namespace Multi
{
    enum class MachineState : uint8_t
    {
        PRESENT = 0,
        // The connection is cut, the game waits for it to come back.
        LOST = 1,
        // It left on purpose. The game waits for the host to go on without it.
        LEFT = 2,
        // The AI of the host plays its players, until it comes back.
        GONE = 3
    };

    struct MachineInfo
    {
        MachineId id{ hostMachine };
        // UTF-8, may be empty.
        std::string nickname;
        // The human colors its players chose.
        PlayerColorsSet ownedColors{ 0 };
        // The human colors it plays now. They change only between two turns, or two days of simultaneous turns.
        PlayerColorsSet playedColors{ 0 };
        MachineState state{ MachineState::PRESENT };
        // How many times it came back after a restart.
        uint16_t comebacks{ 0 };
    };

    struct MachineTable
    {
        // The computer which gets the table.
        MachineId receiver{ hostMachine };
        // The host first, then the guests by number.
        std::vector<MachineInfo> machines;
    };

    Message makeMachineTable( const MachineTable & table );
    std::optional<MachineTable> readMachineTable( const Message & message );

    // The table of the current network game, empty without one. Returns the previous one.
    MachineTable setMachineTable( MachineTable table );
    void clearMachineTable();
    const MachineTable & getMachineTable();

    MachineId getThisMachine();

    // Nothing if the computer is not in the table.
    const MachineInfo * findMachine( const MachineId machine );

    // Host only. Returns false if nothing changed.
    bool setMachineState( const MachineId machine, const MachineState state );

    // Host only. The computer came back after a restart.
    void addMachineComeback( const MachineId machine );

    // Host only, between two turns or two days: a gone computer's players are played by the host, a computer which is
    // back plays its players again. Returns false if nothing changed.
    bool updatePlayedColors();

    // The computer playing this human color now, nothing for an AI player.
    std::optional<MachineId> getMachineOfColor( const PlayerColor color );

    PlayerColorsSet getColorsPlayedBy( const MachineId machine );

    // Every human color of the game, played by any computer.
    PlayerColorsSet getAllHumanColors();

    // The players sitting at this computer: chosen here and played here.
    PlayerColorsSet getSeatedColors();

    // The other computers which are still in the game, gone ones excluded.
    MachineSet getOtherMachines();

    // The computers playing these colors now, this one excluded.
    MachineSet getMachinesOfColors( const PlayerColorsSet colors );

    // The guests playing human colors which are still in the game.
    std::vector<MachineId> getGuestsWithPlayers();

    // In the code page of the game: the nickname, or the colors of its players.
    std::string getMachineName( const MachineId machine );

    // For logs.
    std::string describeMachines();
    std::string describeColors( const PlayerColorsSet colors );
}
