/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2025                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "spell_book.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <iterator>
#include <set>
#include <vector>

#include "agg_image.h"
#include "cursor.h"
#include "dialog.h"
#include "dialog_selectitems.h"
#include "game_hotkeys.h"
#include "heroes_base.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "math_base.h"
#include "math_tools.h"
#include "screen.h"
#include "tools.h"
#include "translations.h"
#include "ui_dialog.h"
#include "ui_text.h"

namespace
{
    const size_t spellsPerPage = 6;

    const fheroes2::Point bookmarkInfoOffset( 123, 273 );
    const fheroes2::Point bookmarkAdvOffset( 266, 269 );
    const fheroes2::Point bookmarkCombatoOffset( 299, 276 );
    const fheroes2::Point bookmarkCloseOffset( 416, 280 );

    const fheroes2::Point spellBookShadow( -16, 16 );

    fheroes2::Size getSpellBookSize( const SpellBook::Filter displayableSpells )
    {
        const fheroes2::Sprite & bookPage = fheroes2::AGG::GetICN( ICN::BOOK, 0 );
        const fheroes2::Sprite & bookmark_info = fheroes2::AGG::GetICN( ICN::BOOK, 6 );
        const fheroes2::Sprite & bookmark_advn = fheroes2::AGG::GetICN( ICN::BOOK, 3 );
        const fheroes2::Sprite & bookmark_cmbt = fheroes2::AGG::GetICN( ICN::BOOK, 4 );
        const fheroes2::Sprite & bookmark_clos = fheroes2::AGG::GetICN( ICN::BOOK, 5 );

        const bool isAdventureTabPresent = displayableSpells != SpellBook::Filter::CMBT;
        const bool isCombatTabPresent = displayableSpells != SpellBook::Filter::ADVN;

        int32_t maximumHeight = bookmarkInfoOffset.y + bookmark_info.height();
        if ( isAdventureTabPresent && maximumHeight < bookmarkAdvOffset.y + bookmark_advn.height() ) {
            maximumHeight = bookmarkAdvOffset.y + bookmark_advn.height();
        }
        if ( isCombatTabPresent && maximumHeight < bookmarkCombatoOffset.y + bookmark_cmbt.height() ) {
            maximumHeight = bookmarkCombatoOffset.y + bookmark_cmbt.height();
        }
        if ( maximumHeight < bookmarkCloseOffset.y + bookmark_clos.height() ) {
            maximumHeight = bookmarkCloseOffset.y + bookmark_clos.height();
        }

        return { bookPage.width() * 2, maximumHeight };
    }

    void SpellBookRedrawSpells( const SpellStorage & spells, std::vector<fheroes2::Rect> & coords, const size_t index, const int32_t px, const int32_t py,
                                const HeroBase & hero, const bool canCastSpell, const bool isRight, fheroes2::Image & output, const fheroes2::Point & outputOffset )
    {
        for ( size_t i = 0; i < spellsPerPage; ++i ) {
            if ( spells.size() <= index + i ) {
                return;
            }

            const int32_t posX = static_cast<int32_t>( i % 2 );
            const int32_t ox = 84 + 81 * posX;
            // Make spell near left and right edge of book 5 pixels lower.
            const int32_t extraOffsetY = ( ( posX == 1 ) == isRight ) ? 0 : 5;
            const int32_t oy = 71 + 78 * static_cast<int32_t>( i / 2 ) - extraOffsetY;

            const Spell & spell = spells[i + index];
            const std::string & spellName = spell.GetName();
            const uint32_t spellCost = spell.spellPoints( &hero );
            // If casting spells is prohibited in principle, it makes no sense to check whether this hero can cast them and highlight them in gray if not
            const bool isAvailable = !canCastSpell || hero.CanCastSpell( spell );

            const fheroes2::Sprite & icon = fheroes2::AGG::GetICN( ICN::SPELLS, spell.IndexSprite() );
            const int32_t vertOffset = std::min( 6, 49 - icon.height() );
            fheroes2::Rect rect( px + ox - ( icon.width() + icon.width() % 2 ) / 2, py + oy - icon.height() - vertOffset + 2, icon.width(), icon.height() + 10 );
            fheroes2::Blit( icon, output, rect.x, rect.y );

            const int32_t maxTextWidth = 80;
            const int32_t rowCount = fheroes2::Text{ spellName, fheroes2::FontType::smallWhite() }.rows( maxTextWidth );

            const fheroes2::Text text( spellName + ( rowCount == 1 ? '\n' : ' ' ) + '[' + std::to_string( spellCost ) + ']',
                                       isAvailable ? fheroes2::FontType::smallWhite() : fheroes2::FontType{ fheroes2::FontSize::SMALL, fheroes2::FontColor::GRAY } );
            text.draw( px + ox - 40, py + oy + 2, maxTextWidth, output );

            rect.x += outputOffset.x;
            rect.y += outputOffset.y;
            coords.push_back( rect );
        }
    }

    void SpellBookRedrawManaPoints( const fheroes2::Point & dst, uint32_t manaPoints, fheroes2::Image & output )
    {
        fheroes2::Point tp( dst.x + 11, dst.y + 11 );
        if ( manaPoints > 999 ) {
            manaPoints = 999; // just in case of a broken code
        }

        fheroes2::Text text( manaPoints >= 100 ? std::to_string( manaPoints / 100 ) : " ", fheroes2::FontType::smallWhite() );
        text.draw( tp.x - text.width() / 2, tp.y, output );
        tp.y += text.height();

        text.set( manaPoints >= 10 ? std::to_string( ( manaPoints % 100 ) / 10 ) : " ", fheroes2::FontType::smallWhite() );
        text.draw( tp.x - text.width() / 2, tp.y, output );
        tp.y += text.height();

        text.set( manaPoints > 0 ? std::to_string( manaPoints % 10 ) : "0", fheroes2::FontType::smallWhite() );
        text.draw( tp.x - text.width() / 2, tp.y, output );
    }

    void spellBookRedrawLists( const SpellStorage & spells, std::vector<fheroes2::Rect> & coords, const size_t index, const fheroes2::Point & pt,
                               const uint32_t spellPoints, const SpellBook::Filter displayableSpells, const HeroBase & hero, const bool canCastSpell )
    {
        const fheroes2::Size spellBookSize = getSpellBookSize( displayableSpells );

        fheroes2::Sprite output( spellBookSize.width, spellBookSize.height );
        output.reset();

        const fheroes2::Sprite & bookPage = fheroes2::AGG::GetICN( ICN::BOOK, 0 );

        // Left page.
        fheroes2::Blit( bookPage, 0, 0, output, 0, 0, bookPage.width(), bookPage.height(), true );
        if ( index == 0 ) {
            const fheroes2::Sprite & straightCorner = fheroes2::AGG::GetICN( ICN::BOOK, 7 );
            fheroes2::Blit( straightCorner, 0, 0, output, bookPage.width() - straightCorner.x() - straightCorner.width(), straightCorner.y(), straightCorner.width(),
                            straightCorner.height(), true );
        }

        // Right page.
        fheroes2::Blit( bookPage, 0, 0, output, bookPage.width(), 0, bookPage.width(), bookPage.height() );
        if ( spells.size() <= ( index + ( spellsPerPage * 2 ) ) ) {
            const fheroes2::Sprite & straightCorner = fheroes2::AGG::GetICN( ICN::BOOK, 7 );
            fheroes2::Blit( straightCorner, 0, 0, output, bookPage.width() + straightCorner.x(), straightCorner.y(), straightCorner.width(), straightCorner.height() );
        }

        const fheroes2::Sprite & bookmarkSpellPoints = fheroes2::AGG::GetICN( ICN::BOOK, 6 );
        fheroes2::Blit( bookmarkSpellPoints, output, bookmarkInfoOffset.x, bookmarkInfoOffset.y );

        if ( displayableSpells != SpellBook::Filter::CMBT ) {
            const fheroes2::Sprite & bookmarkAdvantureSpells = fheroes2::AGG::GetICN( ICN::BOOK, 3 );
            fheroes2::Blit( bookmarkAdvantureSpells, output, bookmarkAdvOffset.x, bookmarkAdvOffset.y );
        }
        if ( displayableSpells != SpellBook::Filter::ADVN ) {
            const fheroes2::Sprite & bookmarkCombatSpells = fheroes2::AGG::GetICN( ICN::BOOK, 4 );
            fheroes2::Blit( bookmarkCombatSpells, output, bookmarkCombatoOffset.x, bookmarkCombatoOffset.y );
        }

        const fheroes2::Sprite & bookmarkClose = fheroes2::AGG::GetICN( ICN::BOOK, 5 );
        fheroes2::Blit( bookmarkClose, output, bookmarkCloseOffset.x, bookmarkCloseOffset.y );

        SpellBookRedrawManaPoints( bookmarkInfoOffset, spellPoints, output );

        coords.clear();
        SpellBookRedrawSpells( spells, coords, index, 0, 0, hero, canCastSpell, false, output, pt );
        SpellBookRedrawSpells( spells, coords, index + spellsPerPage, 220, 0, hero, canCastSpell, true, output, pt );

        output = fheroes2::addShadow( output, spellBookShadow, 3 );

        fheroes2::Blit( output, 0, 0, fheroes2::Display::instance(), pt.x + spellBookShadow.x, pt.y, output.width(), output.height() );
    }
}

Spell SpellBook::Open( const HeroBase & hero, const Filter displayableSpells, const bool canCastSpell, const bool restorePreviousState,
                       const std::function<void( const std::string & )> & statusCallback ) const
{
    if ( !hero.HaveSpellBook() ) {
        fheroes2::showStandardTextMessage( "", _( "You have no Magic Book, so you cannot cast a spell." ), Dialog::OK );
        return Spell::NONE;
    }

    if ( displayableSpells == Filter::ALL ) {
        _spellFilter = Filter::ADVN;
        _startSpellIndex = 0;
    }
    else if ( !restorePreviousState || _spellFilter != displayableSpells ) {
        _spellFilter = displayableSpells;
        _startSpellIndex = 0;
    }

    SpellStorage displayedSpells = SetFilter( _spellFilter, &hero );

    if ( canCastSpell && displayedSpells.empty() ) {
        fheroes2::showStandardTextMessage( "", _( "No spell to cast." ), Dialog::OK );
        return Spell::NONE;
    }

    if ( _startSpellIndex >= displayedSpells.size() ) {
        _startSpellIndex = 0;
    }

    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    const fheroes2::Sprite & bookPage = fheroes2::AGG::GetICN( ICN::BOOK, 0 );

    const fheroes2::Sprite & bookmark_info = fheroes2::AGG::GetICN( ICN::BOOK, 6 );
    const fheroes2::Sprite & bookmark_advn = fheroes2::AGG::GetICN( ICN::BOOK, 3 );
    const fheroes2::Sprite & bookmark_cmbt = fheroes2::AGG::GetICN( ICN::BOOK, 4 );
    const fheroes2::Sprite & bookmark_clos = fheroes2::AGG::GetICN( ICN::BOOK, 5 );

    const fheroes2::Size spellBookSize = getSpellBookSize( displayableSpells );

    fheroes2::Display & display = fheroes2::Display::instance();
    const fheroes2::Rect pos( ( display.width() - ( bookPage.width() * 2 ) ) / 2, ( display.height() - bookPage.height() ) / 2, spellBookSize.width,
                              spellBookSize.height );

    fheroes2::ImageRestorer restorer( display, pos.x + spellBookShadow.x, pos.y, pos.width - spellBookShadow.x, pos.height + spellBookShadow.y );

    const fheroes2::Rect previousPageRoi( pos.x + 30, pos.y + 8, 30, 25 );
    const fheroes2::Rect nextPageRoi( pos.x + 410, pos.y + 8, 30, 25 );

    const fheroes2::Rect spellPointsRoi( pos.x + bookmarkInfoOffset.x, pos.y + bookmarkInfoOffset.y, bookmark_info.width(), bookmark_info.height() );
    const fheroes2::Rect adventureSpellsRoi( pos.x + bookmarkAdvOffset.x, pos.y + bookmarkAdvOffset.y, bookmark_advn.width(), bookmark_advn.height() );
    const fheroes2::Rect combatSpellsRoi( pos.x + bookmarkCombatoOffset.x, pos.y + bookmarkCombatoOffset.y, bookmark_cmbt.width(), bookmark_cmbt.height() );
    const fheroes2::Rect closeRoi( pos.x + bookmarkCloseOffset.x, pos.y + bookmarkCloseOffset.y, bookmark_clos.width(), bookmark_clos.height() );

    Spell curspell = Spell::NONE;

    std::vector<fheroes2::Rect> coords;
    coords.reserve( spellsPerPage * 2 );

    spellBookRedrawLists( displayedSpells, coords, _startSpellIndex, pos.getPosition(), hero.GetSpellPoints(), displayableSpells, hero, canCastSpell );
    bool redraw = false;

    display.render( restorer.rect() );

    const auto getHeroSpellPointsInfo = [&hero]() {
        std::string result = _( "Your hero has %{sp} spell points remaining out of %{max}." );

        StringReplace( result, "%{sp}", hero.GetSpellPoints() );
        StringReplace( result, "%{max}", hero.GetMaxSpellPoints() );

        return result;
    };

    LocalEvent & le = LocalEvent::Get();

    while ( le.HandleEvents() ) {
        if ( !_isFirstPage() && ( le.MouseClickLeft( previousPageRoi ) || HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_LEFT ) ) ) {
            _startSpellIndex -= spellsPerPage * 2;
            redraw = true;
        }
        else if ( !_isLastPage( displayedSpells.size(), spellsPerPage * 2 )
                  && ( le.MouseClickLeft( nextPageRoi ) || HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_RIGHT ) ) ) {
            _startSpellIndex += spellsPerPage * 2;
            redraw = true;
        }
        else if ( le.MouseClickLeft( spellPointsRoi ) ) {
            fheroes2::showStandardTextMessage( "", getHeroSpellPointsInfo(), Dialog::OK );
        }
        else if ( le.MouseClickLeft( adventureSpellsRoi ) && _spellFilter != Filter::ADVN && displayableSpells != Filter::CMBT ) {
            _spellFilter = Filter::ADVN;
            _startSpellIndex = 0;
            displayedSpells = SetFilter( _spellFilter, &hero );
            redraw = true;
        }
        else if ( le.MouseClickLeft( combatSpellsRoi ) && _spellFilter != Filter::CMBT && displayableSpells != Filter::ADVN ) {
            _spellFilter = Filter::CMBT;
            _startSpellIndex = 0;
            displayedSpells = SetFilter( _spellFilter, &hero );
            redraw = true;
        }
        else if ( le.isMouseRightButtonPressedInArea( spellPointsRoi ) ) {
            fheroes2::showStandardTextMessage( "", getHeroSpellPointsInfo(), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( adventureSpellsRoi ) && displayableSpells != Filter::CMBT ) {
            fheroes2::showStandardTextMessage( "", _( "View Adventure Spells" ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( combatSpellsRoi ) && displayableSpells != Filter::ADVN ) {
            fheroes2::showStandardTextMessage( "", _( "View Combat Spells" ), Dialog::ZERO );
        }
        else if ( !_isFirstPage() && le.isMouseRightButtonPressedInArea( previousPageRoi ) ) {
            fheroes2::showStandardTextMessage( "", _( "View previous page" ), Dialog::ZERO );
        }
        else if ( !_isLastPage( displayedSpells.size(), spellsPerPage * 2 ) && le.isMouseRightButtonPressedInArea( nextPageRoi ) ) {
            fheroes2::showStandardTextMessage( "", _( "View next page" ), Dialog::ZERO );
        }
        else if ( le.MouseClickLeft( closeRoi ) || Game::HotKeyCloseWindow() ) {
            break;
        }
        else if ( le.MouseClickLeft( pos ) ) {
            const int32_t index = GetRectIndex( coords, le.getMouseCursorPos() );

            if ( 0 <= index ) {
                const SpellStorage::const_iterator spell = displayedSpells.begin() + ( index + _startSpellIndex );

                if ( spell < displayedSpells.end() ) {
                    if ( canCastSpell ) {
                        std::string str;

                        if ( hero.CanCastSpell( *spell, &str ) ) {
                            curspell = *spell;
                            break;
                        }
                        fheroes2::showStandardTextMessage( spell->GetName(), str, Dialog::OK );
                        display.render();
                    }
                    else {
                        fheroes2::SpellDialogElement( *spell, &hero ).showPopup( Dialog::OK );
                        display.render();
                    }
                }
            }
        }

        if ( statusCallback ) {
            if ( le.isMouseCursorPosInArea( spellPointsRoi ) ) {
                statusCallback( getHeroSpellPointsInfo() );
            }
            else if ( le.isMouseCursorPosInArea( adventureSpellsRoi ) && displayableSpells != Filter::CMBT ) {
                statusCallback( _( "View Adventure Spells" ) );
            }
            else if ( le.isMouseCursorPosInArea( combatSpellsRoi ) && displayableSpells != Filter::ADVN ) {
                statusCallback( _( "View Combat Spells" ) );
            }
            else if ( !_isFirstPage() && le.isMouseCursorPosInArea( previousPageRoi ) ) {
                statusCallback( _( "View previous page" ) );
            }
            else if ( !_isLastPage( displayedSpells.size(), spellsPerPage * 2 ) && le.isMouseCursorPosInArea( nextPageRoi ) ) {
                statusCallback( _( "View next page" ) );
            }
            else if ( le.isMouseCursorPosInArea( closeRoi ) ) {
                statusCallback( _( "Close Spellbook" ) );
            }
            else if ( le.isMouseCursorPosInArea( pos ) ) {
                const int32_t index = GetRectIndex( coords, le.getMouseCursorPos() );

                if ( 0 <= index && index + _startSpellIndex < displayedSpells.size() ) {
                    const Spell & spell = displayedSpells[index + _startSpellIndex];

                    std::string str = _( "View %{spell}" );
                    StringReplace( str, "%{spell}", spell.GetName() );

                    statusCallback( str );
                }
                else if ( displayableSpells == Filter::CMBT ) {
                    statusCallback( _( "View Combat Spells" ) );
                }
                else if ( displayableSpells == Filter::ADVN ) {
                    statusCallback( _( "View Adventure Spells" ) );
                }
                else {
                    statusCallback( _( "View Spells" ) );
                }
            }
        }

        if ( le.isMouseRightButtonPressedInArea( pos ) ) {
            const int32_t index = GetRectIndex( coords, le.getMouseCursorPos() );

            if ( 0 <= index ) {
                const SpellStorage::const_iterator spell = displayedSpells.begin() + ( index + _startSpellIndex );
                if ( spell < displayedSpells.end() ) {
                    fheroes2::SpellDialogElement( *spell, &hero ).showPopup( Dialog::ZERO );
                    display.render();
                }
            }
        }

        if ( redraw ) {
            restorer.restore();
            spellBookRedrawLists( displayedSpells, coords, _startSpellIndex, pos.getPosition(), hero.GetSpellPoints(), displayableSpells, hero, canCastSpell );
            display.render( restorer.rect() );
            redraw = false;
        }
    }

    restorer.restore();
    display.render( restorer.rect() );

    return curspell;
}

void SpellBook::Edit( const HeroBase & hero )
{
    // Editing spells in a spell book does not require memorizing last open page.
    fheroes2::Display & display = fheroes2::Display::instance();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    size_t firstSpellOnPageIndex = 0;
    SpellStorage displayedSpells = SetFilter( Filter::ALL, &hero );

    const fheroes2::Sprite & bookmark_clos = fheroes2::AGG::GetICN( ICN::BOOK, 5 );

    const fheroes2::Sprite & bookPage = fheroes2::AGG::GetICN( ICN::BOOK, 0 );

    const fheroes2::Size spellBookSize = getSpellBookSize( Filter::ALL );

    const fheroes2::Rect pos( ( display.width() - ( bookPage.width() * 2 ) ) / 2, ( display.height() - bookPage.height() ) / 2, spellBookSize.width,
                              spellBookSize.height );

    fheroes2::ImageRestorer restorer( display, pos.x + spellBookShadow.x, pos.y, pos.width - spellBookShadow.x, pos.height + spellBookShadow.y );

    const fheroes2::Rect previousPageRoi( pos.x + 30, pos.y + 8, 30, 25 );
    const fheroes2::Rect nextPageRoi( pos.x + 410, pos.y + 8, 30, 25 );
    const fheroes2::Rect closeRoi( pos.x + 420, pos.y + 284, bookmark_clos.width(), bookmark_clos.height() );

    std::vector<fheroes2::Rect> coords;
    const size_t twoPagesSpells = spellsPerPage * 2;
    coords.reserve( twoPagesSpells );

    spellBookRedrawLists( displayedSpells, coords, firstSpellOnPageIndex, pos.getPosition(), hero.GetSpellPoints(), Filter::ALL, hero, false );
    bool redraw = false;

    display.render( restorer.rect() );

    LocalEvent & le = LocalEvent::Get();

    // message loop
    while ( le.HandleEvents() ) {
        if ( le.MouseClickLeft( previousPageRoi ) && firstSpellOnPageIndex ) {
            firstSpellOnPageIndex -= twoPagesSpells;
            redraw = true;
        }
        else if ( le.MouseClickLeft( nextPageRoi ) && size() > ( firstSpellOnPageIndex + twoPagesSpells ) ) {
            firstSpellOnPageIndex += twoPagesSpells;
            redraw = true;
        }
        else if ( le.MouseClickLeft( closeRoi ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
            // Close Spell Book.
            break;
        }
        else if ( le.MouseClickLeft( pos ) ) {
            const int32_t index = GetRectIndex( coords, le.getMouseCursorPos() );

            if ( 0 <= index ) {
                const SpellStorage::const_iterator spell = displayedSpells.begin() + ( index + firstSpellOnPageIndex );

                if ( spell < displayedSpells.end() ) {
                    fheroes2::SpellDialogElement( *spell, &hero ).showPopup( Dialog::OK );
                }
            }
            else {
                std::set<int32_t> spellsToExclude;
                for ( const Spell spell : *this ) {
                    spellsToExclude.insert( spell.GetID() );
                }

                const Spell spell = Dialog::selectSpell( Spell::NONE, false, spellsToExclude );
                if ( spell != Spell::NONE ) {
                    Append( spell );
                    displayedSpells = SetFilter( Filter::ALL, &hero );
                    redraw = true;
                }
            }
        }
        else if ( le.MouseClickRight( pos ) ) {
            // Remove Spell under the cursor from the Spell Book.
            const int32_t index = GetRectIndex( coords, le.getMouseCursorPos() );

            if ( index >= 0 ) {
                const SpellStorage::const_iterator spell = displayedSpells.begin() + ( index + firstSpellOnPageIndex );

                if ( spell < displayedSpells.end() && removeSpell( *spell ) ) {
                    displayedSpells = SetFilter( Filter::ALL, &hero );
                    redraw = true;
                }
            }
        }

        if ( redraw ) {
            restorer.restore();
            spellBookRedrawLists( displayedSpells, coords, firstSpellOnPageIndex, pos.getPosition(), hero.GetSpellPoints(), Filter::ALL, hero, false );
            display.render( restorer.rect() );
            redraw = false;
        }
    }

    restorer.restore();
    display.render( restorer.rect() );
}

void SpellBook::resetState()
{
    _startSpellIndex = 0;
    _spellFilter = Filter::ADVN;
}

SpellStorage SpellBook::SetFilter( const Filter filter, const HeroBase * hero /* = nullptr */ ) const
{
    const SpellStorage & storage = *this;
    SpellStorage res = storage;

    // add heroes spell scrolls
    if ( hero != nullptr ) {
        res.Append( hero->GetBagArtifacts() );
    }

    if ( filter != SpellBook::Filter::ALL ) {
        res.resize( std::distance( res.begin(), std::remove_if( res.begin(), res.end(), [filter]( const Spell & s ) {
                                       return ( ( SpellBook::Filter::ADVN == filter ) && s.isCombat() ) || ( ( SpellBook::Filter::CMBT == filter ) && !s.isCombat() );
                                   } ) ) );
    }

    // sorting results
    std::sort( res.begin(), res.end() );

    return res;
}
