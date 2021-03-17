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
#include <cassert>
#include <functional>

#include "agg_image.h"
#include "cursor.h"
#include "dialog.h"
#include "dialog_selectitems.h"
#include "game.h"
#include "heroes_base.h"
#include "icn.h"
#include "skill.h"
#include "spell_book.h"
#include "text.h"

namespace
{
    const int spellsPerPage = 6;

    const fheroes2::Point bookmarkInfoOffset( 123, 273 );
    const fheroes2::Point bookmarkAdvOffset( 266, 269 );
    const fheroes2::Point bookmarkCombatoOffset( 299, 276 );
    const fheroes2::Point bookmarkCloseOffset( 416, 280 );

    bool SpellBookSortingSpell( const Spell & spell1, const Spell & spell2 )
    {
        return strcmp( spell1.GetName(), spell2.GetName() ) < 0;
    }
}

void SpellBookRedrawLists( const SpellStorage &, Rects &, size_t, const fheroes2::Point &, u32, SpellBook::Filter only, const HeroBase & hero );
void SpellBookRedrawSpells( const SpellStorage & spells, Rects & coords, const size_t index, int32_t px, int32_t py, const HeroBase & hero, bool isRight = false );
void SpellBookRedrawMP( const fheroes2::Point &, u32 );

Spell SpellBook::Open( const HeroBase & hero, const Filter displayableSpells, bool canCastSpell,
                       std::function<void( const std::string & )> * statusCallback /*= nullptr*/ ) const
{
    if ( !hero.HaveSpellBook() ) {
        Dialog::Message( "", _( "No spell to cast." ), Font::BIG, Dialog::OK );
        return Spell::NONE;
    }

    Filter currentFilter = displayableSpells == Filter::ALL ? Filter::ADVN : displayableSpells;
    SpellStorage displayedSpells = SetFilter( currentFilter, &hero );

    if ( canCastSpell && displayedSpells.empty() ) {
        Dialog::Message( "", _( "No spell to cast." ), Font::BIG, Dialog::OK );
        return Spell::NONE;
    }

    fheroes2::Display & display = fheroes2::Display::instance();
    Cursor & cursor = Cursor::Get();

    const int oldcursor = cursor.Themes();

    const fheroes2::Sprite & bookPage = fheroes2::AGG::GetICN( ICN::BOOK, 0 );

    size_t current_index = 0;

    cursor.Hide();
    cursor.SetThemes( Cursor::POINTER );

    const fheroes2::Sprite & bookmark_info = fheroes2::AGG::GetICN( ICN::BOOK, 6 );
    const fheroes2::Sprite & bookmark_advn = fheroes2::AGG::GetICN( ICN::BOOK, 3 );
    const fheroes2::Sprite & bookmark_cmbt = fheroes2::AGG::GetICN( ICN::BOOK, 4 );
    const fheroes2::Sprite & bookmark_clos = fheroes2::AGG::GetICN( ICN::BOOK, 5 );

    const fheroes2::Rect pos( ( display.width() - ( bookPage.width() * 2 ) ) / 2, ( display.height() - bookPage.height() ) / 2, bookPage.width() * 2,
                              bookPage.height() + 70 );
    fheroes2::ImageRestorer restorer( display, pos.x, pos.y, pos.width, pos.height );

    const fheroes2::Rect prev_list( pos.x + 30, pos.y + 8, 30, 25 );
    const fheroes2::Rect next_list( pos.x + 410, pos.y + 8, 30, 25 );

    const fheroes2::Rect info_rt( pos.x + bookmarkInfoOffset.x, pos.y + bookmarkInfoOffset.y, bookmark_info.width(), bookmark_info.height() );
    const fheroes2::Rect advn_rt( pos.x + bookmarkAdvOffset.x, pos.y + bookmarkAdvOffset.y, bookmark_advn.width(), bookmark_advn.height() );
    const fheroes2::Rect cmbt_rt( pos.x + bookmarkCombatoOffset.x, pos.y + bookmarkCombatoOffset.y, bookmark_cmbt.width(), bookmark_cmbt.height() );
    const fheroes2::Rect clos_rt( pos.x + bookmarkCloseOffset.x, pos.y + bookmarkCloseOffset.y, bookmark_clos.width(), bookmark_clos.height() );

    Spell curspell = Spell::NONE;

    Rects coords;
    coords.reserve( spellsPerPage * 2 );

    SpellBookRedrawLists( displayedSpells, coords, current_index, fheroes2::Point( pos.x, pos.y ), hero.GetSpellPoints(), displayableSpells, hero );
    bool redraw = false;

    cursor.Show();
    display.render();

    LocalEvent & le = LocalEvent::Get();

    // message loop
    while ( le.HandleEvents() ) {
        if ( ( le.MouseClickLeft( prev_list ) || HotKeyPressEvent( Game::EVENT_MOVELEFT ) ) && current_index != 0 ) {
            current_index -= spellsPerPage * 2;
            redraw = true;
        }
        else if ( ( le.MouseClickLeft( next_list ) || HotKeyPressEvent( Game::EVENT_MOVERIGHT ) )
                  && displayedSpells.size() > ( current_index + ( spellsPerPage * 2 ) ) ) {
            current_index += spellsPerPage * 2;
            redraw = true;
        }
        else if ( le.MouseClickLeft( info_rt ) ) {
            std::string str = _( "Your hero has %{point} spell points remaining." );
            StringReplace( str, "%{point}", hero.GetSpellPoints() );
            Dialog::Message( "", str, Font::BIG, Dialog::OK );
        }
        else if ( le.MouseClickLeft( advn_rt ) && currentFilter != Filter::ADVN && displayableSpells != Filter::CMBT ) {
            currentFilter = Filter::ADVN;
            current_index = 0;
            displayedSpells = SetFilter( currentFilter, &hero );
            redraw = true;
        }
        else if ( le.MouseClickLeft( cmbt_rt ) && currentFilter != Filter::CMBT && displayableSpells != Filter::ADVN ) {
            currentFilter = Filter::CMBT;
            current_index = 0;
            displayedSpells = SetFilter( currentFilter, &hero );
            redraw = true;
        }
        else if ( le.MousePressRight( info_rt ) ) {
            std::string str = _( "Your hero has %{point} spell points remaining." );
            StringReplace( str, "%{point}", hero.GetSpellPoints() );
            Dialog::Message( "", str, Font::BIG );
        }
        else if ( le.MousePressRight( advn_rt ) && displayableSpells != Filter::CMBT ) {
            const std::string str = _( "View Adventure Spells" );
            Dialog::Message( "", str, Font::BIG );
        }
        else if ( le.MousePressRight( cmbt_rt ) && displayableSpells != Filter::ADVN ) {
            const std::string str = _( "View Combat Spells" );
            Dialog::Message( "", str, Font::BIG );
        }
        else if ( le.MousePressRight( prev_list ) ) {
            Dialog::Message( "", _( "View previous page" ), Font::BIG );
        }
        else if ( le.MousePressRight( next_list ) ) {
            Dialog::Message( "", _( "View next page" ), Font::BIG );
        }
        else if ( le.MouseClickLeft( clos_rt ) || Game::HotKeyPressEvent( Game::EVENT_DEFAULT_EXIT ) )
            break;
        else if ( le.MouseClickLeft( pos ) ) {
            const s32 index = coords.GetIndex( le.GetMouseCursor() );

            if ( 0 <= index ) {
                SpellStorage::const_iterator spell = displayedSpells.begin() + ( index + current_index );

                if ( spell < displayedSpells.end() ) {
                    if ( canCastSpell ) {
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

        if ( statusCallback != nullptr ) {
            if ( le.MouseCursor( info_rt ) ) {
                std::string str = _( "Your hero has %{point} spell points remaining." );
                StringReplace( str, "%{point}", hero.GetSpellPoints() );
                ( *statusCallback )( str );
            }
            else if ( le.MouseCursor( advn_rt ) && displayableSpells != Filter::CMBT ) {
                ( *statusCallback )( _( "View Adventure Spells" ) );
            }
            else if ( le.MouseCursor( cmbt_rt ) && displayableSpells != Filter::ADVN ) {
                ( *statusCallback )( _( "View Combat Spells" ) );
            }
            else if ( le.MouseCursor( prev_list ) ) {
                ( *statusCallback )( _( "View previous page" ) );
            }
            else if ( le.MouseCursor( next_list ) ) {
                ( *statusCallback )( _( "View next page" ) );
            }
            else if ( le.MouseCursor( clos_rt ) ) {
                ( *statusCallback )( _( "Close Spellbook" ) );
            }
            else if ( le.MouseCursor( pos ) ) {
                const int32_t index = coords.GetIndex( le.GetMouseCursor() );

                if ( 0 <= index && index + current_index < displayedSpells.size() ) {
                    const Spell & spell = displayedSpells[index + current_index];
                    std::string str = _( "View %{spell}" );
                    StringReplace( str, "%{spell}", spell.GetName() );
                    ( *statusCallback )( str );
                }
                else if ( displayableSpells == Filter::CMBT ) {
                    ( *statusCallback )( _( "View Combat Spells" ) );
                }
                else if ( displayableSpells == Filter::ADVN ) {
                    ( *statusCallback )( _( "View Adventure Spells" ) );
                }
                else {
                    ( *statusCallback )( _( "View Spells" ) );
                }
            }
        }

        if ( le.MousePressRight( pos ) ) {
            const s32 index = coords.GetIndex( le.GetMouseCursor() );

            if ( 0 <= index ) {
                SpellStorage::const_iterator spell = displayedSpells.begin() + ( index + current_index );
                if ( spell < displayedSpells.end() ) {
                    cursor.Hide();
                    Dialog::SpellInfo( *spell, false );
                    cursor.Show();
                    display.render();
                }
            }
        }

        if ( redraw ) {
            cursor.Hide();
            SpellBookRedrawLists( displayedSpells, coords, current_index, fheroes2::Point( pos.x, pos.y ), hero.GetSpellPoints(), displayableSpells, hero );
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
    SpellStorage displayedSpells = SetFilter( Filter::ALL, &hero );

    cursor.Hide();
    cursor.SetThemes( Cursor::POINTER );

    const fheroes2::Sprite & bookmark_clos = fheroes2::AGG::GetICN( ICN::BOOK, 5 );

    const fheroes2::Sprite & bookPage = fheroes2::AGG::GetICN( ICN::BOOK, 0 );
    const fheroes2::Rect pos( ( display.width() - ( bookPage.width() * 2 ) ) / 2, ( display.height() - bookPage.height() ) / 2, bookPage.width() * 2,
                              bookPage.height() + 70 );
    fheroes2::ImageRestorer back( display, pos.x, pos.y, pos.width, pos.height );

    const fheroes2::Rect prev_list( pos.x + 30, pos.y + 8, 30, 25 );
    const fheroes2::Rect next_list( pos.x + 410, pos.y + 8, 30, 25 );
    const fheroes2::Rect clos_rt( pos.x + 420, pos.y + 284, bookmark_clos.width(), bookmark_clos.height() );

    Rects coords;
    coords.reserve( spellsPerPage * 2 );

    SpellBookRedrawLists( displayedSpells, coords, current_index, fheroes2::Point( pos.x, pos.y ), hero.GetSpellPoints(), Filter::ALL, hero );
    bool redraw = false;

    cursor.Show();
    display.render();

    LocalEvent & le = LocalEvent::Get();

    // message loop
    while ( le.HandleEvents() ) {
        if ( le.MouseClickLeft( prev_list ) && current_index ) {
            current_index -= spellsPerPage * 2;
            redraw = true;
        }
        else if ( le.MouseClickLeft( next_list ) && size() > ( current_index + spellsPerPage * 2 ) ) {
            current_index += spellsPerPage * 2;
            redraw = true;
        }
        else if ( le.MouseClickLeft( clos_rt ) || Game::HotKeyPressEvent( Game::EVENT_DEFAULT_EXIT ) )
            break;
        else if ( le.MouseClickLeft( pos ) ) {
            const s32 index = coords.GetIndex( le.GetMouseCursor() );

            if ( 0 <= index ) {
                SpellStorage::const_iterator spell = displayedSpells.begin() + ( index + current_index );

                if ( spell < displayedSpells.end() ) {
                    Dialog::SpellInfo( *spell, true );
                    redraw = true;
                }
            }
            else {
                Spell spell = Dialog::SelectSpell();
                displayedSpells.Append( spell );
                Append( spell );
                redraw = true;
            }
        }

        if ( le.MousePressRight( pos ) ) {
            const s32 index = coords.GetIndex( le.GetMouseCursor() );

            if ( 0 <= index ) {
                SpellStorage::const_iterator spell = displayedSpells.begin() + ( index + current_index );

                if ( spell < displayedSpells.end() ) {
                    Dialog::SpellInfo( *spell, false );
                    redraw = true;
                }
            }
        }

        if ( redraw ) {
            cursor.Hide();
            SpellBookRedrawLists( displayedSpells, coords, current_index, fheroes2::Point( pos.x, pos.y ), hero.GetSpellPoints(), Filter::ALL, hero );
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

SpellStorage SpellBook::SetFilter( const Filter filter, const HeroBase * hero ) const
{
    SpellStorage res( *this );

    // add heroes spell scrolls
    if ( hero != nullptr )
        res.Append( hero->GetBagArtifacts() );

    if ( filter != SpellBook::Filter::ALL ) {
        res.resize( std::distance( res.begin(), std::remove_if( res.begin(), res.end(), [filter]( const Spell & s ) {
                                       return ( ( SpellBook::Filter::ADVN == filter ) && s.isCombat() ) || ( ( SpellBook::Filter::CMBT == filter ) && !s.isCombat() );
                                   } ) ) );
    }

    // check on water: disable portal spells
    if ( hero != nullptr && hero->Modes( Heroes::SHIPMASTER ) ) {
        SpellStorage::iterator itend = res.end();
        itend = std::remove( res.begin(), itend, Spell( Spell::TOWNGATE ) );
        itend = std::remove( res.begin(), itend, Spell( Spell::TOWNPORTAL ) );
        if ( res.end() != itend )
            res.resize( std::distance( res.begin(), itend ) );
    }

    // sorting results
    std::sort( res.begin(), res.end(), SpellBookSortingSpell );

    return res;
}

void SpellBookRedrawMP( const fheroes2::Point & dst, u32 mp )
{
    fheroes2::Point tp( dst.x + 11, dst.y + 9 );
    if ( mp > 999 ) {
        mp = 999; // just in case of broken code
    }

    Text text( mp >= 100 ? std::to_string( mp / 100 ) : " ", Font::SMALL );
    text.Blit( tp.x - text.w() / 2, tp.y );
    tp.y += text.h();

    text.Set( mp >= 10 ? std::to_string( ( mp % 100 ) / 10 ) : " ", Font::SMALL );
    text.Blit( tp.x - text.w() / 2, tp.y );
    tp.y += text.h();

    text.Set( mp > 0 ? std::to_string( mp % 10 ) : "0", Font::SMALL );
    text.Blit( tp.x - text.w() / 2, tp.y );
}

void SpellBookRedrawLists( const SpellStorage & spells, Rects & coords, const size_t index, const fheroes2::Point & pt, u32 sp, const SpellBook::Filter displayableSpells,
                           const HeroBase & hero )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    const fheroes2::Sprite & bookPage = fheroes2::AGG::GetICN( ICN::BOOK, 0 );
    const fheroes2::Sprite & bookmark_info = fheroes2::AGG::GetICN( ICN::BOOK, 6 );
    const fheroes2::Sprite & bookmark_advn = fheroes2::AGG::GetICN( ICN::BOOK, 3 );
    const fheroes2::Sprite & bookmark_cmbt = fheroes2::AGG::GetICN( ICN::BOOK, 4 );
    const fheroes2::Sprite & bookmark_clos = fheroes2::AGG::GetICN( ICN::BOOK, 5 );

    const fheroes2::Rect info_rt( pt.x + bookmarkInfoOffset.x, pt.y + bookmarkInfoOffset.y, bookmark_info.width(), bookmark_info.height() );

    fheroes2::Blit( bookPage, display, pt.x, pt.y, true );
    fheroes2::Blit( bookPage, display, pt.x + bookPage.width(), pt.y );

    fheroes2::Blit( bookmark_info, display, info_rt.x, info_rt.y );

    if ( displayableSpells != SpellBook::Filter::CMBT )
        fheroes2::Blit( bookmark_advn, display, pt.x + bookmarkAdvOffset.x, pt.y + bookmarkAdvOffset.y );
    if ( displayableSpells != SpellBook::Filter::ADVN )
        fheroes2::Blit( bookmark_cmbt, display, pt.x + bookmarkCombatoOffset.x, pt.y + bookmarkCombatoOffset.y );

    fheroes2::Blit( bookmark_clos, display, pt.x + bookmarkCloseOffset.x, pt.y + bookmarkCloseOffset.y );

    coords.clear();

    SpellBookRedrawMP( fheroes2::Point( info_rt.x, info_rt.y ), sp );
    SpellBookRedrawSpells( spells, coords, index, pt.x, pt.y, hero );
    SpellBookRedrawSpells( spells, coords, index + spellsPerPage, pt.x + 220, pt.y, hero, true );
}

void SpellBookRedrawSpells( const SpellStorage & spells, Rects & coords, const size_t index, int32_t px, int32_t py, const HeroBase & hero, bool isRight )
{
    const uint32_t heroSpellPoints = hero.GetSpellPoints();

    for ( int32_t i = 0; i < spellsPerPage; ++i ) {
        if ( spells.size() <= index + i )
            return;

        const int32_t ox = 84 + 81 * ( i & 1 );
        const int32_t oy = 71 + 78 * ( i >> 1 ) - ( ( i + isRight ) % 2 ) * 5;

        const Spell & spell = spells[i + index];
        const std::string & spellName = spell.GetName();
        const uint32_t spellCost = spell.SpellPoint( &hero );
        const bool isAvailable = heroSpellPoints >= spellCost;

        const fheroes2::Sprite & icon = fheroes2::AGG::GetICN( ICN::SPELLS, spell.IndexSprite() );
        int vertOffset = 49 - icon.height();
        if ( vertOffset > 6 )
            vertOffset = 6;
        const fheroes2::Rect rect( px + ox - ( icon.width() + icon.width() % 2 ) / 2, py + oy - icon.height() - vertOffset + 2, icon.width(), icon.height() + 10 );
        fheroes2::Blit( icon, fheroes2::Display::instance(), rect.x, rect.y );

        TextBox box( spellName, Font::SMALL, 80 );
        box.Set( spellName + ( box.row() == 1 ? '\n' : ' ' ) + '[' + std::to_string( spellCost ) + ']', isAvailable ? Font::SMALL : Font::GRAY_SMALL, 80 );
        box.Blit( px + ox - 40, py + oy );

        coords.push_back( rect );
    }
}
