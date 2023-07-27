/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
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

#include "dialog_selectitems.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "agg_image.h"
#include "army_troop.h"
#include "cursor.h"
#include "dialog.h"
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
#include "ui_window.h"

class SelectEnum : public Interface::ListBox<int>
{
public:
    using Interface::ListBox<int>::ActionListDoubleClick;
    using Interface::ListBox<int>::ActionListSingleClick;
    using Interface::ListBox<int>::ActionListPressRight;

    explicit SelectEnum( const fheroes2::Rect & rt )
        : Interface::ListBox<int>( rt.getPosition() )
        , area( rt )
    {
        SetAreaItems( { rt.x + 10, rt.y + 30, rt.width - 30, rt.height - 70 } );

        // SelectEnum::RedrawBackground( rt.getPosition() );

        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();
        const int listIcnId = isEvilInterface ? ICN::LISTBOX_EVIL : ICN::LISTBOX;

        SetScrollButtonUp( listIcnId, 3, 4, { rt.x + rt.width - 25, rt.y + 25 } );
        SetScrollButtonDn( listIcnId, 5, 6, { rt.x + rt.width - 25, rt.y + rt.height - 55 } );

        setScrollBarArea( { rt.x + rt.width - 21, rt.y + 48, 14, rt.height - 107 } );
        setScrollBarImage( fheroes2::AGG::GetICN( listIcnId, 10 ) );
        SetAreaMaxItems( 5 );
    }

    void RedrawBackground( const fheroes2::Point & dst ) override
    {
        // if ( rtAreaItems.height < 1 ) {
        //    return;
        //}

        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();
        const int listIcnId = isEvilInterface ? ICN::LISTBOX_EVIL : ICN::LISTBOX;

        fheroes2::Display & display = fheroes2::Display::instance();

        /*const fheroes2::Sprite & upperPart = fheroes2::AGG::GetICN( listIcnId, 0 );
        const fheroes2::Sprite & middlePart = fheroes2::AGG::GetICN( listIcnId, 1 );
        const fheroes2::Sprite & lowerPart = fheroes2::AGG::GetICN( listIcnId, 2 );

        int32_t offsetY = 25;
        fheroes2::Copy( upperPart, 0, 0, display, dst.x, dst.y + offsetY, upperPart.width(), upperPart.height() );

        offsetY += upperPart.height();

        int32_t totalHeight = rtAreaItems.height + 11;
        int32_t middlePartCount = ( totalHeight - upperPart.height() - lowerPart.height() + middlePart.height() - 1 ) / middlePart.height();

        for ( int32_t i = 0; i < middlePartCount; ++i ) {
            fheroes2::Copy( middlePart, 0, 0, display, dst.x, dst.y + offsetY, middlePart.width(), middlePart.height() );
            offsetY += middlePart.height();
        }

        fheroes2::Copy( lowerPart, 0, 0, display, dst.x, dst.y + totalHeight - lowerPart.height() + 25, lowerPart.width(), lowerPart.height() );
        */
        Dialog::FrameBorder::RenderOther( fheroes2::AGG::GetICN( ICN::TEXTBACK, 1 ), { dst.x, dst.y + 25, rtAreaItems.width + 5, rtAreaItems.height + 11 } );

        // scroll

        const fheroes2::Sprite & topSprite = fheroes2::AGG::GetICN( listIcnId, 7 );
        fheroes2::Copy( topSprite, 0, 0, display, dst.x + area.width - 25, dst.y + 45, topSprite.width(), topSprite.height() );

        const fheroes2::Sprite & middleSprite = fheroes2::AGG::GetICN( listIcnId, 8 );
        const int32_t count = ( area.height - 119 ) / middleSprite.height() + 1;
        for ( int32_t i = 1; i < count; ++i ) {
            fheroes2::Copy( middleSprite, 0, 0, display, dst.x + area.width - 25, dst.y + 44 + ( i * 19 ), middleSprite.width(), middleSprite.height() );
        }

        const fheroes2::Sprite & bottomSprite = fheroes2::AGG::GetICN( listIcnId, 9 );
        fheroes2::Copy( bottomSprite, 0, 0, display, dst.x + area.width - 25, dst.y + area.height - 74, bottomSprite.width(), bottomSprite.height() );
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

    void ActionListSingleClick( int & /* unused */ ) override
    {
        // Do nothing.
    }

    void ActionListPressRight( int & /* unused */ ) override
    {
        // Do nothing.
    }

    fheroes2::Rect area;
    bool ok{ false };
};

class SelectEnumMonster : public SelectEnum
{
public:
    explicit SelectEnumMonster( const fheroes2::Rect & rt )
        : SelectEnum( rt )
    {
        SetAreaMaxItems( ( rt.height - 41 - 50 + 43 ) / 43 );
    }

    using SelectEnum::ActionListPressRight;

    void RedrawItem( const int & index, int32_t dstx, int32_t dsty, bool current ) override
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        const Monster mons( index );
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::MONS32, mons.GetSpriteIndex() ), display, dstx + 5, dsty + 3 );

        const fheroes2::Text text( mons.GetName(), current ? fheroes2::FontType::normalYellow() : fheroes2::FontType::normalWhite() );
        text.draw( dstx + 50, dsty + 10, display );
    }

    void RedrawBackground( const fheroes2::Point & dst ) override
    {
        const fheroes2::Text text( _( "Select Monster:" ), fheroes2::FontType::normalYellow() );
        text.draw( dst.x + ( area.width - text.width() ) / 2, dst.y, fheroes2::Display::instance() );

        SelectEnum::RedrawBackground( dst );
    }

    void ActionListPressRight( int & index ) override
    {
        const Troop troop( Monster( index ), 1 );
        Dialog::ArmyInfo( troop, Dialog::ZERO );
    }
};

class SelectEnumHeroes : public SelectEnum
{
public:
    explicit SelectEnumHeroes( const fheroes2::Rect & rt )
        : SelectEnum( rt )
    {
        SetAreaMaxItems( ( rt.height - 41 - 50 + 35 ) / 35 );
    }

    void RedrawItem( const int & index, int32_t dstx, int32_t dsty, bool current ) override
    {
        const fheroes2::Sprite & port = Heroes::GetPortrait( index, PORT_SMALL );
        fheroes2::Display & display = fheroes2::Display::instance();

        if ( !port.empty() ) {
            fheroes2::Blit( port, display, dstx + 5, dsty + 3 );
        }

        const fheroes2::Text text( Heroes::GetName( index ), current ? fheroes2::FontType::normalYellow() : fheroes2::FontType::normalWhite() );
        text.draw( dstx + 50, dsty + 5, display );
    }

    void RedrawBackground( const fheroes2::Point & dst ) override
    {
        const fheroes2::Text text( _( "Select Hero:" ), fheroes2::FontType::normalYellow() );
        text.draw( dst.x + ( area.width - text.width() ) / 2, dst.y, fheroes2::Display::instance() );

        SelectEnum::RedrawBackground( dst );
    }
};

class SelectEnumArtifact : public SelectEnum
{
public:
    explicit SelectEnumArtifact( const fheroes2::Rect & rt )
        : SelectEnum( rt )
    {
        SetAreaMaxItems( ( rt.height - 41 - 50 + 42 ) / 42 );
    }

    void RedrawItem( const int & index, int32_t dstx, int32_t dsty, bool current ) override
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        const Artifact art( index );

        const fheroes2::Sprite & artifactSprite = fheroes2::AGG::GetICN( ICN::ARTFX, art.IndexSprite32() );
        fheroes2::Blit( artifactSprite, display, dstx + 5, dsty + 3 );

        const fheroes2::Text text( art.GetName(), current ? fheroes2::FontType::normalYellow() : fheroes2::FontType::normalWhite() );
        text.draw( dstx + 50, dsty + 10, display );
    }

    void RedrawBackground( const fheroes2::Point & dst ) override
    {
        const fheroes2::Text text( _( "Select Artifact:" ), fheroes2::FontType::normalYellow() );
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
        SetAreaMaxItems( ( rt.height - 41 - 50 + 52 ) / 52 );
    }

    void RedrawItem( const int & index, int32_t dstx, int32_t dsty, bool current ) override
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        const Spell spell( index );
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::SPELLS, spell.IndexSprite() ), display, dstx + 5, dsty + 3 );

        const fheroes2::Text text( spell.GetName(), current ? fheroes2::FontType::normalYellow() : fheroes2::FontType::normalWhite() );
        text.draw( dstx + 80, dsty + 10, display );
    }

    void RedrawBackground( const fheroes2::Point & dst ) override
    {
        const fheroes2::Text text( _( "Select Spell:" ), fheroes2::FontType::normalYellow() );
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
        SetAreaMaxItems( ( rt.height - 41 - 50 + 42 ) / 42 );
    }

    void RedrawItem( const int & index, int32_t dstx, int32_t dsty, bool current ) override
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        const Skill::Secondary skill( 1 + index / 3, 1 + ( index % 3 ) );
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::MINISS, skill.GetIndexSprite2() ), display, dstx + 5, dsty + 3 );

        const fheroes2::Text text( skill.GetName(), current ? fheroes2::FontType::normalYellow() : fheroes2::FontType::normalWhite() );
        text.draw( dstx + 50, dsty + 10, display );
    }

    void RedrawBackground( const fheroes2::Point & dst ) override
    {
        const fheroes2::Text text( _( "Select Skill:" ), fheroes2::FontType::normalYellow() );
        text.draw( dst.x + ( area.width - text.width() ) / 2, dst.y, fheroes2::Display::instance() );

        SelectEnum::RedrawBackground( dst );
    }
};

Skill::Secondary Dialog::selectSecondarySkill()
{
    fheroes2::Display & display = fheroes2::Display::instance();
    LocalEvent & le = LocalEvent::Get();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    std::vector<int> skills( static_cast<int>( MAXSECONDARYSKILL * 3 ), 0 );

    for ( int i = 0; i < MAXSECONDARYSKILL * 3; ++i ) {
        skills[i] = i;
    }

    // const Dialog::FrameBorder frameborder( { 350, display.height() - 200 }, fheroes2::AGG::GetICN( ICN::TEXTBAK2, 0 ) );
    const std::unique_ptr<fheroes2::StandardWindow> frameborder = std::make_unique<fheroes2::StandardWindow>( 350, display.height() - 200, true );

    const fheroes2::Rect & area = frameborder->activeArea();

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

Spell Dialog::selectSpell( const int spellId /* = Spell::NONE */ )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    LocalEvent & le = LocalEvent::Get();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    std::vector<int> spells = Spell::getAllSpellIdsSuitableForSpellBook();

    // const Dialog::FrameBorder frameborder( { 340, 280 }, fheroes2::AGG::GetICN( ICN::TEXTBAK2, 0 ) );
    const std::unique_ptr<fheroes2::StandardWindow> frameborder = std::make_unique<fheroes2::StandardWindow>( 340, display.height() - 200, true );

    const fheroes2::Rect & area = frameborder->activeArea();

    SelectEnumSpell listbox( area );

    listbox.SetListContent( spells );
    if ( spellId != Spell::NONE ) {
        listbox.SetCurrent( spellId );
    }
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

Artifact Dialog::selectArtifact()
{
    fheroes2::Display & display = fheroes2::Display::instance();
    LocalEvent & le = LocalEvent::Get();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    std::vector<int> artifacts;
    artifacts.reserve( Artifact::ARTIFACT_COUNT - 1 );

    const bool isPriceofLoyaltyArtifactAllowed = Settings::Get().isCurrentMapPriceOfLoyalty();

    for ( int artifactId = Artifact::UNKNOWN + 1; artifactId < Artifact::ARTIFACT_COUNT; ++artifactId ) {
        if ( Artifact( artifactId ).isValid() && ( isPriceofLoyaltyArtifactAllowed || !fheroes2::isPriceOfLoyaltyArtifact( artifactId ) ) ) {
            artifacts.emplace_back( artifactId );
        }
    }

    // const Dialog::FrameBorder frameborder( { 370, 280 }, fheroes2::AGG::GetICN( ICN::TEXTBAK2, 0 ) );
    const std::unique_ptr<fheroes2::StandardWindow> frameborder = std::make_unique<fheroes2::StandardWindow>( 370, display.height() - 200, true );

    const fheroes2::Rect & area = frameborder->activeArea();

    SelectEnumArtifact listbox( area );

    listbox.SetListContent( artifacts );
    // Force to select the first artifact in the list.
    listbox.SetCurrent( static_cast<size_t>( 0 ) );
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

    return ( result == Dialog::OK || listbox.ok ) ? Artifact( listbox.GetCurrent() ) : Artifact( Artifact::UNKNOWN );
}

Monster Dialog::selectMonster( const int monsterId /* = Monster::UNKNOWN */ )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    LocalEvent & le = LocalEvent::Get();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    std::vector<int> monsters( static_cast<int>( Monster::WATER_ELEMENT ), Monster::UNKNOWN );

    for ( size_t i = 0; i < monsters.size(); ++i ) {
        // Skip Monster::UNKNOWN, safe to do casting as the monsters can't be more than 2 billion
        monsters[i] = static_cast<int>( i + 1 );
    }

    // const Dialog::FrameBorder frameborder( { 260, 280 }, fheroes2::AGG::GetICN( ICN::TEXTBAK2, 0 ) );
    const std::unique_ptr<fheroes2::StandardWindow> frameborder = std::make_unique<fheroes2::StandardWindow>( 280, display.height() - 200, true );

    const fheroes2::Rect & area = frameborder->activeArea();

    SelectEnumMonster listbox( area );

    listbox.SetListContent( monsters );
    if ( monsterId != Monster::UNKNOWN ) {
        listbox.SetCurrent( monsterId );
    }
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

int Dialog::selectHeroes( const int heroId /* = Heroes::UNKNOWN */ )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    LocalEvent & le = LocalEvent::Get();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    std::vector<int> heroes( static_cast<int>( Settings::Get().isCurrentMapPriceOfLoyalty() ? Heroes::DEBUG_HERO : Heroes::SOLMYR ), Heroes::UNKNOWN );

    for ( size_t i = 0; i < heroes.size(); ++i ) {
        // Safe to do casting as the heroes can't be more than 2 billion.
        heroes[i] = static_cast<int>( i );
    }

    // const Dialog::FrameBorder frameborder( { 240, 280 }, fheroes2::AGG::GetICN( ICN::TEXTBAK2, 0 ) );
    const std::unique_ptr<fheroes2::StandardWindow> frameborder = std::make_unique<fheroes2::StandardWindow>( 240, display.height() - 200, true );

    const fheroes2::Rect & area = frameborder->activeArea();

    SelectEnumHeroes listbox( area );

    listbox.SetListContent( heroes );
    if ( heroId != Heroes::UNKNOWN ) {
        listbox.SetCurrent( heroId );
    }
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
