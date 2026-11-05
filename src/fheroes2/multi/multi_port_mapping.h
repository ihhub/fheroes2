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
#include <memory>
#include <string>

namespace Multi
{
    // Opens a port of the internet box with UPnP. Nothing is blocking, update() must be called once per frame.
    // The port is closed again on destruction.
    class PortMapping
    {
    public:
        enum class State
        {
            IDLE,
            WORKING,
            OPEN,
            FAILED
        };

        PortMapping();
        PortMapping( const PortMapping & ) = delete;

        ~PortMapping();

        PortMapping & operator=( const PortMapping & ) = delete;

        // Looks for the box on the local network.
        void start( const uint16_t port );

        // Used by self-tests. Skips the search for the box.
        void startWithDescription( const uint16_t port, const std::string & descriptionUrl );

        void update();

        State state() const;

        // Returns the internet address of the box once the port is open.
        const std::string & externalAddress() const;

        // Returns true if the box has a private internet address (4G box, address shared between customers): it cannot be reached.
        bool isBehindAnotherNetwork() const;

        // In English, translated part by part with translateError().
        const std::string & error() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> _impl;
    };
}
