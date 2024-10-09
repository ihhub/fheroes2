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

#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <set>

#include "game_mode.h"
#include "ground.h"
#include "image.h"
#include "map_object_info.h"
#include "math_base.h"
#include "ui_button.h"

namespace Interface
{
    class EditorInterface;

    class EditorPanel
    {
    public:
        explicit EditorPanel( EditorInterface & interface_ );

        ~EditorPanel() = default;

        const fheroes2::Rect & getRect() const
        {
            return _rectEditorPanel;
        }

        fheroes2::Rect getBrushArea() const;

        int selectedGroundType() const
        {
            return _getGroundId( _selectedTerrain );
        }

        bool isTerrainEdit() const
        {
            return _selectedInstrument == Instrument::TERRAIN;
        }

        bool isDetailEdit() const
        {
            return _selectedInstrument == Instrument::DETAIL;
        }

        bool isRoadDraw() const
        {
            return _selectedInstrument == Instrument::ROAD;
        }

        bool isStreamDraw() const
        {
            return _selectedInstrument == Instrument::STREAM;
        }

        bool isEraseMode() const
        {
            return _selectedInstrument == Instrument::ERASE;
        }

        bool isObjectMode() const
        {
            return _selectedInstrument == Instrument::LANDSCAPE_OBJECTS || _selectedInstrument == Instrument::ADVENTURE_OBJECTS
                   || _selectedInstrument == Instrument::KINGDOM_OBJECTS || _selectedInstrument == Instrument::MONSTERS;
        }

        std::set<Maps::ObjectGroup> getEraseObjectGroups() const;

        bool showAreaSelectRect() const
        {
            return _selectedInstrument == Instrument::TERRAIN || _selectedInstrument == Instrument::STREAM || _selectedInstrument == Instrument::ROAD
                   || _selectedInstrument == Instrument::ERASE || isObjectMode() || _selectedInstrument == Instrument::DETAIL;
        }

        bool useMouseDragMovement() const
        {
            return !( ( _selectedInstrument == Instrument::TERRAIN || _selectedInstrument == Instrument::ERASE ) && _selectedBrushSize == BrushSize::AREA );
        }

        // Set Editor panel positions on screen.
        void setPos( const int32_t displayX, int32_t displayY );

        // Set flag to redraw Editor buttons panel on the next interface render.
        void setRedraw() const;

        fheroes2::GameMode queueEventProcessing();

        // Do not call this method directly, use Interface::Editor::redraw() instead.
        // The name of this method starts from _ on purpose to do not mix with other public methods.
        void _redraw() const;

        int32_t getSelectedObjectType() const;

        Maps::ObjectGroup getSelectedObjectGroup() const;

        void getTownObjectProperties( int32_t & type, int32_t & color ) const;
        void getMineObjectProperties( int32_t & type, int32_t & color ) const;

        static const char * getObjectGroupName( const Maps::ObjectGroup groupName );

    private:
        static int _getGroundId( const uint8_t brushId );

        static const char * _getTerrainTypeName( const uint8_t brushId )
        {
            return Maps::Ground::String( _getGroundId( brushId ) );
        }

        static const char * _getLandscapeObjectTypeName( const uint8_t brushId );
        static const char * _getAdventureObjectTypeName( const uint8_t brushId );
        static const char * _getKingdomObjectTypeName( const uint8_t brushId );
        static const char * _getEraseObjectTypeName( const uint32_t eraseObjectType );

        static int32_t _generateTownObjectProperties( const int32_t type, const int32_t color );
        static int32_t _generateMineObjectProperties( const int32_t type, const int32_t color );

        void _setCursor();

        void handleObjectMouseClick( const std::function<int( int )> & typeSelection );

        enum Instrument : uint8_t
        {
            // IMPORTANT. This enumeration corresponds with the order of instruments in original assets. Do not change this order.
            TERRAIN = 0U,
            LANDSCAPE_OBJECTS = 1U,
            DETAIL = 2U,
            ADVENTURE_OBJECTS = 3U,
            KINGDOM_OBJECTS = 4U,
            MONSTERS = 5U,
            STREAM = 6U,
            ROAD = 7U,
            ERASE = 8U,

            // The last element corresponds to the editor instruments count.
            INSTRUMENTS_COUNT = 9U
        };

        enum TerrainBrush : uint8_t
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

            // The last element corresponds to the editor terrain types count.
            TERRAIN_COUNT = 9U,
        };

        enum LandscapeObjectBrush : int8_t
        {
            MOUNTAINS = 0,
            ROCKS = 1,
            TREES = 2,
            WATER_OBJECTS = 3,
            LANDSCAPE_MISC = 4,

            LANDSCAPE_COUNT = 5,
        };

        enum AdventureObjectBrush : int8_t
        {
            ARTIFACTS = 0U,
            DWELLINGS = 1U,
            MINES = 2U,
            POWER_UPS = 3U,
            TREASURES = 4U,
            WATER_ADVENTURE = 5U,
            ADVENTURE_MISC = 6U,

            ADVENTURE_COUNT = 7U,
        };

        enum KingdomObjectBrush : int8_t
        {
            HEROES = 0U,
            TOWNS = 1U,

            KINGDOM_OBJECTS_COUNT = 2U,
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

        enum ObjectErasureType : uint8_t
        {
            ERASE_NONE = 0x00,
            // Terrain objects are objects that are placed in editor using a terrain palette in objects placing mode.
            ERASE_LANDSCAPE = 0x01,
            ERASE_ADVENTURE_NON_PICKABLE = 0x02,
            ERASE_TOWNS = 0x04,
            ERASE_ADVENTURE_PICKABLE = 0x08,
            ERASE_MONSTERS = 0x10,
            ERASE_HEROES = 0x20,
            ERASE_STREAMS = 0x40,
            ERASE_ROADS = 0x80,

            ERASE_ALL_OBJECTS
            = ERASE_LANDSCAPE | ERASE_ADVENTURE_NON_PICKABLE | ERASE_TOWNS | ERASE_ADVENTURE_PICKABLE | ERASE_MONSTERS | ERASE_HEROES | ERASE_STREAMS | ERASE_ROADS,
        };

        // This array represents the order of object-to-erase images on the erase tool panel (from left to right, from top to bottom).
        const std::array<uint32_t, 8> _eraseButtonObjectTypes{ ObjectErasureType::ERASE_LANDSCAPE, ObjectErasureType::ERASE_ADVENTURE_NON_PICKABLE,
                                                               ObjectErasureType::ERASE_TOWNS,     ObjectErasureType::ERASE_ADVENTURE_PICKABLE,
                                                               ObjectErasureType::ERASE_MONSTERS,  ObjectErasureType::ERASE_HEROES,
                                                               ObjectErasureType::ERASE_ROADS,     ObjectErasureType::ERASE_STREAMS };

        EditorInterface & _interface;

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

        fheroes2::Image _instrumentPanelBackground;

        std::array<fheroes2::Button, Instrument::INSTRUMENTS_COUNT> _instrumentButtons;
        std::array<fheroes2::Button, BrushSize::BRUSH_SIZE_COUNT> _brushSizeButtons;

        std::array<fheroes2::Rect, Instrument::INSTRUMENTS_COUNT> _instrumentButtonsRect;
        std::array<fheroes2::Rect, TerrainBrush::TERRAIN_COUNT> _terrainButtonsRect;
        std::array<fheroes2::Rect, LandscapeObjectBrush::LANDSCAPE_COUNT> _landscapeObjectButtonsRect;
        std::array<fheroes2::Rect, AdventureObjectBrush::ADVENTURE_COUNT> _adventureObjectButtonsRect;
        std::array<fheroes2::Rect, KingdomObjectBrush::KINGDOM_OBJECTS_COUNT> _kingdomObjectButtonsRect;
        std::array<fheroes2::Rect, BrushSize::BRUSH_SIZE_COUNT> _brushSizeButtonsRect;
        std::array<fheroes2::Rect, 8> _eraseButtonsRect;

        uint8_t _selectedInstrument{ Instrument::TERRAIN };

        // A brand new map is always filled with Water so there is no need to make Water terrain brush as a default terrain selection.
        uint8_t _selectedTerrain{ TerrainBrush::GRASS };
        int8_t _selectedLandscapeObject{ -1 };
        int8_t _selectedAdventureObject{ -1 };
        int8_t _selectedKingdomObject{ -1 };
        uint8_t _selectedBrushSize{ BrushSize::MEDIUM };
        uint32_t _eraseTypes{ ObjectErasureType::ERASE_ALL_OBJECTS };

        std::array<int32_t, LandscapeObjectBrush::LANDSCAPE_COUNT> _selectedLandscapeObjectType;
        std::array<int32_t, AdventureObjectBrush::ADVENTURE_COUNT> _selectedAdventureObjectType;
        std::array<int32_t, KingdomObjectBrush::KINGDOM_OBJECTS_COUNT> _selectedKingdomObjectType;
        int32_t _selectedMonsterType{ -1 };

        static const std::array<Maps::ObjectGroup, LandscapeObjectBrush::LANDSCAPE_COUNT> _selectedLandscapeObjectGroup;
        static const std::array<Maps::ObjectGroup, AdventureObjectBrush::ADVENTURE_COUNT> _selectedAdventureObjectGroup;
        static const std::array<Maps::ObjectGroup, KingdomObjectBrush::KINGDOM_OBJECTS_COUNT> _selectedKingdomObjectGroup;
    };
}
