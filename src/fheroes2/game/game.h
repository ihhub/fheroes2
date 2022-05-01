/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
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

#ifndef H2GAME_H
#define H2GAME_H

#include <string>

#include "agg.h"
#include "game_mode.h"
#include "mp2.h"
#include "mus.h"
#include "types.h"

class Players;
class Heroes;
class Castle;

namespace Game
{
    void Init( void );

    const std::string & GetLastSavename( void );
    void SetLastSavename( const std::string & );
    void SetLoadVersion( uint16_t ver );
    uint16_t GetLoadVersion();

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

    enum HotKeyEvent : int32_t
    {
        NONE,

        MAIN_MENU_NEW_GAME,
        MAIN_MENU_LOAD_GAME,
        MAIN_MENU_HIGHSCORES,
        MAIN_MENU_CREDITS,
        MAIN_MENU_STANDARD,
        MAIN_MENU_CAMPAIGN,
        MAIN_MENU_MULTI,
        MAIN_MENU_SETTINGS,
        MAIN_MENU_SELECT_MAP,
        MAIN_MENU_HOTSEAT,
        MAIN_MENU_BATTLEONLY,
        MAIN_MENU_NEW_CAMPAIGN_SELECTION_SUCCESSION_WARS,
        MAIN_MENU_NEW_CAMPAIGN_SELECTION_PRICE_OF_LOYALTY,

        NEW_ROLAND_CAMPAIGN,
        NEW_ARCHIBALD_CAMPAIGN,
        NEW_PRICE_OF_LOYALTY_CAMPAIGN,
        NEW_VOYAGE_HOME_CAMPAIGN,
        NEW_WIZARDS_ISLE_CAMPAIGN,
        NEW_DESCENDANTS_CAMPAIGN,

        DEFAULT_READY,
        DEFAULT_EXIT,
        DEFAULT_LEFT,
        DEFAULT_RIGHT,
        MOVE_LEFT,
        MOVE_RIGHT,
        MOVE_TOP,
        MOVE_BOTTOM,
        MOVE_TOP_LEFT,
        MOVE_TOP_RIGHT,
        MOVE_BOTTOM_LEFT,
        MOVE_BOTTOM_RIGHT,

        SYSTEM_FULLSCREEN,

        SLEEP_HERO,
        END_TURN,
        NEXT_HERO,
        NEXT_TOWN,
        CONTINUE,
        SAVE_GAME,
        FILE_OPTIONS,
        PUZZLE_MAP,
        INFO_GAME,
        DIG_ARTIFACT,
        CAST_SPELL,
        KINGDOM_INFO,
        VIEW_WORLD,
        DEFAULT_ACTION,
        OPEN_FOCUS,
        SYSTEM_OPTIONS,
        SCROLL_LEFT,
        SCROLL_RIGHT,
        SCROLL_UP,
        SCROLL_DOWN,
        CONTROL_PANEL,
        SHOW_RADAR,
        SHOW_BUTTONS,
        SHOW_STATUS,
        SHOW_ICONS,

        BATTLE_RETREAT,
        BATTLE_SURRENDER,
        BATTLE_AUTOSWITCH,
        BATTLE_OPTIONS,
        BATTLE_SKIP,
        BATTLE_WAIT,

        SPLIT_STACK_BY_HALF,
        SPLIT_STACK_BY_ONE,
        JOIN_STACKS,
        UPGRADE_TROOP,
        DISMISS_TROOP,

        TOWN_DWELLING_LEVEL_1,
        TOWN_DWELLING_LEVEL_2,
        TOWN_DWELLING_LEVEL_3,
        TOWN_DWELLING_LEVEL_4,
        TOWN_DWELLING_LEVEL_5,
        TOWN_DWELLING_LEVEL_6,
        TOWN_WELL,
        TOWN_MARKETPLACE,
        TOWN_MAGE_GUILD,
        TOWN_SHIPYARD,
        TOWN_THIEVES_GUILD,

        // town screen exclusive, not applied to build screen!
        TOWN_TAVERN,
        TOWN_JUMP_TO_BUILD_SELECTION,

        WELL_BUY_ALL_CREATURES,

        // WARNING! Put all new event only above this line. No adding in between.
        NO_EVENT,
    };

    bool HotKeyPressEvent( const HotKeyEvent eventID );
    bool HotKeyHoldEvent( const HotKeyEvent eventID );

    std::string getHotKeyNameByEventId( const HotKeyEvent eventID );

    void mainGameLoop( bool isFirstGameRun );

    fheroes2::GameMode MainMenu( bool isFirstGameRun );
    fheroes2::GameMode NewGame();
    fheroes2::GameMode LoadGame();
    fheroes2::GameMode HighScores();
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
    fheroes2::GameMode ScenarioInfo();
    fheroes2::GameMode SelectCampaignScenario( const fheroes2::GameMode prevMode, const bool allowToRestart );
    fheroes2::GameMode SelectScenario();
    fheroes2::GameMode StartGame();
    fheroes2::GameMode StartBattleOnly();
    fheroes2::GameMode DisplayLoadGameDialog();
    fheroes2::GameMode CompleteCampaignScenario( const bool isLoadingSaveFile );

    bool isSuccessionWarsCampaignPresent();
    bool isPriceOfLoyaltyCampaignPresent();

    void EnvironmentSoundMixer();
    void restoreSoundsForCurrentFocus();
    int GetKingdomColors( void );
    int GetActualKingdomColors( void );
    void DialogPlayers( int color, std::string );
    void SetCurrentMusic( const int mus );
    int CurrentMusic();
    u32 & MapsAnimationFrame( void );
    u32 GetRating( void );
    u32 GetGameOverScores( void );
    u32 GetLostTownDays( void );
    u32 GetWhirlpoolPercent( void );
    u32 SelectCountPlayers( void );
    void PlayPickupSound( void );
    bool UpdateSoundsOnFocusUpdate();
    void SetUpdateSoundsOnFocusUpdate( bool update );
    void OpenHeroesDialog( Heroes & hero, bool updateFocus, bool windowIsGameWorld, bool disableDismiss = false );
    void OpenCastleDialog( Castle & castle, bool updateFocus = true );
    // Returns the difficulty level based on the type of game.
    int getDifficulty();
    void LoadPlayers( const std::string & mapFileName, Players & players );
    void saveDifficulty( const int difficulty );
    void SavePlayers( const std::string & mapFileName, const Players & players );

    std::string GetSaveDir();
    std::string GetSaveFileExtension();
    std::string GetSaveFileExtension( const int gameType );

    // Useful for restoring background music after playing short-term music effects
    class MusicRestorer
    {
    public:
        MusicRestorer()
            : _music( CurrentMusic() )
        {}

        MusicRestorer( const MusicRestorer & ) = delete;

        ~MusicRestorer()
        {
            if ( _music == MUS::UNUSED || _music == MUS::UNKNOWN ) {
                SetCurrentMusic( _music );

                return;
            }

            // Set current music to MUS::UNKNOWN to prevent attempts to play the old music
            // by new instances of MusicRestorer while the music being currently restored
            // is starting in the background
            if ( _music != CurrentMusic() ) {
                SetCurrentMusic( MUS::UNKNOWN );
            }

            AGG::PlayMusic( _music, true, true );
        }

        MusicRestorer & operator=( const MusicRestorer & ) = delete;

    private:
        const int _music;
    };

    namespace ObjectFadeAnimation
    {
        struct FadeTask
        {
            FadeTask();

            FadeTask( MP2::MapObjectType object_, uint32_t objectIndex_, uint32_t animationIndex_, int32_t fromIndex_, int32_t toIndex_, uint8_t alpha_, bool fadeOut_,
                      bool fadeIn_, uint8_t objectTileset_ );

            MP2::MapObjectType object;
            uint32_t objectIndex;
            uint32_t animationIndex;
            int32_t fromIndex;
            int32_t toIndex;
            uint8_t alpha;
            bool fadeOut;
            bool fadeIn;
            uint8_t objectTileset;
        };

        const FadeTask & GetFadeTask();

        void PrepareFadeTask( const MP2::MapObjectType object, int32_t fromTile, int32_t toTile, bool fadeOut, bool fadeIn );
        void PerformFadeTask();
    }

    int32_t GetStep4Player( const int32_t currentId, const int32_t width, const int32_t totalCount );
    std::string CountScoute( uint32_t count, int scoute, bool shorts = false );
    std::string CountThievesGuild( uint32_t monsterCount, int guildCount );
}

#define HotKeyCloseWindow ( Game::HotKeyPressEvent( Game::DEFAULT_EXIT ) || Game::HotKeyPressEvent( Game::DEFAULT_READY ) )

#endif
