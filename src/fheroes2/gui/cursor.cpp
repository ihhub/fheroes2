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

/* constructor */
Cursor::Cursor()
    : theme( NONE )
    , offset_x( 0 )
    , offset_y( 0 )
{
    cursorOffsetTable.insert( std::pair<int, std::pair<s32, s32> >( Cursor::MOVE,  std::pair<s32, s32>( -12, -8 ) ) );
    cursorOffsetTable.insert( std::pair<int, std::pair<s32, s32> >( Cursor::MOVE2, std::pair<s32, s32>( cursorOffsetTable[Cursor::MOVE].first, cursorOffsetTable[Cursor::MOVE].second ) ) );
    cursorOffsetTable.insert( std::pair<int, std::pair<s32, s32> >( Cursor::MOVE3, std::pair<s32, s32>( cursorOffsetTable[Cursor::MOVE].first, cursorOffsetTable[Cursor::MOVE].second ) ) );
    cursorOffsetTable.insert( std::pair<int, std::pair<s32, s32> >( Cursor::MOVE4, std::pair<s32, s32>( cursorOffsetTable[Cursor::MOVE].first, cursorOffsetTable[Cursor::MOVE].second ) ) );

    cursorOffsetTable.insert( std::pair<int, std::pair<s32, s32> >( Cursor::ACTION,  std::pair<s32, s32>( -14, -10 ) ) );
    cursorOffsetTable.insert( std::pair<int, std::pair<s32, s32> >( Cursor::ACTION2, std::pair<s32, s32>( cursorOffsetTable[Cursor::ACTION].first, cursorOffsetTable[Cursor::ACTION].second ) ) );
    cursorOffsetTable.insert( std::pair<int, std::pair<s32, s32> >( Cursor::ACTION3, std::pair<s32, s32>( cursorOffsetTable[Cursor::ACTION].first, cursorOffsetTable[Cursor::ACTION].second ) ) );
    cursorOffsetTable.insert( std::pair<int, std::pair<s32, s32> >( Cursor::ACTION4, std::pair<s32, s32>( cursorOffsetTable[Cursor::ACTION].first, cursorOffsetTable[Cursor::ACTION].second ) ) );

    cursorOffsetTable.insert( std::pair<int, std::pair<s32, s32> >( Cursor::BOAT,     std::pair<s32, s32>( -12, -12 ) ) );
    cursorOffsetTable.insert( std::pair<int, std::pair<s32, s32> >( Cursor::BOAT2,    std::pair<s32, s32>( cursorOffsetTable[Cursor::BOAT].first, cursorOffsetTable[Cursor::BOAT].second ) ) );
    cursorOffsetTable.insert( std::pair<int, std::pair<s32, s32> >( Cursor::BOAT3,    std::pair<s32, s32>( cursorOffsetTable[Cursor::BOAT].first, cursorOffsetTable[Cursor::BOAT].second ) ) );
    cursorOffsetTable.insert( std::pair<int, std::pair<s32, s32> >( Cursor::BOAT4,    std::pair<s32, s32>( cursorOffsetTable[Cursor::BOAT].first, cursorOffsetTable[Cursor::BOAT].second ) ) );
    cursorOffsetTable.insert( std::pair<int, std::pair<s32, s32> >( Cursor::REDBOAT,  std::pair<s32, s32>( cursorOffsetTable[Cursor::BOAT].first, cursorOffsetTable[Cursor::BOAT].second ) ) );
    cursorOffsetTable.insert( std::pair<int, std::pair<s32, s32> >( Cursor::REDBOAT2, std::pair<s32, s32>( cursorOffsetTable[Cursor::BOAT].first, cursorOffsetTable[Cursor::BOAT].second ) ) );
    cursorOffsetTable.insert( std::pair<int, std::pair<s32, s32> >( Cursor::REDBOAT3, std::pair<s32, s32>( cursorOffsetTable[Cursor::BOAT].first, cursorOffsetTable[Cursor::BOAT].second ) ) );
    cursorOffsetTable.insert( std::pair<int, std::pair<s32, s32> >( Cursor::REDBOAT4, std::pair<s32, s32>( cursorOffsetTable[Cursor::BOAT].first, cursorOffsetTable[Cursor::BOAT].second ) ) );

    cursorOffsetTable.insert( std::pair<int, std::pair<s32, s32> >( Cursor::CASTLE,  std::pair<s32, s32>( -6, -4 ) ) );

    cursorOffsetTable.insert( std::pair<int, std::pair<s32, s32> >( Cursor::SCROLL_TOPRIGHT,    std::pair<s32, s32>( -15, 0 ) ) );
    cursorOffsetTable.insert( std::pair<int, std::pair<s32, s32> >( Cursor::SCROLL_RIGHT,       std::pair<s32, s32>( cursorOffsetTable[Cursor::SCROLL_TOPRIGHT].first, cursorOffsetTable[Cursor::SCROLL_TOPRIGHT].first ) ) );

    cursorOffsetTable.insert( std::pair<int, std::pair<s32, s32> >( Cursor::SCROLL_BOTTOM,      std::pair<s32, s32>( 0, -15 ) ) );
    cursorOffsetTable.insert( std::pair<int, std::pair<s32, s32> >( Cursor::SCROLL_BOTTOMLEFT,  std::pair<s32, s32>( cursorOffsetTable[Cursor::SCROLL_BOTTOM].first, cursorOffsetTable[Cursor::SCROLL_BOTTOM].first ) ) );

    cursorOffsetTable.insert( std::pair<int, std::pair<s32, s32> >( Cursor::SCROLL_BOTTOMRIGHT, std::pair<s32, s32>( -20, -20 ) ) );
    cursorOffsetTable.insert( std::pair<int, std::pair<s32, s32> >( Cursor::SWORD_BOTTOMRIGHT,  std::pair<s32, s32>( cursorOffsetTable[Cursor::SCROLL_BOTTOMRIGHT].first, cursorOffsetTable[Cursor::SCROLL_BOTTOMRIGHT].first ) ) );

    cursorOffsetTable.insert( std::pair<int, std::pair<s32, s32> >( Cursor::SWORD_BOTTOMLEFT,   std::pair<s32, s32>( -5, -20 ) ) );

    cursorOffsetTable.insert( std::pair<int, std::pair<s32, s32> >( Cursor::SWORD_TOPLEFT,      std::pair<s32, s32>( -5, -5 ) ) );

    cursorOffsetTable.insert( std::pair<int, std::pair<s32, s32> >( Cursor::SWORD_TOPRIGHT,     std::pair<s32, s32>( -20, -5 ) ) );

    cursorOffsetTable.insert( std::pair<int, std::pair<s32, s32> >( Cursor::SWORD_LEFT,         std::pair<s32, s32>( -5, -7 ) ) );

    cursorOffsetTable.insert( std::pair<int, std::pair<s32, s32> >( Cursor::SWORD_RIGHT,        std::pair<s32, s32>( -25, -7 ) ) );

    cursorOffsetTable.insert( std::pair<int, std::pair<s32, s32> >( Cursor::WAR_MOVE,           std::pair<s32, s32>( -7, -14 ) ) );
    cursorOffsetTable.insert( std::pair<int, std::pair<s32, s32> >( Cursor::WAR_FLY,            std::pair<s32, s32>( cursorOffsetTable[Cursor::WAR_MOVE].first, cursorOffsetTable[Cursor::WAR_MOVE].first ) ) );

    cursorOffsetTable.insert( std::pair<int, std::pair<s32, s32> >( Cursor::WAR_NONE,           std::pair<s32, s32>( -7, -7 ) ) );
    cursorOffsetTable.insert( std::pair<int, std::pair<s32, s32> >( Cursor::WAR_HERO,           std::pair<s32, s32>( cursorOffsetTable[Cursor::WAR_NONE].first, cursorOffsetTable[Cursor::WAR_NONE].first ) ) );
    cursorOffsetTable.insert( std::pair<int, std::pair<s32, s32> >( Cursor::WAR_ARROW,          std::pair<s32, s32>( cursorOffsetTable[Cursor::WAR_NONE].first, cursorOffsetTable[Cursor::WAR_NONE].first ) ) );
    cursorOffsetTable.insert( std::pair<int, std::pair<s32, s32> >( Cursor::WAR_INFO,           std::pair<s32, s32>( cursorOffsetTable[Cursor::WAR_NONE].first, cursorOffsetTable[Cursor::WAR_NONE].first ) ) );
    cursorOffsetTable.insert( std::pair<int, std::pair<s32, s32> >( Cursor::WAR_BROKENARROW,    std::pair<s32, s32>( cursorOffsetTable[Cursor::WAR_NONE].first, cursorOffsetTable[Cursor::WAR_NONE].first ) ) );

    std::vector<s32> cursorIds;
    cursorIds.clear();
    cursorIds.push_back( Cursor::SP_SLOW );
    cursorIds.push_back( Cursor::SP_UNKNOWN );
    cursorIds.push_back( Cursor::SP_CURSE );
    cursorIds.push_back( Cursor::SP_LIGHTNINGBOLT );
    cursorIds.push_back( Cursor::SP_CHAINLIGHTNING );
    cursorIds.push_back( Cursor::SP_CURE );
    cursorIds.push_back( Cursor::SP_BLESS );
    cursorIds.push_back( Cursor::SP_FIREBALL );
    cursorIds.push_back( Cursor::SP_FIREBLAST );
    cursorIds.push_back( Cursor::SP_TELEPORT );
    cursorIds.push_back( Cursor::SP_ELEMENTALSTORM );
    cursorIds.push_back( Cursor::SP_RESURRECT );
    cursorIds.push_back( Cursor::SP_RESURRECTTRUE );
    cursorIds.push_back( Cursor::SP_HASTE );
    cursorIds.push_back( Cursor::SP_SHIELD );
    cursorIds.push_back( Cursor::SP_ARMAGEDDON );
    cursorIds.push_back( Cursor::SP_ANTIMAGIC );
    cursorIds.push_back( Cursor::SP_DISPEL );
    cursorIds.push_back( Cursor::SP_BERSERKER );
    cursorIds.push_back( Cursor::SP_PARALYZE );
    cursorIds.push_back( Cursor::SP_BLIND );
    cursorIds.push_back( Cursor::SP_HOLYWORD );
    cursorIds.push_back( Cursor::SP_HOLYSHOUT );
    cursorIds.push_back( Cursor::SP_METEORSHOWER );
    cursorIds.push_back( Cursor::SP_ANIMATEDEAD );
    cursorIds.push_back( Cursor::SP_MIRRORIMAGE );
    cursorIds.push_back( Cursor::SP_BLOODLUST );
    cursorIds.push_back( Cursor::SP_DEATHRIPPLE );
    cursorIds.push_back( Cursor::SP_DEATHWAVE );
    cursorIds.push_back( Cursor::SP_STEELSKIN );
    cursorIds.push_back( Cursor::SP_STONESKIN );
    cursorIds.push_back( Cursor::SP_DRAGONSLAYER );
    cursorIds.push_back( Cursor::SP_EARTHQUAKE );
    cursorIds.push_back( Cursor::SP_DISRUPTINGRAY );
    cursorIds.push_back( Cursor::SP_COLDRING );
    cursorIds.push_back( Cursor::SP_COLDRAY );
    cursorIds.push_back( Cursor::SP_HYPNOTIZE );
    cursorIds.push_back( Cursor::SP_ARROW );

    for ( std::vector<s32>::iterator iter = cursorIds.begin(); iter != cursorIds.end(); ++iter ) {
        const Sprite sprite = LoadSprite( *iter );

#ifdef USE_SDL_CURSOR
        SDL_Surface * cursorSurface = sprite();
        SDL_Cursor * cursor = SDL_CreateColorCursor( cursorSurface, offset_x, offset_y );
        if ( cursor == NULL ) {
            DEBUG( DBG_ENGINE, DBG_WARN, "SDL_CreateColorCursor failure, name = " << *iter << ", reason: " << SDL_GetError() );
            continue;
        }

        cacheCursors.insert( std::pair<int, SDL_Cursor *>( *iter, cursor ) );
#else
        cacheSprites.insert( std::pair<int, const Sprite>( *iter, sprite ) );
#endif
        const s32 ox = -sprite.w() / 2;
        const s32 oy = -sprite.h() / 2;

        cursorOffsetTable.insert( std::pair<int, std::pair<s32, s32> >( *iter, std::pair<s32, s32>( ox, oy ) ) );
    }
}

/* destructor */
Cursor::~Cursor()
{
#ifdef USE_SDL_CURSOR
    for( std::map<int, SDL_Cursor *>::iterator iter = cacheCursors.begin(); iter != cacheCursors.end(); ++iter)
        SDL_FreeCursor( iter->second );
#endif
}

Cursor & Cursor::Get( void )
{
    static Cursor _cursor;
    return _cursor;
}

Sprite Cursor::LoadSprite( int id ) {
    Sprite result;

    switch ( 0xF000 & id ) {
    case 0x3000: {
        result = AGG::GetICN( ICN::SPELCO, 0xFF & id );
        DEBUG( DBG_ENGINE, DBG_TRACE, ICN::GetString( ICN::SPELCO ) << ", " << ( id & 0xFF ) );
    } break;

    case 0x2000: {
        result = AGG::GetICN( ICN::CMSECO, 0xFF & id );
        DEBUG( DBG_ENGINE, DBG_TRACE, ICN::GetString( ICN::CMSECO ) << ", " << ( id & 0xFF ) );
    } break;

    case 0x1000: {
        result = AGG::GetICN( ICN::ADVMCO, 0xFF & id );
        DEBUG( DBG_ENGINE, DBG_TRACE, ICN::GetString( ICN::ADVMCO ) << ", " << ( id & 0xFF ) );
    } break;

    default:
        result = AGG::GetICN( ICN::ADVMCO, 0 );
        break;
    }

    return result;
}

/* get theme cursor */
int Cursor::Themes( void )
{
    return SP_ARROW >= theme ? theme : NONE;
}

#ifdef USE_SDL_CURSOR
bool Cursor::isVisible() const
{
    // return SpriteMove::isVisible();
    const bool state = SDL_ShowCursor( SDL_QUERY ) == SDL_ENABLE;
    return state;
}
#endif

/* set cursor theme */
bool Cursor::SetThemes( int name, bool force )
{
#ifdef USE_SDL_CURSOR
    if ( force || theme != name ) {
        if ( isVisible() )
            SDL_ShowCursor( SDL_DISABLE );

        theme = name;

        std::map<int, SDL_Cursor *>::iterator iter = cacheCursors.find( name );
        if ( iter == cacheCursors.end() ) {
            const Sprite sprite = LoadSprite( name );
            SDL_Surface * cursorSurface = sprite();
            SetOffset( name );
            SDL_Cursor * cursor = SDL_CreateColorCursor( cursorSurface, offset_x, offset_y );
            if ( cursor == NULL ) {
                DEBUG( DBG_ENGINE, DBG_WARN, "SDL_CreateColorCursor failure, name = " << name << ", reason: " << SDL_GetError() );
                return false;
            }

            iter = cacheCursors.insert( std::pair<int, SDL_Cursor *>( name, cursor ) ).first;
        }

        SDL_SetCursor( iter->second );
        if ( SDL_ShowCursor( SDL_ENABLE ) == SDL_QUERY )
            DEBUG( DBG_ENGINE, DBG_WARN, "SDL_ShowCursor failure, name = " << name << ", reason: " << SDL_GetError() );

        SDL_ShowCursor( SDL_ENABLE );

        return true;
    }

    return false;
}
#else // SDL1 related code
    if ( force || theme != name ) {
        if ( isVisible() )
            Hide();
        theme = name;

        std::map<int, const Sprite>::iterator iter = cacheSprites.find( name );

        if ( iter == cacheSprites.end() ) {
            const Sprite sprite = LoadSprite( name );
            iter = cacheSprites.insert( std::pair<int, const Sprite>( name, sprite ) ).first;
        }

        Set( iter->second, true );

        SetOffset( name );

        return true;
    }

    return false;
}

/* redraw cursor wrapper for local event */
void Cursor::Redraw( s32 x, s32 y )
{
    Cursor & cur = Cursor::Get();

    if ( cur.isVisible() ) {
        cur.Move( x, y );

        Display::Get().Flip();
    }
}

/* move cursor */
void Cursor::Move( s32 x, s32 y )
{
    if ( isVisible() )
        SpriteMove::Move( x + offset_x, y + offset_y );
}
#endif

/* set offset big cursor */
void Cursor::SetOffset( int name )
{
    std::map<int, std::pair<s32, s32> >::const_iterator iter = cursorOffsetTable.find( name );
    if ( iter == cursorOffsetTable.end() ) {
        offset_x = iter->second.first;
        offset_y = iter->second.second;
    }
    else {
        offset_x = 0;
        offset_y = 0;
    }
}

#ifdef USE_SDL_CURSOR
void Cursor::Show( void ) {}
#else
void Cursor::Show( void )
{
    if ( !Settings::Get().ExtPocketHideCursor() )
        SpriteMove::Show();
}
#endif

#ifdef USE_SDL_CURSOR
void Cursor::Hide( void ) {}
#endif

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
