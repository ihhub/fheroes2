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

#include "cursor.h"
#include "agg.h"
#include "settings.h"
#include "sprite.h"

Cursor::Cursor()
    : theme( NONE )
    , offset_x( 0 )
    , offset_y( 0 )
#if defined( USE_SDL_CURSOR )
    , _cursorSDL( NULL )
    , _isVisibleCursor( false )
#endif
{}

Cursor::~Cursor()
{
    _free();
}

void Cursor::_free()
{
#if defined( USE_SDL_CURSOR )
    if ( _cursorSDL != NULL ) {
        SDL_FreeCursor( _cursorSDL );
        _cursorSDL = NULL;
    }
#endif
}

Cursor & Cursor::Get( void )
{
    static Cursor _cursor;
    return _cursor;
}

/* get theme cursor */
int Cursor::Themes( void )
{
    return SP_ARROW >= theme ? theme : NONE;
}

/* set cursor theme */
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
        default:
            break;
        }
        const Sprite spr = AGG::GetICN( icnID, 0xFF & name );
        SetOffset( name, Point( spr.w() / 2, spr.h() / 2 ) );
        Set( spr, true );
#if defined( USE_SDL_CURSOR )
        SDL_Cursor * tempCursor = SDL_CreateColorCursor( surface, -offset_x, -offset_y );
        if ( tempCursor == NULL ) {
            DEBUG( DBG_ENGINE, DBG_WARN, "SDL_CreateColorCursor failure, name = " << name << ", reason: " << SDL_GetError() );
        }
        SDL_SetCursor( tempCursor );
        SDL_ShowCursor( 1 );
        _free();
        std::swap( tempCursor, _cursorSDL );
#endif

        // immediately apply new offset, force
        const Point currentPos = LocalEvent::Get().GetMouseCursor();
        Move( currentPos.x, currentPos.y );
        return true;
    }

    return false;
}

/* redraw cursor wrapper for local event */
void Cursor::Redraw( s32 x, s32 y )
{
#if !defined( USE_SDL_CURSOR )
    Cursor & cur = Cursor::Get();

    if ( cur.isVisible() ) {
        cur.Move( x, y );

        Display::Get().Flip();
    }
#endif
}

/* move cursor */
void Cursor::Move( s32 x, s32 y )
{
#if defined( USE_SDL_CURSOR )
    background.SetPos( Point( x, y ) );
#else
    if ( isVisible() )
        SpriteMove::Move( x + offset_x, y + offset_y );
#endif
}

/* set offset big cursor */
void Cursor::SetOffset( int name, const Point & defaultOffset )
{
    switch ( name ) {
    case Cursor::POINTER:
    case Cursor::POINTER2:
    case Cursor::WAR_POINTER:
    case Cursor::FIGHT:
    case Cursor::FIGHT2:
    case Cursor::FIGHT3:
    case Cursor::FIGHT4:
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

void Cursor::Show( void )
{
#if defined( USE_SDL_CURSOR )
    _isVisibleCursor = true;
#else
    if ( !Settings::Get().ExtPocketHideCursor() )
        SpriteMove::Show();
#endif
}

void Cursor::Hide( void )
{
#if defined( USE_SDL_CURSOR )
    _isVisibleCursor = false;
#else
    SpriteMove::Hide();
#endif
}

bool Cursor::isVisible( void ) const
{
#if defined( USE_SDL_CURSOR )
    return ( SDL_ShowCursor( -1 ) == 1 ) && _isVisibleCursor;
#else
    return SpriteMove::isVisible();
#endif
}

int Cursor::DistanceThemes( int theme, u32 dist )
{
    if ( 0 == dist )
        return POINTER;
    else if ( dist > 4 )
        dist = 4;

    switch ( theme ) {
    case MOVE:
    case FIGHT:
    case BOAT:
    case ANCHOR:
    case CHANGE:
    case ACTION:
        return theme + 6 * ( dist - 1 );

    case REDBOAT:
        return REDBOAT + dist - 1;

    default:
        break;
    }

    return theme;
}

int Cursor::WithoutDistanceThemes( int theme )
{
    switch ( theme ) {
    case MOVE2:
    case MOVE3:
    case MOVE4:
        return MOVE;
    case FIGHT2:
    case FIGHT3:
    case FIGHT4:
        return FIGHT;
    case BOAT2:
    case BOAT3:
    case BOAT4:
        return BOAT;
    case ANCHOR2:
    case ANCHOR3:
    case ANCHOR4:
        return ANCHOR;
    case CHANGE2:
    case CHANGE3:
    case CHANGE4:
        return CHANGE;
    case ACTION2:
    case ACTION3:
    case ACTION4:
        return ACTION;
    case REDBOAT2:
    case REDBOAT3:
    case REDBOAT4:
        return REDBOAT;

    default:
        break;
    }

    return theme;
}
