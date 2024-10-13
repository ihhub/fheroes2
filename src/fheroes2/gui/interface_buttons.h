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
        explicit ButtonsPanel( AdventureMap & );
        ButtonsPanel( const ButtonsPanel & ) = delete;

        ~ButtonsPanel() override = default;

        ButtonsPanel & operator=( const ButtonsPanel & ) = delete;

        void SetPos( int32_t x, int32_t y ) override;
        void SavePosition() override;
        void SetRedraw() const;

        fheroes2::GameMode QueueEventProcessing();

        // Do not call this method directly, use Interface::AdventureMap::redraw() instead to avoid issues in the "no interface" mode.
        // The name of this method starts from _ on purpose to do not mix with other public methods.
        void _redraw();

    private:
        void SetButtonStatus();

        AdventureMap & interface;

        fheroes2::Button buttonNextHero;
        fheroes2::Button buttonHeroMovement;
        fheroes2::Button buttonKingdom;
        fheroes2::Button buttonSpell;
        fheroes2::Button buttonEndTurn;
        fheroes2::Button buttonAdventure;
        fheroes2::Button buttonFile;
        fheroes2::Button buttonSystem;

        fheroes2::Rect nextHeroRect;
        fheroes2::Rect heroMovementRect;
        fheroes2::Rect kingdomRect;
        fheroes2::Rect spellRect;
        fheroes2::Rect endTurnRect;
        fheroes2::Rect adventureRect;
        fheroes2::Rect fileRect;
        fheroes2::Rect systemRect;
    };
}

#endif
