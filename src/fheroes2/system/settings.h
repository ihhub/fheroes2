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
#include "campaign_data.h"
#include "dir.h"
#include "gamedefs.h"
#include "maps_fileinfo.h"
#include "players.h"
#include "system.h"

#define FORMAT_VERSION_090_RELEASE 9000
#define FORMAT_VERSION_082_RELEASE 8200
#define FORMAT_VERSION_080_RELEASE 8000
#define FORMAT_VERSION_070_RELEASE 3269
#define FORMAT_VERSION_3255 3255
#define LAST_FORMAT_VERSION FORMAT_VERSION_3255

#define CURRENT_FORMAT_VERSION FORMAT_VERSION_090_RELEASE // TODO: update this value for a new release

enum
{
    DBG_WARN = 0x0001,
    DBG_INFO = 0x0002,
    DBG_TRACE = 0x0003,

    DBG_ENGINE = 0x000C,
    DBG_GAME = 0x0030,
    DBG_BATTLE = 0x00C0,
    DBG_AI = 0x0300,
    DBG_NETWORK = 0x0C00,
    DBG_OTHER = 0x3000,
    DBG_DEVEL = 0xC000,

    DBG_ENGINE_WARN = 0x0004,
    DBG_GAME_WARN = 0x0010,
    DBG_BATTLE_WARN = 0x0040,
    DBG_AI_WARN = 0x0100,
    DBG_NETWORK_WARN = 0x0400,
    DBG_OTHER_WARN = 0x1000,

    DBG_ENGINE_INFO = 0x0008,
    DBG_GAME_INFO = 0x0020,
    DBG_BATTLE_INFO = 0x0080,
    DBG_AI_INFO = 0x0200,
    DBG_NETWORK_INFO = 0x0800,
    DBG_OTHER_INFO = 0x2000,

    DBG_ENGINE_TRACE = DBG_ENGINE,
    DBG_GAME_TRACE = DBG_GAME,
    DBG_BATTLE_TRACE = DBG_BATTLE,
    DBG_AI_TRACE = DBG_AI,
    DBG_NETWORK_TRACE = DBG_NETWORK,
    DBG_OTHER_TRACE = DBG_OTHER,

    DBG_ALL = DBG_ENGINE | DBG_GAME | DBG_BATTLE | DBG_AI | DBG_NETWORK | DBG_OTHER,

    DBG_ALL_WARN = DBG_ENGINE_WARN | DBG_GAME_WARN | DBG_BATTLE_WARN | DBG_AI_WARN | DBG_NETWORK_WARN | DBG_OTHER_WARN,
    DBG_ALL_INFO = DBG_ENGINE_INFO | DBG_GAME_INFO | DBG_BATTLE_INFO | DBG_AI_INFO | DBG_NETWORK_INFO | DBG_OTHER_INFO,
    DBG_ALL_TRACE = DBG_ENGINE_TRACE | DBG_GAME_TRACE | DBG_BATTLE_TRACE | DBG_AI_TRACE | DBG_NETWORK_TRACE | DBG_OTHER_TRACE
};

const char * StringDebug( int );

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

#ifdef WITH_DEBUG
#define DEBUG( x, y, z )                                                                                                                                                 \
    if ( IS_DEBUG( x, y ) ) {                                                                                                                                            \
        COUT( System::GetTime() << ": [" << StringDebug( x ) << "]\t" << __FUNCTION__ << ":  " << z );                                                                   \
    }
#else
#define DEBUG( x, y, z )
#endif
#define IS_DEVEL() IS_DEBUG( DBG_DEVEL, DBG_INFO )

bool IS_DEBUG( int name, int level );

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
        GAME_DYNAMIC_INTERFACE = 0x10010000,
        GAME_BATTLE_SHOW_DAMAGE = 0x10100000,
        GAME_CONTINUE_AFTER_VICTORY = 0x10200000,
        POCKETPC_TAP_MODE = 0x11000000,
        POCKETPC_DRAG_DROP_SCROLL = 0x12000000,
        POCKETPC_LOW_RESOLUTION = 0x14000000,

        /* influence on game balance: save to savefile */
        WORLD_SHOW_VISITED_CONTENT = 0x20000001,
        WORLD_ABANDONED_MINE_RANDOM = 0x20000002,
        WORLD_ALLOW_SET_GUARDIAN = 0x20000008,
        WORLD_ARTIFACT_CRYSTAL_BALL = 0x20000020,
        WORLD_SCOUTING_EXTENDED = 0x20000040,
        WORLD_ONLY_FIRST_MONSTER_ATTACK = 0x20000080,
        WORLD_EYE_EAGLE_AS_SCHOLAR = 0x20000100,
        HEROES_BUY_BOOK_FROM_SHRINES = 0x20000200,
        WORLD_BAN_WEEKOF = 0x20000400,
        WORLD_BAN_PLAGUES = 0x20000800,
        UNIONS_ALLOW_HERO_MEETINGS = 0x20001000,
        UNIONS_ALLOW_CASTLE_VISITING = 0x20002000,
        // UNUSED = 0x20004000,
        WORLD_BAN_MONTHOF_MONSTERS = 0x20010000,
        HEROES_TRANSCRIBING_SCROLLS = 0x20020000,
        WORLD_NEW_VERSION_WEEKOF = 0x20040000,
        CASTLE_ALLOW_GUARDIANS = 0x20080000,
        HEROES_ALLOW_BANNED_SECSKILLS = 0x20400000,
        HEROES_COST_DEPENDED_FROM_LEVEL = 0x20800000,
        HEROES_REMEMBER_POINTS_RETREAT = 0x21000000,
        HEROES_SURRENDERING_GIVE_EXP = 0x22000000,
        HEROES_RECALCULATE_MOVEMENT = 0x24000000,

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
        BATTLE_SKIP_INCREASE_DEFENSE = 0x40200000,
        BATTLE_OBJECTS_ARCHERS_PENALTY = 0x42000000,

        SETTINGS_LAST
    };

    static Settings & Get( void );

    bool Read( const std::string & );
    bool Save( const std::string & ) const;

    std::string String( void ) const;
    void SetCurrentFileInfo( const Maps::FileInfo & );
    const Maps::FileInfo & CurrentFileInfo( void ) const;

    int Debug( void ) const;
    int HeroesMoveSpeed( void ) const;
    int AIMoveSpeed( void ) const;
    int BattleSpeed( void ) const;
    int ScrollSpeed( void ) const;

    const std::string & SelectVideoDriver( void ) const;

    int GameDifficulty( void ) const;

    const std::string & MapsCharset( void ) const;
    const std::string & ForceLang( void ) const;
    const std::string & FontsNormal( void ) const;
    const std::string & FontsSmall( void ) const;
    int FontsNormalSize( void ) const;
    int FontsSmallSize( void ) const;
    bool FontSmallRenderBlended( void ) const;
    bool FontNormalRenderBlended( void ) const;

    const Point & PosRadar( void ) const;
    const Point & PosButtons( void ) const;
    const Point & PosIcons( void ) const;
    const Point & PosStatus( void ) const;

    void SetPosRadar( const Point & );
    void SetPosButtons( const Point & );
    void SetPosIcons( const Point & );
    void SetPosStatus( const Point & );

    bool FullScreen( void ) const;
    bool KeepAspectRatio( void ) const;
    bool ChangeFullscreenResolution( void ) const;
    bool Sound( void ) const;
    bool Music( void ) const;
    bool ShowControlPanel( void ) const;
    bool ShowRadar( void ) const;
    bool ShowIcons( void ) const;
    bool ShowButtons( void ) const;
    bool ShowStatus( void ) const;
    bool Unicode( void ) const;
    bool PocketPC( void ) const;
    bool BattleShowGrid( void ) const;
    bool BattleShowMouseShadow( void ) const;
    bool BattleShowMoveShadow( void ) const;
    bool UseAltResource( void ) const;
    bool PriceLoyaltyVersion( void ) const;
    bool LoadedGameVersion( void ) const;
    bool MusicExt( void ) const;
    bool MusicMIDI( void ) const;
    bool MusicCD( void ) const;
    void BinarySave( void ) const;
    void BinaryLoad( void );

    bool CanChangeInGame( u32 ) const;
    bool ExtModes( u32 ) const;
    void ExtSetModes( u32 );
    void ExtResetModes( u32 );
    const char * ExtName( u32 ) const;

    bool ExtHeroBuySpellBookFromShrine( void ) const;
    bool ExtHeroRecruitCostDependedFromLevel( void ) const;
    bool ExtHeroRememberPointsForRetreating( void ) const;
    bool ExtHeroSurrenderingGiveExp( void ) const;
    bool ExtHeroRecalculateMovement( void ) const;
    bool ExtHeroAllowTranscribingScroll( void ) const;
    bool ExtHeroAllowBannedSecSkillsUpgrade( void ) const;
    bool ExtHeroArenaCanChoiseAnySkills( void ) const;
    bool ExtUnionsAllowCastleVisiting( void ) const;
    bool ExtUnionsAllowHeroesMeetings( void ) const;
    bool ExtWorldShowVisitedContent( void ) const;
    bool ExtWorldScouteExtended( void ) const;
    bool ExtWorldAbandonedMineRandom( void ) const;
    bool ExtWorldAllowSetGuardian( void ) const;
    bool ExtWorldArtifactCrystalBall( void ) const;
    bool ExtWorldOnlyFirstMonsterAttack( void ) const;
    bool ExtWorldEyeEagleAsScholar( void ) const;
    bool ExtWorldBanMonthOfMonsters( void ) const;
    bool ExtWorldBanWeekOf( void ) const;
    bool ExtWorldNewVersionWeekOf( void ) const;
    bool ExtWorldBanPlagues( void ) const;
    bool ExtWorldStartHeroLossCond4Humans( void ) const;
    bool ExtWorldOneHeroHiredEveryWeek( void ) const;
    bool ExtWorldNeutralArmyDifficultyScaling( void ) const;
    bool ExtWorldUseUniqueArtifactsML( void ) const;
    bool ExtWorldUseUniqueArtifactsRS( void ) const;
    bool ExtWorldUseUniqueArtifactsPS( void ) const;
    bool ExtWorldUseUniqueArtifactsSS( void ) const;
    bool ExtWorldExtObjectsCaptured( void ) const;
    bool ExtWorldDisableBarrowMounds( void ) const;
    bool ExtCastleAllowGuardians( void ) const;
    bool ExtCastleGuildRestorePointsTurn( void ) const;
    bool ExtCastleOneHeroHiredEveryWeek( void ) const;
    bool ExtBattleShowDamage( void ) const;
    bool ExtBattleShowBattleOrder( void ) const;
    bool ExtBattleSoftWait( void ) const;
    bool ExtBattleObjectsArchersPenalty( void ) const;
    bool ExtBattleSkipIncreaseDefense( void ) const;
    bool ExtBattleReverseWaitOrder( void ) const;
    bool ExtGameRememberLastFocus( void ) const;
    bool ExtGameContinueAfterVictory( void ) const;
    bool ExtGameRewriteConfirm( void ) const;
    bool ExtGameShowSystemInfo( void ) const;
    bool ExtGameAutosaveBeginOfDay( void ) const;
    bool ExtGameAutosaveOn( void ) const;
    bool ExtGameUseFade( void ) const;
    bool ExtGameEvilInterface( void ) const;
    bool ExtGameDynamicInterface( void ) const;
    bool ExtGameHideInterface( void ) const;
    bool ExtPocketTapMode( void ) const;
    bool ExtPocketDragDropScroll( void ) const;

    const Size & VideoMode( void ) const;

    void SetDebug( int );
    void SetUnicode( bool );
    void SetPriceLoyaltyVersion( bool set = true );
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

    void SetSoundVolume( int v );
    void SetMusicVolume( int v );
    void SetMusicType( int v );
    void ResetSound( void );
    void ResetMusic( void );

    int SoundVolume( void ) const;
    int MusicVolume( void ) const;
    MusicSource MusicType() const;

    bool IsGameType( int type ) const;
    int GameType( void ) const;
    void SetGameType( int );

    void SetCurrentCampaignScenarioBonus( const Campaign::ScenarioBonusData & bonus );
    void SetCurrentCampaignScenarioID( const int scenarioID );
    void SetCurrentCampaignID( const int campaignID );
    void AddCurrentCampaignMapToFinished();

    Players & GetPlayers( void );
    const Players & GetPlayers( void ) const;

    int CurrentColor( void ) const;
    void SetCurrentColor( int );
    int PreferablyCountPlayers( void ) const;
    void SetPreferablyCountPlayers( int );

    int GetPort( void ) const;

    // from maps info
    bool AllowChangeRace( int ) const;
    const std::string & MapsFile( void ) const;
    const std::string & MapsName( void ) const;
    const std::string & MapsDescription( void ) const;
    int MapsDifficulty( void ) const;
    Size MapsSize( void ) const;
    bool GameStartWithHeroes( void ) const;
    int ConditionWins( void ) const;
    int ConditionLoss( void ) const;
    bool WinsCompAlsoWins( void ) const;
    bool WinsAllowNormalVictory( void ) const;
    int WinsFindArtifactID( void ) const;
    bool WinsFindUltimateArtifact( void ) const;
    u32 WinsAccumulateGold( void ) const;
    Point WinsMapsPositionObject( void ) const;
    Point LossMapsPositionObject( void ) const;
    u32 LossCountDays( void ) const;

    std::string GetProgramPath( void ) const
    {
        return path_program;
    }
    void SetProgramPath( const char * );

    static std::string GetVersion( void );

    static ListFiles GetListFiles( const std::string & prefix, const std::string & filter );
    static ListDirs GetRootDirs( void );
    static std::string GetLastFile( const std::string & prefix, const std::string & name );
    static std::string GetWriteableDir( const char * );
    static std::string GetLangDir( void );

    // deprecated
    const std::string & GetDataParams( void ) const
    {
        return data_params;
    }
    const ListDirs GetMapsParams( void ) const
    {
        return maps_params;
    }

protected:
    void Parse( const std::string & left, const std::string & right );
    void PostLoad( void );

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
    Size video_mode;
    int game_difficulty;

    std::string path_program;
    std::string data_params;
    ListDirs maps_params;

    std::string font_normal;
    std::string font_small;
    std::string force_lang;
    std::string maps_charset;
    int size_normal;
    int size_small;

    Maps::FileInfo current_maps_file;

    int sound_volume;
    int music_volume;
    MusicSource _musicType;
    int heroes_speed;
    int ai_speed;
    int scroll_speed;
    int battle_speed;

    int game_type;
    int preferably_count_players;

    std::string video_driver;

    int port;

    Point pos_radr;
    Point pos_bttn;
    Point pos_icon;
    Point pos_stat;

    Players players;

    Campaign::CampaignData campaignData;
};

StreamBase & operator<<( StreamBase &, const Settings & );
StreamBase & operator>>( StreamBase &, Settings & );

#endif
