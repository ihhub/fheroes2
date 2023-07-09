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

#pragma once

#include <array>
#include <cstdint>

#include "game_mode.h"
#include "math_base.h"
#include "ui_button.h"

namespace Interface
{
    class Editor;

    enum Instrument : uint8_t
    {
        // IMPORTANT. This enumeration corresponds with the order of instruments in original assets. Do not change this order.
        TERRAIN,
        OBJECT,
        DETAIL,
        STREAM,
        ROAD,
        ERASE,

        // The last element corresponds to the editor instruments number.
        INSTRUMENTS_COUNT
    };

    enum Brush : uint8_t
    {
        // IMPORTANT. This enumeration corresponds with the order of instruments in original assets. Do not change this order.
        WATER,
        GRASS,
        SNOW,
        SWAMP,
        LAVA,
        DESERT,
        DIRT,
        WASTELAND,
        BEACH,
        TOWNS,
        MONSTERS,
        HEROES,
        ARTIFACTS,
        TREASURES,

        // The last element corresponds to the editor instruments number.
        BRUSHES_COUNT
    };

    enum BrushSize : uint8_t
    {
        // IMPORTANT. This enumeration corresponds with the order of instruments in original assets. Do not change this order.
        SMALL,
        MEDIUM,
        LARGE,
        AREA,

        // The last element corresponds to the editor instruments number.
        BRUSH_SIZE_COUNT
    };

    class EditorPanel
    {
    public:
        explicit EditorPanel( Editor & interface_ );

        ~EditorPanel() = default;

        const fheroes2::Rect & getRect() const
        {
            return _rectEditorPanel;
        }

        // Set Editor panel positions on screen.
        void setPos( const int32_t displayX, int32_t displayY );

        // Set flag to redraw Editor buttons panel on the next interface render.
        void setRedraw() const;

        fheroes2::GameMode queueEventProcessing();

        // Do not call this method directly, use Interface::Editor::redraw() instead.
        // The name of this method starts from _ on purpose to do not mix with other public methods.
        void _redraw() const;

    private:
        Editor & _interface;

        // Index of selected Map Editor instrument.
        uint8_t _selectedInstrument{ Instrument::TERRAIN };

        std::array<fheroes2::Button, Instrument::INSTRUMENTS_COUNT> _instrumentButtons;
        std::array<fheroes2::Rect, Instrument::INSTRUMENTS_COUNT> _instrumentButtonsRect;

        uint8_t _selectedBrush{ Brush::WATER };
        std::array<fheroes2::Rect, Brush::BRUSHES_COUNT> _brushButtonsRect;

        uint8_t _selectedBrushSize{ BrushSize::MEDIUM };
        std::array<fheroes2::Button, BrushSize::BRUSH_SIZE_COUNT> _brushSizeButtons;
        std::array<fheroes2::Rect, BrushSize::BRUSH_SIZE_COUNT> _brushSizeButtonsRect;

        fheroes2::Button _buttonMagnify;
        fheroes2::Button _buttonUndo;
        fheroes2::Button _buttonNew;
        fheroes2::Button _buttonSpecs;
        fheroes2::Button _buttonFile;
        fheroes2::Button _buttonSystem;

        fheroes2::Rect _rectMagnify;
        fheroes2::Rect _rectUndo;
        fheroes2::Rect _rectNew;
        fheroes2::Rect _rectSpecs;
        fheroes2::Rect _rectFile;
        fheroes2::Rect _rectSystem;

        fheroes2::Rect _rectInstruments;
        fheroes2::Rect _rectInstrumentPanel;
        fheroes2::Rect _rectEditorPanel;
    };
}
