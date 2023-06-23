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

#include <array>
#include <cstdint>

#include "game_interface.h"
#include "game_mode.h"
#include "math_base.h"
#include "ui_button.h"

namespace Interface
{
    class Editor;

    class EditorPanel
    {
    public:
        explicit EditorPanel( Editor & interface_ );

        ~EditorPanel() = default;

        const fheroes2::Rect & getRect() const
        {
            return _rectEditorPanel;
        }

        void setPos( const int32_t displayX, int32_t displayY );
        void setRedraw() const;

        fheroes2::GameMode queueEventProcessing();

    private:
        friend class Editor;

        // Do not call this method directly, use Interface::Basic::Redraw() instead
        // to avoid issues in the "no interface" mode
        void _redraw() const;

        Editor & _interface;

        uint32_t _selectedInstrument{ 0 };

        std::array<fheroes2::Button, 6> _instrumentButtons;
        std::array<fheroes2::Rect, 6> _instrumentButtonsRect;

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

    class Editor : public AdventureMap
    {
    public:
        static Editor & Get();

        void Redraw( const uint32_t force = 0 );

        // Regenerates the game area and updates the panel positions depending on the UI settings
        void Reset();

        fheroes2::GameMode startEdit();
        static fheroes2::GameMode eventLoadMap();
        static fheroes2::GameMode eventNewMap();
        fheroes2::GameMode eventFileDialog() const;
        void EventViewWorld();

    private:
        Editor();

        EditorPanel _editorPanel;
    };
}
#endif
