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
    // distance_t
    enum
    {
        // UNUSED = 0,
        VIEW_CASTLE = 1,
        VIEW_HEROES = 2,
        VIEW_TELESCOPE = 3,
        VIEW_OBSERVATION_TOWER = 4,
        VIEW_MAGI_EYES = 5
    };

    enum
    {
        EVENT_NONE,
        EVENT_BUTTON_NEWGAME,
        EVENT_BUTTON_LOADGAME,
        EVENT_BUTTON_HIGHSCORES,
        EVENT_BUTTON_CREDITS,
        EVENT_BUTTON_STANDARD,
        EVENT_BUTTON_CAMPAIGN,
        EVENT_BUTTON_MULTI,
        EVENT_BUTTON_SETTINGS,
        EVENT_BUTTON_SELECT,
        EVENT_BUTTON_HOTSEAT,
        EVENT_BUTTON_HOST,
        EVENT_BUTTON_GUEST,
        EVENT_BUTTON_BATTLEONLY,
        EVENT_DEFAULT_READY,
        EVENT_DEFAULT_EXIT,
        EVENT_DEFAULT_LEFT,
        EVENT_DEFAULT_RIGHT,
        EVENT_SYSTEM_FULLSCREEN,
        EVENT_SYSTEM_SCREENSHOT,
        EVENT_SLEEPHERO,
        EVENT_ENDTURN,
        EVENT_NEXTHERO,
        EVENT_NEXTTOWN,
        EVENT_CONTINUE,
        EVENT_SAVEGAME,
        EVENT_LOADGAME,
        EVENT_FILEOPTIONS,
        EVENT_PUZZLEMAPS,
        EVENT_INFOGAME,
        EVENT_DIGARTIFACT,
        EVENT_CASTSPELL,
        EVENT_KINGDOM_INFO,
        EVENT_VIEW_WORLD,
        EVENT_DEFAULTACTION,
        EVENT_OPENFOCUS,
        EVENT_SYSTEMOPTIONS,
        EVENT_BATTLE_CASTSPELL,
        EVENT_BATTLE_RETREAT,
        EVENT_BATTLE_SURRENDER,
        EVENT_BATTLE_AUTOSWITCH,
        EVENT_BATTLE_OPTIONS,
        EVENT_BATTLE_HARDSKIP,
        EVENT_BATTLE_SOFTSKIP,
        EVENT_MOVELEFT,
        EVENT_MOVERIGHT,
        EVENT_MOVETOP,
        EVENT_MOVEBOTTOM,
        EVENT_MOVETOPLEFT,
        EVENT_MOVETOPRIGHT,
        EVENT_MOVEBOTTOMLEFT,
        EVENT_MOVEBOTTOMRIGHT,
        EVENT_SCROLLLEFT,
        EVENT_SCROLLRIGHT,
        EVENT_SCROLLUP,
        EVENT_SCROLLDOWN,
        EVENT_CTRLPANEL,
        EVENT_SHOWRADAR,
        EVENT_SHOWBUTTONS,
        EVENT_SHOWSTATUS,
        EVENT_SHOWICONS,
        EVENT_STACKSPLIT_SHIFT,
        EVENT_STACKSPLIT_CTRL,
        EVENT_JOINSTACKS,
        EVENT_UPGRADE_TROOP,
        EVENT_DISMISS_TROOP,
        EVENT_TOWN_DWELLING_LEVEL_1,
        EVENT_TOWN_DWELLING_LEVEL_2,
        EVENT_TOWN_DWELLING_LEVEL_3,
        EVENT_TOWN_DWELLING_LEVEL_4,
        EVENT_TOWN_DWELLING_LEVEL_5,
        EVENT_TOWN_DWELLING_LEVEL_6,
        EVENT_TOWN_WELL,
        EVENT_TOWN_MARKETPLACE,
        EVENT_TOWN_MAGE_GUILD,
        EVENT_TOWN_SHIPYARD,
        EVENT_TOWN_THIEVES_GUILD,

        // town screen exclusive, not applied to build screen!
        EVENT_TOWN_TAVERN,
        EVENT_TOWN_JUMP_TO_BUILD_SELECTION,

        EVENT_WELL_BUY_ALL_CREATURES,

        EVENT_LAST,
    };

    bool HotKeyPressEvent( int );
    bool HotKeyHoldEvent( const int eventID );

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
    u32 GetViewDistance( u32 );
    u32 GetWhirlpoolPercent( void );
    u32 SelectCountPlayers( void );
    void ShowMapLoadingText( void );
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

#define HotKeyCloseWindow ( Game::HotKeyPressEvent( Game::EVENT_DEFAULT_EXIT ) || Game::HotKeyPressEvent( Game::EVENT_DEFAULT_READY ) )

#endif
