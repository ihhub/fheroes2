/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2023 - 2026                                             *
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

#include <cstdint>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <utility>

#include "editor_interface_panel.h"
#include "game_mode.h"
#include "history_manager.h"
#include "interface_base.h"
#include "map_format_info.h"
#include "map_object_info.h"
#include "map_random_generator.h"
#include "timing.h"

enum class PlayerColor : uint8_t;

namespace Maps
{
    class Tile;
}

namespace Interface
{
    class EditorInterface final : public BaseInterface
    {
    public:
        static EditorInterface & Get();

        void redraw( const uint32_t force ) override;

        // Regenerates the game area and updates the panel positions depending on the UI settings
        void reset() override;

        // Start Map Editor interface main function.
        fheroes2::GameMode startEdit();

        static fheroes2::GameMode eventLoadMap();
        static fheroes2::GameMode eventNewMap();
        static fheroes2::GameMode eventFileDialog();
        void eventViewWorld();

        bool useMouseDragMovement() const override
        {
            return _editorPanel.useMouseDragMovement();
        }

        void mouseCursorAreaClickLeft( const int32_t tileIndex ) override;
        void mouseCursorAreaPressRight( const int32_t tileIndex ) const override;

        void mouseCursorAreaLongPressLeft( const int32_t /*Unused*/ ) override
        {
            // Do nothing.
        }

        void undoAction()
        {
            if ( _historyManager.undo() ) {
                _redraw |= ( REDRAW_GAMEAREA | REDRAW_RADAR );
            }
        }

        void redoAction()
        {
            if ( _historyManager.redo() ) {
                _redraw |= ( REDRAW_GAMEAREA | REDRAW_RADAR );
            }
        }

        void updateCursor( const int32_t tileIndex ) override;

        void setCursorUpdater( const std::function<void( const int32_t )> & cursorUpdater )
        {
            _cursorUpdater = cursorUpdater;
        }

        // Generate a random map and start Map Editor interface main function.
        bool generateRandomMap( const int32_t mapWidth );

        bool generateNewMap( const int32_t mapWidth );

        bool loadMap( const std::string & filePath );

        void saveMapToFile();

        void openMapSpecificationsDialog();

        bool updateRandomMapConfiguration( const int32_t mapWidth );

    private:
        class WarningMessage final
        {
        public:
            explicit WarningMessage( EditorInterface & interface )
                : _interface( interface )
            {
                // Do nothing.
            }

            void reset( std::string info )
            {
                _message = std::move( info );

                _interface.setRedraw( REDRAW_GAMEAREA );

                _timer.reset();
            }

            bool isValid() const
            {
                return _timer.getS() < 5 && !_message.empty();
            }

            std::string message() const
            {
                return _message;
            }

        private:
            EditorInterface & _interface;

            std::string _message;

            fheroes2::Time _timer;
        };

        struct MovableObjectInfo final
        {
            int32_t tileIndex{ -1 };
            int32_t objectType{ -1 };
            uint32_t objectUID{ 0 };
            Maps::ObjectGroup groupType{ Maps::ObjectGroup::NONE };
        };

        EditorInterface()
            : BaseInterface( true )
            , _editorPanel( *this )
            , _warningMessage( *this )
        {
            _historyManager.setStateCallback( [&editorPanel = _editorPanel]( const bool isUndoAvailable, const bool isRedoAvailable ) {
                editorPanel.updateUndoRedoButtonsStates( isUndoAvailable, isRedoAvailable );
            } );
        }

        bool _setObjectOnTile( Maps::Tile & tile, const Maps::ObjectGroup groupType, const int32_t objectIndex );

        void _handleObjectMouseLeftClick( Maps::Tile & tile );

        // Returns true if the placement was successful.
        // 'action' input argument will be populated in case of success.
        bool _tryToPlaceObject( Maps::Tile & tile, const int32_t objectType, const Maps::ObjectGroup groupType, const bool isNewObject,
                                std::unique_ptr<fheroes2::ActionCreator> & action );

        void _tryToMoveObject( const MovableObjectInfo & movableObjectInfo, const int32_t destinationTile );

        void _tryToCopyObject( const MovableObjectInfo & movableObjectInfo, const int32_t destinationTile );

        void _validateObjectsOnTerrainUpdate();

        // Returns true if an existing object was moved on top.
        bool _tryToMoveObjectOnTop( const int32_t tileIndex, const Maps::ObjectGroup groupType, int32_t objectIndex );

        void _updateObjectMetadata( const Maps::Map_Format::TileObjectInfo & object, const uint32_t newObjectUID );

        void _updateObjectUID( const uint32_t oldObjectUID, const uint32_t newObjectUID );

        bool _placeCastle( const int32_t posX, const int32_t posY, const PlayerColor color, const int32_t type );

        void _resetMovableObjectInfo();

        void _removeObjectsAsAction( std::set<uint32_t> objectUIDs, const std::set<Maps::ObjectGroup> & groups );

        EditorPanel _editorPanel;

        int32_t _areaSelectionStartTileId{ -1 };
        int32_t _tileUnderCursor{ -1 };

        std::set<int32_t> _brushTiles;

        MovableObjectInfo _movableObjectInfo;

        Maps::Random_Generator::Configuration _randomMapConfig;

        std::function<void( const int32_t )> _cursorUpdater;

        fheroes2::HistoryManager _historyManager;

        Maps::Map_Format::MapFormat _mapFormat;

        WarningMessage _warningMessage;

        std::string _loadedFileName;
    };
}
