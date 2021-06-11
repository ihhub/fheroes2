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

#include <cassert>

#include "cursor.h"
#include "agg_image.h"
#include "icn.h"
#include "localevent.h"
#include "screen.h"

Cursor::Cursor()
    : theme( NONE )
    , offset_x( 0 )
    , offset_y( 0 )
{}

Cursor & Cursor::Get( void )
{
    static Cursor _cursor;
    return _cursor;
}

int Cursor::Themes() const
{
    assert( theme <= CURSOR_HERO_BOAT_ACTION_8 );
    return theme;
}

bool Cursor::SetThemes( int name, bool force )
{
    if ( force || theme != name ) {
        theme = name;

        int icnID = ICN::ADVMCO;
        switch ( 0xF000 & name ) {
        case 0x3000:
            icnID = ICN::SPELCO;
            break;
        case 0x2000:
            icnID = ICN::CMSECO;
            break;
        case 0x4000:
            icnID = ICN::CURSOR_ADVENTURE_MAP;
            break;
        default:
            break;
        }
        const fheroes2::Sprite & spr = fheroes2::AGG::GetICN( icnID, 0xFF & name );
        SetOffset( name, fheroes2::Point( ( spr.width() - spr.x() ) / 2, ( spr.height() - spr.y() ) / 2 ) );
        fheroes2::cursor().update( spr, -offset_x, -offset_y );

        // immediately apply new offset, force
        const fheroes2::Point & currentPos = LocalEvent::Get().GetMouseCursor();
        Move( currentPos.x, currentPos.y );
        return true;
    }

    return false;
}

void Cursor::Redraw( int32_t x, int32_t y )
{
    if ( fheroes2::cursor().isSoftwareEmulation() ) {
        Cursor::Get().Move( x, y );
        if ( fheroes2::cursor().isVisible() ) {
            fheroes2::Display::instance().render( fheroes2::Rect( x, y, 1, 1 ) );
        }
    }
}

void Cursor::Move( int32_t x, int32_t y ) const
{
    fheroes2::cursor().setPosition( x + offset_x, y + offset_y );
}

void Cursor::SetOffset( int name, const fheroes2::Point & defaultOffset )
{
    switch ( name ) {
    case Cursor::POINTER:
    case Cursor::POINTER_VIDEO:
    case Cursor::WAR_POINTER:
    case Cursor::CURSOR_HERO_FIGHT:
    case Cursor::CURSOR_HERO_FIGHT_2:
    case Cursor::CURSOR_HERO_FIGHT_3:
    case Cursor::CURSOR_HERO_FIGHT_4:
    case Cursor::CURSOR_HERO_FIGHT_5:
    case Cursor::CURSOR_HERO_FIGHT_6:
    case Cursor::CURSOR_HERO_FIGHT_7:
    case Cursor::CURSOR_HERO_FIGHT_8:
        offset_x = 0;
        offset_y = 0;
        break;

    case Cursor::SCROLL_TOPRIGHT:
    case Cursor::SCROLL_RIGHT:
        offset_x = -15;
        offset_y = 0;
        break;

    case Cursor::SCROLL_BOTTOM:
    case Cursor::SCROLL_BOTTOMLEFT:
        offset_x = 0;
        offset_y = -15;
        break;

    case Cursor::SCROLL_BOTTOMRIGHT:
    case Cursor::SWORD_BOTTOMRIGHT:
        offset_x = -20;
        offset_y = -20;
        break;

    case Cursor::SWORD_BOTTOMLEFT:
        offset_x = -5;
        offset_y = -20;
        break;

    case Cursor::SWORD_TOPLEFT:
        offset_x = -5;
        offset_y = -5;
        break;

    case Cursor::SWORD_TOPRIGHT:
        offset_x = -20;
        offset_y = -5;
        break;

    case Cursor::SWORD_LEFT:
        offset_x = -5;
        offset_y = -7;
        break;

    case Cursor::SWORD_RIGHT:
        offset_x = -25;
        offset_y = -7;
        break;

    default:
        offset_x = -defaultOffset.x;
        offset_y = -defaultOffset.y;
        break;
    }
}

void Cursor::setVideoPlaybackCursor()
{
    if ( fheroes2::cursor().isSoftwareEmulation() ) {
        SetThemes( Cursor::POINTER_VIDEO );
    }
}

void Cursor::resetVideoPlaybackCursor()
{
    if ( fheroes2::cursor().isSoftwareEmulation() ) {
        SetThemes( Cursor::POINTER );
    }
}

void Cursor::Refresh()
{
    Get().SetThemes( Get().Themes(), true );
}

int Cursor::DistanceThemes( const int theme, uint32_t distance )
{
    if ( 0 == distance )
        return POINTER;
    else if ( distance > 8 )
        distance = 8;

    switch ( theme ) {
    case CURSOR_HERO_MOVE:
    case CURSOR_HERO_FIGHT:
    case CURSOR_HERO_BOAT:
    case CURSOR_HERO_ANCHOR:
    case CURSOR_HERO_MEET:
    case CURSOR_HERO_ACTION:
    case CURSOR_HERO_BOAT_ACTION:
        return theme + distance - 1;
    default:
        break;
    }

    return theme;
}

int Cursor::WithoutDistanceThemes( const int theme )
{
    if ( theme > CURSOR_HERO_MOVE && theme <= CURSOR_HERO_MOVE_8 ) {
        return CURSOR_HERO_MOVE;
    }
    if ( theme > CURSOR_HERO_FIGHT && theme <= CURSOR_HERO_FIGHT_8 ) {
        return CURSOR_HERO_FIGHT;
    }
    if ( theme > CURSOR_HERO_BOAT && theme <= CURSOR_HERO_BOAT_8 ) {
        return CURSOR_HERO_BOAT;
    }
    if ( theme > CURSOR_HERO_ANCHOR && theme <= CURSOR_HERO_ANCHOR_8 ) {
        return CURSOR_HERO_ANCHOR;
    }
    if ( theme > CURSOR_HERO_MEET && theme <= CURSOR_HERO_MEET_8 ) {
        return CURSOR_HERO_MEET;
    }
    if ( theme > CURSOR_HERO_ACTION && theme <= CURSOR_HERO_ACTION_8 ) {
        return CURSOR_HERO_ACTION;
    }
    if ( theme > CURSOR_HERO_BOAT_ACTION && theme <= CURSOR_HERO_BOAT_ACTION_8 ) {
        return CURSOR_HERO_BOAT_ACTION;
    }

    return theme;
}

CursorRestorer::CursorRestorer()
    : _theme( Cursor::Get().Themes() )
    , _visible( fheroes2::cursor().isVisible() )
{}

CursorRestorer::CursorRestorer( const bool visible, const int theme )
    : CursorRestorer()
{
    Cursor::Get().SetThemes( theme );

    fheroes2::cursor().show( visible );
}

CursorRestorer::~CursorRestorer()
{
    Cursor & cursor = Cursor::Get();

    if ( fheroes2::cursor().isVisible() != _visible || cursor.Themes() != _theme ) {
        cursor.SetThemes( _theme );

        fheroes2::cursor().show( _visible );

        // immediately render cursor area in case of software emulated cursor
        if ( fheroes2::cursor().isSoftwareEmulation() ) {
            const fheroes2::Point & pos = LocalEvent::Get().GetMouseCursor();

            fheroes2::Display::instance().render( fheroes2::Rect( pos.x, pos.y, 1, 1 ) );
        }
    }
}
