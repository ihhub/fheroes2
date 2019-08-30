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

#include <map>
#include <algorithm>

#include "system.h"
#include "gamedefs.h"
#include "tinyconfig.h"
#include "settings.h"
#include "maps_tiles.h"
#include "ground.h"
#include "world.h"
#include "kingdom.h"
#include "castle.h"
#include "mp2.h"
#include "agg.h"
#include "test.h"
#include "cursor.h"
#include "monster.h"
#include "spell.h"
#include "payment.h"
#include "profit.h"
#include "buildinginfo.h"
#include "skill.h"
#include "battle.h"
#include "tools.h"
#include "difficulty.h"
#include "game_interface.h"
#include "game_static.h"
#include "ai.h"
#include "game.h"

namespace Game
{
    u32		GetMixerChannelFromObject(const Maps::Tiles &);
    void	AnimateDelaysInitialize(void);
    void	KeyboardGlobalFilter(int, int);
    void	UpdateGlobalDefines(const std::string &);
    void	LoadExternalResource(const Settings &);

    void	HotKeysDefaults(void);
    void	HotKeysLoad(const std::string &);

    bool	disable_change_music = false;
    int		current_music = MUS::UNKNOWN;
    u32		castle_animation_frame = 0;
    u32		maps_animation_frame = 0;
    std::string last_name;
    int		save_version = CURRENT_FORMAT_VERSION;
    std::vector<int>
		reserved_vols(LOOPXX_COUNT, 0);
}

void Game::SetLoadVersion(int ver)
{
    save_version = ver;
}

int Game::GetLoadVersion(void)
{
    return save_version;
}

const std::string & Game::GetLastSavename(void)
{
    return last_name;
}

void Game::SetLastSavename(const std::string & name)
{
    last_name = name;
}

int Game::Testing(int t)
{
#ifndef BUILD_RELEASE
    Test::Run(t);
    return Game::QUITGAME;
#else
    return Game::MAINMENU;
#endif
}

int Game::Credits(void)
{
    const Settings & conf = Settings::Get();

    std::string str;
    str.reserve(200);

    str.append("version: ");
    str.append(conf.GetVersion());
    str.append("\n \n");
    str.append("This program is distributed under the terms of the GPL v2.");
    str.append("\n");
    str.append("AI engine: ");
    str.append(AI::Type());
    str.append(", license: ");
    str.append(AI::License());
    str.append("\n \n");
    str.append("Site project:\n");
    str.append("http://sf.net/projects/fheroes2");
    str.append("\n \n");
    str.append("Authors:\n");
    str.append("Andrey Afletdinov, maintainer\n");
    str.append("email: fheroes2 at gmail.com\n");

    Dialog::Message("Free Heroes II Engine", str, Font::SMALL, Dialog::OK);

    //VERBOSE("Credits: under construction.");

    return Game::MAINMENU;
}

bool Game::ChangeMusicDisabled(void)
{
    return disable_change_music;
}

void Game::DisableChangeMusic(bool f)
{
    //disable_change_music = f;
}

void Game::Init(void)
{
    Settings & conf = Settings::Get();
    LocalEvent & le = LocalEvent::Get();

    // update all global defines
    if(conf.UseAltResource()) LoadExternalResource(conf);

    // default events
    le.SetStateDefaults();

    // set global events
    le.SetGlobalFilterMouseEvents(Cursor::Redraw);
    le.SetGlobalFilterKeysEvents(Game::KeyboardGlobalFilter);
    le.SetGlobalFilter(true);

    le.SetTapMode(conf.ExtPocketTapMode());

    Game::AnimateDelaysInitialize();

    HotKeysDefaults();

    const std::string hotkeys = Settings::GetLastFile("", "fheroes2.key");
    Game::HotKeysLoad(hotkeys);
}

int Game::CurrentMusic(void)
{
    return current_music;
}

void Game::SetCurrentMusic(int mus)
{
    current_music = mus;
}

u32 & Game::MapsAnimationFrame(void)
{
    return maps_animation_frame;
}

u32 & Game::CastleAnimationFrame(void)
{
    return castle_animation_frame;
}

void Game::SetFixVideoMode(void)
{
    const Settings & conf = Settings::Get();

    Size fixsize(conf.VideoMode());
    Size mapSize = conf.MapsSize();

    u32 max_x = Settings::Get().ExtGameHideInterface() ? mapSize.w * TILEWIDTH :
			    (6 + mapSize.w) * TILEWIDTH; // RADARWIDTH + 3 * BORDERWIDTH
    u32 max_y = Settings::Get().ExtGameHideInterface() ? mapSize.h * TILEWIDTH :
			    (1 + mapSize.h) * TILEWIDTH; // 2 * BORDERWIDTH

    if(conf.VideoMode().w > max_x) fixsize.w = max_x;
    if(conf.VideoMode().h > max_y) fixsize.h = max_y;

    Display::Get().SetVideoMode(fixsize.w, fixsize.h, conf.FullScreen());
}

/* play all sound from focus area game */
void Game::EnvironmentSoundMixer(void)
{
    const Point abs_pt(Interface::GetFocusCenter());
    const Settings & conf = Settings::Get();

    if(conf.Sound())
    {
	std::fill(reserved_vols.begin(), reserved_vols.end(), 0);

        // scan 4x4 square from focus
        for(s32 yy = abs_pt.y - 3; yy <= abs_pt.y + 3; ++yy)
    	{
    	    for(s32 xx = abs_pt.x - 3; xx <= abs_pt.x + 3; ++xx)
	    {
		if(Maps::isValidAbsPoint(xx, yy))
		{
		    const u32 channel = GetMixerChannelFromObject(world.GetTiles(xx, yy));
    		    if(channel < reserved_vols.size())
		    {
			// calculation volume
    			const int length = std::max(std::abs(xx - abs_pt.x), std::abs(yy - abs_pt.y));
			const int volume = (2 < length ? 4 : (1 < length ? 8 : (0 < length ? 12 : 16))) * Mixer::MaxVolume() / 16;

			if(volume > reserved_vols[channel]) reserved_vols[channel] = volume;
		    }
		}
	    }
	}

	AGG::LoadLOOPXXSounds(reserved_vols);
    }
}

u32 Game::GetMixerChannelFromObject(const Maps::Tiles & tile)
{
    // force: check stream
    if(tile.isStream()) return 13;

    return M82::GetIndexLOOP00XXFromObject(tile.GetObject(false));
}

u32 Game::GetRating(void)
{
    Settings & conf = Settings::Get();
    u32 rating = 50;

    switch(conf.MapsDifficulty())
    {
        case Difficulty::NORMAL:     rating += 20; break;
        case Difficulty::HARD:       rating += 40; break;
        case Difficulty::EXPERT:
        case Difficulty::IMPOSSIBLE:	rating += 80; break;
	default: break;
    }

    switch(conf.GameDifficulty())
    {
        case Difficulty::NORMAL:     rating += 30; break;
        case Difficulty::HARD:       rating += 50; break;
        case Difficulty::EXPERT:	rating += 70; break;
        case Difficulty::IMPOSSIBLE:	rating += 90; break;
	default: break;
    }

    return rating;
}

u32 Game::GetGameOverScores(void)
{
    Settings & conf = Settings::Get();

    u32 k_size = 0;

    switch(conf.MapsSize().w)
    {
	case Maps::SMALL:	k_size = 140; break;
	case Maps::MEDIUM:	k_size = 100; break;
	case Maps::LARGE:	k_size =  80; break;
	case Maps::XLARGE:	k_size =  60; break;
	default: break;
    }

    u32 flag = 0;
    u32 nk = 0;
    u32 end_days = world.CountDay();

    for(u32 ii = 1; ii <= end_days; ++ii)
    {
	nk = ii * k_size / 100;

	if(0 == flag && nk > 60){ end_days = ii + (world.CountDay() - ii) / 2; flag = 1; }
	else
	if(1 == flag && nk > 120) end_days = ii + (world.CountDay() - ii) / 2;
	else
	if(nk > 180) break;
    }

    return GetRating() * (200 - nk) / 100;
}

void Game::ShowLoadMapsText(void)
{
    Display & display = Display::Get();
    const Rect pos(0, display.h() / 2, display.w(), display.h() / 2);
    TextBox text(_("Maps Loading..."), Font::BIG, pos.w);

    // blit test
    display.Fill(ColorBlack);
    text.Blit(pos, display);
    display.Flip();
}

u32 Game::GetLostTownDays(void)
{
    return GameStatic::GetGameOverLostDays();
}

u32 Game::GetViewDistance(u32 d)
{
    return GameStatic::GetOverViewDistance(d);
}

void Game::UpdateGlobalDefines(const std::string & spec)
{
#ifdef WITH_XML
    // parse profits.xml
    TiXmlDocument doc;
    const TiXmlElement* xml_globals = NULL;

    if(doc.LoadFile(spec.c_str()) &&
	NULL != (xml_globals = doc.FirstChildElement("globals")))
    {
	// starting_resource
	KingdomUpdateStartingResource(xml_globals->FirstChildElement("starting_resource"));
	// view_distance
	OverViewUpdateStatic(xml_globals->FirstChildElement("view_distance"));
	// kingdom
	KingdomUpdateStatic(xml_globals->FirstChildElement("kingdom"));
	// game_over
	GameOverUpdateStatic(xml_globals->FirstChildElement("game_over"));
	// whirlpool
	WhirlpoolUpdateStatic(xml_globals->FirstChildElement("whirlpool"));
	// heroes
	HeroesUpdateStatic(xml_globals->FirstChildElement("heroes"));
	// castle_extra_growth
	CastleUpdateGrowth(xml_globals->FirstChildElement("castle_extra_growth"));
	// monster upgrade ratio
	MonsterUpdateStatic(xml_globals->FirstChildElement("monster_upgrade"));
    }
    else
    VERBOSE(spec << ": " << doc.ErrorDesc());
#endif
}

u32 Game::GetWhirlpoolPercent(void)
{
    return GameStatic::GetLostOnWhirlpoolPercent();
}

void Game::LoadExternalResource(const Settings & conf)
{
    std::string spec;
    const std::string prefix_stats = System::ConcatePath("files", "stats");

    // globals.xml
    spec = Settings::GetLastFile(prefix_stats, "globals.xml");

    if(System::IsFile(spec))
	Game::UpdateGlobalDefines(spec);

    // animations.xml
    spec = Settings::GetLastFile(prefix_stats, "animations.xml");

    if(System::IsFile(spec))
	Battle::UpdateMonsterSpriteAnimation(spec);

    // battle.xml
    spec = Settings::GetLastFile(prefix_stats, "battle.xml");

    if(System::IsFile(spec))
	Battle::UpdateMonsterAttributes(spec);

    // monsters.xml
    spec = Settings::GetLastFile(prefix_stats, "monsters.xml");

    if(System::IsFile(spec))
	Monster::UpdateStats(spec);

    // spells.xml
    spec = Settings::GetLastFile(prefix_stats, "spells.xml");

    if(System::IsFile(spec))
	Spell::UpdateStats(spec);

    // artifacts.xml
    spec = Settings::GetLastFile(prefix_stats, "artifacts.xml");

    if(System::IsFile(spec))
	Artifact::UpdateStats(spec);

    // buildings.xml
    spec = Settings::GetLastFile(prefix_stats, "buildings.xml");

    if(System::IsFile(spec))
	BuildingInfo::UpdateCosts(spec);

    // payments.xml
    spec = Settings::GetLastFile(prefix_stats, "payments.xml");

    if(System::IsFile(spec))
	PaymentConditions::UpdateCosts(spec);

    // profits.xml
    spec = Settings::GetLastFile(prefix_stats, "profits.xml");

    if(System::IsFile(spec))
	ProfitConditions::UpdateCosts(spec);

    // skills.xml
    spec = Settings::GetLastFile(prefix_stats, "skills.xml");

    if(System::IsFile(spec))
	Skill::UpdateStats(spec);
}

std::string Game::GetEncodeString(const std::string & str1)
{
    const Settings & conf = Settings::Get();

    // encode name
    if(conf.Unicode() && conf.MapsCharset().size())
	return EncodeString(str1.c_str(), conf.MapsCharset().c_str());

    return str1;
}

int Game::GetKingdomColors(void)
{
    return Settings::Get().GetPlayers().GetColors();
}

int Game::GetActualKingdomColors(void)
{
    return Settings::Get().GetPlayers().GetActualColors();
}

#include <cmath>
std::string Game::CountScoute(u32 count, int scoute, bool shorts)
{
    float infelicity = 0;
    std::string res;

    switch(scoute)
    {
        case Skill::Level::BASIC:
            infelicity = count * 30 / 100.0;
            break;

        case Skill::Level::ADVANCED:
            infelicity = count * 15 / 100.0;
            break;

        case Skill::Level::EXPERT:
            res = shorts ? GetStringShort(count) : GetString(count);
            break;

        default:
            return Army::SizeString(count);
    }

    if(res.empty())
    {
        u32 min = Rand::Get(static_cast<u32>(std::floor(count - infelicity + 0.5)),
			    static_cast<u32>(std::floor(count + infelicity + 0.5)));
        u32 max = 0;

        if(min > count)
        {
            max = min;
            min = static_cast<u32>(std::floor(count - infelicity + 0.5));
        }
        else
            max = static_cast<u32>(std::floor(count + infelicity + 0.5));

        res = GetString(min);

        if(min != max)
        {
            res.append("-");
            res.append(GetString(max));
        }
    }

    return res;
}

void Game::PlayPickupSound(void)
{
    int wav = M82::UNKNOWN;

    switch(Rand::Get(1, 7))
    {
        case 1: wav = M82::PICKUP01; break;
        case 2: wav = M82::PICKUP02; break;
        case 3: wav = M82::PICKUP03; break;
        case 4: wav = M82::PICKUP04; break;
        case 5: wav = M82::PICKUP05; break;
        case 6: wav = M82::PICKUP06; break;
        case 7: wav = M82::PICKUP07; break;

        default: return;
    }

    AGG::PlaySound(wav);
}
