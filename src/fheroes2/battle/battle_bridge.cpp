/***************************************************************************
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "castle.h"
#include "battle_cell.h"
#include "battle_troop.h"
#include "battle_interface.h"
#include "battle_bridge.h"

Battle::Bridge::Bridge() : destroy(false), down(false)
{
}

bool Battle::Bridge::isValid(void) const
{
    return !isDestroy();
}

bool Battle::Bridge::isDestroy(void) const
{
    return destroy;
}

bool Battle::Bridge::isDown(void) const
{
    return down || isDestroy();
}

void Battle::Bridge::SetDown(bool f)
{
    down = f;
}

bool Battle::Bridge::AllowUp(void) const
{
    return NULL == Board::GetCell(49)->GetUnit() && NULL == Board::GetCell(50)->GetUnit();
}

bool Battle::Bridge::NeedDown(const Unit & b, s32 pos2) const
{
    const s32 pos1 = b.GetHeadIndex();

    if(pos2 == 50)
    {
	if(pos1 == 51) return true;
	if((pos1 == 61 || pos1 == 39) && b.GetColor() == Arena::GetCastle()->GetColor()) return true;
    }
    else
    if(pos2 == 49)
    {
	if(pos1 != 50 && b.GetColor() == Arena::GetCastle()->GetColor()) return true;
    }

    return false;
}

bool Battle::Bridge::isPassable(int color) const
{
    return color == Arena::GetCastle()->GetColor() || isDown();
}

void Battle::Bridge::SetDestroy(void)
{
    destroy = true;
    Board::GetCell(49)->SetObject(0);
    Board::GetCell(50)->SetObject(0);
}

void Battle::Bridge::SetPassable(const Unit & b)
{
    if(Board::isCastleIndex(b.GetHeadIndex()) || b.GetColor() == Arena::GetCastle()->GetColor())
    {
	Board::GetCell(49)->SetObject(0);
	Board::GetCell(50)->SetObject(0);
    }
    else
    {
	Board::GetCell(49)->SetObject(1);
	Board::GetCell(50)->SetObject(1);
    }
}


bool Battle::Bridge::NeedAction(const Unit & b, s32 dst) const
{
    return (!isDown() && NeedDown(b, dst)) ||
	    (isValid() && isDown() && AllowUp());
}

void Battle::Bridge::Action(const Unit & b, s32 dst)
{
    bool action_down = false;

    if(!isDown() && NeedDown(b, dst))
	action_down = true;

    if(Arena::GetInterface())
	Arena::GetInterface()->RedrawBridgeAnimation(action_down);

    SetDown(action_down);
}
