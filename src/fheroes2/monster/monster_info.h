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

#ifndef H2MONSTER_INFO_H
#define H2MONSTER_INFO_H

namespace Monster_Info
{
    enum ATTACK_DIR
    {
        TOP,
        FRONT,
        BOTTOM
    };

    enum ANIMATION_TYPE
    {
        NONE,
        STATIC,
        IDLE,
        MOVE_START,
        MOVING,
        MOVE_END,
        MOVE_QUICK,
        FLY_UP,
        FLY_LAND,
        MELEE_TOP,
        MELEE_TOP_END,
        MELEE_FRONT,
        MELEE_FRONT_END,
        MELEE_BOT,
        MELEE_BOT_END,
        RANG_TOP,
        RANG_TOP_END,
        RANG_FRONT,
        RANG_FRONT_END,
        RANG_BOT,
        RANG_BOT_END,
        WNCE, // combined UP and RETURN anim
        KILL,
        INVALID
    };
}
#endif
