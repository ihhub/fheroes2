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
#include <string>
#include <utility>

#include "agg_image.h"
#include "dialog.h"
#include "dialog_system_options.h"
#include "editor_interface.h"
#include "icn.h"
#include "image.h"
#include "interface_base.h"
#include "localevent.h"
#include "screen.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_text.h"

namespace Interface
{
    EditorPanel::EditorPanel( Editor & interface_ )
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

    void EditorPanel::setPos( const int32_t displayX, int32_t displayY )
    {
        int32_t offsetX = displayX;

        for ( uint8_t i = 0; i < Instrument::INSTRUMENTS_COUNT; ++i ) {
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
        for ( uint8_t i = 0; i < BrushSize::BRUSH_SIZE_COUNT; ++i ) {
            _brushSizeButtons[i].setPosition( offsetX, offsetY );
            _brushSizeButtonsRect[i] = _brushSizeButtons[i].area();
            offsetX += 30;
        }

        offsetX = displayX + 30;
        offsetY = displayY + 11;
        for ( uint8_t i = 0; i < Brush::TERRAIN_COUNT; ++i ) {
            _terrainButtonsRect[i] = { offsetX + ( i % 3 ) * 29, offsetY + ( i / 3 ) * 29, 27, 27 };
        }

        offsetX = displayX + 15;
        ++offsetY;
        for ( uint8_t i = 0; i < Brush::OBJECT_COUNT; ++i ) {
            _objectButtonsRect[i] = { offsetX + ( i % 4 ) * 29, offsetY + ( i / 4 ) * 28, 27, 27 };
        }
        // The last object button is located not next to previous one and needs to be shifted to the right.
        _objectButtonsRect[Brush::TREASURES].x += 29 * 2;

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

            std::string text;
            switch ( _selectedTerrain ) {
            case Brush::WATER:
                text = _( "Ocean" );
                break;
            case Brush::GRASS:
                text = _( "Grass" );
                break;
            case Brush::SNOW:
                text = _( "Snow" );
                break;
            case Brush::SWAMP:
                text = _( "Swamp" );
                break;
            case Brush::LAVA:
                text = _( "Lava" );
                break;
            case Brush::DESERT:
                text = _( "Desert" );
                break;
            case Brush::DIRT:
                text = _( "Dirt" );
                break;
            case Brush::WASTELAND:
                text = _( "Wasteland" );
                break;
            case Brush::BEACH:
                text = _( "Beach" );
                break;
            default:
                // You have added a new terrain type?
                assert( 0 );
                break;
            }

            const fheroes2::Text terrainText( std::move( text ), fheroes2::FontType::smallWhite() );
            terrainText.draw( _rectInstrumentPanel.x + 72 - terrainText.width( 118 ) / 2, _rectInstrumentPanel.y + 107, display );
        }

        if ( _selectedInstrument == Instrument::OBJECT ) {
            const fheroes2::Sprite & selection = fheroes2::AGG::GetICN( ICN::TERRAINS, 9 );
            fheroes2::Blit( selection, 0, 0, display, _objectButtonsRect[_selectedObject].x - 2, _objectButtonsRect[_selectedObject].y - 2, selection.width(),
                            selection.height() );

            std::string text;
            switch ( _selectedObject ) {
            case Brush::WATER:
                text = _( "Ocean Objects" );
                break;
            case Brush::GRASS:
                text = _( "Grass Objects" );
                break;
            case Brush::SNOW:
                text = _( "Snow Objects" );
                break;
            case Brush::SWAMP:
                text = _( "Swamp Objects" );
                break;
            case Brush::LAVA:
                text = _( "Lava Objects" );
                break;
            case Brush::DESERT:
                text = _( "Desert Objects" );
                break;
            case Brush::DIRT:
                text = _( "Dirt Objects" );
                break;
            case Brush::WASTELAND:
                text = _( "Wasteland Objects" );
                break;
            case Brush::BEACH:
                text = _( "Beach Objects" );
                break;
            case Brush::TOWNS:
                text = _( "Towns" );
                break;
            case Brush::MONSTERS:
                text = _( "Monsters" );
                break;
            case Brush::HEROES:
                text = _( "Heroes" );
                break;
            case Brush::ARTIFACTS:
                text = _( "Artifacts" );
                break;
            case Brush::TREASURES:
                text = _( "Treasures" );
                break;
            default:
                // You have added a new object type?
                assert( 0 );
                break;
            }

            const fheroes2::Text terrainText( std::move( text ), fheroes2::FontType::smallWhite() );
            terrainText.draw( _rectInstrumentPanel.x + 72 - terrainText.width( 118 ) / 2, _rectInstrumentPanel.y + 135, display );
        }

        _buttonMagnify.draw();
        _buttonUndo.draw();
        _buttonNew.draw();
        _buttonSpecs.draw();
        _buttonFile.draw();
        _buttonSystem.draw();

        display.render( _rectInstrumentPanel );
    }

    fheroes2::GameMode EditorPanel::queueEventProcessing()
    {
        LocalEvent & le = LocalEvent::Get();
        fheroes2::GameMode res = fheroes2::GameMode::CANCEL;

        if ( le.MousePressLeft( _rectInstruments ) ) {
            for ( uint8_t i = 0; i < Instrument::INSTRUMENTS_COUNT; ++i ) {
                if ( le.MousePressLeft( _instrumentButtonsRect[i] ) ) {
                    if ( _instrumentButtons[i].drawOnPress() ) {
                        _selectedInstrument = i;
                        setRedraw();
                    }
                }
                else {
                    _instrumentButtons[i].drawOnRelease();
                }
            }
        }

        if ( _selectedInstrument == Instrument::TERRAIN || _selectedInstrument == Instrument::ERASE ) {
            for ( uint8_t i = 0; i < BrushSize::BRUSH_SIZE_COUNT; ++i ) {
                if ( le.MousePressLeft( _brushSizeButtonsRect[i] ) ) {
                    if ( _brushSizeButtons[i].drawOnPress() ) {
                        _selectedBrushSize = i;
                    }
                }
                else {
                    if ( i != _selectedBrushSize ) {
                        _brushSizeButtons[i].drawOnRelease();
                    }
                }
            }

            const auto brushSizeText = []( const int & brushSize ) {
                std::string text = _( "Draws objects in %{size} by %{size} square increments." );
                StringReplace( text, "%{size}", brushSize );
                return text;
            };

            if ( le.MousePressRight( _brushSizeButtonsRect[BrushSize::SMALL] ) ) {
                fheroes2::showStandardTextMessage( _( "Small Brush" ), brushSizeText( 1 ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( _brushSizeButtonsRect[BrushSize::MEDIUM] ) ) {
                fheroes2::showStandardTextMessage( _( "Medium Brush" ), brushSizeText( 2 ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( _brushSizeButtonsRect[BrushSize::LARGE] ) ) {
                fheroes2::showStandardTextMessage( _( "Large Brush" ), brushSizeText( 4 ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( _brushSizeButtonsRect[BrushSize::AREA] ) ) {
                fheroes2::showStandardTextMessage( _( "Area Fill" ), _( "Used to click and drag for clearing large areas." ), Dialog::ZERO );
            }
        }

        if ( _selectedInstrument == Instrument::TERRAIN ) {
            for ( uint8_t i = 0; i < Brush::TERRAIN_COUNT; ++i ) {
                if ( ( _selectedTerrain != i ) && le.MousePressLeft( _terrainButtonsRect[i] ) ) {
                    _selectedTerrain = i;
                    _redraw();
                }
            }

            const auto movePenaltyText = []( const std::string & rate ) {
                std::string text = _( "Costs %{rate} times normal movement for all heroes. (Pathfinding reduces or eliminates the penalty.)" );
                StringReplace( text, "%{rate}", rate );
                return text;
            };

            if ( le.MousePressRight( _terrainButtonsRect[Brush::WATER] ) ) {
                fheroes2::showStandardTextMessage( _( "Water" ), _( "Traversable only by boat." ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( _terrainButtonsRect[Brush::GRASS] ) ) {
                fheroes2::showStandardTextMessage( _( "Grass" ), _( "No special modifiers." ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( _terrainButtonsRect[Brush::SNOW] ) ) {
                fheroes2::showStandardTextMessage( _( "Snow" ), movePenaltyText( "1.5" ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( _terrainButtonsRect[Brush::SWAMP] ) ) {
                fheroes2::showStandardTextMessage( _( "Swamp" ), movePenaltyText( "1.75" ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( _terrainButtonsRect[Brush::LAVA] ) ) {
                fheroes2::showStandardTextMessage( _( "Lava" ), _( "No special modifiers." ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( _terrainButtonsRect[Brush::DESERT] ) ) {
                fheroes2::showStandardTextMessage( _( "Desert" ), movePenaltyText( "2" ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( _terrainButtonsRect[Brush::DIRT] ) ) {
                fheroes2::showStandardTextMessage( _( "Dirt" ), _( "No special modifiers." ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( _terrainButtonsRect[Brush::WASTELAND] ) ) {
                fheroes2::showStandardTextMessage( _( "Wasteland" ), movePenaltyText( "1.25" ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( _terrainButtonsRect[Brush::BEACH] ) ) {
                fheroes2::showStandardTextMessage( _( "Beach" ), movePenaltyText( "1.25" ), Dialog::ZERO );
            }
        }

        if ( _selectedInstrument == Instrument::OBJECT ) {
            for ( uint8_t i = 0; i < Brush::OBJECT_COUNT; ++i ) {
                if ( ( _selectedObject != i ) && le.MousePressLeft( _objectButtonsRect[i] ) ) {
                    _selectedObject = i;
                    _redraw();
                }
            }

            const auto terrainObjectsText = []( const std::string & terrain ) {
                std::string text = _( "Used to select objects most appropriate for use on %{terrain}." );
                StringReplace( text, "%{terrain}", terrain );
                return text;
            };

            if ( le.MousePressRight( _objectButtonsRect[Brush::WATER] ) ) {
                fheroes2::showStandardTextMessage( _( "Water Objects" ), terrainObjectsText( _( "water" ) ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( _objectButtonsRect[Brush::GRASS] ) ) {
                fheroes2::showStandardTextMessage( _( "Grass Objects" ), terrainObjectsText( _( "grass" ) ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( _objectButtonsRect[Brush::SNOW] ) ) {
                fheroes2::showStandardTextMessage( _( "Snow Objects" ), terrainObjectsText( _( "snow" ) ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( _objectButtonsRect[Brush::SWAMP] ) ) {
                fheroes2::showStandardTextMessage( _( "Swamp Objects" ), terrainObjectsText( _( "swamp" ) ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( _objectButtonsRect[Brush::LAVA] ) ) {
                fheroes2::showStandardTextMessage( _( "Lava Objects" ), terrainObjectsText( _( "lava" ) ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( _objectButtonsRect[Brush::DESERT] ) ) {
                fheroes2::showStandardTextMessage( _( "Desert Objects" ), terrainObjectsText( _( "desert" ) ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( _objectButtonsRect[Brush::DIRT] ) ) {
                fheroes2::showStandardTextMessage( _( "Dirt Objects" ), terrainObjectsText( _( "dirt" ) ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( _objectButtonsRect[Brush::WASTELAND] ) ) {
                fheroes2::showStandardTextMessage( _( "Wasteland Objects" ), terrainObjectsText( _( "wasteland" ) ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( _objectButtonsRect[Brush::BEACH] ) ) {
                fheroes2::showStandardTextMessage( _( "Beach Objects" ), terrainObjectsText( _( "beach" ) ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( _objectButtonsRect[Brush::TOWNS] ) ) {
                fheroes2::showStandardTextMessage( _( "Towns" ), _( "Used to place a town or castle." ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( _objectButtonsRect[Brush::MONSTERS] ) ) {
                fheroes2::showStandardTextMessage( _( "Monsters" ), _( "Used to place a monster group." ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( _objectButtonsRect[Brush::HEROES] ) ) {
                fheroes2::showStandardTextMessage( _( "Heroes" ), _( "Used to place a hero." ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( _objectButtonsRect[Brush::ARTIFACTS] ) ) {
                fheroes2::showStandardTextMessage( _( "Artifacts" ), _( "Used to place an artifact." ), Dialog::ZERO );
            }
            else if ( le.MousePressRight( _objectButtonsRect[Brush::TREASURES] ) ) {
                fheroes2::showStandardTextMessage( _( "Treasures" ), _( "Used to place a resource or treasure." ), Dialog::ZERO );
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
        else if ( le.MouseClickLeft( _rectUndo ) ) {
            fheroes2::showStandardTextMessage( _( "Warning!" ), "The Map Editor is still in development. This function is not available yet.", Dialog::OK );
        }
        else if ( le.MouseClickLeft( _rectNew ) ) {
            res = Editor::eventNewMap();
        }
        else if ( le.MouseClickLeft( _rectSpecs ) ) {
            // TODO: Make the scenario info editor.
            Dialog::GameInfo();
        }
        else if ( le.MouseClickLeft( _rectFile ) ) {
            res = Interface::Editor::eventFileDialog();
        }
        else if ( le.MouseClickLeft( _rectSystem ) ) {
            // Replace this with Editor options dialog.
            fheroes2::showSystemOptionsDialog();
        }
        if ( le.MousePressRight( _instrumentButtonsRect[Instrument::TERRAIN] ) ) {
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
            fheroes2::showStandardTextMessage( _( "Erase Mode" ), _( "Used to erase objects off the map." ), Dialog::ZERO );
        }
        else if ( le.MousePressRight( _rectMagnify ) ) {
            fheroes2::showStandardTextMessage( _( "Magnify" ), _( "Change between zoom and normal view." ), Dialog::ZERO );
        }
        else if ( le.MousePressRight( _rectUndo ) ) {
            fheroes2::showStandardTextMessage( _( "Undo" ), _( "Undo your last action. Press again to redo the action." ), Dialog::ZERO );
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
