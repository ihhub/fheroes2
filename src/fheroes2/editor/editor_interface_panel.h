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
        TERRAIN = 0U,
        OBJECT = 1U,
        DETAIL = 2U,
        STREAM = 3U,
        ROAD = 4U,
        ERASE = 5U,

        // The last element corresponds to the editor instruments count.
        INSTRUMENTS_COUNT = 6U
    };

    enum Brush : uint8_t
    {
        // IMPORTANT. This enumeration corresponds with the order of instruments in original assets. Do not change this order.
        WATER = 0U,
        GRASS = 1U,
        SNOW = 2U,
        SWAMP = 3U,
        LAVA = 4U,
        DESERT = 5U,
        DIRT = 6U,
        WASTELAND = 7U,
        BEACH = 8U,

        // This element corresponds to the editor terrain types count.
        TERRAIN_COUNT = 9U,

        // For objects this enumeration continues.
        TOWNS = 9U,
        MONSTERS = 10U,
        HEROES = 11U,
        ARTIFACTS = 12U,
        TREASURES = 13U,

        // The last element corresponds to the editor object types count.
        OBJECT_COUNT = 14U
    };

    enum BrushSize : uint8_t
    {
        // IMPORTANT. This enumeration corresponds with the order of brush size in original assets. Do not change this order.
        SMALL = 0U,
        MEDIUM = 1U,
        LARGE = 2U,
        AREA = 3U,

        // The last element corresponds to the editor brush size count.
        BRUSH_SIZE_COUNT = 4U
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

        static const char * _getTerrainTypeName( const uint8_t brushId );

        static const char * _getObjectTypeName( const uint8_t brushId );

        // Index of selected Map Editor instrument.
        uint8_t _selectedInstrument{ Instrument::TERRAIN };

        std::array<fheroes2::Button, Instrument::INSTRUMENTS_COUNT> _instrumentButtons;
        std::array<fheroes2::Rect, Instrument::INSTRUMENTS_COUNT> _instrumentButtonsRect;

        uint8_t _selectedTerrain{ Brush::WATER };
        uint8_t _selectedObject{ Brush::WATER };
        std::array<fheroes2::Rect, Brush::TERRAIN_COUNT> _terrainButtonsRect;
        std::array<fheroes2::Rect, Brush::OBJECT_COUNT> _objectButtonsRect;

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
