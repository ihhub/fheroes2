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

#if defined( WITH_DEBUG )

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

#include "agg_image.h"
#include "dialog.h"
#include "dialog_system_options.h"
#include "editor_interface.h"
#include "game_interface.h"
#include "game_mode.h"
#include "icn.h"
#include "image.h"
#include "interface_base.h"
#include "localevent.h"
#include "math_base.h"
#include "screen.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"

namespace Interface
{
    EditorPanel::EditorPanel( Editor & interface_ )
        : _interface( interface_ )
    {
        int32_t icnIndex = 0;

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

        _instrumentButtons.front().press();
    }

    void EditorPanel::setPos( const int32_t displayX, int32_t displayY )
    {
        int32_t offsetX = displayX;

        for ( size_t i = 0; i < _instrumentButtons.size(); ++i ) {
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

        // System buttons top row.
        displayY += _rectInstrumentPanel.height;
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
        _interface.setRedraw( REDRAW_BUTTONS );
    }

    void EditorPanel::_redraw() const
    {
        for ( const fheroes2::Button & button : _instrumentButtons ) {
            button.draw();
        }

        const fheroes2::Sprite & instrumentPanel = fheroes2::AGG::GetICN( ICN::EDITPANL, _selectedInstrument );
        fheroes2::Copy( instrumentPanel, 0, 0, fheroes2::Display::instance(), _rectEditorPanel.x, _rectInstruments.y + _rectInstruments.height, instrumentPanel.width(),
                        instrumentPanel.height() );

        _buttonMagnify.draw();
        _buttonUndo.draw();
        _buttonNew.draw();
        _buttonSpecs.draw();
        _buttonFile.draw();
        _buttonSystem.draw();
    }

    fheroes2::GameMode EditorPanel::queueEventProcessing()
    {
        LocalEvent & le = LocalEvent::Get();
        fheroes2::GameMode res = fheroes2::GameMode::CANCEL;

        if ( le.MousePressLeft( _rectInstruments ) ) {
            for ( uint32_t i = 0; i < _instrumentButtons.size(); ++i ) {
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

        le.MousePressLeft( _rectMagnify ) ? _buttonMagnify.drawOnPress() : _buttonMagnify.drawOnRelease();
        le.MousePressLeft( _rectUndo ) ? _buttonUndo.drawOnPress() : _buttonUndo.drawOnRelease();
        le.MousePressLeft( _rectNew ) ? _buttonNew.drawOnPress() : _buttonNew.drawOnRelease();
        le.MousePressLeft( _rectSpecs ) ? _buttonSpecs.drawOnPress() : _buttonSpecs.drawOnRelease();
        le.MousePressLeft( _rectFile ) ? _buttonFile.drawOnPress() : _buttonFile.drawOnRelease();
        le.MousePressLeft( _rectSystem ) ? _buttonSystem.drawOnPress() : _buttonSystem.drawOnRelease();

        if ( le.MouseClickLeft( _rectMagnify ) ) {
            _interface.EventViewWorld();
        }
        else if ( le.MouseClickLeft( _rectUndo ) ) {
            fheroes2::showStandardTextMessage( _( "Warning!" ), "The Map Editor is still in development. This function is not available yet.", Dialog::OK );
        }
        else if ( le.MouseClickLeft( _rectNew ) ) {
            res = Editor::eventNewMap();
        }
        else if ( le.MouseClickLeft( _rectSpecs ) ) {
            res = AdventureMap::EventScenarioInformation();
        }
        else if ( le.MouseClickLeft( _rectFile ) ) {
            res = Interface::Editor::eventFileDialog();
        }
        else if ( le.MouseClickLeft( _rectSystem ) ) {
            // Replace this with Editor options dialog.
            fheroes2::showSystemOptionsDialog();
        }

        if ( le.MousePressRight( _rectInstrumentPanel ) || le.MouseClickLeft( _rectInstrumentPanel ) ) {
            fheroes2::showStandardTextMessage( _( "Warning!" ), "The Map Editor is still in development. This panel is currently not functional.",
                                               le.MousePressRight() ? Dialog::ZERO : Dialog::OK );
        }

        if ( le.MousePressRight( _instrumentButtonsRect[0] ) ) {
            fheroes2::showStandardTextMessage( _( "Terrain Mode" ), _( "Used to draw the underlying grass, dirt, water, etc. on the map." ), Dialog::ZERO );
        }
        else if ( le.MousePressRight( _instrumentButtonsRect[1] ) ) {
            fheroes2::showStandardTextMessage( _( "Object Mode" ), _( "Used to place objects (mountains, trees, treasure, etc.) on the map." ), Dialog::ZERO );
        }
        else if ( le.MousePressRight( _instrumentButtonsRect[2] ) ) {
            fheroes2::showStandardTextMessage( _( "Detail Mode" ), _( "Used for special editing of monsters, heroes and towns." ), Dialog::ZERO );
        }
        else if ( le.MousePressRight( _instrumentButtonsRect[3] ) ) {
            fheroes2::showStandardTextMessage( _( "Stream Mode" ), _( "Allows you to draw streams by clicking and dragging." ), Dialog::ZERO );
        }
        else if ( le.MousePressRight( _instrumentButtonsRect[4] ) ) {
            fheroes2::showStandardTextMessage( _( "Road Mode" ), _( "Allows you to draw roads by clicking and dragging." ), Dialog::ZERO );
        }
        else if ( le.MousePressRight( _instrumentButtonsRect[5] ) ) {
            fheroes2::showStandardTextMessage( _( "Erase Mode" ), _( "Used to erase objects off the map." ), Dialog::ZERO );
        }
        else if ( le.MousePressRight( _rectMagnify ) ) {
            fheroes2::showStandardTextMessage( _( "Magnify" ), _( "Change between zoom and normal view." ), Dialog::ZERO );
        }
        else if ( le.MousePressRight( _rectUndo ) ) {
            fheroes2::showStandardTextMessage( _( "Undo" ), _( "Undo your last action.  Press again to redo the action." ), Dialog::ZERO );
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
#endif
