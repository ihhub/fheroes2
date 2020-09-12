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

#include "agg.h"
#include "army_troop.h"
#include "battle_troop.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "gamedefs.h"
#include "monster.h"
#include "statusbar.h"

#ifndef BUILD_RELEASE

void TestMonsterSprite( void )
{
    // cursor
    Cursor & cursor = Cursor::Get();
    cursor.Hide();
    cursor.SetThemes( Cursor::POINTER );

    fheroes2::Display & display = fheroes2::Display::instance();

    // Monster monster(Monster::PEASANT);
    Battle::Unit troop( Troop( Monster::PEASANT, 1 ), -1, false );
    fheroes2::ImageRestorer back( display );
    Rect pos;

    LocalEvent & le = LocalEvent::Get();

    // std::string str;

    StatusBar speed_bar;
    StatusBar count_bar;
    StatusBar start_bar;
    StatusBar frame_bar;
    StatusBar info_bar;

    start_bar.SetCenter( 100, display.height() - 16 );
    count_bar.SetCenter( 200, display.height() - 16 );
    speed_bar.SetCenter( 300, display.height() - 16 );
    frame_bar.SetCenter( 400, display.height() - 16 );
    info_bar.SetCenter( 550, display.height() - 16 );

    u32 ticket = 0;

    u32 start = 0;
    u32 count = fheroes2::AGG::GetICNCount( troop.ICNFile() );
    u32 frame = 0;
    u32 speed = 100;

    frame_bar.ShowMessage( "frame: " + GetString( frame ) );
    speed_bar.ShowMessage( "speed: " + GetString( speed ) );
    start_bar.ShowMessage( "start: " + GetString( start ) );
    count_bar.ShowMessage( "count: " + GetString( count ) );

    cursor.Show();
    display.render();

    // mainmenu loop
    while ( le.HandleEvents() ) {
        if ( Game::HotKeyPressEvent( Game::EVENT_DEFAULT_EXIT ) )
            break;

        if ( le.MouseClickLeft( pos ) ) {
            u32 mons = troop.GetID();
            if ( Dialog::SelectCount( "Monster", Monster::PEASANT, Monster::WATER_ELEMENT, mons ) ) {
                cursor.Hide();
                troop.SetMonster( Monster( mons ) );
                start = 0;
                count = fheroes2::AGG::GetICNCount( troop.ICNFile() );
                frame = 0;
                cursor.Show();
                display.render();
            }
        }

        if ( le.MouseClickLeft( start_bar.GetRect() ) ) {
            u32 start2 = start;
            if ( Dialog::SelectCount( "Start", 0, fheroes2::AGG::GetICNCount( troop.ICNFile() ) - 1, start2 ) ) {
                cursor.Hide();
                start = start2;
                if ( start + count > fheroes2::AGG::GetICNCount( troop.ICNFile() ) )
                    count = fheroes2::AGG::GetICNCount( troop.ICNFile() ) - start;
                start_bar.ShowMessage( "start: " + GetString( start ) );
                cursor.Show();
                display.render();
            }
        }

        if ( le.MouseClickLeft( count_bar.GetRect() ) ) {
            u32 count2 = count;
            if ( Dialog::SelectCount( "Count", 1, fheroes2::AGG::GetICNCount( troop.ICNFile() ), count2 ) ) {
                cursor.Hide();
                count = count2;
                frame = start;
                count_bar.ShowMessage( "count: " + GetString( count ) );
                cursor.Show();
                display.render();
            }
        }

        if ( le.MouseClickLeft( speed_bar.GetRect() ) ) {
            u32 speed2 = speed;
            if ( Dialog::SelectCount( "Speed", 1, 50, speed2 ) ) {
                cursor.Hide();
                speed = speed2;
                frame = start;
                speed_bar.ShowMessage( "speed: " + GetString( speed ) );
                cursor.Show();
                display.render();
            }
        }

        if ( 0 == ( ticket % speed ) ) {
            cursor.Hide();
            const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( troop.ICNFile(), frame );
            pos.x = 320 + sprite.x();
            pos.y = 240 + sprite.y();
            pos.w = sprite.width();
            pos.h = sprite.height();
            back.restore();
            back.update( pos.x, pos.y, pos.w, pos.h );
            fheroes2::Blit( sprite, display, pos.x, pos.y );

            frame_bar.ShowMessage( "frame: " + GetString( frame ) );
            info_bar.ShowMessage( "ox: " + GetString( sprite.x() ) + ", oy: " + GetString( sprite.y() ) );

            cursor.Show();
            display.render();

            ++frame;
            if ( frame >= start + count )
                frame = start;
        }

        ++ticket;
    }
}

#endif
