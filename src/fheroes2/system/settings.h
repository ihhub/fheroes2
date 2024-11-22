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
#include "screen.h"

class IStreamBase;
class OStreamBase;

inline constexpr int defaultBattleSpeed{ 4 };

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

enum class ZoomLevel : uint8_t
{
    ZoomLevel0 = 0,
    ZoomLevel1 = 1,
    ZoomLevel2 = 2,
    ZoomLevel3 = 3, // Max zoom, but should only exists for debug builds
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

    void setCurrentMapInfo( Maps::FileInfo fi );

    const Maps::FileInfo & getCurrentMapInfo() const
    {
        return _currentMapInfo;
    }

    Maps::FileInfo & getCurrentMapInfo()
    {
        return _currentMapInfo;
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
        return _gameDifficulty;
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
    bool BattleShowTurnOrder() const;
    bool isPriceOfLoyaltySupported() const;
    bool isMonochromeCursorEnabled() const;
    bool isTextSupportModeEnabled() const;
    bool is3DAudioEnabled() const;
    bool isSystemInfoEnabled() const;
    bool isAutoSaveAtBeginningOfTurnEnabled() const;
    bool isBattleShowDamageInfoEnabled() const;
    bool isHideInterfaceEnabled() const;
    bool isEvilInterfaceEnabled() const;

    bool isEditorAnimationEnabled() const;
    bool isEditorPassabilityEnabled() const;

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

    const fheroes2::ResolutionInfo & currentResolutionInfo() const
    {
        return _resolutionInfo;
    }

    void EnablePriceOfLoyaltySupport( const bool set );

    void SetGameDifficulty( const int difficulty )
    {
        _gameDifficulty = difficulty;
    }

    void SetBattleGrid( bool );
    void SetBattleMovementShaded( bool );
    void SetBattleMouseShaded( bool );
    void SetShowControlPanel( bool );
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
    void setBattleShowTurnOrder( const bool enable );
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
    void setScreenScalingTypeNearest( const bool enable );

    void SetSoundVolume( int v );
    void SetMusicVolume( int v );

    void SetMusicType( int v )
    {
        _musicType = MUSIC_EXTERNAL <= v ? MUSIC_EXTERNAL : static_cast<MusicSource>( v );
    }

    bool setGameLanguage( const std::string & language );

    void setEditorAnimation( const bool enable );
    void setEditorPassability( const bool enable );

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
        return players.getCurrentColor();
    }

    // The color should belong to one player or be NONE (neutral player).
    void SetCurrentColor( const int color )
    {
        players.setCurrentColor( color );
    }

    int controllerPointerSpeed() const
    {
        return _controllerPointerSpeed;
    }

    ZoomLevel ViewWorldZoomLevel() const
    {
        return _viewWorldZoomLevel;
    }

    void SetViewWorldZoomLevel( ZoomLevel zoomLevel )
    {
        _viewWorldZoomLevel = zoomLevel;
    }

    void SetProgramPath( const char * );

    static std::string GetVersion();

    static const std::vector<std::string> & GetRootDirs();

    static ListFiles FindFiles( const std::string & prefixDir, const std::string & fileNameFilter, const bool exactMatch );
    static bool findFile( const std::string & internalDirectory, const std::string & fileName, std::string & fullPath );
    static std::string GetLastFile( const std::string & prefix, const std::string & name );

private:
    friend OStreamBase & operator<<( OStreamBase & stream, const Settings & conf );
    friend IStreamBase & operator>>( IStreamBase & stream, Settings & conf );

    Settings();

    static void setDebug( int debug );

    // Game related options.
    BitModes _gameOptions;

    // Editor related options.
    BitModes _editorOptions;

    fheroes2::ResolutionInfo _resolutionInfo;
    int _gameDifficulty;

    std::string path_program;

    std::string _gameLanguage;
    // Not saved in the config file or savefile
    std::string _loadedFileLanguage;

    Maps::FileInfo _currentMapInfo;

    int sound_volume;
    int music_volume;
    MusicSource _musicType;
    int _controllerPointerSpeed;
    int heroes_speed;
    int ai_speed;
    int scroll_speed;
    int battle_speed;

    int game_type;
    ZoomLevel _viewWorldZoomLevel{ ZoomLevel::ZoomLevel1 };

    fheroes2::Point pos_radr{ -1, -1 };
    fheroes2::Point pos_bttn{ -1, -1 };
    fheroes2::Point pos_icon{ -1, -1 };
    fheroes2::Point pos_stat{ -1, -1 };

    Players players;
};

OStreamBase & operator<<( OStreamBase & stream, const Settings & conf );
IStreamBase & operator>>( IStreamBase & stream, Settings & conf );

#endif
