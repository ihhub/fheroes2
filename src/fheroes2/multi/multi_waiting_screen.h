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

#include <memory>
#include <string>

#include "game_mode.h"
#include "multi_lobby.h"

namespace fheroes2
{
    class StandardWindow;
}

namespace Multi
{
    // Shown while waiting for the network. Esc leaves it.
    class WaitingScreen
    {
    public:
        explicit WaitingScreen( std::string message );
        WaitingScreen( const WaitingScreen & ) = delete;

        ~WaitingScreen();

        WaitingScreen & operator=( const WaitingScreen & ) = delete;

        // Handles the events of one frame. Returns false once the player chose to leave.
        bool keepWaiting();

        KeepWaiting callback();

        void setMessage( std::string message );

        bool isQuitRequested() const
        {
            return _isQuitRequested;
        }

        // Returns true if the player left on purpose.
        bool isLeftByPlayer() const
        {
            return _isQuitRequested || _isCancelled;
        }

        fheroes2::GameMode modeAfterLeaving() const
        {
            return _isQuitRequested ? fheroes2::GameMode::QUIT_GAME : fheroes2::GameMode::MAIN_MENU;
        }

    private:
        void _draw();

        std::string _message;
        std::unique_ptr<fheroes2::StandardWindow> _window;
        bool _isQuitRequested{ false };
        bool _isCancelled{ false };
    };

    // The header is translated, the reason is an error in English translated part by part.
    void showNetworkError( const std::string & header, const std::string & reason );
}
