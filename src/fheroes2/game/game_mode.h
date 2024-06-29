/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2024                                             *
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

namespace fheroes2
{
    enum class GameMode : int
    {
        CANCEL = 0,
        QUIT_GAME,
        MAIN_MENU,
        NEW_GAME,
        LOAD_GAME,
        HIGHSCORES_STANDARD,
        HIGHSCORES_CAMPAIGN,
        CREDITS,
        NEW_STANDARD,
        NEW_CAMPAIGN_SELECTION,
        NEW_SUCCESSION_WARS_CAMPAIGN,
        NEW_PRICE_OF_LOYALTY_CAMPAIGN,
        NEW_MULTI,
        NEW_HOT_SEAT,
        NEW_BATTLE_ONLY,
        LOAD_STANDARD,
        LOAD_CAMPAIGN,
        LOAD_MULTI,
        LOAD_HOT_SEAT,
        // Do NOT change the order of the below 6 entries!
        SELECT_SCENARIO_ONE_HUMAN_PLAYER,
        SELECT_SCENARIO_TWO_HUMAN_PLAYERS,
        SELECT_SCENARIO_THREE_HUMAN_PLAYERS,
        SELECT_SCENARIO_FOUR_HUMAN_PLAYERS,
        SELECT_SCENARIO_FIVE_HUMAN_PLAYERS,
        SELECT_SCENARIO_SIX_HUMAN_PLAYERS,
        START_GAME,
        SAVE_GAME,
        END_TURN,
        SELECT_CAMPAIGN_SCENARIO,
        COMPLETE_CAMPAIGN_SCENARIO,
        COMPLETE_CAMPAIGN_SCENARIO_FROM_LOAD_FILE,
        EDITOR_MAIN_MENU,
        EDITOR_NEW_MAP,
        EDITOR_LOAD_MAP
    };
}
