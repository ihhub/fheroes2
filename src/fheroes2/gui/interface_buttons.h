/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#ifndef H2INTERFACE_BUTTONS_H
#define H2INTERFACE_BUTTONS_H

#include <cstdint>

#include "game_mode.h"
#include "interface_border.h"
#include "math_base.h"
#include "ui_button.h"

namespace Interface
{
    class AdventureMap;

    class ButtonsPanel final : public BorderWindow
    {
    public:
        explicit ButtonsPanel( AdventureMap & baseInterface )
            : BorderWindow( { 0, 0, 144, 72 } )
            , _interface( baseInterface )
        {
            // Do nothing.
        }

        ButtonsPanel( const ButtonsPanel & ) = delete;

        ~ButtonsPanel() override = default;

        ButtonsPanel & operator=( const ButtonsPanel & ) = delete;

        void SetPos( int32_t x, int32_t y ) override;
        void SavePosition() override;
        void setRedraw() const;

        fheroes2::GameMode queueEventProcessing();

        // Do not call this method directly, use Interface::AdventureMap::redraw() instead to avoid issues in the "no interface" mode.
        // The name of this method starts from _ on purpose to do not mix with other public methods.
        void _redraw();

    private:
        void _setButtonStatus();

        AdventureMap & _interface;

        fheroes2::Button _buttonNextHero;
        fheroes2::Button _buttonHeroMovement;
        fheroes2::Button _buttonKingdom;
        fheroes2::Button _buttonSpell;
        fheroes2::Button _buttonEndTurn;
        fheroes2::Button _buttonAdventure;
        fheroes2::Button _buttonFile;
        fheroes2::Button _buttonSystem;

        fheroes2::Rect _nextHeroRect;
        fheroes2::Rect _heroMovementRect;
        fheroes2::Rect _kingdomRect;
        fheroes2::Rect _spellRect;
        fheroes2::Rect _endTurnRect;
        fheroes2::Rect _adventureRect;
        fheroes2::Rect _fileRect;
        fheroes2::Rect _systemRect;
    };
}

#endif
