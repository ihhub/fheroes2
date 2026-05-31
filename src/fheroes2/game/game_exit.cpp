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

#include "game_exit.h"

#include "dialog.h"
#include "game_mode.h"
#include "translations.h"
#include "ui_dialog.h"

namespace Game
{
    fheroes2::GameMode processExitEvent()
    {
#if defined( __IPHONEOS__ )
        // iOS discourages to exit a running application.
        fheroes2::showStandardTextMessage( _( "Quit" ), _( "To exit fheroes2, press the Home button or swipe up." ), Dialog::OK );
#else
        if ( Dialog::YES & fheroes2::showStandardTextMessage( _( "Quit" ), _( "Are you sure you want to quit?" ), Dialog::YES | Dialog::NO ) ) {
            return fheroes2::GameMode::QUIT_GAME;
        }
#endif

        return fheroes2::GameMode::CANCEL;
    }
}
