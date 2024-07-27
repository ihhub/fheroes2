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

#include "interface_base.h"

#include "game.h"
#include "ui_tool.h"

namespace Interface
{
    void BaseInterface::renderWithFadeInOrPlanRender( const uint32_t redrawMask )
    {
        if ( Game::validateDisplayFadeIn() ) {
            redraw( redrawMask );

            fheroes2::fadeInDisplay();
        }
        else {
            setRedraw( redrawMask );
        }
    }

    void Interface::BaseInterface::validateFadeInAndRender()
    {
        if ( Game::validateDisplayFadeIn() ) {
            fheroes2::fadeInDisplay();

            setRedraw( REDRAW_GAMEAREA );
        }
        else {
            fheroes2::Display::instance().render();
        }
    }
}
