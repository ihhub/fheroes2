/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2020                                                    *
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

#include "radio_buttons_group.h"

RadioButtonsGroup::RadioButtonsGroup( const Point & pos, uint32_t amount, uint32_t vSpacingStep )
{
    if ( amount == 0 ) {
        DEBUG( DBG_ENGINE, DBG_WARN, "passed amount of buttons for RadioButtonsGroup object is zero" );
        return;
    }

    for ( uint32_t i = 0; i < amount; ++i ) {
        u32 y = pos.y;
        if ( i != 0 ) {
            y += buttons[i - 1].h * i + vSpacingStep * i;
        }

        buttons.push_back( Button( pos.x, y, ICN::CAMPXTRG, 9, 8 ) );
    }

    SetActiveButton( 0 );
}

RadioButtonsGroup::~RadioButtonsGroup() {}

void RadioButtonsGroup::Draw()
{
    for ( uint32_t i = 0; i < buttons.size(); ++i ) {
        buttons[i].Draw();
    }
}

void RadioButtonsGroup::QueueEventProcessing()
{
    LocalEvent & le = LocalEvent::Get();

    for ( uint32_t i = 0; i < buttons.size(); ++i ) {
        if ( buttons[i].isEnable() && le.MousePressLeft( buttons[i] ) )
            SetActiveButton( i );
    }
}

void RadioButtonsGroup::SetActiveButton( uint32_t activeID )
{
    if ( activeID >= buttons.size() )
        return; // out of boundaries

    if ( activeID == active )
        return; // there is no need to do anything

    buttons[active].ReleaseDraw();
    buttons[activeID].PressDraw();
    active = activeID;
}

uint32_t RadioButtonsGroup::GetSelectedId( void )
{
    return active;
}
