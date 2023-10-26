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
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

#include "agg_image.h"
#include "army_troop.h"
#include "color.h"
#include "cursor.h"
#include "dialog.h"
#include "game_hotkeys.h"
#include "gamedefs.h"
#include "heroes_base.h"
#include "icn.h"
#include "image.h"
#include "interface_list.h"
#include "localevent.h"
#include "math_base.h"
#include "race.h"
#include "screen.h"
#include "settings.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_scrollbar.h"
#include "ui_text.h"
#include "ui_window.h"
#include "world.h"

class SelectEnum : public Interface::ListBox<int>
{
public:
    using Interface::ListBox<int>::ActionListDoubleClick;
    using Interface::ListBox<int>::ActionListSingleClick;
    using Interface::ListBox<int>::ActionListPressRight;

    SelectEnum() = delete;

    explicit SelectEnum( const fheroes2::Size & dialogSize )
    {
        fheroes2::Display & display = fheroes2::Display::instance();
        background = std::make_unique<fheroes2::StandardWindow>( dialogSize.width, dialogSize.height, true, display );

        const fheroes2::Rect area = background->activeArea();

        const fheroes2::Rect listRoi( area.x + 10, area.y + 30, area.width - 40, area.height - 70 );

        background->applyTextBackgroundShading( listRoi );

        listBackground = std::make_unique<fheroes2::ImageRestorer>( display, listRoi.x, listRoi.y, listRoi.width, listRoi.height );

        SetAreaItems( { listRoi.x + 5, listRoi.y + 5, listRoi.width - 10, listRoi.height - 10 } );

        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

        const fheroes2::Sprite & scrollBar = fheroes2::AGG::GetICN( isEvilInterface ? ICN::ADVBORDE : ICN::ADVBORD, 0 );

        int32_t scrollbarOffsetX = area.x + area.width - 25;

        // Top part of scrollbar background.
        const int32_t topPartHeight = 19;
        const int32_t scrollBarWidth = 16;
        fheroes2::Copy( scrollBar, 536, 176, display, scrollbarOffsetX, listRoi.y, scrollBarWidth, topPartHeight );

        // Middle part of scrollbar background.
        int32_t offsetY = topPartHeight;
        const int32_t middlePartHeight = 88;
        const int32_t middlePartCount = ( listRoi.height - 2 * topPartHeight + middlePartHeight - 1 ) / middlePartHeight;

        for ( int32_t i = 0; i < middlePartCount; ++i ) {
            fheroes2::Copy( scrollBar, 536, 196, display, scrollbarOffsetX, listRoi.y + offsetY, scrollBarWidth,
                            std::min( middlePartHeight, listRoi.height - offsetY - topPartHeight ) );
            offsetY += middlePartHeight;
        }

        // Bottom part of scrollbar background.
        fheroes2::Copy( scrollBar, 536, 285, display, scrollbarOffsetX, listRoi.y + listRoi.height - topPartHeight, scrollBarWidth, topPartHeight );

        const int listIcnId = isEvilInterface ? ICN::SCROLLE : ICN::SCROLL;

        ++scrollbarOffsetX;

        SetScrollButtonUp( listIcnId, 0, 1, { scrollbarOffsetX, listRoi.y + 1 } );
        SetScrollButtonDn( listIcnId, 2, 3, { scrollbarOffsetX, listRoi.y + listRoi.height - 15 } );

        setScrollBarArea( { scrollbarOffsetX + 2, listRoi.y + topPartHeight, 10, listRoi.height - 2 * topPartHeight } );

        setScrollBarImage( fheroes2::AGG::GetICN( listIcnId, 4 ) );

        // Make scrollbar shadow.
        for ( uint8_t i = 0; i < 4; ++i ) {
            const uint8_t transformId = i + 2;
            const int32_t sizeCorrection = i + 1;
            fheroes2::ApplyTransform( display, scrollbarOffsetX - transformId, listRoi.y + sizeCorrection, 1, listRoi.height - sizeCorrection, transformId );
            fheroes2::ApplyTransform( display, scrollbarOffsetX - transformId, listRoi.y + listRoi.height + i, scrollBarWidth, 1, transformId );
        }

        // Dialog buttons.
        const int32_t buttonFromBorderOffsetX = 20;
        const int32_t buttonY = listRoi.y + listRoi.height + 7;

        const int buttonOkIcn = isEvilInterface ? ICN::BUTTON_SMALL_OKAY_EVIL : ICN::BUTTON_SMALL_OKAY_GOOD;
        buttonOk.setICNInfo( buttonOkIcn, 0, 1 );
        buttonOk.setPosition( area.x + buttonFromBorderOffsetX, buttonY );
        const fheroes2::Sprite & buttonOkSprite = fheroes2::AGG::GetICN( buttonOkIcn, 0 );
        fheroes2::addGradientShadow( buttonOkSprite, display, buttonOk.area().getPosition(), { -5, 5 } );
        buttonOk.draw();

        const int buttonCancelIcn = isEvilInterface ? ICN::BUTTON_SMALL_CANCEL_EVIL : ICN::BUTTON_SMALL_CANCEL_GOOD;
        buttonCancel.setICNInfo( buttonCancelIcn, 0, 1 );
        const fheroes2::Sprite & buttonCancelSprite = fheroes2::AGG::GetICN( buttonCancelIcn, 0 );
        buttonCancel.setPosition( area.x + area.width - buttonCancelSprite.width() - buttonFromBorderOffsetX, buttonY );
        fheroes2::addGradientShadow( buttonCancelSprite, display, buttonCancel.area().getPosition(), { -5, 5 } );
        buttonCancel.draw();
    }

    void RedrawBackground( const fheroes2::Point & /* unused */ ) override
    {
        listBackground->restore();
    }

    void ActionListDoubleClick( int & /* unused */ ) override
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

    void updateScrollBarImage()
    {
        const int32_t scrollBarWidth = _scrollbar.width();

        setScrollBarImage( fheroes2::generateScrollbarSlider( _scrollbar, false, _scrollbar.getArea().height, VisibleItemCount(), _size(), { 0, 0, scrollBarWidth, 8 },
                                                              { 0, 7, scrollBarWidth, 8 } ) );
        _scrollbar.moveToIndex( _topId );
    }

    void renderItem( const fheroes2::Sprite & itemSprite, const std::string & itemText, const fheroes2::Point & destination, const fheroes2::Point & offset,
                     const bool current ) const
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        if ( !itemSprite.empty() ) {
            fheroes2::Blit( itemSprite, display, destination.x + ( offset.x - itemSprite.width() ) / 2, destination.y + ( offset.y - itemSprite.height() ) / 2 );
        }

        fheroes2::Text text( itemText, current ? fheroes2::FontType::normalYellow() : fheroes2::FontType::normalWhite() );
        text.fitToOneRow( background->activeArea().width - offset.x - 55 );
        text.draw( destination.x + offset.x + 5, destination.y + ( offset.y - text.height() ) / 2 + 2, display );
    }

    int32_t selectItemsEventProcessing( const char * caption )
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        // setup cursor
        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        const fheroes2::Rect roi = background->activeArea();

        const fheroes2::Text text( caption, fheroes2::FontType::normalYellow() );
        text.draw( roi.x + ( roi.width - text.width() ) / 2, roi.y + 10, display );

        updateScrollBarImage();

        Redraw();
        display.render( background->totalArea() );

        LocalEvent & le = LocalEvent::Get();

        while ( !ok && le.HandleEvents() ) {
            le.MousePressLeft( buttonOk.area() ) ? buttonOk.drawOnPress() : buttonOk.drawOnRelease();
            le.MousePressLeft( buttonCancel.area() ) ? buttonCancel.drawOnPress() : buttonCancel.drawOnRelease();

            if ( le.MouseClickLeft( buttonOk.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) ) {
                return Dialog::OK;
            }
            if ( le.MouseClickLeft( buttonCancel.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
                return Dialog::CANCEL;
            }

            if ( le.MousePressRight( buttonOk.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Okay" ), _( "Accept the choice made." ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( buttonCancel.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Exit this menu without doing anything." ), Dialog::ZERO );
            }

            QueueEventProcessing();

            if ( !IsNeedRedraw() ) {
                continue;
            }

            Redraw();
            display.render( roi );
        }

        return Dialog::ZERO;
    }

    bool ok{ false };
    std::unique_ptr<fheroes2::StandardWindow> background;
    std::unique_ptr<fheroes2::ImageRestorer> listBackground;
    fheroes2::Button buttonOk;
    fheroes2::Button buttonCancel;
};

class SelectEnumMonster : public SelectEnum
{
public:
    explicit SelectEnumMonster( const fheroes2::Size & rt )
        : SelectEnum( rt )
    {
        const int offset = 43;
        SetAreaMaxItems( ( rtAreaItems.height + offset ) / offset );
    }

    using SelectEnum::ActionListPressRight;

    void RedrawItem( const int & index, int32_t dstx, int32_t dsty, bool current ) override
    {
        const Monster mons( index );
        const fheroes2::Sprite & monsterSprite = fheroes2::AGG::GetICN( ICN::MONS32, mons.GetSpriteIndex() );

        renderItem( monsterSprite, mons.GetName(), { dstx, dsty }, { 45, 43 }, current );
    }

    void ActionListPressRight( int & index ) override
    {
        const Monster monster( index );
        if ( !monster.isValid() ) {
            fheroes2::showStandardTextMessage( monster.GetName(), "", Dialog::ZERO );
            return;
        }

        Dialog::ArmyInfo( Troop( monster, 0 ), Dialog::ZERO );
    }
};

class SelectEnumHeroes : public SelectEnum
{
public:
    explicit SelectEnumHeroes( const fheroes2::Size & rt )
        : SelectEnum( rt )
    {
        const int offset = 35;
        SetAreaMaxItems( ( rtAreaItems.height + offset ) / offset );
    }

    using SelectEnum::ActionListPressRight;

    void RedrawItem( const int & index, int32_t dstx, int32_t dsty, bool current ) override
    {
        const fheroes2::Sprite & port = Heroes::GetPortrait( index, PORT_SMALL );

        renderItem( port, Heroes::GetName( index ), { dstx, dsty }, { 45, 35 }, current );
    }

    void ActionListPressRight( int & index ) override
    {
        Dialog::QuickInfo( *world.GetHeroes( index ) );
    }
};

class SelectEnumArtifact : public SelectEnum
{
public:
    explicit SelectEnumArtifact( const fheroes2::Size & rt )
        : SelectEnum( rt )
    {
        const int offset = 42;
        SetAreaMaxItems( ( rtAreaItems.height + offset ) / offset );
    }

    using SelectEnum::ActionListPressRight;

    void RedrawItem( const int & index, int32_t dstx, int32_t dsty, bool current ) override
    {
        const Artifact art( index );
        const fheroes2::Sprite & artifactSprite = fheroes2::AGG::GetICN( ICN::ARTFX, art.IndexSprite32() );

        renderItem( artifactSprite, art.GetName(), { dstx, dsty }, { 45, 42 }, current );
    }

    void ActionListPressRight( int & index ) override
    {
        fheroes2::ArtifactDialogElement( Artifact( index ) ).showPopup( Dialog::ZERO );
    }
};

class SelectEnumSpell : public SelectEnum
{
public:
    explicit SelectEnumSpell( const fheroes2::Size & rt )
        : SelectEnum( rt )
    {
        const int offset = 55;
        SetAreaMaxItems( ( rtAreaItems.height + offset ) / offset );
    }

    using SelectEnum::ActionListPressRight;

    void RedrawItem( const int & index, int32_t dstx, int32_t dsty, bool current ) override
    {
        const Spell spell( index );
        const fheroes2::Sprite & spellSprite = fheroes2::AGG::GetICN( ICN::SPELLS, spell.IndexSprite() );

        renderItem( spellSprite, spell.GetName(), { dstx, dsty }, { 75, 55 }, current );
    }

    void ActionListPressRight( int & index ) override
    {
        fheroes2::SpellDialogElement( Spell( index ), nullptr ).showPopup( Dialog::ZERO );
    }
};

class SelectEnumSecSkill : public SelectEnum
{
public:
    static int getSkillFromListIndex( int index )
    {
        return 1 + index / 3;
    }

    static int getLevelFromListIndex( int index )
    {
        return 1 + ( index % 3 );
    }

    explicit SelectEnumSecSkill( const fheroes2::Size & rt )
        : SelectEnum( rt )
    {
        const int offset = 42;
        SetAreaMaxItems( ( rtAreaItems.height + offset ) / offset );
    }

    using SelectEnum::ActionListPressRight;

    void RedrawItem( const int & index, int32_t dstx, int32_t dsty, bool current ) override
    {
        const Skill::Secondary skill( getSkillFromListIndex( index ), getLevelFromListIndex( index ) );
        const fheroes2::Sprite & skillSprite = fheroes2::AGG::GetICN( ICN::MINISS, skill.GetIndexSprite2() );

        renderItem( skillSprite, skill.GetName(), { dstx, dsty }, { 45, 42 }, current );
    }

    void ActionListPressRight( int & index ) override
    {
        fheroes2::SecondarySkillDialogElement( Skill::Secondary( getSkillFromListIndex( index ), getLevelFromListIndex( index ) ), Heroes() ).showPopup( Dialog::ZERO );
    }
};

namespace
{
    class HeroTypeSelection : public SelectEnum
    {
    public:
        explicit HeroTypeSelection( const fheroes2::Size & size )
            : SelectEnum( size )
        {
            const int offset = fheroes2::AGG::GetICN( ICN::MINIHERO, 0 ).height() + 2;
            SetAreaMaxItems( ( rtAreaItems.height + offset ) / offset );
        }

        using SelectEnum::ActionListPressRight;

        void RedrawItem( const int & id, int32_t offsetX, int32_t offsetY, bool isSelected ) override
        {
            const fheroes2::Sprite & image = fheroes2::AGG::GetICN( ICN::MINIHERO, id );
            renderItem( image, getHeroName( id ), { offsetX, offsetY }, { 32, 50 }, isSelected );
        }

        void ActionListPressRight( int & type ) override
        {
            fheroes2::showStandardTextMessage( getHeroName( type ), "", Dialog::ZERO );
        }

    private:
        static std::string getHeroName( const int type )
        {
            // The game has only 6 races plus random, totaling in 7 races.
            // Also the game has only 6 colors.
            // As a result only 42 mini-heroes can exist.
            if ( type >= 42 ) {
                // Did you add a new hero?
                assert( 0 );
                return _( "Unknown Hero" );
            }

            const int color{ type / 7 };
            int race{ type % 7 };

            if ( race == 6 ) {
                ++race;
            }

            std::string name( _( "%{color} %{race} hero" ) );
            StringReplace( name, "%{color}", Color::String( Color::IndexToColor( color ) ) );
            StringReplace( name, "%{race}", Race::String( Race::IndexToRace( race ) ) );

            return name;
        }
    };
}

Skill::Secondary Dialog::selectSecondarySkill( const Heroes & hero, const int skillId /* = Skill::Secondary::UNKNOWN */ )
{
    std::vector<int> skills;
    skills.reserve( static_cast<size_t>( MAXSECONDARYSKILL * 3 ) );

    for ( int i = 0; i < MAXSECONDARYSKILL * 3; ++i ) {
        if ( !hero.HasSecondarySkill( SelectEnumSecSkill::getSkillFromListIndex( i ) ) ) {
            skills.push_back( i );
        }
    }

    SelectEnumSecSkill listbox( { 350, fheroes2::Display::instance().height() - 200 } );

    listbox.SetListContent( skills );
    if ( skillId != Skill::Secondary::UNKNOWN ) {
        listbox.SetCurrent( skillId );
    }

    const int32_t result = listbox.selectItemsEventProcessing( _( "Select Skill:" ) );

    if ( result == Dialog::OK || listbox.ok ) {
        const int skillIndex = listbox.GetCurrent();
        return { SelectEnumSecSkill::getSkillFromListIndex( skillIndex ), SelectEnumSecSkill::getLevelFromListIndex( skillIndex ) };
    }

    return {};
}

Spell Dialog::selectSpell( const int spellId /* = Spell::NONE */ )
{
    std::vector<int> spells = Spell::getAllSpellIdsSuitableForSpellBook();

    SelectEnumSpell listbox( { 340, fheroes2::Display::instance().height() - 200 } );

    listbox.SetListContent( spells );
    if ( spellId != Spell::NONE ) {
        listbox.SetCurrent( spellId );
    }

    const int32_t result = listbox.selectItemsEventProcessing( _( "Select Spell:" ) );

    return result == Dialog::OK || listbox.ok ? Spell( listbox.GetCurrent() ) : Spell( Spell::NONE );
}

Artifact Dialog::selectArtifact( const int artifactId /* = Artifact::UNKNOWN */ )
{
    std::vector<int> artifacts;
    artifacts.reserve( Artifact::ARTIFACT_COUNT - 1 );

    const bool isPriceofLoyaltyArtifactAllowed = Settings::Get().isCurrentMapPriceOfLoyalty();

    for ( int id = Artifact::UNKNOWN + 1; id < Artifact::ARTIFACT_COUNT; ++id ) {
        if ( Artifact( id ).isValid() && ( isPriceofLoyaltyArtifactAllowed || !fheroes2::isPriceOfLoyaltyArtifact( id ) ) ) {
            artifacts.emplace_back( id );
        }
    }

    SelectEnumArtifact listbox( { 370, fheroes2::Display::instance().height() - 200 } );

    listbox.SetListContent( artifacts );
    if ( artifactId != Artifact::UNKNOWN ) {
        listbox.SetCurrent( artifactId );
    }

    const int32_t result = listbox.selectItemsEventProcessing( _( "Select Artifact:" ) );

    return ( result == Dialog::OK || listbox.ok ) ? Artifact( listbox.GetCurrent() ) : Artifact( Artifact::UNKNOWN );
}

Monster Dialog::selectMonster( const int monsterId, const bool includeRandomMonsters )
{
    std::vector<int> monsters( Monster::MONSTER_COUNT - 1, Monster::UNKNOWN );

    // Skip Monster::UNKNOWN and start from the next one.
    std::iota( monsters.begin(), monsters.end(), Monster::UNKNOWN + 1 );

    if ( !includeRandomMonsters ) {
        monsters.erase( std::remove_if( monsters.begin(), monsters.end(), []( const int id ) { return Monster( id ).isRandomMonster(); } ), monsters.end() );
    }

    SelectEnumMonster listbox( { 280, fheroes2::Display::instance().height() - 200 } );

    listbox.SetListContent( monsters );
    if ( monsterId != Monster::UNKNOWN ) {
        listbox.SetCurrent( monsterId );
    }

    const int32_t result = listbox.selectItemsEventProcessing( _( "Select Monster:" ) );

    return result == Dialog::OK || listbox.ok ? Monster( listbox.GetCurrent() ) : Monster( Monster::UNKNOWN );
}

int Dialog::selectHeroes( const int heroId /* = Heroes::UNKNOWN */ )
{
    std::vector<int> heroes( static_cast<int>( Settings::Get().isCurrentMapPriceOfLoyalty() ? Heroes::JARKONAS : Heroes::BRAX ), Heroes::UNKNOWN );

    std::iota( heroes.begin(), heroes.end(), Heroes::UNKNOWN + 1 );

    SelectEnumHeroes listbox( { 240, fheroes2::Display::instance().height() - 200 } );

    listbox.SetListContent( heroes );
    if ( heroId != Heroes::UNKNOWN ) {
        listbox.SetCurrent( heroId );
    }

    const int32_t result = listbox.selectItemsEventProcessing( _( "Select Hero:" ) );

    return result == Dialog::OK || listbox.ok ? listbox.GetCurrent() : Heroes::UNKNOWN;
}

int Dialog::selectHeroType( const int heroType )
{
    std::vector<int> heroes( 42, 0 );
    std::iota( heroes.begin(), heroes.end(), 0 );

    HeroTypeSelection listbox( { 350, fheroes2::Display::instance().height() - 200 } );

    listbox.SetListContent( heroes );
    listbox.SetCurrent( std::max( heroType, 0 ) );

    const int32_t result = listbox.selectItemsEventProcessing( _( "Select Hero:" ) );
    return result == Dialog::OK || listbox.ok ? listbox.GetCurrent() : -1;
}
