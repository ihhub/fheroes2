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
#include <fstream>

#include "difficulty.h"
#include "game.h"
#include "logging.h"
#include "save_format_version.h"
#include "screen.h"
#include "settings.h"
#include "system.h"
#include "tinyconfig.h"
#include "tools.h"
#include "translations.h"
#include "version.h"

namespace
{
    enum
    {
        GLOBAL_FIRST_RUN = 0x00000001,
        GLOBAL_SHOW_INTRO = 0x00000002,
        GLOBAL_PRICELOYALTY = 0x00000004,

        // UNUSED = 0x00000008,
        // UNUSED = 0x00000010,
        // UNUSED = 0x00000020,

        GLOBAL_SHOWCPANEL = 0x00000040,
        GLOBAL_SHOWRADAR = 0x00000080,
        GLOBAL_SHOWICONS = 0x00000100,
        GLOBAL_SHOWBUTTONS = 0x00000200,
        GLOBAL_SHOWSTATUS = 0x00000400,

        GLOBAL_FULLSCREEN = 0x00008000,
        GLOBAL_USESWSURFACE = 0x00010000,

        GLOBAL_SOUND = 0x00020000,
        GLOBAL_MUSIC_EXT = 0x00040000,
        GLOBAL_MUSIC_CD = 0x00080000,
        GLOBAL_MUSIC_MIDI = 0x00100000,

        GLOBAL_USEUNICODE = 0x00200000,
        GLOBAL_ALTRESOURCE = 0x00400000,

        GLOBAL_BATTLE_SHOW_GRID = 0x00800000,
        GLOBAL_BATTLE_SHOW_MOUSE_SHADOW = 0x01000000,
        GLOBAL_BATTLE_SHOW_MOVE_SHADOW = 0x02000000,
        GLOBAL_BATTLE_AUTO_RESOLVE = 0x04000000,
        GLOBAL_BATTLE_AUTO_SPELLCAST = 0x08000000,

        GLOBAL_MUSIC = GLOBAL_MUSIC_CD | GLOBAL_MUSIC_EXT | GLOBAL_MUSIC_MIDI
    };

    struct settings_t
    {
        u32 id;
        const char * str;

        bool operator==( u32 i ) const
        {
            return id && id == i;
        }
    };

    // external settings
    const settings_t settingsGeneral[] = {
        {
            GLOBAL_SOUND,
            "sound",
        },
        {
            GLOBAL_MUSIC_MIDI,
            "music",
        },
        {
            GLOBAL_FULLSCREEN,
            "fullscreen",
        },
        {
            GLOBAL_USEUNICODE,
            "unicode",
        },
        {
            GLOBAL_ALTRESOURCE,
            "alt resource",
        },
        {
            GLOBAL_USESWSURFACE,
            "use swsurface only",
        },
        {
            0,
            nullptr,
        },
    };

    // internal settings
    const settings_t settingsFHeroes2[] = {
        {
            Settings::GAME_SAVE_REWRITE_CONFIRM,
            _( "game: always confirm for rewrite savefile" ),
        },
        {
            Settings::GAME_REMEMBER_LAST_FOCUS,
            _( "game: remember last focus" ),
        },
        {
            Settings::GAME_BATTLE_SHOW_DAMAGE,
            _( "battle: show damage info" ),
        },
        {
            Settings::WORLD_SHOW_VISITED_CONTENT,
            _( "world: show visited content from objects" ),
        },
        {
            Settings::WORLD_SHOW_TERRAIN_PENALTY,
            _( "world: show terrain penalty" ),
        },
        {
            Settings::WORLD_SCOUTING_EXTENDED,
            _( "world: scouting skill show extended content info" ),
        },
        {
            Settings::WORLD_ALLOW_SET_GUARDIAN,
            _( "world: allow set guardian to objects" ),
        },
        {
            Settings::WORLD_EYE_EAGLE_AS_SCHOLAR,
            _( "world: Eagle Eye also works like Scholar in H3." ),
        },
        {
            Settings::WORLD_BAN_WEEKOF,
            _( "world: ban for WeekOf/MonthOf Monsters" ),
        },
        {
            Settings::WORLD_BAN_PLAGUES,
            _( "world: ban plagues months" ),
        },
        {
            Settings::WORLD_BAN_MONTHOF_MONSTERS,
            _( "world: Months Of Monsters do not place creatures on map" ),
        },
        {
            Settings::WORLD_ARTIFACT_CRYSTAL_BALL,
            _( "world: Crystal Ball also added Identify Hero and Visions spells" ),
        },
        {
            Settings::WORLD_STARTHERO_LOSSCOND4HUMANS,
            _( "world: Starting heroes as Loss Conditions for Human Players" ),
        },
        {
            Settings::WORLD_1HERO_HIRED_EVERY_WEEK,
            _( "world: Only 1 hero can be hired by the one player every week" ),
        },
        {
            Settings::CASTLE_1HERO_HIRED_EVERY_WEEK,
            _( "world: Each castle allows one hero to be recruited every week" ),
        },
        {
            Settings::WORLD_SCALE_NEUTRAL_ARMIES,
            _( "world: Neutral armies scale with game difficulty" ),
        },
        {
            Settings::WORLD_USE_UNIQUE_ARTIFACTS_RS,
            _( "world: use unique artifacts for resource affecting" ),
        },
        {
            Settings::WORLD_USE_UNIQUE_ARTIFACTS_PS,
            _( "world: use unique artifacts for primary skills" ),
        },
        {
            Settings::WORLD_USE_UNIQUE_ARTIFACTS_SS,
            _( "world: use unique artifacts for secondary skills" ),
        },
        {
            Settings::WORLD_EXT_OBJECTS_CAPTURED,
            _( "world: Wind/Water Mills and Magic Garden can be captured" ),
        },
        {
            Settings::WORLD_DISABLE_BARROW_MOUNDS,
            _( "world: disable Barrow Mounds" ),
        },
        {
            Settings::CASTLE_ALLOW_GUARDIANS,
            _( "castle: allow guardians" ),
        },
        {
            Settings::CASTLE_MAGEGUILD_POINTS_TURN,
            _( "castle: higher mage guilds regenerate more spell points/turn (20/40/60/80/100%)" ),
        },
        {
            Settings::HEROES_BUY_BOOK_FROM_SHRINES,
            _( "heroes: allow buy a spellbook from Shrines" ),
        },
        {
            Settings::HEROES_COST_DEPENDED_FROM_LEVEL,
            _( "heroes: recruit cost to be dependent on hero level" ),
        },
        {
            Settings::HEROES_REMEMBER_POINTS_RETREAT,
            _( "heroes: remember move points for retreat/surrender result" ),
        },
        {
            Settings::HEROES_TRANSCRIBING_SCROLLS,
            _( "heroes: allow transcribing scrolls (needs: Eye Eagle skill)" ),
        },
        {
            Settings::HEROES_ARENA_ANY_SKILLS,
            _( "heroes: in Arena can choose any of primary skills" ),
        },
        {
            Settings::UNIONS_ALLOW_HERO_MEETINGS,
            _( "unions: allow meeting heroes" ),
        },
        {
            Settings::UNIONS_ALLOW_CASTLE_VISITING,
            _( "unions: allow castle visiting" ),
        },
        {
            Settings::BATTLE_SHOW_ARMY_ORDER,
            _( "battle: show army order" ),
        },
        {
            Settings::BATTLE_SOFT_WAITING,
            _( "battle: soft wait troop" ),
        },
        {
            Settings::BATTLE_REVERSE_WAIT_ORDER,
            _( "battle: reverse wait order (fast, average, slow)" ),
        },
        {
            Settings::GAME_SHOW_SYSTEM_INFO,
            _( "game: show system info" ),
        },
        {
            Settings::GAME_AUTOSAVE_ON,
            _( "game: autosave on" ),
        },
        {
            Settings::GAME_AUTOSAVE_BEGIN_DAY,
            _( "game: autosave will be made at the beginning of the day" ),
        },
        {
            Settings::GAME_USE_FADE,
            _( "game: use fade" ),
        },
        {
            Settings::GAME_EVIL_INTERFACE,
            _( "game: use evil interface" ),
        },
        {
            Settings::GAME_HIDE_INTERFACE,
            _( "game: hide interface" ),
        },
        {
            Settings::GAME_CONTINUE_AFTER_VICTORY,
            _( "game: offer to continue the game afer victory condition" ),
        },

        { 0, nullptr },
    };

    const char * GetGeneralSettingDescription( int settingId )
    {
        const settings_t * ptr = settingsGeneral;
        while ( ptr->id != 0 ) {
            if ( ptr->id == static_cast<uint32_t>( settingId ) )
                return ptr->str;
            ++ptr;
        }
        return nullptr;
    }
}

std::string Settings::GetVersion()
{
    return std::to_string( MAJOR_VERSION ) + '.' + std::to_string( MINOR_VERSION ) + '.' + std::to_string( INTERMEDIATE_VERSION );
}

Settings::Settings()
    : debug( 0 )
    , video_mode( fheroes2::Size( fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT ) )
    , game_difficulty( Difficulty::NORMAL )
    , font_normal( "dejavusans.ttf" )
    , font_small( "dejavusans.ttf" )
    , size_normal( 15 )
    , size_small( 10 )
    , sound_volume( 6 )
    , music_volume( 6 )
    , _musicType( MUSIC_EXTERNAL )
    , _controllerPointerSpeed( 10 )
    , heroes_speed( DEFAULT_SPEED_DELAY )
    , ai_speed( DEFAULT_SPEED_DELAY )
    , scroll_speed( SCROLL_NORMAL )
    , battle_speed( DEFAULT_SPEED_DELAY )
    , game_type( 0 )
    , preferably_count_players( 0 )
{
    ExtSetModes( GAME_AUTOSAVE_ON );
    ExtSetModes( WORLD_SHOW_VISITED_CONTENT );

    opt_global.SetModes( GLOBAL_FIRST_RUN );
    opt_global.SetModes( GLOBAL_SHOW_INTRO );
    opt_global.SetModes( GLOBAL_SHOWRADAR );
    opt_global.SetModes( GLOBAL_SHOWICONS );
    opt_global.SetModes( GLOBAL_SHOWBUTTONS );
    opt_global.SetModes( GLOBAL_SHOWSTATUS );
    opt_global.SetModes( GLOBAL_MUSIC_EXT );
    opt_global.SetModes( GLOBAL_SOUND );

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

    Logging::SetDebugLevel( debug );

    // opt_globals
    const settings_t * ptr = settingsGeneral;
    while ( ptr->id ) {
        if ( config.Exists( ptr->str ) ) {
            if ( 0 == config.IntParams( ptr->str ) )
                opt_global.ResetModes( ptr->id );
            else
                opt_global.SetModes( ptr->id );
        }

        ++ptr;
    }

    if ( Unicode() ) {
        sval = config.StrParams( "maps charset" );
        if ( !sval.empty() )
            maps_charset = sval;

        sval = config.StrParams( "lang" );
        if ( !sval.empty() )
            force_lang = sval;

        sval = config.StrParams( "fonts normal" );
        if ( !sval.empty() )
            font_normal = sval;

        sval = config.StrParams( "fonts small" );
        if ( !sval.empty() )
            font_small = sval;

        ival = config.IntParams( "fonts normal size" );
        if ( 0 < ival )
            size_normal = ival;

        ival = config.IntParams( "fonts small size" );
        if ( 0 < ival )
            size_small = ival;
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
            if ( isPriceOfLoyaltySupported() )
                _musicType = MUSIC_MIDI_EXPANSION;
        }
        else if ( sval == "cd" ) {
            opt_global.ResetModes( GLOBAL_MUSIC );
            opt_global.SetModes( GLOBAL_MUSIC_CD );
            _musicType = MUSIC_CDROM;
        }
        else if ( sval == "external" ) {
            opt_global.ResetModes( GLOBAL_MUSIC );
            opt_global.SetModes( GLOBAL_MUSIC_EXT );
            _musicType = MUSIC_EXTERNAL;
        }
    }

    // sound volume
    if ( config.Exists( "sound volume" ) ) {
        sound_volume = config.IntParams( "sound volume" );
        if ( sound_volume > 10 )
            sound_volume = 10;
    }

    // music volume
    if ( config.Exists( "music volume" ) ) {
        music_volume = config.IntParams( "music volume" );
        if ( music_volume > 10 )
            music_volume = 10;
    }

    // move speed
    if ( config.Exists( "ai speed" ) ) {
        ai_speed = config.IntParams( "ai speed" );
        if ( ai_speed > 10 ) {
            ai_speed = 10;
        }
        if ( ai_speed < 0 ) {
            ai_speed = 0;
        }
    }

    if ( config.Exists( "heroes speed" ) ) {
        heroes_speed = config.IntParams( "heroes speed" );
        if ( heroes_speed > 10 ) {
            heroes_speed = 10;
        }
        if ( heroes_speed < 1 ) {
            heroes_speed = 1;
        }
    }

    // scroll speed
    switch ( config.IntParams( "scroll speed" ) ) {
    case 1:
        scroll_speed = SCROLL_SLOW;
        break;
    case 2:
        scroll_speed = SCROLL_NORMAL;
        break;
    case 3:
        scroll_speed = SCROLL_FAST1;
        break;
    case 4:
        scroll_speed = SCROLL_FAST2;
        break;
    default:
        scroll_speed = SCROLL_NORMAL;
        break;
    }

    if ( config.Exists( "battle speed" ) ) {
        battle_speed = config.IntParams( "battle speed" );
        if ( battle_speed > 10 ) {
            battle_speed = 10;
        }
        if ( battle_speed < 1 ) {
            battle_speed = 1;
        }
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

    // playmus command
    _externalMusicCommand = config.StrParams( "playmus command" );

    // videodriver
    sval = config.StrParams( "videodriver" );
    if ( !sval.empty() )
        video_driver = sval;

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

    if ( config.Exists( "controller pointer speed" ) ) {
        _controllerPointerSpeed = config.IntParams( "controller pointer speed" );
        if ( _controllerPointerSpeed > 100 )
            _controllerPointerSpeed = 100;
        else if ( _controllerPointerSpeed < 0 )
            _controllerPointerSpeed = 0;
    }

    if ( config.Exists( "first time game run" ) && config.StrParams( "first time game run" ) == "off" ) {
        resetFirstGameRun();
    }

    if ( config.Exists( "show game intro" ) ) {
        setShowIntro( config.StrParams( "show game intro" ) == "on" );
    }

#ifndef WITH_TTF
    opt_global.ResetModes( GLOBAL_USEUNICODE );
#endif

    if ( font_normal.empty() || font_small.empty() )
        opt_global.ResetModes( GLOBAL_USEUNICODE );

#ifdef BUILD_RELEASE
    // reset devel
    debug &= ~( DBG_DEVEL );
#endif
    BinaryLoad();

    if ( video_driver.size() )
        video_driver = StringLower( video_driver );

    if ( video_mode.width > 0 && video_mode.height > 0 )
        PostLoad();

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
    else if ( MusicType() == MUSIC_CDROM ) {
        musicType = "cd";
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

    os << std::endl << "# sound: on/off" << std::endl;
    os << "sound = " << ( opt_global.Modes( GLOBAL_SOUND ) ? "on" : "off" ) << std::endl;

    os << std::endl << "# music: original, expansion, cd, external" << std::endl;
    os << "music = " << musicType << std::endl;

    os << std::endl << "# sound volume: 0 - 10" << std::endl;
    os << "sound volume = " << sound_volume << std::endl;

    os << std::endl << "# music volume: 0 - 10" << std::endl;
    os << "music volume = " << music_volume << std::endl;

    os << std::endl << "# run in fullscreen mode: on/off (use F4 key to switch between modes)" << std::endl;
    os << GetGeneralSettingDescription( GLOBAL_FULLSCREEN ) << " = " << ( opt_global.Modes( GLOBAL_FULLSCREEN ) ? "on" : "off" ) << std::endl;

    os << std::endl << "# use alternative resources (no longer used)" << std::endl;
    os << "alt resource = " << ( opt_global.Modes( GLOBAL_ALTRESOURCE ) ? "on" : "off" ) << std::endl;

    os << std::endl << "# print debug messages (only for development, see src/engine/logging.h for possible values)" << std::endl;
    os << "debug = " << debug << std::endl;

    os << std::endl << "# heroes movement speed: 1 - 10" << std::endl;
    os << "heroes speed = " << heroes_speed << std::endl;

    os << std::endl << "# AI movement speed: 0 - 10" << std::endl;
    os << "ai speed = " << ai_speed << std::endl;

    os << std::endl << "# battle speed: 1 - 10" << std::endl;
    os << "battle speed = " << battle_speed << std::endl;

    os << std::endl << "# scroll speed: 1 - 4" << std::endl;
    os << "scroll speed = ";

    switch ( scroll_speed ) {
    case SCROLL_SLOW:
        os << 1;
        break;
    case SCROLL_NORMAL:
        os << 2;
        break;
    case SCROLL_FAST1:
        os << 3;
        break;
    case SCROLL_FAST2:
        os << 4;
        break;
    default:
        os << 2;
        break;
    }

    os << std::endl;

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

    if ( video_driver.size() ) {
        os << std::endl << "# sdl video driver, windows: windib, directx; wince: gapi, raw; linux: x11; other: see sdl manual (will be deprecated)" << std::endl;
        os << "videodriver = " << video_driver << std::endl;
    }

#ifdef WITH_TTF
    os << std::endl << "# options below are experimental and are currently disabled in the game" << std::endl;
    os << "fonts normal = " << font_normal << std::endl
       << "fonts small = " << font_small << std::endl
       << "fonts normal size = " << static_cast<int>( size_normal ) << std::endl
       << "fonts small size = " << static_cast<int>( size_small ) << std::endl
       << "unicode = " << ( opt_global.Modes( GLOBAL_USEUNICODE ) ? "on" : "off" ) << std::endl;
    if ( force_lang.size() )
        os << "lang = " << force_lang << std::endl;
#endif

    os << std::endl << "# controller pointer speed: 0 - 100" << std::endl;
    os << "controller pointer speed = " << _controllerPointerSpeed << std::endl;

    os << std::endl << "# first time game run (show additional hints): on/off" << std::endl;
    os << "first time game run = " << ( opt_global.Modes( GLOBAL_FIRST_RUN ) ? "on" : "off" ) << std::endl;

    os << std::endl << "# show game intro (splash screen and video): on/off" << std::endl;
    os << "show game intro = " << ( opt_global.Modes( GLOBAL_SHOW_INTRO ) ? "on" : "off" ) << std::endl;

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

int Settings::CurrentColor() const
{
    return players.current_color;
}

const std::string & Settings::SelectVideoDriver() const
{
    return video_driver;
}

/* return fontname */
const std::string & Settings::FontsNormal() const
{
    return font_normal;
}
const std::string & Settings::FontsSmall() const
{
    return font_small;
}
const std::string & Settings::ForceLang() const
{
    return force_lang;
}
const std::string & Settings::loadedFileLanguage() const
{
    return _loadedFileLanguage;
}
const std::string & Settings::MapsCharset() const
{
    return maps_charset;
}
int Settings::FontsNormalSize() const
{
    return size_normal;
}
int Settings::FontsSmallSize() const
{
    return size_small;
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
    dirs.push_back( CONFIGURE_FHEROES2_DATA );
#endif

    // from env
    if ( System::GetEnvironment( "FHEROES2_DATA" ) )
        dirs.push_back( System::GetEnvironment( "FHEROES2_DATA" ) );

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

    auto processDir = [&res, &fileNameFilter, exactMatch]( const std::string & dir ) {
        if ( exactMatch ) {
            res.FindFileInDir( dir, fileNameFilter, false );
        }
        else {
            res.ReadDir( dir, fileNameFilter, false );
        }
    };

    if ( !prefixDir.empty() && System::IsDirectory( prefixDir ) ) {
        processDir( prefixDir );
    }

    for ( const std::string & dir : GetRootDirs() ) {
        const std::string path = !prefixDir.empty() ? System::ConcatePath( dir, prefixDir ) : dir;

        if ( System::IsDirectory( path ) ) {
            processDir( path );
        }
    }

    return res;
}

std::string Settings::GetLastFile( const std::string & prefix, const std::string & name )
{
    const ListFiles & files = FindFiles( prefix, name, true );
    return files.empty() ? name : files.back();
}

std::string Settings::GetLangDir()
{
#ifdef CONFIGURE_FHEROES2_LOCALEDIR
    return std::string( CONFIGURE_FHEROES2_LOCALEDIR );
#else
    std::string res;
    const ListDirs dirs = GetRootDirs();

    for ( ListDirs::const_reverse_iterator it = dirs.rbegin(); it != dirs.rend(); ++it ) {
        res = System::ConcatePath( System::ConcatePath( *it, "files" ), "lang" );
        if ( System::IsDirectory( res ) )
            return res;
    }
#endif

    return "";
}

bool Settings::MusicExt() const
{
    return opt_global.Modes( GLOBAL_MUSIC_EXT );
}
bool Settings::MusicMIDI() const
{
    return opt_global.Modes( GLOBAL_MUSIC_MIDI );
}
bool Settings::MusicCD() const
{
    return opt_global.Modes( GLOBAL_MUSIC_CD );
}

/* return sound */
bool Settings::Sound() const
{
    return opt_global.Modes( GLOBAL_SOUND );
}

/* return music */
bool Settings::Music() const
{
    return opt_global.Modes( GLOBAL_MUSIC );
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

/* set ai speed: 1 - 10 */
void Settings::SetAIMoveSpeed( int speed )
{
    if ( speed < 0 ) {
        speed = 0;
    }
    if ( speed > 10 ) {
        speed = 10;
    }
    ai_speed = speed;
}

/* set hero speed: 1 - 10 */
void Settings::SetHeroesMoveSpeed( int speed )
{
    if ( speed < 1 ) {
        speed = 1;
    }
    if ( speed > 10 ) {
        speed = 10;
    }
    heroes_speed = speed;
}

/* set battle speed: 1 - 10 */
void Settings::SetBattleSpeed( int speed )
{
    if ( speed < 1 ) {
        speed = 1;
    }
    if ( speed > 10 ) {
        speed = 10;
    }
    battle_speed = speed;
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

void Settings::setFullScreen( const bool enable )
{
    if ( enable ) {
        opt_global.SetModes( GLOBAL_FULLSCREEN );
    }
    else {
        opt_global.ResetModes( GLOBAL_FULLSCREEN );
    }
}

void Settings::setShowIntro( const bool enable )
{
    if ( enable ) {
        opt_global.SetModes( GLOBAL_SHOW_INTRO );
    }
    else {
        opt_global.ResetModes( GLOBAL_SHOW_INTRO );
    }
}

/* set scroll speed: 1 - 4 */
void Settings::SetScrollSpeed( int speed )
{
    switch ( speed ) {
    case SCROLL_SLOW:
        scroll_speed = SCROLL_SLOW;
        break;
    case SCROLL_NORMAL:
        scroll_speed = SCROLL_NORMAL;
        break;
    case SCROLL_FAST1:
        scroll_speed = SCROLL_FAST1;
        break;
    case SCROLL_FAST2:
        scroll_speed = SCROLL_FAST2;
        break;
    default:
        scroll_speed = SCROLL_NORMAL;
        break;
    }
}

bool Settings::UseAltResource() const
{
    return opt_global.Modes( GLOBAL_ALTRESOURCE );
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

/* unicode support */
bool Settings::Unicode() const
{
    return opt_global.Modes( GLOBAL_USEUNICODE );
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

/**/
void Settings::SetGameDifficulty( int d )
{
    game_difficulty = d;
}

void Settings::SetCurrentColor( int color )
{
    players.current_color = color;
}

int Settings::SoundVolume() const
{
    return sound_volume;
}
int Settings::MusicVolume() const
{
    return music_volume;
}
MusicSource Settings::MusicType() const
{
    return _musicType;
}

/* sound volume: 0 - 10 */
void Settings::SetSoundVolume( int v )
{
    sound_volume = 10 <= v ? 10 : v;
}

/* music volume: 0 - 10 */
void Settings::SetMusicVolume( int v )
{
    music_volume = 10 <= v ? 10 : v;
}

/* Set music type: check MusicSource enum */
void Settings::SetMusicType( int v )
{
    _musicType = MUSIC_CDROM <= v ? MUSIC_CDROM : static_cast<MusicSource>( v );
}

/* check game type */
bool Settings::IsGameType( int f ) const
{
    return ( game_type & f ) != 0;
}

int Settings::GameType() const
{
    return game_type;
}

/* set game type */
void Settings::SetGameType( int type )
{
    game_type = type;
}

bool Settings::isCampaignGameType() const
{
    return ( game_type & Game::TYPE_CAMPAIGN ) != 0;
}

const Players & Settings::GetPlayers() const
{
    return players;
}

Players & Settings::GetPlayers()
{
    return players;
}

void Settings::SetPreferablyCountPlayers( int c )
{
    preferably_count_players = 6 < c ? 6 : c;
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

const std::string & Settings::externalMusicCommand() const
{
    return _externalMusicCommand;
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

int Settings::ConditionWins() const
{
    return current_maps_file.ConditionWins();
}

int Settings::ConditionLoss() const
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

void Settings::SetUnicode( bool f )
{
    f ? opt_global.SetModes( GLOBAL_USEUNICODE ) : opt_global.ResetModes( GLOBAL_USEUNICODE );
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

void Settings::ResetSound()
{
    opt_global.ResetModes( GLOBAL_SOUND );
}

void Settings::ResetMusic()
{
    opt_global.ResetModes( GLOBAL_MUSIC );
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

const char * Settings::ExtName( u32 f ) const
{
    const settings_t * ptr = std::find( settingsFHeroes2, std::end( settingsFHeroes2 ) - 1, f );

    return ptr ? _( ptr->str ) : nullptr;
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

bool Settings::ExtWorldShowVisitedContent() const
{
    return ExtModes( WORLD_SHOW_VISITED_CONTENT );
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

bool Settings::ExtUnionsAllowCastleVisiting() const
{
    return ExtModes( UNIONS_ALLOW_CASTLE_VISITING );
}

bool Settings::ExtUnionsAllowHeroesMeetings() const
{
    return ExtModes( UNIONS_ALLOW_HERO_MEETINGS );
}

bool Settings::ExtBattleShowDamage() const
{
    return ExtModes( GAME_BATTLE_SHOW_DAMAGE );
}

bool Settings::ExtHeroAllowTranscribingScroll() const
{
    return ExtModes( HEROES_TRANSCRIBING_SCROLLS );
}

bool Settings::ExtBattleShowBattleOrder() const
{
    return ExtModes( BATTLE_SHOW_ARMY_ORDER );
}

bool Settings::ExtBattleSoftWait() const
{
    return ExtModes( BATTLE_SOFT_WAITING );
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

bool Settings::ExtGameAutosaveOn() const
{
    return ExtModes( GAME_AUTOSAVE_ON );
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

bool Settings::ExtWorldBanWeekOf() const
{
    return ExtModes( WORLD_BAN_WEEKOF );
}

bool Settings::ExtWorldBanMonthOfMonsters() const
{
    return ExtModes( WORLD_BAN_MONTHOF_MONSTERS );
}

bool Settings::ExtWorldBanPlagues() const
{
    return ExtModes( WORLD_BAN_PLAGUES );
}

bool Settings::ExtBattleReverseWaitOrder() const
{
    return ExtModes( BATTLE_REVERSE_WAIT_ORDER );
}

bool Settings::ExtWorldStartHeroLossCond4Humans() const
{
    return ExtModes( WORLD_STARTHERO_LOSSCOND4HUMANS );
}

bool Settings::ExtWorldOneHeroHiredEveryWeek() const
{
    return ExtModes( WORLD_1HERO_HIRED_EVERY_WEEK );
}

bool Settings::ExtCastleOneHeroHiredEveryWeek() const
{
    return ExtModes( CASTLE_1HERO_HIRED_EVERY_WEEK );
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
    msg << conf.force_lang << conf.current_maps_file << conf.game_difficulty << conf.game_type << conf.preferably_count_players << conf.debug << conf.opt_game
        << conf.opt_world << conf.opt_battle << conf.opt_addons << conf.players;

    return msg;
}

StreamBase & operator>>( StreamBase & msg, Settings & conf )
{
    msg >> conf._loadedFileLanguage;

    int debug;
    u32 opt_game = 0; // skip: settings

    // map file
    msg >> conf.current_maps_file;

    // TODO: once the minimum supported version will be FORMAT_VERSION_094_RELEASE remove this check.
    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_094_RELEASE, "Remove the check below" );

    if ( Game::GetLoadVersion() >= FORMAT_VERSION_094_RELEASE ) {
        msg >> conf.current_maps_file._version;
    }

    msg >> conf.game_difficulty >> conf.game_type >> conf.preferably_count_players >> debug >> opt_game >> conf.opt_world >> conf.opt_battle >> conf.opt_addons
        >> conf.players;

#ifndef WITH_DEBUG
    conf.debug = debug;
#endif

    return msg;
}
