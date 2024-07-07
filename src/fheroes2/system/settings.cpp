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

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <utility>

#if defined( MACOS_APP_BUNDLE )
#include <CoreFoundation/CoreFoundation.h>
#endif

#include "cursor.h"
#include "difficulty.h"
#include "game.h"
#include "game_io.h"
#include "gamedefs.h"
#include "logging.h"
#include "render_processor.h"
#include "save_format_version.h"
#include "screen.h"
#include "serialize.h"
#include "settings.h"
#include "system.h"
#include "tinyconfig.h"
#include "translations.h"
#include "ui_language.h"
#include "version.h"

#define STRINGIFY( DEF ) #DEF
#define EXPANDDEF( DEF ) STRINGIFY( DEF )

namespace
{
    enum GameOptions : uint32_t
    {
        GAME_FIRST_RUN = 0x00000001,
        GAME_SHOW_INTRO = 0x00000002,
        GAME_PRICELOYALTY = 0x00000004,
        GAME_RENDER_VSYNC = 0x00000008,
        GAME_TEXT_SUPPORT_MODE = 0x00000010,
        GAME_MONOCHROME_CURSOR = 0x00000020,
        GAME_SHOW_CONTROL_PANEL = 0x00000040,
        GAME_SHOW_RADAR = 0x00000080,
        GAME_SHOW_ICONS = 0x00000100,
        GAME_SHOW_BUTTONS = 0x00000200,
        GAME_SHOW_STATUS = 0x00000400,
        GAME_FULLSCREEN = 0x00008000,
        GAME_3D_AUDIO = 0x00010000,
        GAME_SYSTEM_INFO = 0x00020000,
        GAME_CURSOR_SOFT_EMULATION = 0x00040000,
        GAME_EVIL_INTERFACE = 0x00080000,
        GAME_HIDE_INTERFACE = 0x00100000,
        GAME_BATTLE_SHOW_DAMAGE = 0x00200000,
        GAME_BATTLE_SHOW_TURN_ORDER = 0x00400000,
        GAME_BATTLE_SHOW_GRID = 0x00800000,
        GAME_BATTLE_SHOW_MOUSE_SHADOW = 0x01000000,
        GAME_BATTLE_SHOW_MOVE_SHADOW = 0x02000000,
        GAME_BATTLE_AUTO_RESOLVE = 0x04000000,
        GAME_BATTLE_AUTO_SPELLCAST = 0x08000000,
        GAME_AUTO_SAVE_AT_BEGINNING_OF_TURN = 0x10000000,
        GAME_SCREEN_SCALING_TYPE_NEAREST = 0x20000000
    };

    enum EditorOptions : uint32_t
    {
        EDITOR_UNUSED = 0x00000001,
        EDITOR_ANIMATION = 0x00000002,
        EDITOR_PASSABILITY = 0x00000004
    };

    const int defaultSpeedDelay{ 5 };
}

std::string Settings::GetVersion()
{
    return std::to_string( MAJOR_VERSION ) + '.' + std::to_string( MINOR_VERSION ) + '.' + std::to_string( INTERMEDIATE_VERSION );
}

Settings::Settings()
    : _resolutionInfo( fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT )
    , _gameDifficulty( Difficulty::NORMAL )
    , sound_volume( 6 )
    , music_volume( 6 )
    , _musicType( MUSIC_EXTERNAL )
    , _controllerPointerSpeed( 10 )
    , heroes_speed( defaultSpeedDelay )
    , ai_speed( defaultSpeedDelay )
    , scroll_speed( SCROLL_SPEED_NORMAL )
    , battle_speed( DEFAULT_BATTLE_SPEED )
    , game_type( 0 )
{
    _gameOptions.SetModes( GAME_FIRST_RUN );
    _gameOptions.SetModes( GAME_SHOW_INTRO );

    _gameOptions.SetModes( GAME_SHOW_RADAR );
    _gameOptions.SetModes( GAME_SHOW_ICONS );
    _gameOptions.SetModes( GAME_SHOW_BUTTONS );
    _gameOptions.SetModes( GAME_SHOW_STATUS );

    _gameOptions.SetModes( GAME_BATTLE_SHOW_GRID );
    _gameOptions.SetModes( GAME_BATTLE_SHOW_MOUSE_SHADOW );
    _gameOptions.SetModes( GAME_BATTLE_SHOW_MOVE_SHADOW );
    _gameOptions.SetModes( GAME_BATTLE_AUTO_RESOLVE );

    _editorOptions.SetModes( EDITOR_ANIMATION );

    if ( System::isHandheldDevice() ) {
        // Due to the nature of handheld devices having small screens in general it is good to make fullscreen option by default.
        _gameOptions.SetModes( GAME_FULLSCREEN );

        // Adventure Map scrolling is disabled by default for handheld devices as it is very hard to navigate on small screens. Use drag and move logic.
        scroll_speed = SCROLL_SPEED_NONE;
    }

    // The Price of Loyalty is not supported by default.
    EnablePriceOfLoyaltySupport( false );
}

Settings & Settings::Get()
{
    static Settings conf;

    return conf;
}

bool Settings::Read( const std::string & filePath )
{
    TinyConfig config( '=', '#' );

    std::string sval;

    if ( !config.Load( filePath ) ) {
        return false;
    }

    // debug
    if ( config.Exists( "debug" ) ) {
        setDebug( config.IntParams( "debug" ) );
    }

    // game language
    sval = config.StrParams( "lang" );
    if ( !sval.empty() ) {
        _gameLanguage = sval;
    }

    // music source
    _musicType = MUSIC_EXTERNAL;
    sval = config.StrParams( "music" );

    if ( !sval.empty() ) {
        if ( sval == "original" ) {
            _musicType = MUSIC_MIDI_ORIGINAL;
        }
        else if ( sval == "expansion" ) {
            _musicType = MUSIC_MIDI_EXPANSION;
        }
        else if ( sval == "external" ) {
            _musicType = MUSIC_EXTERNAL;
        }
    }

    // sound volume
    if ( config.Exists( "sound volume" ) ) {
        SetSoundVolume( config.IntParams( "sound volume" ) );
    }

    // music volume
    if ( config.Exists( "music volume" ) ) {
        SetMusicVolume( config.IntParams( "music volume" ) );
    }

    // move speed
    if ( config.Exists( "ai speed" ) ) {
        SetAIMoveSpeed( config.IntParams( "ai speed" ) );
    }

    if ( config.Exists( "heroes speed" ) ) {
        SetHeroesMoveSpeed( config.IntParams( "heroes speed" ) );
    }

    // scroll speed
    SetScrollSpeed( config.IntParams( "scroll speed" ) );

    if ( config.Exists( "battle speed" ) ) {
        SetBattleSpeed( config.IntParams( "battle speed" ) );
    }

    if ( config.Exists( "battle grid" ) ) {
        SetBattleGrid( config.StrParams( "battle grid" ) == "on" );
    }

    if ( config.Exists( "battle shadow movement" ) ) {
        SetBattleMovementShaded( config.StrParams( "battle shadow movement" ) == "on" );
    }

    if ( config.Exists( "battle shadow cursor" ) ) {
        SetBattleMouseShaded( config.StrParams( "battle shadow cursor" ) == "on" );
    }

    if ( config.Exists( "battle show damage" ) ) {
        setBattleDamageInfo( config.StrParams( "battle show damage" ) == "on" );
    }

    if ( config.Exists( "auto resolve battles" ) ) {
        setBattleAutoResolve( config.StrParams( "auto resolve battles" ) == "on" );
    }

    if ( config.Exists( "auto spell casting" ) ) {
        setBattleAutoSpellcast( config.StrParams( "auto spell casting" ) == "on" );
    }

    if ( config.Exists( "battle turn order" ) ) {
        setBattleShowTurnOrder( config.StrParams( "battle turn order" ) == "on" );
    }

    if ( config.Exists( "use evil interface" ) ) {
        setEvilInterface( config.StrParams( "use evil interface" ) == "on" );
    }

    if ( config.Exists( "hide interface" ) ) {
        setHideInterface( config.StrParams( "hide interface" ) == "on" );
    }

    if ( config.Exists( "radar window position" ) ) {
        pos_radr = config.PointParams( "radar window position", { -1, -1 } );
    }

    if ( config.Exists( "buttons window position" ) ) {
        pos_bttn = config.PointParams( "buttons window position", { -1, -1 } );
    }

    if ( config.Exists( "icons window position" ) ) {
        pos_icon = config.PointParams( "icons window position", { -1, -1 } );
    }

    if ( config.Exists( "status window position" ) ) {
        pos_stat = config.PointParams( "status window position", { -1, -1 } );
    }

    if ( config.Exists( "videomode" ) ) {
        _resolutionInfo = config.ResolutionParams( "videomode", { fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT } );
    }

    if ( config.Exists( "fullscreen" ) ) {
        setFullScreen( config.StrParams( "fullscreen" ) == "on" );
    }

    if ( config.Exists( "controller pointer speed" ) ) {
        _controllerPointerSpeed = std::clamp( config.IntParams( "controller pointer speed" ), 0, 100 );
    }

    if ( config.Exists( "first time game run" ) && config.StrParams( "first time game run" ) == "off" ) {
        resetFirstGameRun();
    }

    if ( config.Exists( "show game intro" ) ) {
        if ( config.StrParams( "show game intro" ) == "on" ) {
            _gameOptions.SetModes( GAME_SHOW_INTRO );
        }
        else {
            _gameOptions.ResetModes( GAME_SHOW_INTRO );
        }
    }

    if ( config.Exists( "v-sync" ) ) {
        setVSync( config.StrParams( "v-sync" ) == "on" );
    }

    if ( config.Exists( "text support mode" ) ) {
        setTextSupportMode( config.StrParams( "text support mode" ) == "on" );
    }

    if ( config.Exists( "monochrome cursor" ) ) {
        // We cannot set cursor before initializing the system since we read a configuration file before initialization.
        if ( config.StrParams( "monochrome cursor" ) == "on" ) {
            _gameOptions.SetModes( GAME_MONOCHROME_CURSOR );
            Cursor::Get().setMonochromeCursor( true );
        }
        else {
            _gameOptions.ResetModes( GAME_MONOCHROME_CURSOR );
            Cursor::Get().setMonochromeCursor( false );
        }
    }

    if ( config.Exists( "3d audio" ) ) {
        set3DAudio( config.StrParams( "3d audio" ) == "on" );
    }

    if ( config.Exists( "system info" ) ) {
        setSystemInfo( config.StrParams( "system info" ) == "on" );
    }

    if ( config.Exists( "auto save at the beginning of the turn" ) ) {
        setAutoSaveAtBeginningOfTurn( config.StrParams( "auto save at the beginning of the turn" ) == "on" );
    }

    if ( config.Exists( "cursor soft rendering" ) ) {
        if ( config.StrParams( "cursor soft rendering" ) == "on" ) {
            _gameOptions.SetModes( GAME_CURSOR_SOFT_EMULATION );
            fheroes2::cursor().enableSoftwareEmulation( true );
        }
        else {
            _gameOptions.ResetModes( GAME_CURSOR_SOFT_EMULATION );
            fheroes2::cursor().enableSoftwareEmulation( false );
        }
    }

    if ( config.Exists( "screen scaling type" ) ) {
        setScreenScalingTypeNearest( config.StrParams( "screen scaling type" ) == "nearest" );
    }

    if ( config.Exists( "editor animation" ) ) {
        setEditorAnimation( config.StrParams( "editor animation" ) == "on" );
    }

    if ( config.Exists( "editor passability" ) ) {
        setEditorPassability( config.StrParams( "editor passability" ) == "on" );
    }

    return true;
}

bool Settings::Save( const std::string_view fileName ) const
{
    if ( fileName.empty() ) {
        return false;
    }

    const std::string cfgFilename = System::concatPath( System::GetConfigDirectory( "fheroes2" ), fileName );

    std::fstream file;
    file.open( cfgFilename.data(), std::fstream::out | std::fstream::trunc );
    if ( !file ) {
        return false;
    }

    const std::string & data = String();
    file.write( data.data(), data.size() );

    return true;
}

std::string Settings::String() const
{
    std::ostringstream os;

    std::string musicType;
    if ( MusicType() == MUSIC_EXTERNAL ) {
        musicType = "external";
    }
    else if ( MusicType() == MUSIC_MIDI_EXPANSION ) {
        musicType = "expansion";
    }
    else {
        musicType = "original";
    }

    os << "# fheroes2 configuration file (saved by version " << GetVersion() << ")" << std::endl;

    const fheroes2::Display & display = fheroes2::Display::instance();

    os << std::endl << "# video mode: in-game width x in-game height : on-screen width x on-screen height" << std::endl;
    os << "videomode = " << display.width() << "x" << display.height() << ":" << display.screenSize().width << "x" << display.screenSize().height << std::endl;

    os << std::endl << "# music: original, expansion, external" << std::endl;
    os << "music = " << musicType << std::endl;

    os << std::endl << "# sound volume: 0 - 10" << std::endl;
    os << "sound volume = " << sound_volume << std::endl;

    os << std::endl << "# music volume: 0 - 10" << std::endl;
    os << "music volume = " << music_volume << std::endl;

    os << std::endl << "# run in fullscreen mode: on/off" << std::endl;
    os << "fullscreen = " << ( _gameOptions.Modes( GAME_FULLSCREEN ) ? "on" : "off" ) << std::endl;

    os << std::endl << "# print debug messages (only for development, see src/engine/logging.h for possible values)" << std::endl;
    os << "debug = " << Logging::getDebugLevel() << std::endl;

    os << std::endl << "# heroes movement speed: 1 - 10" << std::endl;
    os << "heroes speed = " << heroes_speed << std::endl;

    os << std::endl << "# AI movement speed: 0 - 10" << std::endl;
    os << "ai speed = " << ai_speed << std::endl;

    os << std::endl << "# battle speed: 1 - 10" << std::endl;
    os << "battle speed = " << battle_speed << std::endl;

    os << std::endl << "# Adventure Map scrolling speed: 0 - 4. 0 means no scrolling" << std::endl;
    os << "scroll speed = " << scroll_speed << std::endl;

    os << std::endl << "# show battle grid: on/off" << std::endl;
    os << "battle grid = " << ( _gameOptions.Modes( GAME_BATTLE_SHOW_GRID ) ? "on" : "off" ) << std::endl;

    os << std::endl << "# show battle shadow movement: on/off" << std::endl;
    os << "battle shadow movement = " << ( _gameOptions.Modes( GAME_BATTLE_SHOW_MOVE_SHADOW ) ? "on" : "off" ) << std::endl;

    os << std::endl << "# show battle shadow cursor: on/off" << std::endl;
    os << "battle shadow cursor = " << ( _gameOptions.Modes( GAME_BATTLE_SHOW_MOUSE_SHADOW ) ? "on" : "off" ) << std::endl;

    os << std::endl << "# show battle damage information: on/off" << std::endl;
    os << "battle show damage = " << ( _gameOptions.Modes( GAME_BATTLE_SHOW_DAMAGE ) ? "on" : "off" ) << std::endl;

    os << std::endl << "# auto resolve battles: on/off" << std::endl;
    os << "auto resolve battles = " << ( _gameOptions.Modes( GAME_BATTLE_AUTO_RESOLVE ) ? "on" : "off" ) << std::endl;

    os << std::endl << "# auto combat spell casting: on/off" << std::endl;
    os << "auto spell casting = " << ( _gameOptions.Modes( GAME_BATTLE_AUTO_SPELLCAST ) ? "on" : "off" ) << std::endl;

    os << std::endl << "# show turn order during battle: on/off" << std::endl;
    os << "battle turn order = " << ( _gameOptions.Modes( GAME_BATTLE_SHOW_TURN_ORDER ) ? "on" : "off" ) << std::endl;

    os << std::endl << "# use evil interface style: on/off" << std::endl;
    os << "use evil interface = " << ( _gameOptions.Modes( GAME_EVIL_INTERFACE ) ? "on" : "off" ) << std::endl;

    os << std::endl << "# hide interface elements on the adventure map: on/off" << std::endl;
    os << "hide interface = " << ( _gameOptions.Modes( GAME_HIDE_INTERFACE ) ? "on" : "off" ) << std::endl;

    os << std::endl << "# position of the radar window on the adventure map when interface elements are hidden" << std::endl;
    os << "radar window position = [ " << pos_radr.x << ", " << pos_radr.y << " ]" << std::endl;

    os << std::endl << "# position of the buttons window on the adventure map when interface elements are hidden" << std::endl;
    os << "buttons window position = [ " << pos_bttn.x << ", " << pos_bttn.y << " ]" << std::endl;

    os << std::endl << "# position of the icons window on the adventure map when interface elements are hidden" << std::endl;
    os << "icons window position = [ " << pos_icon.x << ", " << pos_icon.y << " ]" << std::endl;

    os << std::endl << "# position of the status window on the adventure map when interface elements are hidden" << std::endl;
    os << "status window position = [ " << pos_stat.x << ", " << pos_stat.y << " ]" << std::endl;

    os << std::endl << "# game language (an empty value means English)" << std::endl;
    os << "lang = " << _gameLanguage << std::endl;

    os << std::endl << "# controller pointer speed: 0 - 100" << std::endl;
    os << "controller pointer speed = " << _controllerPointerSpeed << std::endl;

    os << std::endl << "# first time game run (show additional hints): on/off" << std::endl;
    os << "first time game run = " << ( _gameOptions.Modes( GAME_FIRST_RUN ) ? "on" : "off" ) << std::endl;

    os << std::endl << "# show game intro (splash screen and video): on/off" << std::endl;
    os << "show game intro = " << ( _gameOptions.Modes( GAME_SHOW_INTRO ) ? "on" : "off" ) << std::endl;

    os << std::endl << "# enable V-Sync (Vertical Synchronization) for rendering" << std::endl;
    os << "v-sync = " << ( _gameOptions.Modes( GAME_RENDER_VSYNC ) ? "on" : "off" ) << std::endl;

    os << std::endl << "# enable text support mode to output extra information in console window: on/off" << std::endl;
    os << "text support mode = " << ( _gameOptions.Modes( GAME_TEXT_SUPPORT_MODE ) ? "on" : "off" ) << std::endl;

    os << std::endl << "# enable monochrome (black and white) cursors in the game: on/off" << std::endl;
    os << "monochrome cursor = " << ( _gameOptions.Modes( GAME_MONOCHROME_CURSOR ) ? "on" : "off" ) << std::endl;

    os << std::endl << "# enable 3D audio for objects on Adventure Map: on/off" << std::endl;
    os << "3d audio = " << ( _gameOptions.Modes( GAME_3D_AUDIO ) ? "on" : "off" ) << std::endl;

    os << std::endl << "# display system information: on/off" << std::endl;
    os << "system info = " << ( _gameOptions.Modes( GAME_SYSTEM_INFO ) ? "on" : "off" ) << std::endl;

    os << std::endl << "# should auto save be performed at the beginning of the turn instead of the end of the turn: on/off" << std::endl;
    os << "auto save at the beginning of the turn = " << ( _gameOptions.Modes( GAME_AUTO_SAVE_AT_BEGINNING_OF_TURN ) ? "on" : "off" ) << std::endl;

    os << std::endl << "# enable cursor software rendering" << std::endl;
    os << "cursor soft rendering = " << ( _gameOptions.Modes( GAME_CURSOR_SOFT_EMULATION ) ? "on" : "off" ) << std::endl;

    os << std::endl << "# scaling type: nearest or linear (set by default)" << std::endl;
    os << "screen scaling type = " << ( _gameOptions.Modes( GAME_SCREEN_SCALING_TYPE_NEAREST ) ? "nearest" : "linear" ) << std::endl;

    os << std::endl << "# show object animation in the Editor: on/off" << std::endl;
    os << "editor animation = " << ( _editorOptions.Modes( EDITOR_ANIMATION ) ? "on" : "off" ) << std::endl;

    os << std::endl << "# display object passability in the Editor: on/off" << std::endl;
    os << "editor passability = " << ( _editorOptions.Modes( EDITOR_PASSABILITY ) ? "on" : "off" ) << std::endl;

    return os.str();
}

void Settings::SetCurrentFileInfo( Maps::FileInfo fi )
{
    current_maps_file = std::move( fi );

    players.Init( current_maps_file );
}

bool Settings::setGameLanguage( const std::string & language )
{
    fheroes2::updateAlphabet( language );

    _gameLanguage = language;

    if ( _gameLanguage.empty() ) {
        Translation::reset();
        return true;
    }

    const std::string fileName = std::string( _gameLanguage ).append( ".mo" );
#if defined( MACOS_APP_BUNDLE )
    const ListFiles translations = Settings::FindFiles( "translations", fileName, false );
#else
    const ListFiles translations = Settings::FindFiles( System::concatPath( "files", "lang" ), fileName, false );
#endif

    if ( !translations.empty() ) {
        return Translation::bindDomain( language.c_str(), translations.back().c_str() );
    }

    ERROR_LOG( "Translation file " << fileName << " was not found." )
    return false;
}

void Settings::setEditorAnimation( const bool enable )
{
    if ( enable ) {
        _editorOptions.SetModes( EDITOR_ANIMATION );
    }
    else {
        _editorOptions.ResetModes( EDITOR_ANIMATION );
    }
}

void Settings::setEditorPassability( const bool enable )
{
    if ( enable ) {
        _editorOptions.SetModes( EDITOR_PASSABILITY );
    }
    else {
        _editorOptions.ResetModes( EDITOR_PASSABILITY );
    }
}

void Settings::SetProgramPath( const char * argv0 )
{
    if ( argv0 )
        path_program = argv0;
}

const std::vector<std::string> & Settings::GetRootDirs()
{
    static std::vector<std::string> dirs;
    if ( !dirs.empty() ) {
        return dirs;
    }

#ifdef FHEROES2_DATA
    // Macro-defined path.
    dirs.emplace_back( EXPANDDEF( FHEROES2_DATA ) );
#endif

    // Environment variable.
    const char * dataEnvPath = getenv( "FHEROES2_DATA" );
    if ( dataEnvPath != nullptr && std::find( dirs.begin(), dirs.end(), dataEnvPath ) == dirs.end() ) {
        dirs.emplace_back( dataEnvPath );
    }

    // The location of the application.
    std::string appPath = System::GetDirname( Settings::Get().path_program );
    if ( !appPath.empty() && std::find( dirs.begin(), dirs.end(), appPath ) == dirs.end() ) {
        dirs.emplace_back( std::move( appPath ) );
    }

    // OS specific directories.
    System::appendOSSpecificDirectories( dirs );

#if defined( MACOS_APP_BUNDLE )
    // macOS app bundle Resources directory
    char resourcePath[PATH_MAX];

    CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL( CFBundleGetMainBundle() );
    if ( CFURLGetFileSystemRepresentation( resourcesURL, TRUE, reinterpret_cast<UInt8 *>( resourcePath ), PATH_MAX )
         && std::find( dirs.begin(), dirs.end(), resourcePath ) == dirs.end() ) {
        dirs.emplace_back( resourcePath );
    }
    else {
        ERROR_LOG( "Unable to get app bundle path" )
    }
    CFRelease( resourcesURL );
#endif

    // User config directory.
    std::string configPath = System::GetConfigDirectory( "fheroes2" );
    if ( !configPath.empty() && std::find( dirs.begin(), dirs.end(), configPath ) == dirs.end() ) {
        dirs.emplace_back( std::move( configPath ) );
    }

    // User data directory.
    std::string dataPath = System::GetDataDirectory( "fheroes2" );
    if ( !dataPath.empty() && std::find( dirs.begin(), dirs.end(), dataPath ) == dirs.end() ) {
        dirs.emplace_back( std::move( dataPath ) );
    }

    // Remove all paths that are not directories.
    dirs.erase( std::remove_if( dirs.begin(), dirs.end(), []( const std::string & path ) { return !System::IsDirectory( path ); } ), dirs.end() );

    return dirs;
}

ListFiles Settings::FindFiles( const std::string & prefixDir, const std::string & fileNameFilter, const bool exactMatch )
{
    ListFiles res;

    for ( const std::string & dir : GetRootDirs() ) {
        const std::string path = !prefixDir.empty() ? System::concatPath( dir, prefixDir ) : dir;

        if ( System::IsDirectory( path ) ) {
            if ( exactMatch ) {
                res.FindFileInDir( path, fileNameFilter, false );
            }
            else {
                res.ReadDir( path, fileNameFilter, false );
            }
        }
    }

    return res;
}

bool Settings::findFile( const std::string & internalDirectory, const std::string & fileName, std::string & fullPath )
{
    std::string tempPath;

    for ( const std::string & rootDir : GetRootDirs() ) {
        tempPath = System::concatPath( rootDir, internalDirectory );
        tempPath = System::concatPath( tempPath, fileName );
        if ( System::IsFile( tempPath ) ) {
            fullPath.swap( tempPath );
            return true;
        }
    }

    return false;
}

std::string Settings::GetLastFile( const std::string & prefix, const std::string & name )
{
    const ListFiles & files = FindFiles( prefix, name, true );
    return files.empty() ? name : files.back();
}

void Settings::SetAIMoveSpeed( int speed )
{
    ai_speed = std::clamp( speed, 0, 10 );
}

void Settings::SetHeroesMoveSpeed( int speed )
{
    heroes_speed = std::clamp( speed, 1, 10 );
}

void Settings::SetBattleSpeed( int speed )
{
    battle_speed = std::clamp( speed, 1, 10 );
}

void Settings::setBattleAutoResolve( bool enable )
{
    if ( enable ) {
        _gameOptions.SetModes( GAME_BATTLE_AUTO_RESOLVE );
    }
    else {
        _gameOptions.ResetModes( GAME_BATTLE_AUTO_RESOLVE );
    }
}

void Settings::setBattleAutoSpellcast( bool enable )
{
    if ( enable ) {
        _gameOptions.SetModes( GAME_BATTLE_AUTO_SPELLCAST );
    }
    else {
        _gameOptions.ResetModes( GAME_BATTLE_AUTO_SPELLCAST );
    }
}

void Settings::setBattleShowTurnOrder( const bool enable )
{
    if ( enable ) {
        _gameOptions.SetModes( GAME_BATTLE_SHOW_TURN_ORDER );
    }
    else {
        _gameOptions.ResetModes( GAME_BATTLE_SHOW_TURN_ORDER );
    }
}

void Settings::setFullScreen( const bool enable )
{
    if ( enable ) {
        _gameOptions.SetModes( GAME_FULLSCREEN );
    }
    else {
        _gameOptions.ResetModes( GAME_FULLSCREEN );
    }

    if ( enable != fheroes2::engine().isFullScreen() ) {
        fheroes2::engine().toggleFullScreen();
        fheroes2::Display::instance().render();
    }
}

void Settings::setMonochromeCursor( const bool enable )
{
    if ( enable ) {
        _gameOptions.SetModes( GAME_MONOCHROME_CURSOR );
        Cursor::Get().setMonochromeCursor( true );
    }
    else {
        _gameOptions.ResetModes( GAME_MONOCHROME_CURSOR );
        Cursor::Get().setMonochromeCursor( false );
    }

    Cursor::Refresh();
}

void Settings::setTextSupportMode( const bool enable )
{
    if ( enable ) {
        _gameOptions.SetModes( GAME_TEXT_SUPPORT_MODE );
        Logging::setTextSupportMode( true );
    }
    else {
        _gameOptions.ResetModes( GAME_TEXT_SUPPORT_MODE );
        Logging::setTextSupportMode( false );
    }
}

void Settings::set3DAudio( const bool enable )
{
    if ( enable ) {
        _gameOptions.SetModes( GAME_3D_AUDIO );
    }
    else {
        _gameOptions.ResetModes( GAME_3D_AUDIO );
    }
}

void Settings::setVSync( const bool enable )
{
    if ( enable ) {
        _gameOptions.SetModes( GAME_RENDER_VSYNC );
        fheroes2::engine().setVSync( true );
    }
    else {
        _gameOptions.ResetModes( GAME_RENDER_VSYNC );
        fheroes2::engine().setVSync( false );
    }
}

void Settings::setSystemInfo( const bool enable )
{
    if ( enable ) {
        _gameOptions.SetModes( GAME_SYSTEM_INFO );
        fheroes2::RenderProcessor::instance().enableRenderers();
    }
    else {
        _gameOptions.ResetModes( GAME_SYSTEM_INFO );
        fheroes2::RenderProcessor::instance().disableRenderers();
    }
}

void Settings::setAutoSaveAtBeginningOfTurn( const bool enable )
{
    if ( enable ) {
        _gameOptions.SetModes( GAME_AUTO_SAVE_AT_BEGINNING_OF_TURN );
    }
    else {
        _gameOptions.ResetModes( GAME_AUTO_SAVE_AT_BEGINNING_OF_TURN );
    }
}

void Settings::setBattleDamageInfo( const bool enable )
{
    if ( enable ) {
        _gameOptions.SetModes( GAME_BATTLE_SHOW_DAMAGE );
    }
    else {
        _gameOptions.ResetModes( GAME_BATTLE_SHOW_DAMAGE );
    }
}

void Settings::setHideInterface( const bool enable )
{
    if ( enable ) {
        _gameOptions.SetModes( GAME_HIDE_INTERFACE );
    }
    else {
        _gameOptions.ResetModes( GAME_HIDE_INTERFACE );
    }
}

void Settings::setEvilInterface( const bool enable )
{
    if ( enable ) {
        _gameOptions.SetModes( GAME_EVIL_INTERFACE );
    }
    else {
        _gameOptions.ResetModes( GAME_EVIL_INTERFACE );
    }
}

void Settings::setScreenScalingTypeNearest( const bool enable )
{
    if ( enable ) {
        _gameOptions.SetModes( GAME_SCREEN_SCALING_TYPE_NEAREST );
        fheroes2::engine().setNearestScaling( true );
    }
    else {
        _gameOptions.ResetModes( GAME_SCREEN_SCALING_TYPE_NEAREST );
        fheroes2::engine().setNearestScaling( false );
    }
}

void Settings::SetScrollSpeed( int speed )
{
    scroll_speed = std::clamp( speed, static_cast<int>( SCROLL_SPEED_NONE ), static_cast<int>( SCROLL_SPEED_VERY_FAST ) );
}

bool Settings::isPriceOfLoyaltySupported() const
{
    return _gameOptions.Modes( GAME_PRICELOYALTY );
}

bool Settings::isMonochromeCursorEnabled() const
{
    return _gameOptions.Modes( GAME_MONOCHROME_CURSOR );
}

bool Settings::isTextSupportModeEnabled() const
{
    return _gameOptions.Modes( GAME_TEXT_SUPPORT_MODE );
}

bool Settings::is3DAudioEnabled() const
{
    return _gameOptions.Modes( GAME_3D_AUDIO );
}

bool Settings::isSystemInfoEnabled() const
{
    return _gameOptions.Modes( GAME_SYSTEM_INFO );
}

bool Settings::isAutoSaveAtBeginningOfTurnEnabled() const
{
    return _gameOptions.Modes( GAME_AUTO_SAVE_AT_BEGINNING_OF_TURN );
}

bool Settings::isBattleShowDamageInfoEnabled() const
{
    return _gameOptions.Modes( GAME_BATTLE_SHOW_DAMAGE );
}

bool Settings::isHideInterfaceEnabled() const
{
    return _gameOptions.Modes( GAME_HIDE_INTERFACE );
}

bool Settings::isEvilInterfaceEnabled() const
{
    return _gameOptions.Modes( GAME_EVIL_INTERFACE );
}

bool Settings::isEditorAnimationEnabled() const
{
    return _editorOptions.Modes( EDITOR_ANIMATION );
}

bool Settings::isEditorPassabilityEnabled() const
{
    return _editorOptions.Modes( EDITOR_PASSABILITY );
}

bool Settings::ShowControlPanel() const
{
    return _gameOptions.Modes( GAME_SHOW_CONTROL_PANEL );
}

bool Settings::ShowRadar() const
{
    return _gameOptions.Modes( GAME_SHOW_RADAR );
}

bool Settings::ShowIcons() const
{
    return _gameOptions.Modes( GAME_SHOW_ICONS );
}

bool Settings::ShowButtons() const
{
    return _gameOptions.Modes( GAME_SHOW_BUTTONS );
}

bool Settings::ShowStatus() const
{
    return _gameOptions.Modes( GAME_SHOW_STATUS );
}

bool Settings::BattleShowGrid() const
{
    return _gameOptions.Modes( GAME_BATTLE_SHOW_GRID );
}

bool Settings::BattleShowMouseShadow() const
{
    return _gameOptions.Modes( GAME_BATTLE_SHOW_MOUSE_SHADOW );
}

bool Settings::BattleShowMoveShadow() const
{
    return _gameOptions.Modes( GAME_BATTLE_SHOW_MOVE_SHADOW );
}

bool Settings::BattleAutoResolve() const
{
    return _gameOptions.Modes( GAME_BATTLE_AUTO_RESOLVE );
}

bool Settings::BattleAutoSpellcast() const
{
    return _gameOptions.Modes( GAME_BATTLE_AUTO_SPELLCAST );
}

bool Settings::BattleShowTurnOrder() const
{
    return _gameOptions.Modes( GAME_BATTLE_SHOW_TURN_ORDER );
}

void Settings::setDebug( int debug )
{
    switch ( debug ) {
    case 0:
        debug = DBG_ALL_WARN;
        break;
    case 1:
        debug = DBG_ALL_INFO;
        break;
    case 2:
        debug = DBG_ALL_TRACE;
        break;
    case 3:
        debug = DBG_ENGINE_TRACE;
        break;
    case 4:
        debug = DBG_GAME_INFO | DBG_BATTLE_INFO | DBG_AI_INFO;
        break;
    case 5:
        debug = DBG_GAME_TRACE | DBG_AI_INFO | DBG_BATTLE_INFO;
        break;
    case 6:
        debug = DBG_AI_TRACE | DBG_BATTLE_INFO | DBG_GAME_INFO;
        break;
    case 7:
        debug = DBG_BATTLE_TRACE | DBG_AI_INFO | DBG_GAME_INFO;
        break;
    case 8:
        debug = DBG_DEVEL | DBG_GAME_TRACE;
        break;
    case 9:
        debug = DBG_DEVEL | DBG_AI_INFO | DBG_BATTLE_INFO | DBG_GAME_INFO;
        break;
    case 10:
        debug = DBG_DEVEL | DBG_AI_TRACE | DBG_BATTLE_INFO | DBG_GAME_INFO;
        break;
    case 11:
        debug = DBG_DEVEL | DBG_AI_TRACE | DBG_BATTLE_TRACE | DBG_GAME_INFO;
        break;
    default:
        break;
    }

#ifndef WITH_DEBUG
    debug &= ~DBG_DEVEL;
#endif

    Logging::setDebugLevel( debug );
}

void Settings::SetSoundVolume( int v )
{
    sound_volume = std::clamp( v, 0, 10 );
}

void Settings::SetMusicVolume( int v )
{
    music_volume = std::clamp( v, 0, 10 );
}

bool Settings::isCampaignGameType() const
{
    return ( game_type & Game::TYPE_CAMPAIGN ) != 0;
}

void Settings::EnablePriceOfLoyaltySupport( const bool set )
{
    if ( set ) {
        _gameOptions.SetModes( GAME_PRICELOYALTY );
    }
    else {
        _gameOptions.ResetModes( GAME_PRICELOYALTY );
        if ( _musicType == MUSIC_MIDI_EXPANSION )
            _musicType = MUSIC_MIDI_ORIGINAL;
    }
}

void Settings::SetBattleGrid( bool f )
{
    f ? _gameOptions.SetModes( GAME_BATTLE_SHOW_GRID ) : _gameOptions.ResetModes( GAME_BATTLE_SHOW_GRID );
}

void Settings::SetBattleMovementShaded( bool f )
{
    f ? _gameOptions.SetModes( GAME_BATTLE_SHOW_MOVE_SHADOW ) : _gameOptions.ResetModes( GAME_BATTLE_SHOW_MOVE_SHADOW );
}

void Settings::SetBattleMouseShaded( bool f )
{
    f ? _gameOptions.SetModes( GAME_BATTLE_SHOW_MOUSE_SHADOW ) : _gameOptions.ResetModes( GAME_BATTLE_SHOW_MOUSE_SHADOW );
}

void Settings::SetShowControlPanel( bool f )
{
    f ? _gameOptions.SetModes( GAME_SHOW_CONTROL_PANEL ) : _gameOptions.ResetModes( GAME_SHOW_CONTROL_PANEL );
}

void Settings::SetShowRadar( bool f )
{
    f ? _gameOptions.SetModes( GAME_SHOW_RADAR ) : _gameOptions.ResetModes( GAME_SHOW_RADAR );
}

void Settings::SetShowIcons( bool f )
{
    f ? _gameOptions.SetModes( GAME_SHOW_ICONS ) : _gameOptions.ResetModes( GAME_SHOW_ICONS );
}

void Settings::SetShowButtons( bool f )
{
    f ? _gameOptions.SetModes( GAME_SHOW_BUTTONS ) : _gameOptions.ResetModes( GAME_SHOW_BUTTONS );
}

void Settings::SetShowStatus( bool f )
{
    f ? _gameOptions.SetModes( GAME_SHOW_STATUS ) : _gameOptions.ResetModes( GAME_SHOW_STATUS );
}

bool Settings::FullScreen() const
{
    return _gameOptions.Modes( GAME_FULLSCREEN );
}

bool Settings::isVSyncEnabled() const
{
    return _gameOptions.Modes( GAME_RENDER_VSYNC );
}

bool Settings::isFirstGameRun() const
{
    return _gameOptions.Modes( GAME_FIRST_RUN );
}

bool Settings::isShowIntro() const
{
    return _gameOptions.Modes( GAME_SHOW_INTRO );
}

void Settings::resetFirstGameRun()
{
    _gameOptions.ResetModes( GAME_FIRST_RUN );
}

StreamBase & operator<<( StreamBase & msg, const Settings & conf )
{
    return msg << conf._gameLanguage << conf.current_maps_file << conf._gameDifficulty << conf.game_type << conf.players;
}

StreamBase & operator>>( StreamBase & msg, Settings & conf )
{
    msg >> conf._loadedFileLanguage >> conf.current_maps_file >> conf._gameDifficulty >> conf.game_type;

    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_PRE1_1101_RELEASE, "Remove the logic below." );
    if ( Game::GetVersionOfCurrentSaveFile() < FORMAT_VERSION_PRE1_1101_RELEASE ) {
        int temp;
        msg >> temp;
    }

    return msg >> conf.players;
}
