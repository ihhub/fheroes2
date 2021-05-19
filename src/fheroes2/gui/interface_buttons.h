/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
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

#include "interface_border.h"
#include "ui_button.h"

namespace Interface
{
    class Basic;

    class ButtonsArea : public BorderWindow
    {
    public:
        explicit ButtonsArea( Basic & );

        void SetPos( s32, s32 ) override;
        void SavePosition( void ) override;
        void SetRedraw( void ) const;

        void Redraw( void );
        fheroes2::GameMode QueueEventProcessing();
        void ResetButtons();

    private:
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

        void SetButtonStatus();
    };
}

#endif
