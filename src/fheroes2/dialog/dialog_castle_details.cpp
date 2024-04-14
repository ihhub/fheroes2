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

#include "dialog_castle_details.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "agg_image.h"
#include "army.h"
#include "army_bar.h"
#include "buildinginfo.h"
#include "castle.h"
#include "castle_building_info.h"
#include "color.h"
#include "cursor.h"
#include "dialog.h"
#include "game_hotkeys.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "map_format_info.h"
#include "math_base.h"
#include "monster.h"
#include "pal.h"
#include "profit.h"
#include "race.h"
#include "resource.h"
#include "screen.h"
#include "settings.h"
#include "statusbar.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_text.h"
#include "ui_tool.h"
#include "ui_window.h"

namespace
{
    class BuildingData
    {
    public:
        explicit BuildingData( building_t buildingType, const int race, const building_t builtBuildings, const building_t restrictedBuildings )
            : _mainBuildingType( buildingType )
            , _race( race )
        {
            if ( builtBuildings & _mainBuildingType ) {
                _builtId = 0;
            }
            else if ( restrictedBuildings & _mainBuildingType ) {
                _restrictedId = 0;
            }

            for ( building_t upgradedBuildingType = fheroes2::getUpgradeForBuilding( _race, _mainBuildingType ); upgradedBuildingType != buildingType;
                  upgradedBuildingType = fheroes2::getUpgradeForBuilding( _race, upgradedBuildingType ) ) {
                if ( builtBuildings & upgradedBuildingType ) {
                    _builtId = _buildingVariants;
                }
                else if ( restrictedBuildings & upgradedBuildingType ) {
                    _restrictedId = _buildingVariants;
                }

                ++_buildingVariants;
                buildingType = upgradedBuildingType;
            }
        }

        void setPosition( const int32_t posX, const int32_t posY )
        {
            _area.x = posX;
            _area.y = posY;
            _buildArea.x = posX;
            _buildArea.y = posY;
            _banArea.x = posX + _buildArea.width;
            _banArea.y = posY;
        }

        void resetBuilding()
        {
            _builtId = -1;
            _restrictedId = -1;
        }

        const fheroes2::Rect & getArea() const
        {
            return _area;
        }

        std::vector<building_t> getBuildLevel() const
        {
            if ( _builtId < 0 ) {
                return {};
            }

            const int8_t levelsBuilt = _builtId + 1;
            std::vector<building_t> buildings;
            buildings.reserve( levelsBuilt );
            for ( int8_t i = 0; i < levelsBuilt; ++i ) {
                buildings.push_back( _getBuildingType( i ) );
            }
            return buildings;
        }

        building_t getRestrictLevel() const
        {
            return _getBuildingType( _restrictedId );
        }

        bool queueEventProcessing( const bool restrictionMode )
        {
            LocalEvent & le = LocalEvent::Get();

            if ( le.MouseClickLeft() ) {
                if ( restrictionMode ) {
                    if ( _restrictedId <= -1 ) {
                        _restrictedId = _buildingVariants;
                    }

                    --_restrictedId;

                    if ( _builtId != -1 && _builtId >= _restrictedId ) {
                        _builtId = _restrictedId - 1;
                    }
                }
                else {
                    ++_builtId;

                    if ( _builtId >= _buildingVariants ) {
                        _builtId = -1;
                    }
                    else if ( _restrictedId != -1 && _builtId >= _restrictedId ) {
                        _restrictedId = ( _builtId < _buildingVariants - 1 ) ? _builtId + 1 : -1;
                    }
                }

                return true;
            }

            if ( le.MousePressRight() ) {
                std::string description;
                const building_t building = _getBuildindTypeForRender();
                if ( BuildingInfo::IsDwelling( building ) ) {
                    description = _( "The %{building} produces %{monster}." );
                    StringReplace( description, "%{building}", Castle::GetStringBuilding( building, _race ) );
                    if ( _race == Race::RAND ) {
                        StringReplace( description, "%{monster}", _getRandomMonstersName( building ) );
                    }
                    else {
                        StringReplaceWithLowercase( description, "%{monster}", Monster( _race, building ).GetMultiName() );
                    }
                }
                else {
                    description = fheroes2::getBuildingDescription( _race, building );

                    switch ( building ) {
                    case BUILD_WELL:
                        StringReplace( description, "%{count}", Castle::GetGrownWell() );
                        break;
                    case BUILD_WEL2:
                        StringReplace( description, "%{count}", Castle::GetGrownWel2() );
                        break;
                    case BUILD_CASTLE:
                    case BUILD_STATUE:
                    case BUILD_SPEC: {
                        const Funds profit = ProfitConditions::FromBuilding( building, _race );
                        StringReplace( description, "%{count}", profit.gold );
                        break;
                    }
                    default:
                        break;
                    }
                }

                fheroes2::showStandardTextMessage( Castle::GetStringBuilding( building, _race ), std::move( description ), Dialog::ZERO );
            }

            return false;
        }

        std::string getSetStatusMessage() const
        {
            return fheroes2::getBuildingName( _race, _getBuildindTypeForRender() );
        }

        void redraw( const bool isNotDefault ) const
        {
            fheroes2::Display & display = fheroes2::Display::instance();
            const int index = fheroes2::getIndexBuildingSprite( _getBuildindTypeForRender() );

            const int buildingIcnId = ICN::Get4Building( _race );
            const fheroes2::Sprite & buildingImage = ( buildingIcnId == ICN::UNKNOWN )
                                                         ? fheroes2::AGG::GetICN( Settings::Get().isEvilInterfaceEnabled() ? ICN::CASLXTRA_EVIL : ICN::CASLXTRA, 0 )
                                                         : fheroes2::AGG::GetICN( buildingIcnId, index );

            const fheroes2::Rect buildingImageRoi( _area.x + 1, _area.y + 1, 135, 57 );

            if ( isNotDefault ) {
                fheroes2::Copy( buildingImage, 0, 0, display, buildingImageRoi );
            }
            else {
                // Apply the blur effect originally used for the Holy Shout spell.
                fheroes2::Copy( fheroes2::CreateHolyShoutEffect( buildingImage, 1, 0 ), 0, 0, display, buildingImageRoi );
                fheroes2::ApplyPalette( display, buildingImageRoi.x, buildingImageRoi.y, display, buildingImageRoi.x, buildingImageRoi.y, buildingImageRoi.width,
                                        buildingImageRoi.height, PAL::GetPalette( PAL::PaletteType::DARKENING ) );
            }

            // Build and restrict status indicator.
            const int32_t maxUpgrades = static_cast<int32_t>( _buildingVariants );
            for ( int32_t i = 0; i < maxUpgrades; ++i ) {
                if ( _restrictedId != -1 && i >= _restrictedId ) {
                    // Render the restricted sign: gray cross.
                    fheroes2::Sprite denySign( fheroes2::AGG::GetICN( ICN::TOWNWIND, 12 ) );
                    fheroes2::ApplyPalette( denySign, PAL::CombinePalettes( PAL::GetPalette( PAL::PaletteType::GRAY ), PAL::GetPalette( PAL::PaletteType::DARKENING ) ) );

                    fheroes2::Blit( denySign, display, _area.x + _area.width - ( maxUpgrades - i ) * 20 - denySign.width() / 2, _area.y + 48 - denySign.height() / 2 );
                    continue;
                }

                const int icnId = ( i > _builtId ) ? ICN::CELLWIN : ICN::TOWNWIND;
                const uint32_t icnIndex = ( i > _builtId ) ? 5 : 11;
                const fheroes2::Sprite & mark = fheroes2::AGG::GetICN( icnId, icnIndex );
                fheroes2::Blit( mark, display, _area.x + _area.width - ( maxUpgrades - i ) * 20 - mark.width() / 2, _area.y + 48 - mark.height() / 2 );
            }

            if ( _builtId > -1 ) {
                const fheroes2::Sprite & textBackground = fheroes2::AGG::GetICN( ICN::BLDGXTRA, 0 );
                fheroes2::Copy( textBackground, 0, 58, display, _area.x, _area.y + 58, _area.width, textBackground.height() );
            }
            else if ( _restrictedId != 0 ) {
                const fheroes2::Sprite & textBackground = fheroes2::AGG::GetICN( ICN::CASLXTRA, 1 );
                fheroes2::Copy( textBackground, 0, 0, display, _area.x, _area.y + 58, _area.width, textBackground.height() );
            }

            if ( _restrictedId > -1 ) {
                fheroes2::Sprite textBackground( fheroes2::AGG::GetICN( ICN::CASLXTRA, 2 ) );
                fheroes2::ApplyPalette( textBackground, 6, 1, textBackground, 6, 1, 125, 12,
                                        PAL::CombinePalettes( PAL::GetPalette( PAL::PaletteType::GRAY ), PAL::GetPalette( PAL::PaletteType::DARKENING ) ) );

                if ( _restrictedId == 0 ) {
                    fheroes2::Copy( textBackground, 0, 0, display, _area.x, _area.y + 58, _area.width - 5, textBackground.height() );
                }
                else {
                    fheroes2::CreateDitheringTransition( textBackground, _buildArea.width - 5, 0, display, _banArea.x - 5, _area.y + 58, 10, textBackground.height(),
                                                         true, false );
                    fheroes2::Copy( textBackground, _buildArea.width + 5, 0, display, _banArea.x + 5, _area.y + 58, _banArea.width - 5, textBackground.height() );
                }
            }

            if ( !isNotDefault ) {
                // Make the header darker for the default buildings.
                fheroes2::ApplyPalette( display, _area.x + 6, _area.y + 59, display, _area.x + 6, _area.y + 59, 125, 12, PAL::GetPalette( PAL::PaletteType::DARKENING ) );
            }

            const fheroes2::Text buildingName( Castle::GetStringBuilding( _getBuildindTypeForRender(), _race ), fheroes2::FontType::smallWhite() );
            buildingName.draw( _area.x + 68 - buildingName.width() / 2, _area.y + 61, display );
        }

    private:
        building_t _getBuildindTypeForRender() const
        {
            if ( _builtId < 1 ) {
                return _mainBuildingType;
            }

            return _getBuildingType( _builtId );
        }

        building_t _getBuildingType( const int8_t level ) const
        {
            if ( level < 0 ) {
                return BUILD_NOTHING;
            }

            building_t building = _mainBuildingType;
            for ( int8_t i = 0; i < level; ++i ) {
                building = fheroes2::getUpgradeForBuilding( _race, building );
            }

            return building;
        }

        static std::string _getRandomMonstersName( const building_t building )
        {
            switch ( building ) {
            case DWELLING_MONSTER1:
                return _( "randomTownDetails|level 1 creatures" );
            case DWELLING_MONSTER2:
            case DWELLING_UPGRADE2:
                return _( "randomTownDetails|level 2 creatures" );
            case DWELLING_MONSTER3:
            case DWELLING_UPGRADE3:
                return _( "randomTownDetails|level 3 creatures" );
            case DWELLING_MONSTER4:
            case DWELLING_UPGRADE4:
                return _( "randomTownDetails|level 4 creatures" );
            case DWELLING_MONSTER5:
            case DWELLING_UPGRADE5:
                return _( "randomTownDetails|level 5 creatures" );
            case DWELLING_MONSTER6:
            case DWELLING_UPGRADE6:
            case DWELLING_UPGRADE7:
                return _( "randomTownDetails|level 6 creatures" );
            default:
                assert( 0 );
                return "unknown monsters";
            }
        }

        building_t _mainBuildingType{ BUILD_NOTHING };
        int _race{ Race::NONE };
        int8_t _builtId{ -1 };
        int8_t _restrictedId{ -1 };
        int8_t _buildingVariants{ 1 };
        fheroes2::Rect _area{ 0, 0, 137, 70 };
        fheroes2::Rect _buildArea{ 0, 0, 69, 70 };
        fheroes2::Rect _banArea{ 0, 0, 68, 70 };
    };
}

namespace Dialog
{
    void castleDetailsDialog( Maps::Map_Format::CastleMetadata & castleMetadata, const int race, const int color )
    {
        // setup cursor
        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        fheroes2::Display & display = fheroes2::Display::instance();

        const fheroes2::StandardWindow background( fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT, false );
        const fheroes2::Rect dialogRoi = background.activeArea();
        const fheroes2::Rect dialogWithShadowRoi = background.totalArea();

        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

        const fheroes2::Sprite & constructionBackground = fheroes2::AGG::GetICN( isEvilInterface ? ICN::CASLWIND_EVIL : ICN::CASLWIND, 0 );
        const int32_t backgroundHeight = constructionBackground.height();

        // Use the left part of town construction dialog.
        const int32_t rightPartOffsetX = 438;
        const int32_t rightPartSizeX = dialogRoi.width - rightPartOffsetX;
        fheroes2::Blit( constructionBackground, 0, 0, display, dialogRoi.x, dialogRoi.y, rightPartOffsetX, backgroundHeight );
        // Use the right part of standard background dialog.
        fheroes2::Copy( fheroes2::AGG::GetICN( isEvilInterface ? ICN::STONEBAK_EVIL : ICN::STONEBAK, 0 ), rightPartOffsetX, 0, display, dialogRoi.x + rightPartOffsetX,
                        dialogRoi.y, rightPartSizeX, backgroundHeight );
        // Add horizontal separators.
        fheroes2::Copy( constructionBackground, rightPartOffsetX, 251, display, dialogRoi.x + rightPartOffsetX, dialogRoi.y + 226, rightPartSizeX, 4 );
        fheroes2::Copy( constructionBackground, rightPartOffsetX, 251, display, dialogRoi.x + rightPartOffsetX, dialogRoi.y + 306, rightPartSizeX, 4 );

        // Castle name background.
        const fheroes2::Sprite & statusBarSprite = fheroes2::AGG::GetICN( ICN::CASLBAR, 0 );
        const fheroes2::Rect nameArea( dialogRoi.x + rightPartOffsetX, dialogRoi.y + 1, rightPartSizeX, statusBarSprite.height() - 2 );
        fheroes2::Copy( statusBarSprite, 17, 0, display, nameArea.x, dialogRoi.y, nameArea.width, statusBarSprite.height() );

        // Castle name text.
        auto drawCastleName = [&castleMetadata, &display, &nameArea]() {
            fheroes2::Text text( castleMetadata.customName.empty() ? _( "Random Castle Name" ) : castleMetadata.customName, fheroes2::FontType::normalWhite() );
            text.fitToOneRow( nameArea.width );
            text.drawInRoi( nameArea.x + ( nameArea.width - text.width() ) / 2, nameArea.y + 2, display, nameArea );
        };
        drawCastleName();

        auto drawCheckboxBackground
            = [&display, &dialogRoi]( fheroes2::MovableSprite & checkSprite, std::string str, const int32_t posX, const int32_t posY, const bool isEvil ) {
                  const fheroes2::Sprite & checkboxBackground = fheroes2::AGG::GetICN( ICN::CELLWIN, 1 );
                  if ( isEvil ) {
                      fheroes2::ApplyPalette( checkboxBackground, 0, 0, display, posX, posY, checkboxBackground.width(), checkboxBackground.height(),
                                              PAL::CombinePalettes( PAL::GetPalette( PAL::PaletteType::GRAY ), PAL::GetPalette( PAL::PaletteType::DARKENING ) ) );
                  }
                  else {
                      fheroes2::Copy( checkboxBackground, 0, 0, display, posX, posY, checkboxBackground.width(), checkboxBackground.height() );
                  }

                  fheroes2::addGradientShadow( checkboxBackground, display, { posX, posY }, { -4, 4 } );
                  const fheroes2::Text checkboxText( std::move( str ), fheroes2::FontType::normalWhite() );
                  checkboxText.drawInRoi( posX + 23, posY + 4, display, dialogRoi );

                  checkSprite = fheroes2::AGG::GetICN( ICN::CELLWIN, 2 );
                  checkSprite.setPosition( posX + 2, posY + 2 );

                  return fheroes2::Rect( posX, posY, 23 + checkboxText.width(), checkboxBackground.height() );
              };

        const bool isCastle = castleMetadata.isCastle();

        // Allow castle building checkbox.
        fheroes2::Point dstPt( dialogRoi.x + rightPartOffsetX + 10, dialogRoi.y + 130 );
        fheroes2::MovableSprite allowCastleSign;
        const fheroes2::Rect allowCastleArea
            = isCastle ? fheroes2::Rect( 0, 0, 0, 0 ) : drawCheckboxBackground( allowCastleSign, _( "Allow Castle build" ), dstPt.x, dstPt.y, isEvilInterface );
        ( !isCastle && castleMetadata.isCastleBuildAllowed() ) ? allowCastleSign.show() : allowCastleSign.hide();

        // Default buildings checkbox indicator.
        dstPt.y += 30;
        fheroes2::MovableSprite defaultBuildingsSign;
        const fheroes2::Rect defaultBuildingsArea = drawCheckboxBackground( defaultBuildingsSign, _( "Default buildings" ), dstPt.x, dstPt.y, isEvilInterface );
        castleMetadata.customBuildings ? defaultBuildingsSign.hide() : defaultBuildingsSign.show();

        // Build restrict mode button.
        fheroes2::Button buttonRestrictBuilding( 0, 0, isEvilInterface ? ICN::BUTTON_RESTRICT_EVIL : ICN::BUTTON_RESTRICT_GOOD, 0, 1 );
        buttonRestrictBuilding.setPosition( dialogRoi.x + rightPartOffsetX + ( rightPartSizeX - buttonRestrictBuilding.area().width ) / 2, dialogRoi.y + 195 );
        const fheroes2::Rect buttonRestrictBuildingArea( buttonRestrictBuilding.area() );
        fheroes2::addGradientShadow( fheroes2::AGG::GetICN( ICN::BUTTON_RESTRICT_GOOD, 0 ), display, buttonRestrictBuildingArea.getPosition(), { -5, 5 } );
        buttonRestrictBuilding.draw();

        const bool isNeutral = ( color == Color::NONE );

        // Castle army.
        dstPt.y = dialogRoi.y + 311;
        fheroes2::MovableSprite defaultArmySign;
        fheroes2::Rect defaultArmyArea;
        if ( isNeutral ) {
            defaultArmyArea = drawCheckboxBackground( defaultArmySign, _( "Default Army" ), dstPt.x, dstPt.y, isEvilInterface );

            castleMetadata.isDefaultDefenderArmy() ? defaultArmySign.show() : defaultArmySign.hide();
        }
        else {
            defaultArmySign.hide();

            const fheroes2::Text armyText( _( "Castle Army" ), fheroes2::FontType::normalWhite() );
            armyText.drawInRoi( dialogRoi.x + rightPartOffsetX + ( rightPartSizeX - armyText.width() ) / 2, dstPt.y + 4, display, dialogRoi );
        }

        Army castleArmy;
        // Load army from metadata.
        Maps::Map_Format::loadArmyFromMetadata( castleArmy, castleMetadata.defenderMonsterType, castleMetadata.defenderMonsterCount );
        ArmyBar armyBar( &castleArmy, true, false, true, false );
        armyBar.setTableSize( { 3, 2 } );
        armyBar.setCustomItemsCountInRow( { 2, 3 } );
        armyBar.setInBetweenItemsOffset( { 3, 3 } );
        armyBar.setRenderingOffset( { dialogRoi.x + rightPartOffsetX + 33, dialogRoi.y + 332 } );
        armyBar.Redraw( display );

        const building_t metadataBuildings = static_cast<building_t>( castleMetadata.getBuiltBuildings() );
        const building_t metadataRestrictedBuildings = static_cast<building_t>( castleMetadata.getBannedBuildings() );

        assert( ( metadataBuildings & metadataRestrictedBuildings ) == 0 );

        const size_t buildingsSize = 19;
        std::array<BuildingData, buildingsSize> buildings{ BuildingData( DWELLING_MONSTER1, race, metadataBuildings, metadataRestrictedBuildings ),
                                                           BuildingData( DWELLING_MONSTER2, race, metadataBuildings, metadataRestrictedBuildings ),
                                                           BuildingData( DWELLING_MONSTER3, race, metadataBuildings, metadataRestrictedBuildings ),
                                                           BuildingData( DWELLING_MONSTER4, race, metadataBuildings, metadataRestrictedBuildings ),
                                                           BuildingData( DWELLING_MONSTER5, race, metadataBuildings, metadataRestrictedBuildings ),
                                                           BuildingData( DWELLING_MONSTER6, race, metadataBuildings, metadataRestrictedBuildings ),
                                                           BuildingData( BUILD_MAGEGUILD1, race, metadataBuildings, metadataRestrictedBuildings ),
                                                           BuildingData( race == Race::NECR ? BUILD_SHRINE : BUILD_TAVERN, race, metadataBuildings,
                                                                         metadataRestrictedBuildings ),
                                                           BuildingData( BUILD_THIEVESGUILD, race, metadataBuildings, metadataRestrictedBuildings ),
                                                           BuildingData( BUILD_SHIPYARD, race, metadataBuildings, metadataRestrictedBuildings ),
                                                           BuildingData( BUILD_STATUE, race, metadataBuildings, metadataRestrictedBuildings ),
                                                           BuildingData( BUILD_MARKETPLACE, race, metadataBuildings, metadataRestrictedBuildings ),
                                                           BuildingData( BUILD_WELL, race, metadataBuildings, metadataRestrictedBuildings ),
                                                           BuildingData( BUILD_WEL2, race, metadataBuildings, metadataRestrictedBuildings ),
                                                           BuildingData( BUILD_SPEC, race, metadataBuildings, metadataRestrictedBuildings ),
                                                           BuildingData( BUILD_LEFTTURRET, race, metadataBuildings, metadataRestrictedBuildings ),
                                                           BuildingData( BUILD_RIGHTTURRET, race, metadataBuildings, metadataRestrictedBuildings ),
                                                           BuildingData( BUILD_MOAT, race, metadataBuildings, metadataRestrictedBuildings ),
                                                           BuildingData( BUILD_CAPTAIN, race, metadataBuildings, metadataRestrictedBuildings ) };

        dstPt.x = dialogRoi.x + 5;
        dstPt.y = dialogRoi.y + 2;

        for ( size_t i = 0; i < buildingsSize - 1; ++i ) {
            buildings[i].setPosition( dstPt.x + ( static_cast<int32_t>( i ) % 3 ) * 144, dstPt.y );
            buildings[i].redraw( defaultBuildingsSign.isHidden() );
            if ( i % 3 == 2 ) {
                dstPt.y += ( i == 5 || i == 14 ) ? 80 : 75;
            }
        }

        // Captain building.
        dstPt.x = dialogRoi.x + rightPartOffsetX + 33;
        dstPt.y = dialogRoi.y + 232;
        const fheroes2::Sprite & buildingFrame = fheroes2::AGG::GetICN( ICN::BLDGXTRA, 0 );
        fheroes2::Blit( buildingFrame, display, dstPt.x, dstPt.y );
        buildings.back().setPosition( dstPt.x, dstPt.y );
        buildings.back().redraw( defaultBuildingsSign.isHidden() );

        // Exit button.
        fheroes2::Button buttonExit( dialogRoi.x + rightPartOffsetX + 50, dialogRoi.y + 430, isEvilInterface ? ICN::BUTTON_SMALL_EXIT_EVIL : ICN::BUTTON_SMALL_EXIT_GOOD,
                                     0, 1 );
        const fheroes2::Rect buttonExitArea( buttonExit.area() );
        fheroes2::addGradientShadow( fheroes2::AGG::GetICN( ICN::BUTTON_SMALL_EXIT_GOOD, 0 ), display, buttonExitArea.getPosition(), { -2, 2 } );
        buttonExit.draw();

        // Status bar.
        const int32_t statusBarWidth = statusBarSprite.width();
        const int32_t statusBarheight = statusBarSprite.height();
        dstPt.y = dialogRoi.y + dialogRoi.height - statusBarheight;
        fheroes2::Copy( statusBarSprite, 0, 0, display, dialogRoi.x, dstPt.y, statusBarWidth, statusBarheight );
        StatusBar statusBar;
        statusBar.setRoi( { dialogRoi.x, dstPt.y + 3, statusBarWidth, 0 } );

        display.render( dialogWithShadowRoi );

        LocalEvent & le = LocalEvent::Get();

        std::string message;
        bool buildingRestriction = false;

        while ( le.HandleEvents() ) {
            le.MousePressLeft( buttonExitArea ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();

            if ( le.MouseClickLeft( buttonExitArea ) || Game::HotKeyCloseWindow() ) {
                break;
            }

            if ( le.MouseClickLeft( buttonRestrictBuildingArea ) ) {
                buildingRestriction = !buildingRestriction;
            }

            ( buildingRestriction || le.MousePressLeft( buttonRestrictBuildingArea ) ) ? buttonRestrictBuilding.drawOnPress() : buttonRestrictBuilding.drawOnRelease();

            if ( le.MouseCursor( nameArea ) ) {
                message = _( "Click to change the Castle name. Right-click to reset to default." );

                bool redrawName = false;
                if ( le.MouseClickLeft() ) {
                    std::string res = castleMetadata.customName;
                    if ( Dialog::InputString( _( "Enter Castle name" ), res, {}, 30 ) && !res.empty() ) {
                        castleMetadata.customName = std::move( res );
                        redrawName = true;
                    }
                }
                else if ( le.MouseClickRight() ) {
                    castleMetadata.customName.clear();
                    redrawName = true;
                }

                if ( redrawName ) {
                    // Restore name background
                    fheroes2::Copy( statusBarSprite, 17, 1, display, nameArea );
                    drawCastleName();
                    display.render( nameArea );
                }
            }
            else if ( !isCastle && le.MouseCursor( allowCastleArea ) ) {
                message = _( "Allow to build a castle in this town." );

                if ( le.MouseClickLeft() ) {
                    allowCastleSign.isHidden() ? allowCastleSign.show() : allowCastleSign.hide();
                    display.render( allowCastleSign.getArea() );
                }
                else if ( le.MousePressRight() ) {
                    fheroes2::showStandardTextMessage( _( "Allow Castle build" ), message, Dialog::ZERO );
                }
            }
            else if ( le.MouseCursor( defaultBuildingsArea ) ) {
                message = _( "Toggle the use of default buildings. Custom buildings will be reset!" );

                if ( le.MouseClickLeft() ) {
                    if ( defaultBuildingsSign.isHidden() ) {
                        // Reset all buildings to their build and restrict default states.
                        for ( BuildingData & building : buildings ) {
                            building.resetBuilding();
                            building.redraw( false );
                        }

                        defaultBuildingsSign.show();
                        display.render( dialogRoi );
                    }
                    else {
                        defaultBuildingsSign.hide();

                        for ( const BuildingData & building : buildings ) {
                            building.redraw( true );
                        }

                        display.render( dialogRoi );
                    }
                }
                else if ( le.MousePressRight() ) {
                    fheroes2::showStandardTextMessage( _( "Default Buildings" ), message, Dialog::ZERO );
                }
            }
            else if ( le.MouseCursor( buttonRestrictBuildingArea ) ) {
                message = _( "Toggle building construction restriction mode." );

                if ( le.MousePressRight() ) {
                    fheroes2::showStandardTextMessage( _( "Restrict Building Construction" ), message, Dialog::ZERO );
                }
            }
            else if ( isNeutral && le.MouseCursor( defaultArmyArea ) ) {
                message = _( "Use default defenders army." );

                if ( le.MouseClickLeft() ) {
                    if ( defaultArmySign.isHidden() ) {
                        defaultArmySign.show();
                        castleArmy.Reset( false );
                        armyBar.Redraw( display );
                        display.render( dialogRoi );
                    }
                    else {
                        defaultArmySign.hide();
                        display.render( defaultArmySign.getArea() );
                    }
                }
                else if ( le.MousePressRight() ) {
                    fheroes2::showStandardTextMessage( _( "Default Army" ), message, Dialog::ZERO );
                }
            }
            else if ( le.MouseCursor( armyBar.GetArea() ) ) {
                if ( armyBar.QueueEventProcessing( &message ) ) {
                    armyBar.Redraw( display );

                    if ( defaultArmySign.isHidden() ) {
                        display.render( armyBar.GetArea() );
                    }
                    else {
                        defaultArmySign.hide();
                        display.render( dialogRoi );
                    }
                }

                if ( message.empty() ) {
                    message = _( "Set custom Castle Army. Right-click to reset unit." );
                }
            }
            else if ( le.MouseCursor( buttonExitArea ) ) {
                message = _( "Exit Castle Options" );

                if ( le.MousePressRight() ) {
                    fheroes2::showStandardTextMessage( _( "Exit" ), message, Dialog::ZERO );
                }
            }
            else {
                for ( size_t i = 0; i < buildingsSize; ++i ) {
                    if ( le.MouseCursor( buildings[i].getArea() ) ) {
                        message = buildings[i].getSetStatusMessage();

                        if ( buildings[i].queueEventProcessing( buildingRestriction ) ) {
                            if ( defaultBuildingsSign.isHidden() ) {
                                buildings[i].redraw( true );
                                display.render( buildings[i].getArea() );
                            }
                            else {
                                // The building properties have been changed. Uncheck the default buildings checkbox.
                                defaultBuildingsSign.hide();

                                for ( const BuildingData & building : buildings ) {
                                    building.redraw( true );
                                }

                                display.render( dialogRoi );
                            }
                        }

                        break;
                    }
                }
            }

            if ( message.empty() ) {
                statusBar.ShowMessage( _( "Castle Options" ) );
            }
            else {
                statusBar.ShowMessage( std::move( message ) );
                message.clear();
            }
        }

        // Update army in metadata.
        if ( isNeutral && !defaultArmySign.isHidden() ) {
            castleMetadata.setDefaultDefenderArmy();
        }
        else {
            Maps::Map_Format::saveArmyToMetadata( castleArmy, castleMetadata.defenderMonsterType, castleMetadata.defenderMonsterCount );
        }

        // Update buildings data.
        castleMetadata.builtBuildings.clear();
        castleMetadata.bannedBuildings.clear();

        castleMetadata.addCastleTentBuilding( isCastle );

        if ( !isCastle && allowCastleSign.isHidden() ) {
            castleMetadata.bannedBuildings.push_back( BUILD_CASTLE );
        }

        castleMetadata.customBuildings = defaultBuildingsSign.isHidden();

        if ( castleMetadata.customBuildings ) {
            for ( const BuildingData & building : buildings ) {
                if ( std::vector<building_t> buildingLevels = building.getBuildLevel(); !buildingLevels.empty() ) {
                    std::move( buildingLevels.begin(), buildingLevels.end(), std::back_inserter( castleMetadata.builtBuildings ) );
                }
                if ( const building_t buildId = building.getRestrictLevel(); buildId != BUILD_NOTHING ) {
                    castleMetadata.bannedBuildings.push_back( static_cast<uint32_t>( buildId ) );
                }
            }
        }
    }
}
