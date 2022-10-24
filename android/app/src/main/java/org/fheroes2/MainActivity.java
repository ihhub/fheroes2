/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2022                                                    *
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

package org.fheroes2;

import org.libsdl.app.SDLActivity;

public class MainActivity extends SDLActivity
{
    @Override
    protected void onDestroy()
    {
        super.onDestroy();

        // TODO: When SDL_main() exits, the Android app can still remain in memory, and restarting it using Launcher may result in
        // TODO: the following errors during SDL reinitialization:
        // TODO:
        // TODO: Fatal signal 11 (SIGSEGV), code 1 (SEGV_MAPERR), fault addr 0x38 in tid 4397 (SDLThread), pid 4295 (SDLActivity)
        // TODO:
        // TODO: This workaround terminates the whole app when this Activity exits, allowing SDL to initialize normally on startup
        System.exit( 0 );
    }
}
