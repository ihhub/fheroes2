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

#include <algorithm>
#include <functional>

#include "agg.h"
#include "cursor.h"
#include "dialog.h"
#include "dialog_selectitems.h"
#include "game.h"
#include "heroes_base.h"
#include "skill.h"
#include "spell_book.h"
#include "text.h"

#define SPELL_PER_PAGE 6

namespace
{
    const fheroes2::Point bookmarkInfoOffset( 123, 273 );
    const fheroes2::Point bookmarkAdvOffset( 266, 269 );
    const fheroes2::Point bookmarkCombatoOffset( 299, 276 );
    const fheroes2::Point bookmarkCloseOffset( 416, 280 );
}

struct SpellFiltered : std::binary_function<Spell, int, bool>
{
    bool operator()( Spell s, int f ) const
    {
        return ( ( SpellBook::ADVN & f ) && s.isCombat() ) || ( ( SpellBook::CMBT & f ) && !s.isCombat() );
    }
};

void SpellBookRedrawLists( const SpellStorage &, Rects &, size_t, const Point &, u32, int only, const HeroBase & hero );
void SpellBookRedrawSpells( const SpellStorage &, Rects &, size_t, s32, s32, const HeroBase & hero );
void SpellBookRedrawMP( const Point &, u32 );

bool SpellBookSortingSpell( const Spell & spell1, const Spell & spell2 )
{
    if ( spell1.isCombat() == spell2.isCombat() )
        return std::string( spell1.GetName() ) < std::string( spell2.GetName() );
    return spell1.isCombat();
}

Spell SpellBook::Open( const HeroBase & hero, int filt, bool canselect ) const
{
    if ( !hero.HaveSpellBook() ) {
        Dialog::Message( "", _( "No spell to cast." ), Font::BIG, Dialog::OK );
        return Spell( Spell::NONE );
    }

    fheroes2::Display & display = fheroes2::Display::instance();
    Cursor & cursor = Cursor::Get();

    const int oldcursor = cursor.Themes();

    const fheroes2::Sprite & bookPage = fheroes2::AGG::GetICN( ICN::BOOK, 0 );

    int filter = ( filt == ALL ) ? ADVN : filt;
    SpellStorage spells2 = SetFilter( filter, &hero );

    if ( canselect && spells2.empty() ) {
        Dialog::Message( "", _( "No spell to cast." ), Font::BIG, Dialog::OK );
        return Spell::NONE;
    }

    // sorting results
    std::sort( spells2.begin(), spells2.end(), SpellBookSortingSpell );

    size_t current_index = 0;

    cursor.Hide();
    cursor.SetThemes( Cursor::POINTER );

    const fheroes2::Sprite & bookmark_info = fheroes2::AGG::GetICN( ICN::BOOK, 6 );
    const fheroes2::Sprite & bookmark_advn = fheroes2::AGG::GetICN( ICN::BOOK, 3 );
    const fheroes2::Sprite & bookmark_cmbt = fheroes2::AGG::GetICN( ICN::BOOK, 4 );
    const fheroes2::Sprite & bookmark_clos = fheroes2::AGG::GetICN( ICN::BOOK, 5 );

    const Rect pos( ( display.width() - ( bookPage.width() * 2 ) ) / 2, ( display.height() - bookPage.height() ) / 2, bookPage.width() * 2, bookPage.height() + 70 );
    fheroes2::ImageRestorer restorer( display, pos.x, pos.y, pos.w, pos.h );

    const Rect prev_list( pos.x + 30, pos.y + 8, 30, 25 );
    const Rect next_list( pos.x + 410, pos.y + 8, 30, 25 );

    const Rect info_rt( pos.x + bookmarkInfoOffset.x, pos.y + bookmarkInfoOffset.y, bookmark_info.width(), bookmark_info.height() );
    const Rect advn_rt( pos.x + bookmarkAdvOffset.x, pos.y + bookmarkAdvOffset.y, bookmark_advn.width(), bookmark_advn.height() );
    const Rect cmbt_rt( pos.x + bookmarkCombatoOffset.x, pos.y + bookmarkCombatoOffset.y, bookmark_cmbt.width(), bookmark_cmbt.height() );
    const Rect clos_rt( pos.x + bookmarkCloseOffset.x, pos.y + bookmarkCloseOffset.y, bookmark_clos.width(), bookmark_clos.height() );

    Spell curspell( Spell::NONE );

    Rects coords;
    coords.reserve( SPELL_PER_PAGE * 2 );

    SpellBookRedrawLists( spells2, coords, current_index, pos, hero.GetSpellPoints(), filt, hero );
    bool redraw = false;

    cursor.Show();
    display.render();

    LocalEvent & le = LocalEvent::Get();

    // message loop
    while ( le.HandleEvents() ) {
        if ( ( le.MouseClickLeft( prev_list ) || HotKeyPressEvent( Game::EVENT_MOVELEFT ) ) && current_index ) {
            current_index -= SPELL_PER_PAGE * 2;
            redraw = true;
        }
        else if ( ( le.MouseClickLeft( next_list ) || HotKeyPressEvent( Game::EVENT_MOVERIGHT ) ) && spells2.size() > ( current_index + ( SPELL_PER_PAGE * 2 ) ) ) {
            current_index += SPELL_PER_PAGE * 2;
            redraw = true;
        }
        else if ( le.MouseClickLeft( info_rt ) ) {
            std::string str = _( "Your hero has %{point} spell points remaining." );
            StringReplace( str, "%{point}", hero.GetSpellPoints() );
            Dialog::Message( "", str, Font::BIG, Dialog::OK );
        }
        else if ( le.MouseClickLeft( advn_rt ) && filter != ADVN && filt != CMBT ) {
            filter = ADVN;
            current_index = 0;
            spells2 = SetFilter( filter, &hero );
            redraw = true;
        }
        else if ( le.MouseClickLeft( cmbt_rt ) && filter != CMBT && filt != ADVN ) {
            filter = CMBT;
            current_index = 0;
            spells2 = SetFilter( filter, &hero );
            redraw = true;
        }
        else if ( le.MousePressRight( info_rt ) ) {
            std::string str = _( "Your hero has %{point} spell points remaining." );
            StringReplace( str, "%{point}", hero.GetSpellPoints() );
            Dialog::Message( "", str, Font::BIG );
        }
        else if ( le.MousePressRight( advn_rt ) ) {
            const std::string str = _( "View Adventure Spells" );
            Dialog::Message( "", str, Font::BIG );
        }
        else if ( le.MousePressRight( cmbt_rt ) ) {
            const std::string str = _( "View Combat Spells" );
            Dialog::Message( "", str, Font::BIG );
        }
        else if ( le.MouseClickLeft( clos_rt ) || Game::HotKeyPressEvent( Game::EVENT_DEFAULT_EXIT ) )
            break;
        else if ( le.MouseClickLeft( pos ) ) {
            const s32 index = coords.GetIndex( le.GetMouseCursor() );

            if ( 0 <= index ) {
                SpellStorage::const_iterator spell = spells2.begin() + ( index + current_index );

                if ( spell < spells2.end() ) {
                    if ( canselect ) {
                        std::string str;
                        if ( hero.CanCastSpell( *spell, &str ) ) {
                            curspell = *spell;
                            break;
                        }
                        else {
                            cursor.Hide();
                            StringReplace( str, "%{mana}", ( *spell ).SpellPoint( &hero ) );
                            StringReplace( str, "%{point}", hero.GetSpellPoints() );
                            Dialog::Message( "", str, Font::BIG, Dialog::OK );
                            cursor.Show();
                            display.render();
                        }
                    }
                    else {
                        cursor.Hide();
                        Dialog::SpellInfo( *spell, true );
                        cursor.Show();
                        display.render();
                    }
                }
            }
        }

        if ( le.MousePressRight( pos ) ) {
            const s32 index = coords.GetIndex( le.GetMouseCursor() );

            if ( 0 <= index ) {
                SpellStorage::const_iterator spell = spells2.begin() + ( index + current_index );
                if ( spell < spells2.end() ) {
                    cursor.Hide();
                    Dialog::SpellInfo( *spell, false );
                    cursor.Show();
                    display.render();
                }
            }
        }

        if ( redraw ) {
            cursor.Hide();
            SpellBookRedrawLists( spells2, coords, current_index, pos, hero.GetSpellPoints(), filt, hero );
            cursor.Show();
            display.render();
            redraw = false;
        }
    }

    cursor.Hide();
    restorer.restore();
    cursor.SetThemes( oldcursor );
    cursor.Show();
    display.render();

    return curspell;
}

void SpellBook::Edit( const HeroBase & hero )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    Cursor & cursor = Cursor::Get();

    const int oldcursor = cursor.Themes();

    size_t current_index = 0;
    SpellStorage spells2 = SetFilter( SpellBook::ALL, &hero );

    cursor.Hide();
    cursor.SetThemes( Cursor::POINTER );

    const fheroes2::Sprite & bookmark_clos = fheroes2::AGG::GetICN( ICN::BOOK, 5 );

    const fheroes2::Sprite & bookPage = fheroes2::AGG::GetICN( ICN::BOOK, 0 );
    const Rect pos( ( display.width() - ( bookPage.width() * 2 ) ) / 2, ( display.height() - bookPage.height() ) / 2, bookPage.width() * 2, bookPage.height() + 70 );
    fheroes2::ImageRestorer back( display, pos.x, pos.y, pos.w, pos.h );

    const Rect prev_list( pos.x + 30, pos.y + 8, 30, 25 );
    const Rect next_list( pos.x + 410, pos.y + 8, 30, 25 );
    const Rect clos_rt( pos.x + 420, pos.y + 284, bookmark_clos.width(), bookmark_clos.height() );

    Rects coords;
    coords.reserve( SPELL_PER_PAGE * 2 );

    SpellBookRedrawLists( spells2, coords, current_index, pos, hero.GetSpellPoints(), SpellBook::ALL, hero );
    bool redraw = false;

    cursor.Show();
    display.render();

    LocalEvent & le = LocalEvent::Get();

    // message loop
    while ( le.HandleEvents() ) {
        if ( le.MouseClickLeft( prev_list ) && current_index ) {
            current_index -= SPELL_PER_PAGE * 2;
            redraw = true;
        }
        else if ( le.MouseClickLeft( next_list ) && size() > ( current_index + SPELL_PER_PAGE * 2 ) ) {
            current_index += SPELL_PER_PAGE * 2;
            redraw = true;
        }
        else if ( le.MouseClickLeft( clos_rt ) || Game::HotKeyPressEvent( Game::EVENT_DEFAULT_EXIT ) )
            break;
        else if ( le.MouseClickLeft( pos ) ) {
            const s32 index = coords.GetIndex( le.GetMouseCursor() );

            if ( 0 <= index ) {
                SpellStorage::const_iterator spell = spells2.begin() + ( index + current_index );

                if ( spell < spells2.end() ) {
                    Dialog::SpellInfo( *spell, true );
                    redraw = true;
                }
            }
            else {
                Spell spell = Dialog::SelectSpell();
                spells2.Append( spell );
                Append( spell );
                redraw = true;
            }
        }

        if ( le.MousePressRight( pos ) ) {
            const s32 index = coords.GetIndex( le.GetMouseCursor() );

            if ( 0 <= index ) {
                SpellStorage::const_iterator spell = spells2.begin() + ( index + current_index );

                if ( spell < spells2.end() ) {
                    Dialog::SpellInfo( *spell, false );
                    redraw = true;
                }
            }
        }

        if ( redraw ) {
            cursor.Hide();
            SpellBookRedrawLists( spells2, coords, current_index, pos, hero.GetSpellPoints(), SpellBook::ALL, hero );
            cursor.Show();
            display.render();
            redraw = false;
        }
    }

    cursor.Hide();
    back.restore();
    cursor.SetThemes( oldcursor );
    cursor.Show();
    display.render();
}

SpellStorage SpellBook::SetFilter( int filter, const HeroBase * hero ) const
{
    SpellStorage res( *this );

    // added heroes spell scrolls
    if ( hero )
        res.Append( hero->GetBagArtifacts() );

    if ( filter != SpellBook::ALL ) {
        res.resize( std::distance( res.begin(), std::remove_if( res.begin(), res.end(), std::bind2nd( SpellFiltered(), filter ) ) ) );
    }

    // check on water: disable portal spells
    if ( hero && hero->Modes( Heroes::SHIPMASTER ) ) {
        SpellStorage::iterator itend = res.end();
        itend = std::remove( res.begin(), itend, Spell( Spell::TOWNGATE ) );
        itend = std::remove( res.begin(), itend, Spell( Spell::TOWNPORTAL ) );
        if ( res.end() != itend )
            res.resize( std::distance( res.begin(), itend ) );
    }

    return res;
}

void SpellBookRedrawMP( const Point & dst, u32 mp )
{
    Point tp( dst.x + 11, dst.y + 9 );
    if ( mp > 999 ) {
        mp = 999; // just in case of broken code
    }

    Text text( mp > 100 ? GetString( mp / 100 ) : " ", Font::SMALL );
    text.Blit( tp.x - text.w() / 2, tp.y );
    tp.y += text.h();

    text.Set( mp > 10 ? GetString( ( mp % 100 ) / 10 ) : " ", Font::SMALL );
    text.Blit( tp.x - text.w() / 2, tp.y );
    tp.y += text.h();

    text.Set( mp > 0 ? GetString( mp % 10 ) : "0", Font::SMALL );
    text.Blit( tp.x - text.w() / 2, tp.y );
}

void SpellBookRedrawLists( const SpellStorage & spells, Rects & coords, const size_t cur, const Point & pt, u32 sp, int only, const HeroBase & hero )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    const fheroes2::Sprite & bookPage = fheroes2::AGG::GetICN( ICN::BOOK, 0 );
    const fheroes2::Sprite & bookmark_info = fheroes2::AGG::GetICN( ICN::BOOK, 6 );
    const fheroes2::Sprite & bookmark_advn = fheroes2::AGG::GetICN( ICN::BOOK, 3 );
    const fheroes2::Sprite & bookmark_cmbt = fheroes2::AGG::GetICN( ICN::BOOK, 4 );
    const fheroes2::Sprite & bookmark_clos = fheroes2::AGG::GetICN( ICN::BOOK, 5 );

    const Rect info_rt( pt.x + bookmarkInfoOffset.x, pt.y + bookmarkInfoOffset.y, bookmark_info.width(), bookmark_info.height() );

    fheroes2::Blit( bookPage, display, pt.x, pt.y, true );
    fheroes2::Blit( bookPage, display, pt.x + bookPage.width(), pt.y );

    fheroes2::Blit( bookmark_info, display, info_rt.x, info_rt.y );

    if ( SpellBook::CMBT != only )
        fheroes2::Blit( bookmark_advn, display, pt.x + bookmarkAdvOffset.x, pt.y + bookmarkAdvOffset.y );
    if ( SpellBook::ADVN != only )
        fheroes2::Blit( bookmark_cmbt, display, pt.x + bookmarkCombatoOffset.x, pt.y + bookmarkCombatoOffset.y );

    fheroes2::Blit( bookmark_clos, display, pt.x + bookmarkCloseOffset.x, pt.y + bookmarkCloseOffset.y );

    if ( coords.size() )
        coords.clear();

    SpellBookRedrawMP( info_rt, sp );
    SpellBookRedrawSpells( spells, coords, cur, pt.x, pt.y, hero );
    SpellBookRedrawSpells( spells, coords, cur + SPELL_PER_PAGE, pt.x + 220, pt.y, hero );
}

void SpellBookRedrawSpells( const SpellStorage & spells, Rects & coords, const size_t cur, s32 px, s32 py, const HeroBase & hero )
{
    s32 ox = 0;
    s32 oy = 0;

    const uint32_t heroSpellPoints = hero.GetSpellPoints();

    for ( u32 ii = 0; ii < SPELL_PER_PAGE; ++ii )
        if ( spells.size() > cur + ii ) {
            if ( 0 == ( ii % ( SPELL_PER_PAGE / 2 ) ) ) {
                oy = 50;
                ox += 80;
            }

            const Spell & spell = spells[ii + cur];
            const fheroes2::Sprite & icon = fheroes2::AGG::GetICN( ICN::SPELLS, spell.IndexSprite() );
            const Rect rect( px + ox - icon.width() / 2, py + oy - icon.height() / 2, icon.width(), icon.height() + 10 );
            fheroes2::Blit( icon, fheroes2::Display::instance(), rect.x, rect.y );

            const uint32_t spellCost = spell.SpellPoint( &hero );
            const bool isAvailable = heroSpellPoints >= spellCost;

            TextBox box( std::string( spell.GetName() ) + " [" + GetString( spellCost ) + "]", isAvailable ? Font::SMALL : Font::GRAY_SMALL, 80 );
            box.Blit( px + ox - 40, py + oy + 25 );

            oy += 80;

            coords.push_back( rect );
        }
}
