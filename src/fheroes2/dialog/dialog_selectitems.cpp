/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
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
#include <iterator>
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
#include "ground.h"
#include "heroes_base.h"
#include "icn.h"
#include "kingdom.h"
#include "localevent.h"
#include "map_object_info.h"
#include "maps.h"
#include "maps_fileinfo.h"
#include "mp2.h"
#include "pal.h"
#include "race.h"
#include "resource.h"
#include "screen.h"
#include "settings.h"
#include "tools.h"
#include "translations.h"
#include "ui_castle.h"
#include "ui_constants.h"
#include "ui_dialog.h"
#include "ui_map_object.h"
#include "ui_scrollbar.h"
#include "ui_text.h"
#include "ui_tool.h"
#include "world.h"

namespace
{
    const int32_t dialogHeightDeduction = 150;

    // Returns the town type according the given player color, race and castle/town state.
    int getPackedTownType( const int townRace, const bool isCastle )
    {
        // NOTICE: This calculation should be consistent with the number of KINGDOM_TOWNS objects.
        return townRace * 2 + ( isCastle ? 0 : 1 );
    }

    class SelectEnumMonster final : public Dialog::ItemSelectionWindow
    {
    public:
        explicit SelectEnumMonster( const fheroes2::Size & rt, std::string title )
            : Dialog::ItemSelectionWindow( rt, std::move( title ) )
        {
            SetAreaMaxItems( rtAreaItems.height / _offsetY );
        }

        using Dialog::ItemSelectionWindow::ActionListPressRight;

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

    class SelectEnumHeroes final : public Dialog::ItemSelectionWindow
    {
    public:
        explicit SelectEnumHeroes( const fheroes2::Size & rt, std::string title )
            : Dialog::ItemSelectionWindow( rt, std::move( title ) )
        {
            SetAreaMaxItems( rtAreaItems.height / _offsetY );
        }

        using Dialog::ItemSelectionWindow::ActionListPressRight;

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

    class SelectEnumArtifact final : public Dialog::ItemSelectionWindow
    {
    public:
        explicit SelectEnumArtifact( const fheroes2::Size & rt, std::string title )
            : Dialog::ItemSelectionWindow( rt, std::move( title ) )
        {
            SetAreaMaxItems( rtAreaItems.height / _offsetY );
        }

        using Dialog::ItemSelectionWindow::ActionListPressRight;

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

    class SelectEnumSpell final : public Dialog::ItemSelectionWindow
    {
    public:
        explicit SelectEnumSpell( const fheroes2::Size & rt, std::string title )
            : Dialog::ItemSelectionWindow( rt, std::move( title ) )
        {
            SetAreaMaxItems( rtAreaItems.height / _offsetY );
        }

        using Dialog::ItemSelectionWindow::ActionListPressRight;

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

    class SelectEnumSecSkill final : public Dialog::ItemSelectionWindow
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
            : Dialog::ItemSelectionWindow( rt, std::move( title ) )
        {
            SetAreaMaxItems( rtAreaItems.height / _offsetY );
        }

        using Dialog::ItemSelectionWindow::ActionListPressRight;

        void RedrawItem( const int & index, int32_t dstx, int32_t dsty, bool current ) override
        {
            const Skill::Secondary skill( getSkillFromListIndex( index ), getLevelFromListIndex( index ) );
            const fheroes2::Sprite & skillSprite = fheroes2::AGG::GetICN( ICN::MINISS, skill.GetIndexSprite2() );

            renderItem( skillSprite, skill.GetName(), { dstx, dsty }, 45 / 2, 50, _offsetY / 2, current );
        }

        void ActionListPressRight( int & index ) override
        {
            fheroes2::SecondarySkillDialogElement( Skill::Secondary( getSkillFromListIndex( index ), getLevelFromListIndex( index ) ), Heroes() )
                .showPopup( Dialog::ZERO );
        }

    private:
        static const int32_t _offsetY{ 42 };
    };

    class SelectKingdomCastle final : public Dialog::ItemSelectionWindow
    {
    public:
        explicit SelectKingdomCastle( const fheroes2::Size & rt, std::string title, std::string description )
            : Dialog::ItemSelectionWindow( rt, std::move( title ), std::move( description ) )
            , _townFrameIcnId( Settings::Get().isEvilInterfaceEnabled() ? ICN::LOCATORE : ICN::LOCATORS )
        {
            SetAreaMaxItems( rtAreaItems.height / itemsOffsetY );
        }

        using Dialog::ItemSelectionWindow::ActionListPressRight;

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
            Dialog::QuickInfoWithIndicationOnRadar( *world.getCastleEntrance( Maps::GetPoint( index ) ), getBackgroundArea() );
        }

        static const int32_t itemsOffsetY{ 35 };

    private:
        const int _townFrameIcnId;
    };

    // This is a base class for items used in the Editor and they rely on Maps::ObjectInfo structures.
    class ObjectTypeSelection : public Dialog::ItemSelectionWindow
    {
    public:
        ObjectTypeSelection( const std::vector<Maps::ObjectInfo> & objectInfo, const fheroes2::Size & size, std::string title, const int32_t imageOffsetX,
                             const int32_t textOffsetX, const int32_t offsetY )
            : Dialog::ItemSelectionWindow( size, std::move( title ) )
            , _objectInfo( objectInfo )
            , _imageOffsetX( imageOffsetX )
            , _textOffsetX( textOffsetX )
            , _offsetY( offsetY )
        {
            SetAreaMaxItems( rtAreaItems.height / _offsetY );
        }

        using Dialog::ItemSelectionWindow::ActionListPressRight;

        void RedrawItem( const int & objectId, int32_t posX, int32_t posY, bool isSelected ) override
        {
            // If this assertion blows up then you are setting different number of items.
            assert( objectId >= 0 && objectId < static_cast<int>( _objectInfo.size() ) );

            const fheroes2::Sprite & image = fheroes2::generateMapObjectImage( _objectInfo[objectId] );
            const int32_t imageHeight = image.height();
            const int32_t imageWidth = image.width();
            if ( imageHeight > fheroes2::tileWidthPx * 3 || imageWidth > fheroes2::tileWidthPx * 5 ) {
                // Reduce the size of very tall images to fit the list.
                const double ratio = std::max( imageHeight / ( fheroes2::tileWidthPx * 3. ), imageWidth / ( fheroes2::tileWidthPx * 5. ) );
                fheroes2::Image resized( static_cast<int32_t>( imageWidth / ratio ), static_cast<int32_t>( imageHeight / ratio ) );
                fheroes2::Resize( image, resized );
                renderItem( resized, getObjectName( _objectInfo[objectId] ), { posX, posY }, _imageOffsetX, _textOffsetX, _offsetY / 2, isSelected );
            }
            else {
                renderItem( image, getObjectName( _objectInfo[objectId] ), { posX, posY }, _imageOffsetX, _textOffsetX, _offsetY / 2, isSelected );
            }
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

    class MonsterTypeSelection final : public ObjectTypeSelection
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

    class ArtifactTypeSelection final : public ObjectTypeSelection
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

    class TreasureTypeSelection final : public ObjectTypeSelection
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
            case MP2::OBJ_CAMPFIRE:
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
            case MP2::OBJ_CAMPFIRE:
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

    class OceanObjectTypeSelection final : public ObjectTypeSelection
    {
    public:
        OceanObjectTypeSelection( const std::vector<Maps::ObjectInfo> & objectInfo, const fheroes2::Size & size, std::string title )
            : ObjectTypeSelection( objectInfo, size, std::move( title ), 3 * 32 / 2, 3 * 32 + 10, 2 * 32 + 24 )
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
            if ( info.objectType == MP2::OBJ_NONE ) {
                return _( "Terrain object" );
            }

            return MP2::StringObject( info.objectType );
        }
    };

    class GenericObjectTypeSelection final : public ObjectTypeSelection
    {
    public:
        GenericObjectTypeSelection( const std::vector<Maps::ObjectInfo> & objectInfo, const fheroes2::Size & size, std::string title )
            : ObjectTypeSelection( objectInfo, size, std::move( title ), 6 * 32 / 2, 6 * 32 + 10, 3 * 32 )
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
            if ( info.objectType == MP2::OBJ_NONE ) {
                return _( "Terrain object" );
            }

            // Barriers and Traveller's Tents use the same name while we need to show their color as well.
            if ( info.objectType == MP2::OBJ_BARRIER ) {
                std::string str = _( "%{color} Barrier" );
                StringReplace( str, "%{color}", fheroes2::getBarrierColorName( static_cast<int32_t>( info.metadata[0] ) ) );
                return str;
            }

            if ( info.objectType == MP2::OBJ_TRAVELLER_TENT ) {
                std::string str = _( "%{color} Tent" );
                StringReplace( str, "%{color}", fheroes2::getTentColorName( static_cast<int32_t>( info.metadata[0] ) ) );
                return str;
            }

            return MP2::StringObject( info.objectType );
        }
    };

    class PowerUpObjectTypeSelection final : public ObjectTypeSelection
    {
    public:
        PowerUpObjectTypeSelection( const std::vector<Maps::ObjectInfo> & objectInfo, const fheroes2::Size & size, std::string title )
            : ObjectTypeSelection( objectInfo, size, std::move( title ), 4 * 32 / 2, 4 * 32 + 10, 3 * 32 )
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
            return MP2::StringObject( info.objectType );
        }
    };

    int selectObjectType( const int objectType, const size_t objectCount, ObjectTypeSelection & objectSelection )
    {
        std::vector<int> objects( objectCount, 0 );
        std::iota( objects.begin(), objects.end(), 0 );
        objectSelection.SetListContent( objects );

        objectSelection.SetCurrent( std::max( objectType, 0 ) );

        const int32_t result = objectSelection.selectItemsEventProcessing();
        return result == Dialog::OK ? objectSelection.GetCurrent() : -1;
    }

    class MineTypeList final : public Interface::ListBox<Maps::ObjectInfo>
    {
    public:
        using Interface::ListBox<Maps::ObjectInfo>::ActionListDoubleClick;
        using Interface::ListBox<Maps::ObjectInfo>::ActionListSingleClick;
        using Interface::ListBox<Maps::ObjectInfo>::ActionListPressRight;

        using ListBox::ListBox;

        void ActionListPressRight( Maps::ObjectInfo & info ) override
        {
            if ( info.objectType == MP2::OBJ_MINE ) {
                // Mine description is done by its resource type stored in metadata.
                fheroes2::showStandardTextMessage( Maps::GetMineName( static_cast<int>( info.metadata[0] ) ), "", Dialog::ZERO );
                return;
            }
            fheroes2::showStandardTextMessage( MP2::StringObject( info.objectType ), "", Dialog::ZERO );
        }

        int getCurrentId() const
        {
            return _currentId;
        }

        void initListBackgroundRestorer( fheroes2::Rect roi )
        {
            _listBackground = std::make_unique<fheroes2::ImageRestorer>( fheroes2::Display::instance(), roi.x, roi.y, roi.width, roi.height );
        }

        bool isDoubleClicked() const
        {
            return _isDoubleClicked;
        }

        void updateScrollBarImage()
        {
            const int32_t scrollBarWidth = _scrollbar.width();

            setScrollBarImage( fheroes2::generateScrollbarSlider( _scrollbar, false, _scrollbar.getArea().height, VisibleItemCount(), _size(),
                                                                  { 0, 0, scrollBarWidth, 8 }, { 0, 7, scrollBarWidth, 8 } ) );
            _scrollbar.moveToIndex( _topId );
        }

        void RedrawItem( const Maps::ObjectInfo & info, int32_t dstx, int32_t dsty, bool current ) override
        {
            if ( _color < 0 ) {
                assert( 0 );
                return;
            }

            const fheroes2::Sprite mineSprite( fheroes2::generateMapObjectImage( info ) );

            fheroes2::Display & display = fheroes2::Display::instance();

            fheroes2::Blit( mineSprite, display, dstx + fheroes2::tileWidthPx * 5 + 5 - mineSprite.width(), dsty + 1 );

            // Mine type selection mark background.
            const fheroes2::Sprite & markBackground = fheroes2::AGG::GetICN( Settings::Get().isEvilInterfaceEnabled() ? ICN::CELLWIN_EVIL : ICN::CELLWIN, 4 );
            fheroes2::Blit( markBackground, display, dstx + 10, dsty + 24 );

            if ( current ) {
                const fheroes2::Sprite & mark = fheroes2::AGG::GetICN( ICN::CELLWIN, 5 );
                fheroes2::Blit( mark, display, dstx + 13, dsty + 27 );
            }
        }

        void RedrawBackground( const fheroes2::Point & /* unused */ ) override
        {
            _listBackground->restore();
        }

        void ActionCurrentUp() override
        {
            // Do nothing.
        }

        void ActionCurrentDn() override
        {
            // Do nothing.
        }

        void ActionListDoubleClick( Maps::ObjectInfo & /*unused*/ ) override
        {
            _isDoubleClicked = true;
        }

        void ActionListSingleClick( Maps::ObjectInfo & /*unused*/ ) override
        {
            // Do nothing.
        }

        void setOwnerColor( const int32_t color )
        {
            _color = color;
        }

    private:
        bool _isDoubleClicked{ false };
        int32_t _color{ 0 };
        std::unique_ptr<fheroes2::ImageRestorer> _listBackground;
    };
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

    const int32_t maxHeight = std::min( 100 + SelectKingdomCastle::itemsOffsetY * 12, fheroes2::Display::instance().height() - dialogHeightDeduction );
    const int32_t itemsHeight = std::max( 100 + SelectKingdomCastle::itemsOffsetY * static_cast<int32_t>( castles.size() ), 100 + SelectKingdomCastle::itemsOffsetY * 5 );
    const int32_t totalHeight = std::min( itemsHeight, maxHeight );

    SelectKingdomCastle listbox( { 350, totalHeight }, std::move( title ), std::move( description ) );

    listbox.SetListContent( castles );
    if ( castlePositionIndex != -1 ) {
        listbox.SetCurrent( castlePositionIndex );
    }

    const int32_t result = listbox.selectItemsEventProcessing();

    return ( result == Dialog::OK ) ? listbox.GetCurrent() : -1;
}

namespace Dialog
{
    ItemSelectionWindow::ItemSelectionWindow( const fheroes2::Size & dialogSize, std::string title, std::string description )
    {
        assert( !title.empty() );

        fheroes2::Display & display = fheroes2::Display::instance();
        _window = std::make_unique<fheroes2::StandardWindow>( dialogSize.width, dialogSize.height, true, display );

        const fheroes2::Rect & area = _window->activeArea();

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

        _window->applyTextBackgroundShading( listRoi );

        _backgroundRestorer = std::make_unique<fheroes2::ImageRestorer>( display, listRoi.x, listRoi.y, listRoi.width, listRoi.height );

        SetAreaItems( { listRoi.x + 5, listRoi.y + 5, listRoi.width - 10, listRoi.height - 10 } );

        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();
        const int32_t scrollbarOffsetX = area.x + area.width - 25;

        _window->renderScrollbarBackground( { scrollbarOffsetX, listRoi.y, listRoi.width, listRoi.height }, isEvilInterface );

        const int32_t topPartHeight = 19;
        const int listIcnId = isEvilInterface ? ICN::SCROLLE : ICN::SCROLL;

        SetScrollButtonUp( listIcnId, 0, 1, { scrollbarOffsetX + 1, listRoi.y + 1 } );
        SetScrollButtonDn( listIcnId, 2, 3, { scrollbarOffsetX + 1, listRoi.y + listRoi.height - 15 } );
        setScrollBarArea( { scrollbarOffsetX + 3, listRoi.y + topPartHeight, 10, listRoi.height - 2 * topPartHeight } );
        setScrollBarImage( fheroes2::AGG::GetICN( listIcnId, 4 ) );

        // Render dialog buttons.
        _window->renderOkayCancelButtons( _buttonOk, _buttonCancel, isEvilInterface );
    }

    void ItemSelectionWindow::updateScrollBarImage()
    {
        const int32_t scrollBarWidth = _scrollbar.width();

        setScrollBarImage( fheroes2::generateScrollbarSlider( _scrollbar, false, _scrollbar.getArea().height, VisibleItemCount(), _size(), { 0, 0, scrollBarWidth, 8 },
                                                              { 0, 7, scrollBarWidth, 8 } ) );
        _scrollbar.moveToIndex( _topId );
    }

    // An image with text should have offset of 10 pixels from all left and right edges.
    void ItemSelectionWindow::renderItem( const fheroes2::Sprite & itemSprite, std::string itemText, const fheroes2::Point & destination,
                                          const int32_t middleImageOffsetX, const int32_t textOffsetX, const int32_t itemOffsetY, const bool current ) const
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        fheroes2::Blit( itemSprite, display, destination.x + middleImageOffsetX - ( itemSprite.width() / 2 ), destination.y + itemOffsetY - ( itemSprite.height() / 2 ) );

        fheroes2::Text text( std::move( itemText ), current ? fheroes2::FontType::normalYellow() : fheroes2::FontType::normalWhite() );
        text.fitToOneRow( _window->activeArea().width - textOffsetX - 55 );
        text.draw( destination.x + textOffsetX, destination.y + itemOffsetY - ( text.height() / 2 ) + 2, display );
    }

    int32_t ItemSelectionWindow::selectItemsEventProcessing()
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        // setup cursor
        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        const fheroes2::Rect & roi = _window->activeArea();

        updateScrollBarImage();

        Redraw();
        display.render( _window->totalArea() );

        LocalEvent & le = LocalEvent::Get();

        while ( !_isDoubleClicked && le.HandleEvents() ) {
            _buttonOk.drawOnState( le.isMouseLeftButtonPressedInArea( _buttonOk.area() ) );
            _buttonCancel.drawOnState( le.isMouseLeftButtonPressedInArea( _buttonCancel.area() ) );

            if ( le.MouseClickLeft( _buttonOk.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) ) {
                return Dialog::OK;
            }
            if ( le.MouseClickLeft( _buttonCancel.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
                return Dialog::CANCEL;
            }

            if ( le.isMouseRightButtonPressedInArea( _buttonOk.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Okay" ), _( "Accept the choice made." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( _buttonCancel.area() ) ) {
                fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Exit this menu without doing anything." ), Dialog::ZERO );
            }

            QueueEventProcessing();

            if ( !IsNeedRedraw() ) {
                continue;
            }

            Redraw();
            display.render( roi );
        }

        if ( _isDoubleClicked ) {
            return Dialog::OK;
        }

        return Dialog::ZERO;
    }

    fheroes2::Rect ItemSelectionWindow::getBackgroundArea() const
    {
        assert( _window );
        return _window->totalArea();
    }
}

Skill::Secondary Dialog::selectSecondarySkill( const Heroes & hero, const int skillId /* = Skill::Secondary::UNKNOWN */ )
{
    std::vector<int> skills;
    skills.reserve( static_cast<size_t>( Skill::numOfSecondarySkills ) * 3 );

    for ( int i = 0; i < Skill::numOfSecondarySkills * 3; ++i ) {
        if ( !hero.HasSecondarySkill( SelectEnumSecSkill::getSkillFromListIndex( i ) ) ) {
            skills.push_back( i );
        }
    }

    SelectEnumSecSkill listbox( { 350, fheroes2::Display::instance().height() - dialogHeightDeduction }, _( "Select Skill:" ) );

    listbox.SetListContent( skills );
    if ( skillId != Skill::Secondary::UNKNOWN ) {
        listbox.SetCurrent( skillId );
    }

    const int32_t result = listbox.selectItemsEventProcessing();

    if ( result == Dialog::OK ) {
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

    SelectEnumSpell listbox( { 340, fheroes2::Display::instance().height() - dialogHeightDeduction }, _( "Select Spell:" ) );

    listbox.SetListContent( spells );
    if ( spellId != Spell::NONE ) {
        listbox.SetCurrent( spellId );
    }

    const int32_t result = listbox.selectItemsEventProcessing();

    return ( result == Dialog::OK ) ? Spell( listbox.GetCurrent() ) : Spell( Spell::NONE );
}

Artifact Dialog::selectArtifact( const int artifactId, const bool isForVictoryConditions )
{
    std::vector<int> artifacts;
    artifacts.reserve( Artifact::ARTIFACT_COUNT - 2 );

    const GameVersion version = Settings::Get().getCurrentMapInfo().version;
    const bool isPriceofLoyaltyArtifactAllowed = ( version == GameVersion::PRICE_OF_LOYALTY || version == GameVersion::RESURRECTION );

    if ( !isForVictoryConditions ) {
        // We show the Magic Book at the first place.
        artifacts.emplace_back( Artifact::MAGIC_BOOK );
    }
    else {
        // We show the Ultimate Artifact at the first place.
        artifacts.emplace_back( Artifact::EDITOR_ANY_ULTIMATE_ARTIFACT );
    }

    for ( int id = Artifact::UNKNOWN + 1; id < Artifact::ARTIFACT_COUNT; ++id ) {
        if ( id == Artifact::MAGIC_BOOK ) {
            continue;
        }

        if ( isForVictoryConditions && id == Artifact::SPELL_SCROLL ) {
            continue;
        }

        if ( !Artifact( id ).isValid() ) {
            continue;
        }

        if ( !isPriceofLoyaltyArtifactAllowed && fheroes2::isPriceOfLoyaltyArtifact( id ) ) {
            continue;
        }

        artifacts.emplace_back( id );
    }

    SelectEnumArtifact listbox( { 370, fheroes2::Display::instance().height() - dialogHeightDeduction }, _( "Select Artifact:" ) );

    listbox.SetListContent( artifacts );
    if ( artifactId != Artifact::UNKNOWN ) {
        listbox.SetCurrent( artifactId );
    }

    const int32_t result = listbox.selectItemsEventProcessing();

    return ( result == Dialog::OK ) ? Artifact( listbox.GetCurrent() ) : Artifact( Artifact::UNKNOWN );
}

Monster Dialog::selectMonster( const int monsterId )
{
    std::vector<int> monsters( Monster::MONSTER_COUNT - 1, Monster::UNKNOWN );

    // Skip Monster::UNKNOWN and start from the next one.
    std::iota( monsters.begin(), monsters.end(), Monster::UNKNOWN + 1 );
    monsters.erase( std::remove_if( monsters.begin(), monsters.end(), []( const int id ) { return Monster( id ).isRandomMonster(); } ), monsters.end() );

    SelectEnumMonster listbox( { 320, fheroes2::Display::instance().height() - dialogHeightDeduction }, _( "Select Monster:" ) );

    listbox.SetListContent( monsters );
    if ( monsterId != Monster::UNKNOWN ) {
        listbox.SetCurrent( monsterId );
    }

    const int32_t result = listbox.selectItemsEventProcessing();

    return ( result == Dialog::OK ) ? Monster( listbox.GetCurrent() ) : Monster( Monster::UNKNOWN );
}

int Dialog::selectHeroes( const int heroId /* = Heroes::UNKNOWN */ )
{
    const GameVersion version = Settings::Get().getCurrentMapInfo().version;
    const bool isPoLHeroesAllowed = ( version == GameVersion::PRICE_OF_LOYALTY || version == GameVersion::RESURRECTION );

    std::vector<int> heroes( static_cast<int>( isPoLHeroesAllowed ? Heroes::JARKONAS : Heroes::BRAX ), Heroes::UNKNOWN );

    std::iota( heroes.begin(), heroes.end(), Heroes::UNKNOWN + 1 );

    SelectEnumHeroes listbox( { 300, fheroes2::Display::instance().height() - dialogHeightDeduction }, _( "Select Hero:" ) );

    listbox.SetListContent( heroes );
    if ( heroId != Heroes::UNKNOWN ) {
        listbox.SetCurrent( heroId );
    }

    const int32_t result = listbox.selectItemsEventProcessing();

    return ( result == Dialog::OK ) ? listbox.GetCurrent() : Heroes::UNKNOWN;
}

int Dialog::selectHeroType( const int heroType )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    fheroes2::StandardWindow background( 520, 250, true, display );

    const fheroes2::Rect & area = background.activeArea();

    fheroes2::Text text( _( "Select Hero:" ), fheroes2::FontType::normalYellow() );
    text.draw( area.x + ( area.width - text.width() ) / 2, area.y + 10, display );

    // Render color and race selection sprites.
    const int32_t stepX = 70;
    const int32_t raceOffsetY = 80;
    fheroes2::Point pos( area.x + 20, area.y + 30 );
    const fheroes2::Sprite & colorSpriteBorderSelected = fheroes2::AGG::GetICN( ICN::BRCREST, 6 );
    const fheroes2::Sprite & raceShadow = fheroes2::AGG::GetICN( ICN::NGEXTRA, 61 );
    fheroes2::Sprite colorSpriteBorder( colorSpriteBorderSelected );

    const std::vector<uint8_t> darkGrayPalette = PAL::CombinePalettes( PAL::GetPalette( PAL::PaletteType::GRAY ), PAL::GetPalette( PAL::PaletteType::DARKENING ) );
    fheroes2::ApplyPalette( colorSpriteBorder, darkGrayPalette );

    // Make race selection borders.
    const fheroes2::Sprite & randomRaceSprite = fheroes2::AGG::GetICN( ICN::NGEXTRA, 58 );
    const int32_t raceBorderWidth = randomRaceSprite.width();
    const int32_t raceBorderHeight = randomRaceSprite.height();
    fheroes2::Image raceSpriteBorder( raceBorderWidth, raceBorderHeight );
    raceSpriteBorder.reset();
    const int32_t raceImageWidth = raceBorderWidth - 8;
    const int32_t raceImageHeight = raceBorderHeight - 8;
    fheroes2::Copy( randomRaceSprite, 0, 0, raceSpriteBorder, 0, 0, raceBorderWidth, 4 );
    fheroes2::Copy( randomRaceSprite, 0, 4, raceSpriteBorder, 0, 4, 4, raceImageHeight );
    int32_t imageCopyOffset = raceBorderWidth - 4;
    fheroes2::Copy( randomRaceSprite, imageCopyOffset, 4, raceSpriteBorder, imageCopyOffset, 4, 4, raceImageHeight );
    imageCopyOffset = raceBorderHeight - 4;
    fheroes2::Copy( randomRaceSprite, 0, imageCopyOffset, raceSpriteBorder, 0, imageCopyOffset, raceBorderWidth, 4 );
    fheroes2::Image raceSpriteBorderSelected( raceSpriteBorder );
    fheroes2::ApplyPalette( raceSpriteBorder, darkGrayPalette );
    fheroes2::ApplyPalette( raceSpriteBorderSelected, PAL::GetPalette( PAL::PaletteType::RED ) );

    std::array<fheroes2::Rect, 6> colorRect;
    std::array<fheroes2::Rect, 7> raceRect;

    // Random race.
    raceRect.back() = { pos.x - 2, pos.y + raceOffsetY, raceBorderWidth, raceBorderHeight };
    fheroes2::Copy( randomRaceSprite, 4, 4, display, raceRect.back().x + 4, raceRect.back().y + 4, raceImageWidth, raceImageHeight );
    fheroes2::Blit( raceShadow, display, pos.x - 5, pos.y + raceOffsetY + 2 );
    text.set( _( "race|Random" ), fheroes2::FontType::smallWhite() );
    text.draw( raceRect.back().x, raceRect.back().y + raceRect.back().height + 4, raceBorderWidth, display );

    pos.x += stepX;
    for ( uint32_t i = 0; i < 6; ++i ) {
        const fheroes2::Sprite & colorSprite = fheroes2::AGG::GetICN( ICN::BRCREST, i );

        fheroes2::addGradientShadow( colorSpriteBorder, display, { pos.x - stepX / 2, pos.y }, { -5, 5 } );
        colorRect[i] = { pos.x - stepX / 2, pos.y, colorSpriteBorder.width(), colorSpriteBorder.height() };
        fheroes2::Copy( colorSprite, 0, 0, display, colorRect[i].x + 4, colorRect[i].y + 4, colorSprite.width(), colorSprite.height() );
        text.set( Color::String( Color::IndexToColor( static_cast<int>( i ) ) ), fheroes2::FontType::smallWhite() );
        text.draw( colorRect[i].x + ( colorRect[i].width - text.width() ) / 2, colorRect[i].y + colorRect[i].height + 4, display );

        const fheroes2::Sprite & raceSprite = fheroes2::AGG::GetICN( ICN::NGEXTRA, 51U + i );
        raceRect[i] = { pos.x - 2, pos.y + raceOffsetY, raceBorderWidth, raceBorderHeight };
        fheroes2::Copy( raceSprite, 4, 4, display, raceRect[i].x + 4, raceRect[i].y + 4, raceImageWidth, raceImageHeight );
        fheroes2::Blit( raceShadow, display, pos.x - 5, pos.y + raceOffsetY + 2 );
        text.set( Race::DoubleLinedString( Race::IndexToRace( static_cast<int>( i ) ) ), fheroes2::FontType::smallWhite() );
        text.draw( raceRect[i].x, raceRect[i].y + raceRect[i].height + 4, raceBorderWidth, display );

        pos.x += stepX;
    }

    pos.x = area.x + area.width / 2;
    pos.y += 175;

    const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

    // Render dialog buttons.
    fheroes2::Button buttonOk;
    fheroes2::Button buttonCancel;
    background.renderOkayCancelButtons( buttonOk, buttonCancel, isEvilInterface );

    const auto & objectInfo = Maps::getObjectsByGroup( Maps::ObjectGroup::KINGDOM_HEROES );
    fheroes2::Rect heroRoi;

    {
        // We are doing this is a code block to reduce variable scope.
        // This calculation is done with an assumption that all hero images are the same.
        const fheroes2::Sprite heroImage = fheroes2::generateMapObjectImage( objectInfo[0] );
        heroRoi = { heroImage.x(), heroImage.y(), heroImage.width(), heroImage.height() };
    }

    fheroes2::ImageRestorer heroBackground( display, pos.x + heroRoi.x, pos.y + heroRoi.y, heroRoi.width, heroRoi.height );

    LocalEvent & le = LocalEvent::Get();
    bool needRedraw = true;
    int heroRace = 0;
    int heroColor = 0;

    if ( heroType > -1 ) {
        heroRace = heroType % 7;
        heroColor = heroType / 7;
    }

    while ( le.HandleEvents() ) {
        le.isMouseLeftButtonPressedInArea( buttonOk.area() ) ? buttonOk.drawOnPress() : buttonOk.drawOnRelease();
        le.isMouseLeftButtonPressedInArea( buttonCancel.area() ) ? buttonCancel.drawOnPress() : buttonCancel.drawOnRelease();

        if ( le.MouseClickLeft( buttonOk.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) ) {
            return heroColor * 7 + heroRace;
        }
        if ( le.MouseClickLeft( buttonCancel.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
            break;
        }

        if ( le.isMouseRightButtonPressedInArea( buttonCancel.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Exit this menu without doing anything." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( buttonOk.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Okay" ), _( "Click to start placing the selected hero." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( heroBackground.rect() ) ) {
            std::string name( _( "%{color} %{race}" ) );
            StringReplace( name, "%{color}", Color::String( Color::IndexToColor( heroColor ) ) );
            StringReplace( name, "%{race}", ( heroRace < 6 ) ? Race::String( Race::IndexToRace( heroRace ) ) : _( "race|Random" ) );
            fheroes2::showStandardTextMessage( _( "You will place" ), std::move( name ), Dialog::ZERO );
        }
        else {
            for ( size_t i = 0; i < 7; ++i ) {
                if ( le.MouseClickLeft( raceRect[i] ) ) {
                    heroRace = static_cast<int>( i );
                    needRedraw = true;
                    break;
                }

                if ( le.isMouseRightButtonPressedInArea( raceRect[i] ) ) {
                    fheroes2::showStandardTextMessage( Race::String( Race::IndexToRace( static_cast<int>( i == 6 ? 7 : i ) ) ), _( "Click to select this class." ),
                                                       Dialog::ZERO );
                }
            }

            for ( size_t i = 0; i < 6; ++i ) {
                if ( le.MouseClickLeft( colorRect[i] ) ) {
                    heroColor = static_cast<int>( i );
                    needRedraw = true;
                    break;
                }

                if ( le.isMouseRightButtonPressedInArea( colorRect[i] ) ) {
                    fheroes2::showStandardTextMessage( Color::String( Color::IndexToColor( static_cast<int>( i ) ) ), _( "Click to select this color." ), Dialog::ZERO );
                }
            }
        }

        if ( !needRedraw ) {
            continue;
        }

        heroBackground.restore();

        // Update hero image.
        const fheroes2::Sprite heroImage = fheroes2::generateMapObjectImage( objectInfo[heroColor * 7 + heroRace] );
        fheroes2::Blit( heroImage, display, pos.x + heroImage.x(), pos.y + heroImage.y() );

        // Update color and race borders.
        for ( int i = 0; i < 6; ++i ) {
            fheroes2::Blit( ( i == heroColor ) ? colorSpriteBorderSelected : colorSpriteBorder, display, colorRect[i].x, colorRect[i].y );
        }

        for ( int i = 0; i < 7; ++i ) {
            fheroes2::Blit( ( i == heroRace ) ? raceSpriteBorderSelected : raceSpriteBorder, display, raceRect[i].x, raceRect[i].y );
        }

        display.render( background.totalArea() );

        needRedraw = false;
    }

    return Heroes::UNKNOWN;
}

int Dialog::selectMonsterType( const int monsterType )
{
    const auto & objectInfo = Maps::getObjectsByGroup( Maps::ObjectGroup::MONSTERS );

    MonsterTypeSelection listbox( objectInfo, { 350, fheroes2::Display::instance().height() - dialogHeightDeduction }, _( "Select Monster:" ) );

    return selectObjectType( monsterType, objectInfo.size(), listbox );
}

int Dialog::selectArtifactType( const int artifactType )
{
    const auto & objectInfo = Maps::getObjectsByGroup( Maps::ObjectGroup::ADVENTURE_ARTIFACTS );

    ArtifactTypeSelection listbox( objectInfo, { 350, fheroes2::Display::instance().height() - dialogHeightDeduction }, _( "Select Artifact:" ) );

    return selectObjectType( artifactType, objectInfo.size(), listbox );
}

int Dialog::selectTreasureType( const int resourceType )
{
    const auto & objectInfo = Maps::getObjectsByGroup( Maps::ObjectGroup::ADVENTURE_TREASURES );

    TreasureTypeSelection listbox( objectInfo, { 350, fheroes2::Display::instance().height() - dialogHeightDeduction }, _( "Select Treasure:" ) );

    return selectObjectType( resourceType, objectInfo.size(), listbox );
}

int Dialog::selectOceanObjectType( const int objectType )
{
    const auto & objectInfo = Maps::getObjectsByGroup( Maps::ObjectGroup::ADVENTURE_WATER );

    OceanObjectTypeSelection listbox( objectInfo, { 350, fheroes2::Display::instance().height() - dialogHeightDeduction }, _( "Select Ocean Object:" ) );

    return selectObjectType( objectType, objectInfo.size(), listbox );
}

int Dialog::selectLandscapeOceanObjectType( const int objectType )
{
    const auto & objectInfo = Maps::getObjectsByGroup( Maps::ObjectGroup::LANDSCAPE_WATER );

    OceanObjectTypeSelection listbox( objectInfo, { 350, fheroes2::Display::instance().height() - dialogHeightDeduction }, _( "Select Ocean Object:" ) );

    return selectObjectType( objectType, objectInfo.size(), listbox );
}

void Dialog::selectTownType( int & type, int & color )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    fheroes2::StandardWindow background( 520, 370, true, display );

    const fheroes2::Rect & area = background.activeArea();

    fheroes2::Text text( _( "Castle/town placing" ), fheroes2::FontType::normalYellow() );
    text.draw( area.x + ( area.width - text.width() ) / 2, area.y + 10, display );

    // Render color and race selection sprites.
    const int32_t stepX = 70;
    const int32_t raceOffsetY = 80;
    fheroes2::Point pos( area.x + 20, area.y + 30 );
    const fheroes2::Sprite & colorSpriteBorderSelected = fheroes2::AGG::GetICN( ICN::BRCREST, 6 );
    const fheroes2::Sprite & raceShadow = fheroes2::AGG::GetICN( ICN::NGEXTRA, 61 );
    fheroes2::Sprite colorSpriteBorder( colorSpriteBorderSelected );

    const std::vector<uint8_t> darkGrayPalette = PAL::CombinePalettes( PAL::GetPalette( PAL::PaletteType::GRAY ), PAL::GetPalette( PAL::PaletteType::DARKENING ) );
    fheroes2::ApplyPalette( colorSpriteBorder, darkGrayPalette );

    // Make race selection borders.
    const fheroes2::Sprite & randomRaceSprite = fheroes2::AGG::GetICN( ICN::NGEXTRA, 58 );
    const int32_t raceBorderWidth = randomRaceSprite.width();
    const int32_t raceBorderHeight = randomRaceSprite.height();
    fheroes2::Image raceSpriteBorder( raceBorderWidth, raceBorderHeight );
    raceSpriteBorder.reset();
    const int32_t raceImageWidth = raceBorderWidth - 8;
    const int32_t raceImageHeight = raceBorderHeight - 8;
    fheroes2::Copy( randomRaceSprite, 0, 0, raceSpriteBorder, 0, 0, raceBorderWidth, 4 );
    fheroes2::Copy( randomRaceSprite, 0, 4, raceSpriteBorder, 0, 4, 4, raceImageHeight );
    int32_t imageCopyOffset = raceBorderWidth - 4;
    fheroes2::Copy( randomRaceSprite, imageCopyOffset, 4, raceSpriteBorder, imageCopyOffset, 4, 4, raceImageHeight );
    imageCopyOffset = raceBorderHeight - 4;
    fheroes2::Copy( randomRaceSprite, 0, imageCopyOffset, raceSpriteBorder, 0, imageCopyOffset, raceBorderWidth, 4 );
    fheroes2::Image raceSpriteBorderSelected( raceSpriteBorder );
    fheroes2::ApplyPalette( raceSpriteBorder, darkGrayPalette );
    fheroes2::ApplyPalette( raceSpriteBorderSelected, PAL::GetPalette( PAL::PaletteType::RED ) );

    std::array<fheroes2::Rect, 7> colorRect;
    std::array<fheroes2::Rect, 7> raceRect;

    // Neutral color.
    fheroes2::addGradientShadow( colorSpriteBorder, display, pos, { -5, 5 } );
    const fheroes2::Sprite & neutralColorSprite = fheroes2::AGG::GetICN( ICN::BRCREST, 7 );
    colorRect.back() = { pos.x, pos.y, colorSpriteBorder.width(), colorSpriteBorder.height() };
    fheroes2::Copy( neutralColorSprite, 0, 0, display, colorRect.back().x + 4, colorRect.back().y + 4, neutralColorSprite.width(), neutralColorSprite.height() );
    text.set( _( "doubleLinedRace|Neutral" ), fheroes2::FontType::smallWhite() );
    text.draw( colorRect.back().x + ( colorRect.back().width - text.width() ) / 2, colorRect.back().y + colorRect.back().height + 4, display );

    // Random race.
    raceRect.back() = { pos.x - 2, pos.y + raceOffsetY, raceBorderWidth, raceBorderHeight };
    fheroes2::Copy( randomRaceSprite, 4, 4, display, raceRect.back().x + 4, raceRect.back().y + 4, raceImageWidth, raceImageHeight );
    fheroes2::Blit( raceShadow, display, pos.x - 5, pos.y + raceOffsetY + 2 );
    text.set( _( "race|Random" ), fheroes2::FontType::smallWhite() );
    text.draw( raceRect.back().x, raceRect.back().y + raceRect.back().height + 4, raceBorderWidth, display );

    pos.x += stepX;
    for ( uint32_t i = 0; i < 6; ++i ) {
        const fheroes2::Sprite & colorSprite = fheroes2::AGG::GetICN( ICN::BRCREST, i );

        fheroes2::addGradientShadow( colorSpriteBorder, display, pos, { -5, 5 } );
        colorRect[i] = { pos.x, pos.y, colorSpriteBorder.width(), colorSpriteBorder.height() };
        fheroes2::Copy( colorSprite, 0, 0, display, colorRect[i].x + 4, colorRect[i].y + 4, colorSprite.width(), colorSprite.height() );
        text.set( Color::String( Color::IndexToColor( static_cast<int>( i ) ) ), fheroes2::FontType::smallWhite() );
        text.draw( colorRect[i].x + ( colorRect[i].width - text.width() ) / 2, colorRect[i].y + colorRect[i].height + 4, display );

        const fheroes2::Sprite & raceSprite = fheroes2::AGG::GetICN( ICN::NGEXTRA, 51U + i );
        raceRect[i] = { pos.x - 2, pos.y + raceOffsetY, raceBorderWidth, raceBorderHeight };
        fheroes2::Copy( raceSprite, 4, 4, display, raceRect[i].x + 4, raceRect[i].y + 4, raceImageWidth, raceImageHeight );
        fheroes2::Blit( raceShadow, display, pos.x - 5, pos.y + raceOffsetY + 2 );
        text.set( Race::DoubleLinedString( Race::IndexToRace( static_cast<int>( i ) ) ), fheroes2::FontType::smallWhite() );
        text.draw( raceRect[i].x, raceRect[i].y + raceRect[i].height + 4, raceBorderWidth, display );

        pos.x += stepX;
    }

    pos.x = area.x + area.width / 2;
    pos.y += 260;

    const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

    // Render dialog buttons.
    fheroes2::Button buttonOk;
    fheroes2::Button buttonCancel;
    fheroes2::Button buttonTown;
    fheroes2::Button buttonCastle;
    background.renderOkayCancelButtons( buttonOk, buttonCancel, isEvilInterface );
    background.renderButton( buttonTown, isEvilInterface ? ICN::BUTTON_TOWN_EVIL : ICN::BUTTON_TOWN_GOOD, 0, 1, { -50, 7 },
                             fheroes2::StandardWindow::Padding::BOTTOM_CENTER );
    background.renderButton( buttonCastle, isEvilInterface ? ICN::BUTTON_CASTLE_EVIL : ICN::BUTTON_CASTLE_GOOD, 0, 1, { 50, 7 },
                             fheroes2::StandardWindow::Padding::BOTTOM_CENTER );

    const fheroes2::Rect castleRoi{ pos.x - 2 * fheroes2::tileWidthPx - fheroes2::tileWidthPx / 2, pos.y - 4 * fheroes2::tileWidthPx + fheroes2::tileWidthPx / 2,
                                    5 * fheroes2::tileWidthPx, 5 * fheroes2::tileWidthPx };

    fheroes2::ImageRestorer castleBackground( display, pos.x - 5 * fheroes2::tileWidthPx, pos.y - 4 * fheroes2::tileWidthPx + fheroes2::tileWidthPx / 2,
                                              8 * fheroes2::tileWidthPx, 5 * fheroes2::tileWidthPx );

    LocalEvent & le = LocalEvent::Get();
    bool needRedraw = true;
    bool isCastle = false;
    int townRace = 6;
    int townColor = 6;
    const int basementGround = isEvilInterface ? Maps::Ground::LAVA : Maps::Ground::DIRT;

    if ( type > -1 && color > -1 ) {
        isCastle = ( type % 2 ) == 0;
        townRace = type / 2;
        townColor = color;
    }

    isCastle ? buttonCastle.drawOnPress() : buttonTown.drawOnPress();

    while ( le.HandleEvents() ) {
        le.isMouseLeftButtonPressedInArea( buttonOk.area() ) ? buttonOk.drawOnPress() : buttonOk.drawOnRelease();
        le.isMouseLeftButtonPressedInArea( buttonCancel.area() ) ? buttonCancel.drawOnPress() : buttonCancel.drawOnRelease();

        if ( isCastle ) {
            if ( le.isMouseLeftButtonPressedInArea( buttonTown.area() ) ) {
                buttonTown.drawOnPress();
            }
            else if ( le.MouseClickLeft( buttonTown.area() ) ) {
                buttonTown.drawOnPress();
                isCastle = false;
                needRedraw = true;
            }
            else {
                buttonTown.drawOnRelease();
            }
        }
        else {
            if ( le.isMouseLeftButtonPressedInArea( buttonCastle.area() ) ) {
                buttonCastle.drawOnPress();
            }
            else if ( le.MouseClickLeft( buttonCastle.area() ) ) {
                buttonCastle.drawOnPress();
                isCastle = true;
                needRedraw = true;
            }
            else {
                buttonCastle.drawOnRelease();
            }
        }

        if ( le.MouseClickLeft( buttonOk.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) ) {
            type = getPackedTownType( townRace, isCastle );
            color = townColor;
            return;
        }
        if ( le.MouseClickLeft( buttonCancel.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
            return;
        }

        if ( le.isMouseRightButtonPressedInArea( buttonCancel.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Exit this menu without doing anything." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( buttonOk.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Okay" ), _( "Click to start placing the selected castle/town." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( buttonTown.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Town" ), _( "Click to select town placing." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( buttonCastle.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Castle" ), _( "Click to select castle placing." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( castleRoi ) ) {
            std::string name( _( "%{color} %{race} %{townOrCastle}" ) );
            StringReplace( name, "%{color}", ( townColor < 6 ) ? Color::String( Color::IndexToColor( townColor ) ) : _( "race|Neutral" ) );
            StringReplace( name, "%{race}", ( townRace < 6 ) ? Race::String( Race::IndexToRace( townRace ) ) : _( "race|Random" ) );
            StringReplace( name, "%{townOrCastle}", isCastle ? _( "Castle" ) : _( "Town" ) );
            fheroes2::showStandardTextMessage( _( "You will place" ), std::move( name ), Dialog::ZERO );
        }
        else {
            for ( size_t i = 0; i < 7; ++i ) {
                if ( le.MouseClickLeft( raceRect[i] ) ) {
                    townRace = static_cast<int>( i );
                    needRedraw = true;
                    break;
                }
                if ( le.MouseClickLeft( colorRect[i] ) ) {
                    townColor = static_cast<int>( i );
                    needRedraw = true;
                    break;
                }

                if ( le.isMouseRightButtonPressedInArea( raceRect[i] ) ) {
                    fheroes2::showStandardTextMessage( Race::String( Race::IndexToRace( static_cast<int>( i == 6 ? 7 : i ) ) ), _( "Click to select this class." ),
                                                       Dialog::ZERO );
                }
                else if ( le.isMouseRightButtonPressedInArea( colorRect[i] ) ) {
                    fheroes2::showStandardTextMessage( i == 6 ? _( "race|Neutral" ) : Color::String( Color::IndexToColor( static_cast<int>( i ) ) ),
                                                       _( "Click to select this color." ), Dialog::ZERO );
                }
            }
        }

        if ( !needRedraw ) {
            continue;
        }

        castleBackground.restore();

        // Update town image.
        const fheroes2::Sprite townImage = fheroes2::generateTownObjectImage( getPackedTownType( townRace, isCastle ), townColor, basementGround );
        fheroes2::Blit( townImage, display, pos.x + townImage.x(), pos.y + townImage.y() );

        // Update color and race borders.
        for ( int i = 0; i < 7; ++i ) {
            fheroes2::Blit( ( i == townColor ) ? colorSpriteBorderSelected : colorSpriteBorder, display, colorRect[i].x, colorRect[i].y );
            fheroes2::Blit( ( i == townRace ) ? raceSpriteBorderSelected : raceSpriteBorder, display, raceRect[i].x, raceRect[i].y );
        }

        display.render( background.totalArea() );

        needRedraw = false;
    }
}

int Dialog::selectDwellingType( const int dwellingType )
{
    const auto & objectInfo = Maps::getObjectsByGroup( Maps::ObjectGroup::ADVENTURE_DWELLINGS );

    GenericObjectTypeSelection listbox( objectInfo, { 480, fheroes2::Display::instance().height() - 180 }, _( "Select Dwelling:" ) );

    return selectObjectType( dwellingType, objectInfo.size(), listbox );
}

int Dialog::selectLandscapeMiscellaneousObjectType( const int objectType )
{
    const auto & objectInfo = Maps::getObjectsByGroup( Maps::ObjectGroup::LANDSCAPE_MISCELLANEOUS );

    GenericObjectTypeSelection listbox( objectInfo, { 420, fheroes2::Display::instance().height() - 180 }, _( "Select Landscape Object:" ) );

    return selectObjectType( objectType, objectInfo.size(), listbox );
}

void Dialog::selectMineType( int32_t & type, int32_t & color )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    fheroes2::StandardWindow background( 380, 395, true, display );

    const fheroes2::Rect & area = background.activeArea();

    fheroes2::Text text( _( "Mine placing" ), fheroes2::FontType::normalYellow() );
    int32_t offsetY = area.y + 10;
    text.draw( area.x + ( area.width - text.width() ) / 2, offsetY, display );

    // There are 7 resource types (WOOD, MERCURY, ORE, SULFUR, CRYSTAL, GEMS, GOLD) and abandoned mine.
    const uint32_t resourceCount{ 8 };
    const int32_t stepY = 35;
    offsetY += 50;
    const fheroes2::Rect resourceSelectRoi( area.x + 22, offsetY - 5, 100, stepY * resourceCount + 10 );
    background.applyTextBackgroundShading( resourceSelectRoi );

    text.set( _( "Resource\ntype:" ), fheroes2::FontType::smallWhite() );
    text.draw( resourceSelectRoi.x, resourceSelectRoi.y - text.height( resourceSelectRoi.width ), resourceSelectRoi.width, display );

    const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();
    std::array<fheroes2::Rect, resourceCount> resourceRoi;

    // Resource type selection mark background.
    const fheroes2::Sprite & markBackground = fheroes2::AGG::GetICN( isEvilInterface ? ICN::CELLWIN_EVIL : ICN::CELLWIN, 4 );

    // Resource icons.
    const int32_t iconOffsetX = 35;
    const std::vector<Maps::ObjectInfo> & resourceInfo = Maps::getObjectsByGroup( Maps::ObjectGroup::ADVENTURE_TREASURES );

    assert( resourceInfo.size() > resourceCount - 1 );

    for ( uint32_t i = 0; i < resourceCount - 1; ++i ) {
        const fheroes2::Sprite & resourceImage = fheroes2::generateMapObjectImage( resourceInfo[i] );
        const fheroes2::Point imagePosition( area.x + iconOffsetX + ( resourceSelectRoi.width - fheroes2::tileWidthPx * 3 ) / 2,
                                             offsetY + ( stepY - fheroes2::tileWidthPx ) / 2 );
        resourceRoi[i] = { resourceSelectRoi.x + 5, imagePosition.y, resourceSelectRoi.width - 10, fheroes2::tileWidthPx };
        fheroes2::Blit( resourceImage, display, imagePosition.x, imagePosition.y - 4 );
        fheroes2::Blit( markBackground, display, resourceRoi[i].x + 2, resourceRoi[i].y + ( resourceRoi[i].height - markBackground.height() ) / 2 );
        offsetY += stepY;
    }

    // Render ghosts to select Abandoned mine type.
    const fheroes2::Sprite & ghostImage = fheroes2::AGG::GetICN( ICN::MONS32, 59 );
    const int32_t ghostsOnIcon = 4;
    const int32_t ghostsIconOverlap = 12;
    const int32_t ghostsStepX = ghostImage.width() - ghostsIconOverlap;
    const int32_t ghostsIconWight = ghostsOnIcon * ghostsStepX + ghostsIconOverlap;
    const int32_t ghostsIconHeight = ghostImage.height();
    resourceRoi[7] = { resourceSelectRoi.x + 5, offsetY + ( stepY - ghostsIconHeight ) / 2, resourceSelectRoi.width - 10, ghostsIconHeight };
    for ( int32_t i = 0; i < ghostsOnIcon; ++i ) {
        fheroes2::Blit( ghostImage, display, area.x + iconOffsetX + ( resourceSelectRoi.width - ghostsIconWight ) / 2 + ghostsStepX * i, resourceRoi[7].y, i < 2 );
    }
    fheroes2::Blit( markBackground, display, resourceRoi[7].x + 2, resourceRoi[7].y + ( resourceRoi[7].height - markBackground.height() ) / 2 );

    // Resource type selection mark.
    const fheroes2::Sprite & mark = fheroes2::AGG::GetICN( ICN::CELLWIN, 5 );
    uint32_t selectedResourceType = 0;

    const std::vector<Maps::ObjectInfo> & allObjectInfo = Maps::getObjectsByGroup( Maps::ObjectGroup::ADVENTURE_MINES );
    if ( type > 0 ) {
        assert( static_cast<size_t>( type ) < allObjectInfo.size() );

        if ( allObjectInfo[type].objectType == MP2::OBJ_ABANDONED_MINE ) {
            selectedResourceType = 7;
        }
        else {
            switch ( allObjectInfo[type].metadata[0] ) {
            case Resource::ORE:
                selectedResourceType = 2;
                break;
            case Resource::SULFUR:
                selectedResourceType = 3;
                break;
            case Resource::CRYSTAL:
                selectedResourceType = 4;
                break;
            case Resource::GEMS:
                selectedResourceType = 5;
                break;
            case Resource::GOLD:
                selectedResourceType = 6;
                break;
            case Resource::WOOD:
                selectedResourceType = 0;
                break;
            case Resource::MERCURY:
                selectedResourceType = 1;
                break;
            default:
                // Have you added a new resource type for mines?
                assert( 0 );
            }
        }
    }

    fheroes2::MovableSprite resourceTypeSelection( mark );
    auto redrawResourceSelection = [&resourceTypeSelection, &mark]( const fheroes2::Rect & pos ) {
        resourceTypeSelection.setPosition( pos.x + 5, pos.y + ( pos.height - mark.height() ) / 2 );
        resourceTypeSelection.redraw();
    };
    redrawResourceSelection( resourceRoi[selectedResourceType] );

    // Mine appearance type selection list.
    MineTypeList listbox( area.getPosition() );
    const fheroes2::Rect listRoi( area.x + area.width - 5 * fheroes2::tileWidthPx - 75, resourceSelectRoi.y, 5 * fheroes2::tileWidthPx + 20, resourceSelectRoi.height );
    background.applyTextBackgroundShading( listRoi );

    // Initialize list background restorer to use it in list method 'listbox.RedrawBackground()'.
    listbox.initListBackgroundRestorer( listRoi );

    listbox.SetAreaItems( { listRoi.x, listRoi.y + 3, listRoi.width, listRoi.height - 3 } );

    int32_t scrollbarOffsetX = listRoi.x + listRoi.width + 15;
    background.renderScrollbarBackground( { scrollbarOffsetX, listRoi.y, listRoi.width, listRoi.height }, isEvilInterface );

    const int listIcnId = isEvilInterface ? ICN::SCROLLE : ICN::SCROLL;
    const int32_t topPartHeight = 19;
    ++scrollbarOffsetX;

    listbox.SetScrollButtonUp( listIcnId, 0, 1, { scrollbarOffsetX, listRoi.y + 1 } );
    listbox.SetScrollButtonDn( listIcnId, 2, 3, { scrollbarOffsetX, listRoi.y + listRoi.height - 15 } );
    listbox.setScrollBarArea( { scrollbarOffsetX + 2, listRoi.y + topPartHeight, 10, listRoi.height - 2 * topPartHeight } );
    listbox.SetAreaMaxItems( 4 );

    auto getObjectTypeByResource = []( const uint32_t resourceType ) {
        switch ( resourceType ) {
        case 0:
            return MP2::OBJ_SAWMILL;
        case 1:
            return MP2::OBJ_ALCHEMIST_LAB;
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
            return MP2::OBJ_MINE;
        case 7:
            return MP2::OBJ_ABANDONED_MINE;
        default:
            // Have you added a new resource type for mines?
            assert( 0 );
        }
        return MP2::OBJ_NONE;
    };

    std::vector<Maps::ObjectInfo> objectInfo;
    std::vector<int32_t> objectInfoIndexes;

    auto updateListContent = [&listbox, &objectInfo, &allObjectInfo, &objectInfoIndexes, &listIcnId, &getObjectTypeByResource]( const uint32_t resourceId ) {
        const MP2::MapObjectType objectType = getObjectTypeByResource( resourceId );

        // Mine appearance is the same for different mine resources so we keep the selection.
        const bool keepSelection = ( objectType == MP2::OBJ_MINE ) && !objectInfo.empty() && ( objectInfo.front().objectType == MP2::OBJ_MINE );
        const int selectedId = keepSelection ? listbox.getCurrentId() : 0;
        const int topId = keepSelection ? listbox.getTopId() : 0;

        objectInfo.clear();
        objectInfoIndexes.clear();

        // For the Abandoned Mine there is no metadata set so 'metadata[0] == 0'.
        const uint32_t resourceType = ( objectType != MP2::OBJ_ABANDONED_MINE ) ? Resource::getResourceTypeFromIconIndex( resourceId ) : 0;
        for ( size_t i = 0; i < allObjectInfo.size(); ++i ) {
            if ( allObjectInfo[i].objectType == objectType && allObjectInfo[i].metadata[0] == resourceType ) {
                objectInfo.push_back( allObjectInfo[i] );
                objectInfoIndexes.push_back( static_cast<int32_t>( i ) );
            }
        }

        listbox.SetListContent( objectInfo );
        // We need to reset the initial slider image to be able to reduce the current slider height.
        listbox.setScrollBarImage( fheroes2::AGG::GetICN( listIcnId, 4 ) );
        listbox.updateScrollBarImage();

        if ( keepSelection ) {
            listbox.SetCurrent( selectedId );
            listbox.setTopVisibleItem( topId );
        }
    };

    updateListContent( selectedResourceType );

    // Select the previously used mine type and resource type in the lists.
    if ( type > -1 ) {
        listbox.SetCurrent( std::distance( objectInfoIndexes.begin(), std::find( objectInfoIndexes.begin(), objectInfoIndexes.end(), type ) ) );
    }

    fheroes2::ImageRestorer appearanceTextBackground( display, 0, 0, 0, 0 );
    // Mine appearance selection text.
    auto redrawAppearanceText = [&appearanceTextBackground, &listRoi, &display]( const Maps::ObjectInfo & info ) {
        std::string mineText( _( "%{mineName} appearance:" ) );

        if ( info.objectType == MP2::OBJ_MINE ) {
            StringReplace( mineText, "%{mineName}", Maps::GetMineName( static_cast<int>( info.metadata[0] ) ) );
        }
        else {
            StringReplace( mineText, "%{mineName}", MP2::StringObject( info.objectType ) );
        }

        const fheroes2::Text appearanceText( std::move( mineText ), fheroes2::FontType::smallWhite() );
        const int32_t textWidth = appearanceText.width( listRoi.width );
        const int32_t textHeight = appearanceText.height( listRoi.width );

        appearanceTextBackground.restore();

        // The text height does not take into account the diacritical marks. Add extra 2 pixels to the restorer.
        appearanceTextBackground.update( listRoi.x + ( listRoi.width - textWidth ) / 2, listRoi.y - textHeight - 2, textWidth, textHeight + 2 );
        appearanceText.draw( listRoi.x, listRoi.y - textHeight, listRoi.width, display );
    };
    redrawAppearanceText( objectInfo[listbox.getCurrentId()] );

    // TODO: Implement owner color selection.
    listbox.setOwnerColor( 0 );

    listbox.Redraw();

    // Render dialog buttons.
    fheroes2::Button buttonOk;
    fheroes2::Button buttonCancel;
    background.renderOkayCancelButtons( buttonOk, buttonCancel, isEvilInterface );

    display.render( background.totalArea() );

    LocalEvent & le = LocalEvent::Get();

    while ( le.HandleEvents() ) {
        le.isMouseLeftButtonPressedInArea( buttonOk.area() ) ? buttonOk.drawOnPress() : buttonOk.drawOnRelease();
        le.isMouseLeftButtonPressedInArea( buttonCancel.area() ) ? buttonCancel.drawOnPress() : buttonCancel.drawOnRelease();

        bool needRedraw = listbox.QueueEventProcessing();

        if ( listbox.isDoubleClicked() || le.MouseClickLeft( buttonOk.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) ) {
            type = objectInfoIndexes[listbox.getCurrentId()];
            color = 0;
            return;
        }
        if ( le.MouseClickLeft( buttonCancel.area() ) || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
            return;
        }

        if ( le.isMouseRightButtonPressedInArea( buttonCancel.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Exit this menu without doing anything." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( buttonOk.area() ) ) {
            fheroes2::showStandardTextMessage( _( "Okay" ), _( "Click to start placing the selected castle/town." ), Dialog::ZERO );
        }
        else {
            for ( uint32_t i = 0; i < resourceCount; ++i ) {
                if ( le.MouseClickLeft( resourceRoi[i] ) ) {
                    if ( i != selectedResourceType ) {
                        selectedResourceType = i;
                        redrawResourceSelection( resourceRoi[selectedResourceType] );
                        updateListContent( selectedResourceType );
                        redrawAppearanceText( objectInfo[listbox.getCurrentId()] );

                        needRedraw = true;
                    }
                    break;
                }
                if ( le.isMouseRightButtonPressedInArea( resourceRoi[i] ) ) {
                    const MP2::MapObjectType objectType = getObjectTypeByResource( i );
                    std::string objectName;
                    if ( objectType == MP2::OBJ_MINE ) {
                        objectName = Maps::GetMineName( Resource::getResourceTypeFromIconIndex( i ) );
                    }
                    else {
                        objectName = MP2::StringObject( objectType );
                    }
                    std::string description( _( "Click to select %{object} as the resource generator to be placed." ) );
                    StringReplace( description, "%{object}", objectName );

                    fheroes2::showStandardTextMessage( std::move( objectName ), std::move( description ), Dialog::ZERO );

                    break;
                }
            }
        }

        if ( !needRedraw ) {
            continue;
        }

        listbox.Redraw();
        display.render( area );
    }
}

int Dialog::selectMountainType( const int mountainType )
{
    const auto & objectInfo = Maps::getObjectsByGroup( Maps::ObjectGroup::LANDSCAPE_MOUNTAINS );

    GenericObjectTypeSelection listbox( objectInfo, { 420, fheroes2::Display::instance().height() - 180 }, _( "Select Mountain Object:" ) );

    return selectObjectType( mountainType, objectInfo.size(), listbox );
}

int Dialog::selectRockType( const int rockType )
{
    const auto & objectInfo = Maps::getObjectsByGroup( Maps::ObjectGroup::LANDSCAPE_ROCKS );

    GenericObjectTypeSelection listbox( objectInfo, { 420, fheroes2::Display::instance().height() - 180 }, _( "Select Rock Object:" ) );

    return selectObjectType( rockType, objectInfo.size(), listbox );
}

int Dialog::selectTreeType( const int treeType )
{
    const auto & objectInfo = Maps::getObjectsByGroup( Maps::ObjectGroup::LANDSCAPE_TREES );

    GenericObjectTypeSelection listbox( objectInfo, { 420, fheroes2::Display::instance().height() - 180 }, _( "Select Tree Object:" ) );

    return selectObjectType( treeType, objectInfo.size(), listbox );
}

int Dialog::selectPowerUpObjectType( const int powerUpObjectType )
{
    const auto & objectInfo = Maps::getObjectsByGroup( Maps::ObjectGroup::ADVENTURE_POWER_UPS );

    PowerUpObjectTypeSelection listbox( objectInfo, { 420, fheroes2::Display::instance().height() - 180 }, _( "Select Power Up Object:" ) );

    return selectObjectType( powerUpObjectType, objectInfo.size(), listbox );
}

int Dialog::selectAdventureMiscellaneousObjectType( const int objectType )
{
    const auto & objectInfo = Maps::getObjectsByGroup( Maps::ObjectGroup::ADVENTURE_MISCELLANEOUS );

    GenericObjectTypeSelection listbox( objectInfo, { 420, fheroes2::Display::instance().height() - 180 }, _( "Select Adventure Object:" ) );

    return selectObjectType( objectType, objectInfo.size(), listbox );
}
