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
#include <memory>
#include <numeric>
#include <utility>
#include <vector>

#include "agg_image.h"
#include "army_troop.h"
#include "castle.h"
#include "color.h"
#include "cursor.h"
#include "dialog.h"
#include "game_hotkeys.h"
#include "gamedefs.h"
#include "heroes_base.h"
#include "icn.h"
#include "image.h"
#include "interface_list.h"
#include "kingdom.h"
#include "localevent.h"
#include "map_object_info.h"
#include "maps.h"
#include "math_base.h"
#include "mp2.h"
#include "race.h"
#include "resource.h"
#include "screen.h"
#include "settings.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_castle.h"
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

    explicit SelectEnum( const fheroes2::Size & dialogSize, std::string title, std::string description = {} )
    {
        assert( !title.empty() );

        fheroes2::Display & display = fheroes2::Display::instance();
        background = std::make_unique<fheroes2::StandardWindow>( dialogSize.width, dialogSize.height, true, display );

        const fheroes2::Rect area( background->activeArea() );

        int32_t listOffsetY = 0;

        fheroes2::Text text( std::move( title ), fheroes2::FontType::normalYellow() );
        text.draw( area.x + ( area.width - text.width() ) / 2, area.y + 10, display );

        // The additional text under the title.
        if ( !description.empty() ) {
            text.set( std::move( description ), fheroes2::FontType::normalWhite() );
            text.draw( area.x + ( area.width - text.width() ) / 2, area.y + 30, display );
            listOffsetY = text.height() + 3;
        }

        const fheroes2::Rect listRoi( area.x + 10, area.y + 30 + listOffsetY, area.width - 40, area.height - 70 - listOffsetY );

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

    // An image with text should have offset of 10 pixels from all left and right edges.
    void renderItem( const fheroes2::Sprite & itemSprite, const std::string & itemText, const fheroes2::Point & destination, const int32_t middleImageOffsetX,
                     const int32_t textOffsetX, const int32_t itemOffsetY, const bool current ) const
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        fheroes2::Blit( itemSprite, display, destination.x + middleImageOffsetX - ( itemSprite.width() / 2 ), destination.y + itemOffsetY - ( itemSprite.height() / 2 ) );

        fheroes2::Text text( itemText, current ? fheroes2::FontType::normalYellow() : fheroes2::FontType::normalWhite() );
        text.fitToOneRow( background->activeArea().width - textOffsetX - 55 );
        text.draw( destination.x + textOffsetX, destination.y + itemOffsetY - ( text.height() / 2 ) + 2, display );
    }

    int32_t selectItemsEventProcessing()
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        // setup cursor
        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        const fheroes2::Rect roi = background->activeArea();

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
    explicit SelectEnumMonster( const fheroes2::Size & rt, std::string title )
        : SelectEnum( rt, std::move( title ) )
    {
        SetAreaMaxItems( rtAreaItems.height / _offsetY );
    }

    using SelectEnum::ActionListPressRight;

    void RedrawItem( const int & index, int32_t dstx, int32_t dsty, bool current ) override
    {
        const Monster mons( index );
        const fheroes2::Sprite & monsterSprite = fheroes2::AGG::GetICN( ICN::MONS32, mons.GetSpriteIndex() );

        renderItem( monsterSprite, mons.GetName(), { dstx, dsty }, 45 / 2, 50, _offsetY / 2, current );
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

private:
    static const int32_t _offsetY{ 43 };
};

class SelectEnumHeroes : public SelectEnum
{
public:
    explicit SelectEnumHeroes( const fheroes2::Size & rt, std::string title )
        : SelectEnum( rt, std::move( title ) )
    {
        SetAreaMaxItems( rtAreaItems.height / _offsetY );
    }

    using SelectEnum::ActionListPressRight;

    void RedrawItem( const int & index, int32_t dstx, int32_t dsty, bool current ) override
    {
        const fheroes2::Sprite & port = Heroes::GetPortrait( index, PORT_SMALL );

        renderItem( port, Heroes::GetName( index ), { dstx, dsty }, 45 / 2, 50, _offsetY / 2, current );
    }

    void ActionListPressRight( int & index ) override
    {
        Dialog::QuickInfo( *world.GetHeroes( index ) );
    }

private:
    static const int32_t _offsetY{ 35 };
};

class SelectEnumArtifact : public SelectEnum
{
public:
    explicit SelectEnumArtifact( const fheroes2::Size & rt, std::string title )
        : SelectEnum( rt, std::move( title ) )
    {
        SetAreaMaxItems( rtAreaItems.height / _offsetY );
    }

    using SelectEnum::ActionListPressRight;

    void RedrawItem( const int & index, int32_t dstx, int32_t dsty, bool current ) override
    {
        const Artifact art( index );
        const fheroes2::Sprite & artifactSprite = fheroes2::AGG::GetICN( ICN::ARTFX, art.IndexSprite32() );

        renderItem( artifactSprite, art.GetName(), { dstx, dsty }, 45 / 2, 50, _offsetY / 2, current );
    }

    void ActionListPressRight( int & index ) override
    {
        fheroes2::ArtifactDialogElement( Artifact( index ) ).showPopup( Dialog::ZERO );
    }

private:
    static const int32_t _offsetY{ 42 };
};

class SelectEnumSpell : public SelectEnum
{
public:
    explicit SelectEnumSpell( const fheroes2::Size & rt, std::string title )
        : SelectEnum( rt, std::move( title ) )
    {
        SetAreaMaxItems( rtAreaItems.height / _offsetY );
    }

    using SelectEnum::ActionListPressRight;

    void RedrawItem( const int & index, int32_t dstx, int32_t dsty, bool current ) override
    {
        const Spell spell( index );
        const fheroes2::Sprite & spellSprite = fheroes2::AGG::GetICN( ICN::SPELLS, spell.IndexSprite() );

        renderItem( spellSprite, spell.GetName(), { dstx, dsty }, 75 / 2, 80, _offsetY / 2, current );
    }

    void ActionListPressRight( int & index ) override
    {
        fheroes2::SpellDialogElement( Spell( index ), nullptr ).showPopup( Dialog::ZERO );
    }

private:
    static const int32_t _offsetY{ 55 };
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

    explicit SelectEnumSecSkill( const fheroes2::Size & rt, std::string title )
        : SelectEnum( rt, std::move( title ) )
    {
        SetAreaMaxItems( rtAreaItems.height / _offsetY );
    }

    using SelectEnum::ActionListPressRight;

    void RedrawItem( const int & index, int32_t dstx, int32_t dsty, bool current ) override
    {
        const Skill::Secondary skill( getSkillFromListIndex( index ), getLevelFromListIndex( index ) );
        const fheroes2::Sprite & skillSprite = fheroes2::AGG::GetICN( ICN::MINISS, skill.GetIndexSprite2() );

        renderItem( skillSprite, skill.GetName(), { dstx, dsty }, 45 / 2, 50, _offsetY / 2, current );
    }

    void ActionListPressRight( int & index ) override
    {
        fheroes2::SecondarySkillDialogElement( Skill::Secondary( getSkillFromListIndex( index ), getLevelFromListIndex( index ) ), Heroes() ).showPopup( Dialog::ZERO );
    }

private:
    static const int32_t _offsetY{ 42 };
};

class SelectKingdomCastle : public SelectEnum
{
public:
    explicit SelectKingdomCastle( const fheroes2::Size & rt, std::string title, std::string description )
        : SelectEnum( rt, std::move( title ), std::move( description ) )
        , _townFrameIcnId( Settings::Get().isEvilInterfaceEnabled() ? ICN::LOCATORE : ICN::LOCATORS )
    {
        SetAreaMaxItems( rtAreaItems.height / itemsOffsetY );
    }

    using SelectEnum::ActionListPressRight;

    void RedrawItem( const int & index, int32_t dstx, int32_t dsty, bool current ) override
    {
        const Castle * castle = world.getCastleEntrance( Maps::GetPoint( index ) );

        assert( castle != nullptr );

        fheroes2::Sprite castleIcon( fheroes2::AGG::GetICN( _townFrameIcnId, 23 ) );
        fheroes2::drawCastleIcon( *castle, castleIcon, { 4, 4 } );

        renderItem( castleIcon, castle->GetName(), { dstx, dsty }, 35, 75, itemsOffsetY / 2, current );
    }

    void ActionListPressRight( int & index ) override
    {
        Dialog::QuickInfoWithIndicationOnRadar( *world.getCastleEntrance( Maps::GetPoint( index ) ), background->totalArea() );
    }

    static const int32_t itemsOffsetY{ 35 };

private:
    const int _townFrameIcnId;
};

namespace
{
    // This is a base class for items used in the Editor and they rely on Maps::ObjectInfo structures.
    class ObjectTypeSelection : public SelectEnum
    {
    public:
        ObjectTypeSelection( const std::vector<Maps::ObjectInfo> & objectInfo, const fheroes2::Size & size, std::string title, const int32_t imageOffsetX,
                             const int32_t textOffsetX, const int32_t offsetY )
            : SelectEnum( size, std::move( title ) )
            , _objectInfo( objectInfo )
            , _imageOffsetX( imageOffsetX )
            , _textOffsetX( textOffsetX )
            , _offsetY( offsetY )
        {
            SetAreaMaxItems( rtAreaItems.height / _offsetY );
        }

        using SelectEnum::ActionListPressRight;

        void RedrawItem( const int & objectId, int32_t posX, int32_t posY, bool isSelected ) override
        {
            // If this assertion blows up then you are setting different number of items.
            assert( objectId >= 0 && objectId < static_cast<int>( _objectInfo.size() ) );

            const fheroes2::Sprite & image = fheroes2::generateMapObjectImage( _objectInfo[objectId] );
            renderItem( image, getObjectName( _objectInfo[objectId] ), { posX, posY }, _imageOffsetX, _textOffsetX, _offsetY / 2, isSelected );
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

        const int32_t _imageOffsetX{ 0 };

        const int32_t _textOffsetX{ 0 };

        const int32_t _offsetY{ 0 };
    };

    class HeroTypeSelection : public ObjectTypeSelection
    {
    public:
        HeroTypeSelection( const std::vector<Maps::ObjectInfo> & objectInfo, const fheroes2::Size & size, std::string title )
            : ObjectTypeSelection( objectInfo, size, std::move( title ), 21, 47, fheroes2::AGG::GetICN( ICN::MINIHERO, 0 ).height() + 2 )
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
        MonsterTypeSelection( const std::vector<Maps::ObjectInfo> & objectInfo, const fheroes2::Size & size, std::string title )
            : ObjectTypeSelection( objectInfo, size, std::move( title ), 45 / 2, 50, 43 )
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

    class ArtifactTypeSelection : public ObjectTypeSelection
    {
    public:
        ArtifactTypeSelection( const std::vector<Maps::ObjectInfo> & objectInfo, const fheroes2::Size & size, std::string title )
            : ObjectTypeSelection( objectInfo, size, std::move( title ), 24, 65, 40 )
        {
            // Do nothing.
        }

    private:
        void showPopupWindow( const Maps::ObjectInfo & info ) override
        {
            switch ( info.objectType ) {
            case MP2::OBJ_ARTIFACT:
                fheroes2::ArtifactDialogElement( Artifact( static_cast<int>( info.metadata[0] ) ) ).showPopup( Dialog::ZERO );
                break;
            case MP2::OBJ_RANDOM_ARTIFACT:
            case MP2::OBJ_RANDOM_ARTIFACT_MINOR:
            case MP2::OBJ_RANDOM_ARTIFACT_MAJOR:
            case MP2::OBJ_RANDOM_ARTIFACT_TREASURE:
            case MP2::OBJ_RANDOM_ULTIMATE_ARTIFACT:
                fheroes2::showStandardTextMessage( MP2::StringObject( info.objectType ), "", Dialog::ZERO );
                break;
            default:
                // Did you expand the list of artifacts? Add the corresponding logic!
                assert( 0 );
                break;
            }
        }

        std::string getObjectName( const Maps::ObjectInfo & info ) override
        {
            switch ( info.objectType ) {
            case MP2::OBJ_ARTIFACT:
                return Artifact( static_cast<int>( info.metadata[0] ) ).GetName();
            case MP2::OBJ_RANDOM_ARTIFACT:
            case MP2::OBJ_RANDOM_ARTIFACT_MINOR:
            case MP2::OBJ_RANDOM_ARTIFACT_MAJOR:
            case MP2::OBJ_RANDOM_ARTIFACT_TREASURE:
            case MP2::OBJ_RANDOM_ULTIMATE_ARTIFACT:
                return MP2::StringObject( info.objectType );
            default:
                // Did you expand the list of treasures? Add the corresponding logic!
                assert( 0 );
                break;
            }

            return {};
        }
    };

    class TreasureTypeSelection : public ObjectTypeSelection
    {
    public:
        TreasureTypeSelection( const std::vector<Maps::ObjectInfo> & objectInfo, const fheroes2::Size & size, std::string title )
            : ObjectTypeSelection( objectInfo, size, std::move( title ), 17, 60, 40 )
        {
            // Do nothing.
        }

    private:
        void showPopupWindow( const Maps::ObjectInfo & info ) override
        {
            switch ( info.objectType ) {
            case MP2::OBJ_RESOURCE:
                fheroes2::showResourceMessage( fheroes2::Text{ getObjectName( info ), fheroes2::FontType::normalYellow() },
                                               fheroes2::Text{ Resource::getDescription(), fheroes2::FontType::normalWhite() }, Dialog::ZERO,
                                               Funds{ static_cast<int>( info.metadata[0] ), 0 } );
                break;
            case MP2::OBJ_GENIE_LAMP:
            case MP2::OBJ_RANDOM_RESOURCE:
            case MP2::OBJ_TREASURE_CHEST:
                fheroes2::showStandardTextMessage( getObjectName( info ), "", Dialog::ZERO );
                break;
            default:
                // Did you expand the list of treasures? Add the corresponding logic!
                assert( 0 );
                break;
            }
        }

        std::string getObjectName( const Maps::ObjectInfo & info ) override
        {
            switch ( info.objectType ) {
            case MP2::OBJ_RESOURCE:
                return Resource::String( static_cast<int>( info.metadata[0] ) );
            case MP2::OBJ_GENIE_LAMP:
            case MP2::OBJ_RANDOM_RESOURCE:
            case MP2::OBJ_TREASURE_CHEST:
                return MP2::StringObject( info.objectType );
            default:
                // Did you expand the list of treasures? Add the corresponding logic!
                assert( 0 );
                break;
            }

            return {};
        }
    };

    int selectObjectType( const int objectType, const size_t objectCount, ObjectTypeSelection & objectSelection )
    {
        std::vector<int> objects( objectCount, 0 );
        std::iota( objects.begin(), objects.end(), 0 );
        objectSelection.SetListContent( objects );

        objectSelection.SetCurrent( std::max( objectType, 0 ) );

        const int32_t result = objectSelection.selectItemsEventProcessing();
        return result == Dialog::OK || objectSelection.ok ? objectSelection.GetCurrent() : -1;
    }
}

int32_t Dialog::selectKingdomCastle( const Kingdom & kingdom, const bool notOccupiedByHero, std::string title, std::string description /* = {} */,
                                     int32_t castlePositionIndex /* = -1 */ )
{
    std::vector<int32_t> castles;
    const VecCastles & kingdomCastles = kingdom.GetCastles();
    castles.reserve( kingdomCastles.size() );

    for ( const Castle * castle : kingdomCastles ) {
        assert( castle != nullptr );

        if ( notOccupiedByHero && castle->GetHero() ) {
            continue;
        }

        castles.push_back( castle->GetIndex() );
    }

    const int32_t maxHeight = std::min( 100 + SelectKingdomCastle::itemsOffsetY * 12, fheroes2::Display::instance().height() - 200 );
    const int32_t itemsHeight = std::max( 100 + SelectKingdomCastle::itemsOffsetY * static_cast<int32_t>( castles.size() ), 100 + SelectKingdomCastle::itemsOffsetY * 5 );
    const int32_t totalHeight = std::min( itemsHeight, maxHeight );

    SelectKingdomCastle listbox( { 350, totalHeight }, std::move( title ), std::move( description ) );

    listbox.SetListContent( castles );
    if ( castlePositionIndex != -1 ) {
        listbox.SetCurrent( castlePositionIndex );
    }

    const int32_t result = listbox.selectItemsEventProcessing();

    return ( result == Dialog::OK || listbox.ok ) ? listbox.GetCurrent() : -1;
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

    SelectEnumSecSkill listbox( { 350, fheroes2::Display::instance().height() - 200 }, _( "Select Skill:" ) );

    listbox.SetListContent( skills );
    if ( skillId != Skill::Secondary::UNKNOWN ) {
        listbox.SetCurrent( skillId );
    }

    const int32_t result = listbox.selectItemsEventProcessing();

    if ( result == Dialog::OK || listbox.ok ) {
        const int skillIndex = listbox.GetCurrent();
        return { SelectEnumSecSkill::getSkillFromListIndex( skillIndex ), SelectEnumSecSkill::getLevelFromListIndex( skillIndex ) };
    }

    return {};
}

Spell Dialog::selectSpell( const int spellId, const bool includeRandomSpells )
{
    std::vector<int> spells = Spell::getAllSpellIdsSuitableForSpellBook();

    if ( includeRandomSpells ) {
        // We add random spell items to the end of the list.
        for ( int randomSpellId = Spell::RANDOM; randomSpellId <= Spell::RANDOM5; ++randomSpellId ) {
            spells.push_back( randomSpellId );
        }
    }

    SelectEnumSpell listbox( { 340, fheroes2::Display::instance().height() - 200 }, _( "Select Spell:" ) );

    listbox.SetListContent( spells );
    if ( spellId != Spell::NONE ) {
        listbox.SetCurrent( spellId );
    }

    const int32_t result = listbox.selectItemsEventProcessing();

    return result == Dialog::OK || listbox.ok ? Spell( listbox.GetCurrent() ) : Spell( Spell::NONE );
}

Artifact Dialog::selectArtifact( const int artifactId )
{
    std::vector<int> artifacts;
    artifacts.reserve( Artifact::ARTIFACT_COUNT - 1 );

    const bool isPriceofLoyaltyArtifactAllowed = Settings::Get().isCurrentMapPriceOfLoyalty();

    // We show the magic book at the first place.
    artifacts.emplace_back( Artifact::MAGIC_BOOK );

    for ( int id = Artifact::UNKNOWN + 1; id < Artifact::ARTIFACT_COUNT; ++id ) {
        if ( id != Artifact::MAGIC_BOOK && Artifact( id ).isValid() && ( isPriceofLoyaltyArtifactAllowed || !fheroes2::isPriceOfLoyaltyArtifact( id ) ) ) {
            artifacts.emplace_back( id );
        }
    }

    SelectEnumArtifact listbox( { 370, fheroes2::Display::instance().height() - 200 }, _( "Select Artifact:" ) );

    listbox.SetListContent( artifacts );
    if ( artifactId != Artifact::UNKNOWN ) {
        listbox.SetCurrent( artifactId );
    }

    const int32_t result = listbox.selectItemsEventProcessing();

    return ( result == Dialog::OK || listbox.ok ) ? Artifact( listbox.GetCurrent() ) : Artifact( Artifact::UNKNOWN );
}

Monster Dialog::selectMonster( const int monsterId )
{
    std::vector<int> monsters( Monster::MONSTER_COUNT - 1, Monster::UNKNOWN );

    // Skip Monster::UNKNOWN and start from the next one.
    std::iota( monsters.begin(), monsters.end(), Monster::UNKNOWN + 1 );
    monsters.erase( std::remove_if( monsters.begin(), monsters.end(), []( const int id ) { return Monster( id ).isRandomMonster(); } ), monsters.end() );

    SelectEnumMonster listbox( { 280, fheroes2::Display::instance().height() - 200 }, _( "Select Monster:" ) );

    listbox.SetListContent( monsters );
    if ( monsterId != Monster::UNKNOWN ) {
        listbox.SetCurrent( monsterId );
    }

    const int32_t result = listbox.selectItemsEventProcessing();

    return result == Dialog::OK || listbox.ok ? Monster( listbox.GetCurrent() ) : Monster( Monster::UNKNOWN );
}

int Dialog::selectHeroes( const int heroId /* = Heroes::UNKNOWN */ )
{
    std::vector<int> heroes( static_cast<int>( Settings::Get().isCurrentMapPriceOfLoyalty() ? Heroes::JARKONAS : Heroes::BRAX ), Heroes::UNKNOWN );

    std::iota( heroes.begin(), heroes.end(), Heroes::UNKNOWN + 1 );

    SelectEnumHeroes listbox( { 240, fheroes2::Display::instance().height() - 200 }, _( "Select Hero:" ) );

    listbox.SetListContent( heroes );
    if ( heroId != Heroes::UNKNOWN ) {
        listbox.SetCurrent( heroId );
    }

    const int32_t result = listbox.selectItemsEventProcessing();

    return result == Dialog::OK || listbox.ok ? listbox.GetCurrent() : Heroes::UNKNOWN;
}

int Dialog::selectHeroType( const int heroType )
{
    const auto & objectInfo = Maps::getObjectsByGroup( Maps::ObjectGroup::Hero );
    HeroTypeSelection listbox( objectInfo, { 350, fheroes2::Display::instance().height() - 200 }, _( "Select Hero:" ) );

    return selectObjectType( heroType, objectInfo.size(), listbox );
}

int Dialog::selectMonsterType( const int monsterType )
{
    const auto & objectInfo = Maps::getObjectsByGroup( Maps::ObjectGroup::Monster );

    MonsterTypeSelection listbox( objectInfo, { 350, fheroes2::Display::instance().height() - 200 }, _( "Select Monster:" ) );

    return selectObjectType( monsterType, objectInfo.size(), listbox );
}

int Dialog::selectArtifactType( const int artifactType )
{
    const auto & objectInfo = Maps::getObjectsByGroup( Maps::ObjectGroup::Artifact );

    ArtifactTypeSelection listbox( objectInfo, { 350, fheroes2::Display::instance().height() - 200 }, _( "Select Artifact:" ) );

    return selectObjectType( artifactType, objectInfo.size(), listbox );
}

int Dialog::selectTreasureType( const int resourceType )
{
    const auto & objectInfo = Maps::getObjectsByGroup( Maps::ObjectGroup::Treasure );

    TreasureTypeSelection listbox( objectInfo, { 350, fheroes2::Display::instance().height() - 200 }, _( "Select Treasure:" ) );

    return selectObjectType( resourceType, objectInfo.size(), listbox );
}
