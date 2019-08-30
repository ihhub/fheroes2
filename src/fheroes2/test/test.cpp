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

#include "agg.h"
#include "settings.h"
#include "gamedefs.h"
#include "battle.h"
#include "world.h"
#include "army.h"
#include "game.h"
#include "castle.h"
#include "kingdom.h"
#include "heroes.h"
#include "test.h"

#ifndef BUILD_RELEASE

void RunTest1(void);
void RunTest2(void);
void RunTest3(void);

void TestMonsterSprite(void);

void Test::Run(int num)
{
    switch(num)
    {
	case 1: RunTest1(); break;
	case 2: RunTest2(); break;
	case 3: RunTest3(); break;

	case 9: TestMonsterSprite(); break;

	default: DEBUG(DBG_ENGINE, DBG_WARN, "unknown test"); break;
    }
}

void RunTest1(void)
{
    VERBOSE("Run Test1");
}

void RunTest2(void)
{
    VERBOSE("Run Test2");

    Display & display = Display::Get();
    LocalEvent & le = LocalEvent::Get();

    display.Fill(RGBA(0x85, 0x85, 0x85));
    Point pt;

    // test alpha sprite
    Sprite sp1 = AGG::GetICN(ICN::BTNSHNGL, 1);

    sp1.Blit(pt);
    pt.x += sp1.w() + 20;

    Surface sf1 = sp1.GetSurface();
    sf1.SetAlphaMod(50);

    sf1.Blit(pt, display);
    pt.x += sf1.w() + 20;

    // test alpha sprite with shadow
    Sprite sp2 = AGG::GetICN(ICN::DRAGBLAK, 1);
    pt.y = 130;
    pt.x = 0;

    sp2.Blit(pt, display);
    pt.x += sp2.w() + 20;

    Surface sf2 = sp2.GetSurface();
    sf2.Blit(pt, display);
    pt.x += sf2.w() + 20;

    VERBOSE(sp2.Info());
    VERBOSE(sf2.Info());

    sf2.SetAlphaMod(50);
    sf2.Blit(pt, display);
    pt.x += sf2.w() + 20;

    // contour, stensil, change color
    Surface sf3 = sp2.RenderContour(RGBA(0xFF, 0xFF, 0));
    pt.x = 0;
    pt.y = 260;

    sf3.Blit(pt, display);
    pt.x += sf3.w() + 20;

    RGBA color = RGBA(0x80, 0x50, 0x30);
    Surface sf4 = sp2.RenderStencil(color);
    sf4.Blit(pt, display);
    pt.x += sf4.w() + 20;

    Surface sf5 = sf4.RenderChangeColor(color, RGBA(0x30, 0x90, 0x30));
    sf5.Blit(pt, display);
    pt.x += sf5.w() + 20;

    //
    display.Flip();

    while(le.HandleEvents())
    {
        if(Game::HotKeyPressEvent(Game::EVENT_DEFAULT_EXIT)) break;
    }
}

ListFiles GetMapsFiles(const char*);

void RunTest3(void)
{
    VERBOSE("Run Test3");

    ListFiles maps = GetMapsFiles(".mp2");
    if(maps.empty()) return;

    const std::string & amap = maps.front();
    Settings & conf = Settings::Get();

    Maps::FileInfo fi;
    if(!fi.ReadMP2(amap)) return;
    
    conf.SetCurrentFileInfo(fi);
    world.LoadMapMP2(amap);

    Heroes & hero1 = *world.GetHeroes(Heroes::SANDYSANDY);
    Heroes & hero2 = *world.GetHeroes(Heroes::BAX);

    Players & players = conf.GetPlayers();

    int mycolor = Color::GetFirst(players.GetColors(CONTROL_HUMAN));
    int aicolor = Color::GetFirst(players.GetColors((CONTROL_AI)));

    players.SetPlayerControl(mycolor, CONTROL_HUMAN);
    players.SetPlayerControl(aicolor, CONTROL_HUMAN);

    Kingdom & kingdom1 = world.GetKingdom(mycolor);
    Kingdom & kingdom2 = world.GetKingdom(aicolor);

    conf.SetCurrentColor(mycolor);
    conf.SetGameType(Game::TYPE_BATTLEONLY);

    players.SetStartGame();

    hero1.SetSpellPoints(150);

    int xx = world.w() / 2;
    int yy = world.h() / 2;

    if(kingdom1.GetCastles().size())
    hero1.Recruit(kingdom1.GetColor(), Point(xx, yy));
    hero2.Recruit(kingdom2.GetColor(), Point(xx, yy + 1));

    Army & army1 = hero1.GetArmy();

    Castle* castle = kingdom2.GetCastles().at(0);
    castle->ActionNewDay();
    castle->BuyBuilding(BUILD_MAGEGUILD1);
    castle->ActionNewDay();
    castle->BuyBuilding(BUILD_CAPTAIN);
    castle->ActionNewDay();
    castle->BuyBuilding(BUILD_MOAT);

    //Army army2;
    //Army & army2 = hero2.GetArmy();
    Army & army2 = castle->GetArmy();
    if(army2.GetCommander())
    {
	army2.GetCommander()->SpellBookActivate();
	army2.GetCommander()->AppendSpellToBook(Spell::SHIELD, true);
    }

    army1.Clean();
    //army1.JoinTroop(Monster::PHOENIX, 10);
    //army1.GetTroop(0)->Set(Monster::ARCHER, 30);
    army1.GetTroop(1)->Set(Monster::BOAR, 20);
    army1.GetTroop(2)->Set(Monster::OGRE_LORD, 20);

    //army1.JoinTroop(Monster::Rand(Monster::LEVEL1), 30);
    //army1.JoinTroop(Monster::Rand(Monster::LEVEL2), 20);
    //army1.JoinTroop(Monster::Rand(Monster::LEVEL3), 10);

    army2.Clean();
    army2.GetTroop(0)->Set(Monster::BOAR, 20);
    army2.GetTroop(2)->Set(Monster::OGRE_LORD, 20);
//    army2.at(0) = Troop(Monster::OGRE, 1);
//    army2.at(1) = Troop(Monster::DWARF, 2);
//    army2.at(2) = Troop(Monster::DWARF, 2);
//    army2.at(3) = Troop(Monster::DWARF, 2);
//    army2.at(4) = Troop(Monster::DWARF, 2);
//    army2.JoinTroop(static_cast<Monster::monster_t>(1), 10);
//    army2.JoinTroop(static_cast<Monster::monster_t>(4), 10);
//    army2.JoinTroop(static_cast<Monster::monster_t>(6), 10);
//    army2.JoinTroop(static_cast<Monster::monster_t>(8), 10);

    Battle::Loader(army1, army2, army1.GetCommander()->GetIndex());
}

#endif
