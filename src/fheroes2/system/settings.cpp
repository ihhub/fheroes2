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

#include "text.h"
#include "maps.h"
#include "race.h"
#include "game.h"
#include "tinyconfig.h"
#include "difficulty.h"
#include "dialog.h"
#include "settings.h"

#define DEFAULT_PORT	5154
#define DEFAULT_DEBUG	DBG_ALL_WARN

bool IS_DEBUG(int name, int level)
{
    const int debug = Settings::Get().Debug();
    return
        ((DBG_ENGINE & name) && ((DBG_ENGINE & debug) >> 2) >= level) ||
        ((DBG_GAME & name) && ((DBG_GAME & debug) >> 4) >= level) ||
        ((DBG_BATTLE & name) && ((DBG_BATTLE & debug) >> 6) >= level) ||
        ((DBG_AI & name) && ((DBG_AI & debug) >> 8) >= level) ||
        ((DBG_NETWORK & name) && ((DBG_NETWORK & debug) >> 10) >= level) ||
        ((DBG_DEVEL & name) && ((DBG_DEVEL & debug) >> 12) >= level);
}

const char* StringDebug(int name)
{
    if(name & DBG_ENGINE)	return "DBG_ENGINE";
    else
    if(name & DBG_GAME)		return "DBG_GAME";
    else
    if(name & DBG_BATTLE)	return "DBG_BATTLE";
    else
    if(name & DBG_AI)		return "DBG_AI";
    else
    if(name & DBG_NETWORK)	return "DBG_NETWORK";
    else
    if(name & DBG_OTHER)	return "DBG_OTHER";
    else
    if(name & DBG_DEVEL)	return "DBG_DEVEL";
    return "";
}

enum
{
    GLOBAL_PRICELOYALTY      = 0x00000004,

    GLOBAL_POCKETPC          = 0x00000010,
    GLOBAL_DEDICATEDSERVER   = 0x00000020,
    GLOBAL_LOCALCLIENT       = 0x00000040,

    GLOBAL_SHOWCPANEL        = 0x00000100,
    GLOBAL_SHOWRADAR         = 0x00000200,
    GLOBAL_SHOWICONS         = 0x00000400,
    GLOBAL_SHOWBUTTONS       = 0x00000800,
    GLOBAL_SHOWSTATUS        = 0x00001000,

    GLOBAL_FONTRENDERBLENDED1= 0x00020000,
    GLOBAL_FONTRENDERBLENDED2= 0x00040000,
    GLOBAL_FULLSCREEN        = 0x00400000,
    GLOBAL_USESWSURFACE      = 0x00800000,

    GLOBAL_SOUND             = 0x01000000,
    GLOBAL_MUSIC_EXT         = 0x02000000,
    GLOBAL_MUSIC_CD          = 0x04000000,
    GLOBAL_MUSIC_MIDI        = 0x08000000,

    //GLOBAL_UNUSED          = 0x20000000,
    GLOBAL_USEUNICODE        = 0x40000000,
    GLOBAL_ALTRESOURCE       = 0x80000000,

    GLOBAL_MUSIC           = GLOBAL_MUSIC_CD | GLOBAL_MUSIC_EXT | GLOBAL_MUSIC_MIDI
};

struct settings_t
{
    u32 id;
    const char* str;

    bool operator== (const std::string & s) const { return str && s == str; };
    bool operator== (u32 i) const { return id && id == i; };
};

// external settings
const settings_t settingsGeneral[] =
{
    { GLOBAL_SOUND,       "sound",        },
    { GLOBAL_MUSIC_MIDI,  "music",        },
    { GLOBAL_FULLSCREEN,  "fullscreen",   },
    { GLOBAL_FULLSCREEN,  "full screen",  },
    { GLOBAL_USEUNICODE,  "unicode",      },
    { GLOBAL_ALTRESOURCE, "alt resource", },
    { GLOBAL_POCKETPC,    "pocketpc",     },
    { GLOBAL_POCKETPC,    "pocket pc",    },
    { GLOBAL_USESWSURFACE,"use swsurface only",},
    { 0, NULL, },
};

// internal settings
const settings_t settingsFHeroes2[] =
{
    { Settings::GAME_SAVE_REWRITE_CONFIRM,	_("game: always confirm for rewrite savefile"),		},
    { Settings::GAME_ALSO_CONFIRM_AUTOSAVE,	_("game: also confirm autosave"),			},
    { Settings::GAME_REMEMBER_LAST_FOCUS,	_("game: remember last focus"),				},
    { Settings::GAME_BATTLE_SHOW_GRID,		_("game: battle show grid"),				},
    { Settings::GAME_BATTLE_SHOW_MOUSE_SHADOW,	_("game: battle mouse shadow")				},
    { Settings::GAME_BATTLE_SHOW_MOVE_SHADOW,	_("game: battle move shadow"),				},
    { Settings::GAME_BATTLE_SHOW_DAMAGE,	_("game: battle show damage info"),  			},
    { Settings::GAME_CASTLE_FLASH_BUILDING,	_("game: castle flash building"),			},
    { Settings::WORLD_SHOW_VISITED_CONTENT,	_("world: show visited content from objects"),		},
    { Settings::WORLD_SCOUTING_EXTENDED,	_("world: scouting skill show extended content info"),  },
    { Settings::WORLD_ABANDONED_MINE_RANDOM,	_("world: abandoned mine random resource"),		},
    { Settings::WORLD_SAVE_MONSTER_BATTLE,	_("world: save count monster after battle"),		},
    { Settings::WORLD_ALLOW_SET_GUARDIAN,	_("world: allow set guardian to objects"),		},
    { Settings::WORLD_GUARDIAN_TWO_DEFENSE,	_("world: guardian objects gets +2 defense"),		},
    { Settings::WORLD_NOREQ_FOR_ARTIFACTS,	_("world: no in-built requirements or guardians for placed artifacts"),	},
    { Settings::WORLD_ONLY_FIRST_MONSTER_ATTACK,_("world: only the first monster will attack (H2 bug)."), },
    { Settings::WORLD_EYE_EAGLE_AS_SCHOLAR,	_("world: Eagle Eye also works like Scholar in H3."),   },
    { Settings::WORLD_BAN_WEEKOF,		_("world: ban for WeekOf/MonthOf Monsters"),            },
    { Settings::WORLD_NEW_VERSION_WEEKOF,	_("world: new version WeekOf (+growth)"),       	},
    { Settings::WORLD_BAN_PLAGUES,		_("world: ban plagues months"),                         },
    { Settings::WORLD_BAN_MONTHOF_MONSTERS,	_("world: Months Of Monsters do not place creatures on map"),   },
    { Settings::WORLD_ARTIFACT_CRYSTAL_BALL,	_("world: Crystal Ball also added Identify Hero and Visions spells"), },
    { Settings::WORLD_ARTSPRING_SEPARATELY_VISIT,_("world: Artesian Springs have two separately visitable squares (h3 ver)"), },
    { Settings::WORLD_STARTHERO_LOSSCOND4HUMANS,_("world: Starting heroes as Loss Conditions for Human Players"), },
    { Settings::WORLD_1HERO_HIRED_EVERY_WEEK,	_("world: Only 1 hero can be hired by the one player every week"), },
    { Settings::CASTLE_1HERO_HIRED_EVERY_WEEK,	_("world: each castle allows one hero to be recruited every week"), },
    { Settings::WORLD_DWELLING_ACCUMULATE_UNITS,_("world: Outer creature dwellings should accumulate units"), },
    { Settings::WORLD_USE_UNIQUE_ARTIFACTS_ML,	_("world: use unique artifacts for morale/luck"),       },
    { Settings::WORLD_USE_UNIQUE_ARTIFACTS_RS,	_("world: use unique artifacts for resource affecting"),},
    { Settings::WORLD_USE_UNIQUE_ARTIFACTS_PS,	_("world: use unique artifacts for primary skills"),},
    { Settings::WORLD_USE_UNIQUE_ARTIFACTS_SS,	_("world: use unique artifacts for secondary skills"),},
    { Settings::WORLD_EXT_OBJECTS_CAPTURED,	_("world: Wind/Water Mills and Magic Garden can be captured"),},
    { Settings::WORLD_DISABLE_BARROW_MOUNDS,	_("world: disable Barrow Mounds"),			},
    { Settings::CASTLE_ALLOW_BUY_FROM_WELL,	_("castle: allow buy from well"),			},
    { Settings::CASTLE_ALLOW_GUARDIANS,		_("castle: allow guardians"),				},
    { Settings::CASTLE_MAGEGUILD_POINTS_TURN,	_("castle: higher mage guilds regenerate more spell points/turn (20/40/60/80/100%)"), },
    { Settings::CASTLE_ALLOW_RECRUITS_SPECIAL,	_("castle: allow recruits special/expansion heroes"), },
    { Settings::HEROES_BUY_BOOK_FROM_SHRINES,	_("heroes: allow buy a spellbook from Shrines"),         },
    { Settings::HEROES_LEARN_SPELLS_WITH_DAY,	_("heroes: learn new spells with day"),  		},
    { Settings::HEROES_COST_DEPENDED_FROM_LEVEL,_("heroes: recruit cost to be dependent on hero level"),},
    { Settings::HEROES_REMEMBER_POINTS_RETREAT, _("heroes: remember MP/SP for retreat/surrender result"),},
    { Settings::HEROES_SURRENDERING_GIVE_EXP,   _("heroes: surrendering gives some experience"),        },
    { Settings::HEROES_RECALCULATE_MOVEMENT,    _("heroes: recalculate movement points after creatures movement"), },
    { Settings::HEROES_PATROL_ALLOW_PICKUP,     _("heroes: allow pickup objects for patrol"),           },
    { Settings::HEROES_AUTO_MOVE_BATTLE_DST,	_("heroes: after battle move to target cell"),		},
    { Settings::HEROES_TRANSCRIBING_SCROLLS,	_("heroes: allow transcribing scrolls (needs: Eye Eagle skill)"), },
    { Settings::HEROES_ALLOW_BANNED_SECSKILLS,	_("heroes: allow banned sec. skills upgrade"), 		},
    { Settings::HEROES_ARENA_ANY_SKILLS,	_("heroes: in Arena can choose any of primary skills"), },
    { Settings::UNIONS_ALLOW_HERO_MEETINGS,	_("unions: allow meeting heroes"),                      },
    { Settings::UNIONS_ALLOW_CASTLE_VISITING,	_("unions: allow castle visiting"),                     },
    { Settings::BATTLE_SOFT_WAITING,		_("battle: soft wait troop"),				},
    { Settings::BATTLE_OBJECTS_ARCHERS_PENALTY, _("battle: high objects are an obstacle for archers"),  },
    { Settings::BATTLE_MERGE_ARMIES, 		_("battle: merge armies for hero from castle"),  	},
    { Settings::BATTLE_ARCHMAGE_RESIST_BAD_SPELL,_("battle: archmage can resists (20%) bad spells"),     },
    { Settings::BATTLE_MAGIC_TROOP_RESIST,	_("battle: magical creature resists (20%) the same magic"),},
    { Settings::BATTLE_SKIP_INCREASE_DEFENSE,	_("battle: skip increase +2 defense"), 			},
    { Settings::BATTLE_REVERSE_WAIT_ORDER,	_("battle: reverse wait order (fast, average, slow)"),	},
    { Settings::GAME_SHOW_SYSTEM_INFO,		_("game: show system info"),				},
    { Settings::GAME_AUTOSAVE_ON,		_("game: autosave on"),					},
    { Settings::GAME_AUTOSAVE_BEGIN_DAY,	_("game: autosave will be made at the beginning of the day"), },
    { Settings::GAME_USE_FADE,			_("game: use fade"),					},
    { Settings::GAME_SHOW_SDL_LOGO,		_("game: show SDL logo"),				},
    { Settings::GAME_EVIL_INTERFACE,		_("game: use evil interface"),				},
    { Settings::GAME_DYNAMIC_INTERFACE,		_("game: also use dynamic interface for castles"),	},
    { Settings::GAME_HIDE_INTERFACE,		_("game: hide interface"),				},
    { Settings::GAME_CONTINUE_AFTER_VICTORY,	_("game: offer to continue the game afer victory condition"), },
    { Settings::POCKETPC_HIDE_CURSOR,		_("pocketpc: hide cursor"),				},
    { Settings::POCKETPC_TAP_MODE,		_("pocketpc: tap mode"),				},
    { Settings::POCKETPC_DRAG_DROP_SCROLL,	_("pocketpc: drag&drop gamearea as scroll"),		},
    { Settings::POCKETPC_LOW_MEMORY,		_("pocketpc: low memory"),				},

    { 0, NULL },
};

std::string Settings::GetVersion()
{
    std::ostringstream os;

    os << static_cast<int>(MAJOR_VERSION) << "." << static_cast<int>(MINOR_VERSION) << "."
#ifdef SVN_REVISION
    SVN_REVISION;
#else
    "0000";
#endif

    return os.str();
}

/* constructor */
Settings::Settings() : debug(DEFAULT_DEBUG), video_mode(640, 480), game_difficulty(Difficulty::NORMAL),
    font_normal("dejavusans.ttf"), font_small("dejavusans.ttf"), size_normal(15), size_small(10),
    sound_volume(6), music_volume(6), heroes_speed(DEFAULT_SPEED_DELAY), ai_speed(DEFAULT_SPEED_DELAY), scroll_speed(SCROLL_NORMAL), battle_speed(DEFAULT_SPEED_DELAY),
    blit_speed(0), game_type(0), preferably_count_players(0), port(DEFAULT_PORT), memory_limit(0)
{
    ExtSetModes(GAME_SHOW_SDL_LOGO);
    ExtSetModes(GAME_AUTOSAVE_ON);

    opt_global.SetModes(GLOBAL_SHOWRADAR);
    opt_global.SetModes(GLOBAL_SHOWICONS);
    opt_global.SetModes(GLOBAL_SHOWBUTTONS);
    opt_global.SetModes(GLOBAL_SHOWSTATUS);
    opt_global.SetModes(GLOBAL_MUSIC_MIDI);
    if(System::isEmbededDevice())
    {
	opt_global.SetModes(GLOBAL_POCKETPC);
	ExtSetModes(POCKETPC_HIDE_CURSOR);
	ExtSetModes(POCKETPC_TAP_MODE);
	ExtSetModes(POCKETPC_DRAG_DROP_SCROLL);
    }
}

Settings::~Settings()
{
    if(!LoadedGameVersion()) BinarySave();
}

Settings & Settings::Get()
{
    static Settings conf;

    return conf;
}

bool Settings::Read(const std::string & filename)
{
    TinyConfig config('=', '#');
    std::string sval; int ival;
    LocalEvent & le = LocalEvent::Get();

    if(! config.Load(filename)) return false;

    // debug
    ival = config.IntParams("debug");

    switch(ival)
    {
	case 0:	debug = DBG_ALL_WARN; break;
	case 1:	debug = DBG_ENGINE_INFO; break;
	case 2:	debug = DBG_ENGINE_INFO | DBG_GAME_INFO; break;
	case 3:	debug = DBG_ENGINE_INFO | DBG_BATTLE_INFO; break;
	case 4:	debug = DBG_ENGINE_INFO | DBG_BATTLE_INFO | DBG_AI_INFO; break;
	case 5:	debug = DBG_ALL_INFO; break;
	case 6:	debug = DBG_GAME_TRACE; break;
	case 7:	debug = DBG_GAME_TRACE | DBG_AI_TRACE; break;
	case 8:	debug = DBG_ENGINE_TRACE | DBG_GAME_TRACE | DBG_AI_TRACE; break;
	case 9:	debug = DBG_ALL_TRACE; break;
	default: debug = ival; break;
    }

    // opt_globals
    const settings_t* ptr = settingsGeneral;
    while(ptr->id)
    {
	if(config.Exists(ptr->str))
	{
	    if(0 == config.IntParams(ptr->str))
		opt_global.ResetModes(ptr->id);
	    else
		opt_global.SetModes(ptr->id);
	}

	++ptr;
    }

    // maps directories
    maps_params.Append(config.ListStr("maps"));
    maps_params.sort();
    maps_params.unique();

    // data
    sval = config.StrParams("data");
    if(! sval.empty()) data_params = sval;

    if(Unicode())
    {
	sval = config.StrParams("maps charset");
	if(! sval.empty()) maps_charset = sval;

	sval = config.StrParams("lang");
	if(! sval.empty()) force_lang = sval;

	sval = config.StrParams("fonts normal");
	if(! sval.empty()) font_normal = sval;

	sval = config.StrParams("fonts small");
	if(! sval.empty()) font_small = sval;

	ival = config.IntParams("fonts normal size");
	if(0 < ival) size_normal = ival;

	ival = config.IntParams("fonts small size");
	if(0 < ival) size_small = ival;

	if(config.StrParams("fonts small render") == "blended") opt_global.SetModes(GLOBAL_FONTRENDERBLENDED1);
	if(config.StrParams("fonts normal render") == "blended") opt_global.SetModes(GLOBAL_FONTRENDERBLENDED2);
    }

    // music
    sval = config.StrParams("music");

    if(! sval.empty())
    {
	if(sval == "midi")
	{
	    opt_global.ResetModes(GLOBAL_MUSIC);
	    opt_global.SetModes(GLOBAL_MUSIC_MIDI);
	}
	else
	if(sval == "cd")
        {
	    opt_global.ResetModes(GLOBAL_MUSIC);
	    opt_global.SetModes(GLOBAL_MUSIC_CD);
	}
	else
	if(sval == "ext")
	{
	    opt_global.ResetModes(GLOBAL_MUSIC);
	    opt_global.SetModes(GLOBAL_MUSIC_EXT);
	}
    }

    // sound volume
    if(config.Exists("sound volume"))
    {
	sound_volume = config.IntParams("sound volume");
	if(sound_volume > 10) sound_volume = 10;
    }

    // music volume
    if(config.Exists("music volume"))
    {
	music_volume = config.IntParams("music volume");
	if(music_volume > 10) music_volume = 10;
    }

    // memory limit
    memory_limit = config.IntParams("memory limit");

    // default depth
    ival = config.IntParams("default depth");
    if(ival) Surface::SetDefaultDepth(ival);

    // move speed
    if(config.Exists("ai speed"))
    {
	ai_speed = config.IntParams("ai speed");
	if(10 < ai_speed) ai_speed = 10;
    }

    if(config.Exists("heroes speed"))
    {
	heroes_speed = config.IntParams("heroes speed");
	if(10 < heroes_speed) heroes_speed = 10;
    }

    // scroll speed
    switch(config.IntParams("scroll speed"))
    {
	case 1:		scroll_speed = SCROLL_SLOW; break;
	case 2:		scroll_speed = SCROLL_NORMAL; break;
	case 3:		scroll_speed = SCROLL_FAST1; break;
	case 4:		scroll_speed = SCROLL_FAST2; break;
	default:	scroll_speed = SCROLL_NORMAL; break;
    }

    if(config.Exists("battle speed"))
    {
	battle_speed = config.IntParams("battle speed");
	if(10 < battle_speed) battle_speed = 10;
    }

    // network port
    port = config.Exists("port") ? config.IntParams("port") : DEFAULT_PORT;

    // playmus command
    sval = config.StrParams("playmus command");
    if(! sval.empty()) Music::SetExtCommand(sval);

    // videodriver
    sval = config.StrParams("videodriver");
    if(! sval.empty()) video_driver = sval;

    // pocketpc
    if(PocketPC())
    {
	ival = config.IntParams("pointer offset x");
	if(ival) le.SetMouseOffsetX(ival);

	ival = config.IntParams("pointer offset y");
	if(ival) le.SetMouseOffsetY(ival);

	ival = config.IntParams("tap delay");
	if(ival) le.SetTapDelayForRightClickEmulation(ival);

	sval = config.StrParams("pointer rotate fix");
	if(! sval.empty())
    	    System::SetEnvironment("GAPI_POINTER_FIX", sval.c_str());
    }

    // videomode
    sval = config.StrParams("videomode");
    if(! sval.empty())
    {
        // default
	video_mode.w = 640;
        video_mode.h = 480;

        std::string value = StringLower(sval);
        const size_t pos = value.find('x');

        if(std::string::npos != pos)
        {
    	    std::string width(value.substr(0, pos));
	    std::string height(value.substr(pos + 1, value.length() - pos - 1));

	    video_mode.w = GetInt(width);
	    video_mode.h = GetInt(height);
        }
	else
	if(value == "auto")
	{
            video_mode.w = 0;
            video_mode.h = 0;
	}
        else DEBUG(DBG_ENGINE, DBG_WARN, "unknown video mode: " << value);
    }

#ifdef WITHOUT_MOUSE
    ival = config.IntParams("emulate mouse");
    if(ival)
    {
	le.SetEmulateMouse(ival);

	ival = config.IntParams("emulate mouse step");
        if(ival) le.SetEmulateMouseStep(ival);
    }
#endif

#ifndef WITH_TTF
    opt_global.ResetModes(GLOBAL_USEUNICODE);
#endif

    if(font_normal.empty() || font_small.empty()) opt_global.ResetModes(GLOBAL_USEUNICODE);

#ifdef BUILD_RELEASE
    // reset devel
    debug &= ~(DBG_DEVEL);
#endif
    BinaryLoad();

    if(video_driver.size())
	video_driver = StringLower(video_driver);

    if(video_mode.w && video_mode.h)
	PostLoad();

    return true;
}

void Settings::PostLoad()
{
    if(QVGA())
    {
	opt_global.SetModes(GLOBAL_POCKETPC);
	ExtSetModes(GAME_HIDE_INTERFACE);
    }

    if(opt_global.Modes(GLOBAL_POCKETPC))
        opt_global.SetModes(GLOBAL_FULLSCREEN);
    else
    {
	ExtResetModes(POCKETPC_HIDE_CURSOR);
	ExtResetModes(POCKETPC_TAP_MODE);
	ExtResetModes(POCKETPC_LOW_MEMORY);
    }

    if(ExtModes(GAME_HIDE_INTERFACE))
    {
       opt_global.SetModes(GLOBAL_SHOWCPANEL);
       opt_global.ResetModes(GLOBAL_SHOWRADAR);
       opt_global.ResetModes(GLOBAL_SHOWICONS);
       opt_global.ResetModes(GLOBAL_SHOWBUTTONS);
       opt_global.ResetModes(GLOBAL_SHOWSTATUS);
    }
}

void Settings::SetAutoVideoMode()
{
    video_mode = Display::Get().GetMaxMode(PocketPC());
    PostLoad();
}

bool Settings::Save(const std::string & filename) const
{
    if(filename.empty()) return false;

    StreamFile fs;
    if(! fs.open(filename, "wb")) return false;
    fs << String();

    return true;
}

std::string Settings::String() const
{
    std::ostringstream os;

    os << "# fheroes2 config, version: " << GetVersion() << std::endl;
    os << "data = " << data_params << std::endl;

    for(ListDirs::const_iterator
	it = maps_params.begin(); it != maps_params.end(); ++it)
    os << "maps = " << *it << std::endl;

    os << "videomode = ";
    if(video_mode.w && video_mode.h)
	os << video_mode.w << "x" << video_mode.h << std::endl;
    else
	os << "auto" << std::endl;

    os <<
	"sound = " << (opt_global.Modes(GLOBAL_SOUND) ? "on"  : "off") << std::endl <<
	"music = " << (opt_global.Modes(GLOBAL_MUSIC_CD) ? "cd" : (opt_global.Modes(GLOBAL_MUSIC_MIDI) ? "midi" : (opt_global.Modes(GLOBAL_MUSIC_EXT) ? "ext" : "off"))) << std::endl <<
	"sound volume = " << static_cast<int>(sound_volume) << std::endl <<
	"music volume = " << static_cast<int>(music_volume) << std::endl <<
	"fullscreen = " << (opt_global.Modes(GLOBAL_FULLSCREEN) ? "on"  : "off") << std::endl <<
	"alt resource = " << (opt_global.Modes(GLOBAL_ALTRESOURCE) ? "on"  : "off") << std::endl <<
	"debug = " << (debug ? "on"  : "off") << std::endl;

#ifdef WITH_TTF
    os <<
	"fonts normal = " << font_normal << std::endl <<
	"fonts small = " << font_small << std::endl <<
	"fonts normal size = " << static_cast<int>(size_normal) << std::endl <<
	"fonts small size = " << static_cast<int>(size_small) << std::endl <<
	"unicode = " << (opt_global.Modes(GLOBAL_USEUNICODE) ? "on" : "off") << std::endl;
    if(force_lang.size())
    os << "lang = " << force_lang << std::endl;
#endif

    if(video_driver.size())
    os << "videodriver = " << video_driver << std::endl;

    if(opt_global.Modes(GLOBAL_POCKETPC))
    os << "pocket pc = on" << std::endl;

    return os.str();
}

/* read maps info */
void Settings::SetCurrentFileInfo(const Maps::FileInfo & fi)
{
    current_maps_file = fi;

    players.Init(current_maps_file);

    // game difficulty
    game_difficulty = Difficulty::NORMAL;
    preferably_count_players = 0;
}

const Maps::FileInfo & Settings::CurrentFileInfo() const
{
    return current_maps_file;
}

/* return debug */
int Settings::Debug() const { return debug; }

/* return game difficulty */
int Settings::GameDifficulty() const { return game_difficulty; }

int Settings::CurrentColor() const { return players.current_color; }

const std::string & Settings::SelectVideoDriver() const { return video_driver; }

/* return fontname */
const std::string & Settings::FontsNormal() const { return font_normal; }
const std::string & Settings::FontsSmall() const { return font_small; }
const std::string & Settings::ForceLang() const { return force_lang; }
const std::string & Settings::MapsCharset() const { return maps_charset; }
int Settings::FontsNormalSize() const { return size_normal; }
int Settings::FontsSmallSize() const { return size_small; }
bool Settings::FontSmallRenderBlended() const { return opt_global.Modes(GLOBAL_FONTRENDERBLENDED1); }
bool Settings::FontNormalRenderBlended() const { return opt_global.Modes(GLOBAL_FONTRENDERBLENDED2); }

void Settings::SetProgramPath(const char* argv0)
{
    if(argv0) path_program = argv0;
}

ListDirs Settings::GetRootDirs()
{
    const Settings & conf = Settings::Get();
    ListDirs dirs;

    // from build
#ifdef CONFIGURE_FHEROES2_DATA
    dirs.push_back(CONFIGURE_FHEROES2_DATA);
#endif

    // from env
    if(System::GetEnvironment("FHEROES2_DATA"))
	dirs.push_back(System::GetEnvironment("FHEROES2_DATA"));

    // from dirname
    dirs.push_back(System::GetDirname(conf.path_program));

    // from HOME
    const std::string & home = System::GetHomeDirectory("fheroes2");
    if(! home.empty()) dirs.push_back(home);

    return dirs;
}

/* return list files */
ListFiles Settings::GetListFiles(const std::string & prefix, const std::string & filter)
{
    const ListDirs dirs = GetRootDirs();
    ListFiles res;

    if(prefix.size() && System::IsDirectory(prefix))
	res.ReadDir(prefix, filter, false);

    for(ListDirs::const_iterator
	it = dirs.begin(); it != dirs.end(); ++it)
    {
        std::string path = prefix.size() ? System::ConcatePath(*it, prefix) : *it;

	if(System::IsDirectory(path))
	    res.ReadDir(path, filter, false);
    }

    res.Append(System::GetListFiles("fheroes2", prefix, filter));

    return res;
}

std::string Settings::GetLastFile(const std::string & prefix, const std::string & name)
{
    const ListFiles & files = GetListFiles(prefix, name);
    return files.empty() ? name : files.back();
}

std::string Settings::GetLangDir()
{
#ifdef CONFIGURE_FHEROES2_LOCALEDIR
    return std::string(CONFIGURE_FHEROES2_LOCALEDIR);
#else
    std::string res;
    const ListDirs dirs = GetRootDirs();

    for(ListDirs::const_reverse_iterator
	it = dirs.rbegin(); it != dirs.rend(); ++it)
    {
	res = System::ConcatePath(System::ConcatePath(*it, "files"), "lang");
        if(System::IsDirectory(res)) return res;
    }
#endif

    return "";
}

std::string Settings::GetWriteableDir(const char* subdir)
{
    ListDirs dirs = GetRootDirs();
    dirs.Append(System::GetDataDirectories("fheroes2"));

    for(ListDirs::const_iterator
	it = dirs.begin(); it != dirs.end(); ++it)
    {
	std::string dir_files = System::ConcatePath(*it, "files");

	// create files
	if(System::IsDirectory(*it, true) &&
	    ! System::IsDirectory(dir_files, true))
	    System::MakeDirectory(dir_files);

	// create subdir
        if(System::IsDirectory(dir_files, true))
	{
	    std::string dir_subdir = System::ConcatePath(dir_files, subdir);

    	    if(! System::IsDirectory(dir_subdir, true))
		System::MakeDirectory(dir_subdir);

    	    if(System::IsDirectory(dir_subdir, true))
		return dir_subdir;
	}
    }

    DEBUG(DBG_GAME, DBG_WARN, "writable directory not found");

    return "";
}

std::string Settings::GetSaveDir()
{
    return GetWriteableDir("save");
}

bool Settings::MusicExt() const { return opt_global.Modes(GLOBAL_MUSIC_EXT); }
bool Settings::MusicMIDI() const { return opt_global.Modes(GLOBAL_MUSIC_MIDI); }
bool Settings::MusicCD() const { return opt_global.Modes(GLOBAL_MUSIC_CD); }

/* return sound */
bool Settings::Sound() const { return opt_global.Modes(GLOBAL_SOUND); }

/* return music */
bool Settings::Music() const { return opt_global.Modes(GLOBAL_MUSIC); }

/* return move speed */
int Settings::HeroesMoveSpeed() const { return heroes_speed; }
int Settings::AIMoveSpeed() const { return ai_speed; }
int Settings::BattleSpeed() const { return battle_speed; }

/* return scroll speed */
int Settings::ScrollSpeed() const { return scroll_speed; }

/* set ai speed: 0 - 10 */
void Settings::SetAIMoveSpeed(int speed) { ai_speed = (10 <= speed ? 10 : speed); }

/* set hero speed: 0 - 10 */
void Settings::SetHeroesMoveSpeed(int speed){ heroes_speed = (10 <= speed ? 10 : speed); }

/* set battle speed: 0 - 10 */
void Settings::SetBattleSpeed(int speed) { battle_speed = (10 <= speed ? 10 : speed); }

void Settings::SetBlitSpeed(int speed) { blit_speed = speed; }

int Settings::BlitSpeed() const { return blit_speed; }

/* set scroll speed: 1 - 4 */
void Settings::SetScrollSpeed(int speed)
{
    switch(speed)
    {
	case SCROLL_SLOW:  scroll_speed = SCROLL_SLOW; break;
	case SCROLL_NORMAL:scroll_speed = SCROLL_NORMAL; break;
	case SCROLL_FAST1: scroll_speed = SCROLL_FAST1; break;
	case SCROLL_FAST2: scroll_speed = SCROLL_FAST2; break;
	default:           scroll_speed = SCROLL_NORMAL; break;
    }
}

/* return full screen */
bool Settings::QVGA() const { return video_mode.w && video_mode.h && (video_mode.w < 640 || video_mode.h < 480); }

bool Settings::UseAltResource() const { return opt_global.Modes(GLOBAL_ALTRESOURCE); }
bool Settings::PriceLoyaltyVersion() const { return opt_global.Modes(GLOBAL_PRICELOYALTY); }
bool Settings::LoadedGameVersion() const { return game_type & Game::TYPE_LOADFILE; }

bool Settings::ShowControlPanel() const { return opt_global.Modes(GLOBAL_SHOWCPANEL); }
bool Settings::ShowRadar() const { return opt_global.Modes(GLOBAL_SHOWRADAR); }
bool Settings::ShowIcons() const { return opt_global.Modes(GLOBAL_SHOWICONS); }
bool Settings::ShowButtons() const { return opt_global.Modes(GLOBAL_SHOWBUTTONS); }
bool Settings::ShowStatus() const { return opt_global.Modes(GLOBAL_SHOWSTATUS); }

/* unicode support */
bool Settings::Unicode() const { return opt_global.Modes(GLOBAL_USEUNICODE); }
/* pocketpc mode */
bool Settings::PocketPC() const { return opt_global.Modes(GLOBAL_POCKETPC); }

/* get video mode */
const Size & Settings::VideoMode() const { return video_mode; }

/* set level debug */
void Settings::SetDebug(int d) { debug = d; }

/**/
void Settings::SetGameDifficulty(int d) { game_difficulty = d; }
void Settings::SetCurrentColor(int color) { players.current_color = color; }

int Settings::SoundVolume() const { return sound_volume; }
int  Settings::MusicVolume() const { return music_volume; }

/* sound volume: 0 - 10 */
void Settings::SetSoundVolume(int v) { sound_volume = 10 <= v ? 10 : v; }

/* music volume: 0 - 10 */
void Settings::SetMusicVolume(int v) { music_volume = 10 <= v ? 10 : v; }

/* check game type */
bool Settings::GameType(int f) const { return game_type & f; }
int Settings::GameType() const { return game_type; }

/* set game type */
void Settings::SetGameType(int type) { game_type = type; }

const Players & Settings::GetPlayers() const
{
    return players;
}

Players & Settings::GetPlayers()
{
    return players;
}

void Settings::SetPreferablyCountPlayers(int c)
{
    preferably_count_players = 6 < c ? 6 : c;
}

int Settings::PreferablyCountPlayers() const
{
    return preferably_count_players;
}

int Settings::GetPort() const
{
    return port;
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

Size Settings::MapsSize() const
{
    return Size(current_maps_file.size_w, current_maps_file.size_h);
}

bool Settings::AllowChangeRace(int f) const
{
    return current_maps_file.rnd_races & f;
}

bool Settings::GameStartWithHeroes() const
{
    return current_maps_file.with_heroes;
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

bool Settings::WinsAllowNormalVictory() const
{
    return current_maps_file.WinsAllowNormalVictory();
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

Point Settings::WinsMapsPositionObject() const
{
    return current_maps_file.WinsMapsPositionObject();
}

Point Settings::LossMapsPositionObject() const
{
    return current_maps_file.LossMapsPositionObject();
}

u32 Settings::LossCountDays() const
{
    return current_maps_file.LossCountDays();
}

void Settings::SetUnicode(bool f)
{
    f ? opt_global.SetModes(GLOBAL_USEUNICODE) : opt_global.ResetModes(GLOBAL_USEUNICODE);
}

void Settings::SetPriceLoyaltyVersion()
{
    opt_global.SetModes(GLOBAL_PRICELOYALTY);
}

void Settings::SetEvilInterface(bool f)
{
    f ? ExtSetModes(GAME_EVIL_INTERFACE) : ExtResetModes(GAME_EVIL_INTERFACE);
}

void Settings::SetHideInterface(bool f)
{
    f ? ExtSetModes(GAME_HIDE_INTERFACE) : ExtResetModes(GAME_HIDE_INTERFACE);
}

void Settings::SetBattleGrid(bool f)
{
    f ? ExtSetModes(GAME_BATTLE_SHOW_GRID) : ExtResetModes(GAME_BATTLE_SHOW_GRID);
}

void Settings::SetBattleMovementShaded(bool f)
{
    f ? ExtSetModes(GAME_BATTLE_SHOW_MOVE_SHADOW) : ExtResetModes(GAME_BATTLE_SHOW_MOVE_SHADOW);
}

void Settings::SetBattleMouseShaded(bool f)
{
    f ? ExtSetModes(GAME_BATTLE_SHOW_MOUSE_SHADOW) : ExtResetModes(GAME_BATTLE_SHOW_MOUSE_SHADOW);
}

void Settings::ResetSound()
{
    opt_global.ResetModes(GLOBAL_SOUND);
}

void Settings::ResetMusic()
{
    opt_global.ResetModes(GLOBAL_MUSIC);
}

void Settings::SetShowPanel(bool f)
{
    f ? opt_global.SetModes(GLOBAL_SHOWCPANEL) : opt_global.ResetModes(GLOBAL_SHOWCPANEL);
}

void Settings::SetShowRadar(bool f)
{
    f ? opt_global.SetModes(GLOBAL_SHOWRADAR) : opt_global.ResetModes(GLOBAL_SHOWRADAR);
}

void Settings::SetShowIcons(bool f)
{
    f ? opt_global.SetModes(GLOBAL_SHOWICONS) : opt_global.ResetModes(GLOBAL_SHOWICONS);
}

void Settings::SetShowButtons(bool f)
{
    f ? opt_global.SetModes(GLOBAL_SHOWBUTTONS) : opt_global.ResetModes(GLOBAL_SHOWBUTTONS);
}

void Settings::SetShowStatus(bool f)
{
    f ? opt_global.SetModes(GLOBAL_SHOWSTATUS) : opt_global.ResetModes(GLOBAL_SHOWSTATUS);
}

bool Settings::CanChangeInGame(u32 f) const
{
    return (f >> 28) == 0x01; // GAME_ and POCKETPC_
}

bool Settings::ExtModes(u32 f) const
{
    const u32 mask = 0x0FFFFFFF;
    switch(f >> 28)
    {
	case 0x01: return opt_game.Modes(f & mask);
	case 0x02: return opt_world.Modes(f & mask);
	case 0x03: return opt_addons.Modes(f & mask);
	case 0x04: return opt_battle.Modes(f & mask);
	default: break;
    }
    return false;
}

const char* Settings::ExtName(u32 f) const
{
    const settings_t* ptr = std::find(settingsFHeroes2,
		ARRAY_COUNT_END(settingsFHeroes2) - 1, f);

    return ptr ? _(ptr->str) : NULL;
}

void Settings::ExtSetModes(u32 f)
{
    const u32 mask = 0x0FFFFFFF;
    switch(f >> 28)
    {
	case 0x01: opt_game.SetModes(f & mask); break;
	case 0x02: opt_world.SetModes(f & mask); break;
	case 0x03: opt_addons.SetModes(f & mask); break;
	case 0x04: opt_battle.SetModes(f & mask); break;
	default: break;
    }
}

void Settings::ExtResetModes(u32 f)
{
    const u32 mask = 0x0FFFFFFF;
    switch(f >> 28)
    {
	case 0x01: opt_game.ResetModes(f & mask); break;
	case 0x02: opt_world.ResetModes(f & mask); break;
	case 0x03: opt_addons.ResetModes(f & mask); break;
	case 0x04: opt_battle.ResetModes(f & mask); break;
	default: break;
    }
}

bool Settings::ExtCastleAllowBuyFromWell() const
{
    return ExtModes(CASTLE_ALLOW_BUY_FROM_WELL);
}

bool Settings::ExtCastleGuildRestorePointsTurn() const
{
    return ExtModes(CASTLE_MAGEGUILD_POINTS_TURN);
}

bool Settings::ExtCastleAllowFlash() const
{
    return ExtModes(GAME_CASTLE_FLASH_BUILDING);
}

bool Settings::ExtCastleAllowGuardians() const
{
    return ExtModes(CASTLE_ALLOW_GUARDIANS);
}

bool Settings::ExtWorldShowVisitedContent() const
{
    return ExtModes(WORLD_SHOW_VISITED_CONTENT);
}

bool Settings::ExtWorldScouteExtended() const
{
    return ExtModes(WORLD_SCOUTING_EXTENDED);
}

bool Settings::ExtGameRememberLastFocus() const
{
    return ExtModes(GAME_REMEMBER_LAST_FOCUS);
}

bool Settings::ExtWorldAbandonedMineRandom() const
{
    return ExtModes(WORLD_ABANDONED_MINE_RANDOM);
}

bool Settings::ExtWorldSaveMonsterBattle() const
{
    return ExtModes(WORLD_SAVE_MONSTER_BATTLE);
}

bool Settings::ExtWorldAllowSetGuardian() const
{
    return ExtModes(WORLD_ALLOW_SET_GUARDIAN);
}

bool Settings::ExtWorldNoRequirementsForArtifacts() const
{
    return ExtModes(WORLD_NOREQ_FOR_ARTIFACTS);
}

bool Settings::ExtWorldArtifactCrystalBall() const
{
    return ExtModes(WORLD_ARTIFACT_CRYSTAL_BALL);
}

bool Settings::ExtWorldOnlyFirstMonsterAttack() const
{
    return ExtModes(WORLD_ONLY_FIRST_MONSTER_ATTACK);
}

bool Settings::ExtWorldEyeEagleAsScholar() const
{
    return ExtModes(WORLD_EYE_EAGLE_AS_SCHOLAR);
}

bool Settings::ExtHeroBuySpellBookFromShrine() const
{
    return ExtModes(HEROES_BUY_BOOK_FROM_SHRINES);
}

bool Settings::ExtHeroRecruitCostDependedFromLevel() const
{
    return ExtModes(HEROES_COST_DEPENDED_FROM_LEVEL);
}

bool Settings::ExtHeroPatrolAllowPickup() const
{
    return ExtModes(HEROES_PATROL_ALLOW_PICKUP);
}

bool Settings::ExtHeroRememberPointsForRetreating() const
{
    return ExtModes(HEROES_REMEMBER_POINTS_RETREAT);
}

bool Settings::ExtHeroSurrenderingGiveExp() const
{
    return ExtModes(HEROES_SURRENDERING_GIVE_EXP);
}

bool Settings::ExtHeroRecalculateMovement() const
{
    return ExtModes(HEROES_RECALCULATE_MOVEMENT);
}

bool Settings::ExtHeroLearnSpellsWithDay() const
{
    return ExtModes(HEROES_LEARN_SPELLS_WITH_DAY);
}

bool Settings::ExtUnionsAllowCastleVisiting() const
{
    return ExtModes(UNIONS_ALLOW_CASTLE_VISITING);
}

bool Settings::ExtUnionsAllowHeroesMeetings() const
{
    return ExtModes(UNIONS_ALLOW_HERO_MEETINGS);
}

bool Settings::ExtUnionsAllowViewMaps() const
{
    return true;
}

bool Settings::ExtBattleShowDamage() const
{
    return ExtModes(GAME_BATTLE_SHOW_DAMAGE);
}

bool Settings::ExtBattleSkipIncreaseDefense() const
{
    return ExtModes(BATTLE_SKIP_INCREASE_DEFENSE);
}

bool Settings::ExtHeroAllowTranscribingScroll() const
{
    return ExtModes(HEROES_TRANSCRIBING_SCROLLS);
}

bool Settings::ExtHeroAutoMove2BattleTarget() const
{
    return ExtModes(HEROES_AUTO_MOVE_BATTLE_DST);
}

bool Settings::ExtBattleSoftWait() const
{
    return ExtModes(BATTLE_SOFT_WAITING);
}

bool Settings::ExtBattleShowGrid() const
{
    return ExtModes(GAME_BATTLE_SHOW_GRID);
}

bool Settings::ExtBattleShowMouseShadow() const
{
    return ExtModes(GAME_BATTLE_SHOW_MOUSE_SHADOW);
}

bool Settings::ExtBattleShowMoveShadow() const
{
    return ExtModes(GAME_BATTLE_SHOW_MOVE_SHADOW);
}

bool Settings::ExtBattleObjectsArchersPenalty() const
{
    return ExtModes(BATTLE_OBJECTS_ARCHERS_PENALTY);
}

bool Settings::ExtBattleMergeArmies() const
{
    return ExtModes(BATTLE_MERGE_ARMIES);
}

bool Settings::ExtBattleArchmageCanResistBadMagic() const
{
    return ExtModes(BATTLE_ARCHMAGE_RESIST_BAD_SPELL);
}

bool Settings::ExtBattleMagicTroopCanResist() const
{
    return ExtModes(BATTLE_MAGIC_TROOP_RESIST);
}

bool Settings::ExtGameRewriteConfirm() const
{
    return ExtModes(GAME_SAVE_REWRITE_CONFIRM);
}

bool Settings::ExtGameAutosaveConfirm() const
{
    return ExtModes(GAME_ALSO_CONFIRM_AUTOSAVE);
}

bool Settings::ExtPocketHideCursor() const
{
    return ExtModes(POCKETPC_HIDE_CURSOR);
}

bool Settings::ExtGameShowSystemInfo() const
{
    return ExtModes(GAME_SHOW_SYSTEM_INFO);
}

bool Settings::ExtGameAutosaveBeginOfDay() const
{
    return ExtModes(GAME_AUTOSAVE_BEGIN_DAY);
}

bool Settings::ExtGameAutosaveOn() const
{
    return ExtModes(GAME_AUTOSAVE_ON);
}

bool Settings::ExtGameUseFade() const
{
    return video_mode.w == 640 && video_mode.h == 480 && ExtModes(GAME_USE_FADE);
}

bool Settings::ExtGameShowSDL() const
{
    return ExtModes(GAME_SHOW_SDL_LOGO);
}

bool Settings::ExtGameEvilInterface() const
{
    return ExtModes(GAME_EVIL_INTERFACE);
}

bool Settings::ExtGameDynamicInterface() const
{
    return ExtModes(GAME_DYNAMIC_INTERFACE);
}

bool Settings::ExtGameHideInterface() const
{
    return ExtModes(GAME_HIDE_INTERFACE);
}

bool Settings::ExtPocketLowMemory() const
{
    return ExtModes(POCKETPC_LOW_MEMORY);
}

bool Settings::ExtPocketTapMode() const
{
    return ExtModes(POCKETPC_TAP_MODE);
}

bool Settings::ExtPocketDragDropScroll() const
{
    return ExtModes(POCKETPC_DRAG_DROP_SCROLL);
}

bool Settings::ExtCastleAllowRecruitSpecialHeroes() const
{
    return PriceLoyaltyVersion() && ExtModes(CASTLE_ALLOW_RECRUITS_SPECIAL);
}

bool Settings::ExtWorldNewVersionWeekOf() const
{
    return ExtModes(WORLD_NEW_VERSION_WEEKOF);
}

bool Settings::ExtWorldBanWeekOf() const
{
    return ExtModes(WORLD_BAN_WEEKOF);
}

bool Settings::ExtWorldBanMonthOfMonsters() const
{
    return ExtModes(WORLD_BAN_MONTHOF_MONSTERS);
}

bool Settings::ExtWorldArtesianSpringSeparatelyVisit() const
{
    return ExtModes(WORLD_ARTSPRING_SEPARATELY_VISIT);
}

bool Settings::ExtWorldBanPlagues() const
{
    return ExtModes(WORLD_BAN_PLAGUES);
}

bool Settings::ExtBattleReverseWaitOrder() const
{
    return ExtModes(BATTLE_REVERSE_WAIT_ORDER);
}

bool Settings::ExtWorldStartHeroLossCond4Humans() const
{
    return ExtModes(WORLD_STARTHERO_LOSSCOND4HUMANS);
}

bool Settings::ExtHeroAllowBannedSecSkillsUpgrade() const
{
    return ExtModes(HEROES_ALLOW_BANNED_SECSKILLS);
}

bool Settings::ExtWorldOneHeroHiredEveryWeek() const
{
    return ExtModes(WORLD_1HERO_HIRED_EVERY_WEEK);
}

bool Settings::ExtCastleOneHeroHiredEveryWeek() const
{
    return ExtModes(CASTLE_1HERO_HIRED_EVERY_WEEK);
}

bool Settings::ExtWorldDwellingsAccumulateUnits() const
{
    return ExtModes(WORLD_DWELLING_ACCUMULATE_UNITS);
}

bool Settings::ExtWorldUseUniqueArtifactsML() const
{
    return ExtModes(WORLD_USE_UNIQUE_ARTIFACTS_ML);
}

bool Settings::ExtWorldUseUniqueArtifactsRS() const
{
    return ExtModes(WORLD_USE_UNIQUE_ARTIFACTS_RS);
}

bool Settings::ExtWorldUseUniqueArtifactsPS() const
{
    return ExtModes(WORLD_USE_UNIQUE_ARTIFACTS_PS);
}

bool Settings::ExtWorldUseUniqueArtifactsSS() const
{
    return ExtModes(WORLD_USE_UNIQUE_ARTIFACTS_SS);
}

bool Settings::ExtHeroArenaCanChoiseAnySkills() const
{
    return ExtModes(HEROES_ARENA_ANY_SKILLS);
}

bool Settings::ExtWorldExtObjectsCaptured() const
{
    return ExtModes(WORLD_EXT_OBJECTS_CAPTURED);
}

bool Settings::ExtWorldGuardianObjectsTwoDefense() const
{
    return ExtModes(WORLD_GUARDIAN_TWO_DEFENSE);
}

bool Settings::ExtWorldDisableBarrowMounds() const
{
    return ExtModes(WORLD_DISABLE_BARROW_MOUNDS);
}

bool Settings::ExtGameContinueAfterVictory() const
{
    return ExtModes(GAME_CONTINUE_AFTER_VICTORY);
}

const Point & Settings::PosRadar() const { return pos_radr; }
const Point & Settings::PosButtons() const { return pos_bttn; }
const Point & Settings::PosIcons() const { return pos_icon; }
const Point & Settings::PosStatus() const { return pos_stat; }

void Settings::SetPosRadar(const Point & pt) { pos_radr = pt; }
void Settings::SetPosButtons(const Point & pt) { pos_bttn = pt; }
void Settings::SetPosIcons(const Point & pt) { pos_icon = pt; }
void Settings::SetPosStatus(const Point & pt) { pos_stat = pt; }

void Settings::BinarySave() const
{
    const std::string fname = System::ConcatePath(GetSaveDir(), "fheroes2.bin");

    StreamFile fs;
    fs.setbigendian(true);

    if(fs.open(fname, "wb"))
    {
	fs << static_cast<u16>(CURRENT_FORMAT_VERSION) <<
	    opt_game << opt_world << opt_battle << opt_addons <<
	    pos_radr << pos_bttn << pos_icon << pos_stat;
    }
}

void Settings::BinaryLoad()
{
    std::string fname = System::ConcatePath(GetSaveDir(), "fheroes2.bin");

    if(! System::IsFile(fname))
	fname = GetLastFile("", "fheroes2.bin");

    StreamFile fs;
    fs.setbigendian(true);

    if(fs.open(fname, "rb"))
    {
	u16 version = 0;

	fs >> version >>
	    opt_game >> opt_world >> opt_battle >> opt_addons >>
	    pos_radr >> pos_bttn >> pos_icon >> pos_stat;
    }
}

void Settings::SetMemoryLimit(u32 limit)
{
    memory_limit = limit;
}

u32 Settings::MemoryLimit() const
{
    return memory_limit;
}

bool Settings::FullScreen() const
{
    return System::isEmbededDevice() ||
	opt_global.Modes(GLOBAL_FULLSCREEN);
}

StreamBase & operator<< (StreamBase & msg, const Settings & conf)
{
    return msg <<
       // lang
       conf.force_lang <<
       // current maps
       conf.current_maps_file <<
       // game config
       conf.game_difficulty <<
       conf.game_type <<
       conf.preferably_count_players <<
       conf.debug <<
       conf.opt_game << conf.opt_world << conf.opt_battle << conf.opt_addons <<
       conf.players;
}

StreamBase & operator>> (StreamBase & msg, Settings & conf)
{
    std::string lang;

    msg >> lang;

    if(lang != "en" && lang != conf.force_lang && !conf.Unicode())
    {
        std::string warningMessage("This is an saved game is localized for lang = ");
        warningMessage.append(lang);
        warningMessage.append(", and most of the messages will be displayed incorrectly.\n \n");
        warningMessage.append("(tip: set unicode = on)");
        Dialog::Message("Warning!", warningMessage, Font::BIG, Dialog::OK);
    }

    int debug;
    u32 opt_game = 0; // skip: settings

    // map file
    msg >> conf.current_maps_file >>
	conf.game_difficulty >> conf.game_type >>
	conf.preferably_count_players >> debug >>
	opt_game >> conf.opt_world >> conf.opt_battle >> conf.opt_addons >>
	conf.players;

#ifndef WITH_DEBUG
    conf.debug = debug;
#endif

    return msg;
}
