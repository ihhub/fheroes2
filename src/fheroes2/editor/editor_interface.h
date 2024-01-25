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

#include <cstdint>
#include <functional>
#include <string>

#include "editor_interface_panel.h"
#include "game_mode.h"
#include "history_manager.h"
#include "interface_base.h"
#include "map_format_info.h"
#include "timing.h"

namespace Maps
{
    class Tiles;

    enum class ObjectGroup : int32_t;
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
        fheroes2::GameMode startEdit( const bool isNewMap );

        static fheroes2::GameMode eventLoadMap();
        static fheroes2::GameMode eventNewMap();
        static fheroes2::GameMode eventFileDialog();
        void eventViewWorld();

        bool useMouseDragMovement() override
        {
            return _editorPanel.useMouseDragMovement();
        }

        void mouseCursorAreaClickLeft( const int32_t tileIndex ) override;
        void mouseCursorAreaPressRight( const int32_t tileIndex ) const override;

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

        bool loadMap( const std::string & filePath );

    private:
        class WarningMessage
        {
        public:
            explicit WarningMessage( EditorInterface & interface )
                : _interface( interface )
            {
                // Do nothing.
            }

            void reset( const char * info )
            {
                _message = info;

                _interface.setRedraw( REDRAW_GAMEAREA );

                _timer.reset();
            }

            bool isValid() const
            {
                return _timer.getS() < 5 && ( _message != nullptr );
            }

            const char * message() const
            {
                return _message;
            }

        private:
            EditorInterface & _interface;

            const char * _message{ nullptr };

            fheroes2::Time _timer;
        };

        EditorInterface()
            : BaseInterface( true )
            , _editorPanel( *this )
            , _warningMessage( *this )
        {
            // Do nothing.
        }

        bool setObjectOnTile( Maps::Tiles & tile, const Maps::ObjectGroup groupType, const int32_t objectIndex );

        void setObjectOnTileAsAction( Maps::Tiles & tile, const Maps::ObjectGroup groupType, const int32_t objectIndex );

        void handleObjectMouseLeftClick( Maps::Tiles & tile );

        EditorPanel _editorPanel;

        int32_t _selectedTile{ -1 };
        int32_t _tileUnderCursor{ -1 };

        std::function<void( const int32_t )> _cursorUpdater;

        fheroes2::HistoryManager _historyManager;

        Maps::Map_Format::MapFormat _mapFormat;

        WarningMessage _warningMessage;
    };
}
