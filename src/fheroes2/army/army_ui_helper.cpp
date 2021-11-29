/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2021                                                    *
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

#include "army.h"
#include "army_ui_helper.h"
#include "skill.h"

/* draw MONS32 sprite in line, first valid = 0, count = 0 */
void fheroes2::DrawArmy::DrawMons32Line( const Troops & troops, s32 cx, s32 cy, u32 width, u32 first, u32 count )
{
    troops.DrawMons32Line( cx, cy, width, first, count, Skill::Level::EXPERT, false, true );
}

void fheroes2::DrawArmy::DrawMonsterLines( const Troops & troops, int32_t posX, int32_t posY, uint32_t lineWidth, uint32_t drawPower, bool compact, bool isScouteView )
{
    const uint32_t count = troops.GetCount();
    const int offsetX = lineWidth / 6;
    const int offsetY = compact ? 31 : 49;

    if ( count < 3 ) {
        troops.DrawMons32Line( posX + offsetX, posY + offsetY / 2 + 1, lineWidth * 2 / 3, 0, 0, drawPower, compact, isScouteView );
    }
    else {
        const int firstLineTroopCount = 2;
        const int secondLineTroopCount = count - firstLineTroopCount;
        const int secondLineWidth = secondLineTroopCount == 2 ? lineWidth * 2 / 3 : lineWidth;

        troops.DrawMons32Line( posX + offsetX, posY, lineWidth * 2 / 3, 0, firstLineTroopCount, drawPower, compact, isScouteView );
        troops.DrawMons32Line( posX, posY + offsetY, secondLineWidth, firstLineTroopCount, secondLineTroopCount, drawPower, compact, isScouteView );
    }
}
