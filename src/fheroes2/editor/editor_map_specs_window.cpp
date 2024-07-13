/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2024                                                    *
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

#include "editor_map_specs_window.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include "agg_image.h"
#include "artifact.h"
#include "castle.h"
#include "color.h"
#include "cursor.h"
#include "dialog.h"
#include "dialog_selectitems.h"
#include "difficulty.h"
#include "editor_daily_events_window.h"
#include "editor_rumor_window.h"
#include "editor_ui_helper.h"
#include "game_hotkeys.h"
#include "game_over.h"
#include "icn.h"
#include "image.h"
#include "interface_list.h"
#include "localevent.h"
#include "map_format_helper.h"
#include "map_format_info.h"
#include "map_object_info.h"
#include "maps_fileinfo.h"
#include "math_base.h"
#include "race.h"
#include "screen.h"
#include "settings.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_castle.h"
#include "ui_dialog.h"
#include "ui_text.h"
#include "ui_tool.h"
#include "ui_window.h"

namespace
{
    // In original Editor map name is limited to 17 characters. We keep this limit to fit the Select Scenario dialog.
    const int32_t maxMapNameLength = 17;

    const int32_t descriptionBoxWidth = 292;
    const int32_t descriptionBoxHeight = 90;

    const int32_t playerStepX = 80;
    const int32_t difficultyStepX = 77;

    const int32_t daysInMonth{ 7 * 4 };
    const int32_t daysInYear{ daysInMonth * 12 };

    const uint32_t ultimateArtifactId = static_cast<uint32_t>( Artifact::EDITOR_ANY_ULTIMATE_ARTIFACT );

    const std::vector<uint8_t> supportedVictoryConditions{ Maps::FileInfo::VICTORY_DEFEAT_EVERYONE, Maps::FileInfo::VICTORY_CAPTURE_TOWN,
                                                           Maps::FileInfo::VICTORY_KILL_HERO, Maps::FileInfo::VICTORY_OBTAIN_ARTIFACT,
                                                           Maps::FileInfo::VICTORY_COLLECT_ENOUGH_GOLD };
    const std::vector<uint8_t> supportedLossConditions{ Maps::FileInfo::LOSS_EVERYTHING, Maps::FileInfo::LOSS_TOWN, Maps::FileInfo::LOSS_HERO,
                                                        Maps::FileInfo::LOSS_OUT_OF_TIME };

    struct HeroInfo
    {
        int32_t tileIndex{ -1 };
        int32_t color{ Color::NONE };
        const Maps::Map_Format::HeroMetadata * heroMetadata{ nullptr };
    };

    struct TownInfo
    {
        int32_t tileIndex{ -1 };
        int32_t color{ Color::NONE };
        int32_t race{ Race::NONE };
        const Maps::Map_Format::CastleMetadata * castleMetadata{ nullptr };
    };

    fheroes2::Sprite getHeroIcon( const int32_t heroPortait, const int32_t race, const int32_t color, const int townIcnId )
    {
        // To render hero icons we use castle flags and frame.
        const uint32_t flagIcnIndex = fheroes2::getCastleLeftFlagIcnIndex( color );

        const fheroes2::Sprite & castleLeftFlag = fheroes2::AGG::GetICN( ICN::FLAG32, flagIcnIndex );
        const fheroes2::Sprite & castleFrame = fheroes2::AGG::GetICN( townIcnId, 22 );
        const fheroes2::Sprite & castleRightFlag = fheroes2::AGG::GetICN( ICN::FLAG32, flagIcnIndex + 1 );

        fheroes2::Sprite castleIcon( castleFrame.width() + castleLeftFlag.width() + castleRightFlag.width() + 4, castleFrame.height() );
        castleIcon.reset();

        Blit( castleLeftFlag, 0, 0, castleIcon, 0, 5, castleLeftFlag.width(), castleLeftFlag.height() );
        Blit( castleFrame, 0, 0, castleIcon, castleLeftFlag.width() + 2, 0, castleFrame.width(), castleFrame.height() );
        Blit( castleRightFlag, 0, 0, castleIcon, castleFrame.width() + castleLeftFlag.width() + 4, 5, castleRightFlag.width(), castleRightFlag.height() );

        if ( heroPortait > 0 ) {
            const fheroes2::Sprite & heroPortrait = fheroes2::AGG::GetICN( ICN::MINIPORT, heroPortait - 1 );
            Copy( heroPortrait, 0, 0, castleIcon, castleLeftFlag.width() + 6, 4, heroPortrait.width(), heroPortrait.height() );
        }
        else {
            // This is a hero with a random race dependent portrait. Render the default race portrait.

            uint32_t portraitIndex = 0;
            switch ( race ) {
            case Race::KNGT:
                portraitIndex = 51;
                break;
            case Race::BARB:
                portraitIndex = 52;
                break;
            case Race::SORC:
                portraitIndex = 53;
                break;
            case Race::WRLK:
                portraitIndex = 54;
                break;
            case Race::WZRD:
                portraitIndex = 55;
                break;
            case Race::NECR:
                portraitIndex = 56;
                break;
            case Race::RAND:
                portraitIndex = 58;
                break;
            default:
                // Have you added a new race? Correct the logic above!
                assert( 0 );
                break;
            }

            fheroes2::Copy( fheroes2::AGG::GetICN( ICN::NGEXTRA, portraitIndex ), 17, 10, castleIcon, castleLeftFlag.width() + 6, 4, 30, 22 );
        }

        return castleIcon;
    }

    fheroes2::Sprite getTownIcon( const bool isTown, const int32_t race, const int32_t color, const int townIcnId )
    {
        const uint32_t flagIcnIndex = fheroes2::getCastleLeftFlagIcnIndex( color );

        const fheroes2::Sprite & castleLeftFlag = fheroes2::AGG::GetICN( ICN::FLAG32, flagIcnIndex );
        const fheroes2::Sprite & castleFrame = fheroes2::AGG::GetICN( townIcnId, 23 );
        const fheroes2::Sprite & castleRightFlag = fheroes2::AGG::GetICN( ICN::FLAG32, flagIcnIndex + 1 );

        fheroes2::Sprite castleIcon( castleFrame.width() + castleLeftFlag.width() + castleRightFlag.width() + 4, castleFrame.height() );
        castleIcon.reset();

        Blit( castleLeftFlag, 0, 0, castleIcon, 0, 5, castleLeftFlag.width(), castleLeftFlag.height() );
        Blit( castleFrame, 0, 0, castleIcon, castleLeftFlag.width() + 2, 0, castleFrame.width(), castleFrame.height() );
        Blit( castleRightFlag, 0, 0, castleIcon, castleFrame.width() + castleLeftFlag.width() + 4, 5, castleRightFlag.width(), castleRightFlag.height() );

        const uint32_t icnIndex = fheroes2::getCastleIcnIndex( race, !isTown );

        const fheroes2::Sprite & castleImage = fheroes2::AGG::GetICN( townIcnId, icnIndex );
        Copy( castleImage, 0, 0, castleIcon, castleLeftFlag.width() + 6, 4, castleImage.width(), castleImage.height() );

        return castleIcon;
    }

    std::string getHeroTitle( const std::string & name, const int race, const int32_t tileIndex, const int32_t mapWidth )
    {
        std::string title;

        if ( name.empty() ) {
            title = _( "[%{pos}]: %{race} hero" );
        }
        else {
            title = _( "[%{pos}]: %{name}, %{race} hero" );

            StringReplace( title, "%{name}", name );
        }

        StringReplace( title, "%{pos}", std::to_string( tileIndex % mapWidth ) + ", " + std::to_string( tileIndex / mapWidth ) );

        StringReplace( title, "%{race}", Race::String( race ) );

        return title;
    }

    std::string getTownTitle( const std::string & name, const int race, const bool isTown, const int32_t tileIndex, const int32_t mapWidth )
    {
        std::string title;

        if ( name.empty() ) {
            if ( isTown ) {
                title = _( "[%{pos}]: %{race} town" );
            }
            else {
                title = _( "[%{pos}]: %{race} castle" );
            }
        }
        else {
            if ( isTown ) {
                title = _( "[%{pos}]: %{name}, %{race} town" );
            }
            else {
                title = _( "[%{pos}]: %{name}, %{race} castle" );
            }

            StringReplace( title, "%{name}", name );
        }

        StringReplace( title, "%{pos}", std::to_string( tileIndex % mapWidth ) + ", " + std::to_string( tileIndex / mapWidth ) );

        StringReplace( title, "%{race}", Race::String( race ) );

        return title;
    }

    class SelectMapHero final : public Dialog::ItemSelectionWindow
    {
    public:
        explicit SelectMapHero( const fheroes2::Size & dialogSize, std::string title, std::string description, const int32_t mapWidth,
                                const std::vector<HeroInfo> & heroInfos, const bool isEvilInterface )
            : Dialog::ItemSelectionWindow( dialogSize, std::move( title ), std::move( description ) )
            , _townIcnId( isEvilInterface ? ICN::LOCATORE : ICN::LOCATORS )
            , _mapWidth( mapWidth )
            , _heroInfos( heroInfos )
        {
            SetAreaMaxItems( rtAreaItems.height / itemsOffsetY );
        }

        using Dialog::ItemSelectionWindow::ActionListPressRight;

        void RedrawItem( const int & index, int32_t dstx, int32_t dsty, bool current ) override
        {
            assert( index >= 0 && static_cast<size_t>( index ) < _heroInfos.size() );

            const auto & heroInfo = _heroInfos[index];
            const auto * heroMetadata = heroInfo.heroMetadata;

            renderItem( getHeroIcon( heroMetadata->customPortrait, heroMetadata->race, heroInfo.color, _townIcnId ),
                        getHeroTitle( heroMetadata->customName, heroMetadata->race, heroInfo.tileIndex, _mapWidth ), { dstx, dsty }, 40, 85, itemsOffsetY / 2, current );
        }

        void ActionListPressRight( int & index ) override
        {
            assert( index >= 0 && static_cast<size_t>( index ) < _heroInfos.size() );

            const auto & heroInfo = _heroInfos[index];

            fheroes2::showStandardTextMessage( {}, getHeroTitle( heroInfo.heroMetadata->customName, heroInfo.heroMetadata->race, heroInfo.tileIndex, _mapWidth ),
                                               Dialog::ZERO );
        }

        static const int32_t itemsOffsetY{ 35 };

    private:
        const int _townIcnId{ ICN::UNKNOWN };
        const int32_t _mapWidth{ 0 };
        const std::vector<HeroInfo> & _heroInfos;
    };

    class SelectMapCastle final : public Dialog::ItemSelectionWindow
    {
    public:
        explicit SelectMapCastle( const fheroes2::Size & dialogSize, std::string title, std::string description, const int32_t mapWidth,
                                  const std::vector<TownInfo> & townInfos, const bool isEvilInterface )
            : Dialog::ItemSelectionWindow( dialogSize, std::move( title ), std::move( description ) )
            , _townIcnId( isEvilInterface ? ICN::LOCATORE : ICN::LOCATORS )
            , _mapWidth( mapWidth )
            , _townInfos( townInfos )
        {
            SetAreaMaxItems( rtAreaItems.height / itemsOffsetY );
        }

        using Dialog::ItemSelectionWindow::ActionListPressRight;

        void RedrawItem( const int & index, int32_t dstx, int32_t dsty, bool current ) override
        {
            assert( index >= 0 && static_cast<size_t>( index ) < _townInfos.size() );

            const auto & townInfo = _townInfos[index];
            const auto * castleMetadata = townInfo.castleMetadata;

            const bool isTown
                = std::find( castleMetadata->builtBuildings.begin(), castleMetadata->builtBuildings.end(), BUILD_CASTLE ) == castleMetadata->builtBuildings.end();

            renderItem( getTownIcon( isTown, townInfo.race, townInfo.color, _townIcnId ),
                        getTownTitle( castleMetadata->customName, townInfo.race, isTown, townInfo.tileIndex, _mapWidth ), { dstx, dsty }, 45, 95, itemsOffsetY / 2,
                        current );
        }

        void ActionListPressRight( int & index ) override
        {
            assert( index >= 0 && static_cast<size_t>( index ) < _townInfos.size() );
            const auto & townInfo = _townInfos[index];
            const auto * castleMetadata = townInfo.castleMetadata;

            const bool isTown
                = std::find( castleMetadata->builtBuildings.begin(), castleMetadata->builtBuildings.end(), BUILD_CASTLE ) == castleMetadata->builtBuildings.end();

            fheroes2::showStandardTextMessage( {}, getTownTitle( townInfo.castleMetadata->customName, townInfo.race, isTown, townInfo.tileIndex, _mapWidth ),
                                               Dialog::ZERO );
        }

        static const int32_t itemsOffsetY{ 35 };

    private:
        const int _townIcnId{ ICN::UNKNOWN };
        const int32_t _mapWidth{ 0 };
        const std::vector<TownInfo> & _townInfos;
    };

    std::vector<HeroInfo> getMapHeroes( const Maps::Map_Format::MapFormat & map, const int32_t allowedColors )
    {
        if ( allowedColors == Color::NONE ) {
            // Nothing to do.
            return {};
        }

        const auto & heroObjects = Maps::getObjectsByGroup( Maps::ObjectGroup::KINGDOM_HEROES );

        std::vector<HeroInfo> heroInfos;

        // TODO: cache all heroes once this dialog is open. No need to run through all objects every time.
        for ( size_t tileIndex = 0; tileIndex < map.tiles.size(); ++tileIndex ) {
            for ( const auto & object : map.tiles[tileIndex].objects ) {
                if ( object.group != Maps::ObjectGroup::KINGDOM_HEROES ) {
                    continue;
                }

                if ( object.index >= heroObjects.size() ) {
                    assert( 0 );
                    continue;
                }

                const auto & metadata = heroObjects[object.index].metadata;
                const int32_t color = 1 << metadata[0];

                if ( !( color & allowedColors ) ) {
                    // Current hero color is not allowed.
                    continue;
                }

                heroInfos.emplace_back();
                HeroInfo & heroInfo = heroInfos.back();

                heroInfo.tileIndex = static_cast<int32_t>( tileIndex );
                heroInfo.color = color;

                auto heroMetadataIter = map.heroMetadata.find( object.id );
                assert( heroMetadataIter != map.heroMetadata.end() );

                heroInfo.heroMetadata = &heroMetadataIter->second;
            }
        }

        return heroInfos;
    }

    std::vector<TownInfo> getMapTowns( const Maps::Map_Format::MapFormat & map, const int32_t allowedColors, const bool excludeNeutralTowns )
    {
        if ( excludeNeutralTowns && allowedColors == Color::NONE ) {
            // Nothing to do.
            return {};
        }
        const auto & townObjects = Maps::getObjectsByGroup( Maps::ObjectGroup::KINGDOM_TOWNS );

        std::vector<TownInfo> townInfos;

        // TODO: cache all towns once this dialog is open. No need to run through all objects every time.
        for ( size_t tileIndex = 0; tileIndex < map.tiles.size(); ++tileIndex ) {
            for ( const auto & object : map.tiles[tileIndex].objects ) {
                if ( object.group != Maps::ObjectGroup::KINGDOM_TOWNS ) {
                    continue;
                }

                if ( object.index >= townObjects.size() ) {
                    assert( 0 );
                    continue;
                }

                const int32_t color = Color::IndexToColor( Maps::getTownColorIndex( map, tileIndex, object.id ) );
                if ( !( color & allowedColors ) && ( excludeNeutralTowns || color != Color::NONE ) ) {
                    // Current town color is not allowed.
                    continue;
                }

                townInfos.emplace_back();
                TownInfo & townInfo = townInfos.back();

                townInfo.tileIndex = static_cast<int32_t>( tileIndex );
                townInfo.color = color;
                townInfo.race = Race::IndexToRace( static_cast<int>( townObjects[object.index].metadata[0] ) );

                const auto castleMetadataIter = map.castleMetadata.find( object.id );
                assert( castleMetadataIter != map.castleMetadata.end() );

                townInfo.castleMetadata = &castleMetadataIter->second;
            }
        }

        return townInfos;
    }

    const char * getVictoryConditionText( const uint8_t victoryConditionType )
    {
        switch ( victoryConditionType ) {
        case Maps::FileInfo::VICTORY_DEFEAT_EVERYONE:
            return _( "None." );
        case Maps::FileInfo::VICTORY_CAPTURE_TOWN:
            return GameOver::GetString( GameOver::WINS_TOWN );
        case Maps::FileInfo::VICTORY_KILL_HERO:
            return GameOver::GetString( GameOver::WINS_HERO );
        case Maps::FileInfo::VICTORY_OBTAIN_ARTIFACT:
            return GameOver::GetString( GameOver::WINS_ARTIFACT );
        case Maps::FileInfo::VICTORY_DEFEAT_OTHER_SIDE:
            return _( "One side defeats another." );
        case Maps::FileInfo::VICTORY_COLLECT_ENOUGH_GOLD:
            return _( "Accumulate gold." );
        default:
            // This is an unknown condition. Add the logic for it above!
            assert( 0 );
            break;
        }

        return nullptr;
    }

    uint32_t getVictoryIcnIndex( const uint8_t victoryConditionType )
    {
        switch ( victoryConditionType ) {
        case Maps::FileInfo::VICTORY_DEFEAT_EVERYONE:
            return 30;
        case Maps::FileInfo::VICTORY_CAPTURE_TOWN:
            return 31;
        case Maps::FileInfo::VICTORY_KILL_HERO:
            return 32;
        case Maps::FileInfo::VICTORY_OBTAIN_ARTIFACT:
            return 33;
        case Maps::FileInfo::VICTORY_DEFEAT_OTHER_SIDE:
            return 34;
        case Maps::FileInfo::VICTORY_COLLECT_ENOUGH_GOLD:
            return 35;
        default:
            // This is an unknown condition. Add the logic for it above!
            assert( 0 );
            break;
        }

        return 0;
    }

    const char * getLossConditionText( const uint8_t lossConditionType )
    {
        switch ( lossConditionType ) {
        case Maps::FileInfo::LOSS_EVERYTHING:
            return _( "None." );
        case Maps::FileInfo::LOSS_TOWN:
            return GameOver::GetString( GameOver::LOSS_TOWN );
        case Maps::FileInfo::LOSS_HERO:
            return GameOver::GetString( GameOver::LOSS_HERO );
        case Maps::FileInfo::LOSS_OUT_OF_TIME:
            return _( "Run out of time." );
        default:
            // This is an unknown condition. Add the logic for it above!
            assert( 0 );
            break;
        }

        return nullptr;
    }

    uint32_t getLossIcnIndex( const uint8_t lossConditionType )
    {
        switch ( lossConditionType ) {
        case Maps::FileInfo::LOSS_EVERYTHING:
            return 36;
        case Maps::FileInfo::LOSS_TOWN:
            return 37;
        case Maps::FileInfo::LOSS_HERO:
            return 38;
        case Maps::FileInfo::LOSS_OUT_OF_TIME:
            return 39;
        default:
            // This is an unknown condition. Add the logic for it above!
            assert( 0 );
            break;
        }

        return 0;
    }

    void redrawVictoryCondition( const uint8_t condition, const fheroes2::Rect & roi, const bool yellowFont, fheroes2::Image & output )
    {
        const fheroes2::Sprite & winIcon = fheroes2::AGG::GetICN( ICN::REQUESTS, getVictoryIcnIndex( condition ) );
        fheroes2::Copy( winIcon, 0, 0, output, roi.x + 1, roi.y, winIcon.width(), winIcon.height() );
        const fheroes2::Text winText( getVictoryConditionText( condition ), yellowFont ? fheroes2::FontType::normalYellow() : fheroes2::FontType::normalWhite() );
        winText.drawInRoi( roi.x + 20, roi.y + 2, output, roi );
    }

    void redrawLossCondition( const uint8_t condition, const fheroes2::Rect & roi, const bool yellowFont, fheroes2::Image & output )
    {
        const fheroes2::Sprite & icon = fheroes2::AGG::GetICN( ICN::REQUESTS, getLossIcnIndex( condition ) );
        fheroes2::Copy( icon, 0, 0, output, roi.x + 1, roi.y, icon.width(), icon.height() );
        const fheroes2::Text winText( getLossConditionText( condition ), yellowFont ? fheroes2::FontType::normalYellow() : fheroes2::FontType::normalWhite() );
        winText.drawInRoi( roi.x + 20, roi.y + 2, output, roi );
    }

    class DropBoxList final : public Interface::ListBox<uint8_t>
    {
    public:
        using Interface::ListBox<uint8_t>::ActionListDoubleClick;
        using Interface::ListBox<uint8_t>::ActionListSingleClick;
        using Interface::ListBox<uint8_t>::ActionListPressRight;

        DropBoxList( const DropBoxList & ) = delete;
        DropBoxList & operator=( const DropBoxList & ) = delete;

        explicit DropBoxList( const fheroes2::Point & pt, const int32_t itemsCount, const bool isLossList, const int dropBoxIcn )
            : Interface::ListBox<uint8_t>( pt )
            , _isLossList( isLossList )
        {
            SetAreaMaxItems( itemsCount );

            fheroes2::Display & display = fheroes2::Display::instance();

            const fheroes2::Sprite & image = fheroes2::AGG::GetICN( dropBoxIcn, 0 );

            const int32_t topPartHeight = image.height() - 2;
            const int32_t listWidth = image.width();
            const int32_t middlePartHeight = topPartHeight - 2;
            const int32_t bottomPartHeight = topPartHeight;
            const int32_t listHeight = itemsCount * ( _itemHeight + 2 ) + 6;

            _itemWidth = listWidth - 6;

            _restorer = std::make_unique<fheroes2::ImageRestorer>( display, pt.x, pt.y, listWidth + 1, listHeight );

            // Top part of list background.
            fheroes2::Copy( image, 0, 0, display, pt.x, pt.y, listWidth, topPartHeight );
            const int32_t lineFixOffsetX = pt.x + listWidth;
            fheroes2::Copy( image, 0, 0, display, lineFixOffsetX, pt.y, 1, topPartHeight );

            // Middle part of list background.
            const int32_t middlePartCount = ( listHeight - topPartHeight - bottomPartHeight + middlePartHeight - 1 ) / middlePartHeight;
            int32_t offsetY = topPartHeight;

            for ( int32_t i = 0; i < middlePartCount; ++i ) {
                const int32_t copyHeight = std::min( middlePartHeight, listHeight - bottomPartHeight - offsetY );
                const int32_t posY = pt.y + offsetY;

                fheroes2::Copy( image, 0, 2, display, pt.x, posY, listWidth, copyHeight );
                fheroes2::Copy( image, 0, 2, display, lineFixOffsetX, posY, 1, copyHeight );

                offsetY += middlePartHeight;
            }

            // Bottom part of list background.
            offsetY = pt.y + listHeight - bottomPartHeight;
            fheroes2::Copy( image, 0, 2, display, pt.x, offsetY, listWidth, bottomPartHeight );
            fheroes2::Copy( image, 0, 2, display, lineFixOffsetX, offsetY, 1, bottomPartHeight );

            _background = std::make_unique<fheroes2::ImageRestorer>( display, pt.x + 2, pt.y + 2, listWidth - 3, listHeight - 4 );

            SetAreaItems( { pt.x + 2, pt.y + 5, listWidth - 4, listHeight - 6 } );
        }

        ~DropBoxList() override
        {
            // After closing the drop list we also need to render the restored background.
            _restorer->restore();
            fheroes2::Display::instance().render( _restorer->rect() );
        }

        void RedrawItem( const uint8_t & condition, int32_t dstX, int32_t dstY, bool current ) override
        {
            if ( _isLossList ) {
                redrawLossCondition( condition, { dstX, dstY, _itemWidth, _itemHeight }, current, fheroes2::Display::instance() );
            }
            else {
                redrawVictoryCondition( condition, { dstX, dstY, _itemWidth, _itemHeight }, current, fheroes2::Display::instance() );
            }
        }

        void RedrawBackground( const fheroes2::Point & /*dst*/ ) override
        {
            _background->restore();
        }

        void ActionCurrentUp() override
        {
            // Do nothing.
        }

        void ActionCurrentDn() override
        {
            // Do nothing.
        }

        void ActionListDoubleClick( uint8_t & /* item */ ) override
        {
            _isClicked = true;
        }

        void ActionListSingleClick( uint8_t & /* item */ ) override
        {
            _isClicked = true;
        }

        void ActionListPressRight( uint8_t & condition ) override
        {
            if ( _isLossList ) {
                fheroes2::showStandardTextMessage( _( "Special Loss Condition" ), getLossConditionText( condition ), Dialog::ZERO );
            }
            else {
                fheroes2::showStandardTextMessage( _( "Special Victory Condition" ), getVictoryConditionText( condition ), Dialog::ZERO );
            }
        }

        fheroes2::Rect getArea() const
        {
            if ( _restorer == nullptr ) {
                return {};
            }

            return _restorer->rect();
        }

        bool isClicked() const
        {
            return _isClicked;
        }

    private:
        bool _isClicked{ false };
        bool _isLossList{ false };
        int32_t _itemWidth;
        int32_t _itemHeight{ fheroes2::getFontHeight( fheroes2::FontSize::NORMAL ) + 1 };
        std::unique_ptr<fheroes2::ImageRestorer> _restorer;
        std::unique_ptr<fheroes2::ImageRestorer> _background;
    };

    class VictoryConditionUI final
    {
    public:
        VictoryConditionUI( fheroes2::Image & output, const fheroes2::Rect & roi, const Maps::Map_Format::MapFormat & mapFormat, const bool isEvilInterface )
            : _conditionType( mapFormat.victoryConditionType )
            , _isNormalVictoryAllowed( mapFormat.allowNormalVictory )
            , _isVictoryConditionApplicableForAI( mapFormat.isVictoryConditionApplicableForAI )
            , _isEvilInterface( isEvilInterface )
            , _mapWidth( mapFormat.size )
            , _restorer( output, roi.x, roi.y, roi.width, roi.height )
        {
            // Set the initial state for all victory conditions.
            switch ( _conditionType ) {
            case Maps::FileInfo::VICTORY_DEFEAT_EVERYONE:
                // This condition has no extra options.

                break;
            case Maps::FileInfo::VICTORY_CAPTURE_TOWN:
                if ( mapFormat.victoryConditionMetadata.size() == 2 ) {
                    std::copy( mapFormat.victoryConditionMetadata.begin(), mapFormat.victoryConditionMetadata.end(), _townToCapture.begin() );

                    // Verify that this is a valid computer-only town.
                    _mapTownInfos = getMapTowns( mapFormat, mapFormat.computerPlayerColors & ( ~mapFormat.humanPlayerColors ), false );
                    const int32_t townTileIndex = static_cast<int32_t>( _townToCapture[0] );

                    bool townFound = false;
                    for ( const auto & town : _mapTownInfos ) {
                        if ( townTileIndex == town.tileIndex && static_cast<int32_t>( _townToCapture[1] ) == town.color ) {
                            townFound = true;
                            break;
                        }
                    }

                    if ( !townFound ) {
                        _conditionType = Maps::FileInfo::VICTORY_DEFEAT_EVERYONE;
                    }
                }
                else {
                    // Since the metadata is invalid we have 2 options:
                    // - fall back to normal victory condition as no town was set
                    // - generate a list of towns and pick one
                    // For simplicity we are choosing the first option for now.
                    _conditionType = Maps::FileInfo::VICTORY_DEFEAT_EVERYONE;
                }

                break;
            case Maps::FileInfo::VICTORY_KILL_HERO:
                if ( mapFormat.victoryConditionMetadata.size() == 2 ) {
                    std::copy( mapFormat.victoryConditionMetadata.begin(), mapFormat.victoryConditionMetadata.end(), _heroToKill.begin() );

                    // Verify that this is a valid computer-only hero.
                    _mapHeroInfos = getMapHeroes( mapFormat, mapFormat.computerPlayerColors & ( ~mapFormat.humanPlayerColors ) );
                    const int32_t heroTileIndex = static_cast<int32_t>( _heroToKill[0] );
                    bool heroFound = false;
                    for ( const auto & hero : _mapHeroInfos ) {
                        if ( heroTileIndex == hero.tileIndex && static_cast<int32_t>( _heroToKill[1] ) == hero.color ) {
                            heroFound = true;
                            break;
                        }
                    }

                    if ( !heroFound ) {
                        _conditionType = Maps::FileInfo::VICTORY_DEFEAT_EVERYONE;
                    }
                }
                else {
                    // Since the metadata is invalid we have 2 options:
                    // - fall back to normal victory condition as no hero was set
                    // - generate a list of heroes and pick one
                    // For simplicity we are choosing the first option for now.
                    _conditionType = Maps::FileInfo::VICTORY_DEFEAT_EVERYONE;
                }

                break;
            case Maps::FileInfo::VICTORY_OBTAIN_ARTIFACT:
                if ( mapFormat.victoryConditionMetadata.size() == 1 ) {
                    // In original game's map format '0' stands for any Ultimate Artifact.
                    _victoryArtifactId = ( mapFormat.victoryConditionMetadata[0] == 0 ) ? ultimateArtifactId : mapFormat.victoryConditionMetadata[0];
                }

                break;
            case Maps::FileInfo::VICTORY_COLLECT_ENOUGH_GOLD: {
                if ( mapFormat.victoryConditionMetadata.size() == 1 ) {
                    _goldAccumulationValue.setValue( static_cast<int32_t>( mapFormat.victoryConditionMetadata[0] ) );
                }

                break;
            }
            default:
                // Did you add more conditions? Add the logic for them!
                assert( 0 );

                break;
            }
        }

        void setConditionType( const uint8_t victoryConditionType )
        {
            _conditionType = victoryConditionType;
        }

        bool updateCondition( Maps::Map_Format::MapFormat & mapFormat )
        {
            switch ( _conditionType ) {
            case Maps::FileInfo::VICTORY_CAPTURE_TOWN: {
                _mapTownInfos = getMapTowns( mapFormat, mapFormat.computerPlayerColors & ( ~mapFormat.humanPlayerColors ), false );
                if ( _mapTownInfos.empty() ) {
                    // No towns exist for computer-only players.
                    _conditionType = Maps::FileInfo::VICTORY_DEFEAT_EVERYONE;
                    mapFormat.victoryConditionType = _conditionType;

                    return true;
                }

                bool townFound = false;
                for ( const auto & town : _mapTownInfos ) {
                    if ( static_cast<int32_t>( _townToCapture[0] ) == town.tileIndex && static_cast<int32_t>( _townToCapture[1] ) == town.color ) {
                        townFound = true;
                        break;
                    }
                }

                if ( !townFound ) {
                    // The town doesn't exist in the list. Select the first one.
                    _townToCapture[0] = static_cast<uint32_t>( _mapTownInfos[0].tileIndex );
                    _townToCapture[1] = static_cast<uint32_t>( _mapTownInfos[0].color );
                    return true;
                }

                return false;
            }
            case Maps::FileInfo::VICTORY_KILL_HERO: {
                _mapHeroInfos = getMapHeroes( mapFormat, mapFormat.computerPlayerColors & ( ~mapFormat.humanPlayerColors ) );
                if ( _mapHeroInfos.empty() ) {
                    // No heroes exist for computer-only players.
                    _conditionType = Maps::FileInfo::VICTORY_DEFEAT_EVERYONE;
                    mapFormat.victoryConditionType = _conditionType;

                    return true;
                }

                const int32_t heroTileIndex = static_cast<int32_t>( _heroToKill[0] );
                bool heroFound = false;
                for ( const auto & hero : _mapHeroInfos ) {
                    if ( heroTileIndex == hero.tileIndex && static_cast<int32_t>( _heroToKill[1] ) == hero.color ) {
                        heroFound = true;
                        break;
                    }
                }

                if ( !heroFound ) {
                    // The hero doesn't exist in the list. Select the first one.
                    _heroToKill[0] = static_cast<uint32_t>( _mapHeroInfos[0].tileIndex );
                    _heroToKill[1] = static_cast<uint32_t>( _mapHeroInfos[0].color );
                }

                return false;
            }
            default:
                // No changes for other victory conditions.
                break;
            }

            return false;
        }

        void getConditionMetadata( Maps::Map_Format::MapFormat & mapFormat ) const
        {
            assert( mapFormat.victoryConditionType == _conditionType );

            switch ( _conditionType ) {
            case Maps::FileInfo::VICTORY_DEFEAT_EVERYONE:
                // This condition has no metadata.
                mapFormat.victoryConditionMetadata.clear();

                mapFormat.allowNormalVictory = false;
                mapFormat.isVictoryConditionApplicableForAI = false;

                return;
            case Maps::FileInfo::VICTORY_CAPTURE_TOWN:
                if ( mapFormat.victoryConditionMetadata.size() != 2 ) {
                    mapFormat.victoryConditionMetadata.resize( 2 );
                }

                std::copy( _townToCapture.begin(), _townToCapture.end(), mapFormat.victoryConditionMetadata.begin() );

                mapFormat.allowNormalVictory = _isNormalVictoryAllowed;

                // For all non-neutral towns disable the "Allow this condition also for AI" setting.
                mapFormat.isVictoryConditionApplicableForAI = ( _townToCapture[1] == Color::NONE ) ? _isVictoryConditionApplicableForAI : false;

                return;
            case Maps::FileInfo::VICTORY_KILL_HERO:
                if ( mapFormat.victoryConditionMetadata.size() != 2 ) {
                    mapFormat.victoryConditionMetadata.resize( 2 );
                }

                std::copy( _heroToKill.begin(), _heroToKill.end(), mapFormat.victoryConditionMetadata.begin() );

                mapFormat.allowNormalVictory = false;
                mapFormat.isVictoryConditionApplicableForAI = false;

                return;
            case Maps::FileInfo::VICTORY_OBTAIN_ARTIFACT:
                if ( mapFormat.victoryConditionMetadata.size() != 1 ) {
                    mapFormat.victoryConditionMetadata.resize( 1 );
                }

                mapFormat.isVictoryConditionApplicableForAI = false;

                // In original game's map format '0' stands for any Ultimate Artifact. Set it also to '0' for the compatibility.
                mapFormat.victoryConditionMetadata[0] = ( _victoryArtifactId == ultimateArtifactId ) ? 0 : _victoryArtifactId;
                mapFormat.allowNormalVictory = _isNormalVictoryAllowed;

                return;
            case Maps::FileInfo::VICTORY_COLLECT_ENOUGH_GOLD:
                if ( mapFormat.victoryConditionMetadata.size() != 1 ) {
                    mapFormat.victoryConditionMetadata.resize( 1 );
                }

                mapFormat.victoryConditionMetadata[0] = static_cast<uint32_t>( _goldAccumulationValue.getValue() );
                mapFormat.allowNormalVictory = _isNormalVictoryAllowed;
                mapFormat.isVictoryConditionApplicableForAI = _isVictoryConditionApplicableForAI;

                return;
            default:
                // Did you add more conditions? Add the logic for them!
                assert( 0 );

                // Reset the unknown condition to the default condition type.
                mapFormat.victoryConditionType = Maps::FileInfo::VICTORY_DEFEAT_EVERYONE;

                mapFormat.allowNormalVictory = false;
                mapFormat.isVictoryConditionApplicableForAI = false;

                break;
            }
        }

        void render( fheroes2::Image & output, const bool renderEverything )
        {
            if ( renderEverything ) {
                // Restore background to make sure that other UI elements aren't being rendered.
                _restorer.restore();
            }

            switch ( _conditionType ) {
            case Maps::FileInfo::VICTORY_DEFEAT_EVERYONE:
                // No special UI is needed.

                break;
            case Maps::FileInfo::VICTORY_CAPTURE_TOWN: {
                if ( !renderEverything ) {
                    // To render this condition we always redraw the whole conditions UI.
                    // TODO: optimize the rendering.
                    _restorer.restore();
                }

                assert( !_mapTownInfos.empty() );

                size_t selectedTownIndex = 0;
                for ( size_t i = 0; i < _mapTownInfos.size(); ++i ) {
                    if ( static_cast<int32_t>( _townToCapture[0] ) == _mapTownInfos[i].tileIndex
                         && static_cast<int32_t>( _townToCapture[1] ) == _mapTownInfos[i].color ) {
                        selectedTownIndex = i;
                        break;
                    }
                }

                const auto & townInfo = _mapTownInfos[selectedTownIndex];
                const auto * castleMetadata = townInfo.castleMetadata;
                const int townIcnId = _isEvilInterface ? ICN::LOCATORE : ICN::LOCATORS;

                const bool isTown
                    = std::find( castleMetadata->builtBuildings.begin(), castleMetadata->builtBuildings.end(), BUILD_CASTLE ) == castleMetadata->builtBuildings.end();

                const fheroes2::Sprite townIcon( getTownIcon( isTown, townInfo.race, townInfo.color, townIcnId ) );

                const fheroes2::Rect roi{ _restorer.rect() };
                fheroes2::Blit( townIcon, output, roi.x, roi.y + 4 );

                fheroes2::Text text( getTownTitle( castleMetadata->customName, townInfo.race, isTown, townInfo.tileIndex, _mapWidth ),
                                     fheroes2::FontType::normalWhite() );
                text.fitToOneRow( roi.width - townIcon.width() - 5 );
                text.drawInRoi( roi.x + townIcon.width() + 5, roi.y + 12, output, roi );

                _selectConditionRoi = { roi.x, roi.y + 4, townIcon.width() + 5 + text.width(), townIcon.height() };

                if ( !_isNormalVictoryAllowed ) {
                    _allowNormalVictory.hide();
                }

                _allowNormalVictoryRoi = Editor::drawCheckboxWithText( _allowNormalVictory, _( "Allow standard victory conditions" ), output, roi.x + 5,
                                                                       roi.y + _selectConditionRoi.height + 35, _isEvilInterface );

                if ( _isNormalVictoryAllowed ) {
                    _allowNormalVictory.show();
                }

                // Allow "Allow this condition also for AI" setting only for neutral towns.
                if ( _townToCapture[1] == Color::NONE ) {
                    if ( !_isVictoryConditionApplicableForAI ) {
                        _allowVictoryConditionForAI.hide();
                    }

                    _allowVictoryConditionForAIRoi = Editor::drawCheckboxWithText( _allowVictoryConditionForAI, _( "Allow this condition also for AI" ), output,
                                                                                   roi.x + 5, roi.y + _selectConditionRoi.height + 10, _isEvilInterface );

                    if ( _isVictoryConditionApplicableForAI ) {
                        _allowVictoryConditionForAI.show();
                    }
                }

                break;
            }
            case Maps::FileInfo::VICTORY_KILL_HERO: {
                if ( !renderEverything ) {
                    // To render this condition we always redraw the whole conditions UI.
                    // TODO: optimize the rendering.
                    _restorer.restore();
                }

                const fheroes2::Rect roi{ _restorer.rect() };

                const fheroes2::Sprite & heroFrame = fheroes2::AGG::GetICN( ICN::SWAPWIN, 0 );

                const int32_t heroFrameWidth = 111;
                const int32_t heroFrameHeight = 105;

                _selectConditionRoi = { roi.x + ( roi.width - heroFrameWidth ) / 2, roi.y + 4, heroFrameWidth, heroFrameHeight };

                fheroes2::Blit( heroFrame, 88, 66, output, _selectConditionRoi.x, _selectConditionRoi.y, heroFrameWidth, heroFrameHeight );

                assert( !_mapHeroInfos.empty() );

                size_t selectedHeroIndex = 0;
                for ( size_t i = 0; i < _mapHeroInfos.size(); ++i ) {
                    if ( static_cast<int32_t>( _heroToKill[0] ) == _mapHeroInfos[i].tileIndex && static_cast<int32_t>( _heroToKill[1] ) == _mapHeroInfos[i].color ) {
                        selectedHeroIndex = i;
                        break;
                    }
                }

                // To render hero icons we use castle flags and frame.
                const uint32_t flagIcnIndex = fheroes2::getCastleLeftFlagIcnIndex( static_cast<int>( _heroToKill[1] ) );
                const fheroes2::Sprite & castleLeftFlag = fheroes2::AGG::GetICN( ICN::FLAG32, flagIcnIndex );
                const fheroes2::Sprite & castleRightFlag = fheroes2::AGG::GetICN( ICN::FLAG32, flagIcnIndex + 1 );
                Blit( castleLeftFlag, 0, 0, output, _selectConditionRoi.x - 21, _selectConditionRoi.y + 45, castleLeftFlag.width(), castleLeftFlag.height() );
                Blit( castleRightFlag, 0, 0, output, _selectConditionRoi.x + _selectConditionRoi.width + 2, _selectConditionRoi.y + 45, castleRightFlag.width(),
                      castleRightFlag.height() );

                const auto * heroMetadata = _mapHeroInfos[selectedHeroIndex].heroMetadata;
                const int32_t heroPortraitId = heroMetadata->customPortrait;

                if ( heroPortraitId > 0 ) {
                    const fheroes2::Sprite & heroPortrait = fheroes2::AGG::GetICN( ICN::PORTxxxx( heroPortraitId ), 0 );

                    fheroes2::Copy( heroPortrait, 0, 0, output, _selectConditionRoi.x + 5, _selectConditionRoi.y + 6, heroPortrait.width(), heroPortrait.height() );
                }
                else {
                    fheroes2::renderHeroRacePortrait( heroMetadata->race, { _selectConditionRoi.x + 5, _selectConditionRoi.y + 6, 101, 93 }, output );
                }

                fheroes2::Text extraText( getHeroTitle( heroMetadata->customName, heroMetadata->race, static_cast<int32_t>( _heroToKill[0] ), _mapWidth ),
                                          fheroes2::FontType::normalWhite() );
                extraText.fitToOneRow( roi.width );
                extraText.drawInRoi( roi.x, _selectConditionRoi.y + _selectConditionRoi.height + 5, roi.width, output, roi );

                break;
            }
            case Maps::FileInfo::VICTORY_OBTAIN_ARTIFACT: {
                if ( renderEverything ) {
                    const fheroes2::Rect roi{ _restorer.rect() };

                    const fheroes2::Sprite & artifactFrame = fheroes2::AGG::GetICN( ICN::RESOURCE, 7 );
                    _selectConditionRoi = { roi.x + ( roi.width - artifactFrame.width() ) / 2, roi.y + 4, artifactFrame.width(), artifactFrame.height() };

                    fheroes2::Blit( artifactFrame, output, _selectConditionRoi.x, _selectConditionRoi.y );

                    if ( !_isNormalVictoryAllowed ) {
                        _allowNormalVictory.hide();
                    }

                    _allowNormalVictoryRoi = Editor::drawCheckboxWithText( _allowNormalVictory, _( "Allow standard victory conditions" ), output, roi.x + 5,
                                                                           roi.y + _selectConditionRoi.height + 10, _isEvilInterface );
                }

                const fheroes2::Sprite & artifactImage = fheroes2::AGG::GetICN( ICN::ARTIFACT, Artifact( static_cast<int>( _victoryArtifactId ) ).IndexSprite64() );

                fheroes2::Copy( artifactImage, 0, 0, output, _selectConditionRoi.x + 6, _selectConditionRoi.y + 6, artifactImage.width(), artifactImage.height() );

                if ( _isNormalVictoryAllowed ) {
                    _allowNormalVictory.show();
                }
                else {
                    _allowNormalVictory.hide();
                }

                break;
            }
            case Maps::FileInfo::VICTORY_COLLECT_ENOUGH_GOLD: {
                if ( renderEverything ) {
                    const fheroes2::Size valueSectionUiSize = fheroes2::ValueSelectionDialogElement::getArea();
                    const fheroes2::Rect roi{ _restorer.rect() };
                    const fheroes2::Point uiOffset{ roi.x + ( roi.width - valueSectionUiSize.width ) / 2, roi.y };

                    _goldAccumulationValue.setOffset( uiOffset );
                    _goldAccumulationValue.draw( output );

                    const fheroes2::Text text( _( "Gold:" ), fheroes2::FontType::normalWhite() );
                    text.draw( uiOffset.x - text.width() - 5, roi.y + ( valueSectionUiSize.height - text.height() ) / 2 + 2, output );

                    if ( !_isVictoryConditionApplicableForAI ) {
                        _allowVictoryConditionForAI.hide();
                    }
                    if ( !_isNormalVictoryAllowed ) {
                        _allowNormalVictory.hide();
                    }

                    _allowVictoryConditionForAIRoi = Editor::drawCheckboxWithText( _allowVictoryConditionForAI, _( "Allow this condition also for AI" ), output,
                                                                                   roi.x + 5, roi.y + valueSectionUiSize.height + 10, _isEvilInterface );
                    _allowNormalVictoryRoi = Editor::drawCheckboxWithText( _allowNormalVictory, _( "Allow standard victory conditions" ), output, roi.x + 5,
                                                                           roi.y + valueSectionUiSize.height + 35, _isEvilInterface );
                }

                _goldAccumulationValue.draw( output );

                if ( _isNormalVictoryAllowed ) {
                    _allowNormalVictory.show();
                }
                else {
                    _allowNormalVictory.hide();
                }

                if ( _isVictoryConditionApplicableForAI ) {
                    _allowVictoryConditionForAI.show();
                }
                else {
                    _allowVictoryConditionForAI.hide();
                }

                break;
            }
            default:
                // Did you add more conditions? Add the logic for them!
                assert( 0 );

                break;
            }
        }

        // Returns true if rendering is required.
        bool processEvents()
        {
            switch ( _conditionType ) {
            case Maps::FileInfo::VICTORY_DEFEAT_EVERYONE:
                // No events to process.

                return false;
            case Maps::FileInfo::VICTORY_CAPTURE_TOWN: {
                LocalEvent & le = LocalEvent::Get();

                if ( le.MouseClickLeft( _selectConditionRoi ) ) {
                    assert( !_mapTownInfos.empty() );

                    const int32_t maxHeight = std::min( 100 + SelectMapCastle::itemsOffsetY * 12, fheroes2::Display::instance().height() - 100 );
                    const int32_t itemsHeight
                        = std::max( 100 + SelectMapCastle::itemsOffsetY * static_cast<int32_t>( _mapTownInfos.size() ), 100 + SelectMapCastle::itemsOffsetY * 5 );
                    const int32_t totalHeight = std::min( itemsHeight, maxHeight );

                    SelectMapCastle listbox( { 450, totalHeight }, _( "Select a Town to capture to achieve victory" ), {}, _mapWidth, _mapTownInfos, _isEvilInterface );

                    std::vector<int> townIndicies( _mapTownInfos.size() );
                    std::iota( townIndicies.begin(), townIndicies.end(), 0 );

                    listbox.SetListContent( townIndicies );

                    int initiallySelectedTownIndex = 0;

                    for ( size_t i = 0; i < _mapTownInfos.size(); ++i ) {
                        if ( static_cast<int32_t>( _townToCapture[0] ) == _mapTownInfos[i].tileIndex
                             && static_cast<int32_t>( _townToCapture[1] ) == _mapTownInfos[i].color ) {
                            initiallySelectedTownIndex = static_cast<int>( i );
                            listbox.SetCurrent( initiallySelectedTownIndex );
                            break;
                        }
                    }

                    const int32_t result = listbox.selectItemsEventProcessing();

                    if ( result == Dialog::OK ) {
                        const int townIndex = listbox.GetCurrent();

                        if ( townIndex != initiallySelectedTownIndex ) {
                            _townToCapture[0] = static_cast<uint32_t>( _mapTownInfos[townIndex].tileIndex );
                            _townToCapture[1] = static_cast<uint32_t>( _mapTownInfos[townIndex].color );
                        }
                    }

                    return true;
                }

                if ( le.isMouseRightButtonPressedInArea( _selectConditionRoi ) ) {
                    fheroes2::showStandardTextMessage( _( "Special Victory Condition" ), _( "Click here to change the town needed to capture to achieve victory." ),
                                                       Dialog::ZERO );
                    return false;
                }

                if ( le.MouseClickLeft( _allowNormalVictoryRoi ) ) {
                    _isNormalVictoryAllowed = !_isNormalVictoryAllowed;

                    return true;
                }

                if ( le.MouseClickLeft( _allowVictoryConditionForAIRoi ) ) {
                    _isVictoryConditionApplicableForAI = !_isVictoryConditionApplicableForAI;

                    return true;
                }

                break;
            }
            case Maps::FileInfo::VICTORY_KILL_HERO: {
                LocalEvent & le = LocalEvent::Get();

                if ( le.MouseClickLeft( _selectConditionRoi ) ) {
                    assert( !_mapHeroInfos.empty() );

                    const int32_t maxHeight = std::min( 100 + SelectMapCastle::itemsOffsetY * 12, fheroes2::Display::instance().height() - 100 );
                    const int32_t itemsHeight
                        = std::max( 100 + SelectMapCastle::itemsOffsetY * static_cast<int32_t>( _mapHeroInfos.size() ), 100 + SelectMapCastle::itemsOffsetY * 5 );
                    const int32_t totalHeight = std::min( itemsHeight, maxHeight );

                    SelectMapHero listbox( { 450, totalHeight }, _( "Select a Hero to defeat to achieve victory" ), {}, _mapWidth, _mapHeroInfos, _isEvilInterface );

                    std::vector<int> heroIndicies( _mapHeroInfos.size() );
                    std::iota( heroIndicies.begin(), heroIndicies.end(), 0 );

                    listbox.SetListContent( heroIndicies );

                    int initiallySelectedHeroIndex = 0;

                    for ( size_t i = 0; i < _mapHeroInfos.size(); ++i ) {
                        if ( static_cast<int32_t>( _heroToKill[0] ) == _mapHeroInfos[i].tileIndex && static_cast<int32_t>( _heroToKill[1] ) == _mapHeroInfos[i].color ) {
                            initiallySelectedHeroIndex = static_cast<int>( i );
                            listbox.SetCurrent( initiallySelectedHeroIndex );
                            break;
                        }
                    }

                    const int32_t result = listbox.selectItemsEventProcessing();

                    if ( result == Dialog::OK ) {
                        const int heroIndex = listbox.GetCurrent();

                        if ( heroIndex != initiallySelectedHeroIndex ) {
                            _heroToKill[0] = static_cast<uint32_t>( _mapHeroInfos[heroIndex].tileIndex );
                            _heroToKill[1] = static_cast<uint32_t>( _mapHeroInfos[heroIndex].color );
                        }
                    }

                    return true;
                }

                if ( le.isMouseRightButtonPressedInArea( _selectConditionRoi ) ) {
                    fheroes2::showStandardTextMessage( _( "Special Victory Condition" ), _( "Click here to change the hero needed to defeat to achieve victory." ),
                                                       Dialog::ZERO );
                    return false;
                }

                break;
            }
            case Maps::FileInfo::VICTORY_OBTAIN_ARTIFACT: {
                LocalEvent & le = LocalEvent::Get();

                if ( le.MouseClickLeft( _selectConditionRoi ) ) {
                    const Artifact artifact = Dialog::selectArtifact( static_cast<int>( _victoryArtifactId ), true );

                    if ( artifact.isValid() || artifact.GetID() == Artifact::EDITOR_ANY_ULTIMATE_ARTIFACT ) {
                        _victoryArtifactId = artifact.GetID();
                    }

                    return true;
                }

                if ( le.isMouseRightButtonPressedInArea( _selectConditionRoi ) ) {
                    fheroes2::ArtifactDialogElement( Artifact( static_cast<int>( _victoryArtifactId ) ) ).showPopup( Dialog::ZERO );

                    return false;
                }

                if ( le.MouseClickLeft( _allowNormalVictoryRoi ) ) {
                    _isNormalVictoryAllowed = !_isNormalVictoryAllowed;

                    return true;
                }

                break;
            }
            case Maps::FileInfo::VICTORY_COLLECT_ENOUGH_GOLD: {
                if ( _goldAccumulationValue.processEvents() ) {
                    return true;
                }

                LocalEvent & le = LocalEvent::Get();

                if ( le.MouseClickLeft( _allowNormalVictoryRoi ) ) {
                    _isNormalVictoryAllowed = !_isNormalVictoryAllowed;

                    return true;
                }

                if ( le.MouseClickLeft( _allowVictoryConditionForAIRoi ) ) {
                    _isVictoryConditionApplicableForAI = !_isVictoryConditionApplicableForAI;

                    return true;
                }

                break;
            }
            default:
                // Did you add more conditions? Add the logic for them!
                assert( 0 );

                break;
            }

            return false;
        }

    private:
        uint8_t _conditionType{ Maps::FileInfo::VICTORY_DEFEAT_EVERYONE };
        bool _isNormalVictoryAllowed{ false };
        bool _isVictoryConditionApplicableForAI{ false };
        const bool _isEvilInterface{ false };
        uint32_t _victoryArtifactId{ ultimateArtifactId };
        const int32_t _mapWidth{ 0 };
        // Town or hero loss metadata include tile ID and color.
        std::array<uint32_t, 2> _heroToKill{ 0 };
        std::array<uint32_t, 2> _townToCapture{ 0 };
        std::vector<TownInfo> _mapTownInfos;
        std::vector<HeroInfo> _mapHeroInfos;

        fheroes2::ImageRestorer _restorer;

        fheroes2::ValueSelectionDialogElement _goldAccumulationValue{ 10000, 1000000, 10000, 1000, {} };

        fheroes2::MovableSprite _allowNormalVictory;
        fheroes2::MovableSprite _allowVictoryConditionForAI;
        fheroes2::Rect _allowNormalVictoryRoi;
        fheroes2::Rect _allowVictoryConditionForAIRoi;
        fheroes2::Rect _selectConditionRoi;
    };

    class LossConditionUI final
    {
    public:
        LossConditionUI( fheroes2::Image & output, const fheroes2::Rect & roi, const Maps::Map_Format::MapFormat & mapFormat, const bool isEvilInterface )
            : _conditionType( mapFormat.lossConditionType )
            , _isEvilInterface( isEvilInterface )
            , _mapWidth( mapFormat.size )
            , _restorer( output, roi.x, roi.y, roi.width, roi.height )
        {
            switch ( _conditionType ) {
            case Maps::FileInfo::LOSS_EVERYTHING:
                // This condition has no metadata.

                break;
            case Maps::FileInfo::LOSS_TOWN:
                if ( mapFormat.lossConditionMetadata.size() == 2 ) {
                    std::copy( mapFormat.lossConditionMetadata.begin(), mapFormat.lossConditionMetadata.end(), _townToLose.begin() );

                    // Verify that this is a valid human-only town.
                    _mapTownInfos = getMapTowns( mapFormat, mapFormat.humanPlayerColors & ( ~mapFormat.computerPlayerColors ), true );
                    const int32_t townTileIndex = static_cast<int32_t>( _townToLose[0] );

                    bool townFound = false;
                    for ( const auto & town : _mapTownInfos ) {
                        if ( townTileIndex == town.tileIndex && static_cast<int32_t>( _townToLose[1] ) == town.color ) {
                            townFound = true;
                            break;
                        }
                    }

                    if ( !townFound ) {
                        _conditionType = Maps::FileInfo::LOSS_EVERYTHING;
                    }
                }
                else {
                    // Since the metadata is invalid we have 2 options:
                    // - fall back to normal loss condition as no town was set
                    // - generate a list of towns and pick one
                    // For simplicity we are choosing the first option for now.
                    _conditionType = Maps::FileInfo::LOSS_EVERYTHING;
                }

                break;
            case Maps::FileInfo::LOSS_HERO:
                if ( mapFormat.lossConditionMetadata.size() == 2 ) {
                    std::copy( mapFormat.lossConditionMetadata.begin(), mapFormat.lossConditionMetadata.end(), _heroToLose.begin() );

                    // Verify that this is a valid human-only hero.
                    _mapHeroInfos = getMapHeroes( mapFormat, mapFormat.humanPlayerColors & ( ~mapFormat.computerPlayerColors ) );
                    const int32_t heroTileIndex = static_cast<int32_t>( _heroToLose[0] );
                    bool heroFound = false;
                    for ( const auto & hero : _mapHeroInfos ) {
                        if ( heroTileIndex == hero.tileIndex && static_cast<int32_t>( _heroToLose[1] ) == hero.color ) {
                            heroFound = true;
                            break;
                        }
                    }

                    if ( !heroFound ) {
                        _conditionType = Maps::FileInfo::LOSS_EVERYTHING;
                    }
                }
                else {
                    // Since the metadata is invalid we have 2 options:
                    // - fall back to normal loss condition as no hero was set
                    // - generate a list of heroes and pick one
                    // For simplicity we are choosing the first option for now.
                    _conditionType = Maps::FileInfo::LOSS_EVERYTHING;
                }

                break;
            case Maps::FileInfo::LOSS_OUT_OF_TIME:
                if ( mapFormat.lossConditionMetadata.size() == 1 ) {
                    _outOfTimeValue.setValue( static_cast<int32_t>( mapFormat.lossConditionMetadata[0] ) );
                }

                break;
            default:
                // Did you add more conditions? Add the logic for them!
                assert( 0 );

                break;
            }
        }

        void setConditionType( const uint8_t lossConditionType )
        {
            _conditionType = lossConditionType;
        }

        bool updateCondition( Maps::Map_Format::MapFormat & mapFormat )
        {
            switch ( _conditionType ) {
            case Maps::FileInfo::LOSS_TOWN: {
                _mapTownInfos = getMapTowns( mapFormat, mapFormat.humanPlayerColors & ( ~mapFormat.computerPlayerColors ), true );
                if ( _mapTownInfos.empty() ) {
                    // No towns exist for human-only players.
                    _conditionType = Maps::FileInfo::LOSS_EVERYTHING;
                    mapFormat.lossConditionType = _conditionType;

                    return true;
                }

                const int32_t townTileIndex = static_cast<int32_t>( _townToLose[0] );

                bool townFound = false;
                for ( const auto & town : _mapTownInfos ) {
                    if ( townTileIndex == town.tileIndex && static_cast<int32_t>( _townToLose[1] ) == town.color ) {
                        townFound = true;
                        break;
                    }
                }

                if ( !townFound ) {
                    // The town doesn't exist in the list. Select the first one.
                    _townToLose[0] = static_cast<uint32_t>( _mapTownInfos[0].tileIndex );
                    _townToLose[1] = static_cast<uint32_t>( _mapTownInfos[0].color );
                    return true;
                }

                break;
            }
            case Maps::FileInfo::LOSS_HERO: {
                _mapHeroInfos = getMapHeroes( mapFormat, mapFormat.humanPlayerColors & ( ~mapFormat.computerPlayerColors ) );
                if ( _mapHeroInfos.empty() ) {
                    // No heroes exist for human-only players.
                    _conditionType = Maps::FileInfo::LOSS_EVERYTHING;
                    mapFormat.lossConditionType = _conditionType;

                    return true;
                }

                const int32_t heroTileIndex = static_cast<int32_t>( _heroToLose[0] );
                bool heroFound = false;
                for ( const auto & hero : _mapHeroInfos ) {
                    if ( heroTileIndex == hero.tileIndex && static_cast<int32_t>( _heroToLose[1] ) == hero.color ) {
                        heroFound = true;
                        break;
                    }
                }

                if ( !heroFound ) {
                    // The hero doesn't exist in the list. Select the first one.
                    _heroToLose[0] = static_cast<uint32_t>( _mapHeroInfos[0].tileIndex );
                    _heroToLose[1] = static_cast<uint32_t>( _mapHeroInfos[0].color );
                }

                return false;
            }
            default:
                // No changes for other loss conditions.
                break;
            }

            return false;
        }

        void getConditionMetadata( Maps::Map_Format::MapFormat & mapFormat ) const
        {
            assert( _conditionType == mapFormat.lossConditionType );

            switch ( _conditionType ) {
            case Maps::FileInfo::LOSS_EVERYTHING:
                // This condition has no metadata.
                mapFormat.lossConditionMetadata.clear();

                break;
            case Maps::FileInfo::LOSS_TOWN:
                if ( mapFormat.lossConditionMetadata.size() != 2 ) {
                    mapFormat.lossConditionMetadata.resize( 2 );
                }

                std::copy( _townToLose.begin(), _townToLose.end(), mapFormat.lossConditionMetadata.begin() );

                return;
            case Maps::FileInfo::LOSS_HERO:
                if ( mapFormat.lossConditionMetadata.size() != 2 ) {
                    mapFormat.lossConditionMetadata.resize( 2 );
                }

                std::copy( _heroToLose.begin(), _heroToLose.end(), mapFormat.lossConditionMetadata.begin() );

                return;
            case Maps::FileInfo::LOSS_OUT_OF_TIME:
                if ( mapFormat.lossConditionMetadata.size() != 1 ) {
                    mapFormat.lossConditionMetadata.resize( 1 );
                }

                mapFormat.lossConditionMetadata[0] = static_cast<uint32_t>( _outOfTimeValue.getValue() );

                break;
            default:
                // Did you add more conditions? Add the logic for them!
                assert( 0 );

                // Reset the unknown condition to the default condition type.
                mapFormat.lossConditionType = Maps::FileInfo::LOSS_EVERYTHING;

                break;
            }
        }

        void render( fheroes2::Image & output, const bool renderEverything )
        {
            // Restore background to make sure that other UI elements aren't being rendered.
            _restorer.restore();

            switch ( _conditionType ) {
            case Maps::FileInfo::LOSS_EVERYTHING:
                // No special UI is needed.

                break;
            case Maps::FileInfo::LOSS_TOWN: {
                if ( !renderEverything ) {
                    // To render this condition we always redraw the whole conditions UI.
                    // TODO: optimize the rendering.
                    _restorer.restore();
                }

                assert( !_mapTownInfos.empty() );

                size_t selectedTownIndex = 0;
                for ( size_t i = 0; i < _mapTownInfos.size(); ++i ) {
                    if ( static_cast<int32_t>( _townToLose[0] ) == _mapTownInfos[i].tileIndex && static_cast<int32_t>( _townToLose[1] ) == _mapTownInfos[i].color ) {
                        selectedTownIndex = i;
                        break;
                    }
                }

                const auto & townInfo = _mapTownInfos[selectedTownIndex];
                const auto * castleMetadata = townInfo.castleMetadata;
                const int townIcnId = _isEvilInterface ? ICN::LOCATORE : ICN::LOCATORS;

                const bool isTown
                    = std::find( castleMetadata->builtBuildings.begin(), castleMetadata->builtBuildings.end(), BUILD_CASTLE ) == castleMetadata->builtBuildings.end();

                const fheroes2::Sprite townIcon( getTownIcon( isTown, townInfo.race, townInfo.color, townIcnId ) );

                const fheroes2::Rect roi{ _restorer.rect() };
                fheroes2::Blit( townIcon, output, roi.x, roi.y + 4 );

                fheroes2::Text text( getTownTitle( castleMetadata->customName, townInfo.race, isTown, townInfo.tileIndex, _mapWidth ),
                                     fheroes2::FontType::normalWhite() );
                text.fitToOneRow( roi.width - townIcon.width() - 5 );
                text.draw( roi.x + townIcon.width() + 5, roi.y + 12, output );

                _selectConditionRoi = { roi.x, roi.y + 4, townIcon.width() + 5 + text.width(), townIcon.height() };

                break;
            }
            case Maps::FileInfo::LOSS_HERO: {
                if ( !renderEverything ) {
                    // To render this condition we always redraw the whole conditions UI.
                    // TODO: optimize the rendering.
                    _restorer.restore();
                }

                const fheroes2::Rect roi{ _restorer.rect() };

                const fheroes2::Sprite & heroFrame = fheroes2::AGG::GetICN( ICN::SWAPWIN, 0 );

                const int32_t heroFrameWidth = 111;
                const int32_t heroFrameHeight = 105;

                _selectConditionRoi = { roi.x + ( roi.width - heroFrameWidth ) / 2, roi.y + 4, heroFrameWidth, heroFrameHeight };

                fheroes2::Blit( heroFrame, 88, 66, output, _selectConditionRoi.x, _selectConditionRoi.y, heroFrameWidth, heroFrameHeight );

                assert( !_mapHeroInfos.empty() );

                size_t selectedHeroIndex = 0;
                for ( size_t i = 0; i < _mapHeroInfos.size(); ++i ) {
                    if ( static_cast<int32_t>( _heroToLose[0] ) == _mapHeroInfos[i].tileIndex && static_cast<int32_t>( _heroToLose[1] ) == _mapHeroInfos[i].color ) {
                        selectedHeroIndex = i;
                        break;
                    }
                }

                // To render hero icons we use castle flags and frame.
                const uint32_t flagIcnIndex = fheroes2::getCastleLeftFlagIcnIndex( static_cast<int>( _heroToLose[1] ) );
                const fheroes2::Sprite & castleLeftFlag = fheroes2::AGG::GetICN( ICN::FLAG32, flagIcnIndex );
                const fheroes2::Sprite & castleRightFlag = fheroes2::AGG::GetICN( ICN::FLAG32, flagIcnIndex + 1 );
                Blit( castleLeftFlag, 0, 0, output, _selectConditionRoi.x - 21, _selectConditionRoi.y + 45, castleLeftFlag.width(), castleLeftFlag.height() );
                Blit( castleRightFlag, 0, 0, output, _selectConditionRoi.x + _selectConditionRoi.width + 2, _selectConditionRoi.y + 45, castleRightFlag.width(),
                      castleRightFlag.height() );

                const auto * heroMetadata = _mapHeroInfos[selectedHeroIndex].heroMetadata;
                const int32_t heroPortraitId = heroMetadata->customPortrait;

                if ( heroPortraitId > 0 ) {
                    const fheroes2::Sprite & heroPortrait = fheroes2::AGG::GetICN( ICN::PORTxxxx( heroPortraitId ), 0 );

                    fheroes2::Copy( heroPortrait, 0, 0, output, _selectConditionRoi.x + 5, _selectConditionRoi.y + 6, heroPortrait.width(), heroPortrait.height() );
                }
                else {
                    fheroes2::renderHeroRacePortrait( heroMetadata->race, { _selectConditionRoi.x + 5, _selectConditionRoi.y + 6, 101, 93 }, output );
                }

                fheroes2::Text extraText( getHeroTitle( heroMetadata->customName, heroMetadata->race, static_cast<int32_t>( _heroToLose[0] ), _mapWidth ),
                                          fheroes2::FontType::normalWhite() );
                extraText.fitToOneRow( roi.width );
                extraText.drawInRoi( roi.x, _selectConditionRoi.y + _selectConditionRoi.height + 5, roi.width, output, roi );

                break;
            }
            case Maps::FileInfo::LOSS_OUT_OF_TIME: {
                const fheroes2::Rect roi{ _restorer.x(), _restorer.y(), _restorer.width(), _restorer.height() };
                const fheroes2::Point uiOffset{ roi.x + ( roi.width - fheroes2::ValueSelectionDialogElement::getArea().width ) / 2, roi.y };

                _outOfTimeValue.setOffset( uiOffset );
                _outOfTimeValue.draw( output );

                fheroes2::Text text( _( "Days:" ), fheroes2::FontType::normalWhite() );
                text.draw( uiOffset.x - text.width() - 5, roi.y + ( fheroes2::ValueSelectionDialogElement::getArea().height - text.height() ) / 2 + 2, output );

                const int32_t offsetY = roi.y + fheroes2::ValueSelectionDialogElement::getArea().height + 14;

                text.set( Editor::getDateDescription( _outOfTimeValue.getValue() ), fheroes2::FontType::normalWhite() );
                text.draw( roi.x, offsetY, roi.width, output );

                break;
            }
            default:
                // Did you add more conditions? Add the logic for them!
                assert( 0 );

                break;
            }
        }

        // Returns true if rendering is required.
        bool processEvents()
        {
            switch ( _conditionType ) {
            case Maps::FileInfo::LOSS_EVERYTHING:
                // No events to process.

                return false;
            case Maps::FileInfo::LOSS_TOWN: {
                LocalEvent & le = LocalEvent::Get();

                if ( le.MouseClickLeft( _selectConditionRoi ) ) {
                    assert( !_mapTownInfos.empty() );

                    const int32_t maxHeight = std::min( 100 + SelectMapCastle::itemsOffsetY * 12, fheroes2::Display::instance().height() - 100 );
                    const int32_t itemsHeight
                        = std::max( 100 + SelectMapCastle::itemsOffsetY * static_cast<int32_t>( _mapTownInfos.size() ), 100 + SelectMapCastle::itemsOffsetY * 5 );
                    const int32_t totalHeight = std::min( itemsHeight, maxHeight );

                    SelectMapCastle listbox( { 450, totalHeight }, _( "Select a Town to lose to suffer defeat" ), {}, _mapWidth, _mapTownInfos, _isEvilInterface );

                    std::vector<int> townIndicies( _mapTownInfos.size() );
                    std::iota( townIndicies.begin(), townIndicies.end(), 0 );

                    listbox.SetListContent( townIndicies );

                    int initiallySelectedTownIndex = 0;

                    for ( size_t i = 0; i < _mapTownInfos.size(); ++i ) {
                        if ( static_cast<int32_t>( _townToLose[0] ) == _mapTownInfos[i].tileIndex && static_cast<int32_t>( _townToLose[1] ) == _mapTownInfos[i].color ) {
                            initiallySelectedTownIndex = static_cast<int>( i );
                            listbox.SetCurrent( initiallySelectedTownIndex );
                            break;
                        }
                    }

                    const int32_t result = listbox.selectItemsEventProcessing();

                    if ( result == Dialog::OK ) {
                        const int townIndex = listbox.GetCurrent();

                        if ( townIndex != initiallySelectedTownIndex ) {
                            _townToLose[0] = static_cast<uint32_t>( _mapTownInfos[townIndex].tileIndex );
                            _townToLose[1] = static_cast<uint32_t>( _mapTownInfos[townIndex].color );
                        }
                    }

                    return true;
                }

                if ( le.isMouseRightButtonPressedInArea( _selectConditionRoi ) ) {
                    fheroes2::showStandardTextMessage( _( "Special Loss Condition" ), _( "Click here to change the town whose loss would mean defeat." ), Dialog::ZERO );
                    return false;
                }

                break;
            }
            case Maps::FileInfo::LOSS_HERO: {
                LocalEvent & le = LocalEvent::Get();

                if ( le.MouseClickLeft( _selectConditionRoi ) ) {
                    assert( !_mapHeroInfos.empty() );

                    const int32_t maxHeight = std::min( 100 + SelectMapCastle::itemsOffsetY * 12, fheroes2::Display::instance().height() - 100 );
                    const int32_t itemsHeight
                        = std::max( 100 + SelectMapCastle::itemsOffsetY * static_cast<int32_t>( _mapHeroInfos.size() ), 100 + SelectMapCastle::itemsOffsetY * 5 );
                    const int32_t totalHeight = std::min( itemsHeight, maxHeight );

                    SelectMapHero listbox( { 450, totalHeight }, _( "Select a Hero to lose to suffer defeat" ), {}, _mapWidth, _mapHeroInfos, _isEvilInterface );

                    std::vector<int> heroIndicies( _mapHeroInfos.size() );
                    std::iota( heroIndicies.begin(), heroIndicies.end(), 0 );

                    listbox.SetListContent( heroIndicies );

                    int initiallySelectedHeroIndex = 0;

                    for ( size_t i = 0; i < _mapHeroInfos.size(); ++i ) {
                        if ( static_cast<int32_t>( _heroToLose[0] ) == _mapHeroInfos[i].tileIndex && static_cast<int32_t>( _heroToLose[1] ) == _mapHeroInfos[i].color ) {
                            initiallySelectedHeroIndex = static_cast<int>( i );
                            listbox.SetCurrent( initiallySelectedHeroIndex );
                            break;
                        }
                    }

                    const int32_t result = listbox.selectItemsEventProcessing();

                    if ( result == Dialog::OK ) {
                        const int heroIndex = listbox.GetCurrent();

                        if ( heroIndex != initiallySelectedHeroIndex ) {
                            _heroToLose[0] = static_cast<uint32_t>( _mapHeroInfos[heroIndex].tileIndex );
                            _heroToLose[1] = static_cast<uint32_t>( _mapHeroInfos[heroIndex].color );
                        }
                    }

                    return true;
                }

                if ( le.isMouseRightButtonPressedInArea( _selectConditionRoi ) ) {
                    fheroes2::showStandardTextMessage( _( "Special Loss Condition" ), _( "Click here to change the hero whose loss would mean defeat." ), Dialog::ZERO );
                    return false;
                }

                break;
            }
            case Maps::FileInfo::LOSS_OUT_OF_TIME:
                return _outOfTimeValue.processEvents();
            default:
                // Did you add more conditions? Add the logic for them!
                assert( 0 );

                break;
            }

            return false;
        }

    private:
        uint8_t _conditionType{ Maps::FileInfo::LOSS_EVERYTHING };
        const bool _isEvilInterface{ false };
        const int32_t _mapWidth{ 0 };
        std::array<uint32_t, 2> _heroToLose{ 0 };
        std::array<uint32_t, 2> _townToLose{ 0 };
        std::vector<TownInfo> _mapTownInfos;
        std::vector<HeroInfo> _mapHeroInfos;

        fheroes2::ImageRestorer _restorer;

        fheroes2::ValueSelectionDialogElement _outOfTimeValue{ 1, 10 * daysInYear, daysInMonth, 1, {} };

        fheroes2::Rect _selectConditionRoi;
    };

    uint8_t showWinLoseList( const Maps::Map_Format::MapFormat & mapFormat, const fheroes2::Point & offset, const uint8_t selectedCondition, const bool isLossList,
                             const int dropBoxIcn )
    {
        std::vector<uint8_t> conditions = isLossList ? supportedLossConditions : supportedVictoryConditions;
        assert( std::find( conditions.begin(), conditions.end(), selectedCondition ) != conditions.end() );

        if ( isLossList ) {
            // Remove the conditions that have no selection among objects.
            if ( getMapHeroes( mapFormat, mapFormat.humanPlayerColors & ( ~mapFormat.computerPlayerColors ) ).empty() ) {
                conditions.erase( std::remove_if( conditions.begin(), conditions.end(),
                                                  []( const uint8_t condition ) { return condition == Maps::FileInfo::LOSS_HERO; } ),
                                  conditions.end() );
            }

            if ( getMapTowns( mapFormat, mapFormat.humanPlayerColors & ( ~mapFormat.computerPlayerColors ), true ).empty() ) {
                conditions.erase( std::remove_if( conditions.begin(), conditions.end(),
                                                  []( const uint8_t condition ) { return condition == Maps::FileInfo::LOSS_TOWN; } ),
                                  conditions.end() );
            }
        }
        else {
            // Remove the conditions that have no selection among objects.
            if ( getMapHeroes( mapFormat, mapFormat.computerPlayerColors & ( ~mapFormat.humanPlayerColors ) ).empty() ) {
                conditions.erase( std::remove_if( conditions.begin(), conditions.end(),
                                                  []( const uint8_t condition ) { return condition == Maps::FileInfo::VICTORY_KILL_HERO; } ),
                                  conditions.end() );
            }

            if ( getMapTowns( mapFormat, mapFormat.computerPlayerColors & ( ~mapFormat.humanPlayerColors ), false ).empty() ) {
                conditions.erase( std::remove_if( conditions.begin(), conditions.end(),
                                                  []( const uint8_t condition ) { return condition == Maps::FileInfo::VICTORY_CAPTURE_TOWN; } ),
                                  conditions.end() );
            }
        }

        DropBoxList conditionList( offset, static_cast<int32_t>( conditions.size() ), isLossList, dropBoxIcn );
        conditionList.SetListContent( conditions );
        conditionList.SetCurrent( selectedCondition );
        conditionList.Redraw();

        const fheroes2::Rect listArea( conditionList.getArea() );

        fheroes2::Display & display = fheroes2::Display::instance();
        display.render( listArea );

        LocalEvent & le = LocalEvent::Get();

        while ( le.HandleEvents() ) {
            conditionList.QueueEventProcessing();

            if ( conditionList.isClicked() || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) ) {
                assert( conditionList.IsValid() );

                return conditionList.GetCurrent();
            }

            if ( le.MouseClickLeft() || Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) ) {
                break;
            }

            if ( conditionList.IsNeedRedraw() ) {
                conditionList.Redraw();
                display.render( listArea );
            }
        }

        return selectedCondition;
    }

    uint32_t getPlayerIcnIndex( const Maps::Map_Format::MapFormat & mapFormat, const int currentColor )
    {
        if ( !( mapFormat.availablePlayerColors & currentColor ) ) {
            // This player is not available.
            assert( 0 );

            return 70;
        }

        if ( mapFormat.humanPlayerColors & currentColor ) {
            if ( mapFormat.computerPlayerColors & currentColor ) {
                // Both AI and human can choose this player color.
                return 82;
            }

            // Human only.
            return 9;
        }

        if ( mapFormat.computerPlayerColors & currentColor ) {
            // AI only.
            return 3;
        }

        // It is not possible to have available player which is not allowed to be controlled by AI or human.
        assert( 0 );

        return 70;
    }

    size_t getDifficultyIndex( const uint8_t difficulty )
    {
        switch ( difficulty ) {
        case Difficulty::EASY:
            return 0;
        case Difficulty::NORMAL:
            return 1;
        case Difficulty::HARD:
            return 2;
        case Difficulty::EXPERT:
            return 3;
        default:
            // Did you add a new difficulty mode? Add the corresponding case above!
            assert( 0 );
            break;
        }

        return 0;
    }

    uint8_t setDifficultyByIndex( const size_t difficultyIndex )
    {
        switch ( difficultyIndex ) {
        case 0:
            return Difficulty::EASY;
        case 1:
            return Difficulty::NORMAL;
        case 2:
            return Difficulty::HARD;
        case 3:
            return Difficulty::EXPERT;
        default:
            // Did you add a new difficulty mode? Add the corresponding case above!
            assert( 0 );
            break;
        }

        return Difficulty::EASY;
    }
}

namespace Editor
{
    bool mapSpecificationsDialog( Maps::Map_Format::MapFormat & mapFormat )
    {
        // Verify victory and loss condition types.
        if ( std::find( supportedVictoryConditions.begin(), supportedVictoryConditions.end(), mapFormat.victoryConditionType ) == supportedVictoryConditions.end() ) {
            assert( 0 );
            mapFormat.victoryConditionType = Maps::FileInfo::VICTORY_DEFEAT_EVERYONE;
        }

        if ( std::find( supportedLossConditions.begin(), supportedLossConditions.end(), mapFormat.lossConditionType ) == supportedLossConditions.end() ) {
            assert( 0 );
            mapFormat.lossConditionType = Maps::FileInfo::LOSS_EVERYTHING;
        }

        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        fheroes2::Display & display = fheroes2::Display::instance();

        const bool isDefaultScreenSize = display.isDefaultSize();

        fheroes2::StandardWindow background( fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT, !isDefaultScreenSize );
        const fheroes2::Rect activeArea( background.activeArea() );

        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

        if ( isDefaultScreenSize ) {
            const fheroes2::Sprite & backgroundImage = fheroes2::AGG::GetICN( isEvilInterface ? ICN::STONEBAK_EVIL : ICN::STONEBAK, 0 );
            fheroes2::Copy( backgroundImage, 0, 0, display, activeArea );
        }

        if ( mapFormat.name.empty() ) {
            mapFormat.name = "My Map";
        }

        // Map name.
        const fheroes2::Sprite & scenarioBox = fheroes2::AGG::GetICN( isEvilInterface ? ICN::METALLIC_BORDERED_TEXTBOX_EVIL : ICN::METALLIC_BORDERED_TEXTBOX_GOOD, 0 );
        const fheroes2::Rect scenarioBoxRoi( activeArea.x + ( activeArea.width - scenarioBox.width() ) / 2, activeArea.y + 10, scenarioBox.width(),
                                             scenarioBox.height() );
        const fheroes2::Rect mapNameRoi( scenarioBoxRoi.x + 6, scenarioBoxRoi.y + 5, scenarioBoxRoi.width - 12, scenarioBoxRoi.height - 11 );

        fheroes2::Copy( scenarioBox, 0, 0, display, scenarioBoxRoi );
        fheroes2::addGradientShadow( scenarioBox, display, scenarioBoxRoi.getPosition(), { -5, 5 } );

        fheroes2::Text text( mapFormat.name, fheroes2::FontType::normalWhite() );
        text.drawInRoi( mapNameRoi.x, mapNameRoi.y + 3, mapNameRoi.width, display, mapNameRoi );

        // Players setting (AI or human).
        const int32_t availablePlayersCount = Color::Count( mapFormat.availablePlayerColors );
        int32_t offsetX = activeArea.x + ( activeArea.width - availablePlayersCount * playerStepX ) / 2;
        int32_t offsetY = scenarioBoxRoi.y + scenarioBoxRoi.height + 10;

        std::vector<fheroes2::Rect> playerRects( availablePlayersCount );
        const Colors availableColors( mapFormat.availablePlayerColors );

        const fheroes2::Sprite & playerIconShadow = fheroes2::AGG::GetICN( ICN::NGEXTRA, 61 );
        for ( int32_t i = 0; i < availablePlayersCount; ++i ) {
            playerRects[i].x = offsetX + i * playerStepX;
            playerRects[i].y = offsetY;

            fheroes2::Blit( playerIconShadow, display, playerRects[i].x - 5, playerRects[i].y + 3 );

            const uint32_t icnIndex = Color::GetIndex( availableColors[i] ) + getPlayerIcnIndex( mapFormat, availableColors[i] );

            const fheroes2::Sprite & playerIcon = fheroes2::AGG::GetICN( ICN::NGEXTRA, icnIndex );
            playerRects[i].width = playerIcon.width();
            playerRects[i].height = playerIcon.height();
            fheroes2::Copy( playerIcon, 0, 0, display, playerRects[i].x, playerRects[i].y, playerRects[i].width, playerRects[i].height );
        }

        // Draw difficulty icons.
        offsetX = activeArea.x + 15;
        offsetY = scenarioBoxRoi.y + scenarioBoxRoi.height + 65;

        text.set( _( "Map Difficulty" ), fheroes2::FontType::normalWhite() );
        text.draw( offsetX + ( difficultyStepX * 4 - text.width() ) / 2, scenarioBoxRoi.y + scenarioBoxRoi.height + 65, display );

        std::array<fheroes2::Rect, 4> difficultyRects;
        offsetY += 23;

        const fheroes2::Sprite & difficultyCursorSprite = fheroes2::AGG::GetICN( ICN::NGEXTRA, 62 );
        const int32_t difficultyIconSideLength = difficultyCursorSprite.width();
        const int difficultyIcnIndex = isEvilInterface ? 1 : 0;

        for ( int i = 0; i < 4; ++i ) {
            difficultyRects[i].x = offsetX + difficultyStepX * i;
            difficultyRects[i].y = offsetY;
            difficultyRects[i].width = difficultyIconSideLength;
            difficultyRects[i].height = difficultyIconSideLength;

            const fheroes2::Sprite & icon = fheroes2::AGG::GetICN( ICN::DIFFICULTY_ICON_EASY + i, difficultyIcnIndex );
            fheroes2::Copy( icon, 0, 0, display, difficultyRects[i] );
            fheroes2::addGradientShadow( icon, display, { difficultyRects[i].x, difficultyRects[i].y }, { -5, 5 } );

            difficultyRects[i].x -= 3;
            difficultyRects[i].y -= 3;

            text.set( Difficulty::String( setDifficultyByIndex( i ) ), fheroes2::FontType::smallWhite() );
            text.draw( difficultyRects[i].x + ( difficultyRects[i].width - text.width() ) / 2, difficultyRects[i].y + difficultyRects[i].height + 7, display );
        }

        fheroes2::MovableSprite difficultyCursor( difficultyCursorSprite );

        size_t difficultyIndex = getDifficultyIndex( mapFormat.difficulty );
        difficultyCursor.setPosition( difficultyRects[difficultyIndex].x, difficultyRects[difficultyIndex].y );
        difficultyCursor.redraw();

        // Map description.
        offsetX = activeArea.x + activeArea.width - descriptionBoxWidth - 15;
        offsetY -= 23;

        text.set( _( "Map Description" ), fheroes2::FontType::normalWhite() );
        text.draw( offsetX + ( descriptionBoxWidth - text.width() ) / 2, offsetY, display );

        offsetY += 25;
        const fheroes2::Rect descriptionTextRoi( offsetX, offsetY, descriptionBoxWidth, descriptionBoxHeight );
        background.applyTextBackgroundShading( { descriptionTextRoi.x - 6, descriptionTextRoi.y - 6, descriptionTextRoi.width + 12, descriptionTextRoi.height + 12 } );
        fheroes2::ImageRestorer descriptionBackground( display, descriptionTextRoi.x, descriptionTextRoi.y, descriptionTextRoi.width, descriptionTextRoi.height );

        text.set( mapFormat.description, fheroes2::FontType::normalWhite() );
        text.drawInRoi( descriptionTextRoi.x, descriptionTextRoi.y, descriptionTextRoi.width, display, descriptionTextRoi );

        // Victory conditions.
        offsetY += descriptionTextRoi.height + 20;

        text.set( _( "Special Victory Condition" ), fheroes2::FontType::normalWhite() );
        text.draw( activeArea.x + activeArea.width / 4 - text.width() / 2, offsetY, display );

        offsetY += 20;

        const int dropListIcn = isEvilInterface ? ICN::DROPLISL_EVIL : ICN::DROPLISL;
        const fheroes2::Sprite & itemBackground = fheroes2::AGG::GetICN( dropListIcn, 0 );
        const int32_t itemBackgroundWidth = itemBackground.width();
        const int32_t itemBackgroundHeight = itemBackground.height();
        const int32_t itemBackgroundOffsetX = activeArea.width / 4 - itemBackgroundWidth / 2 - 11;

        offsetX = activeArea.x + itemBackgroundOffsetX;
        fheroes2::Copy( itemBackground, 0, 0, display, offsetX, offsetY, itemBackgroundWidth, itemBackgroundHeight );
        const fheroes2::Rect victoryTextRoi( offsetX + 2, offsetY + 3, itemBackgroundWidth - 4, itemBackgroundHeight - 5 );

        redrawVictoryCondition( mapFormat.victoryConditionType, victoryTextRoi, false, display );

        const fheroes2::Sprite & dropListButtonSprite = fheroes2::AGG::GetICN( dropListIcn, 1 );
        const fheroes2::Sprite & dropListButtonPressedSprite = fheroes2::AGG::GetICN( dropListIcn, 2 );

        fheroes2::ButtonSprite victoryDroplistButton( offsetX + itemBackgroundWidth, offsetY, dropListButtonSprite, dropListButtonPressedSprite );
        const fheroes2::Rect victoryDroplistButtonRoi( fheroes2::getBoundaryRect( victoryDroplistButton.area(), victoryTextRoi ) );
        victoryDroplistButton.draw();

        offsetY += 30;

        const fheroes2::Rect victoryConditionUIRoi{ offsetX, offsetY, victoryDroplistButtonRoi.width, 150 };
        VictoryConditionUI victoryConditionUI( display, victoryConditionUIRoi, mapFormat, isEvilInterface );

        victoryConditionUI.render( display, true );

        // Loss conditions.
        offsetY = descriptionTextRoi.y + descriptionTextRoi.height + 20;

        text.set( _( "Special Loss Condition" ), fheroes2::FontType::normalWhite() );
        text.draw( activeArea.x + 3 * activeArea.width / 4 - text.width() / 2, offsetY, display );

        offsetY += 20;
        offsetX = activeArea.x + activeArea.width / 2 + itemBackgroundOffsetX;
        fheroes2::Copy( itemBackground, 0, 0, display, offsetX, offsetY, itemBackgroundWidth, itemBackground.height() );
        const fheroes2::Rect lossTextRoi( offsetX + 2, offsetY + 3, victoryTextRoi.width, victoryTextRoi.height );

        redrawLossCondition( mapFormat.lossConditionType, lossTextRoi, false, display );

        fheroes2::ButtonSprite lossDroplistButton( offsetX + itemBackgroundWidth, offsetY, dropListButtonSprite, dropListButtonPressedSprite );
        const fheroes2::Rect lossDroplistButtonRoi( fheroes2::getBoundaryRect( lossDroplistButton.area(), lossTextRoi ) );
        lossDroplistButton.draw();

        offsetY += 30;

        const fheroes2::Rect lossConditionUIRoi{ offsetX, offsetY, lossDroplistButtonRoi.width, 150 };
        LossConditionUI lossConditionUI( display, lossConditionUIRoi, mapFormat, isEvilInterface );

        lossConditionUI.render( display, true );

        // Buttons.
        fheroes2::Button buttonCancel;
        const int buttonCancelIcn = isEvilInterface ? ICN::BUTTON_SMALL_CANCEL_EVIL : ICN::BUTTON_SMALL_CANCEL_GOOD;
        background.renderButton( buttonCancel, buttonCancelIcn, 0, 1, { 20, 6 }, fheroes2::StandardWindow::Padding::BOTTOM_RIGHT );
        const fheroes2::Rect buttonCancelRoi( buttonCancel.area() );

        fheroes2::Button buttonOk;
        const int buttonOkIcn = isEvilInterface ? ICN::BUTTON_SMALL_OKAY_EVIL : ICN::BUTTON_SMALL_OKAY_GOOD;
        background.renderButton( buttonOk, buttonOkIcn, 0, 1, { 20 + buttonCancelRoi.width + 10, 6 }, fheroes2::StandardWindow::Padding::BOTTOM_RIGHT );
        const fheroes2::Rect buttonOkRoi( buttonOk.area() );

        fheroes2::Button buttonRumors;
        const int buttonRumorsIcn = isEvilInterface ? ICN::BUTTON_RUMORS_EVIL : ICN::BUTTON_RUMORS_GOOD;
        background.renderButton( buttonRumors, buttonRumorsIcn, 0, 1, { 20, 6 }, fheroes2::StandardWindow::Padding::BOTTOM_LEFT );
        const fheroes2::Rect buttonRumorsRoi( buttonRumors.area() );

        fheroes2::Button buttonEvents;
        const int buttonEventsIcn = isEvilInterface ? ICN::BUTTON_EVENTS_EVIL : ICN::BUTTON_EVENTS_GOOD;
        background.renderButton( buttonEvents, buttonEventsIcn, 0, 1, { 20 + buttonRumorsRoi.width + 10, 6 }, fheroes2::StandardWindow::Padding::BOTTOM_LEFT );
        const fheroes2::Rect buttonEventsRoi( buttonEvents.area() );

        LocalEvent & le = LocalEvent::Get();

        display.render( background.totalArea() );

        while ( le.HandleEvents() ) {
            buttonOk.drawOnState( le.isMouseLeftButtonPressedInArea( buttonOkRoi ) );
            buttonCancel.drawOnState( le.isMouseLeftButtonPressedInArea( buttonCancelRoi ) );
            buttonRumors.drawOnState( le.isMouseLeftButtonPressedInArea( buttonRumorsRoi ) );
            buttonEvents.drawOnState( le.isMouseLeftButtonPressedInArea( buttonEventsRoi ) );
            victoryDroplistButton.drawOnState( le.isMouseLeftButtonPressedInArea( victoryDroplistButtonRoi ) );
            lossDroplistButton.drawOnState( le.isMouseLeftButtonPressedInArea( lossDroplistButtonRoi ) );

            if ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_CANCEL ) || le.MouseClickLeft( buttonCancelRoi ) ) {
                return false;
            }

            if ( Game::HotKeyPressEvent( Game::HotKeyEvent::DEFAULT_OKAY ) || le.MouseClickLeft( buttonOkRoi ) ) {
                break;
            }

            if ( victoryConditionUI.processEvents() ) {
                victoryConditionUI.render( display, false );
                display.render( victoryConditionUIRoi );
            }
            else if ( lossConditionUI.processEvents() ) {
                lossConditionUI.render( display, false );
                display.render( lossConditionUIRoi );
            }
            else if ( le.MouseClickLeft( buttonRumorsRoi ) ) {
                auto temp = mapFormat.rumors;
                if ( openRumorWindow( temp ) ) {
                    mapFormat.rumors = std::move( temp );
                }

                display.render( background.totalArea() );
            }
            else if ( le.MouseClickLeft( buttonEventsRoi ) ) {
                auto temp = mapFormat.dailyEvents;
                if ( openDailyEventsWindow( temp, mapFormat.humanPlayerColors, mapFormat.computerPlayerColors ) ) {
                    mapFormat.dailyEvents = std::move( temp );
                }

                display.render( background.totalArea() );
            }
            else if ( le.MouseClickLeft( mapNameRoi ) ) {
                // TODO: Edit texts directly in this dialog.

                std::string editableMapName = mapFormat.name;
                if ( Dialog::inputString( _( "Change Map Name" ), editableMapName, {}, maxMapNameLength, false, true ) ) {
                    mapFormat.name = std::move( editableMapName );
                    text.set( mapFormat.name, fheroes2::FontType::normalWhite() );
                    fheroes2::Copy( scenarioBox, 0, 0, display, scenarioBoxRoi );
                    text.drawInRoi( mapNameRoi.x, mapNameRoi.y + 3, mapNameRoi.width, display, mapNameRoi );

                    display.render( scenarioBoxRoi );
                }
            }
            else if ( le.MouseClickLeft( descriptionTextRoi ) ) {
                // TODO: Edit texts directly in this dialog.
                // TODO: Limit description to 5 text lines.

                std::string signText = mapFormat.description;
                if ( Dialog::inputString( _( "Change Map Description" ), signText, {}, 150, true, true ) ) {
                    mapFormat.description = std::move( signText );

                    text.set( mapFormat.description, fheroes2::FontType::normalWhite() );

                    // TODO: Remove this temporary fix when direct text edit with text length checks is implemented.
                    if ( text.rows( descriptionTextRoi.width ) > 5 ) {
                        fheroes2::showStandardTextMessage(
                            _( "Warning" ), _( "The entered map description exceeds the maximum allowed 5 rows. It will be shortened to fit the map description field." ),
                            Dialog::OK );

                        // As a temporary solution we cut the end of the text to fit 5 rows.
                        while ( text.rows( descriptionTextRoi.width ) > 5 ) {
                            mapFormat.description.pop_back();
                            text.set( mapFormat.description, fheroes2::FontType::normalWhite() );
                        }
                    }

                    descriptionBackground.restore();
                    text.drawInRoi( descriptionTextRoi.x, descriptionTextRoi.y, descriptionTextRoi.width, display, descriptionTextRoi );
                    display.render( descriptionTextRoi );
                }
            }
            else if ( le.MouseClickLeft( victoryDroplistButtonRoi ) ) {
                const uint8_t result = showWinLoseList( mapFormat, { victoryTextRoi.x - 2, victoryTextRoi.y + victoryTextRoi.height }, mapFormat.victoryConditionType,
                                                        false, dropListIcn );

                if ( result != mapFormat.victoryConditionType ) {
                    mapFormat.victoryConditionType = result;

                    victoryConditionUI.setConditionType( mapFormat.victoryConditionType );
                    victoryConditionUI.updateCondition( mapFormat );
                    victoryConditionUI.render( display, true );

                    fheroes2::Copy( itemBackground, 2, 3, display, victoryTextRoi );
                    redrawVictoryCondition( mapFormat.victoryConditionType, victoryTextRoi, false, display );
                    display.render( fheroes2::getBoundaryRect( victoryTextRoi, victoryConditionUIRoi ) );
                }
            }
            else if ( le.MouseClickLeft( lossDroplistButtonRoi ) ) {
                const uint8_t result
                    = showWinLoseList( mapFormat, { lossTextRoi.x - 2, lossTextRoi.y + lossTextRoi.height }, mapFormat.lossConditionType, true, dropListIcn );

                if ( result != mapFormat.lossConditionType ) {
                    mapFormat.lossConditionType = result;

                    lossConditionUI.setConditionType( mapFormat.lossConditionType );
                    lossConditionUI.updateCondition( mapFormat );
                    lossConditionUI.render( display, true );

                    fheroes2::Copy( itemBackground, 2, 3, display, lossTextRoi );
                    redrawLossCondition( mapFormat.lossConditionType, lossTextRoi, false, display );
                    display.render( fheroes2::getBoundaryRect( lossTextRoi, lossConditionUIRoi ) );
                }
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonCancelRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Cancel" ), _( "Exit this menu without doing anything." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonOkRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Okay" ), _( "Click to accept the changes made." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonRumorsRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Rumors" ), _( "Click to edit custom rumors." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( buttonEventsRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Events" ), _( "Click to edit daily events." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( mapNameRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Map Name" ), _( "Click to change your map name." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( descriptionTextRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Map Description" ), _( "Click to change the description of the current map." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( victoryDroplistButtonRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Special Victory Condition" ), _( "Click to change the victory condition of the current map." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( lossDroplistButtonRoi ) ) {
                fheroes2::showStandardTextMessage( _( "Special Loss Condition" ), _( "Click to change the loss condition of the current map." ), Dialog::ZERO );
            }

            for ( int32_t i = 0; i < availablePlayersCount; ++i ) {
                if ( le.MouseClickLeft( playerRects[i] ) ) {
                    if ( !( mapFormat.availablePlayerColors & availableColors[i] ) ) {
                        break;
                    }

                    const bool allowAi = mapFormat.computerPlayerColors & availableColors[i];
                    const bool allowHuman = mapFormat.humanPlayerColors & availableColors[i];

                    if ( allowHuman ) {
                        if ( allowAi ) {
                            // Disable AI.
                            mapFormat.computerPlayerColors ^= availableColors[i];
                        }
                        else {
                            // Enable AI.
                            mapFormat.computerPlayerColors |= availableColors[i];
                            if ( Color::Count( mapFormat.humanPlayerColors ) > 1 ) {
                                // and disable human only if any other player can be controlled by human.
                                mapFormat.humanPlayerColors ^= availableColors[i];
                            }
                        }
                    }
                    else {
                        // Enable human.
                        mapFormat.humanPlayerColors |= availableColors[i];
                    }

                    fheroes2::Rect renderRoi;
                    if ( victoryConditionUI.updateCondition( mapFormat ) ) {
                        victoryConditionUI.render( display, true );

                        fheroes2::Copy( itemBackground, 2, 3, display, victoryTextRoi );
                        redrawVictoryCondition( mapFormat.victoryConditionType, victoryTextRoi, false, display );

                        renderRoi = fheroes2::getBoundaryRect( renderRoi, victoryConditionUIRoi );
                        renderRoi = fheroes2::getBoundaryRect( renderRoi, victoryTextRoi );
                    }

                    if ( lossConditionUI.updateCondition( mapFormat ) ) {
                        lossConditionUI.render( display, true );

                        fheroes2::Copy( itemBackground, 2, 3, display, lossTextRoi );
                        redrawLossCondition( mapFormat.lossConditionType, lossTextRoi, false, display );

                        renderRoi = fheroes2::getBoundaryRect( renderRoi, lossConditionUIRoi );
                        renderRoi = fheroes2::getBoundaryRect( renderRoi, lossTextRoi );
                    }

                    // Update player icon.
                    const uint32_t icnIndex = Color::GetIndex( availableColors[i] ) + getPlayerIcnIndex( mapFormat, availableColors[i] );
                    const fheroes2::Sprite & playerIcon = fheroes2::AGG::GetICN( ICN::NGEXTRA, icnIndex );
                    fheroes2::Copy( playerIcon, 0, 0, display, playerRects[i].x, playerRects[i].y, playerRects[i].width, playerRects[i].height );

                    renderRoi = fheroes2::getBoundaryRect( renderRoi, playerRects[i] );

                    display.render( renderRoi );

                    break;
                }

                if ( le.isMouseRightButtonPressedInArea( playerRects[i] ) ) {
                    fheroes2::showStandardTextMessage( _( "Player Type" ), _( "Indicates the player types in the scenario. Click to change." ), Dialog::ZERO );
                }
            }

            for ( size_t i = 0; i < 4; ++i ) {
                if ( le.MouseClickLeft( difficultyRects[i] ) ) {
                    if ( i == difficultyIndex ) {
                        // This difficulty is already selected.
                        break;
                    }

                    difficultyCursor.setPosition( difficultyRects[i].x, difficultyRects[i].y );
                    difficultyCursor.redraw();
                    mapFormat.difficulty = setDifficultyByIndex( i );

                    display.updateNextRenderRoi( difficultyRects[difficultyIndex] );

                    difficultyIndex = i;

                    display.render( difficultyRects[i] );

                    break;
                }

                if ( le.isMouseRightButtonPressedInArea( difficultyRects[i] ) ) {
                    fheroes2::showStandardTextMessage(
                        _( "Map Difficulty" ),
                        _( "Click to set map difficulty. More difficult maps might include more or stronger enemies, fewer resources, or other special conditions making things tougher for the human player." ),
                        Dialog::ZERO );
                }
            }
        }

        // Retrieve victory and loss conditions.
        victoryConditionUI.getConditionMetadata( mapFormat );
        lossConditionUI.getConditionMetadata( mapFormat );

        return true;
    }
}
