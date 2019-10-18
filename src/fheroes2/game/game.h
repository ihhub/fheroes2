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

#ifndef H2GAME_H
#define H2GAME_H

#include <string>
#include "rect.h"
#include "types.h"
#include "gamedefs.h"

class Surface;
class Kingdom;
class Player;
class Heroes;
class Castle;

namespace Game
{
    enum
    {
	CANCEL = 0,
	QUITGAME,
	MAINMENU,
        NEWGAME,
        LOADGAME,
        HIGHSCORES,
        CREDITS,
        NEWSTANDARD,
        NEWCAMPAIN,
        NEWMULTI,
        NEWHOTSEAT,
        NEWNETWORK,
	NEWBATTLEONLY,
        LOADSTANDARD,
        LOADCAMPAIN,
        LOADMULTI,
        SCENARIOINFO,
        SELECTSCENARIO,
	STARTGAME,
	SAVEGAME,
	EDITMAINMENU,
	EDITNEWMAP,
	EDITLOADMAP,
	EDITSAVEMAP,
	EDITSTART,
	ENDTURN,
	TESTING
    };

    void Init(void);

    const std::string & GetLastSavename(void);
    void		SetLastSavename(const std::string &);
    void		SetLoadVersion(int);
    int			GetLoadVersion(void);

    // type_t
    enum { TYPE_MENU = 0, TYPE_STANDARD = 0x01, TYPE_CAMPAIGN = 0x02, TYPE_HOTSEAT = 0x04, TYPE_NETWORK = 0x08, TYPE_BATTLEONLY = 0x10, TYPE_LOADFILE = 0x80, TYPE_MULTI = TYPE_HOTSEAT | TYPE_NETWORK };
    // distance_t
    enum { VIEW_TOWN  = 0, VIEW_CASTLE = 1, VIEW_HEROES = 2, VIEW_TELESCOPE = 3, VIEW_OBSERVATION_TOWER = 4, VIEW_MAGI_EYES = 5, VIEW_LIGHT_HOUSE = 6 };

    enum
    {
	EVENT_NONE,
	EVENT_BUTTON_NEWGAME,
	EVENT_BUTTON_LOADGAME,
	EVENT_BUTTON_HIGHSCORES,
	EVENT_BUTTON_CREDITS,
	EVENT_BUTTON_STANDARD,
	EVENT_BUTTON_CAMPAIN,
	EVENT_BUTTON_MULTI,
	EVENT_BUTTON_SETTINGS,
	EVENT_BUTTON_SELECT,
	EVENT_BUTTON_HOTSEAT,
	EVENT_BUTTON_NETWORK,
	EVENT_BUTTON_HOST,
	EVENT_BUTTON_GUEST,
	EVENT_BUTTON_BATTLEONLY,
	EVENT_DEFAULT_READY,
	EVENT_DEFAULT_EXIT,
	EVENT_DEFAULT_LEFT,
	EVENT_DEFAULT_RIGHT,
	EVENT_SYSTEM_FULLSCREEN,
	EVENT_SYSTEM_SCREENSHOT,
	EVENT_SYSTEM_DEBUG1,
	EVENT_SYSTEM_DEBUG2,
	EVENT_SLEEPHERO,
	EVENT_ENDTURN,
	EVENT_NEXTHERO,
	EVENT_NEXTTOWN,
	EVENT_CONTINUE,
	EVENT_SAVEGAME,
	EVENT_LOADGAME,
	EVENT_FILEOPTIONS,
	EVENT_PUZZLEMAPS,
	EVENT_INFOGAME,
	EVENT_DIGARTIFACT,
	EVENT_CASTSPELL,
	EVENT_DEFAULTACTION,
	EVENT_OPENFOCUS,
	EVENT_SYSTEMOPTIONS,
	EVENT_BATTLE_CASTSPELL,
	EVENT_BATTLE_RETREAT,
	EVENT_BATTLE_SURRENDER,
	EVENT_BATTLE_AUTOSWITCH,
	EVENT_BATTLE_OPTIONS,
	EVENT_BATTLE_HARDSKIP,
	EVENT_BATTLE_SOFTSKIP,
	EVENT_MOVELEFT,
	EVENT_MOVERIGHT,
	EVENT_MOVETOP,
	EVENT_MOVEBOTTOM,
	EVENT_MOVETOPLEFT,
	EVENT_MOVETOPRIGHT,
	EVENT_MOVEBOTTOMLEFT,
	EVENT_MOVEBOTTOMRIGHT,
	EVENT_SCROLLLEFT,
	EVENT_SCROLLRIGHT,
	EVENT_SCROLLUP,
	EVENT_SCROLLDOWN,
	EVENT_CTRLPANEL,
	EVENT_SHOWRADAR,
	EVENT_SHOWBUTTONS,
	EVENT_SHOWSTATUS,
	EVENT_SHOWICONS,
	EVENT_SWITCHGROUP,
	EVENT_EMULATETOGGLE,
	EVENT_LAST
    };

    bool HotKeyPressEvent(int);

    enum
    {
	SCROLL_DELAY,
	MAIN_MENU_DELAY,
	MAPS_DELAY,
	CASTLE_TAVERN_DELAY,
	CASTLE_AROUND_DELAY,
	CASTLE_BUYHERO_DELAY,
	CASTLE_BUILD_DELAY,
	HEROES_MOVE_DELAY,
	HEROES_FADE_DELAY,
	HEROES_PICKUP_DELAY,
	PUZZLE_FADE_DELAY,
	BATTLE_DIALOG_DELAY,
	BATTLE_FRAME_DELAY,
	BATTLE_MISSILE_DELAY,
	BATTLE_SPELL_DELAY,
	BATTLE_DISRUPTING_DELAY,
	BATTLE_CATAPULT_DELAY,
	BATTLE_CATAPULT2_DELAY,
	BATTLE_CATAPULT3_DELAY,
	BATTLE_BRIDGE_DELAY,
	BATTLE_IDLE_DELAY,
	BATTLE_IDLE2_DELAY,
	BATTLE_OPPONENTS_DELAY,
	BATTLE_FLAGS_DELAY,
	BATTLE_POPUP_DELAY,
	AUTOHIDE_STATUS_DELAY,
	//
	CURRENT_HERO_DELAY,
	CURRENT_AI_DELAY,
	//
	LAST_DELAY
    };

    bool	AnimateInfrequentDelay(int);
    void	AnimateResetDelay(int);
    void	UpdateHeroesMoveSpeed(void);
    void	UpdateBattleSpeed(void);
    int		MainMenu(void);
    int		NewGame(void);
    int		LoadGame(void);
    int		HighScores(bool);
    int		Credits(void);
    int		NewStandard(void);
    int		NewCampain(void);
    int		NewMulti(void);
    int		NewHotSeat(void);
    int		NewNetwork(void);
    int		NewBattleOnly(void);
    int		LoadStandard(void);
    int		LoadCampain(void);
    int		LoadMulti(void);
    int		ScenarioInfo(void);
    int		SelectScenario(void);
    int		StartGame(void);
    int		StartBattleOnly(void);
    int		NetworkHost(void);
    int		NetworkGuest(void);
    int		Testing(int);

    void	DrawInterface(void);
    void	SetFixVideoMode(void);
    void	EnvironmentSoundMixer(void);
    int		GetKingdomColors(void);
    int		GetActualKingdomColors(void);
    void	DialogPlayers(int color, std::string);
    void	SetCurrentMusic(int);
    int		CurrentMusic(void);
    u32 &	CastleAnimationFrame(void);
    u32 &	MapsAnimationFrame(void);
    u32		GetRating(void);
    u32		GetGameOverScores(void);
    u32		GetLostTownDays(void);
    u32		GetViewDistance(u32);
    u32		GetWhirlpoolPercent(void);
    u32		SelectCountPlayers(void);
    void	ShowLoadMapsText(void);
    void	PlayPickupSound(void);
    void	DisableChangeMusic(bool);
    bool	ChangeMusicDisabled(void);
    void	OpenHeroesDialog(Heroes &);
    void	OpenCastleDialog(Castle &);
    std::string GetEncodeString(const std::string &);

    namespace Editor
    {
	int	MainMenu(void);
	int	NewMaps(void);
	int	LoadMaps(void);
	int	StartGame(void);
	int	StartGame(void);
    }

    u32		GetStep4Player(u32, u32, u32);
    std::string CountScoute(u32 count, int scoute, bool shorts = false);
}

#define HotKeyCloseWindow (Game::HotKeyPressEvent(Game::EVENT_DEFAULT_EXIT) || Game::HotKeyPressEvent(Game::EVENT_DEFAULT_READY))

#endif
