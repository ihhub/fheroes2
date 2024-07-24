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

#ifndef H2GAME_H
#define H2GAME_H

#include <cstdint>
#include <string>

#include "game_mode.h"

class Players;
class Heroes;
class Castle;

namespace Game
{
    void Init();

    // type_t
    enum
    {
        TYPE_MENU = 0,
        TYPE_STANDARD = 0x01,
        TYPE_CAMPAIGN = 0x02,
        TYPE_HOTSEAT = 0x04,
        TYPE_NETWORK = 0x08,
        TYPE_BATTLEONLY = 0x10,

        // TYPE_LOADFILE used in the Settings::LoadedGameVersion, if you change that value,
        // change in that function as well.
        TYPE_LOADFILE = 0x80,
        TYPE_MULTI = TYPE_HOTSEAT
    };

    void mainGameLoop( bool isFirstGameRun, bool isProbablyDemoVersion );

    fheroes2::GameMode MainMenu( const bool isFirstGameRun );
    fheroes2::GameMode NewGame( const bool isProbablyDemoVersion );
    fheroes2::GameMode LoadGame();
    fheroes2::GameMode Credits();
    fheroes2::GameMode NewStandard();
    fheroes2::GameMode CampaignSelection();
    fheroes2::GameMode NewSuccessionWarsCampaign();
    fheroes2::GameMode NewPriceOfLoyaltyCampaign();
    fheroes2::GameMode NewMulti();
    fheroes2::GameMode NewHotSeat();
    fheroes2::GameMode NewBattleOnly();
    fheroes2::GameMode NewNetwork(); // To be utilized in future.
    fheroes2::GameMode LoadStandard();
    fheroes2::GameMode LoadCampaign();
    fheroes2::GameMode LoadMulti();
    fheroes2::GameMode LoadHotseat();
    fheroes2::GameMode SelectCampaignScenario( const fheroes2::GameMode prevMode, const bool allowToRestart );
    fheroes2::GameMode SelectScenario( const uint8_t humanPlayerCount );
    fheroes2::GameMode StartGame();
    fheroes2::GameMode StartBattleOnly();
    fheroes2::GameMode DisplayLoadGameDialog();
    fheroes2::GameMode CompleteCampaignScenario( const bool isLoadingSaveFile );
    fheroes2::GameMode DisplayHighScores( const bool isCampaign );

    bool isSuccessionWarsCampaignPresent();
    bool isPriceOfLoyaltyCampaignPresent();

    void EnvironmentSoundMixer();
    void restoreSoundsForCurrentFocus();

    bool UpdateSoundsOnFocusUpdate();
    void SetUpdateSoundsOnFocusUpdate( const bool update );

    bool isFadeInNeeded();
    // Set display fade-in state to true.
    void setDisplayFadeIn();
    // If display fade-in state is set reset it to false and return true. Otherwise return false.
    bool validateDisplayFadeIn();

    int GetKingdomColors();
    int GetActualKingdomColors();
    void DialogPlayers( int color, std::string title, std::string message );

    uint32_t getAdventureMapAnimationIndex();
    void updateAdventureMapAnimationIndex();

    uint32_t GetRating();
    uint32_t getGameOverScoreFactor();
    uint32_t GetLostTownDays();
    uint32_t GetWhirlpoolPercent();
    uint8_t SelectCountPlayers();

    void PlayPickupSound();

    void OpenHeroesDialog( Heroes & hero, bool updateFocus, const bool renderBackgroundDialog, const bool disableDismiss = false );
    void OpenCastleDialog( Castle & castle, bool updateFocus = true, const bool renderBackgroundDialog = true );

    void LoadPlayers( const std::string & mapFileName, Players & players );
    void SavePlayers( const std::string & mapFileName, const Players & players );

    // Returns true if the currently active game is a campaign, otherwise returns false
    bool isCampaign();

    // Returns the difficulty level of the currently active game, depending on its type (see the implementation for details)
    int getDifficulty();
    void saveDifficulty( const int difficulty );

    int32_t GetStep4Player( const int32_t currentId, const int32_t width, const int32_t totalCount );

    // Returns the string representation of the monster count. If a detailed view is requested, the exact number is returned
    // (unless the abbreviated number is requested), otherwise, a qualitative estimate is returned (Few, Several, etc).
    std::string formatMonsterCount( const uint32_t count, const bool isDetailedView, const bool abbreviateNumber = false );
}

#endif
