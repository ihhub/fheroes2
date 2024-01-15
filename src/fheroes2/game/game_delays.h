/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2023                                             *
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

#ifndef H2GAME_DELAYS_H
#define H2GAME_DELAYS_H

#include <cstdint>
#include <vector>

namespace Game
{
    enum DelayType : int
    {
        SCROLL_DELAY = 0,
        SCROLL_START_DELAY,
        CURSOR_BLINK_DELAY,
        MAIN_MENU_DELAY,
        MAPS_DELAY,
        CASTLE_TAVERN_DELAY,
        CASTLE_AROUND_DELAY,
        CASTLE_BUYHERO_DELAY,
        CASTLE_BUILD_DELAY,
        CASTLE_UNIT_DELAY,
        HEROES_FADE_DELAY,
        HEROES_PICKUP_DELAY,
        PUZZLE_FADE_DELAY,
        BATTLE_DIALOG_DELAY,
        BATTLE_FRAME_DELAY,
        BATTLE_MISSILE_DELAY,
        BATTLE_SPELL_DELAY,
        BATTLE_DISRUPTING_DELAY,
        BATTLE_CATAPULT_DELAY,
        BATTLE_CATAPULT_BOULDER_DELAY,
        BATTLE_CATAPULT_CLOUD_DELAY,
        BATTLE_BRIDGE_DELAY,
        BATTLE_IDLE_DELAY,
        BATTLE_OPPONENTS_DELAY,
        BATTLE_FLAGS_DELAY,
        BATTLE_POPUP_DELAY,
        BATTLE_COLOR_CYCLE_DELAY,
        BATTLE_SELECTED_UNIT_DELAY,
        CUSTOM_BATTLE_UNIT_MOVEMENT_DELAY,

        CURRENT_HERO_DELAY,
        CURRENT_AI_DELAY,
        CUSTOM_DELAY,

        // IMPORTANT!!! All new entries must be put before this entry!
        LAST_DELAY
    };

    // If delay is passed reset it and return true. Otherwise return false. DelayType::CUSTOM_DELAY must not be passed!
    bool validateAnimationDelay( const DelayType delayType );

    // If custom delay (DelayType::CUSTOM_DELAY) is passed reset it and return true. Otherwise return false.
    bool validateCustomAnimationDelay( const uint64_t delayMs );

    // Explicitly pass delay. Useful for loops when first iterator must pass. DelayType::CUSTOM_DELAY must not be passed!
    void passAnimationDelay( const DelayType delayType );

    // Explicitly pass custom delay. Useful for loops when first iterator must pass.
    void passCustomAnimationDelay( const uint64_t delayMs );

    // Set the delay between frames of unit movement animation sequence on the battlefield.
    void setCustomUnitMovementDelay( const uint64_t delayMs );

    void AnimateResetDelay( const DelayType delayType );
    void UpdateGameSpeed();

    uint32_t ApplyBattleSpeed( uint32_t delay );

    // Returns the animation speed multiplier for a human-controlled hero.
    int HumanHeroAnimSpeedMultiplier();

    // Returns the animation speed multiplier for an AI-controlled hero.
    int AIHeroAnimSpeedMultiplier();

    // Returns true if every of delay type has passed.
    bool hasEveryDelayPassed( const std::vector<Game::DelayType> & delayTypes );

    // Returns true if every of delay type is not passed yet. DelayType::CUSTOM_DELAY must not be added in this function!
    bool isDelayNeeded( const std::vector<Game::DelayType> & delayTypes );

    bool isCustomDelayNeeded( const uint64_t delayMs );

    // Custom delay must never be called in this function.
    uint64_t getAnimationDelayValue( const DelayType delayType );
}

#endif
