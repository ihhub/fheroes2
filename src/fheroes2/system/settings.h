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

#include "bitmodes.h"
#include "dir.h"
#include "maps_fileinfo.h"
#include "players.h"

enum : int
{
    SCROLL_SLOW = 1,
    SCROLL_NORMAL = 2,
    SCROLL_FAST1 = 3,
    SCROLL_FAST2 = 4
};

enum MusicSource
{
    MUSIC_MIDI_ORIGINAL,
    MUSIC_MIDI_EXPANSION,
    MUSIC_EXTERNAL
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
        // UNUSED = 0x20000001,
        // UNUSED = 0x20000002,
        WORLD_ALLOW_SET_GUARDIAN = 0x20000008,
        WORLD_ARTIFACT_CRYSTAL_BALL = 0x20000020,
        WORLD_SCOUTING_EXTENDED = 0x20000040,
        // UNUSED = 0x20000080,
        WORLD_EYE_EAGLE_AS_SCHOLAR = 0x20000100,
        HEROES_BUY_BOOK_FROM_SHRINES = 0x20000200,
        // UNUSED = 0x20000400,
        // UNUSED = 0x20000800,
        // UNUSED = 0x20001000,
        // UNUSED = 0x20002000,
        WORLD_SHOW_TERRAIN_PENALTY = 0x20004000,
        // UNUSED = 0x20008000,
        // UNUSED = 0x20010000,
        HEROES_TRANSCRIBING_SCROLLS = 0x20020000,
        // UNUSED = 0x20040000,
        CASTLE_ALLOW_GUARDIANS = 0x20080000,
        HEROES_COST_DEPENDED_FROM_LEVEL = 0x20800000,
        HEROES_REMEMBER_POINTS_RETREAT = 0x21000000,

        CASTLE_MAGEGUILD_POINTS_TURN = 0x30000001,
        // UNUSED = 0x30000008,
        // UNUSED = 0x30000010,
        WORLD_SCALE_NEUTRAL_ARMIES = 0x30000020,
        HEROES_ARENA_ANY_SKILLS = 0x30000080,
        // UNUSED = 0x30000100,
        WORLD_USE_UNIQUE_ARTIFACTS_RS = 0x30000200,
        WORLD_USE_UNIQUE_ARTIFACTS_PS = 0x30000400,
        WORLD_USE_UNIQUE_ARTIFACTS_SS = 0x30000800,
        WORLD_DISABLE_BARROW_MOUNDS = 0x30001000,
        WORLD_EXT_OBJECTS_CAPTURED = 0x30004000,
        // UNUSED = 0x30008000,

        BATTLE_SHOW_ARMY_ORDER = 0x40004000,
        BATTLE_DETERMINISTIC_RESULT = 0x40008000,
        BATTLE_SOFT_WAITING = 0x40010000,
        BATTLE_REVERSE_WAIT_ORDER = 0x40020000
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

    int GameDifficulty() const;

    const std::string & getGameLanguage() const;
    const std::string & loadedFileLanguage() const;

    const fheroes2::Point & PosRadar() const;
    const fheroes2::Point & PosButtons() const;
    const fheroes2::Point & PosIcons() const;
    const fheroes2::Point & PosStatus() const;

    void SetPosRadar( const fheroes2::Point & );
    void SetPosButtons( const fheroes2::Point & );
    void SetPosIcons( const fheroes2::Point & );
    void SetPosStatus( const fheroes2::Point & );

    bool FullScreen() const;
    bool ShowControlPanel() const;
    bool ShowRadar() const;
    bool ShowIcons() const;
    bool ShowButtons() const;
    bool ShowStatus() const;
    bool BattleShowGrid() const;
    bool BattleShowMouseShadow() const;
    bool BattleShowMoveShadow() const;
    bool BattleAutoResolve() const;
    bool BattleAutoSpellcast() const;
    bool isPriceOfLoyaltySupported() const;
    bool LoadedGameVersion() const;
    bool MusicMIDI() const;
    bool isShowIntro() const;

    bool isVSyncEnabled() const;

    bool isFirstGameRun() const;
    void resetFirstGameRun();

    void BinarySave() const;
    void BinaryLoad();

    bool CanChangeInGame( u32 ) const;
    bool ExtModes( u32 ) const;
    void ExtSetModes( u32 );
    void ExtResetModes( u32 );
    static std::string ExtName( const uint32_t settingId );

    bool ExtHeroBuySpellBookFromShrine() const;
    bool ExtHeroRecruitCostDependedFromLevel() const;
    bool ExtHeroRememberPointsForRetreating() const;
    bool ExtHeroAllowTranscribingScroll() const;
    bool ExtHeroArenaCanChoiseAnySkills() const;
    bool ExtWorldShowTerrainPenalty() const;
    bool ExtWorldScouteExtended() const;
    bool ExtWorldAllowSetGuardian() const;
    bool ExtWorldArtifactCrystalBall() const;
    bool ExtWorldEyeEagleAsScholar() const;
    bool ExtWorldNeutralArmyDifficultyScaling() const;
    bool ExtWorldUseUniqueArtifactsRS() const;
    bool ExtWorldUseUniqueArtifactsPS() const;
    bool ExtWorldUseUniqueArtifactsSS() const;
    bool ExtWorldExtObjectsCaptured() const;
    bool ExtWorldDisableBarrowMounds() const;
    bool ExtCastleAllowGuardians() const;
    bool ExtCastleGuildRestorePointsTurn() const;
    bool ExtBattleShowDamage() const;
    bool ExtBattleShowBattleOrder() const;
    bool ExtBattleSoftWait() const;
    bool ExtBattleDeterministicResult() const;
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

    bool setGameLanguage( const std::string & language );

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
    int MapsDifficulty() const;
    fheroes2::Size MapsSize() const;
    bool GameStartWithHeroes() const;
    uint32_t ConditionWins() const;
    uint32_t ConditionLoss() const;
    bool WinsCompAlsoWins() const;
    int WinsFindArtifactID() const;
    bool WinsFindUltimateArtifact() const;
    u32 WinsAccumulateGold() const;
    fheroes2::Point WinsMapsPositionObject() const;
    fheroes2::Point LossMapsPositionObject() const;
    u32 LossCountDays() const;
    int controllerPointerSpeed() const;

    void SetMapsFile( const std::string & file );

    std::string GetProgramPath() const
    {
        return path_program;
    }
    void SetProgramPath( const char * );

    static std::string GetVersion();

    static ListDirs GetRootDirs();

    static ListFiles FindFiles( const std::string & prefixDir, const std::string & fileNameFilter, const bool exactMatch );
    static std::string GetLastFile( const std::string & prefix, const std::string & name );

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

    std::string _gameLanguage;
    std::string _loadedFileLanguage; // not a part of save or configuration file

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

    fheroes2::Point pos_radr;
    fheroes2::Point pos_bttn;
    fheroes2::Point pos_icon;
    fheroes2::Point pos_stat;

    Players players;
};

StreamBase & operator<<( StreamBase &, const Settings & );
StreamBase & operator>>( StreamBase &, Settings & );

#endif
