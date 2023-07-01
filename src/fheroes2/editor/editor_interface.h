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

#if defined( WITH_DEBUG )

#include <cstdint>

#include "editor_interface_panel.h"
#include "game_mode.h"
#include "interface_base.h"

namespace Interface
{
    enum editorRedraw_t : uint32_t
    {
        // IMPORTANT: values 0x01 ... 0x10 are base for game/editor are declared in 'interface_base.h'. Do not use them here.

        REDRAW_PANEL = 0x20,
    };

    class Editor final : public BaseInterface
    {
    public:
        static Editor & Get();

        void redraw( const uint32_t force = 0 ) override;

        // Regenerates the game area and updates the panel positions depending on the UI settings
        void reset() override;

        // Start Map Editor interface main function.
        fheroes2::GameMode startEdit();

        static fheroes2::GameMode eventLoadMap();
        static fheroes2::GameMode eventNewMap();
        static fheroes2::GameMode eventFileDialog();
        void eventViewWorld();

        void mouseCursorAreaClickLeft( const int32_t tileIndex ) override;
        void mouseCursorAreaPressRight( const int32_t tileIndex ) const override;

    private:
        Editor();

        EditorPanel _editorPanel;
    };
}
#endif
