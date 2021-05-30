/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#ifndef H2SETTINGS_H
#define H2SETTINGS_H

#include <iomanip>
#include <list>

#include "bitmodes.h"
#include "dir.h"
#include "gamedefs.h"
#include "maps_fileinfo.h"
#include "players.h"

#define FORMAT_VERSION_094_RELEASE 9400
#define FORMAT_VERSION_093_RELEASE 9300
#define FORMAT_VERSION_091_RELEASE 9100
#define FORMAT_VERSION_090_RELEASE 9001

// TODO: once FORMAT_VERSION_094_RELEASE version becomes minimal supported please remove game version handling in HeaderSAV class and FileInfo structure.
#define LAST_SUPPORTED_FORMAT_VERSION FORMAT_VERSION_090_RELEASE

#define CURRENT_FORMAT_VERSION FORMAT_VERSION_094_RELEASE // TODO: update this value for a new release

enum
{
    SCROLL_SLOW = 4,
    SCROLL_NORMAL = 8,
    SCROLL_FAST1 = 16,
    SCROLL_FAST2 = 32
};

enum MusicSource
{
    MUSIC_MIDI_ORIGINAL,
    MUSIC_MIDI_EXPANSION,
    MUSIC_EXTERNAL,
    MUSIC_CDROM
};

class Settings
{
public:
    enum
    {
        GAME_AUTOSAVE_BEGIN_DAY = 0x10000010,
        GAME_REMEMBER_LAST_FOCUS = 0x10000020,
        GAME_SAVE_REWRITE_CONFIRM = 0x10000040,
        GAME_SHOW_SYSTEM_INFO = 0x10000100,
        GAME_AUTOSAVE_ON = 0x10000200,
        GAME_USE_FADE = 0x10000400,
        GAME_EVIL_INTERFACE = 0x10001000,
        GAME_HIDE_INTERFACE = 0x10002000,
        // UNUSED = 0x10008000,
        // UNUSED = 0x10010000,
        GAME_BATTLE_SHOW_DAMAGE = 0x10100000,
        GAME_CONTINUE_AFTER_VICTORY = 0x10200000,

        /* influence on game balance: save to savefile */
        WORLD_SHOW_VISITED_CONTENT = 0x20000001,
        // UNUSED = 0x20000002,
        WORLD_ALLOW_SET_GUARDIAN = 0x20000008,
        WORLD_ARTIFACT_CRYSTAL_BALL = 0x20000020,
        WORLD_SCOUTING_EXTENDED = 0x20000040,
        // UNUSED = 0x20000080,
        WORLD_EYE_EAGLE_AS_SCHOLAR = 0x20000100,
        HEROES_BUY_BOOK_FROM_SHRINES = 0x20000200,
        WORLD_BAN_WEEKOF = 0x20000400,
        WORLD_BAN_PLAGUES = 0x20000800,
        UNIONS_ALLOW_HERO_MEETINGS = 0x20001000,
        UNIONS_ALLOW_CASTLE_VISITING = 0x20002000,
        WORLD_SHOW_TERRAIN_PENALTY = 0x20004000,
        // UNUSED = 0x20008000,
        WORLD_BAN_MONTHOF_MONSTERS = 0x20010000,
        HEROES_TRANSCRIBING_SCROLLS = 0x20020000,
        // UNUSED = 0x20040000,
        CASTLE_ALLOW_GUARDIANS = 0x20080000,
        HEROES_COST_DEPENDED_FROM_LEVEL = 0x20800000,
        HEROES_REMEMBER_POINTS_RETREAT = 0x21000000,

        CASTLE_MAGEGUILD_POINTS_TURN = 0x30000001,
        WORLD_STARTHERO_LOSSCOND4HUMANS = 0x30000008,
        WORLD_1HERO_HIRED_EVERY_WEEK = 0x30000010,
        WORLD_SCALE_NEUTRAL_ARMIES = 0x30000020,
        HEROES_ARENA_ANY_SKILLS = 0x30000080,
        WORLD_USE_UNIQUE_ARTIFACTS_ML = 0x30000100,
        WORLD_USE_UNIQUE_ARTIFACTS_RS = 0x30000200,
        WORLD_USE_UNIQUE_ARTIFACTS_PS = 0x30000400,
        WORLD_USE_UNIQUE_ARTIFACTS_SS = 0x30000800,
        WORLD_DISABLE_BARROW_MOUNDS = 0x30001000,
        WORLD_EXT_OBJECTS_CAPTURED = 0x30004000,
        CASTLE_1HERO_HIRED_EVERY_WEEK = 0x30008000,

        BATTLE_SHOW_ARMY_ORDER = 0x40004000,
        // UNUSED = 0x40008000,
        BATTLE_SOFT_WAITING = 0x40010000,
        BATTLE_REVERSE_WAIT_ORDER = 0x40020000,
        BATTLE_SKIP_INCREASE_DEFENSE = 0x40200000
    };

    Settings( const Settings & ) = delete;

    Settings & operator=( const Settings & ) = delete;

    static Settings & Get();

    bool Read( const std::string & );
    bool Save( const std::string & ) const;

    std::string String() const;
    void SetCurrentFileInfo( const Maps::FileInfo & );
    const Maps::FileInfo & CurrentFileInfo() const;

    bool isCurrentMapPriceOfLoyalty() const;

    int Debug() const;
    int HeroesMoveSpeed() const;
    int AIMoveSpeed() const;
    int BattleSpeed() const;
    int ScrollSpeed() const;

    const std::string & SelectVideoDriver() const;

    int GameDifficulty() const;

    const std::string & MapsCharset() const;
    const std::string & ForceLang() const;
    const std::string & loadedFileLanguage() const;
    const std::string & FontsNormal() const;
    const std::string & FontsSmall() const;
    int FontsNormalSize() const;
    int FontsSmallSize() const;

    const fheroes2::Point & PosRadar() const;
    const fheroes2::Point & PosButtons() const;
    const fheroes2::Point & PosIcons() const;
    const fheroes2::Point & PosStatus() const;

    void SetPosRadar( const fheroes2::Point & );
    void SetPosButtons( const fheroes2::Point & );
    void SetPosIcons( const fheroes2::Point & );
    void SetPosStatus( const fheroes2::Point & );

    bool FullScreen() const;
    bool Sound() const;
    bool Music() const;
    bool ShowControlPanel() const;
    bool ShowRadar() const;
    bool ShowIcons() const;
    bool ShowButtons() const;
    bool ShowStatus() const;
    bool Unicode() const;
    bool BattleShowGrid() const;
    bool BattleShowMouseShadow() const;
    bool BattleShowMoveShadow() const;
    bool BattleAutoResolve() const;
    bool BattleAutoSpellcast() const;
    bool UseAltResource() const;
    bool isPriceOfLoyaltySupported() const;
    bool LoadedGameVersion() const;
    bool MusicExt() const;
    bool MusicMIDI() const;
    bool MusicCD() const;

    bool isFirstGameRun() const;
    void resetFirstGameRun();

    void BinarySave() const;
    void BinaryLoad();

    bool CanChangeInGame( u32 ) const;
    bool ExtModes( u32 ) const;
    void ExtSetModes( u32 );
    void ExtResetModes( u32 );
    const char * ExtName( u32 ) const;

    bool ExtHeroBuySpellBookFromShrine() const;
    bool ExtHeroRecruitCostDependedFromLevel() const;
    bool ExtHeroRememberPointsForRetreating() const;
    bool ExtHeroAllowTranscribingScroll() const;
    bool ExtHeroArenaCanChoiseAnySkills() const;
    bool ExtUnionsAllowCastleVisiting() const;
    bool ExtUnionsAllowHeroesMeetings() const;
    bool ExtWorldShowVisitedContent() const;
    bool ExtWorldShowTerrainPenalty() const;
    bool ExtWorldScouteExtended() const;
    bool ExtWorldAllowSetGuardian() const;
    bool ExtWorldArtifactCrystalBall() const;
    bool ExtWorldEyeEagleAsScholar() const;
    bool ExtWorldBanMonthOfMonsters() const;
    bool ExtWorldBanWeekOf() const;
    bool ExtWorldBanPlagues() const;
    bool ExtWorldStartHeroLossCond4Humans() const;
    bool ExtWorldOneHeroHiredEveryWeek() const;
    bool ExtWorldNeutralArmyDifficultyScaling() const;
    bool ExtWorldUseUniqueArtifactsML() const;
    bool ExtWorldUseUniqueArtifactsRS() const;
    bool ExtWorldUseUniqueArtifactsPS() const;
    bool ExtWorldUseUniqueArtifactsSS() const;
    bool ExtWorldExtObjectsCaptured() const;
    bool ExtWorldDisableBarrowMounds() const;
    bool ExtCastleAllowGuardians() const;
    bool ExtCastleGuildRestorePointsTurn() const;
    bool ExtCastleOneHeroHiredEveryWeek() const;
    bool ExtBattleShowDamage() const;
    bool ExtBattleShowBattleOrder() const;
    bool ExtBattleSoftWait() const;
    bool ExtBattleSkipIncreaseDefense() const;
    bool ExtBattleReverseWaitOrder() const;
    bool ExtGameRememberLastFocus() const;
    bool ExtGameContinueAfterVictory() const;
    bool ExtGameRewriteConfirm() const;
    bool ExtGameShowSystemInfo() const;
    bool ExtGameAutosaveBeginOfDay() const;
    bool ExtGameAutosaveOn() const;
    bool ExtGameUseFade() const;
    bool ExtGameEvilInterface() const;
    bool ExtGameHideInterface() const;

    const fheroes2::Size & VideoMode() const;

    void SetDebug( int );
    void SetUnicode( bool );
    void EnablePriceOfLoyaltySupport( const bool set );
    void SetGameDifficulty( int );
    void SetEvilInterface( bool );
    void SetHideInterface( bool );
    void SetBattleGrid( bool );
    void SetBattleMovementShaded( bool );
    void SetBattleMouseShaded( bool );
    void SetShowPanel( bool );
    void SetShowRadar( bool );
    void SetShowIcons( bool );
    void SetShowButtons( bool );
    void SetShowStatus( bool );
    void SetAIMoveSpeed( int );
    void SetScrollSpeed( int );
    void SetHeroesMoveSpeed( int );
    void SetBattleSpeed( int );
    void setBattleAutoResolve( bool enable );
    void setBattleAutoSpellcast( bool enable );
    void setFullScreen( const bool enable );

    void SetSoundVolume( int v );
    void SetMusicVolume( int v );
    void SetMusicType( int v );
    void ResetSound();
    void ResetMusic();

    int SoundVolume() const;
    int MusicVolume() const;
    MusicSource MusicType() const;

    bool IsGameType( int type ) const;
    int GameType() const;
    void SetGameType( int );
    bool isCampaignGameType() const;

    Players & GetPlayers();
    const Players & GetPlayers() const;

    int CurrentColor() const;
    void SetCurrentColor( int );
    int PreferablyCountPlayers() const;
    void SetPreferablyCountPlayers( int );

    // from maps info
    bool AllowChangeRace( int ) const;
    const std::string & MapsFile() const;
    const std::string & MapsName() const;
    const std::string & MapsDescription() const;
    const std::string & externalMusicCommand() const;
    int MapsDifficulty() const;
    fheroes2::Size MapsSize() const;
    bool GameStartWithHeroes() const;
    int ConditionWins() const;
    int ConditionLoss() const;
    bool WinsCompAlsoWins() const;
    int WinsFindArtifactID() const;
    bool WinsFindUltimateArtifact() const;
    u32 WinsAccumulateGold() const;
    fheroes2::Point WinsMapsPositionObject() const;
    fheroes2::Point LossMapsPositionObject() const;
    u32 LossCountDays() const;
    int controllerPointerSpeed() const;

    std::string GetProgramPath() const
    {
        return path_program;
    }
    void SetProgramPath( const char * );

    static std::string GetVersion();

    static ListFiles GetListFiles( const std::string & prefix, const std::string & filter );
    static ListDirs GetRootDirs();
    static std::string GetLastFile( const std::string & prefix, const std::string & name );
    static std::string GetWriteableDir( const char * );
    static std::string GetLangDir();

    static ListFiles FindFiles( const std::string & directory, const std::string & fileName );

    // deprecated
    const std::string & GetDataParams() const
    {
        return data_params;
    }
    ListDirs GetMapsParams() const
    {
        return maps_params;
    }

protected:
    void PostLoad();

private:
    friend StreamBase & operator<<( StreamBase &, const Settings & );
    friend StreamBase & operator>>( StreamBase &, Settings & );

    Settings();
    ~Settings();

    BitModes opt_global;
    BitModes opt_game;
    BitModes opt_battle;
    BitModes opt_world;
    BitModes opt_addons;

    int debug;
    fheroes2::Size video_mode;
    int game_difficulty;

    std::string path_program;
    std::string data_params;
    ListDirs maps_params;

    std::string font_normal;
    std::string font_small;
    std::string force_lang;
    std::string _loadedFileLanguage; // not a part of save or configuration file
    std::string _externalMusicCommand;
    std::string maps_charset;
    int size_normal;
    int size_small;

    Maps::FileInfo current_maps_file;

    int sound_volume;
    int music_volume;
    MusicSource _musicType;
    int _controllerPointerSpeed;
    int heroes_speed;
    int ai_speed;
    int scroll_speed;
    int battle_speed;

    int game_type;
    int preferably_count_players;

    std::string video_driver;

    fheroes2::Point pos_radr;
    fheroes2::Point pos_bttn;
    fheroes2::Point pos_icon;
    fheroes2::Point pos_stat;

    Players players;
};

StreamBase & operator<<( StreamBase &, const Settings & );
StreamBase & operator>>( StreamBase &, Settings & );

#endif
