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
#include <array>
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
#include "map_object_info.h"
#include "math_base.h"
#include "race.h"
#include "screen.h"
#include "settings.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_map_object.h"
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

        const fheroes2::Rect area( background->activeArea() );
        const fheroes2::Rect listRoi( area.x + 10, area.y + 30, area.width - 40, area.height - 70 );

        background->applyTextBackgroundShading( listRoi );

        listBackground = std::make_unique<fheroes2::ImageRestorer>( display, listRoi.x, listRoi.y, listRoi.width, listRoi.height );

        SetAreaItems( { listRoi.x + 5, listRoi.y + 5, listRoi.width - 10, listRoi.height - 10 } );

        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();
        const int32_t scrollbarOffsetX = area.x + area.width - 25;

        background->renderScrollbarBackground( { scrollbarOffsetX, listRoi.y, listRoi.width, listRoi.height }, isEvilInterface );

        const int32_t topPartHeight = 19;
        const int listIcnId = isEvilInterface ? ICN::SCROLLE : ICN::SCROLL;

        SetScrollButtonUp( listIcnId, 0, 1, { scrollbarOffsetX + 1, listRoi.y + 1 } );
        SetScrollButtonDn( listIcnId, 2, 3, { scrollbarOffsetX + 1, listRoi.y + listRoi.height - 15 } );
        setScrollBarArea( { scrollbarOffsetX + 3, listRoi.y + topPartHeight, 10, listRoi.height - 2 * topPartHeight } );
        setScrollBarImage( fheroes2::AGG::GetICN( listIcnId, 4 ) );

        // Render dialog buttons.
        background->renderOkayCancelButtons( buttonOk, buttonCancel, isEvilInterface );
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
    // This is a base class for items used in the Editor and they rely on Maps::ObjectInfo structures.
    class ObjectTypeSelection : public SelectEnum
    {
    public:
        ObjectTypeSelection( const std::vector<Maps::ObjectInfo> & objectInfo, const fheroes2::Size & size, const int32_t offset )
            : SelectEnum( size )
            , _objectInfo( objectInfo )
        {
            SetAreaMaxItems( ( rtAreaItems.height + offset ) / offset );
        }

        using SelectEnum::ActionListPressRight;

        void RedrawItem( const int & objectId, int32_t offsetX, int32_t offsetY, bool isSelected ) override
        {
            // If this assertion blows up then you are setting different number of items.
            assert( objectId >= 0 && objectId < static_cast<int>( _objectInfo.size() ) );

            const fheroes2::Sprite & image = fheroes2::generateMapObjectImage( _objectInfo[objectId] );
            renderItem( image, getObjectName( _objectInfo[objectId] ), { offsetX, offsetY }, { 32, 50 }, isSelected );
        }

        void ActionListPressRight( int & objectId ) override
        {
            // If this assertion blows up then you are setting different number of items.
            assert( objectId >= 0 && objectId < static_cast<int>( _objectInfo.size() ) );

            showPopupWindow( _objectInfo[objectId] );
        }

    private:
        virtual void showPopupWindow( const Maps::ObjectInfo & info ) = 0;

        virtual std::string getObjectName( const Maps::ObjectInfo & info ) = 0;

        const std::vector<Maps::ObjectInfo> & _objectInfo;
    };

    class HeroTypeSelection : public ObjectTypeSelection
    {
    public:
        HeroTypeSelection( const std::vector<Maps::ObjectInfo> & objectInfo, const fheroes2::Size & size )
            : ObjectTypeSelection( objectInfo, size, fheroes2::AGG::GetICN( ICN::MINIHERO, 0 ).height() + 2 )
        {
            // Do nothing.
        }

    private:
        void showPopupWindow( const Maps::ObjectInfo & info ) override
        {
            fheroes2::showStandardTextMessage( getObjectName( info ), "", Dialog::ZERO );
        }

        std::string getObjectName( const Maps::ObjectInfo & info ) override
        {
            const int color = static_cast<int>( info.metadata[0] );
            const int race = static_cast<int>( info.metadata[1] );

            std::string name( _( "%{color} %{race} hero" ) );
            StringReplace( name, "%{color}", Color::String( Color::IndexToColor( color ) ) );
            StringReplace( name, "%{race}", Race::String( Race::IndexToRace( race ) ) );

            return name;
        }
    };

    class MonsterTypeSelection : public ObjectTypeSelection
    {
    public:
        MonsterTypeSelection( const std::vector<Maps::ObjectInfo> & objectInfo, const fheroes2::Size & size )
            : ObjectTypeSelection( objectInfo, size, fheroes2::AGG::GetICN( ICN::MINIHERO, 0 ).height() + 2 )
        {
            // Do nothing.
        }

    private:
        void showPopupWindow( const Maps::ObjectInfo & info ) override
        {
            const Monster monster( static_cast<int32_t>( info.metadata[0] ) );
            if ( !monster.isValid() ) {
                fheroes2::showStandardTextMessage( monster.GetName(), "", Dialog::ZERO );
                return;
            }

            Dialog::ArmyInfo( Troop( monster, 0 ), Dialog::ZERO );
        }

        std::string getObjectName( const Maps::ObjectInfo & info ) override
        {
            return Monster( static_cast<int32_t>( info.metadata[0] ) ).GetName();
        }
    };

    int selectObjectType( const int objectType, const size_t objectCount, ObjectTypeSelection & objectSelection, const char * title )
    {
        assert( title != nullptr );

        std::vector<int> objects( objectCount, 0 );
        std::iota( objects.begin(), objects.end(), 0 );
        objectSelection.SetListContent( objects );

        objectSelection.SetCurrent( std::max( objectType, 0 ) );

        const int32_t result = objectSelection.selectItemsEventProcessing( title );
        return result == Dialog::OK || objectSelection.ok ? objectSelection.GetCurrent() : -1;
    }
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

Monster Dialog::selectMonster( const int monsterId )
{
    std::vector<int> monsters( Monster::MONSTER_COUNT - 1, Monster::UNKNOWN );

    // Skip Monster::UNKNOWN and start from the next one.
    std::iota( monsters.begin(), monsters.end(), Monster::UNKNOWN + 1 );
    monsters.erase( std::remove_if( monsters.begin(), monsters.end(), []( const int id ) { return Monster( id ).isRandomMonster(); } ), monsters.end() );

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
    const auto & objectInfo = Maps::getObjectsByGroup( Maps::ObjectGroup::Hero );
    HeroTypeSelection listbox( objectInfo, { 350, fheroes2::Display::instance().height() - 200 } );

    return selectObjectType( heroType, objectInfo.size(), listbox, _( "Select Hero:" ) );
}

int Dialog::selectMonsterType( const int monsterType )
{
    const auto & objectInfo = Maps::getObjectsByGroup( Maps::ObjectGroup::Monster );

    MonsterTypeSelection listbox( objectInfo, { 280, fheroes2::Display::instance().height() - 200 } );

    return selectObjectType( monsterType, objectInfo.size(), listbox, _( "Select Monster:" ) );
}
