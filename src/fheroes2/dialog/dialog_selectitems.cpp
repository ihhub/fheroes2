/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2011 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "agg_image.h"
#include "army_troop.h"
#include "cursor.h"
#include "dialog.h"
#include "dialog_selectitems.h"
#include "gamedefs.h"
#include "heroes_base.h"
#include "icn.h"
#include "image.h"
#include "interface_list.h"
#include "localevent.h"
#include "math_base.h"
#include "screen.h"
#include "settings.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_text.h"

class SelectEnum : public Interface::ListBox<int>
{
public:
    using Interface::ListBox<int>::ActionListDoubleClick;
    using Interface::ListBox<int>::ActionListSingleClick;
    using Interface::ListBox<int>::ActionListPressRight;

    explicit SelectEnum( const fheroes2::Rect & rt )
        : Interface::ListBox<int>( rt.getPosition() )
        , area( rt )
        , ok( false )
    {
        SelectEnum::RedrawBackground( rt.getPosition() );

        SetScrollButtonUp( ICN::LISTBOX, 3, 4, { rt.x + rt.width - 25, rt.y + 25 } );
        SetScrollButtonDn( ICN::LISTBOX, 5, 6, { rt.x + rt.width - 25, rt.y + rt.height - 55 } );

        setScrollBarArea( { rt.x + rt.width - 21, rt.y + 48, 14, rt.height - 107 } );
        setScrollBarImage( fheroes2::AGG::GetICN( ICN::LISTBOX, 10 ) );
        SetAreaMaxItems( 5 );
        SetAreaItems( { rt.x + 10, rt.y + 30, rt.width - 30, rt.height - 70 } );
    }

    void RedrawBackground( const fheroes2::Point & dst ) override
    {
        Dialog::FrameBorder::RenderOther( fheroes2::AGG::GetICN( ICN::CELLWIN, 1 ), { dst.x, dst.y + 25, rtAreaItems.width + 5, rtAreaItems.height + 11 } );

        // scroll
        fheroes2::Display & display = fheroes2::Display::instance();
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::LISTBOX, 7 ), display, dst.x + area.width - 25, dst.y + 45 );

        for ( int32_t i = 1; i < 9; ++i )
            fheroes2::Blit( fheroes2::AGG::GetICN( ICN::LISTBOX, 8 ), display, dst.x + area.width - 25, dst.y + 44 + ( i * 19 ) );

        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::LISTBOX, 9 ), display, dst.x + area.width - 25, dst.y + area.height - 74 );
    }

    void ActionListDoubleClick( int & /*index*/ ) override
    {
        ok = true;
    }

    void RedrawItem( const int & /* unused */, int32_t /* ox */, int32_t /* oy */, bool /* current */ ) override
    {
        // Do nothing.
    }

    void ActionCurrentUp() override
    {
        // Do nothing.
    }

    void ActionCurrentDn() override
    {
        // Do nothing.
    }

    void ActionListSingleClick( int & ) override
    {
        // Do nothing.
    }

    void ActionListPressRight( int & ) override
    {
        // Do nothing.
    }

    fheroes2::Rect area;
    bool ok;
};

class SelectEnumMonster : public SelectEnum
{
public:
    using SelectEnum::SelectEnum;

    using SelectEnum::ActionListPressRight;

    void RedrawItem( const int & index, int32_t dstx, int32_t dsty, bool current ) override
    {
        Monster mons( index );
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::MONS32, mons.GetSpriteIndex() ), fheroes2::Display::instance(), dstx + 5, dsty + 3 );

        if ( current ) {
            fheroes2::Text text( mons.GetName(), fheroes2::FontType::normalYellow() );
            text.draw( dstx + 50, dsty + 10, fheroes2::Display::instance() );
        }
        else {
            fheroes2::Text text( mons.GetName(), fheroes2::FontType() );
            text.draw( dstx + 50, dsty + 10, fheroes2::Display::instance() );
        }
    }

    void RedrawBackground( const fheroes2::Point & dst ) override
    {
        fheroes2::Text text( _( "Select Monster:" ), fheroes2::FontType::normalYellow() );
        text.draw( dst.x + ( area.width - text.width() ) / 2, dst.y, fheroes2::Display::instance() );

        SelectEnum::RedrawBackground( dst );
    }

    void ActionListPressRight( int & index ) override
    {
        Troop troop( Monster( index ), 1 );
        Dialog::ArmyInfo( troop, 0 );
    }
};

class SelectEnumHeroes : public SelectEnum
{
public:
    explicit SelectEnumHeroes( const fheroes2::Rect & rt )
        : SelectEnum( rt )
    {
        SetAreaMaxItems( 6 );
    }

    void RedrawItem( const int & index, int32_t dstx, int32_t dsty, bool current ) override
    {
        const fheroes2::Sprite & port = Heroes::GetPortrait( index, PORT_SMALL );

        if ( !port.empty() )
            fheroes2::Blit( port, fheroes2::Display::instance(), dstx + 5, dsty + 3 );

        if ( current ) {
            fheroes2::Text text( Heroes::GetName( index ), fheroes2::FontType::normalYellow() );
            text.draw( dstx + 50, dsty + 5, fheroes2::Display::instance() );
        }
        else {
            fheroes2::Text text( Heroes::GetName( index ), fheroes2::FontType() );
            text.draw( dstx + 50, dsty + 5, fheroes2::Display::instance() );
        }
    }

    void RedrawBackground( const fheroes2::Point & dst ) override
    {
        fheroes2::Text text( _( "Select Hero:" ), fheroes2::FontType::normalYellow() );
        text.draw( dst.x + ( area.width - text.width() ) / 2, dst.y, fheroes2::Display::instance() );

        SelectEnum::RedrawBackground( dst );
    }
};

class SelectEnumArtifact : public SelectEnum
{
public:
    using SelectEnum::SelectEnum;

    void RedrawItem( const int & index, int32_t dstx, int32_t dsty, bool current ) override
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        const Artifact art( index );

        const fheroes2::Sprite & artifactSprite = fheroes2::AGG::GetICN( ICN::ARTFX, art.IndexSprite32() );
        fheroes2::Blit( artifactSprite, display, dstx + 5, dsty + 3 );

        if ( current ) {
            fheroes2::Text text( art.GetName(), fheroes2::FontType::normalYellow() );
            text.draw( dstx + 50, dsty + 10, display );
        }
        else {
            fheroes2::Text text( art.GetName(), fheroes2::FontType() );
            text.draw( dstx + 50, dsty + 10, display );
        }
    }

    void RedrawBackground( const fheroes2::Point & dst ) override
    {
        fheroes2::Text text( _( "Select Artifact:" ), fheroes2::FontType::normalYellow() );
        text.draw( dst.x + ( area.width - text.width() ) / 2, dst.y, fheroes2::Display::instance() );

        SelectEnum::RedrawBackground( dst );
    }
};

class SelectEnumSpell : public SelectEnum
{
public:
    explicit SelectEnumSpell( const fheroes2::Rect & rt )
        : SelectEnum( rt )
    {
        SetAreaMaxItems( 4 );
    }

    void RedrawItem( const int & index, int32_t dstx, int32_t dsty, bool current ) override
    {
        Spell spell( index );
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::SPELLS, spell.IndexSprite() ), fheroes2::Display::instance(), dstx + 5, dsty + 3 );

        if ( current ) {
            fheroes2::Text text( spell.GetName(), fheroes2::FontType::normalYellow() );
            text.draw( dstx + 80, dsty + 10, fheroes2::Display::instance() );
        }
        else {
            fheroes2::Text text( spell.GetName(), fheroes2::FontType() );
            text.draw( dstx + 80, dsty + 10, fheroes2::Display::instance() );
        }
    }

    void RedrawBackground( const fheroes2::Point & dst ) override
    {
        fheroes2::Text text( _( "Select Spell:" ), fheroes2::FontType::normalYellow() );
        text.draw( dst.x + ( area.width - text.width() ) / 2, dst.y, fheroes2::Display::instance() );

        SelectEnum::RedrawBackground( dst );
    }
};

class SelectEnumSecSkill : public SelectEnum
{
public:
    explicit SelectEnumSecSkill( const fheroes2::Rect & rt )
        : SelectEnum( rt )
    {
        SetAreaMaxItems( 5 );
    }

    void RedrawItem( const int & index, int32_t dstx, int32_t dsty, bool current ) override
    {
        Skill::Secondary skill( 1 + index / 3, 1 + ( index % 3 ) );
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::MINISS, skill.GetIndexSprite2() ), fheroes2::Display::instance(), dstx + 5, dsty + 3 );
        std::string str = skill.GetName();

        if ( current ) {
            fheroes2::Text text( str, fheroes2::FontType::normalYellow() );
            text.draw( dstx + 50, dsty + 10, fheroes2::Display::instance() );
        }
        else {
            fheroes2::Text text( str, fheroes2::FontType() );
            text.draw( dstx + 50, dsty + 10, fheroes2::Display::instance() );
        }
    }

    void RedrawBackground( const fheroes2::Point & dst ) override
    {
        fheroes2::Text text( _( "Select Skill:" ), fheroes2::FontType::normalYellow() );
        text.draw( dst.x + ( area.width - text.width() ) / 2, dst.y, fheroes2::Display::instance() );

        SelectEnum::RedrawBackground( dst );
    }
};

Skill::Secondary Dialog::SelectSecondarySkill()
{
    fheroes2::Display & display = fheroes2::Display::instance();
    LocalEvent & le = LocalEvent::Get();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    std::vector<int> skills( MAXSECONDARYSKILL * 3, 0 );

    for ( int i = 0; i < MAXSECONDARYSKILL * 3; ++i )
        skills[i] = i;

    Dialog::FrameBorder frameborder( { 310, 280 }, fheroes2::AGG::GetICN( ICN::TEXTBAK2, 0 ) );
    const fheroes2::Rect & area = frameborder.GetArea();

    SelectEnumSecSkill listbox( area );

    listbox.SetListContent( skills );
    listbox.Redraw();

    fheroes2::ButtonGroup btnGroups( area, Dialog::OK | Dialog::CANCEL );
    btnGroups.draw();

    display.render();

    int result = Dialog::ZERO;

    while ( result == Dialog::ZERO && !listbox.ok && le.HandleEvents() ) {
        result = btnGroups.processEvents();
        listbox.QueueEventProcessing();

        if ( !listbox.IsNeedRedraw() ) {
            continue;
        }

        listbox.Redraw();
        display.render();
    }

    Skill::Secondary skill;

    if ( result == Dialog::OK || listbox.ok ) {
        skill.SetSkill( 1 + ( listbox.GetCurrent() / 3 ) );
        skill.SetLevel( 1 + ( listbox.GetCurrent() % 3 ) );
    }

    return skill;
}

Spell Dialog::SelectSpell( int cur )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    LocalEvent & le = LocalEvent::Get();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    std::vector<int> spells( static_cast<int>( Spell::RANDOM - 1 ), Spell::NONE );

    for ( size_t i = 0; i < spells.size(); ++i )
        spells[i] = static_cast<int>( i + 1 ); // safe to do this as the number of spells can't be more than 2 billion

    Dialog::FrameBorder frameborder( { 340, 280 }, fheroes2::AGG::GetICN( ICN::TEXTBAK2, 0 ) );
    const fheroes2::Rect & area = frameborder.GetArea();

    SelectEnumSpell listbox( area );

    listbox.SetListContent( spells );
    if ( cur != Spell::NONE )
        listbox.SetCurrent( cur );
    listbox.Redraw();

    fheroes2::ButtonGroup btnGroups( area, Dialog::OK | Dialog::CANCEL );
    btnGroups.draw();

    display.render();

    int result = Dialog::ZERO;
    while ( result == Dialog::ZERO && !listbox.ok && le.HandleEvents() ) {
        result = btnGroups.processEvents();
        listbox.QueueEventProcessing();

        if ( !listbox.IsNeedRedraw() ) {
            continue;
        }

        listbox.Redraw();
        display.render();
    }

    return result == Dialog::OK || listbox.ok ? Spell( listbox.GetCurrent() ) : Spell( Spell::NONE );
}

Artifact Dialog::SelectArtifact( int cur )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    LocalEvent & le = LocalEvent::Get();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    std::vector<int> artifacts( static_cast<int>( Settings::Get().isCurrentMapPriceOfLoyalty() ? Artifact::UNKNOWN : Artifact::SPELL_SCROLL ), Artifact::UNKNOWN );

    for ( size_t i = 0; i < artifacts.size(); ++i )
        artifacts[i] = static_cast<int>( i ); // safe to do this as the number of artifacts can't be more than 2 billion

    Dialog::FrameBorder frameborder( { 370, 280 }, fheroes2::AGG::GetICN( ICN::TEXTBAK2, 0 ) );
    const fheroes2::Rect & area = frameborder.GetArea();

    SelectEnumArtifact listbox( area );

    listbox.SetListContent( artifacts );
    if ( cur != Artifact::UNKNOWN )
        listbox.SetCurrent( cur );
    listbox.Redraw();

    fheroes2::ButtonGroup btnGroups( area, Dialog::OK | Dialog::CANCEL );
    btnGroups.draw();

    display.render();

    int result = Dialog::ZERO;
    while ( result == Dialog::ZERO && !listbox.ok && le.HandleEvents() ) {
        result = btnGroups.processEvents();
        listbox.QueueEventProcessing();

        if ( !listbox.IsNeedRedraw() ) {
            continue;
        }

        listbox.Redraw();
        display.render();
    }

    return result == Dialog::OK || listbox.ok ? Artifact( listbox.GetCurrent() ) : Artifact( Artifact::UNKNOWN );
}

Monster Dialog::SelectMonster( int id )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    LocalEvent & le = LocalEvent::Get();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    std::vector<int> monsters( static_cast<int>( Monster::WATER_ELEMENT ), Monster::UNKNOWN );

    for ( size_t i = 0; i < monsters.size(); ++i )
        monsters[i] = static_cast<int>( i + 1 ); // skip Monser::UNKNOWN, safe to do this as the monsters of spells can't be more than 2 billion

    Dialog::FrameBorder frameborder( { 260, 280 }, fheroes2::AGG::GetICN( ICN::TEXTBAK2, 0 ) );
    const fheroes2::Rect & area = frameborder.GetArea();

    SelectEnumMonster listbox( area );

    listbox.SetListContent( monsters );
    if ( id != Monster::UNKNOWN )
        listbox.SetCurrent( id );
    listbox.Redraw();

    fheroes2::ButtonGroup btnGroups( area, Dialog::OK | Dialog::CANCEL );
    btnGroups.draw();

    display.render();

    int result = Dialog::ZERO;
    while ( result == Dialog::ZERO && !listbox.ok && le.HandleEvents() ) {
        result = btnGroups.processEvents();
        listbox.QueueEventProcessing();

        if ( !listbox.IsNeedRedraw() ) {
            continue;
        }

        listbox.Redraw();
        display.render();
    }

    return result == Dialog::OK || listbox.ok ? Monster( listbox.GetCurrent() ) : Monster( Monster::UNKNOWN );
}

int Dialog::SelectHeroes( int cur )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    LocalEvent & le = LocalEvent::Get();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    std::vector<int> heroes( static_cast<int>( Settings::Get().isCurrentMapPriceOfLoyalty() ? Heroes::DEBUG_HERO : Heroes::SOLMYR ), Heroes::UNKNOWN );

    for ( size_t i = 0; i < heroes.size(); ++i )
        heroes[i] = static_cast<int>( i ); // safe to do this as the heroes of spells can't be more than 2 billion

    Dialog::FrameBorder frameborder( { 240, 280 }, fheroes2::AGG::GetICN( ICN::TEXTBAK2, 0 ) );
    const fheroes2::Rect & area = frameborder.GetArea();

    SelectEnumHeroes listbox( area );

    listbox.SetListContent( heroes );
    if ( cur != Heroes::UNKNOWN )
        listbox.SetCurrent( cur );
    listbox.Redraw();

    fheroes2::ButtonGroup btnGroups( area, Dialog::OK | Dialog::CANCEL );
    btnGroups.draw();

    display.render();

    int result = Dialog::ZERO;
    while ( result == Dialog::ZERO && !listbox.ok && le.HandleEvents() ) {
        result = btnGroups.processEvents();
        listbox.QueueEventProcessing();

        if ( !listbox.IsNeedRedraw() ) {
            continue;
        }

        listbox.Redraw();
        display.render();
    }

    return result == Dialog::OK || listbox.ok ? listbox.GetCurrent() : Heroes::UNKNOWN;
}
