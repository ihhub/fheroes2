/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
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

#ifndef H2SETTINGS_H
#define H2SETTINGS_H

#include <cstdint>
#include <string>
#include <vector>

#include "bitmodes.h"
#include "dir.h"
#include "maps_fileinfo.h"
#include "math_base.h"
#include "players.h"

class StreamBase;

enum AdventureMapScrollSpeed : int
{
    SCROLL_SPEED_NONE = 0,
    SCROLL_SPEED_SLOW = 1,
    SCROLL_SPEED_NORMAL = 2,
    SCROLL_SPEED_FAST = 3,
    SCROLL_SPEED_VERY_FAST = 4
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
    static constexpr const char * configFileName = "fheroes2.cfg";

    enum : uint32_t
    {
        // The following extended options do not affect the overall
        // game balance and are saved in the binary config file
        //
        // UNUSED = 0x10000010,
        // UNUSED = 0x10000020,
        // UNUSED = 0x10000040,
        // UNUSED = 0x10000100,
        // UNUSED = 0x10000200,
        // UNUSED = 0x10000400,
        GAME_EVIL_INTERFACE = 0x10001000,
        GAME_HIDE_INTERFACE = 0x10002000,
        // UNUSED = 0x10008000,
        // UNUSED = 0x10010000,
        // UNUSED = 0x10100000,
        // UNUSED = 0x10200000,

        // The following extended options affect the overall game balance and
        // are saved both in the binary config file and in the savefile
        //
        // TODO: combine them all into one bitset
        //
        // UNUSED = 0x20000001,
        // UNUSED = 0x20000002,
        // UNUSED = 0x20000008,
        // UNUSED = 0x20000020,
        // UNUSED = 0x20000040,
        // UNUSED = 0x20000080,
        // UNUSED = 0x20000100,
        // UNUSED = 0x20000200,
        // UNUSED = 0x20000400,
        // UNUSED = 0x20000800,
        // UNUSED = 0x20001000,
        // UNUSED = 0x20002000,
        // UNUSED = 0x20004000,
        // UNUSED = 0x20008000,
        // UNUSED = 0x20010000,
        // UNUSED = 0x20020000,
        // UNUSED = 0x20040000,
        // UNUSED = 0x20080000,
        // UNUSED = 0x20800000,
        // UNUSED = 0x21000000,

        // UNUSED = 0x30000001,
        // UNUSED = 0x30000008,
        // UNUSED = 0x30000010,
        // UNUSED = 0x30000020,
        HEROES_ARENA_ANY_SKILLS = 0x30000080,
        // UNUSED = 0x30000100,
        // UNUSED = 0x30000200,
        // UNUSED = 0x30000400,
        // UNUSED = 0x30000800,
        // UNUSED = 0x30001000,
        // UNUSED = 0x30004000,
        // UNUSED = 0x30008000,

        // UNUSED = 0x40004000,
        // UNUSED = 0x40008000,
        BATTLE_SOFT_WAITING = 0x40010000,
        // UNUSED = 0x40020000
    };

    Settings( const Settings & ) = delete;

    Settings & operator=( const Settings & ) = delete;

    static Settings & Get();

    bool Read( const std::string & );
    bool Save( const std::string & ) const;

    std::string String() const;
    void SetCurrentFileInfo( const Maps::FileInfo & );

    const Maps::FileInfo & CurrentFileInfo() const
    {
        return current_maps_file;
    }

    bool isCurrentMapPriceOfLoyalty() const
    {
        return current_maps_file._version == GameVersion::PRICE_OF_LOYALTY;
    }

    int Debug() const
    {
        return debug;
    }

    int HeroesMoveSpeed() const
    {
        return heroes_speed;
    }

    int AIMoveSpeed() const
    {
        return ai_speed;
    }

    int BattleSpeed() const
    {
        return battle_speed;
    }

    int ScrollSpeed() const
    {
        return scroll_speed;
    }

    int GameDifficulty() const
    {
        return game_difficulty;
    }

    const std::string & getGameLanguage() const
    {
        return _gameLanguage;
    }

    const std::string & loadedFileLanguage() const
    {
        return _loadedFileLanguage;
    }

    const fheroes2::Point & PosRadar() const
    {
        return pos_radr;
    }

    const fheroes2::Point & PosButtons() const
    {
        return pos_bttn;
    }

    const fheroes2::Point & PosIcons() const
    {
        return pos_icon;
    }

    const fheroes2::Point & PosStatus() const
    {
        return pos_stat;
    }

    void SetPosRadar( const fheroes2::Point & pt )
    {
        pos_radr = pt;
    }

    void SetPosButtons( const fheroes2::Point & pt )
    {
        pos_bttn = pt;
    }

    void SetPosIcons( const fheroes2::Point & pt )
    {
        pos_icon = pt;
    }

    void SetPosStatus( const fheroes2::Point & pt )
    {
        pos_stat = pt;
    }

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
    bool BattleShowArmyOrder() const;
    bool isPriceOfLoyaltySupported() const;
    bool isMonochromeCursorEnabled() const;
    bool isTextSupportModeEnabled() const;
    bool is3DAudioEnabled() const;
    bool isSystemInfoEnabled() const;
    bool isBattleShowDamageInfoEnabled() const;

    bool LoadedGameVersion() const
    {
        // 0x80 value should be same as in Game::TYPE_LOADFILE enumeration value
        // This constant not used here, to not drag dependency on the game.h and game.cpp in compilation target.
        return ( game_type & 0x80 ) != 0;
    }

    bool MusicMIDI() const
    {
        return _musicType == MUSIC_MIDI_ORIGINAL || _musicType == MUSIC_MIDI_EXPANSION;
    }

    bool isShowIntro() const;

    bool isVSyncEnabled() const;

    bool isFirstGameRun() const;
    void resetFirstGameRun();

    bool CanChangeInGame( uint32_t ) const;
    bool ExtModes( uint32_t ) const;
    void ExtSetModes( uint32_t );
    void ExtResetModes( uint32_t );
    static std::string ExtName( const uint32_t settingId );

    bool ExtHeroArenaCanChoiseAnySkills() const
    {
        return ExtModes( HEROES_ARENA_ANY_SKILLS );
    }

    bool ExtBattleSoftWait() const
    {
        return ExtModes( BATTLE_SOFT_WAITING );
    }

    static bool ExtGameUseFade()
    {
        // TODO: fix fading effect for the original resolution (640 x 480) and enable back this option.
        // return video_mode == fheroes2::Size( fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT );
        return false;
    }

    bool ExtGameEvilInterface() const
    {
        return ExtModes( GAME_EVIL_INTERFACE );
    }

    bool ExtGameHideInterface() const
    {
        return ExtModes( GAME_HIDE_INTERFACE );
    }

    const fheroes2::Size & VideoMode() const
    {
        return video_mode;
    }

    void SetDebug( int );
    void EnablePriceOfLoyaltySupport( const bool set );

    void SetGameDifficulty( const int difficulty )
    {
        game_difficulty = difficulty;
    }

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
    void setBattleShowArmyOrder( const bool enable );
    void setFullScreen( const bool enable );
    void setMonochromeCursor( const bool enable );
    void setTextSupportMode( const bool enable );
    void set3DAudio( const bool enable );
    void setVSync( const bool enable );
    void setSystemInfo( const bool enable );
    void setBattleDamageInfo( const bool enable );

    void SetSoundVolume( int v );
    void SetMusicVolume( int v );

    void SetMusicType( int v )
    {
        _musicType = MUSIC_EXTERNAL <= v ? MUSIC_EXTERNAL : static_cast<MusicSource>( v );
    }

    bool setGameLanguage( const std::string & language );

    int SoundVolume() const
    {
        return sound_volume;
    }

    int MusicVolume() const
    {
        return music_volume;
    }

    MusicSource MusicType() const
    {
        return _musicType;
    }

    /* check game type */
    bool IsGameType( int type ) const
    {
        return ( game_type & type ) != 0;
    }

    int GameType() const
    {
        return game_type;
    }

    void SetGameType( int type )
    {
        game_type = type;
    }

    bool isCampaignGameType() const;

    Players & GetPlayers()
    {
        return players;
    }

    const Players & GetPlayers() const
    {
        return players;
    }

    int CurrentColor() const
    {
        return players.current_color;
    }

    void SetCurrentColor( int color )
    {
        players.current_color = color;
    }

    int PreferablyCountPlayers() const
    {
        return preferably_count_players;
    }

    void SetPreferablyCountPlayers( int );

    // from maps info
    bool AllowChangeRace( int f ) const
    {
        return ( current_maps_file.rnd_races & f ) != 0;
    }

    const std::string & MapsFile() const
    {
        return current_maps_file.file;
    }

    const std::string & MapsName() const
    {
        return current_maps_file.name;
    }

    const std::string & MapsDescription() const
    {
        return current_maps_file.description;
    }

    int MapsDifficulty() const
    {
        return current_maps_file.difficulty;
    }

    fheroes2::Size MapsSize() const
    {
        return { current_maps_file.size_w, current_maps_file.size_h };
    }

    bool GameStartWithHeroes() const
    {
        return current_maps_file.startWithHeroInEachCastle;
    }

    uint32_t ConditionWins() const
    {
        return current_maps_file.ConditionWins();
    }

    uint32_t ConditionLoss() const
    {
        return current_maps_file.ConditionLoss();
    }

    bool WinsCompAlsoWins() const
    {
        return current_maps_file.WinsCompAlsoWins();
    }

    int WinsFindArtifactID() const
    {
        return current_maps_file.WinsFindArtifactID();
    }

    bool WinsFindUltimateArtifact() const
    {
        return current_maps_file.WinsFindUltimateArtifact();
    }

    uint32_t getWinningGoldAccumulationValue() const
    {
        return current_maps_file.getWinningGoldAccumulationValue();
    }

    fheroes2::Point WinsMapsPositionObject() const
    {
        return current_maps_file.WinsMapsPositionObject();
    }

    fheroes2::Point LossMapsPositionObject() const
    {
        return current_maps_file.LossMapsPositionObject();
    }

    uint32_t LossCountDays() const
    {
        return current_maps_file.LossCountDays();
    }

    int controllerPointerSpeed() const
    {
        return _controllerPointerSpeed;
    }

    void SetMapsFile( const std::string & file )
    {
        current_maps_file.file = file;
    }

    void SetProgramPath( const char * );

    static std::string GetVersion();

    static const std::vector<std::string> & GetRootDirs();

    static ListFiles FindFiles( const std::string & prefixDir, const std::string & fileNameFilter, const bool exactMatch );

    static bool findFile( const std::string & internalDirectory, const std::string & fileName, std::string & fullPath );

    static std::string GetLastFile( const std::string & prefix, const std::string & name );

private:
    friend StreamBase & operator<<( StreamBase &, const Settings & );
    friend StreamBase & operator>>( StreamBase &, Settings & );

    Settings();
    ~Settings();

    void BinarySave() const;
    void BinaryLoad();

    // Global game options (GLOBAL_), they are saved in the text config file
    BitModes _optGlobal;
    // Extended options that do not affect the overall game balance (GAME_),
    // they are saved in the binary config file
    BitModes _optExtGame;
    // Extended options that affect the overall game balance, they are saved
    // both in the binary config file and in the savefile
    BitModes _optExtBalance2; // Options with codes starting with 0x2
    BitModes _optExtBalance3; // Options with codes starting with 0x3
    BitModes _optExtBalance4; // Options with codes starting with 0x4

    int debug;
    fheroes2::Size video_mode;
    int game_difficulty;

    std::string path_program;

    std::string _gameLanguage;
    // Not saved in the config file or savefile
    std::string _loadedFileLanguage;

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

    fheroes2::Point pos_radr{ -1, -1 };
    fheroes2::Point pos_bttn{ -1, -1 };
    fheroes2::Point pos_icon{ -1, -1 };
    fheroes2::Point pos_stat{ -1, -1 };

    Players players;
};

StreamBase & operator<<( StreamBase &, const Settings & );
StreamBase & operator>>( StreamBase &, Settings & );

#endif
