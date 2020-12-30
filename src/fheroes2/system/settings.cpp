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

#include "audio_music.h"
#include "dialog.h"
#include "difficulty.h"
#include "game.h"
#include "maps.h"
#include "race.h"
#include "settings.h"
#include "text.h"
#include "tinyconfig.h"

#define DEFAULT_PORT 5154
#define DEFAULT_DEBUG DBG_ALL_WARN

bool IS_DEBUG( int name, int level )
{
    const int debug = Settings::Get().Debug();
    return ( ( DBG_ENGINE & name ) && ( ( DBG_ENGINE & debug ) >> 2 ) >= level ) || ( ( DBG_GAME & name ) && ( ( DBG_GAME & debug ) >> 4 ) >= level )
           || ( ( DBG_BATTLE & name ) && ( ( DBG_BATTLE & debug ) >> 6 ) >= level ) || ( ( DBG_AI & name ) && ( ( DBG_AI & debug ) >> 8 ) >= level )
           || ( ( DBG_NETWORK & name ) && ( ( DBG_NETWORK & debug ) >> 10 ) >= level ) || ( ( DBG_DEVEL & name ) && ( ( DBG_DEVEL & debug ) >> 12 ) >= level );
}

const char * StringDebug( int name )
{
    if ( name & DBG_ENGINE )
        return "DBG_ENGINE";
    else if ( name & DBG_GAME )
        return "DBG_GAME";
    else if ( name & DBG_BATTLE )
        return "DBG_BATTLE";
    else if ( name & DBG_AI )
        return "DBG_AI";
    else if ( name & DBG_NETWORK )
        return "DBG_NETWORK";
    else if ( name & DBG_OTHER )
        return "DBG_OTHER";
    else if ( name & DBG_DEVEL )
        return "DBG_DEVEL";
    return "";
}

enum
{
    // ??? = 0x00000001,
    // ??? = 0x00000002,
    GLOBAL_PRICELOYALTY = 0x00000004,

    GLOBAL_POCKETPC = 0x00000008,
    GLOBAL_DEDICATEDSERVER = 0x00000010,
    GLOBAL_LOCALCLIENT = 0x00000020,

    GLOBAL_SHOWCPANEL = 0x00000040,
    GLOBAL_SHOWRADAR = 0x00000080,
    GLOBAL_SHOWICONS = 0x00000100,
    GLOBAL_SHOWBUTTONS = 0x00000200,
    GLOBAL_SHOWSTATUS = 0x00000400,

    GLOBAL_CHANGE_FULLSCREEN_RESOLUTION = 0x00000800,
    GLOBAL_KEEP_ASPECT_RATIO = 0x00001000,
    GLOBAL_FONTRENDERBLENDED1 = 0x00002000,
    GLOBAL_FONTRENDERBLENDED2 = 0x00004000,
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

    GLOBAL_MUSIC = GLOBAL_MUSIC_CD | GLOBAL_MUSIC_EXT | GLOBAL_MUSIC_MIDI
};

struct settings_t
{
    u32 id;
    const char * str;

    bool operator==( const std::string & s ) const
    {
        return str && s == str;
    };
    bool operator==( u32 i ) const
    {
        return id && id == i;
    };
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
        GLOBAL_FULLSCREEN,
        "full screen",
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
        GLOBAL_POCKETPC,
        "pocketpc",
    },
    {
        GLOBAL_POCKETPC,
        "pocket pc",
    },
    {
        GLOBAL_USESWSURFACE,
        "use swsurface only",
    },
    {
        GLOBAL_KEEP_ASPECT_RATIO,
        "keep aspect ratio",
    },
    {
        GLOBAL_CHANGE_FULLSCREEN_RESOLUTION,
        "change fullscreen resolution",
    },
    {
        0,
        NULL,
    },
};

const char * GetGeneralSettingDescription( int settingId )
{
    const settings_t * ptr = settingsGeneral;
    while ( ptr->id != 0 ) {
        if ( ptr->id == static_cast<uint32_t>( settingId ) )
            return ptr->str;
        ++ptr;
    }
    return NULL;
}

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
        Settings::WORLD_SCOUTING_EXTENDED,
        _( "world: scouting skill show extended content info" ),
    },
    {
        Settings::WORLD_ABANDONED_MINE_RANDOM,
        _( "world: abandoned mine random resource" ),
    },
    {
        Settings::WORLD_ALLOW_SET_GUARDIAN,
        _( "world: allow set guardian to objects" ),
    },
    {
        Settings::WORLD_ONLY_FIRST_MONSTER_ATTACK,
        _( "world: only the first monster will attack (H2 bug)." ),
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
        Settings::WORLD_NEW_VERSION_WEEKOF,
        _( "world: new version WeekOf (+growth)" ),
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
        Settings::WORLD_USE_UNIQUE_ARTIFACTS_ML,
        _( "world: use unique artifacts for morale/luck" ),
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
        Settings::HEROES_SURRENDERING_GIVE_EXP,
        _( "heroes: surrendering gives some experience" ),
    },
    {
        Settings::HEROES_RECALCULATE_MOVEMENT,
        _( "heroes: recalculate movement points after creatures movement" ),
    },
    {
        Settings::HEROES_TRANSCRIBING_SCROLLS,
        _( "heroes: allow transcribing scrolls (needs: Eye Eagle skill)" ),
    },
    {
        Settings::HEROES_ALLOW_BANNED_SECSKILLS,
        _( "heroes: allow banned sec. skills upgrade" ),
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
        Settings::BATTLE_OBJECTS_ARCHERS_PENALTY,
        _( "battle: high objects are an obstacle for archers" ),
    },
    {
        Settings::BATTLE_SKIP_INCREASE_DEFENSE,
        _( "battle: skip increase +2 defense" ),
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
        Settings::GAME_DYNAMIC_INTERFACE,
        _( "game: also use dynamic interface for castles" ),
    },
    {
        Settings::GAME_HIDE_INTERFACE,
        _( "game: hide interface" ),
    },
    {
        Settings::GAME_CONTINUE_AFTER_VICTORY,
        _( "game: offer to continue the game afer victory condition" ),
    },
    {
        Settings::POCKETPC_TAP_MODE,
        _( "pocketpc: tap mode" ),
    },
    {
        Settings::POCKETPC_DRAG_DROP_SCROLL,
        _( "pocketpc: drag&drop gamearea as scroll" ),
    },

    {0, NULL},
};

std::string Settings::GetVersion( void )
{
    std::ostringstream os;
    os << static_cast<int>( MAJOR_VERSION ) << "." << static_cast<int>( MINOR_VERSION ) << "." << static_cast<int>( INTERMEDIATE_VERSION );
    return os.str();
}

/* constructor */
Settings::Settings()
    : debug( 0 )
    , video_mode( Size( fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT ) )
    , game_difficulty( Difficulty::NORMAL )
    , font_normal( "dejavusans.ttf" )
    , font_small( "dejavusans.ttf" )
    , size_normal( 15 )
    , size_small( 10 )
    , sound_volume( 6 )
    , music_volume( 6 )
    , _musicType( MUSIC_EXTERNAL )
    , heroes_speed( DEFAULT_SPEED_DELAY )
    , ai_speed( DEFAULT_SPEED_DELAY )
    , scroll_speed( SCROLL_NORMAL )
    , battle_speed( DEFAULT_SPEED_DELAY )
    , game_type( 0 )
    , preferably_count_players( 0 )
    , port( DEFAULT_PORT )
{
    ExtSetModes( GAME_AUTOSAVE_ON );
    ExtSetModes( WORLD_SHOW_VISITED_CONTENT );
    ExtSetModes( WORLD_ONLY_FIRST_MONSTER_ATTACK );

    opt_global.SetModes( GLOBAL_SHOWRADAR );
    opt_global.SetModes( GLOBAL_SHOWICONS );
    opt_global.SetModes( GLOBAL_SHOWBUTTONS );
    opt_global.SetModes( GLOBAL_SHOWSTATUS );
    opt_global.SetModes( GLOBAL_MUSIC_EXT );
    opt_global.SetModes( GLOBAL_SOUND );
    // Set expansion version by default - turn off if heroes2x.agg not found
    opt_global.SetModes( GLOBAL_PRICELOYALTY );

    opt_global.SetModes( GLOBAL_BATTLE_SHOW_GRID );
    opt_global.SetModes( GLOBAL_BATTLE_SHOW_MOUSE_SHADOW );
    opt_global.SetModes( GLOBAL_BATTLE_SHOW_MOVE_SHADOW );

    if ( System::isEmbededDevice() ) {
        opt_global.SetModes( GLOBAL_POCKETPC );
        ExtSetModes( POCKETPC_TAP_MODE );
        ExtSetModes( POCKETPC_DRAG_DROP_SCROLL );
    }
}

Settings::~Settings()
{
    if ( !LoadedGameVersion() )
        BinarySave();
}

Settings & Settings::Get( void )
{
    static Settings conf;

    return conf;
}

bool Settings::Read( const std::string & filename )
{
    TinyConfig config( '=', '#' );
    std::string sval;
    int ival;
    LocalEvent & le = LocalEvent::Get();

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

    // maps directories
    maps_params.Append( config.ListStr( "maps" ) );
    maps_params.sort();
    maps_params.unique();

    // data
    sval = config.StrParams( "data" );
    if ( !sval.empty() )
        data_params = sval;

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

        if ( config.StrParams( "fonts small render" ) == "blended" )
            opt_global.SetModes( GLOBAL_FONTRENDERBLENDED1 );
        if ( config.StrParams( "fonts normal render" ) == "blended" )
            opt_global.SetModes( GLOBAL_FONTRENDERBLENDED2 );
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
            if ( PriceLoyaltyVersion() )
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
        if ( ai_speed < 1 ) {
            ai_speed = 1;
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

    // network port
    port = config.Exists( "port" ) ? config.IntParams( "port" ) : DEFAULT_PORT;

    // playmus command
    sval = config.StrParams( "playmus command" );
    if ( !sval.empty() )
        Music::SetExtCommand( sval );

    // videodriver
    sval = config.StrParams( "videodriver" );
    if ( !sval.empty() )
        video_driver = sval;

    // pocketpc
    if ( PocketPC() ) {
        ival = config.IntParams( "pointer offset x" );
        if ( ival )
            le.SetMouseOffsetX( ival );

        ival = config.IntParams( "pointer offset y" );
        if ( ival )
            le.SetMouseOffsetY( ival );

        ival = config.IntParams( "tap delay" );
        if ( ival )
            le.SetTapDelayForRightClickEmulation( ival );

        sval = config.StrParams( "pointer rotate fix" );
        if ( !sval.empty() )
            System::SetEnvironment( "GAPI_POINTER_FIX", sval.c_str() );
    }

    // videomode
    sval = config.StrParams( "videomode" );
    if ( !sval.empty() ) {
        // default
        video_mode.w = fheroes2::Display::DEFAULT_WIDTH;
        video_mode.h = fheroes2::Display::DEFAULT_HEIGHT;

        std::string value = StringLower( sval );
        const size_t pos = value.find( 'x' );

        if ( std::string::npos != pos ) {
            std::string width( value.substr( 0, pos ) );
            std::string height( value.substr( pos + 1, value.length() - pos - 1 ) );

            video_mode.w = GetInt( width );
            video_mode.h = GetInt( height );
        }
        else {
            DEBUG( DBG_ENGINE, DBG_WARN, "unknown video mode: " << value );
        }
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

    if ( video_mode.w && video_mode.h )
        PostLoad();

    return true;
}

void Settings::PostLoad( void )
{
    if ( opt_global.Modes( GLOBAL_POCKETPC ) )
        opt_global.SetModes( GLOBAL_FULLSCREEN );
    else {
        ExtResetModes( POCKETPC_TAP_MODE );
    }

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
    file.open( filename.data(), std::fstream::out | std::fstream::trunc );
    if ( !file )
        return false;

    const std::string & data = String();
    file.write( data.data(), data.size() );

    return true;
}

std::string Settings::String( void ) const
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

    os << "# fheroes2 configuration file (saved under version " << GetVersion() << ")" << std::endl;

    os << std::endl << "# path to directory data" << std::endl;
    os << "data = " << data_params << std::endl;

    os << std::endl << "# path to directory maps (you can set few map directies)" << std::endl;
    for ( ListDirs::const_iterator it = maps_params.begin(); it != maps_params.end(); ++it )
        os << "maps = " << *it << std::endl;

    os << std::endl << "# video mode (game resolution)" << std::endl;
    os << "videomode = " << fheroes2::Display::instance().width() << "x" << fheroes2::Display::instance().height() << std::endl;

    os << std::endl << "# sound: on off" << std::endl;
    os << "sound = " << ( opt_global.Modes( GLOBAL_SOUND ) ? "on" : "off" ) << std::endl;

    os << std::endl << "# music: original, expansion, cd, external" << std::endl;
    os << "music = " << musicType << std::endl;

    os << std::endl << "# sound volume: 0 - 10" << std::endl;
    os << "sound volume = " << sound_volume << std::endl;

    os << std::endl << "# music volume: 0 - 10" << std::endl;
    os << "music volume = " << music_volume << std::endl;

    os << std::endl << "# keep aspect ratio in fullscreen mode (experimental)" << std::endl;
    os << GetGeneralSettingDescription( GLOBAL_KEEP_ASPECT_RATIO ) << " = " << ( opt_global.Modes( GLOBAL_KEEP_ASPECT_RATIO ) ? "on" : "off" ) << std::endl;

    os << std::endl << "# change resolution in fullscreen mode (experimental)" << std::endl;
    os << GetGeneralSettingDescription( GLOBAL_CHANGE_FULLSCREEN_RESOLUTION ) << " = " << ( opt_global.Modes( GLOBAL_CHANGE_FULLSCREEN_RESOLUTION ) ? "on" : "off" )
       << std::endl;

    os << std::endl << "# run in fullscreen mode: on off (use F4 key to switch between)" << std::endl;
    os << GetGeneralSettingDescription( GLOBAL_FULLSCREEN ) << " = " << ( opt_global.Modes( GLOBAL_FULLSCREEN ) ? "on" : "off" ) << std::endl;

    os << std::endl << "# use alternative resources (not in use anymore)" << std::endl;
    os << "alt resource = " << ( opt_global.Modes( GLOBAL_ALTRESOURCE ) ? "on" : "off" ) << std::endl;

    os << std::endl << "# run in debug mode (0 - 11) [only for development]" << std::endl;
    os << "debug = " << debug << std::endl;

    os << std::endl << "# heroes move speed: 0 - 10" << std::endl;
    os << "heroes speed = " << heroes_speed << std::endl;

    os << std::endl << "# AI move speed: 0 - 10" << std::endl;
    os << "ai speed = " << ai_speed << std::endl;

    os << std::endl << "# battle speed: 0 - 10" << std::endl;
    os << "battle speed = " << battle_speed << std::endl;

    os << std::endl << "# scroll speed: 1 - 4" << std::endl;
    os << "scroll speed = " << scroll_speed << std::endl;

    os << std::endl << "# show battle grid: on off" << std::endl;
    os << "battle grid = " << ( opt_global.Modes( GLOBAL_BATTLE_SHOW_GRID ) ? "on" : "off" ) << std::endl;

    os << std::endl << "# show battle shadow movement: on off" << std::endl;
    os << "battle shadow movement = " << ( opt_global.Modes( GLOBAL_BATTLE_SHOW_MOVE_SHADOW ) ? "on" : "off" ) << std::endl;

    os << std::endl << "# show battle shadow cursor: on off" << std::endl;
    os << "battle shadow cursor = " << ( opt_global.Modes( GLOBAL_BATTLE_SHOW_MOUSE_SHADOW ) ? "on" : "off" ) << std::endl;

    if ( video_driver.size() ) {
        os << std::endl << "# sdl video driver, windows: windib, directx, wince: gapi, raw, linux: x11, other see sdl manual (to be deprecated)" << std::endl;
        os << "videodriver = " << video_driver << std::endl;
    }

#ifdef WITH_TTF
    os << std::endl << "Below options are experimental and disabled in the game for now" << std::endl;
    os << "fonts normal = " << font_normal << std::endl
       << "fonts small = " << font_small << std::endl
       << "fonts normal size = " << static_cast<int>( size_normal ) << std::endl
       << "fonts small size = " << static_cast<int>( size_small ) << std::endl
       << "unicode = " << ( opt_global.Modes( GLOBAL_USEUNICODE ) ? "on" : "off" ) << std::endl;
    if ( force_lang.size() )
        os << "lang = " << force_lang << std::endl;
#endif

    return os.str();
}

/* read maps info */
void Settings::SetCurrentFileInfo( const Maps::FileInfo & fi )
{
    current_maps_file = fi;

    players.Init( current_maps_file );

    // game difficulty
    game_difficulty = Difficulty::NORMAL;
    preferably_count_players = 0;
}

const Maps::FileInfo & Settings::CurrentFileInfo( void ) const
{
    return current_maps_file;
}

/* return debug */
int Settings::Debug( void ) const
{
    return debug;
}

/* return game difficulty */
int Settings::GameDifficulty( void ) const
{
    return game_difficulty;
}

int Settings::CurrentColor( void ) const
{
    return players.current_color;
}

const std::string & Settings::SelectVideoDriver( void ) const
{
    return video_driver;
}

/* return fontname */
const std::string & Settings::FontsNormal( void ) const
{
    return font_normal;
}
const std::string & Settings::FontsSmall( void ) const
{
    return font_small;
}
const std::string & Settings::ForceLang( void ) const
{
    return force_lang;
}
const std::string & Settings::MapsCharset( void ) const
{
    return maps_charset;
}
int Settings::FontsNormalSize( void ) const
{
    return size_normal;
}
int Settings::FontsSmallSize( void ) const
{
    return size_small;
}
bool Settings::FontSmallRenderBlended( void ) const
{
    return opt_global.Modes( GLOBAL_FONTRENDERBLENDED1 );
}
bool Settings::FontNormalRenderBlended( void ) const
{
    return opt_global.Modes( GLOBAL_FONTRENDERBLENDED2 );
}

void Settings::SetProgramPath( const char * argv0 )
{
    if ( argv0 )
        path_program = argv0;
}

ListDirs Settings::GetRootDirs( void )
{
    const Settings & conf = Settings::Get();
    ListDirs dirs;

    // from build
#ifdef CONFIGURE_FHEROES2_DATA
    dirs.push_back( CONFIGURE_FHEROES2_DATA );
#endif

    // from env
    if ( System::GetEnvironment( "FHEROES2_DATA" ) )
        dirs.push_back( System::GetEnvironment( "FHEROES2_DATA" ) );

    // from dirname
    dirs.push_back( System::GetDirname( conf.path_program ) );

    // from HOME
    const std::string & home = System::GetHomeDirectory( "fheroes2" );
    if ( !home.empty() )
        dirs.push_back( home );

    return dirs;
}

/* return list files */
ListFiles Settings::GetListFiles( const std::string & prefix, const std::string & filter )
{
    const ListDirs dirs = GetRootDirs();
    ListFiles res;

    if ( prefix.size() && System::IsDirectory( prefix ) )
        res.ReadDir( prefix, filter, false );

    for ( ListDirs::const_iterator it = dirs.begin(); it != dirs.end(); ++it ) {
        std::string path = prefix.size() ? System::ConcatePath( *it, prefix ) : *it;

        if ( System::IsDirectory( path ) )
            res.ReadDir( path, filter, false );
    }

    res.Append( System::GetListFiles( "fheroes2", prefix, filter ) );

    return res;
}

std::string Settings::GetLastFile( const std::string & prefix, const std::string & name )
{
    const ListFiles & files = GetListFiles( prefix, name );
    return files.empty() ? name : files.back();
}

std::string Settings::GetLangDir( void )
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

std::string Settings::GetWriteableDir( const char * subdir )
{
    ListDirs dirs = GetRootDirs();
    dirs.Append( System::GetDataDirectories( "fheroes2" ) );

    for ( ListDirs::const_iterator it = dirs.begin(); it != dirs.end(); ++it ) {
        std::string dir_files = System::ConcatePath( *it, "files" );

        // create files
        if ( System::IsDirectory( *it, true ) && !System::IsDirectory( dir_files, true ) )
            System::MakeDirectory( dir_files );

        // create subdir
        if ( System::IsDirectory( dir_files, true ) ) {
            std::string dir_subdir = System::ConcatePath( dir_files, subdir );

            if ( !System::IsDirectory( dir_subdir, true ) )
                System::MakeDirectory( dir_subdir );

            if ( System::IsDirectory( dir_subdir, true ) )
                return dir_subdir;
        }
    }

    DEBUG( DBG_GAME, DBG_WARN, "writable directory not found" );

    return "";
}

bool Settings::MusicExt( void ) const
{
    return opt_global.Modes( GLOBAL_MUSIC_EXT );
}
bool Settings::MusicMIDI( void ) const
{
    return opt_global.Modes( GLOBAL_MUSIC_MIDI );
}
bool Settings::MusicCD( void ) const
{
    return opt_global.Modes( GLOBAL_MUSIC_CD );
}

/* return sound */
bool Settings::Sound( void ) const
{
    return opt_global.Modes( GLOBAL_SOUND );
}

/* return music */
bool Settings::Music( void ) const
{
    return opt_global.Modes( GLOBAL_MUSIC );
}

/* return move speed */
int Settings::HeroesMoveSpeed( void ) const
{
    return heroes_speed;
}
int Settings::AIMoveSpeed( void ) const
{
    return ai_speed;
}
int Settings::BattleSpeed( void ) const
{
    return battle_speed;
}

/* return scroll speed */
int Settings::ScrollSpeed( void ) const
{
    return scroll_speed;
}

/* set ai speed: 1 - 10 */
void Settings::SetAIMoveSpeed( int speed )
{
    if ( speed < 1 ) {
        speed = 1;
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

bool Settings::UseAltResource( void ) const
{
    return opt_global.Modes( GLOBAL_ALTRESOURCE );
}

bool Settings::PriceLoyaltyVersion( void ) const
{
    return opt_global.Modes( GLOBAL_PRICELOYALTY );
}

bool Settings::LoadedGameVersion( void ) const
{
    return ( game_type & Game::TYPE_LOADFILE ) != 0;
}

bool Settings::ShowControlPanel( void ) const
{
    return opt_global.Modes( GLOBAL_SHOWCPANEL );
}

bool Settings::ShowRadar( void ) const
{
    return opt_global.Modes( GLOBAL_SHOWRADAR );
}

bool Settings::ShowIcons( void ) const
{
    return opt_global.Modes( GLOBAL_SHOWICONS );
}

bool Settings::ShowButtons( void ) const
{
    return opt_global.Modes( GLOBAL_SHOWBUTTONS );
}

bool Settings::ShowStatus( void ) const
{
    return opt_global.Modes( GLOBAL_SHOWSTATUS );
}

/* unicode support */
bool Settings::Unicode( void ) const
{
    return opt_global.Modes( GLOBAL_USEUNICODE );
}

/* pocketpc mode */
bool Settings::PocketPC( void ) const
{
    return opt_global.Modes( GLOBAL_POCKETPC );
}

bool Settings::BattleShowGrid( void ) const
{
    return opt_global.Modes( GLOBAL_BATTLE_SHOW_GRID );
}

bool Settings::BattleShowMouseShadow( void ) const
{
    return opt_global.Modes( GLOBAL_BATTLE_SHOW_MOUSE_SHADOW );
}

bool Settings::BattleShowMoveShadow( void ) const
{
    return opt_global.Modes( GLOBAL_BATTLE_SHOW_MOVE_SHADOW );
}

/* get video mode */
const Size & Settings::VideoMode( void ) const
{
    return video_mode;
}

/* set level debug */
void Settings::SetDebug( int d )
{
    debug = d;
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

int Settings::SoundVolume( void ) const
{
    return sound_volume;
}
int Settings::MusicVolume( void ) const
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

int Settings::GameType( void ) const
{
    return game_type;
}

/* set game type */
void Settings::SetGameType( int type )
{
    game_type = type;
}

void Settings::SetCurrentCampaignScenarioBonus( const Campaign::ScenarioBonusData & bonus )
{
    campaignData.setCurrentScenarioBonus( bonus );
}

void Settings::SetCurrentCampaignScenarioID( const int scenarioID )
{
    campaignData.setCurrentScenarioID( scenarioID );
}

void Settings::SetCurrentCampaignID( const int campaignID )
{
    campaignData.setCampaignID( campaignID );
}

void Settings::AddCurrentCampaignMapToFinished()
{
    campaignData.addCurrentMapToFinished();
}

const Players & Settings::GetPlayers( void ) const
{
    return players;
}

Players & Settings::GetPlayers( void )
{
    return players;
}

void Settings::SetPreferablyCountPlayers( int c )
{
    preferably_count_players = 6 < c ? 6 : c;
}

int Settings::PreferablyCountPlayers( void ) const
{
    return preferably_count_players;
}

int Settings::GetPort( void ) const
{
    return port;
}

const std::string & Settings::MapsFile( void ) const
{
    return current_maps_file.file;
}

const std::string & Settings::MapsName( void ) const
{
    return current_maps_file.name;
}

const std::string & Settings::MapsDescription( void ) const
{
    return current_maps_file.description;
}

int Settings::MapsDifficulty( void ) const
{
    return current_maps_file.difficulty;
}

Size Settings::MapsSize( void ) const
{
    return Size( current_maps_file.size_w, current_maps_file.size_h );
}

bool Settings::AllowChangeRace( int f ) const
{
    return ( current_maps_file.rnd_races & f ) != 0;
}

bool Settings::GameStartWithHeroes( void ) const
{
    return current_maps_file.with_heroes;
}

int Settings::ConditionWins( void ) const
{
    return current_maps_file.ConditionWins();
}

int Settings::ConditionLoss( void ) const
{
    return current_maps_file.ConditionLoss();
}

bool Settings::WinsCompAlsoWins( void ) const
{
    return current_maps_file.WinsCompAlsoWins();
}

bool Settings::WinsAllowNormalVictory( void ) const
{
    return current_maps_file.WinsAllowNormalVictory();
}

int Settings::WinsFindArtifactID( void ) const
{
    return current_maps_file.WinsFindArtifactID();
}

bool Settings::WinsFindUltimateArtifact( void ) const
{
    return current_maps_file.WinsFindUltimateArtifact();
}

u32 Settings::WinsAccumulateGold( void ) const
{
    return current_maps_file.WinsAccumulateGold();
}

Point Settings::WinsMapsPositionObject( void ) const
{
    return current_maps_file.WinsMapsPositionObject();
}

Point Settings::LossMapsPositionObject( void ) const
{
    return current_maps_file.LossMapsPositionObject();
}

u32 Settings::LossCountDays( void ) const
{
    return current_maps_file.LossCountDays();
}

void Settings::SetUnicode( bool f )
{
    f ? opt_global.SetModes( GLOBAL_USEUNICODE ) : opt_global.ResetModes( GLOBAL_USEUNICODE );
}

void Settings::SetPriceLoyaltyVersion( bool set )
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

void Settings::ResetSound( void )
{
    opt_global.ResetModes( GLOBAL_SOUND );
}

void Settings::ResetMusic( void )
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
    const settings_t * ptr = std::find( settingsFHeroes2, ARRAY_COUNT_END( settingsFHeroes2 ) - 1, f );

    return ptr ? _( ptr->str ) : NULL;
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

bool Settings::ExtCastleGuildRestorePointsTurn( void ) const
{
    return ExtModes( CASTLE_MAGEGUILD_POINTS_TURN );
}

bool Settings::ExtCastleAllowGuardians( void ) const
{
    return ExtModes( CASTLE_ALLOW_GUARDIANS );
}

bool Settings::ExtWorldShowVisitedContent( void ) const
{
    return ExtModes( WORLD_SHOW_VISITED_CONTENT );
}

bool Settings::ExtWorldScouteExtended( void ) const
{
    return ExtModes( WORLD_SCOUTING_EXTENDED );
}

bool Settings::ExtGameRememberLastFocus( void ) const
{
    return ExtModes( GAME_REMEMBER_LAST_FOCUS );
}

bool Settings::ExtWorldAbandonedMineRandom( void ) const
{
    return ExtModes( WORLD_ABANDONED_MINE_RANDOM );
}

bool Settings::ExtWorldAllowSetGuardian( void ) const
{
    return ExtModes( WORLD_ALLOW_SET_GUARDIAN );
}

bool Settings::ExtWorldArtifactCrystalBall( void ) const
{
    return ExtModes( WORLD_ARTIFACT_CRYSTAL_BALL );
}

bool Settings::ExtWorldOnlyFirstMonsterAttack( void ) const
{
    return ExtModes( WORLD_ONLY_FIRST_MONSTER_ATTACK );
}

bool Settings::ExtWorldEyeEagleAsScholar( void ) const
{
    return ExtModes( WORLD_EYE_EAGLE_AS_SCHOLAR );
}

bool Settings::ExtHeroBuySpellBookFromShrine( void ) const
{
    return ExtModes( HEROES_BUY_BOOK_FROM_SHRINES );
}

bool Settings::ExtHeroRecruitCostDependedFromLevel( void ) const
{
    return ExtModes( HEROES_COST_DEPENDED_FROM_LEVEL );
}

bool Settings::ExtHeroRememberPointsForRetreating( void ) const
{
    return ExtModes( HEROES_REMEMBER_POINTS_RETREAT );
}

bool Settings::ExtHeroSurrenderingGiveExp( void ) const
{
    return ExtModes( HEROES_SURRENDERING_GIVE_EXP );
}

bool Settings::ExtHeroRecalculateMovement( void ) const
{
    return ExtModes( HEROES_RECALCULATE_MOVEMENT );
}

bool Settings::ExtUnionsAllowCastleVisiting( void ) const
{
    return ExtModes( UNIONS_ALLOW_CASTLE_VISITING );
}

bool Settings::ExtUnionsAllowHeroesMeetings( void ) const
{
    return ExtModes( UNIONS_ALLOW_HERO_MEETINGS );
}

bool Settings::ExtBattleShowDamage( void ) const
{
    return ExtModes( GAME_BATTLE_SHOW_DAMAGE );
}

bool Settings::ExtBattleSkipIncreaseDefense( void ) const
{
    return ExtModes( BATTLE_SKIP_INCREASE_DEFENSE );
}

bool Settings::ExtHeroAllowTranscribingScroll( void ) const
{
    return ExtModes( HEROES_TRANSCRIBING_SCROLLS );
}

bool Settings::ExtBattleShowBattleOrder( void ) const
{
    return ExtModes( BATTLE_SHOW_ARMY_ORDER );
}

bool Settings::ExtBattleSoftWait( void ) const
{
    return ExtModes( BATTLE_SOFT_WAITING );
}

bool Settings::ExtBattleObjectsArchersPenalty( void ) const
{
    return ExtModes( BATTLE_OBJECTS_ARCHERS_PENALTY );
}

bool Settings::ExtGameRewriteConfirm( void ) const
{
    return ExtModes( GAME_SAVE_REWRITE_CONFIRM );
}

bool Settings::ExtGameShowSystemInfo( void ) const
{
    return ExtModes( GAME_SHOW_SYSTEM_INFO );
}

bool Settings::ExtGameAutosaveBeginOfDay( void ) const
{
    return ExtModes( GAME_AUTOSAVE_BEGIN_DAY );
}

bool Settings::ExtGameAutosaveOn( void ) const
{
    return ExtModes( GAME_AUTOSAVE_ON );
}

bool Settings::ExtGameUseFade( void ) const
{
    return video_mode == Size( fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT ) && ExtModes( GAME_USE_FADE );
}

bool Settings::ExtGameEvilInterface( void ) const
{
    return ExtModes( GAME_EVIL_INTERFACE );
}

bool Settings::ExtGameDynamicInterface( void ) const
{
    return ExtModes( GAME_DYNAMIC_INTERFACE );
}

bool Settings::ExtGameHideInterface( void ) const
{
    return ExtModes( GAME_HIDE_INTERFACE );
}

bool Settings::ExtPocketTapMode( void ) const
{
    return ExtModes( POCKETPC_TAP_MODE );
}

bool Settings::ExtPocketDragDropScroll( void ) const
{
    return ExtModes( POCKETPC_DRAG_DROP_SCROLL );
}

bool Settings::ExtWorldNewVersionWeekOf( void ) const
{
    return ExtModes( WORLD_NEW_VERSION_WEEKOF );
}

bool Settings::ExtWorldBanWeekOf( void ) const
{
    return ExtModes( WORLD_BAN_WEEKOF );
}

bool Settings::ExtWorldBanMonthOfMonsters( void ) const
{
    return ExtModes( WORLD_BAN_MONTHOF_MONSTERS );
}

bool Settings::ExtWorldBanPlagues( void ) const
{
    return ExtModes( WORLD_BAN_PLAGUES );
}

bool Settings::ExtBattleReverseWaitOrder( void ) const
{
    return ExtModes( BATTLE_REVERSE_WAIT_ORDER );
}

bool Settings::ExtWorldStartHeroLossCond4Humans( void ) const
{
    return ExtModes( WORLD_STARTHERO_LOSSCOND4HUMANS );
}

bool Settings::ExtHeroAllowBannedSecSkillsUpgrade( void ) const
{
    return ExtModes( HEROES_ALLOW_BANNED_SECSKILLS );
}

bool Settings::ExtWorldOneHeroHiredEveryWeek( void ) const
{
    return ExtModes( WORLD_1HERO_HIRED_EVERY_WEEK );
}

bool Settings::ExtCastleOneHeroHiredEveryWeek( void ) const
{
    return ExtModes( CASTLE_1HERO_HIRED_EVERY_WEEK );
}

bool Settings::ExtWorldNeutralArmyDifficultyScaling( void ) const
{
    return ExtModes( WORLD_SCALE_NEUTRAL_ARMIES );
}

bool Settings::ExtWorldUseUniqueArtifactsML( void ) const
{
    return ExtModes( WORLD_USE_UNIQUE_ARTIFACTS_ML );
}

bool Settings::ExtWorldUseUniqueArtifactsRS( void ) const
{
    return ExtModes( WORLD_USE_UNIQUE_ARTIFACTS_RS );
}

bool Settings::ExtWorldUseUniqueArtifactsPS( void ) const
{
    return ExtModes( WORLD_USE_UNIQUE_ARTIFACTS_PS );
}

bool Settings::ExtWorldUseUniqueArtifactsSS( void ) const
{
    return ExtModes( WORLD_USE_UNIQUE_ARTIFACTS_SS );
}

bool Settings::ExtHeroArenaCanChoiseAnySkills( void ) const
{
    return ExtModes( HEROES_ARENA_ANY_SKILLS );
}

bool Settings::ExtWorldExtObjectsCaptured( void ) const
{
    return ExtModes( WORLD_EXT_OBJECTS_CAPTURED );
}

bool Settings::ExtWorldDisableBarrowMounds( void ) const
{
    return ExtModes( WORLD_DISABLE_BARROW_MOUNDS );
}

bool Settings::ExtGameContinueAfterVictory( void ) const
{
    return ExtModes( GAME_CONTINUE_AFTER_VICTORY );
}

const Point & Settings::PosRadar( void ) const
{
    return pos_radr;
}
const Point & Settings::PosButtons( void ) const
{
    return pos_bttn;
}
const Point & Settings::PosIcons( void ) const
{
    return pos_icon;
}
const Point & Settings::PosStatus( void ) const
{
    return pos_stat;
}

void Settings::SetPosRadar( const Point & pt )
{
    pos_radr = pt;
}
void Settings::SetPosButtons( const Point & pt )
{
    pos_bttn = pt;
}
void Settings::SetPosIcons( const Point & pt )
{
    pos_icon = pt;
}
void Settings::SetPosStatus( const Point & pt )
{
    pos_stat = pt;
}

void Settings::BinarySave( void ) const
{
    const std::string fname = System::ConcatePath( Game::GetSaveDir(), "fheroes2.bin" );

    StreamFile fs;
    fs.setbigendian( true );

    if ( fs.open( fname, "wb" ) ) {
        fs << static_cast<u16>( CURRENT_FORMAT_VERSION ) << opt_game << opt_world << opt_battle << opt_addons << pos_radr << pos_bttn << pos_icon << pos_stat;
    }
}

void Settings::BinaryLoad( void )
{
    std::string fname = System::ConcatePath( Game::GetSaveDir(), "fheroes2.bin" );

    if ( !System::IsFile( fname ) )
        fname = GetLastFile( "", "fheroes2.bin" );

    StreamFile fs;
    fs.setbigendian( true );

    if ( fs.open( fname, "rb" ) ) {
        u16 version = 0;

        fs >> version >> opt_game >> opt_world >> opt_battle >> opt_addons >> pos_radr >> pos_bttn >> pos_icon >> pos_stat;
    }
}

bool Settings::FullScreen( void ) const
{
    return System::isEmbededDevice() || opt_global.Modes( GLOBAL_FULLSCREEN );
}

bool Settings::KeepAspectRatio( void ) const
{
    return opt_global.Modes( GLOBAL_KEEP_ASPECT_RATIO );
}

bool Settings::ChangeFullscreenResolution( void ) const
{
    return opt_global.Modes( GLOBAL_CHANGE_FULLSCREEN_RESOLUTION );
}

StreamBase & operator<<( StreamBase & msg, const Settings & conf )
{
    msg << conf.force_lang << conf.current_maps_file << conf.game_difficulty << conf.game_type << conf.preferably_count_players << conf.debug << conf.opt_game
        << conf.opt_world << conf.opt_battle << conf.opt_addons << conf.players;

    // TODO: add verification logic
    if ( conf.game_type & Game::TYPE_CAMPAIGN )
        msg << conf.campaignData;

    return msg;
}

StreamBase & operator>>( StreamBase & msg, Settings & conf )
{
    std::string lang;

    msg >> lang;

    if ( lang != "en" && lang != conf.force_lang && !conf.Unicode() ) {
        std::string warningMessage( "This is an saved game is localized for lang = " );
        warningMessage.append( lang );
        warningMessage.append( ", and most of the messages will be displayed incorrectly.\n \n" );
        warningMessage.append( "(tip: set unicode = on)" );
        Dialog::Message( "Warning!", warningMessage, Font::BIG, Dialog::OK );
    }

    int debug;
    u32 opt_game = 0; // skip: settings

    // map file
    msg >> conf.current_maps_file >> conf.game_difficulty >> conf.game_type >> conf.preferably_count_players >> debug >> opt_game >> conf.opt_world >> conf.opt_battle
        >> conf.opt_addons >> conf.players;

    if ( conf.game_type & Game::TYPE_CAMPAIGN )
        msg >> conf.campaignData;

#ifndef WITH_DEBUG
    conf.debug = debug;
#endif

    return msg;
}
