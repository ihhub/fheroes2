/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
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
    class Basic;

    class ButtonsArea final : public BorderWindow
    {
    public:
        explicit ButtonsArea( Basic & );
        ButtonsArea( const ButtonsArea & ) = delete;

        ~ButtonsArea() override = default;

        ButtonsArea & operator=( const ButtonsArea & ) = delete;

        void SetPos( int32_t ox, int32_t oy ) override;
        void SavePosition() override;
        void SetRedraw() const;

        fheroes2::GameMode QueueEventProcessing();
        void ResetButtons();

    private:
        friend Basic;

        // Do not call this method directly, use Interface::Basic::Redraw() instead
        // to avoid issues in the "no interface" mode
        void Redraw();
        void SetButtonStatus();

        Basic & interface;

        fheroes2::Button buttonNextHero;
        fheroes2::Button buttonMovement;
        fheroes2::Button buttonKingdom;
        fheroes2::Button buttonSpell;
        fheroes2::Button buttonEndTurn;
        fheroes2::Button buttonAdventure;
        fheroes2::Button buttonFile;
        fheroes2::Button buttonSystem;

        fheroes2::Rect nextHeroRect;
        fheroes2::Rect movementRect;
        fheroes2::Rect kingdomRect;
        fheroes2::Rect spellRect;
        fheroes2::Rect endTurnRect;
        fheroes2::Rect adventureRect;
        fheroes2::Rect fileRect;
        fheroes2::Rect systemRect;
    };
}

#endif
