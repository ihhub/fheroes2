/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
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

#include "game.h"
#include "agg.h"
#include "cursor.h"
#include "dialog.h"
#include "game_io.h"
#include "gamedefs.h"
#include "mus.h"
#include "screen.h"
#include "settings.h"

int Game::LoadCampain( void )
{
    VERBOSE( "Load Campain Game: under construction." );
    return Game::LOADGAME;
}

int Game::LoadMulti( void )
{
    VERBOSE( "Load Multi Game: under construction." );
    return Game::LOADGAME;
}

int Game::LoadGame( void )
{
    return LOADSTANDARD;
}

int Game::LoadStandard( void )
{
    Mixer::Pause();
    AGG::PlayMusic( MUS::MAINMENU );
    // cursor
    Cursor & cursor = Cursor::Get();
    cursor.Hide();
    cursor.SetThemes( cursor.POINTER );

    fheroes2::Display & display = fheroes2::Display::instance();

    // image background
    fheroes2::Copy( fheroes2::AGG::GetICN( ICN::HEROES, 0 ), display );

    cursor.Show();
    display.render();

    std::string file = Dialog::SelectFileLoad();
    if ( file.empty() || !Game::Load( file ) )
        return MAINMENU;

    return STARTGAME;
}
