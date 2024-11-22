/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2023 - 2024                                             *
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

#include "editor_interface_panel.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "agg_image.h"
#include "cursor.h"
#include "dialog.h"
#include "dialog_selectitems.h"
#include "editor_interface.h"
#include "editor_options.h"
#include "ground.h"
#include "icn.h"
#include "image.h"
#include "interface_base.h"
#include "localevent.h"
#include "maps_tiles.h"
#include "monster.h"
#include "pal.h"
#include "screen.h"
#include "settings.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_constants.h"
#include "ui_dialog.h"
#include "ui_map_object.h"
#include "ui_text.h"
#include "ui_window.h"
#include "world.h"

namespace
{
    fheroes2::Sprite getObjectImage( const Maps::ObjectGroup group, const int32_t type )
    {
        if ( type == -1 ) {
            return {};
        }
        const auto & objectInfo = Maps::getObjectsByGroup( group );
        if ( type < 0 || type >= static_cast<int32_t>( objectInfo.size() ) ) {
            // You are trying to render some unknown stuff!
            assert( 0 );
            return {};
        }

        return fheroes2::generateMapObjectImage( objectInfo[type] );
    }

    void setCustomCursor( const Maps::ObjectGroup group, const int32_t type )
    {
        if ( type == -1 ) {
            // The object type is not set. We show the POINTER cursor for this case.
            Cursor::Get().SetThemes( Cursor::POINTER );
            return;
        }

        const fheroes2::Sprite & image = getObjectImage( group, type );

        Cursor::Get().setCustomImage( image, { image.x(), image.y() } );
    }

    fheroes2::Rect getObjectOccupiedArea( const Maps::ObjectGroup group, const int32_t objectType )
    {
        if ( group == Maps::ObjectGroup::KINGDOM_TOWNS ) {
            // TODO: make occupied area calculation for complex objects.
            return { -2, -3, 5, 5 };
        }

        const auto & objectInfo = Maps::getObjectsByGroup( group );
        if ( objectType < 0 || objectType >= static_cast<int32_t>( objectInfo.size() ) ) {
            assert( 0 );
            return { 0, 0, 1, 1 };
        }

        const auto & offsets = Maps::getGroundLevelOccupiedTileOffset( objectInfo[objectType] );
        if ( offsets.size() < 2 ) {
            return { 0, 0, 1, 1 };
        }

        fheroes2::Point minPos{ offsets.front() };
        fheroes2::Point maxPos{ offsets.front() };

        for ( const auto & offset : offsets ) {
            minPos.x = std::min( minPos.x, offset.x );
            minPos.y = std::min( minPos.y, offset.y );
            maxPos.x = std::max( maxPos.x, offset.x );
            maxPos.y = std::max( maxPos.y, offset.y );
        }

        return { minPos.x, minPos.y, maxPos.x - minPos.x + 1, maxPos.y - minPos.y + 1 };
    }

    fheroes2::Image makeInstrumentPanelBackground( const int32_t width, const int32_t height )
    {
        fheroes2::Image background;
        background._disableTransformLayer();
        background.resize( width, height );
        fheroes2::StandardWindow::renderBackgroundImage( background, { 0, 0, width, height }, 0, Settings::Get().isEvilInterfaceEnabled() );

        // Make background borders: it consists of rectangles with different transform shading.
        auto applyRectTransform = [width, height]( fheroes2::Image & output, const int32_t offset, const uint8_t transformId ) {
            const int32_t doubleOffset = 2 * offset;
            // Top horizontal line.
            ApplyTransform( output, offset, offset, width - doubleOffset, 1, transformId );
            // Left vertical line without pixels that are parts of horizontal lines.
            ApplyTransform( output, offset, offset + 1, 1, height - doubleOffset - 2, transformId );
            // Bottom horizontal line.
            ApplyTransform( output, offset, height - 1 - offset, width - doubleOffset, 1, transformId );
            // Right vertical line without pixels that are parts of horizontal lines.
            ApplyTransform( output, width - 1 - offset, offset + 1, 1, height - doubleOffset - 2, transformId );
        };

        applyRectTransform( background, 0, 2 );
        applyRectTransform( background, 1, 4 );
        applyRectTransform( background, 2, 5 );
        applyRectTransform( background, 2, 9 );
        applyRectTransform( background, 3, 8 );
        applyRectTransform( background, 4, 9 );

        return background;
    }

    void drawInstrumentName( fheroes2::Image & output, const fheroes2::Point & pos, std::string text )
    {
        const fheroes2::Sprite & originalPanel = fheroes2::AGG::GetICN( ICN::EDITPANL, 0 );

        const int32_t nameBackgroundOffsetX{ 7 };
        const int32_t nameBackgroundOffsetY{ 104 };
        const int32_t nameBackgroundWidth{ 130 };
        const int32_t nameBackgroundHeight{ 14 };

        if ( Settings::Get().isEvilInterfaceEnabled() ) {
            fheroes2::ApplyPalette( originalPanel, nameBackgroundOffsetX, nameBackgroundOffsetY, output, pos.x, pos.y, nameBackgroundWidth, nameBackgroundHeight,
                                    PAL::GetPalette( PAL::PaletteType::GOOD_TO_EVIL_INTERFACE ) );
        }
        else {
            fheroes2::Copy( originalPanel, nameBackgroundOffsetX, nameBackgroundOffsetY, output, pos.x, pos.y, nameBackgroundWidth, nameBackgroundHeight );
        }
        const fheroes2::Text terrainText( std::move( text ), fheroes2::FontType::smallWhite() );
        terrainText.draw( pos.x + ( nameBackgroundWidth - terrainText.width() ) / 2, pos.y + 3, output );
    }

    void drawObjectTypeSelectionRect( fheroes2::Image & output, const fheroes2::Point & pos )
    {
        const fheroes2::Sprite & selection = fheroes2::AGG::GetICN( ICN::TERRAINS, 9 );
        fheroes2::Blit( selection, 0, 0, output, pos.x - 2, pos.y - 2, selection.width(), selection.height() );
    }

    void showObjectTypeInfoText( std::string objectName )
    {
        std::string text = _( "Used to place %{object}." );
        StringReplaceWithLowercase( text, "%{object}", objectName );
        fheroes2::showStandardTextMessage( std::move( objectName ), std::move( text ), Dialog::ZERO );
    }

    template <size_t TSize>
    void updateObjectTypeSelection( const int8_t objectId, const std::array<fheroes2::Rect, TSize> & buttonAreas,
                                    const std::function<const char *( uint8_t )> & getObjectTypeName, const fheroes2::Point & ObjectTypeNamePosition,
                                    fheroes2::Image & output )
    {
        if ( objectId < 0 ) {
            drawInstrumentName( output, ObjectTypeNamePosition, _( "Select object type" ) );
        }
        else {
            drawObjectTypeSelectionRect( output, buttonAreas[objectId].getPosition() );
            drawInstrumentName( output, ObjectTypeNamePosition, getObjectTypeName( objectId ) );
        }
    }
}

namespace Interface
{
    const std::array<Maps::ObjectGroup, Interface::EditorPanel::LandscapeObjectBrush::LANDSCAPE_COUNT>
        EditorPanel::_selectedLandscapeObjectGroup{ Maps::ObjectGroup::LANDSCAPE_MOUNTAINS, Maps::ObjectGroup::LANDSCAPE_ROCKS, Maps::ObjectGroup::LANDSCAPE_TREES,
                                                    Maps::ObjectGroup::LANDSCAPE_WATER, Maps::ObjectGroup::LANDSCAPE_MISCELLANEOUS };
    const std::array<Maps::ObjectGroup, Interface::EditorPanel::AdventureObjectBrush::ADVENTURE_COUNT>
        EditorPanel::_selectedAdventureObjectGroup{ Maps::ObjectGroup::ADVENTURE_ARTIFACTS,    Maps::ObjectGroup::ADVENTURE_DWELLINGS, Maps::ObjectGroup::ADVENTURE_MINES,
                                                    Maps::ObjectGroup::ADVENTURE_POWER_UPS,    Maps::ObjectGroup::ADVENTURE_TREASURES, Maps::ObjectGroup::ADVENTURE_WATER,
                                                    Maps::ObjectGroup::ADVENTURE_MISCELLANEOUS };
    const std::array<Maps::ObjectGroup, Interface::EditorPanel::KingdomObjectBrush::KINGDOM_OBJECTS_COUNT>
        EditorPanel::_selectedKingdomObjectGroup{ Maps::ObjectGroup::KINGDOM_HEROES, Maps::ObjectGroup::KINGDOM_TOWNS };

    EditorPanel::EditorPanel( EditorInterface & interface_ )
        : _interface( interface_ )
    {
        _instrumentButtons[_selectedInstrument].press();
        _brushSizeButtons[_selectedBrushSize].press();

        _selectedLandscapeObjectType.fill( -1 );
        _selectedAdventureObjectType.fill( -1 );
        _selectedKingdomObjectType.fill( -1 );
    }

    fheroes2::Rect EditorPanel::getBrushArea() const
    {
        // Roads and streams are placed using only 1x1 brush.
        if ( _selectedInstrument == Instrument::STREAM || _selectedInstrument == Instrument::ROAD || _selectedInstrument == Instrument::DETAIL
             || _selectedInstrument == Instrument::MONSTERS ) {
            return { 0, 0, 1, 1 };
        }

        if ( _selectedInstrument == Instrument::LANDSCAPE_OBJECTS || _selectedInstrument == Instrument::ADVENTURE_OBJECTS
             || _selectedInstrument == Instrument::KINGDOM_OBJECTS ) {
            const int32_t objectType = getSelectedObjectType();
            if ( objectType >= 0 ) {
                const Maps::ObjectGroup objectGroup = getSelectedObjectGroup();
                if ( objectGroup == Maps::ObjectGroup::ADVENTURE_MINES ) {
                    // For mine we need to decode the objectType.
                    int32_t type = -1;
                    int32_t color = -1;
                    getMineObjectProperties( type, color );

                    return getObjectOccupiedArea( objectGroup, type );
                }

                return getObjectOccupiedArea( objectGroup, objectType );
            }

            return {};
        }

        switch ( _selectedBrushSize ) {
        case BrushSize::SMALL:
            return { 0, 0, 1, 1 };
        case BrushSize::MEDIUM:
            return { 0, 0, 2, 2 };
        case BrushSize::LARGE:
            return { -1, -1, 4, 4 };
        case BrushSize::AREA:
            return {};
        default:
            // Have you added a new Brush size? Update the logic above!
            assert( 0 );
            break;
        }

        return {};
    }

    std::set<Maps::ObjectGroup> EditorPanel::getEraseObjectGroups() const
    {
        if ( _eraseTypes == ObjectErasureType::ERASE_NONE ) {
            return {};
        }

        std::set<Maps::ObjectGroup> objectGroups;

        if ( _eraseTypes & ObjectErasureType::ERASE_LANDSCAPE ) {
            objectGroups.insert( { Maps::ObjectGroup::LANDSCAPE_MOUNTAINS, Maps::ObjectGroup::LANDSCAPE_ROCKS, Maps::ObjectGroup::LANDSCAPE_TREES,
                                   Maps::ObjectGroup::LANDSCAPE_WATER, Maps::ObjectGroup::LANDSCAPE_MISCELLANEOUS } );
        }

        if ( _eraseTypes & ObjectErasureType::ERASE_ADVENTURE_NON_PICKABLE ) {
            objectGroups.insert( { Maps::ObjectGroup::ADVENTURE_DWELLINGS, Maps::ObjectGroup::ADVENTURE_MINES, Maps::ObjectGroup::ADVENTURE_POWER_UPS,
                                   Maps::ObjectGroup::ADVENTURE_WATER, Maps::ObjectGroup::ADVENTURE_MISCELLANEOUS } );
        }

        if ( _eraseTypes & ObjectErasureType::ERASE_TOWNS ) {
            objectGroups.insert( Maps::ObjectGroup::KINGDOM_TOWNS );
        }

        if ( _eraseTypes & ObjectErasureType::ERASE_ADVENTURE_PICKABLE ) {
            objectGroups.insert( { Maps::ObjectGroup::ADVENTURE_ARTIFACTS, Maps::ObjectGroup::ADVENTURE_TREASURES, Maps::ObjectGroup::ADVENTURE_WATER } );
        }

        if ( _eraseTypes & ObjectErasureType::ERASE_MONSTERS ) {
            objectGroups.insert( Maps::ObjectGroup::MONSTERS );
        }

        if ( _eraseTypes & ObjectErasureType::ERASE_HEROES ) {
            objectGroups.insert( Maps::ObjectGroup::KINGDOM_HEROES );
        }

        if ( _eraseTypes & ObjectErasureType::ERASE_STREAMS ) {
            objectGroups.insert( Maps::ObjectGroup::STREAMS );
        }

        if ( _eraseTypes & ObjectErasureType::ERASE_ROADS ) {
            objectGroups.insert( Maps::ObjectGroup::ROADS );
        }

        return objectGroups;
    }

    void EditorPanel::setPos( const int32_t displayX, int32_t displayY )
    {
        int32_t offsetX = displayX;

        // Editor panel consists of 3 instrument button rows, instrument panel and 2 system button rows.
        // Each button row is 36 pixels height.
        const fheroes2::Display & display = fheroes2::Display::instance();
        const int32_t instrumentPanelWidth = display.width() - displayX - fheroes2::borderWidthPx;
        const int32_t bottomBorderOffset = ( display.height() > fheroes2::Display::DEFAULT_HEIGHT + fheroes2::borderWidthPx ) ? fheroes2::borderWidthPx : 0;
        const int32_t instrumentPanelHeight = display.height() - displayY - fheroes2::AGG::GetICN( ICN::EDITBTNS, 0 ).height() * 5 - bottomBorderOffset;

        _instrumentPanelBackground = makeInstrumentPanelBackground( instrumentPanelWidth, instrumentPanelHeight );

        uint32_t icnIndex = 0;

        const int icnId = Settings::Get().isEvilInterfaceEnabled() ? ICN::EDITBTNS_EVIL : ICN::EDITBTNS;

        // Editor Instruments go in this order in ICN: TERRAIN, LANDSCAPE_OBJECTS, DETAIL, ADVENTURE_OBJECTS, KINGDOM_OBJECTS, MONSTERS, STREAM, ROAD, ERASE.
        for ( size_t i = 0; i < Instrument::INSTRUMENTS_COUNT; ++i ) {
            if ( i == Instrument::ADVENTURE_OBJECTS ) {
                // Second row buttons ICN index starts from 53.
                icnIndex = 35;
            }
            else if ( i == Instrument::STREAM ) {
                // Third row buttons ICN index starts from 53.
                icnIndex = 6;
            }
            _instrumentButtons[i].setICNInfo( icnId, icnIndex, icnIndex + 1 );
            icnIndex += 2;
        }

        _buttonMagnify.setICNInfo( icnId, 12, 13 );
        _buttonUndo.setICNInfo( icnId, 14, 15 );
        _buttonNew.setICNInfo( icnId, 16, 17 );
        _buttonSpecs.setICNInfo( icnId, 18, 19 );
        _buttonFile.setICNInfo( icnId, 20, 21 );
        _buttonSystem.setICNInfo( icnId, 22, 23 );

        // Brush Size buttons go in this order in ICN: SMALL (1x), MEDIUM (2x), LARGE (4x), AREA.
        icnIndex = 24;
        for ( fheroes2::Button & button : _brushSizeButtons ) {
            button.setICNInfo( icnId, icnIndex, icnIndex + 1 );
            icnIndex += 2;
        }

        for ( size_t i = 0; i < _instrumentButtonsRect.size(); ++i ) {
            _instrumentButtons[i].setPosition( offsetX, displayY );
            _instrumentButtonsRect[i] = _instrumentButtons[i].area();

            // We have 3 buttons in a row.
            if ( ( i + 1 ) % 3 == 0 ) {
                offsetX = displayX;
                displayY += _instrumentButtonsRect[i].height;
            }
            else {
                offsetX += _instrumentButtonsRect[i].width;
            }
        }

        _rectInstruments
            = { _instrumentButtonsRect.front().x, _instrumentButtonsRect.front().y,
                _instrumentButtonsRect.back().x + _instrumentButtonsRect.back().width - _instrumentButtonsRect.front().x, displayY - _instrumentButtonsRect.front().y };

        // Instrument panel.
        _rectInstrumentPanel = { displayX, displayY, instrumentPanelWidth, instrumentPanelHeight };

        // Brush size buttons position. Shown on the terrain and erasure instrument panels.
        offsetX = displayX + 14;
        int32_t offsetY = displayY + std::min( instrumentPanelHeight - 27, 135 );
        for ( size_t i = 0; i < _brushSizeButtonsRect.size(); ++i ) {
            _brushSizeButtons[i].setPosition( offsetX, offsetY );
            _brushSizeButtonsRect[i] = _brushSizeButtons[i].area();
            offsetX += 30;
        }

        const int32_t buttonStepX{ 36 };
        const int32_t buttonStepY{ 33 };
        const int32_t buttonWidth{ 27 };
        const int32_t buttonHeight{ 27 };
        const int32_t buttonHalfStepX = buttonStepX / 2;

        // Terrain type select buttons position. Shown on the terrain instrument panel.
        offsetX = displayX + 23;
        offsetY = displayY + 11;
        for ( size_t i = 0; i < _terrainButtonsRect.size(); ++i ) {
            _terrainButtonsRect[i]
                = { offsetX + static_cast<int32_t>( i % 3 ) * buttonStepX, offsetY + static_cast<int32_t>( i / 3 ) * buttonStepY, buttonWidth, buttonHeight };
        }

        // Landscape objects buttons position.
        for ( size_t i = 0; i < _landscapeObjectButtonsRect.size(); ++i ) {
            _landscapeObjectButtonsRect[i] = { offsetX + static_cast<int32_t>( i % 3 ) * buttonStepX + ( i > 2 ? buttonHalfStepX : 0 ),
                                               offsetY + static_cast<int32_t>( i / 3 ) * buttonStepY, buttonWidth, buttonHeight };
        }

        // Adventure objects buttons position.
        for ( size_t i = 0; i < _adventureObjectButtonsRect.size(); ++i ) {
            _adventureObjectButtonsRect[i] = { offsetX + static_cast<int32_t>( i % 2 ) * buttonStepX + ( i < 4 ? buttonHalfStepX : ( i < 6 ? 0 : buttonStepX * 2 ) ),
                                               offsetY + static_cast<int32_t>( i / 2 ) * buttonStepY - ( i < 6 ? 0 : buttonStepY ), buttonWidth, buttonHeight };
        }

        // Kingdom objects buttons position.
        for ( size_t i = 0; i < _kingdomObjectButtonsRect.size(); ++i ) {
            // We have only two buttons and have enough space to make a double stepX.
            _kingdomObjectButtonsRect[i] = { offsetX + static_cast<int32_t>( i ) * buttonStepX + buttonHalfStepX, offsetY, buttonWidth, buttonHeight };
        }

        // Erase tool object type buttons.
        offsetY += 13;
        for ( size_t i = 0; i < _eraseButtonsRect.size(); ++i ) {
            _eraseButtonsRect[i] = { offsetX + static_cast<int32_t>( i % 3 ) * buttonStepX + ( i > 5 ? buttonHalfStepX : 0 ),
                                     offsetY + static_cast<int32_t>( i / 3 ) * ( buttonStepY - ( display.height() < 500 ? 3 : 0 ) ), buttonWidth, buttonHeight };
        }

        displayY += _rectInstrumentPanel.height;

        // System buttons top row.
        _buttonMagnify.setPosition( displayX, displayY );
        _rectMagnify = _buttonMagnify.area();

        _buttonUndo.setPosition( _rectMagnify.x + _rectMagnify.width, displayY );
        _rectUndo = _buttonUndo.area();

        _buttonNew.setPosition( _rectUndo.x + _rectUndo.width, displayY );
        _rectNew = _buttonNew.area();

        // System buttons bottom row.
        displayY += _rectMagnify.height;

        _buttonSpecs.setPosition( displayX, displayY );
        _rectSpecs = _buttonSpecs.area();

        _buttonFile.setPosition( _rectSpecs.x + _rectSpecs.width, displayY );
        _rectFile = _buttonFile.area();

        _buttonSystem.setPosition( _rectFile.x + _rectFile.width, displayY );
        _rectSystem = _buttonSystem.area();

        _rectEditorPanel
            = { _rectInstruments.x, _rectInstruments.y, _rectSystem.x + _rectSystem.width - _rectInstruments.x, _rectSystem.y + _rectSystem.height - _rectInstruments.y };
    }

    void EditorPanel::setRedraw() const
    {
        _interface.setRedraw( REDRAW_PANEL );
    }

    void EditorPanel::_redraw() const
    {
        for ( const fheroes2::Button & button : _instrumentButtons ) {
            button.draw();
        }

        fheroes2::Display & display = fheroes2::Display::instance();

        fheroes2::Copy( _instrumentPanelBackground, 0, 0, display, _rectInstrumentPanel.x, _rectInstrumentPanel.y, _rectInstrumentPanel.width,
                        _rectInstrumentPanel.height );

        if ( _selectedInstrument == Instrument::TERRAIN || _selectedInstrument == Instrument::ERASE ) {
            for ( const fheroes2::Button & button : _brushSizeButtons ) {
                button.draw();
            }
        }

        if ( _selectedInstrument == Instrument::TERRAIN ) {
            // We use terrain images from the original terrain instrument panel sprite.
            const fheroes2::Sprite & originalPanel = fheroes2::AGG::GetICN( ICN::EDITPANL, 0 );
            for ( size_t i = 0; i < TerrainBrush::TERRAIN_COUNT; ++i ) {
                const int32_t originalOffsetX = 30 + static_cast<int32_t>( i % 3 ) * 29;
                const int32_t originalOffsetY = 11 + static_cast<int32_t>( i / 3 ) * 29;
                fheroes2::Copy( originalPanel, originalOffsetX, originalOffsetY, display, _terrainButtonsRect[i] );
            }

            // Terrain type selection yellow rectangle.
            drawObjectTypeSelectionRect( display, _terrainButtonsRect[_selectedTerrain].getPosition() );

            // On high resolutions we have space to show selected terrain text.
            if ( _rectInstrumentPanel.height > 160 ) {
                drawInstrumentName( display, { _rectInstrumentPanel.x + 7, _rectInstrumentPanel.y + 113 }, _getTerrainTypeName( _selectedTerrain ) );
            }
        }
        else if ( _selectedInstrument == Instrument::LANDSCAPE_OBJECTS ) {
            // Landscape objects buttons.
            for ( uint32_t i = 0; i < LandscapeObjectBrush::LANDSCAPE_COUNT; ++i ) {
                fheroes2::Copy( fheroes2::AGG::GetICN( ICN::EDITPANL, i + 6 ), 0, 0, display, _landscapeObjectButtonsRect[i] );
            }

            updateObjectTypeSelection( _selectedLandscapeObject, _landscapeObjectButtonsRect, _getLandscapeObjectTypeName,
                                       { _rectInstrumentPanel.x + 7, _rectInstrumentPanel.y + 80 }, display );
        }
        else if ( _selectedInstrument == Instrument::ADVENTURE_OBJECTS ) {
            // Adventure objects buttons.
            const fheroes2::Sprite & originalPanel = fheroes2::AGG::GetICN( ICN::EDITPANL, 1 );
            const int32_t originalArtifactsImageOffsetX{ 15 };
            const int32_t originalTreasuresImageOffsetX{ 102 };
            const int32_t originalImagesOffsetY{ 96 };

            fheroes2::Copy( originalPanel, originalArtifactsImageOffsetX, originalImagesOffsetY, display, _adventureObjectButtonsRect[AdventureObjectBrush::ARTIFACTS] );
            for ( uint32_t i = AdventureObjectBrush::DWELLINGS; i < AdventureObjectBrush::TREASURES; ++i ) {
                fheroes2::Copy( fheroes2::AGG::GetICN( ICN::EDITPANL, i + 10 ), 0, 0, display, _adventureObjectButtonsRect[i] );
            }
            fheroes2::Copy( originalPanel, originalTreasuresImageOffsetX, originalImagesOffsetY, display, _adventureObjectButtonsRect[AdventureObjectBrush::TREASURES] );
            for ( uint32_t i = AdventureObjectBrush::WATER_ADVENTURE; i < AdventureObjectBrush::ADVENTURE_COUNT; ++i ) {
                fheroes2::Copy( fheroes2::AGG::GetICN( ICN::EDITPANL, i + 9 ), 0, 0, display, _adventureObjectButtonsRect[i] );
            }

            updateObjectTypeSelection( _selectedAdventureObject, _adventureObjectButtonsRect, _getAdventureObjectTypeName,
                                       { _rectInstrumentPanel.x + 7, _rectInstrumentPanel.y + 113 }, display );
        }
        else if ( _selectedInstrument == Instrument::KINGDOM_OBJECTS ) {
            // Kingdom objects buttons.
            const fheroes2::Sprite & originalPanel = fheroes2::AGG::GetICN( ICN::EDITPANL, 1 );
            const int32_t originalHeroesImageOffsetX{ 102 };
            const int32_t originalTownsImageOffsetX{ 44 };
            const int32_t originalImagesOffsetY{ 68 };

            fheroes2::Copy( originalPanel, originalHeroesImageOffsetX, originalImagesOffsetY, display, _kingdomObjectButtonsRect[KingdomObjectBrush::HEROES] );
            fheroes2::Copy( originalPanel, originalTownsImageOffsetX, originalImagesOffsetY, display, _kingdomObjectButtonsRect[KingdomObjectBrush::TOWNS] );

            updateObjectTypeSelection( _selectedKingdomObject, _kingdomObjectButtonsRect, _getKingdomObjectTypeName,
                                       { _rectInstrumentPanel.x + 7, _rectInstrumentPanel.y + 48 }, display );
        }
        else if ( _selectedInstrument == Instrument::MONSTERS ) {
            const fheroes2::Text instrumentName( getObjectGroupName( Maps::ObjectGroup::MONSTERS ), fheroes2::FontType::normalWhite() );
            instrumentName.draw( _rectInstrumentPanel.x + ( _rectInstrumentPanel.width - instrumentName.width() ) / 2, _rectInstrumentPanel.y + 8, display );

            if ( _selectedMonsterType < 0 ) {
                // Show a tip.
                const fheroes2::Text text( _( "Click here to\nselect a monster." ), fheroes2::FontType::smallWhite() );
                text.draw( _rectInstrumentPanel.x + 5, _rectInstrumentPanel.y + 38, _rectInstrumentPanel.width - 10, display );
            }
            else {
                // Show the selected monster image on the panel.
                const fheroes2::Sprite & image = getObjectImage( Maps::ObjectGroup::MONSTERS, _selectedMonsterType );
                fheroes2::Blit( image, display, _rectInstrumentPanel.x + ( _rectInstrumentPanel.width - image.width() ) / 2,
                                _rectInstrumentPanel.y + 67 - image.height() );

                fheroes2::Text text( Monster( _selectedMonsterType + 1 ).GetName(), fheroes2::FontType::smallWhite() );
                text.draw( _rectInstrumentPanel.x + 5, _rectInstrumentPanel.y + 70, _rectInstrumentPanel.width - 10, display );
                text.set( _( "Click here to\nselect another monster." ), fheroes2::FontType::smallWhite() );
                text.draw( _rectInstrumentPanel.x + 5, _rectInstrumentPanel.y + 95, _rectInstrumentPanel.width - 10, display );
            }
        }
        else if ( _selectedInstrument == Instrument::DETAIL ) {
            const fheroes2::Text instrumentName( _( "Cell\nDetails" ), fheroes2::FontType::normalWhite() );
            instrumentName.draw( _rectInstrumentPanel.x + 5, _rectInstrumentPanel.y + 8, _rectInstrumentPanel.width - 10, display );
        }
        else if ( _selectedInstrument == Instrument::ROAD ) {
            const fheroes2::Text instrumentName( getObjectGroupName( Maps::ObjectGroup::ROADS ), fheroes2::FontType::normalWhite() );
            instrumentName.draw( _rectInstrumentPanel.x + ( _rectInstrumentPanel.width - instrumentName.width() ) / 2, _rectInstrumentPanel.y + 8, display );
        }
        else if ( _selectedInstrument == Instrument::STREAM ) {
            const fheroes2::Text instrumentName( getObjectGroupName( Maps::ObjectGroup::STREAMS ), fheroes2::FontType::normalWhite() );
            instrumentName.draw( _rectInstrumentPanel.x + ( _rectInstrumentPanel.width - instrumentName.width() ) / 2, _rectInstrumentPanel.y + 8, display );
        }
        else if ( _selectedInstrument == Instrument::ERASE ) {
            const fheroes2::Text instrumentName( _( "Erase" ), fheroes2::FontType::normalWhite() );
            instrumentName.draw( _rectInstrumentPanel.x + ( _rectInstrumentPanel.width - instrumentName.width() ) / 2, _rectInstrumentPanel.y + 8, display );

            // Object type to erase buttons.
            const fheroes2::Sprite & originalPanel = fheroes2::AGG::GetICN( ICN::EDITPANL, 1 );
            const int32_t originalTownsImageOffsetX{ 44 };
            const int32_t originalMonstersImageOffsetX{ 73 };
            const int32_t originalHeroesTreasuresImageOffsetX{ 102 };
            const int32_t originalTownsMonstersHeroesOffsetY{ 68 };
            const int32_t originalArtifactsTreasresOffsetY{ 96 };

            // Landscape objects icon.
            fheroes2::Copy( fheroes2::AGG::GetICN( ICN::EDITPANL, 8 ), 0, 0, display, _eraseButtonsRect[0] );
            // Adventure non pickable objects icon.
            fheroes2::Copy( fheroes2::AGG::GetICN( ICN::EDITPANL, 13 ), 0, 0, display, _eraseButtonsRect[1] );
            // Castle objects icon.
            fheroes2::Copy( originalPanel, originalTownsImageOffsetX, originalTownsMonstersHeroesOffsetY, display, _eraseButtonsRect[2] );
            // Adventure pickable objects icon.
            fheroes2::Copy( originalPanel, originalHeroesTreasuresImageOffsetX, originalArtifactsTreasresOffsetY, display, _eraseButtonsRect[3] );
            // Monster objects icon.
            fheroes2::Copy( originalPanel, originalMonstersImageOffsetX, originalTownsMonstersHeroesOffsetY, display, _eraseButtonsRect[4] );
            // Hero objects icon.
            fheroes2::Copy( originalPanel, originalHeroesTreasuresImageOffsetX, originalTownsMonstersHeroesOffsetY, display, _eraseButtonsRect[5] );
            // Road objects icon.
            fheroes2::Copy( fheroes2::AGG::GetICN( ICN::EDITPANL, 16 ), 0, 0, display, _eraseButtonsRect[6] );
            // Stream objects icon.
            fheroes2::Copy( fheroes2::AGG::GetICN( ICN::EDITPANL, 17 ), 0, 0, display, _eraseButtonsRect[7] );

            // Object type to erase selection marks.
            const fheroes2::Sprite & selectionMark = fheroes2::AGG::GetICN( ICN::TOWNWIND, 11 );
            for ( size_t i = 0; i < _eraseButtonsRect.size(); ++i ) {
                if ( _eraseButtonObjectTypes[i] & _eraseTypes ) {
                    fheroes2::Blit( selectionMark, 0, 0, display, _eraseButtonsRect[i].x + 10, _eraseButtonsRect[i].y + 11, selectionMark.width(),
                                    selectionMark.height() );
                }
            }
        }

        _buttonMagnify.draw();
        _buttonUndo.draw();
        _buttonNew.draw();
        _buttonSpecs.draw();
        _buttonFile.draw();
        _buttonSystem.draw();
    }

    int32_t EditorPanel::getSelectedObjectType() const
    {
        switch ( _selectedInstrument ) {
        case Instrument::MONSTERS:
            return _selectedMonsterType;
        case Instrument::LANDSCAPE_OBJECTS:
            if ( _selectedLandscapeObject < 0 ) {
                return -1;
            }
            return _selectedLandscapeObjectType[_selectedLandscapeObject];
        case Instrument::ADVENTURE_OBJECTS:
            if ( _selectedAdventureObject < 0 ) {
                return -1;
            }
            return _selectedAdventureObjectType[_selectedAdventureObject];
        case Instrument::KINGDOM_OBJECTS:
            if ( _selectedKingdomObject < 0 ) {
                return -1;
            }
            return _selectedKingdomObjectType[_selectedKingdomObject];
        default:
            // Why are you trying to get type for the non-object instrument. Check your logic!
            assert( 0 );
            return -1;
        }
    }

    Maps::ObjectGroup EditorPanel::getSelectedObjectGroup() const
    {
        switch ( _selectedInstrument ) {
        case Instrument::MONSTERS:
            return Maps::ObjectGroup::MONSTERS;
        case Instrument::LANDSCAPE_OBJECTS:
            assert( _selectedLandscapeObject > -1 );
            return _selectedLandscapeObjectGroup[_selectedLandscapeObject];
        case Instrument::ADVENTURE_OBJECTS:
            assert( _selectedAdventureObject > -1 );
            return _selectedAdventureObjectGroup[_selectedAdventureObject];
        case Instrument::KINGDOM_OBJECTS:
            assert( _selectedKingdomObject > -1 );
            return _selectedKingdomObjectGroup[_selectedKingdomObject];
        default:
            // Why are you trying to get object group for the non-object instrument. Check your logic!
            assert( 0 );
            return Maps::ObjectGroup::GROUP_COUNT;
        }
    }

    const char * EditorPanel::getObjectGroupName( const Maps::ObjectGroup groupName )
    {
        switch ( groupName ) {
        case Maps::ObjectGroup::ROADS:
            return _( "Roads" );
        case Maps::ObjectGroup::STREAMS:
            return _( "Streams" );
        case Maps::ObjectGroup::LANDSCAPE_MOUNTAINS:
            return _( "Mountains" );
        case Maps::ObjectGroup::LANDSCAPE_ROCKS:
            return _( "Rocks" );
        case Maps::ObjectGroup::LANDSCAPE_TREES:
            return _( "Trees" );
        case Maps::ObjectGroup::ADVENTURE_WATER:
        case Maps::ObjectGroup::LANDSCAPE_WATER:
            return _( "Water Objects" );
        case Maps::ObjectGroup::ADVENTURE_MISCELLANEOUS:
        case Maps::ObjectGroup::LANDSCAPE_MISCELLANEOUS:
            return _( "Miscellaneous" );
        case Maps::ObjectGroup::LANDSCAPE_TOWN_BASEMENTS:
            // You shouldn't call this path!
            assert( 0 );
            return "Towns";
        case Maps::ObjectGroup::LANDSCAPE_FLAGS:
            // You shouldn't call this path!
            assert( 0 );
            return "Flags";
        case Maps::ObjectGroup::ADVENTURE_ARTIFACTS:
            return _( "Artifacts" );
        case Maps::ObjectGroup::ADVENTURE_DWELLINGS:
            return _( "Dwellings" );
        case Maps::ObjectGroup::ADVENTURE_MINES:
            return _( "Mines" );
        case Maps::ObjectGroup::ADVENTURE_POWER_UPS:
            return _( "Power-ups" );
        case Maps::ObjectGroup::ADVENTURE_TREASURES:
            return _( "Treasures" );
        case Maps::ObjectGroup::KINGDOM_HEROES:
            return _( "Heroes" );
        case Maps::ObjectGroup::KINGDOM_TOWNS:
            return _( "Towns" );
        case Maps::ObjectGroup::MONSTERS:
            return _( "Monsters" );
        default:
            // Did you add a new group? Add the logic for it!
            assert( 0 );
            return "Unknown objects";
        }
    }

    int EditorPanel::_getGroundId( const uint8_t brushId )
    {
        switch ( brushId ) {
        case TerrainBrush::WATER:
            return Maps::Ground::WATER;
        case TerrainBrush::GRASS:
            return Maps::Ground::GRASS;
        case TerrainBrush::SNOW:
            return Maps::Ground::SNOW;
        case TerrainBrush::SWAMP:
            return Maps::Ground::SWAMP;
        case TerrainBrush::LAVA:
            return Maps::Ground::LAVA;
        case TerrainBrush::DESERT:
            return Maps::Ground::DESERT;
        case TerrainBrush::DIRT:
            return Maps::Ground::DIRT;
        case TerrainBrush::WASTELAND:
            return Maps::Ground::WASTELAND;
        case TerrainBrush::BEACH:
            return Maps::Ground::BEACH;
        default:
            // Have you added a new terrain type? Add the logic above!
            assert( 0 );
            break;
        }
        return Maps::Ground::UNKNOWN;
    }

    const char * EditorPanel::_getLandscapeObjectTypeName( const uint8_t brushId )
    {
        if ( brushId >= _selectedLandscapeObjectGroup.size() ) {
            return "Unknown object type";
        }

        return getObjectGroupName( _selectedLandscapeObjectGroup[brushId] );
    }

    const char * EditorPanel::_getAdventureObjectTypeName( const uint8_t brushId )
    {
        if ( brushId >= _selectedAdventureObjectGroup.size() ) {
            return "Unknown object type";
        }

        return getObjectGroupName( _selectedAdventureObjectGroup[brushId] );
    }

    const char * EditorPanel::_getKingdomObjectTypeName( const uint8_t brushId )
    {
        if ( brushId >= _selectedKingdomObjectGroup.size() ) {
            return "Unknown object type";
        }

        return getObjectGroupName( _selectedKingdomObjectGroup[brushId] );
    }

    const char * EditorPanel::_getEraseObjectTypeName( const uint32_t eraseObjectType )
    {
        switch ( eraseObjectType ) {
        case ObjectErasureType::ERASE_LANDSCAPE:
            return _( "editorErasure|Landscape objects" );
        case ObjectErasureType::ERASE_ADVENTURE_NON_PICKABLE:
            return _( "editorErasure|Adventure non pickable objects" );
        case ObjectErasureType::ERASE_TOWNS:
            return _( "editorErasure|Castles" );
        case ObjectErasureType::ERASE_ADVENTURE_PICKABLE:
            return _( "editorErasure|Adventure pickable objects" );
        case ObjectErasureType::ERASE_MONSTERS:
            return _( "editorErasure|Monsters" );
        case ObjectErasureType::ERASE_HEROES:
            return _( "editorErasure|Heroes" );
        case ObjectErasureType::ERASE_ROADS:
            return _( "editorErasure|Roads" );
        case ObjectErasureType::ERASE_STREAMS:
            return _( "editorErasure|Streams" );
        default:
            // Have you added a new object type to erase? Add the logic above!
            assert( 0 );
            break;
        }

        return "Unknown object type";
    }

    void EditorPanel::_setCursor()
    {
        switch ( _selectedInstrument ) {
        case Instrument::MONSTERS:
            _interface.setCursorUpdater(
                [type = getSelectedObjectType(), group = getSelectedObjectGroup()]( const int32_t /*tileIndex*/ ) { setCustomCursor( group, type ); } );
            return;
        case Instrument::LANDSCAPE_OBJECTS:
            switch ( _selectedLandscapeObject ) {
            case LandscapeObjectBrush::MOUNTAINS:
            case LandscapeObjectBrush::ROCKS:
            case LandscapeObjectBrush::TREES:
            case LandscapeObjectBrush::WATER_OBJECTS:
            case LandscapeObjectBrush::LANDSCAPE_MISC:
                _interface.setCursorUpdater(
                    [type = getSelectedObjectType(), group = getSelectedObjectGroup()]( const int32_t /*tileIndex*/ ) { setCustomCursor( group, type ); } );
                return;
            default:
                break;
            }
            break;
        case Instrument::ADVENTURE_OBJECTS:
            switch ( _selectedAdventureObject ) {
            case AdventureObjectBrush::ARTIFACTS:
            case AdventureObjectBrush::DWELLINGS:
            case AdventureObjectBrush::POWER_UPS:
            case AdventureObjectBrush::TREASURES:
            case AdventureObjectBrush::WATER_ADVENTURE:
            case AdventureObjectBrush::ADVENTURE_MISC:
                _interface.setCursorUpdater(
                    [type = getSelectedObjectType(), group = getSelectedObjectGroup()]( const int32_t /*tileIndex*/ ) { setCustomCursor( group, type ); } );
                return;
            case AdventureObjectBrush::MINES:
                _interface.setCursorUpdater( [this]( const int32_t /*tileIndex*/ ) {
                    int32_t type = -1;
                    int32_t color = -1;
                    getMineObjectProperties( type, color );

                    if ( type == -1 || color == -1 ) {
                        // The object type is not set. We show the POINTER cursor for this case.
                        Cursor::Get().SetThemes( Cursor::POINTER );
                        return;
                    }

                    assert( Maps::getObjectsByGroup( Maps::ObjectGroup::ADVENTURE_MINES ).size() > static_cast<size_t>( type ) );

                    // TODO: Implement a function to render also the owner flag after ownership selection is implemented.
                    const fheroes2::Sprite & image = getObjectImage( Maps::ObjectGroup::ADVENTURE_MINES, type );

                    Cursor::Get().setCustomImage( image, { image.x(), image.y() } );
                } );
                return;
            default:
                break;
            }
            break;
        case Instrument::KINGDOM_OBJECTS:
            switch ( _selectedKingdomObject ) {
            case KingdomObjectBrush::HEROES:
                _interface.setCursorUpdater( [type = getSelectedObjectType()]( const int32_t /*tileIndex*/ ) {
                    if ( type == -1 ) {
                        // The object type is not set. We show the POINTER cursor for this case.
                        Cursor::Get().SetThemes( Cursor::POINTER );
                        return;
                    }

                    // TODO: render ICN::MINIHERO from the existing hero images.
                    const fheroes2::Sprite & image = fheroes2::AGG::GetICN( ICN::MINIHERO, type );

                    // Mini-hero images contain a pole with a flag.
                    // This causes a situation that a selected tile does not properly correspond to current position of the cursor.
                    // We need to add a hardcoded correction.
                    const int32_t heroCorrectionY{ 12 };
                    Cursor::Get().setCustomImage( image, { -image.width() / 2, -image.height() / 2 - heroCorrectionY } );
                } );
                return;
            case KingdomObjectBrush::TOWNS:
                _interface.setCursorUpdater( [this]( const int32_t tileIndex ) {
                    int32_t type = -1;
                    int32_t color = -1;
                    getTownObjectProperties( type, color );

                    if ( type == -1 || color == -1 ) {
                        // The object type is not set. We show the POINTER cursor for this case.
                        Cursor::Get().SetThemes( Cursor::POINTER );
                        return;
                    }

                    const fheroes2::Sprite & image = fheroes2::generateTownObjectImage( type, color, world.getTile( tileIndex ).GetGround() );

                    Cursor::Get().setCustomImage( image, { image.x(), image.y() } );
                } );
                return;
            default:
                break;
            }
            break;
        default:
            break;
        }

        _interface.setCursorUpdater( {} );
    }

    fheroes2::GameMode EditorPanel::queueEventProcessing()
    {
        LocalEvent & le = LocalEvent::Get();
        fheroes2::GameMode res = fheroes2::GameMode::CANCEL;

        if ( le.isMouseLeftButtonPressedInArea( _rectInstruments ) ) {
            for ( size_t i = 0; i < _instrumentButtonsRect.size(); ++i ) {
                if ( i != _selectedInstrument && ( _instrumentButtonsRect[i] & le.getMouseLeftButtonPressedPos() ) ) {
                    _selectedInstrument = static_cast<uint8_t>( i );

                    // Reset cursor updater since this UI element was clicked.
                    _setCursor();

                    // Redraw all instrument buttons.
                    for ( size_t index = 0; index < _instrumentButtonsRect.size(); ++index ) {
                        _instrumentButtons[index].drawOnState( index == _selectedInstrument );
                    }

                    // When opening Monsters placing and no monster was previously selected force open the Select Monster dialog.
                    if ( _selectedInstrument == Instrument::MONSTERS && _selectedMonsterType == -1 ) {
                        // Update panel image and then open the Select Monster dialog.
                        _redraw();
                        handleObjectMouseClick( Dialog::selectMonsterType );
                    }

                    setRedraw();

                    break;
                }
            }

            return res;
        }

        if ( _selectedInstrument == Instrument::TERRAIN || _selectedInstrument == Instrument::ERASE ) {
            for ( size_t i = 0; i < _brushSizeButtonsRect.size(); ++i ) {
                if ( i != _selectedBrushSize && le.isMouseLeftButtonPressedInArea( _brushSizeButtonsRect[i] ) ) {
                    _selectedBrushSize = static_cast<uint8_t>( i );

                    // Redraw all brush size buttons.
                    for ( size_t index = 0; index < _brushSizeButtonsRect.size(); ++index ) {
                        _brushSizeButtons[index].drawOnState( index == _selectedBrushSize );
                    }

                    break;
                }
            }

            const auto brushSizeText = []( const int brushSize, const bool isFillBrush ) {
                std::string text
                    = isFillBrush ? _( "Draws terrain in\n%{size} by %{size} square increments." ) : _( "Erases objects in\n%{size} by %{size} square increments." );

                StringReplace( text, "%{size}", brushSize );
                return text;
            };

            if ( le.isMouseRightButtonPressedInArea( _brushSizeButtonsRect[BrushSize::SMALL] ) ) {
                fheroes2::showStandardTextMessage( _( "Small Brush" ), brushSizeText( 1, _selectedInstrument == Instrument::TERRAIN ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( _brushSizeButtonsRect[BrushSize::MEDIUM] ) ) {
                fheroes2::showStandardTextMessage( _( "Medium Brush" ), brushSizeText( 2, _selectedInstrument == Instrument::TERRAIN ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( _brushSizeButtonsRect[BrushSize::LARGE] ) ) {
                fheroes2::showStandardTextMessage( _( "Large Brush" ), brushSizeText( 4, _selectedInstrument == Instrument::TERRAIN ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( _brushSizeButtonsRect[BrushSize::AREA] ) ) {
                if ( _selectedInstrument == Instrument::TERRAIN ) {
                    fheroes2::showStandardTextMessage( _( "Area Fill" ), _( "Used to click and drag for filling in large areas." ), Dialog::ZERO );
                }
                else {
                    fheroes2::showStandardTextMessage( _( "Clear Area" ), _( "Used to click and drag for clearing large areas." ), Dialog::ZERO );
                }
            }
        }

        if ( _selectedInstrument == Instrument::TERRAIN ) {
            for ( size_t i = 0; i < _terrainButtonsRect.size(); ++i ) {
                if ( ( _selectedTerrain != i ) && le.isMouseLeftButtonPressedInArea( _terrainButtonsRect[i] ) ) {
                    _selectedTerrain = static_cast<uint8_t>( i );
                    setRedraw();

                    // There is no need to continue the loop as only one button can be pressed at one moment.
                    break;
                }
            }

            const auto movePenaltyText = []( const std::string & rate ) {
                std::string text = _( "Costs %{rate} times normal movement for all heroes. (Pathfinding reduces or eliminates the penalty.)" );
                StringReplace( text, "%{rate}", rate );
                return text;
            };

            if ( le.isMouseRightButtonPressedInArea( _terrainButtonsRect[TerrainBrush::WATER] ) ) {
                fheroes2::showStandardTextMessage( _getTerrainTypeName( TerrainBrush::WATER ), _( "Traversable only by boat." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( _terrainButtonsRect[TerrainBrush::GRASS] ) ) {
                fheroes2::showStandardTextMessage( _getTerrainTypeName( TerrainBrush::GRASS ), _( "No special modifiers." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( _terrainButtonsRect[TerrainBrush::SNOW] ) ) {
                fheroes2::showStandardTextMessage( _getTerrainTypeName( TerrainBrush::SNOW ), movePenaltyText( "1.5" ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( _terrainButtonsRect[TerrainBrush::SWAMP] ) ) {
                fheroes2::showStandardTextMessage( _getTerrainTypeName( TerrainBrush::SWAMP ), movePenaltyText( "1.75" ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( _terrainButtonsRect[TerrainBrush::LAVA] ) ) {
                fheroes2::showStandardTextMessage( _getTerrainTypeName( TerrainBrush::LAVA ), _( "No special modifiers." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( _terrainButtonsRect[TerrainBrush::DESERT] ) ) {
                fheroes2::showStandardTextMessage( _getTerrainTypeName( TerrainBrush::DESERT ), movePenaltyText( "2" ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( _terrainButtonsRect[TerrainBrush::DIRT] ) ) {
                fheroes2::showStandardTextMessage( _getTerrainTypeName( TerrainBrush::DIRT ), _( "No special modifiers." ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( _terrainButtonsRect[TerrainBrush::WASTELAND] ) ) {
                fheroes2::showStandardTextMessage( _getTerrainTypeName( TerrainBrush::WASTELAND ), movePenaltyText( "1.25" ), Dialog::ZERO );
            }
            else if ( le.isMouseRightButtonPressedInArea( _terrainButtonsRect[TerrainBrush::BEACH] ) ) {
                fheroes2::showStandardTextMessage( _getTerrainTypeName( TerrainBrush::BEACH ), movePenaltyText( "1.25" ), Dialog::ZERO );
            }
        }
        else if ( _selectedInstrument == Instrument::LANDSCAPE_OBJECTS ) {
            for ( size_t i = 0; i < _landscapeObjectButtonsRect.size(); ++i ) {
                if ( ( _selectedLandscapeObject != static_cast<int8_t>( i ) ) && le.isMouseLeftButtonPressedInArea( _landscapeObjectButtonsRect[i] ) ) {
                    _selectedLandscapeObject = static_cast<int8_t>( i );

                    // Reset cursor updater since this UI element was clicked.
                    _setCursor();

                    setRedraw();
                    break;
                }
            }

            for ( uint8_t objectId = LandscapeObjectBrush::MOUNTAINS; objectId < LandscapeObjectBrush::LANDSCAPE_COUNT; ++objectId ) {
                if ( le.isMouseRightButtonPressedInArea( _landscapeObjectButtonsRect[objectId] ) ) {
                    showObjectTypeInfoText( _getLandscapeObjectTypeName( objectId ) );
                    break;
                }
            }

            if ( le.MouseClickLeft( _landscapeObjectButtonsRect[LandscapeObjectBrush::MOUNTAINS] ) ) {
                handleObjectMouseClick( Dialog::selectMountainType );
                return res;
            }

            if ( le.MouseClickLeft( _landscapeObjectButtonsRect[LandscapeObjectBrush::ROCKS] ) ) {
                handleObjectMouseClick( Dialog::selectRockType );
                return res;
            }

            if ( le.MouseClickLeft( _landscapeObjectButtonsRect[LandscapeObjectBrush::TREES] ) ) {
                handleObjectMouseClick( Dialog::selectTreeType );
                return res;
            }

            if ( le.MouseClickLeft( _landscapeObjectButtonsRect[LandscapeObjectBrush::WATER_OBJECTS] ) ) {
                handleObjectMouseClick( Dialog::selectLandscapeOceanObjectType );
                return res;
            }

            if ( le.MouseClickLeft( _landscapeObjectButtonsRect[LandscapeObjectBrush::LANDSCAPE_MISC] ) ) {
                handleObjectMouseClick( Dialog::selectLandscapeMiscellaneousObjectType );
                return res;
            }
        }
        else if ( _selectedInstrument == Instrument::ADVENTURE_OBJECTS ) {
            for ( size_t i = 0; i < _adventureObjectButtonsRect.size(); ++i ) {
                if ( ( _selectedAdventureObject != static_cast<int8_t>( i ) ) && le.isMouseLeftButtonPressedInArea( _adventureObjectButtonsRect[i] ) ) {
                    _selectedAdventureObject = static_cast<int8_t>( i );

                    // Reset cursor updater since this UI element was clicked.
                    _setCursor();

                    setRedraw();
                    break;
                }
            }

            for ( uint8_t objectId = AdventureObjectBrush::ARTIFACTS; objectId < AdventureObjectBrush::ADVENTURE_COUNT; ++objectId ) {
                if ( le.isMouseRightButtonPressedInArea( _adventureObjectButtonsRect[objectId] ) ) {
                    showObjectTypeInfoText( _getAdventureObjectTypeName( objectId ) );
                    break;
                }
            }

            if ( le.MouseClickLeft( _adventureObjectButtonsRect[AdventureObjectBrush::ARTIFACTS] ) ) {
                handleObjectMouseClick( Dialog::selectArtifactType );
                return res;
            }
            if ( le.MouseClickLeft( _adventureObjectButtonsRect[AdventureObjectBrush::TREASURES] ) ) {
                handleObjectMouseClick( Dialog::selectTreasureType );
                return res;
            }
            if ( le.MouseClickLeft( _adventureObjectButtonsRect[AdventureObjectBrush::WATER_ADVENTURE] ) ) {
                handleObjectMouseClick( Dialog::selectOceanObjectType );
                return res;
            }
            if ( le.MouseClickLeft( _adventureObjectButtonsRect[AdventureObjectBrush::MINES] ) ) {
                handleObjectMouseClick( [this]( const int32_t /* type */ ) {
                    int32_t type = -1;
                    int32_t color = -1;

                    getMineObjectProperties( type, color );
                    Dialog::selectMineType( type, color );

                    return _generateMineObjectProperties( type, color );
                } );
                return res;
            }
            if ( le.MouseClickLeft( _adventureObjectButtonsRect[AdventureObjectBrush::DWELLINGS] ) ) {
                handleObjectMouseClick( Dialog::selectDwellingType );
                return res;
            }
            if ( le.MouseClickLeft( _adventureObjectButtonsRect[AdventureObjectBrush::POWER_UPS] ) ) {
                handleObjectMouseClick( Dialog::selectPowerUpObjectType );
                return res;
            }
            if ( le.MouseClickLeft( _adventureObjectButtonsRect[AdventureObjectBrush::ADVENTURE_MISC] ) ) {
                handleObjectMouseClick( Dialog::selectAdventureMiscellaneousObjectType );
                return res;
            }
        }
        else if ( _selectedInstrument == Instrument::KINGDOM_OBJECTS ) {
            for ( size_t i = 0; i < _kingdomObjectButtonsRect.size(); ++i ) {
                if ( ( _selectedKingdomObject != static_cast<int8_t>( i ) ) && le.isMouseLeftButtonPressedInArea( _kingdomObjectButtonsRect[i] ) ) {
                    _selectedKingdomObject = static_cast<int8_t>( i );

                    // Reset cursor updater since this UI element was clicked.
                    _setCursor();

                    setRedraw();
                    break;
                }
            }

            for ( uint8_t objectId = KingdomObjectBrush::HEROES; objectId < KingdomObjectBrush::KINGDOM_OBJECTS_COUNT; ++objectId ) {
                if ( le.isMouseRightButtonPressedInArea( _kingdomObjectButtonsRect[objectId] ) ) {
                    showObjectTypeInfoText( _getKingdomObjectTypeName( objectId ) );
                    break;
                }
            }

            if ( le.MouseClickLeft( _kingdomObjectButtonsRect[KingdomObjectBrush::HEROES] ) ) {
                handleObjectMouseClick( Dialog::selectHeroType );
                return res;
            }
            if ( le.MouseClickLeft( _kingdomObjectButtonsRect[KingdomObjectBrush::TOWNS] ) ) {
                handleObjectMouseClick( [this]( const int32_t /* type */ ) {
                    int32_t type = -1;
                    int32_t color = -1;

                    getTownObjectProperties( type, color );
                    Dialog::selectTownType( type, color );

                    return _generateTownObjectProperties( type, color );
                } );
                return res;
            }
        }
        else if ( _selectedInstrument == Instrument::MONSTERS && le.MouseClickLeft( _rectInstrumentPanel ) ) {
            handleObjectMouseClick( Dialog::selectMonsterType );
            setRedraw();
            return res;
        }
        else if ( _selectedInstrument == Instrument::ERASE ) {
            for ( size_t i = 0; i < _eraseButtonsRect.size(); ++i ) {
                if ( le.MouseClickLeft( _eraseButtonsRect[i] ) ) {
                    _eraseTypes ^= _eraseButtonObjectTypes[i];
                    setRedraw();
                }
                else if ( le.isMouseRightButtonPressedInArea( _eraseButtonsRect[i] ) ) {
                    std::string header = _( "Toggle the erasure of %{type} objects." );
                    StringReplaceWithLowercase( header, "%{type}", _getEraseObjectTypeName( _eraseButtonObjectTypes[i] ) );

                    fheroes2::showStandardTextMessage(
                        std::move( header ),
                        ( _eraseButtonObjectTypes[i] & _eraseTypes )
                            ? _(
                                "Objects of this type will be deleted with the Erase tool. Left-click here to deselect this type. Press and hold this button to deselect all other object types." )
                            : _(
                                "Objects of this type will NOT be deleted with the Erase tool. Left-click here to select this type. Press and hold this button to select all other object types." ),
                        Dialog::ZERO );
                }
                else if ( le.MouseLongPressLeft( _eraseButtonsRect[i] ) ) {
                    _eraseTypes = ( _eraseButtonObjectTypes[i] & _eraseTypes ) ? _eraseButtonObjectTypes[i]
                                                                               : ( ObjectErasureType::ERASE_ALL_OBJECTS & ~_eraseButtonObjectTypes[i] );
                    setRedraw();
                }
            }
        }

        _buttonMagnify.drawOnState( le.isMouseLeftButtonPressedInArea( _rectMagnify ) );
        _buttonUndo.drawOnState( le.isMouseLeftButtonPressedInArea( _rectUndo ) );
        _buttonNew.drawOnState( le.isMouseLeftButtonPressedInArea( _rectNew ) );
        _buttonSpecs.drawOnState( le.isMouseLeftButtonPressedInArea( _rectSpecs ) );
        _buttonFile.drawOnState( le.isMouseLeftButtonPressedInArea( _rectFile ) );
        _buttonSystem.drawOnState( le.isMouseLeftButtonPressedInArea( _rectSystem ) );

        if ( le.MouseClickLeft( _rectMagnify ) ) {
            _interface.eventViewWorld();
        }
        else if ( _buttonUndo.isEnabled() && le.MouseClickLeft( _rectUndo ) ) {
            _interface.undoAction();
            return fheroes2::GameMode::CANCEL;
        }
        else if ( le.MouseClickLeft( _rectNew ) ) {
            res = EditorInterface::eventNewMap();
        }
        else if ( le.MouseClickLeft( _rectSpecs ) ) {
            EditorInterface::Get().openMapSpecificationsDialog();
        }
        else if ( le.MouseClickLeft( _rectFile ) ) {
            res = Interface::EditorInterface::eventFileDialog();
        }
        else if ( le.MouseClickLeft( _rectSystem ) ) {
            Editor::openEditorSettings();
        }
        else if ( le.isMouseRightButtonPressedInArea( _instrumentButtonsRect[Instrument::TERRAIN] ) ) {
            fheroes2::showStandardTextMessage( _( "Terrain Mode" ), _( "Used to draw the underlying grass, dirt, water, etc. on the map." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( _instrumentButtonsRect[Instrument::LANDSCAPE_OBJECTS] ) ) {
            fheroes2::showStandardTextMessage( _( "Landscape Objects Mode" ), _( "Used to place landscape objects (mountains, rocks, trees, etc.) on the map." ),
                                               Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( _instrumentButtonsRect[Instrument::DETAIL] ) ) {
            fheroes2::showStandardTextMessage( _( "Detail Mode" ), _( "Used for special editing of action objects." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( _instrumentButtonsRect[Instrument::ADVENTURE_OBJECTS] ) ) {
            fheroes2::showStandardTextMessage( _( "Adventure Objects Mode" ),
                                               _( "Used to place adventure objects (artifacts, dwellings, mines, treasures, etc.) on the map." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( _instrumentButtonsRect[Instrument::KINGDOM_OBJECTS] ) ) {
            fheroes2::showStandardTextMessage( _( "Kingdom Objects Mode" ), _( "Used to place kingdom objects (towns, castles and heroes) on the map." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( _instrumentButtonsRect[Instrument::MONSTERS] ) ) {
            fheroes2::showStandardTextMessage( _( "Monsters Mode" ), _( "Used to place monsters on the map." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( _instrumentButtonsRect[Instrument::STREAM] ) ) {
            fheroes2::showStandardTextMessage( _( "Stream Mode" ), _( "Allows you to draw streams by clicking and dragging." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( _instrumentButtonsRect[Instrument::ROAD] ) ) {
            fheroes2::showStandardTextMessage( _( "Road Mode" ), _( "Allows you to draw roads by clicking and dragging." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( _instrumentButtonsRect[Instrument::ERASE] ) ) {
            fheroes2::showStandardTextMessage( _( "Erase Mode" ), _( "Used to erase objects from the map." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( _rectMagnify ) ) {
            fheroes2::showStandardTextMessage( _( "Magnify" ), _( "Change between zoom and normal view." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( _rectUndo ) ) {
            fheroes2::showStandardTextMessage( _( "Undo" ), _( "Undo your last action." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( _rectNew ) ) {
            // TODO: update this text once random map generator is ready.
            //       The original text should be "Create a new map, either from scratch or using the random map generator."
            fheroes2::showStandardTextMessage( _( "New Map" ), _( "Create a new map from scratch." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( _rectSpecs ) ) {
            fheroes2::showStandardTextMessage( _( "Specifications" ), _( "Edit map title, description, and other general information." ), Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( _rectFile ) ) {
            fheroes2::showStandardTextMessage( _( "File Options" ), _( "Open the file options menu, where you can save or load maps, or quit out of the editor." ),
                                               Dialog::ZERO );
        }
        else if ( le.isMouseRightButtonPressedInArea( _rectSystem ) ) {
            fheroes2::showStandardTextMessage( _( "System Options" ), _( "View the editor system options, which let you customize the editor." ), Dialog::ZERO );
        }

        return res;
    }

    void EditorPanel::handleObjectMouseClick( const std::function<int( int )> & typeSelection )
    {
        const int type = typeSelection( getSelectedObjectType() );
        if ( type >= 0 ) {
            switch ( _selectedInstrument ) {
            case Instrument::MONSTERS:
                _selectedMonsterType = type;
                break;
            case Instrument::LANDSCAPE_OBJECTS:
                _selectedLandscapeObjectType[_selectedLandscapeObject] = type;
                break;
            case Instrument::ADVENTURE_OBJECTS:
                _selectedAdventureObjectType[_selectedAdventureObject] = type;
                break;
            case Instrument::KINGDOM_OBJECTS:
                _selectedKingdomObjectType[_selectedKingdomObject] = type;
                break;
            default:
                // Why are you trying to get type for the non-object instrument. Check your logic!
                assert( 0 );
            }
        }
        _setCursor();
        _interface.updateCursor( 0 );
    }

    void EditorPanel::getTownObjectProperties( int32_t & type, int32_t & color ) const
    {
        const auto & townObjects = Maps::getObjectsByGroup( Maps::ObjectGroup::KINGDOM_TOWNS );
        if ( townObjects.empty() ) {
            // How is it even possible?
            assert( 0 );
            type = -1;
            color = -1;
            return;
        }

        type = _selectedKingdomObjectType[KingdomObjectBrush::TOWNS] % static_cast<int32_t>( townObjects.size() );
        color = _selectedKingdomObjectType[KingdomObjectBrush::TOWNS] / static_cast<int32_t>( townObjects.size() );
    }

    int32_t EditorPanel::_generateTownObjectProperties( const int32_t type, const int32_t color )
    {
        const auto & townObjects = Maps::getObjectsByGroup( Maps::ObjectGroup::KINGDOM_TOWNS );
        if ( townObjects.empty() ) {
            // How is it even possible?
            assert( 0 );
            return -1;
        }

        return color * static_cast<int32_t>( townObjects.size() ) + type;
    }

    void EditorPanel::getMineObjectProperties( int32_t & type, int32_t & color ) const
    {
        const auto & mineObjects = Maps::getObjectsByGroup( Maps::ObjectGroup::ADVENTURE_MINES );
        if ( mineObjects.empty() ) {
            // How is it even possible?
            assert( 0 );
            type = -1;
            color = -1;
            return;
        }

        type = _selectedAdventureObjectType[AdventureObjectBrush::MINES] % static_cast<int32_t>( mineObjects.size() );
        color = _selectedAdventureObjectType[AdventureObjectBrush::MINES] / static_cast<int32_t>( mineObjects.size() );
    }

    int32_t EditorPanel::_generateMineObjectProperties( const int32_t type, const int32_t color )
    {
        const auto & mineObjects = Maps::getObjectsByGroup( Maps::ObjectGroup::ADVENTURE_MINES );
        if ( mineObjects.empty() ) {
            // How is it even possible?
            assert( 0 );
            return -1;
        }

        const int32_t objectType = ( color * static_cast<int32_t>( mineObjects.size() ) + type );
        return ( objectType < 0 ) ? -1 : objectType;
    }
}
