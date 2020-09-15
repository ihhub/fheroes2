/***************************************************************************
 *   Copyright (C) 2011 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "dialog_selectitems.h"
#include "agg.h"
#include "army_troop.h"
#include "dialog.h"
#include "interface_list.h"
#include "text.h"

class SelectEnum : public Interface::ListBox<int>
{
public:
    SelectEnum( const Rect & rt )
        : Interface::ListBox<int>( rt )
        , area( rt )
        , ok( false )
    {
        RedrawBackground( rt );
        SetScrollButtonUp( ICN::LISTBOX, 3, 4, fheroes2::Point( rt.x + rt.w - 24, rt.y + 25 ) );
        SetScrollButtonDn( ICN::LISTBOX, 5, 6, fheroes2::Point( rt.x + rt.w - 24, rt.y + rt.h - 55 ) );

        SetScrollSplitter( fheroes2::AGG::GetICN( ICN::LISTBOX, 10 ), Rect( rt.x + rt.w - 19, rt.y + 48, 14, rt.h - 106 ) );
        SetAreaMaxItems( 5 );
        SetAreaItems( Rect( rt.x + 10, rt.y + 30, rt.w - 30, rt.h - 70 ) );
    };

    void RedrawBackground( const Point & dst )
    {
        Dialog::FrameBorder::RenderOther( fheroes2::AGG::GetICN( ICN::CELLWIN, 1 ), Rect( dst.x, dst.y + 25, rtAreaItems.w + 5, rtAreaItems.h + 10 ) );

        // scroll
        fheroes2::Display & display = fheroes2::Display::instance();
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::LISTBOX, 7 ), display, dst.x + area.w - 24, dst.y + 45 );

        for ( u32 ii = 1; ii < 9; ++ii )
            fheroes2::Blit( fheroes2::AGG::GetICN( ICN::LISTBOX, 8 ), display, dst.x + area.w - 24, dst.y + 44 + ( ii * 19 ) );

        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::LISTBOX, 9 ), display, dst.x + area.w - 24, dst.y + area.h - 74 );
    };

    void ActionListDoubleClick( int & index )
    {
        ok = true;
    };

    void RedrawItem( const int &, s32, s32, bool ){};
    void ActionCurrentUp( void ){};
    void ActionCurrentDn( void ){};
    void ActionListSingleClick( int & ){};
    void ActionListPressRight( int & ){};

    Rect area;
    bool ok;
};

class SelectEnumMonster : public SelectEnum
{
public:
    SelectEnumMonster( const Rect & rt )
        : SelectEnum( rt ){};

    void RedrawItem( const int & index, s32 dstx, s32 dsty, bool current )
    {
        Monster mons( index );
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::MONS32, mons.GetSpriteIndex() ), fheroes2::Display::instance(), dstx + 5, dsty + 3 );

        Text text( mons.GetName(), ( current ? Font::YELLOW_BIG : Font::BIG ) );
        text.Blit( dstx + 50, dsty + 10 );
    };

    void RedrawBackground( const Point & dst )
    {
        Text text( "Select Monster:", Font::YELLOW_BIG );
        text.Blit( dst.x + ( area.w - text.w() ) / 2, dst.y );

        SelectEnum::RedrawBackground( dst );
    };

    void ActionListPressRight( int & index )
    {
        Troop troop( Monster( index ), 1 );
        Dialog::ArmyInfo( troop, 0 );
    };
};

class SelectEnumHeroes : public SelectEnum
{
public:
    SelectEnumHeroes( const Rect & rt )
        : SelectEnum( rt )
    {
        SetAreaMaxItems( 6 );
    };

    void RedrawItem( const int & index, s32 dstx, s32 dsty, bool current )
    {
        fheroes2::Image port = Heroes::GetPortrait( index, PORT_SMALL );

        if ( !port.empty() )
            fheroes2::Blit( port, fheroes2::Display::instance(), dstx + 5, dsty + 3 );

        Text text( Heroes::GetName( index ), ( current ? Font::YELLOW_BIG : Font::BIG ) );
        text.Blit( dstx + 50, dsty + 5 );
    };

    void RedrawBackground( const Point & dst )
    {
        Text text( "Select Hero:", Font::YELLOW_BIG );
        text.Blit( dst.x + ( area.w - text.w() ) / 2, dst.y );

        SelectEnum::RedrawBackground( dst );
    };
};

class SelectEnumArtifact : public SelectEnum
{
public:
    SelectEnumArtifact( const Rect & rt )
        : SelectEnum( rt ){};

    void RedrawItem( const int & index, s32 dstx, s32 dsty, bool current )
    {
        Artifact art( index );
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::ARTFX, art.IndexSprite32() ), fheroes2::Display::instance(), dstx + 5, dsty + 3 );

        Text text( art.GetName(), ( current ? Font::YELLOW_BIG : Font::BIG ) );
        text.Blit( dstx + 50, dsty + 10 );
    };

    void RedrawBackground( const Point & dst )
    {
        Text text( "Select Artifact:", Font::YELLOW_BIG );
        text.Blit( dst.x + ( area.w - text.w() ) / 2, dst.y );

        SelectEnum::RedrawBackground( dst );
    };
};

class SelectEnumSpell : public SelectEnum
{
public:
    SelectEnumSpell( const Rect & rt )
        : SelectEnum( rt )
    {
        SetAreaMaxItems( 4 );
    };

    void RedrawItem( const int & index, s32 dstx, s32 dsty, bool current )
    {
        Spell spell( index );
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::SPELLS, spell.IndexSprite() ), fheroes2::Display::instance(), dstx + 5, dsty + 3 );

        Text text( spell.GetName(), ( current ? Font::YELLOW_BIG : Font::BIG ) );
        text.Blit( dstx + 80, dsty + 10 );
    };

    void RedrawBackground( const Point & dst )
    {
        Text text( "Select Spell:", Font::YELLOW_BIG );
        text.Blit( dst.x + ( area.w - text.w() ) / 2, dst.y );

        SelectEnum::RedrawBackground( dst );
    };
};

class SelectEnumSecSkill : public SelectEnum
{
public:
    SelectEnumSecSkill( const Rect & rt )
        : SelectEnum( rt )
    {
        SetAreaMaxItems( 5 );
    };

    void RedrawItem( const int & index, s32 dstx, s32 dsty, bool current )
    {
        Skill::Secondary skill( 1 + index / 3, 1 + ( index % 3 ) );
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::MINISS, skill.GetIndexSprite2() ), fheroes2::Display::instance(), dstx + 5, dsty + 3 );
        std::string str = skill.GetName();
        Text text( str, ( current ? Font::YELLOW_BIG : Font::BIG ) );
        text.Blit( dstx + 50, dsty + 10 );
    };

    void RedrawBackground( const Point & dst )
    {
        Text text( "Select Skill:", Font::YELLOW_BIG );
        text.Blit( dst.x + ( area.w - text.w() ) / 2, dst.y );

        SelectEnum::RedrawBackground( dst );
    };
};

Skill::Secondary Dialog::SelectSecondarySkill( void )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();

    std::vector<int> skills( MAXSECONDARYSKILL * 3, 0 );

    cursor.Hide();
    cursor.SetThemes( cursor.POINTER );

    for ( size_t ii = 0; ii < MAXSECONDARYSKILL * 3; ++ii )
        skills[ii] = ii;

    Dialog::FrameBorder frameborder( Size( 310, 280 ), fheroes2::AGG::GetICN( ICN::TEXTBAK2, 0 ) );
    const Rect & area = frameborder.GetArea();

    SelectEnumSecSkill listbox( area );

    listbox.SetListContent( skills );
    listbox.Redraw();

    fheroes2::ButtonGroup btnGroups( fheroes2::Rect( area.x, area.y, area.w, area.h ), Dialog::OK | Dialog::CANCEL );
    btnGroups.draw();

    cursor.Show();
    display.render();

    int result = Dialog::ZERO;

    while ( result == Dialog::ZERO && !listbox.ok && le.HandleEvents() ) {
        result = btnGroups.processEvents();
        listbox.QueueEventProcessing();

        if ( !cursor.isVisible() ) {
            listbox.Redraw();
            cursor.Show();
            display.render();
        }
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
    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();

    std::vector<int> spells( static_cast<int>( Spell::STONE - 1 ), Spell::NONE );

    cursor.Hide();
    cursor.SetThemes( cursor.POINTER );

    for ( size_t ii = 0; ii < spells.size(); ++ii )
        spells[ii] = ii + 1;

    Dialog::FrameBorder frameborder( Size( 340, 280 ), fheroes2::AGG::GetICN( ICN::TEXTBAK2, 0 ) );
    const Rect & area = frameborder.GetArea();

    SelectEnumSpell listbox( area );

    listbox.SetListContent( spells );
    if ( cur != Spell::NONE )
        listbox.SetCurrent( static_cast<int>( cur ) );
    listbox.Redraw();

    fheroes2::ButtonGroup btnGroups( fheroes2::Rect( area.x, area.y, area.w, area.h ), Dialog::OK | Dialog::CANCEL );
    btnGroups.draw();

    cursor.Show();
    display.render();

    int result = Dialog::ZERO;
    while ( result == Dialog::ZERO && !listbox.ok && le.HandleEvents() ) {
        result = btnGroups.processEvents();
        listbox.QueueEventProcessing();

        if ( !cursor.isVisible() ) {
            listbox.Redraw();
            cursor.Show();
            display.render();
        }
    }

    return result == Dialog::OK || listbox.ok ? Spell( listbox.GetCurrent() ) : Spell( Spell::NONE );
}

Artifact Dialog::SelectArtifact( int cur )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();

    std::vector<int> artifacts( static_cast<int>( Artifact::UNKNOWN ), Artifact::UNKNOWN );

    cursor.Hide();
    cursor.SetThemes( cursor.POINTER );

    for ( size_t ii = 0; ii < artifacts.size(); ++ii )
        artifacts[ii] = ii;

    Dialog::FrameBorder frameborder( Size( 370, 280 ), fheroes2::AGG::GetICN( ICN::TEXTBAK2, 0 ) );
    const Rect & area = frameborder.GetArea();

    SelectEnumArtifact listbox( area );

    listbox.SetListContent( artifacts );
    if ( cur != Artifact::UNKNOWN )
        listbox.SetCurrent( static_cast<int>( cur ) );
    listbox.Redraw();

    fheroes2::ButtonGroup btnGroups( fheroes2::Rect( area.x, area.y, area.w, area.h ), Dialog::OK | Dialog::CANCEL );
    btnGroups.draw();

    cursor.Show();
    display.render();

    int result = Dialog::ZERO;
    while ( result == Dialog::ZERO && !listbox.ok && le.HandleEvents() ) {
        result = btnGroups.processEvents();
        listbox.QueueEventProcessing();

        if ( !cursor.isVisible() ) {
            listbox.Redraw();
            cursor.Show();
            display.render();
        }
    }

    return result == Dialog::OK || listbox.ok ? Artifact( listbox.GetCurrent() ) : Artifact( Artifact::UNKNOWN );
}

Monster Dialog::SelectMonster( int id )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();

    std::vector<int> monsters( static_cast<int>( Monster::WATER_ELEMENT ), Monster::UNKNOWN );

    cursor.Hide();
    cursor.SetThemes( cursor.POINTER );

    for ( size_t ii = 0; ii < monsters.size(); ++ii )
        monsters[ii] = ii + 1; // skip Monser::UNKNOWN

    Dialog::FrameBorder frameborder( Size( 260, 280 ), fheroes2::AGG::GetICN( ICN::TEXTBAK2, 0 ) );
    const Rect & area = frameborder.GetArea();

    SelectEnumMonster listbox( area );

    listbox.SetListContent( monsters );
    if ( id != Monster::UNKNOWN )
        listbox.SetCurrent( static_cast<int>( id ) );
    listbox.Redraw();

    fheroes2::ButtonGroup btnGroups( fheroes2::Rect( area.x, area.y, area.w, area.h ), Dialog::OK | Dialog::CANCEL );
    btnGroups.draw();

    cursor.Show();
    display.render();

    int result = Dialog::ZERO;
    while ( result == Dialog::ZERO && !listbox.ok && le.HandleEvents() ) {
        result = btnGroups.processEvents();
        listbox.QueueEventProcessing();

        if ( !cursor.isVisible() ) {
            listbox.Redraw();
            cursor.Show();
            display.render();
        }
    }

    return result == Dialog::OK || listbox.ok ? Monster( listbox.GetCurrent() ) : Monster( Monster::UNKNOWN );
}

int Dialog::SelectHeroes( int cur )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();

    std::vector<int> heroes( static_cast<int>( Heroes::SANDYSANDY ), Heroes::UNKNOWN );

    cursor.Hide();
    cursor.SetThemes( cursor.POINTER );

    for ( size_t ii = 0; ii < heroes.size(); ++ii )
        heroes[ii] = ii;

    Dialog::FrameBorder frameborder( Size( 240, 280 ), fheroes2::AGG::GetICN( ICN::TEXTBAK2, 0 ) );
    const Rect & area = frameborder.GetArea();

    SelectEnumHeroes listbox( area );

    listbox.SetListContent( heroes );
    if ( cur != Heroes::UNKNOWN )
        listbox.SetCurrent( cur );
    listbox.Redraw();

    fheroes2::ButtonGroup btnGroups( fheroes2::Rect( area.x, area.y, area.w, area.h ), Dialog::OK | Dialog::CANCEL );
    btnGroups.draw();

    cursor.Show();
    display.render();

    int result = Dialog::ZERO;
    while ( result == Dialog::ZERO && !listbox.ok && le.HandleEvents() ) {
        result = btnGroups.processEvents();
        listbox.QueueEventProcessing();

        if ( !cursor.isVisible() ) {
            listbox.Redraw();
            cursor.Show();
            display.render();
        }
    }

    return result == Dialog::OK || listbox.ok ? listbox.GetCurrent() : Heroes::UNKNOWN;
}
