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

#include <algorithm>
#include <cstdlib>
#include <fstream>

#include "difficulty.h"
#include "game.h"
#include "logging.h"
#include "save_format_version.h"
#include "screen.h"
#include "serialize.h"
#include "settings.h"
#include "system.h"
#include "tinyconfig.h"
#include "tools.h"
#include "translations.h"
#include "ui_language.h"
#include "version.h"

#define STRINGIFY( DEF ) #DEF
#define EXPANDDEF( DEF ) STRINGIFY( DEF )

namespace
{
    enum
    {
        GLOBAL_FIRST_RUN = 0x00000001,
        GLOBAL_SHOW_INTRO = 0x00000002,
        GLOBAL_PRICELOYALTY = 0x00000004,

        GLOBAL_RENDER_VSYNC = 0x00000008,
        // UNUSED = 0x00000010,
        // UNUSED = 0x00000020,

        GLOBAL_SHOWCPANEL = 0x00000040,
        GLOBAL_SHOWRADAR = 0x00000080,
        GLOBAL_SHOWICONS = 0x00000100,
        GLOBAL_SHOWBUTTONS = 0x00000200,
        GLOBAL_SHOWSTATUS = 0x00000400,

        GLOBAL_FULLSCREEN = 0x00008000,
        // UNUSED = 0x00010000,

        GLOBAL_MUSIC_EXT = 0x00020000,
        GLOBAL_MUSIC_MIDI = 0x00040000,
        GLOBAL_MUSIC = GLOBAL_MUSIC_EXT | GLOBAL_MUSIC_MIDI,

        // UNUSED = 0x00080000,
        // UNUSED = 0x00100000,
        // UNUSED = 0x00200000,

        GLOBAL_BATTLE_SHOW_ARMY_ORDER = 0x00400000,
        GLOBAL_BATTLE_SHOW_GRID = 0x00800000,
        GLOBAL_BATTLE_SHOW_MOUSE_SHADOW = 0x01000000,
        GLOBAL_BATTLE_SHOW_MOVE_SHADOW = 0x02000000,
        GLOBAL_BATTLE_AUTO_RESOLVE = 0x04000000,
        GLOBAL_BATTLE_AUTO_SPELLCAST = 0x08000000
    };
}

std::string Settings::GetVersion()
{
    return std::to_string( MAJOR_VERSION ) + '.' + std::to_string( MINOR_VERSION ) + '.' + std::to_string( INTERMEDIATE_VERSION );
}

Settings::Settings()
    : debug( 0 )
    , video_mode( fheroes2::Size( fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT ) )
    , game_difficulty( Difficulty::NORMAL )
    , sound_volume( 6 )
    , music_volume( 6 )
    , _musicType( MUSIC_EXTERNAL )
    , _controllerPointerSpeed( 10 )
    , heroes_speed( DEFAULT_SPEED_DELAY )
    , ai_speed( DEFAULT_SPEED_DELAY )
    , scroll_speed( SCROLL_NORMAL )
    , battle_speed( DEFAULT_BATTLE_SPEED )
    , game_type( 0 )
    , preferably_count_players( 0 )
{
    opt_global.SetModes( GLOBAL_FIRST_RUN );
    opt_global.SetModes( GLOBAL_SHOW_INTRO );
    opt_global.SetModes( GLOBAL_SHOWRADAR );
    opt_global.SetModes( GLOBAL_SHOWICONS );
    opt_global.SetModes( GLOBAL_SHOWBUTTONS );
    opt_global.SetModes( GLOBAL_SHOWSTATUS );
    opt_global.SetModes( GLOBAL_MUSIC_EXT );

    opt_global.SetModes( GLOBAL_BATTLE_SHOW_GRID );
    opt_global.SetModes( GLOBAL_BATTLE_SHOW_MOUSE_SHADOW );
    opt_global.SetModes( GLOBAL_BATTLE_SHOW_MOVE_SHADOW );
    opt_global.SetModes( GLOBAL_BATTLE_AUTO_SPELLCAST );

    // The Price of Loyalty is not supported by default.
    EnablePriceOfLoyaltySupport( false );
}

Settings::~Settings()
{
    if ( !LoadedGameVersion() )
        BinarySave();
}

Settings & Settings::Get()
{
    static Settings conf;

    return conf;
}

bool Settings::Read( const std::string & filename )
{
    TinyConfig config( '=', '#' );

    std::string sval;
    int ival;

    if ( !config.Load( filename ) )
        return false;

    // debug
    ival = config.IntParams( "debug" );

    switch ( ival ) {
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
        debug = ival;
        break;
    }

#ifndef WITH_DEBUG
    // reset devel
    debug &= ~( DBG_DEVEL );
#endif

    Logging::SetDebugLevel( debug );

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
            opt_global.ResetModes( GLOBAL_MUSIC );
            opt_global.SetModes( GLOBAL_MUSIC_MIDI );
            _musicType = MUSIC_MIDI_ORIGINAL;
        }
        else if ( sval == "expansion" ) {
            opt_global.ResetModes( GLOBAL_MUSIC );
            opt_global.SetModes( GLOBAL_MUSIC_MIDI );
            _musicType = MUSIC_MIDI_EXPANSION;
        }
        else if ( sval == "external" ) {
            opt_global.ResetModes( GLOBAL_MUSIC );
            opt_global.SetModes( GLOBAL_MUSIC_EXT );
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

    if ( config.Exists( "auto resolve battles" ) ) {
        setBattleAutoResolve( config.StrParams( "auto resolve battles" ) == "on" );
    }

    if ( config.Exists( "auto spell casting" ) ) {
        setBattleAutoSpellcast( config.StrParams( "auto spell casting" ) == "on" );
    }

    if ( config.Exists( "battle army order" ) ) {
        setBattleShowArmyOrder( config.StrParams( "battle army order" ) == "on" );
    }

    // videomode
    sval = config.StrParams( "videomode" );
    if ( !sval.empty() ) {
        // default
        video_mode.width = fheroes2::Display::DEFAULT_WIDTH;
        video_mode.height = fheroes2::Display::DEFAULT_HEIGHT;

        std::string value = StringLower( sval );
        const size_t pos = value.find( 'x' );

        if ( std::string::npos != pos ) {
            std::string width( value.substr( 0, pos ) );
            std::string height( value.substr( pos + 1, value.length() - pos - 1 ) );

            video_mode.width = GetInt( width );
            video_mode.height = GetInt( height );
        }
        else {
            DEBUG_LOG( DBG_ENGINE, DBG_WARN, "unknown video mode: " << value );
        }
    }

    // full screen
    if ( config.Exists( "fullscreen" ) ) {
        setFullScreen( config.StrParams( "fullscreen" ) == "on" );
    }

    if ( config.Exists( "controller pointer speed" ) ) {
        _controllerPointerSpeed = clamp( config.IntParams( "controller pointer speed" ), 0, 100 );
    }

    if ( config.Exists( "first time game run" ) && config.StrParams( "first time game run" ) == "off" ) {
        resetFirstGameRun();
    }

    if ( config.Exists( "show game intro" ) ) {
        if ( config.StrParams( "show game intro" ) == "on" ) {
            opt_global.SetModes( GLOBAL_SHOW_INTRO );
        }
        else {
            opt_global.ResetModes( GLOBAL_SHOW_INTRO );
        }
    }

    if ( config.Exists( "v-sync" ) ) {
        if ( config.StrParams( "v-sync" ) == "on" ) {
            opt_global.SetModes( GLOBAL_RENDER_VSYNC );
        }
        else {
            opt_global.ResetModes( GLOBAL_RENDER_VSYNC );
        }
    }

    BinaryLoad();

    if ( video_mode.width > 0 && video_mode.height > 0 ) {
        PostLoad();
    }

    return true;
}

void Settings::PostLoad()
{
    if ( ExtModes( GAME_HIDE_INTERFACE ) ) {
        opt_global.SetModes( GLOBAL_SHOWCPANEL );
        opt_global.ResetModes( GLOBAL_SHOWRADAR );
        opt_global.ResetModes( GLOBAL_SHOWICONS );
        opt_global.ResetModes( GLOBAL_SHOWBUTTONS );
        opt_global.ResetModes( GLOBAL_SHOWSTATUS );
    }
}

bool Settings::Save( const std::string & filename ) const
{
    if ( filename.empty() )
        return false;

    std::fstream file;
#if defined( FHEROES2_VITA )
    const std::string vitaFilename = "ux0:data/fheroes2/" + filename;
    file.open( vitaFilename.data(), std::fstream::out | std::fstream::trunc );
#else
    const std::string cfgFilename = System::ConcatePath( System::GetConfigDirectory( "fheroes2" ), filename );
    file.open( cfgFilename.data(), std::fstream::out | std::fstream::trunc );
#endif
    if ( !file )
        return false;

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

    os << std::endl << "# video mode (game resolution)" << std::endl;
    os << "videomode = " << fheroes2::Display::instance().width() << "x" << fheroes2::Display::instance().height() << std::endl;

    os << std::endl << "# music: original, expansion, external" << std::endl;
    os << "music = " << musicType << std::endl;

    os << std::endl << "# sound volume: 0 - 10" << std::endl;
    os << "sound volume = " << sound_volume << std::endl;

    os << std::endl << "# music volume: 0 - 10" << std::endl;
    os << "music volume = " << music_volume << std::endl;

    os << std::endl << "# run in fullscreen mode: on/off (use F4 key to switch between modes)" << std::endl;
    os << "fullscreen = " << ( opt_global.Modes( GLOBAL_FULLSCREEN ) ? "on" : "off" ) << std::endl;

    os << std::endl << "# print debug messages (only for development, see src/engine/logging.h for possible values)" << std::endl;
    os << "debug = " << debug << std::endl;

    os << std::endl << "# heroes movement speed: 1 - 10" << std::endl;
    os << "heroes speed = " << heroes_speed << std::endl;

    os << std::endl << "# AI movement speed: 0 - 10" << std::endl;
    os << "ai speed = " << ai_speed << std::endl;

    os << std::endl << "# battle speed: 1 - 10" << std::endl;
    os << "battle speed = " << battle_speed << std::endl;

    os << std::endl << "# scroll speed: 1 - 4" << std::endl;
    os << "scroll speed = " << scroll_speed << std::endl;

    os << std::endl << "# show battle grid: on/off" << std::endl;
    os << "battle grid = " << ( opt_global.Modes( GLOBAL_BATTLE_SHOW_GRID ) ? "on" : "off" ) << std::endl;

    os << std::endl << "# show battle shadow movement: on/off" << std::endl;
    os << "battle shadow movement = " << ( opt_global.Modes( GLOBAL_BATTLE_SHOW_MOVE_SHADOW ) ? "on" : "off" ) << std::endl;

    os << std::endl << "# show battle shadow cursor: on/off" << std::endl;
    os << "battle shadow cursor = " << ( opt_global.Modes( GLOBAL_BATTLE_SHOW_MOUSE_SHADOW ) ? "on" : "off" ) << std::endl;

    os << std::endl << "# auto resolve battles: on/off" << std::endl;
    os << "auto resolve battles = " << ( opt_global.Modes( GLOBAL_BATTLE_AUTO_RESOLVE ) ? "on" : "off" ) << std::endl;

    os << std::endl << "# auto combat spell casting: on/off" << std::endl;
    os << "auto spell casting = " << ( opt_global.Modes( GLOBAL_BATTLE_AUTO_SPELLCAST ) ? "on" : "off" ) << std::endl;

    os << std::endl << "# show army order during battle: on/off" << std::endl;
    os << "battle army order = " << ( opt_global.Modes( GLOBAL_BATTLE_SHOW_ARMY_ORDER ) ? "on" : "off" ) << std::endl;

    os << std::endl << "# game language (an empty value means English)" << std::endl;
    os << "lang = " << _gameLanguage << std::endl;

    os << std::endl << "# controller pointer speed: 0 - 100" << std::endl;
    os << "controller pointer speed = " << _controllerPointerSpeed << std::endl;

    os << std::endl << "# first time game run (show additional hints): on/off" << std::endl;
    os << "first time game run = " << ( opt_global.Modes( GLOBAL_FIRST_RUN ) ? "on" : "off" ) << std::endl;

    os << std::endl << "# show game intro (splash screen and video): on/off" << std::endl;
    os << "show game intro = " << ( opt_global.Modes( GLOBAL_SHOW_INTRO ) ? "on" : "off" ) << std::endl;

    os << std::endl << "# enable V-Sync (Vertical Synchronization) for rendering" << std::endl;
    os << "v-sync = " << ( opt_global.Modes( GLOBAL_RENDER_VSYNC ) ? "on" : "off" ) << std::endl;

    return os.str();
}

/* read maps info */
void Settings::SetCurrentFileInfo( const Maps::FileInfo & fi )
{
    current_maps_file = fi;

    players.Init( current_maps_file );

    preferably_count_players = 0;
}

const Maps::FileInfo & Settings::CurrentFileInfo() const
{
    return current_maps_file;
}

bool Settings::isCurrentMapPriceOfLoyalty() const
{
    return current_maps_file._version == GameVersion::PRICE_OF_LOYALTY;
}

/* return debug */
int Settings::Debug() const
{
    return debug;
}

/* return game difficulty */
int Settings::GameDifficulty() const
{
    return game_difficulty;
}

const std::string & Settings::getGameLanguage() const
{
    return _gameLanguage;
}

bool Settings::setGameLanguage( const std::string & language )
{
    fheroes2::updateAlphabet( language );

    Translation::setStripContext( '|' );

    _gameLanguage = language;

    if ( _gameLanguage.empty() ) {
        Translation::reset();
        return true;
    }

    const std::string fileName = std::string( _gameLanguage ).append( ".mo" );
    const ListFiles translations = Settings::FindFiles( System::ConcatePath( "files", "lang" ), fileName, false );

    if ( !translations.empty() ) {
        return Translation::bindDomain( language.c_str(), translations.back().c_str() ) && Translation::setDomain( language.c_str() );
    }

    ERROR_LOG( "Translation file " << fileName << " is not found." )
    return false;
}

const std::string & Settings::loadedFileLanguage() const
{
    return _loadedFileLanguage;
}

void Settings::SetMapsFile( const std::string & file )
{
    current_maps_file.file = file;
}

void Settings::SetProgramPath( const char * argv0 )
{
    if ( argv0 )
        path_program = argv0;
}

ListDirs Settings::GetRootDirs()
{
    ListDirs dirs;

    // from build
#ifdef CONFIGURE_FHEROES2_DATA
    dirs.push_back( EXPANDDEF( CONFIGURE_FHEROES2_DATA ) );
#endif

    // from env
    if ( getenv( "FHEROES2_DATA" ) )
        dirs.push_back( getenv( "FHEROES2_DATA" ) );

    // from app path
    dirs.push_back( System::GetDirname( Settings::Get().path_program ) );

    // os-specific directories
    dirs.splice( dirs.end(), System::GetOSSpecificDirectories() );

    // user config directory
    const std::string & config = System::GetConfigDirectory( "fheroes2" );
    if ( !config.empty() )
        dirs.push_back( config );

    // user data directory (may be the same as user config directory, so check this to avoid unnecessary work)
    const std::string & data = System::GetDataDirectory( "fheroes2" );
    if ( !data.empty() && ( std::find( dirs.cbegin(), dirs.cend(), data ) == dirs.cend() ) )
        dirs.push_back( data );

    return dirs;
}

ListFiles Settings::FindFiles( const std::string & prefixDir, const std::string & fileNameFilter, const bool exactMatch )
{
    ListFiles res;

    for ( const std::string & dir : GetRootDirs() ) {
        const std::string path = !prefixDir.empty() ? System::ConcatePath( dir, prefixDir ) : dir;

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

std::string Settings::GetLastFile( const std::string & prefix, const std::string & name )
{
    const ListFiles & files = FindFiles( prefix, name, true );
    return files.empty() ? name : files.back();
}

bool Settings::MusicMIDI() const
{
    return opt_global.Modes( GLOBAL_MUSIC_MIDI );
}

/* return move speed */
int Settings::HeroesMoveSpeed() const
{
    return heroes_speed;
}
int Settings::AIMoveSpeed() const
{
    return ai_speed;
}
int Settings::BattleSpeed() const
{
    return battle_speed;
}

/* return scroll speed */
int Settings::ScrollSpeed() const
{
    return scroll_speed;
}

/* set ai speed: 0 (don't show) - 10 */
void Settings::SetAIMoveSpeed( int speed )
{
    ai_speed = clamp( speed, 0, 10 );
}

/* set hero speed: 1 - 10 */
void Settings::SetHeroesMoveSpeed( int speed )
{
    heroes_speed = clamp( speed, 1, 10 );
}

/* set battle speed: 1 - 10 */
void Settings::SetBattleSpeed( int speed )
{
    battle_speed = clamp( speed, 1, 10 );
}

void Settings::setBattleAutoResolve( bool enable )
{
    if ( enable ) {
        opt_global.SetModes( GLOBAL_BATTLE_AUTO_RESOLVE );
    }
    else {
        opt_global.ResetModes( GLOBAL_BATTLE_AUTO_RESOLVE );
    }
}

void Settings::setBattleAutoSpellcast( bool enable )
{
    if ( enable ) {
        opt_global.SetModes( GLOBAL_BATTLE_AUTO_SPELLCAST );
    }
    else {
        opt_global.ResetModes( GLOBAL_BATTLE_AUTO_SPELLCAST );
    }
}

void Settings::setBattleShowArmyOrder( const bool enable )
{
    if ( enable ) {
        opt_global.SetModes( GLOBAL_BATTLE_SHOW_ARMY_ORDER );
    }
    else {
        opt_global.ResetModes( GLOBAL_BATTLE_SHOW_ARMY_ORDER );
    }
}

void Settings::setFullScreen( const bool enable )
{
    if ( enable ) {
        opt_global.SetModes( GLOBAL_FULLSCREEN );
    }
    else {
        opt_global.ResetModes( GLOBAL_FULLSCREEN );
    }
}

/* set scroll speed: 1 - 4 */
void Settings::SetScrollSpeed( int speed )
{
    scroll_speed = clamp( speed, static_cast<int>( SCROLL_SLOW ), static_cast<int>( SCROLL_FAST2 ) );
}

bool Settings::isPriceOfLoyaltySupported() const
{
    return opt_global.Modes( GLOBAL_PRICELOYALTY );
}

bool Settings::LoadedGameVersion() const
{
    // 0x80 value should be same as in Game::TYPE_LOADFILE enumeration value
    // This constant not used here, to not drag dependency on the game.h and game.cpp in compilation target.
    return ( game_type & 0x80 ) != 0;
}

bool Settings::ShowControlPanel() const
{
    return opt_global.Modes( GLOBAL_SHOWCPANEL );
}

bool Settings::ShowRadar() const
{
    return opt_global.Modes( GLOBAL_SHOWRADAR );
}

bool Settings::ShowIcons() const
{
    return opt_global.Modes( GLOBAL_SHOWICONS );
}

bool Settings::ShowButtons() const
{
    return opt_global.Modes( GLOBAL_SHOWBUTTONS );
}

bool Settings::ShowStatus() const
{
    return opt_global.Modes( GLOBAL_SHOWSTATUS );
}

bool Settings::BattleShowGrid() const
{
    return opt_global.Modes( GLOBAL_BATTLE_SHOW_GRID );
}

bool Settings::BattleShowMouseShadow() const
{
    return opt_global.Modes( GLOBAL_BATTLE_SHOW_MOUSE_SHADOW );
}

bool Settings::BattleShowMoveShadow() const
{
    return opt_global.Modes( GLOBAL_BATTLE_SHOW_MOVE_SHADOW );
}

bool Settings::BattleAutoResolve() const
{
    return opt_global.Modes( GLOBAL_BATTLE_AUTO_RESOLVE );
}

bool Settings::BattleAutoSpellcast() const
{
    return opt_global.Modes( GLOBAL_BATTLE_AUTO_SPELLCAST );
}

bool Settings::BattleShowArmyOrder() const
{
    return opt_global.Modes( GLOBAL_BATTLE_SHOW_ARMY_ORDER );
}

const fheroes2::Size & Settings::VideoMode() const
{
    return video_mode;
}

/* set level debug */
void Settings::SetDebug( int d )
{
    debug = d;
    Logging::SetDebugLevel( debug );
}

void Settings::SetGameDifficulty( int d )
{
    game_difficulty = d;
}

/* sound volume: 0 - 10 */
void Settings::SetSoundVolume( int v )
{
    sound_volume = clamp( v, 0, 10 );
}

/* music volume: 0 - 10 */
void Settings::SetMusicVolume( int v )
{
    music_volume = clamp( v, 0, 10 );
}

/* Set music type: check MusicSource enum */
void Settings::SetMusicType( int v )
{
    _musicType = MUSIC_EXTERNAL <= v ? MUSIC_EXTERNAL : static_cast<MusicSource>( v );
}

bool Settings::isCampaignGameType() const
{
    return ( game_type & Game::TYPE_CAMPAIGN ) != 0;
}

void Settings::SetPreferablyCountPlayers( int c )
{
    preferably_count_players = std::min( c, 6 );
}

int Settings::PreferablyCountPlayers() const
{
    return preferably_count_players;
}

const std::string & Settings::MapsFile() const
{
    return current_maps_file.file;
}

const std::string & Settings::MapsName() const
{
    return current_maps_file.name;
}

const std::string & Settings::MapsDescription() const
{
    return current_maps_file.description;
}

int Settings::MapsDifficulty() const
{
    return current_maps_file.difficulty;
}

fheroes2::Size Settings::MapsSize() const
{
    return fheroes2::Size( current_maps_file.size_w, current_maps_file.size_h );
}

bool Settings::AllowChangeRace( int f ) const
{
    return ( current_maps_file.rnd_races & f ) != 0;
}

bool Settings::GameStartWithHeroes() const
{
    return current_maps_file.startWithHeroInEachCastle;
}

uint32_t Settings::ConditionWins() const
{
    return current_maps_file.ConditionWins();
}

uint32_t Settings::ConditionLoss() const
{
    return current_maps_file.ConditionLoss();
}

bool Settings::WinsCompAlsoWins() const
{
    return current_maps_file.WinsCompAlsoWins();
}

int Settings::WinsFindArtifactID() const
{
    return current_maps_file.WinsFindArtifactID();
}

bool Settings::WinsFindUltimateArtifact() const
{
    return current_maps_file.WinsFindUltimateArtifact();
}

u32 Settings::WinsAccumulateGold() const
{
    return current_maps_file.WinsAccumulateGold();
}

fheroes2::Point Settings::WinsMapsPositionObject() const
{
    return current_maps_file.WinsMapsPositionObject();
}

fheroes2::Point Settings::LossMapsPositionObject() const
{
    return current_maps_file.LossMapsPositionObject();
}

u32 Settings::LossCountDays() const
{
    return current_maps_file.LossCountDays();
}

int Settings::controllerPointerSpeed() const
{
    return _controllerPointerSpeed;
}

void Settings::EnablePriceOfLoyaltySupport( const bool set )
{
    if ( set ) {
        opt_global.SetModes( GLOBAL_PRICELOYALTY );
    }
    else {
        opt_global.ResetModes( GLOBAL_PRICELOYALTY );
        if ( _musicType == MUSIC_MIDI_EXPANSION )
            _musicType = MUSIC_MIDI_ORIGINAL;
    }
}

void Settings::SetEvilInterface( bool f )
{
    f ? ExtSetModes( GAME_EVIL_INTERFACE ) : ExtResetModes( GAME_EVIL_INTERFACE );
}

void Settings::SetHideInterface( bool f )
{
    f ? ExtSetModes( GAME_HIDE_INTERFACE ) : ExtResetModes( GAME_HIDE_INTERFACE );
}

void Settings::SetBattleGrid( bool f )
{
    f ? opt_global.SetModes( GLOBAL_BATTLE_SHOW_GRID ) : opt_global.ResetModes( GLOBAL_BATTLE_SHOW_GRID );
}

void Settings::SetBattleMovementShaded( bool f )
{
    f ? opt_global.SetModes( GLOBAL_BATTLE_SHOW_MOVE_SHADOW ) : opt_global.ResetModes( GLOBAL_BATTLE_SHOW_MOVE_SHADOW );
}

void Settings::SetBattleMouseShaded( bool f )
{
    f ? opt_global.SetModes( GLOBAL_BATTLE_SHOW_MOUSE_SHADOW ) : opt_global.ResetModes( GLOBAL_BATTLE_SHOW_MOUSE_SHADOW );
}

void Settings::SetShowPanel( bool f )
{
    f ? opt_global.SetModes( GLOBAL_SHOWCPANEL ) : opt_global.ResetModes( GLOBAL_SHOWCPANEL );
}

void Settings::SetShowRadar( bool f )
{
    f ? opt_global.SetModes( GLOBAL_SHOWRADAR ) : opt_global.ResetModes( GLOBAL_SHOWRADAR );
}

void Settings::SetShowIcons( bool f )
{
    f ? opt_global.SetModes( GLOBAL_SHOWICONS ) : opt_global.ResetModes( GLOBAL_SHOWICONS );
}

void Settings::SetShowButtons( bool f )
{
    f ? opt_global.SetModes( GLOBAL_SHOWBUTTONS ) : opt_global.ResetModes( GLOBAL_SHOWBUTTONS );
}

void Settings::SetShowStatus( bool f )
{
    f ? opt_global.SetModes( GLOBAL_SHOWSTATUS ) : opt_global.ResetModes( GLOBAL_SHOWSTATUS );
}

bool Settings::CanChangeInGame( u32 f ) const
{
    return ( f >> 28 ) == 0x01; // GAME_ and POCKETPC_
}

bool Settings::ExtModes( u32 f ) const
{
    const u32 mask = 0x0FFFFFFF;
    switch ( f >> 28 ) {
    case 0x01:
        return opt_game.Modes( f & mask );
    case 0x02:
        return opt_world.Modes( f & mask );
    case 0x03:
        return opt_addons.Modes( f & mask );
    case 0x04:
        return opt_battle.Modes( f & mask );
    default:
        break;
    }
    return false;
}

std::string Settings::ExtName( const uint32_t settingId )
{
    switch ( settingId ) {
    case Settings::GAME_SAVE_REWRITE_CONFIRM:
        return _( "game: always confirm for rewrite savefile" );
    case Settings::GAME_REMEMBER_LAST_FOCUS:
        return _( "game: remember last focus" );
    case Settings::GAME_BATTLE_SHOW_DAMAGE:
        return _( "battle: show damage info" );
    case Settings::WORLD_SHOW_TERRAIN_PENALTY:
        return _( "world: show terrain penalty" );
    case Settings::WORLD_SCOUTING_EXTENDED:
        return _( "world: scouting skill shows extended content info" );
    case Settings::WORLD_ALLOW_SET_GUARDIAN:
        return _( "world: allow to set guardian to objects" );
    case Settings::WORLD_EYE_EAGLE_AS_SCHOLAR:
        return _( "world: Eagle Eye also works like Scholar in H3." );
    case Settings::WORLD_ARTIFACT_CRYSTAL_BALL:
        return _( "world: Crystal Ball gives Identify Hero and Visions spells" );
    case Settings::WORLD_SCALE_NEUTRAL_ARMIES:
        return _( "world: Neutral armies scale with game difficulty" );
    case Settings::WORLD_USE_UNIQUE_ARTIFACTS_RS:
        return _( "world: use unique artifacts providing resources" );
    case Settings::WORLD_USE_UNIQUE_ARTIFACTS_PS:
        return _( "world: use unique artifacts affecting primary skills" );
    case Settings::WORLD_USE_UNIQUE_ARTIFACTS_SS:
        return _( "world: use unique artifacts affecting secondary skills" );
    case Settings::WORLD_EXT_OBJECTS_CAPTURED:
        return _( "world: Wind Mills, Water Wheels and Magic Gardens can be captured" );
    case Settings::WORLD_DISABLE_BARROW_MOUNDS:
        return _( "world: disable Barrow Mounds" );
    case Settings::CASTLE_ALLOW_GUARDIANS:
        return _( "castle: allow guardians" );
    case Settings::CASTLE_MAGEGUILD_POINTS_TURN:
        return _( "castle: higher mage guilds regenerate more spell points/turn (20/40/60/80/100%)" );
    case Settings::HEROES_BUY_BOOK_FROM_SHRINES:
        return _( "heroes: allow buy a spellbook from Shrines" );
    case Settings::HEROES_COST_DEPENDED_FROM_LEVEL:
        return _( "heroes: recruit cost depends on hero level" );
    case Settings::HEROES_REMEMBER_POINTS_RETREAT:
        return _( "heroes: remember move points for retreat/surrender result" );
    case Settings::HEROES_TRANSCRIBING_SCROLLS:
        return _( "heroes: allow transcribing scrolls (needs: Eye Eagle skill)" );
    case Settings::HEROES_ARENA_ANY_SKILLS:
        return _( "heroes: allow to choose any primary skill in Arena" );
    case Settings::BATTLE_SOFT_WAITING:
        return _( "battle: allow soft wait for troops" );
    case Settings::BATTLE_REVERSE_WAIT_ORDER:
        return _( "battle: reverse wait order (fast, average, slow)" );
    case Settings::BATTLE_DETERMINISTIC_RESULT:
        return _( "battle: deterministic events" );
    case Settings::GAME_SHOW_SYSTEM_INFO:
        return _( "game: show system info" );
    case Settings::GAME_AUTOSAVE_BEGIN_DAY:
        return _( "game: autosave will be made at the beginning of the day" );
    case Settings::GAME_USE_FADE:
        return _( "game: use fade" );
    case Settings::GAME_EVIL_INTERFACE:
        return _( "game: use evil interface" );
    case Settings::GAME_HIDE_INTERFACE:
        return _( "game: hide interface" );
    case Settings::GAME_CONTINUE_AFTER_VICTORY:
        return _( "game: offer to continue the game afer victory condition" );
    default:
        break;
    }

    return std::string();
}

void Settings::ExtSetModes( u32 f )
{
    const u32 mask = 0x0FFFFFFF;
    switch ( f >> 28 ) {
    case 0x01:
        opt_game.SetModes( f & mask );
        break;
    case 0x02:
        opt_world.SetModes( f & mask );
        break;
    case 0x03:
        opt_addons.SetModes( f & mask );
        break;
    case 0x04:
        opt_battle.SetModes( f & mask );
        break;
    default:
        break;
    }
}

void Settings::ExtResetModes( u32 f )
{
    const u32 mask = 0x0FFFFFFF;
    switch ( f >> 28 ) {
    case 0x01:
        opt_game.ResetModes( f & mask );
        break;
    case 0x02:
        opt_world.ResetModes( f & mask );
        break;
    case 0x03:
        opt_addons.ResetModes( f & mask );
        break;
    case 0x04:
        opt_battle.ResetModes( f & mask );
        break;
    default:
        break;
    }
}

bool Settings::ExtCastleGuildRestorePointsTurn() const
{
    return ExtModes( CASTLE_MAGEGUILD_POINTS_TURN );
}

bool Settings::ExtCastleAllowGuardians() const
{
    return ExtModes( CASTLE_ALLOW_GUARDIANS );
}

bool Settings::ExtWorldShowTerrainPenalty() const
{
    return ExtModes( WORLD_SHOW_TERRAIN_PENALTY );
}

bool Settings::ExtWorldScouteExtended() const
{
    return ExtModes( WORLD_SCOUTING_EXTENDED );
}

bool Settings::ExtGameRememberLastFocus() const
{
    return ExtModes( GAME_REMEMBER_LAST_FOCUS );
}

bool Settings::ExtWorldAllowSetGuardian() const
{
    return ExtModes( WORLD_ALLOW_SET_GUARDIAN );
}

bool Settings::ExtWorldArtifactCrystalBall() const
{
    return ExtModes( WORLD_ARTIFACT_CRYSTAL_BALL );
}

bool Settings::ExtWorldEyeEagleAsScholar() const
{
    return ExtModes( WORLD_EYE_EAGLE_AS_SCHOLAR );
}

bool Settings::ExtHeroBuySpellBookFromShrine() const
{
    return ExtModes( HEROES_BUY_BOOK_FROM_SHRINES );
}

bool Settings::ExtHeroRecruitCostDependedFromLevel() const
{
    return ExtModes( HEROES_COST_DEPENDED_FROM_LEVEL );
}

bool Settings::ExtHeroRememberPointsForRetreating() const
{
    return ExtModes( HEROES_REMEMBER_POINTS_RETREAT );
}

bool Settings::ExtBattleShowDamage() const
{
    return ExtModes( GAME_BATTLE_SHOW_DAMAGE );
}

bool Settings::ExtHeroAllowTranscribingScroll() const
{
    return ExtModes( HEROES_TRANSCRIBING_SCROLLS );
}

bool Settings::ExtBattleSoftWait() const
{
    return ExtModes( BATTLE_SOFT_WAITING );
}

bool Settings::ExtBattleDeterministicResult() const
{
    return ExtModes( BATTLE_DETERMINISTIC_RESULT );
}

bool Settings::ExtGameRewriteConfirm() const
{
    return ExtModes( GAME_SAVE_REWRITE_CONFIRM );
}

bool Settings::ExtGameShowSystemInfo() const
{
    return ExtModes( GAME_SHOW_SYSTEM_INFO );
}

bool Settings::ExtGameAutosaveBeginOfDay() const
{
    return ExtModes( GAME_AUTOSAVE_BEGIN_DAY );
}

bool Settings::ExtGameUseFade() const
{
    return video_mode == fheroes2::Size( fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT ) && ExtModes( GAME_USE_FADE );
}

bool Settings::ExtGameEvilInterface() const
{
    return ExtModes( GAME_EVIL_INTERFACE );
}

bool Settings::ExtGameHideInterface() const
{
    return ExtModes( GAME_HIDE_INTERFACE );
}

bool Settings::ExtBattleReverseWaitOrder() const
{
    return ExtModes( BATTLE_REVERSE_WAIT_ORDER );
}

bool Settings::ExtWorldNeutralArmyDifficultyScaling() const
{
    return ExtModes( WORLD_SCALE_NEUTRAL_ARMIES );
}

bool Settings::ExtWorldUseUniqueArtifactsRS() const
{
    return ExtModes( WORLD_USE_UNIQUE_ARTIFACTS_RS );
}

bool Settings::ExtWorldUseUniqueArtifactsPS() const
{
    return ExtModes( WORLD_USE_UNIQUE_ARTIFACTS_PS );
}

bool Settings::ExtWorldUseUniqueArtifactsSS() const
{
    return ExtModes( WORLD_USE_UNIQUE_ARTIFACTS_SS );
}

bool Settings::ExtHeroArenaCanChoiseAnySkills() const
{
    return ExtModes( HEROES_ARENA_ANY_SKILLS );
}

bool Settings::ExtWorldExtObjectsCaptured() const
{
    return ExtModes( WORLD_EXT_OBJECTS_CAPTURED );
}

bool Settings::ExtWorldDisableBarrowMounds() const
{
    return ExtModes( WORLD_DISABLE_BARROW_MOUNDS );
}

bool Settings::ExtGameContinueAfterVictory() const
{
    return ExtModes( GAME_CONTINUE_AFTER_VICTORY );
}

const fheroes2::Point & Settings::PosRadar() const
{
    return pos_radr;
}

const fheroes2::Point & Settings::PosButtons() const
{
    return pos_bttn;
}

const fheroes2::Point & Settings::PosIcons() const
{
    return pos_icon;
}

const fheroes2::Point & Settings::PosStatus() const
{
    return pos_stat;
}

void Settings::SetPosRadar( const fheroes2::Point & pt )
{
    pos_radr = pt;
}

void Settings::SetPosButtons( const fheroes2::Point & pt )
{
    pos_bttn = pt;
}

void Settings::SetPosIcons( const fheroes2::Point & pt )
{
    pos_icon = pt;
}

void Settings::SetPosStatus( const fheroes2::Point & pt )
{
    pos_stat = pt;
}

void Settings::BinarySave() const
{
    const std::string fname = System::ConcatePath( System::GetConfigDirectory( "fheroes2" ), "fheroes2.bin" );

    StreamFile fs;
    fs.setbigendian( true );

    if ( fs.open( fname, "wb" ) ) {
        fs << static_cast<u16>( CURRENT_FORMAT_VERSION ) << opt_game << opt_world << opt_battle << opt_addons << pos_radr << pos_bttn << pos_icon << pos_stat;
    }
}

void Settings::BinaryLoad()
{
    std::string fname = System::ConcatePath( System::GetConfigDirectory( "fheroes2" ), "fheroes2.bin" );

    if ( !System::IsFile( fname ) )
        fname = GetLastFile( "", "fheroes2.bin" );

    StreamFile fs;
    fs.setbigendian( true );

    if ( fs.open( fname, "rb" ) ) {
        u16 version = 0;

        fs >> version >> opt_game >> opt_world >> opt_battle >> opt_addons >> pos_radr >> pos_bttn >> pos_icon >> pos_stat;
    }
}

bool Settings::FullScreen() const
{
    return System::isEmbededDevice() || opt_global.Modes( GLOBAL_FULLSCREEN );
}

bool Settings::isVSyncEnabled() const
{
    return opt_global.Modes( GLOBAL_RENDER_VSYNC );
}

bool Settings::isFirstGameRun() const
{
    return opt_global.Modes( GLOBAL_FIRST_RUN );
}

bool Settings::isShowIntro() const
{
    return opt_global.Modes( GLOBAL_SHOW_INTRO );
}

void Settings::resetFirstGameRun()
{
    opt_global.ResetModes( GLOBAL_FIRST_RUN );
}

StreamBase & operator<<( StreamBase & msg, const Settings & conf )
{
    msg << conf._gameLanguage << conf.current_maps_file << conf.game_difficulty << conf.game_type << conf.preferably_count_players << conf.debug << conf.opt_game
        << conf.opt_world << conf.opt_battle << conf.opt_addons << conf.players;

    return msg;
}

StreamBase & operator>>( StreamBase & msg, Settings & conf )
{
    msg >> conf._loadedFileLanguage;

    int debug;
    u32 opt_game = 0; // skip: settings

    // map file
    msg >> conf.current_maps_file >> conf.game_difficulty >> conf.game_type >> conf.preferably_count_players >> debug >> opt_game >> conf.opt_world >> conf.opt_battle
        >> conf.opt_addons >> conf.players;

#ifndef WITH_DEBUG
    conf.debug = debug;
#endif

    return msg;
}
