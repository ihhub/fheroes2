/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2023                                                    *
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

#include <cassert>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "agg_image.h"
#include "cursor.h"
#include "dialog.h"
#include "dialog_selectitems.h"
#include "dialog_system_options.h"
#include "editor_interface.h"
#include "ground.h"
#include "icn.h"
#include "image.h"
#include "interface_base.h"
#include "localevent.h"
#include "map_object_info.h"
#include "screen.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_map_object.h"
#include "ui_text.h"

namespace
{
    void setCustomCursor( const Maps::ObjectGroup group, const int32_t type )
    {
        if ( type == -1 ) {
            // The object type is not set. We show the POINTER cursor for this case.
            Cursor::Get().SetThemes( Cursor::POINTER );
            return;
        }

        const auto & objectInfo = Maps::getObjectsByGroup( group );
        if ( type < 0 || type >= static_cast<int32_t>( objectInfo.size() ) ) {
            // You are trying to render some unknown stuff!
            assert( 0 );
            return;
        }

        const fheroes2::Sprite & image = fheroes2::generateMapObjectImage( objectInfo[type] );

        Cursor::Get().setCustomImage( image, { image.x(), image.y() } );
    }
}

namespace Interface
{
    EditorPanel::EditorPanel( EditorInterface & interface_ )
        : _interface( interface_ )
    {
        int32_t icnIndex = 0;

        // Editor Instruments go in this order in ICN: TERRAIN, OBJECT, DETAIL, STREAM, ROAD, ERASE.
        for ( fheroes2::Button & button : _instrumentButtons ) {
            button.setICNInfo( ICN::EDITBTNS, icnIndex, icnIndex + 1 );
            icnIndex += 2;
        }

        _buttonMagnify.setICNInfo( ICN::EDITBTNS, 12, 13 );
        _buttonUndo.setICNInfo( ICN::EDITBTNS, 14, 15 );
        _buttonNew.setICNInfo( ICN::EDITBTNS, 16, 17 );
        _buttonSpecs.setICNInfo( ICN::EDITBTNS, 18, 19 );
        _buttonFile.setICNInfo( ICN::EDITBTNS, 20, 21 );
        _buttonSystem.setICNInfo( ICN::EDITBTNS, 22, 23 );

        // Brush Size buttons go in this order in ICN: SMALL (1x), MEDIUM (2x), LARGE (4x), AREA.
        icnIndex = 24;
        for ( fheroes2::Button & button : _brushSizeButtons ) {
            button.setICNInfo( ICN::EDITBTNS, icnIndex, icnIndex + 1 );
            icnIndex += 2;
        }

        _instrumentButtons[_selectedInstrument].press();
        _brushSizeButtons[_selectedBrushSize].press();
    }

    int32_t EditorPanel::getBrushSize() const
    {
        // Roads and streams are placed using only 1x1 brush.
        if ( _selectedInstrument == Instrument::STREAM || _selectedInstrument == Instrument::ROAD || isMonsterSettingMode() || isHeroSettingMode()
             || isTreasureSettingMode() || isArtifactSettingMode() ) {
            return 1;
        }

        switch ( _selectedBrushSize ) {
        case BrushSize::SMALL:
            return 1;
        case BrushSize::MEDIUM:
            return 2;
        case BrushSize::LARGE:
            return 4;
        case BrushSize::AREA:
            return 0;
        default:
            // Have you added a new Brush size? Update the logic above!
            assert( 0 );
            break;
        }

        return 0;
    }

    void EditorPanel::setPos( const int32_t displayX, int32_t displayY )
    {
        int32_t offsetX = displayX;

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
        const fheroes2::Sprite & instrumentPanel = fheroes2::AGG::GetICN( ICN::EDITPANL, _selectedInstrument );
        _rectInstrumentPanel = { displayX, displayY, instrumentPanel.width(), instrumentPanel.height() };

        offsetX = displayX + 14;
        int32_t offsetY = displayY + 128;
        for ( size_t i = 0; i < _brushSizeButtonsRect.size(); ++i ) {
            _brushSizeButtons[i].setPosition( offsetX, offsetY );
            _brushSizeButtonsRect[i] = _brushSizeButtons[i].area();
            offsetX += 30;
        }

        offsetX = displayX + 30;
        offsetY = displayY + 11;
        for ( size_t i = 0; i < _terrainButtonsRect.size(); ++i ) {
            _terrainButtonsRect[i] = { offsetX + static_cast<int32_t>( i % 3 ) * 29, offsetY + static_cast<int32_t>( i / 3 ) * 29, 27, 27 };
        }

        offsetX = displayX + 15;
        ++offsetY;
        for ( size_t i = 0; i < _objectButtonsRect.size(); ++i ) {
            _objectButtonsRect[i] = { offsetX + static_cast<int32_t>( i % 4 ) * 29, offsetY + static_cast<int32_t>( i / 4 ) * 28, 27, 27 };
        }
        // The last object button is located not next to previous one and needs to be shifted to the right.
        _objectButtonsRect[Brush::TREASURES].x += 29 * 2;

        offsetY += 56;
        for ( size_t i = 0; i < _eraseButtonsRect.size(); ++i ) {
            _eraseButtonsRect[i] = { offsetX + static_cast<int32_t>( i % 4 ) * 29, offsetY + static_cast<int32_t>( i / 4 ) * 28, 27, 27 };
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

        const fheroes2::Sprite & instrumentPanel = fheroes2::AGG::GetICN( ICN::EDITPANL, _selectedInstrument );
        fheroes2::Copy( instrumentPanel, 0, 0, display, _rectEditorPanel.x, _rectInstruments.y + _rectInstruments.height, instrumentPanel.width(),
                        instrumentPanel.height() );

        if ( _selectedInstrument == Instrument::TERRAIN || _selectedInstrument == Instrument::ERASE ) {
            for ( const fheroes2::Button & button : _brushSizeButtons ) {
                button.draw();
            }
        }

        if ( _selectedInstrument == Instrument::TERRAIN ) {
            const fheroes2::Sprite & selection = fheroes2::AGG::GetICN( ICN::TERRAINS, 9 );
            fheroes2::Blit( selection, 0, 0, display, _terrainButtonsRect[_selectedTerrain].x - 2, _terrainButtonsRect[_selectedTerrain].y - 2, selection.width(),
                            selection.height() );

            const fheroes2::Text terrainText( _getTerrainTypeName( _selectedTerrain ), fheroes2::FontType::smallWhite() );
            terrainText.draw( _rectInstrumentPanel.x + 72 - terrainText.width() / 2, _rectInstrumentPanel.y + 107, display );
        }
        else if ( _selectedInstrument == Instrument::OBJECT ) {
            const fheroes2::Sprite & selection = fheroes2::AGG::GetICN( ICN::TERRAINS, 9 );
            fheroes2::Blit( selection, 0, 0, display, _objectButtonsRect[_selectedObject].x - 2, _objectButtonsRect[_selectedObject].y - 2, selection.width(),
                            selection.height() );

            const fheroes2::Text terrainText( _getObjectTypeName( _selectedObject ), fheroes2::FontType::smallWhite() );
            terrainText.draw( _rectInstrumentPanel.x + 72 - terrainText.width() / 2, _rectInstrumentPanel.y + 135, display );
        }
        else if ( _selectedInstrument == Instrument::ERASE ) {
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

        display.render( _rectInstrumentPanel );
    }

    int EditorPanel::_getGroundId( const uint8_t brushId )
    {
        switch ( brushId ) {
        case Brush::WATER:
            return Maps::Ground::WATER;
        case Brush::GRASS:
            return Maps::Ground::GRASS;
        case Brush::SNOW:
            return Maps::Ground::SNOW;
        case Brush::SWAMP:
            return Maps::Ground::SWAMP;
        case Brush::LAVA:
            return Maps::Ground::LAVA;
        case Brush::DESERT:
            return Maps::Ground::DESERT;
        case Brush::DIRT:
            return Maps::Ground::DIRT;
        case Brush::WASTELAND:
            return Maps::Ground::WASTELAND;
        case Brush::BEACH:
            return Maps::Ground::BEACH;
        default:
            // Have you added a new terrain type? Add the logic above!
            assert( 0 );
            break;
        }
        return Maps::Ground::UNKNOWN;
    }

    const char * EditorPanel::_getObjectTypeName( const uint8_t brushId )
    {
        switch ( brushId ) {
        case Brush::WATER:
            return _( "Ocean Objects" );
        case Brush::GRASS:
            return _( "Grass Objects" );
        case Brush::SNOW:
            return _( "Snow Objects" );
        case Brush::SWAMP:
            return _( "Swamp Objects" );
        case Brush::LAVA:
            return _( "Lava Objects" );
        case Brush::DESERT:
            return _( "Desert Objects" );
        case Brush::DIRT:
            return _( "Dirt Objects" );
        case Brush::WASTELAND:
            return _( "Wasteland Objects" );
        case Brush::BEACH:
            return _( "Beach Objects" );
        case Brush::TOWNS:
            return _( "Towns" );
        case Brush::MONSTERS:
            return _( "Monsters" );
        case Brush::HEROES:
            return _( "Heroes" );
        case Brush::ARTIFACTS:
            return _( "Artifacts" );
        case Brush::TREASURES:
            return _( "Treasures" );
        default:
            // Have you added a new object type? Add the logic above!
            assert( 0 );
            break;
        }

        return "Unknown object type";
    }

    const char * EditorPanel::_getEraseObjectTypeName( const uint32_t eraseObjectType )
    {
        switch ( eraseObjectType ) {
        case Maps::ObjectErasureType::TERRAIN_OBJECTS:
            return _( "Terrain" );
        case Maps::ObjectErasureType::CASTLES:
            return _( "Castles" );
        case Maps::ObjectErasureType::MONSTERS:
            return _( "Monsters" );
        case Maps::ObjectErasureType::HEROES:
            return _( "Heroes" );
        case Maps::ObjectErasureType::ARTIFACTS:
            return _( "Artifacts" );
        case Maps::ObjectErasureType::ROADS:
            return _( "Roads" );
        case Maps::ObjectErasureType::STREAMS:
            return _( "Streams" );
        case Maps::ObjectErasureType::TREASURES:
            return _( "Treasures" );
        default:
            // Have you added a new object type to erase? Add the logic above!
            assert( 0 );
            break;
        }

        return "Unknown object type";
    }

    void EditorPanel::_setCursor()
    {
        if ( _selectedInstrument != Instrument::OBJECT ) {
            _interface.setCursorUpdater( {} );
            return;
        }

        switch ( _selectedObject ) {
        case Brush::MONSTERS:
            _interface.setCursorUpdater( [type = _monsterType]( const int32_t /*tileIndex*/ ) { setCustomCursor( Maps::ObjectGroup::Monster, type ); } );
            break;
        case Brush::TREASURES:
            _interface.setCursorUpdater( [type = _treasureType]( const int32_t /*tileIndex*/ ) { setCustomCursor( Maps::ObjectGroup::Treasure, type ); } );
            break;
        case Brush::HEROES:
            _interface.setCursorUpdater( [type = _heroType]( const int32_t /*tileIndex*/ ) {
                if ( type == -1 ) {
                    // The object type is not set. We show the POINTER cursor for this case.
                    Cursor::Get().SetThemes( Cursor::POINTER );
                    return;
                }

                // TODO: render ICN::MINIHERO from the existing hero images.
                const fheroes2::Sprite & image = fheroes2::AGG::GetICN( ICN::MINIHERO, type );

                // Mini-hero images contain a pole with a flag.
                // This causes a situation that a selected tile does not properly correspond to the position of cursor.
                // We need to add a hardcoded correction.
                const int32_t heroCorrectionY{ 12 };
                Cursor::Get().setCustomImage( image, { -image.width() / 2, -image.height() / 2 - heroCorrectionY } );
            } );
            break;
        case Brush::ARTIFACTS:
            _interface.setCursorUpdater( [type = _artifactType]( const int32_t /*tileIndex*/ ) { setCustomCursor( Maps::ObjectGroup::Artifact, type ); } );
            break;
        default:
            _interface.setCursorUpdater( {} );
            break;
        }
    }

    fheroes2::GameMode EditorPanel::queueEventProcessing()
    {
        LocalEvent & le = LocalEvent::Get();
        fheroes2::GameMode res = fheroes2::GameMode::CANCEL;

        if ( le.MousePressLeft( _rectInstruments ) ) {
            for ( size_t i = 0; i < _instrumentButtonsRect.size(); ++i ) {
                if ( le.MousePressLeft( _instrumentButtonsRect[i] ) ) {
                    if ( _instrumentButtons[i].drawOnPress() ) {
                        _selectedInstrument = static_cast<uint8_t>( i );

                        // Reset cursor updater since this UI element was clicked.
                        _setCursor();

                        setRedraw();
                    }
                }
                else {
                    _instrumentButtons[i].drawOnRelease();
                }
            }
        }

        if ( _selectedInstrument == Instrument::TERRAIN || _selectedInstrument == Instrument::ERASE ) {
            for ( size_t i = 0; i < _brushSizeButtonsRect.size(); ++i ) {
                if ( le.MousePressLeft( _brushSizeButtonsRect[i] ) ) {
                    if ( _brushSizeButtons[i].drawOnPress() ) {
                        _selectedBrushSize = static_cast<uint8_t>( i );
                    }
                }
                else if ( i != _selectedBrushSize ) {
                    _brushSizeButtons[i].drawOnRelease();
                }
            }

            const auto brushSizeText = []( const int brushSize, const bool isFillBrush ) {
                std::string text
                    = isFillBrush ? _( "Draws terrain in\n%{size} by %{size} square increments." ) : _( "Erases objects in\n%{size} by %{size} square increments." );

                StringReplace( text, "%{size}", brushSize );
                return text;
            };

            if ( le.MousePressRight( _brushSizeButtonsRect[BrushSize::SMALL] ) ) {
                fheroes2::showStandardTextMessage( _( "Small Brush" ), brushSizeText( 1, _selectedInstrument == Instrument::TERRAIN ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( _brushSizeButtonsRect[BrushSize::MEDIUM] ) ) {
                fheroes2::showStandardTextMessage( _( "Medium Brush" ), brushSizeText( 2, _selectedInstrument == Instrument::TERRAIN ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( _brushSizeButtonsRect[BrushSize::LARGE] ) ) {
                fheroes2::showStandardTextMessage( _( "Large Brush" ), brushSizeText( 4, _selectedInstrument == Instrument::TERRAIN ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( _brushSizeButtonsRect[BrushSize::AREA] ) ) {
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
                if ( ( _selectedTerrain != i ) && le.MousePressLeft( _terrainButtonsRect[i] ) ) {
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

            if ( le.MousePressRight( _terrainButtonsRect[Brush::WATER] ) ) {
                fheroes2::showStandardTextMessage( _getTerrainTypeName( Brush::WATER ), _( "Traversable only by boat." ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( _terrainButtonsRect[Brush::GRASS] ) ) {
                fheroes2::showStandardTextMessage( _getTerrainTypeName( Brush::GRASS ), _( "No special modifiers." ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( _terrainButtonsRect[Brush::SNOW] ) ) {
                fheroes2::showStandardTextMessage( _getTerrainTypeName( Brush::SNOW ), movePenaltyText( "1.5" ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( _terrainButtonsRect[Brush::SWAMP] ) ) {
                fheroes2::showStandardTextMessage( _getTerrainTypeName( Brush::SWAMP ), movePenaltyText( "1.75" ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( _terrainButtonsRect[Brush::LAVA] ) ) {
                fheroes2::showStandardTextMessage( _getTerrainTypeName( Brush::LAVA ), _( "No special modifiers." ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( _terrainButtonsRect[Brush::DESERT] ) ) {
                fheroes2::showStandardTextMessage( _getTerrainTypeName( Brush::DESERT ), movePenaltyText( "2" ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( _terrainButtonsRect[Brush::DIRT] ) ) {
                fheroes2::showStandardTextMessage( _getTerrainTypeName( Brush::DIRT ), _( "No special modifiers." ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( _terrainButtonsRect[Brush::WASTELAND] ) ) {
                fheroes2::showStandardTextMessage( _getTerrainTypeName( Brush::WASTELAND ), movePenaltyText( "1.25" ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( _terrainButtonsRect[Brush::BEACH] ) ) {
                fheroes2::showStandardTextMessage( _getTerrainTypeName( Brush::BEACH ), movePenaltyText( "1.25" ), Dialog::ZERO );
            }
        }
        else if ( _selectedInstrument == Instrument::OBJECT ) {
            for ( size_t i = 0; i < _objectButtonsRect.size(); ++i ) {
                if ( ( _selectedObject != i ) && le.MousePressLeft( _objectButtonsRect[i] ) ) {
                    _selectedObject = static_cast<uint8_t>( i );

                    // Reset cursor updater since this UI element was clicked.
                    _setCursor();

                    setRedraw();

                    // There is no need to continue the loop as only one button can be pressed at one moment.
                    break;
                }
            }

            for ( uint8_t objectId = Brush::WATER; objectId < Brush::TOWNS; ++objectId ) {
                if ( le.MousePressRight( _objectButtonsRect[objectId] ) ) {
                    std::string text = _( "Used to place objects most appropriate for use on %{terrain}." );
                    StringReplaceWithLowercase( text, "%{terrain}", _getTerrainTypeName( objectId ) );
                    fheroes2::showStandardTextMessage( _getObjectTypeName( objectId ), std::move( text ), Dialog::ZERO );

                    // There is no need to continue the loop as only one button can be pressed at one moment.
                    break;
                }
            }

            if ( le.MousePressRight( _objectButtonsRect[Brush::TOWNS] ) ) {
                fheroes2::showStandardTextMessage( _getObjectTypeName( Brush::TOWNS ), _( "Used to place\na town or castle." ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( _objectButtonsRect[Brush::MONSTERS] ) ) {
                fheroes2::showStandardTextMessage( _getObjectTypeName( Brush::MONSTERS ), _( "Used to place\na monster group." ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( _objectButtonsRect[Brush::HEROES] ) ) {
                fheroes2::showStandardTextMessage( _getObjectTypeName( Brush::HEROES ), _( "Used to place a hero." ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( _objectButtonsRect[Brush::ARTIFACTS] ) ) {
                fheroes2::showStandardTextMessage( _getObjectTypeName( Brush::ARTIFACTS ), _( "Used to place an artifact." ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( _objectButtonsRect[Brush::TREASURES] ) ) {
                fheroes2::showStandardTextMessage( _getObjectTypeName( Brush::TREASURES ), _( "Used to place\na resource or treasure." ), Dialog::ZERO );
            }
            else if ( le.MouseClickLeft( _objectButtonsRect[Brush::MONSTERS] ) ) {
                const int monsterType = Dialog::selectMonsterType( _monsterType );
                if ( monsterType >= 0 ) {
                    _monsterType = monsterType;
                }
                _setCursor();
                _interface.updateCursor( 0 );
                return res;
            }
            else if ( le.MouseClickLeft( _objectButtonsRect[Brush::TREASURES] ) ) {
                const int treasureType = Dialog::selectTreasureType( _treasureType );
                if ( treasureType >= 0 ) {
                    _treasureType = treasureType;
                }
                _setCursor();
                _interface.updateCursor( 0 );
                return res;
            }
            else if ( le.MouseClickLeft( _objectButtonsRect[Brush::HEROES] ) ) {
                const int32_t heroType = Dialog::selectHeroType( _heroType );
                if ( heroType >= 0 ) {
                    _heroType = heroType;
                }

                _setCursor();
                _interface.updateCursor( 0 );
                return res;
            }
            else if ( le.MouseClickLeft( _objectButtonsRect[Brush::ARTIFACTS] ) ) {
                const int artifactType = Dialog::selectArtifactType( _artifactType );
                if ( artifactType >= 0 ) {
                    _artifactType = artifactType;
                }
                _setCursor();
                _interface.updateCursor( 0 );
                return res;
            }
        }
        else if ( _selectedInstrument == Instrument::ERASE ) {
            for ( size_t i = 0; i < _eraseButtonsRect.size(); ++i ) {
                if ( le.MouseClickLeft( _eraseButtonsRect[i] ) ) {
                    _eraseTypes ^= _eraseButtonObjectTypes[i];
                    setRedraw();
                }
                else if ( le.MousePressRight( _eraseButtonsRect[i] ) ) {
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
                                                                               : ( Maps::ObjectErasureType::ALL_OBJECTS & ~_eraseButtonObjectTypes[i] );
                    setRedraw();
                }
            }
        }

        le.MousePressLeft( _rectMagnify ) ? _buttonMagnify.drawOnPress() : _buttonMagnify.drawOnRelease();
        le.MousePressLeft( _rectUndo ) ? _buttonUndo.drawOnPress() : _buttonUndo.drawOnRelease();
        le.MousePressLeft( _rectNew ) ? _buttonNew.drawOnPress() : _buttonNew.drawOnRelease();
        le.MousePressLeft( _rectSpecs ) ? _buttonSpecs.drawOnPress() : _buttonSpecs.drawOnRelease();
        le.MousePressLeft( _rectFile ) ? _buttonFile.drawOnPress() : _buttonFile.drawOnRelease();
        le.MousePressLeft( _rectSystem ) ? _buttonSystem.drawOnPress() : _buttonSystem.drawOnRelease();

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
            // TODO: Make the scenario info editor.
            Dialog::GameInfo();
        }
        else if ( le.MouseClickLeft( _rectFile ) ) {
            res = Interface::EditorInterface::eventFileDialog();
        }
        else if ( le.MouseClickLeft( _rectSystem ) ) {
            // Replace this with Editor options dialog.
            fheroes2::showSystemOptionsDialog();
        }
        else if ( le.MousePressRight( _instrumentButtonsRect[Instrument::TERRAIN] ) ) {
            fheroes2::showStandardTextMessage( _( "Terrain Mode" ), _( "Used to draw the underlying grass, dirt, water, etc. on the map." ), Dialog::ZERO );
        }
        else if ( le.MousePressRight( _instrumentButtonsRect[Instrument::OBJECT] ) ) {
            fheroes2::showStandardTextMessage( _( "Object Mode" ), _( "Used to place objects (mountains, trees, treasure, etc.) on the map." ), Dialog::ZERO );
        }
        else if ( le.MousePressRight( _instrumentButtonsRect[Instrument::DETAIL] ) ) {
            fheroes2::showStandardTextMessage( _( "Detail Mode" ), _( "Used for special editing of monsters, heroes and towns." ), Dialog::ZERO );
        }
        else if ( le.MousePressRight( _instrumentButtonsRect[Instrument::STREAM] ) ) {
            fheroes2::showStandardTextMessage( _( "Stream Mode" ), _( "Allows you to draw streams by clicking and dragging." ), Dialog::ZERO );
        }
        else if ( le.MousePressRight( _instrumentButtonsRect[Instrument::ROAD] ) ) {
            fheroes2::showStandardTextMessage( _( "Road Mode" ), _( "Allows you to draw roads by clicking and dragging." ), Dialog::ZERO );
        }
        else if ( le.MousePressRight( _instrumentButtonsRect[Instrument::ERASE] ) ) {
            fheroes2::showStandardTextMessage( _( "Erase Mode" ), _( "Used to erase objects from the map." ), Dialog::ZERO );
        }
        else if ( le.MousePressRight( _rectMagnify ) ) {
            fheroes2::showStandardTextMessage( _( "Magnify" ), _( "Change between zoom and normal view." ), Dialog::ZERO );
        }
        else if ( le.MousePressRight( _rectUndo ) ) {
            fheroes2::showStandardTextMessage( _( "Undo" ), _( "Undo your last action." ), Dialog::ZERO );
        }
        else if ( le.MousePressRight( _rectNew ) ) {
            fheroes2::showStandardTextMessage( _( "New Map" ), _( "Create a new map either from scratch or using the random map generator." ), Dialog::ZERO );
        }
        else if ( le.MousePressRight( _rectSpecs ) ) {
            fheroes2::showStandardTextMessage( _( "Specifications" ), _( "Edit map title, description, and other general information." ), Dialog::ZERO );
        }
        else if ( le.MousePressRight( _rectFile ) ) {
            fheroes2::showStandardTextMessage( _( "File Options" ), _( "Open the file options menu, where you can save or load maps, or quit out of the editor." ),
                                               Dialog::ZERO );
        }
        else if ( le.MousePressRight( _rectSystem ) ) {
            fheroes2::showStandardTextMessage( _( "System Options" ), _( "View the editor system options, which let you customize the editor." ), Dialog::ZERO );
        }

        return res;
    }
}
