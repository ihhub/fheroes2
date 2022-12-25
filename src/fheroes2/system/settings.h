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
#include <string_view>
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

    Settings( const Settings & ) = delete;

    Settings & operator=( const Settings & ) = delete;

    ~Settings() = default;

    static Settings & Get();

    bool Read( const std::string & filePath );
    bool Save( const std::string_view fileName ) const;

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
    bool isAutoSaveAtBeginningOfTurnEnabled() const;
    bool isBattleShowDamageInfoEnabled() const;
    bool isHideInterfaceEnabled() const;
    bool isEvilInterfaceEnabled() const;

    static bool isFadeEffectEnabled()
    {
        // TODO: fix fading effect for the original resolution (640 x 480) and enable back this option.
        // return video_mode == fheroes2::Size( fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT );
        return false;
    }

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

    const fheroes2::Size & VideoMode() const
    {
        return video_mode;
    }

    const fheroes2::Point WindowPosition() const {
        return window_position;
    }

    void EnablePriceOfLoyaltySupport( const bool set );

    void SetGameDifficulty( const int difficulty )
    {
        game_difficulty = difficulty;
    }

    void SetBattleGrid( bool );
    void SetBattleMovementShaded( bool );
    void SetBattleMouseShaded( bool );
    void SetShowPanel( bool );
    void SetShowRadar( bool );
    void SetShowIcons( bool );
    void SetShowButtons( bool );
    void SetShowStatus( bool );
    // Sets the speed of AI-controlled heroes in the range 0 - 10, 0 means "don't show"
    void SetAIMoveSpeed( int );
    void SetScrollSpeed( int );
    // Sets the speed of human-controlled heroes in the range 1 - 10
    void SetHeroesMoveSpeed( int );
    // Sets the animation speed during combat in the range 1 - 10
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
    void setAutoSaveAtBeginningOfTurn( const bool enable );
    void setBattleDamageInfo( const bool enable );
    void setHideInterface( const bool enable );
    void setEvilInterface( const bool enable );
    void setNearestLinearScaling( const bool enable );

    void SetSoundVolume( int v );
    void SetMusicVolume( int v );

    void SetWindowPosition( int x, int y );

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

    static void setDebug( int debug );

    // Global game options (GLOBAL_)
    BitModes _optGlobal;

    fheroes2::Size video_mode;
    fheroes2::Point window_position{ 0, 0 };
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
