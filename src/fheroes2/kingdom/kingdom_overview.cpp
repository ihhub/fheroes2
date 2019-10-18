/***************************************************************************
 *   Copyright (C) 2012 by Andrey Afletdinov <fheroes2@gmail.com>          *
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
#include "text.h"
#include "cursor.h"
#include "button.h"
#include "settings.h"
#include "castle.h"
#include "heroes.h"
#include "world.h"
#include "kingdom.h"
#include "skill.h"
#include "game.h"
#include "army.h"
#include "army_bar.h"
#include "buildinginfo.h"
#include "interface_icons.h"
#include "interface_list.h"

struct HeroRow
{
    Heroes* 		hero;
    ArmyBar*		armyBar;
    ArtifactsBar*	artifactsBar;
    SecondarySkillsBar*	secskillsBar;
    PrimarySkillsBar*	primskillsBar;

    HeroRow() : hero(NULL), armyBar(NULL), artifactsBar(NULL), secskillsBar(NULL), primskillsBar(NULL) {}
    ~HeroRow()
    {
	Clear();
    }

    void Clear(void)
    {
	if(armyBar) delete armyBar;
	if(artifactsBar) delete artifactsBar;
	if(secskillsBar) delete secskillsBar;
	if(primskillsBar) delete primskillsBar;
    }

    void Init(Heroes* ptr)
    {
	hero = ptr;

	Clear();

	armyBar = new ArmyBar(&hero->GetArmy(), true, false);
	armyBar->SetBackground(Size(41, 53), RGBA(72, 28, 0));
	armyBar->SetColRows(5, 1);
	armyBar->SetHSpace(-1);

	artifactsBar = new ArtifactsBar(hero, true, false);
	artifactsBar->SetColRows(7, 2);
	artifactsBar->SetHSpace(1);
	artifactsBar->SetVSpace(8);
	artifactsBar->SetContent(hero->GetBagArtifacts());

	secskillsBar = new SecondarySkillsBar();
	secskillsBar->SetColRows(4, 2);
        secskillsBar->SetHSpace(-1);
        secskillsBar->SetVSpace(8);
        secskillsBar->SetContent(hero->GetSecondarySkills().ToVector());

	primskillsBar = new PrimarySkillsBar(ptr, true);
	primskillsBar->SetColRows(4, 1);
	primskillsBar->SetHSpace(2);
	primskillsBar->SetTextOff(20, -13);
    }
};


class StatsHeroesList : public Interface::ListBox<HeroRow>
{
    std::vector<HeroRow> content;

public:
    StatsHeroesList(const Point & pt, KingdomHeroes &);

    void RedrawItem(const HeroRow &, s32, s32, bool);
    void RedrawBackground(const Point &);

    void ActionCurrentUp(void){};
    void ActionCurrentDn(void){};
    void ActionListSingleClick(HeroRow &){};
    void ActionListDoubleClick(HeroRow &){};
    void ActionListPressRight(HeroRow &){};

    void ActionListSingleClick(HeroRow &, const Point &, s32, s32);
    void ActionListDoubleClick(HeroRow &, const Point &, s32, s32);
    void ActionListPressRight(HeroRow &, const Point &, s32, s32);
    bool ActionListCursor(HeroRow &, const Point &, s32, s32);
};

StatsHeroesList::StatsHeroesList(const Point & pt, KingdomHeroes & heroes) : Interface::ListBox<HeroRow>(pt)
{
    const Sprite & back = AGG::GetICN(ICN::OVERVIEW, 13);

    SetTopLeft(pt);
    SetScrollSplitter(AGG::GetICN(ICN::SCROLL, 4),
                        Rect(pt.x + 629, pt.y + 18, back.w(), back.h()));
    SetScrollButtonUp(ICN::SCROLL, 0, 1,
                        Point(pt.x + 626, pt.y));
    SetScrollButtonDn(ICN::SCROLL, 2, 3,
                        Point(pt.x + 626, pt.y + 20 + back.h()));
    SetAreaMaxItems(4);
    SetAreaItems(Rect(pt.x + 30, pt.y + 17, 594, 344));

    content.resize(heroes.size());

    for(KingdomHeroes::iterator
	it = heroes.begin(); it != heroes.end(); ++it)
	content[std::distance(heroes.begin(), it)].Init(*it);

    SetListContent(content);
}

void StatsHeroesList::ActionListDoubleClick(HeroRow & row, const Point & cursor, s32 ox, s32 oy)
{
    ActionListSingleClick(row, cursor, ox, oy);
}

void StatsHeroesList::ActionListSingleClick(HeroRow & row, const Point & cursor, s32 ox, s32 oy)
{
    if(row.hero && (Rect(ox + 5, oy + 4, Interface::IconsBar::GetItemWidth(), Interface::IconsBar::GetItemHeight()) & cursor))
	Game::OpenHeroesDialog(*row.hero);
}

void StatsHeroesList::ActionListPressRight(HeroRow & row, const Point & cursor, s32 ox, s32 oy)
{
    if(row.hero && (Rect(ox + 5, oy + 4, Interface::IconsBar::GetItemWidth(), Interface::IconsBar::GetItemHeight()) & cursor))
	Dialog::QuickInfo(*row.hero);
}

bool StatsHeroesList::ActionListCursor(HeroRow & row, const Point & cursor, s32 ox, s32 oy)
{
    if((row.armyBar->GetArea() & cursor) &&
	row.armyBar->QueueEventProcessing())
    {
	if(row.artifactsBar->isSelected()) row.artifactsBar->ResetSelected();
	Cursor::Get().Hide();
	return true;
    }
    else
    if((row.artifactsBar->GetArea() & cursor) &&
	row.artifactsBar->QueueEventProcessing())
    {
	if(row.armyBar->isSelected()) row.armyBar->ResetSelected();
	Cursor::Get().Hide();
	return true;
    }
    else
    if((row.primskillsBar->GetArea() & cursor) &&
	row.primskillsBar->QueueEventProcessing())
    {
	Cursor::Get().Hide();
	return true;
    }
    else
    if((row.secskillsBar->GetArea() & cursor) &&
	row.secskillsBar->QueueEventProcessing())
    {
	Cursor::Get().Hide();
	return true;
    }

    return false;
}

void StatsHeroesList::RedrawItem(const HeroRow & row, s32 dstx, s32 dsty, bool current)
{
    if(row.hero)
    {
	Text text("", Font::SMALL);
	const Sprite & back = AGG::GetICN(ICN::OVERVIEW, 10);
	back.Blit(dstx, dsty);

	// base info
	Interface::RedrawHeroesIcon(*row.hero, dstx + 5, dsty + 4);

	text.Set(GetString(row.hero->GetAttack()));
	text.Blit(dstx + 90 - text.w(), dsty + 20);

	text.Set(GetString(row.hero->GetDefense()));
	text.Blit(dstx + 125 - text.w(), dsty + 20);

	text.Set(GetString(row.hero->GetPower()));
	text.Blit(dstx + 160 - text.w(), dsty + 20);

	text.Set(GetString(row.hero->GetKnowledge()));
	text.Blit(dstx + 195 - text.w(), dsty + 20);

	// primary skills info
	const_cast<PrimarySkillsBar*>(row.primskillsBar)->SetPos(dstx + 56, dsty - 3);
	const_cast<PrimarySkillsBar*>(row.primskillsBar)->Redraw();

	// secondary skills info
	const_cast<SecondarySkillsBar*>(row.secskillsBar)->SetPos(dstx + 206, dsty + 3);
	const_cast<SecondarySkillsBar*>(row.secskillsBar)->Redraw();

	// artifacts info
	const_cast<ArtifactsBar*>(row.artifactsBar)->SetPos(dstx + 348, dsty + 3);
	const_cast<ArtifactsBar*>(row.artifactsBar)->Redraw();

	// army info
	const_cast<ArmyBar*>(row.armyBar)->SetPos(dstx - 1, dsty + 30);
	const_cast<ArmyBar*>(row.armyBar)->Redraw();
    }
}

void StatsHeroesList::RedrawBackground(const Point & dst)
{
    Text text("", Font::SMALL);

    // header
    AGG::GetICN(ICN::OVERVIEW, 6).Blit(dst.x + 30, dst.y);

    text.Set(_("Hero/Stats"));
    text.Blit(dst.x + 130 - text.w() / 2, dst.y + 1);

    text.Set(_("Skills"));
    text.Blit(dst.x + 300 - text.w() / 2, dst.y + 1);

    text.Set(_("Artifacts"));
    text.Blit(dst.x + 500 - text.w() / 2, dst.y + 1);

    // splitter background
    AGG::GetICN(ICN::OVERVIEW, 13).Blit(dst.x + 628, dst.y + 17);

    // items background
    for(u32 ii = 0; ii < maxItems; ++ii)
    {
	const Sprite & back = AGG::GetICN(ICN::OVERVIEW, 8);
	back.Blit(dst.x + 30, dst.y + 17 + ii * (back.h() + 4));
    }
}

struct CstlRow
{
    Castle* 		castle;
    ArmyBar*		armyBarGuard;
    ArmyBar*		armyBarGuest;
    DwellingsBar*	dwellingsBar;

    CstlRow() : castle(NULL), armyBarGuard(NULL), armyBarGuest(NULL), dwellingsBar(NULL) {}
    ~CstlRow()
    {
	Clear();
    }

    void Clear(void)
    {
	if(armyBarGuard) delete armyBarGuard;
	if(armyBarGuest) delete armyBarGuest;
	if(dwellingsBar) delete dwellingsBar;
    }

    void Init(Castle* ptr)
    {
	castle = ptr;

	Clear();
	const RGBA fill(40, 12, 0);

	armyBarGuard = new ArmyBar(& castle->GetArmy(), true, false);
	armyBarGuard->SetBackground(Size(41, 41), fill);
	armyBarGuard->SetColRows(5, 1);
	armyBarGuard->SetHSpace(-1);

	CastleHeroes heroes = world.GetHeroes(*castle);

        if(heroes.Guest())
	{
	    armyBarGuest = new ArmyBar(& heroes.Guest()->GetArmy(), true, false);
	    armyBarGuest->SetBackground(Size(41, 41), fill);
	    armyBarGuest->SetColRows(5, 1);
	    armyBarGuest->SetHSpace(-1);
	}

	dwellingsBar = new DwellingsBar(*castle, Size(39, 52), fill);
	dwellingsBar->SetColRows(6, 1);
	dwellingsBar->SetHSpace(2);
    }
};

class StatsCastlesList : public Interface::ListBox<CstlRow>
{
    std::vector<CstlRow> content;

public:
    StatsCastlesList(const Point & pt, KingdomCastles &);

    void RedrawItem(const CstlRow &, s32, s32, bool);
    void RedrawBackground(const Point &);

    void ActionCurrentUp(void){};
    void ActionCurrentDn(void){};
    void ActionListDoubleClick(CstlRow &){};
    void ActionListSingleClick(CstlRow &){};
    void ActionListPressRight(CstlRow &){};

    void ActionListSingleClick(CstlRow &, const Point &, s32, s32);
    void ActionListDoubleClick(CstlRow &, const Point &, s32, s32);
    void ActionListPressRight(CstlRow &, const Point &, s32, s32);
    bool ActionListCursor(CstlRow &, const Point &, s32, s32);
};

StatsCastlesList::StatsCastlesList(const Point & pt, KingdomCastles & castles) : Interface::ListBox<CstlRow>(pt)
{
    const Sprite & back = AGG::GetICN(ICN::OVERVIEW, 13);

    SetTopLeft(pt);
    SetScrollSplitter(AGG::GetICN(ICN::SCROLL, 4),
                        Rect(pt.x + 629, pt.y + 18, back.w(), back.h()));
    SetScrollButtonUp(ICN::SCROLL, 0, 1,
                        Point(pt.x + 626, pt.y));
    SetScrollButtonDn(ICN::SCROLL, 2, 3,
                        Point(pt.x + 626, pt.y + 20 + back.h()));
    SetAreaMaxItems(4);
    SetAreaItems(Rect(pt.x + 30, pt.y + 17, 594, 344));

    content.resize(castles.size());

    for(KingdomCastles::iterator
	it = castles.begin(); it != castles.end(); ++it)
	content[std::distance(castles.begin(), it)].Init(*it);

    SetListContent(content);
}

void StatsCastlesList::ActionListDoubleClick(CstlRow & row, const Point & cursor, s32 ox, s32 oy)
{
    ActionListSingleClick(row, cursor, ox, oy);
}

void StatsCastlesList::ActionListSingleClick(CstlRow & row, const Point & cursor, s32 ox, s32 oy)
{
    if(row.castle)
    {
	// click castle icon
	if(Rect(ox + 17, oy + 19, Interface::IconsBar::GetItemWidth(), Interface::IconsBar::GetItemHeight()) & cursor)
	{
	    Game::OpenCastleDialog(*row.castle);
	    row.Init(row.castle);
	}
	else
	// click hero icon
	if(Rect(ox + 82, oy + 19, Interface::IconsBar::GetItemWidth(), Interface::IconsBar::GetItemHeight()) & cursor)
	{
	    Heroes* hero = row.castle->GetHeroes().GuardFirst();
	    if(hero)
	    {
		Game::OpenHeroesDialog(*hero);
		row.Init(row.castle);
	    }
	}
    }
}

void StatsCastlesList::ActionListPressRight(CstlRow & row, const Point & cursor, s32 ox, s32 oy)
{
    if(row.castle)
    {
	if((Rect(ox + 17, oy + 19, Interface::IconsBar::GetItemWidth(), Interface::IconsBar::GetItemHeight()) & cursor))
	    Dialog::QuickInfo(*row.castle);
	else
	if(Rect(ox + 82, oy + 19, Interface::IconsBar::GetItemWidth(), Interface::IconsBar::GetItemHeight()) & cursor)
	{
	    Heroes* hero = row.castle->GetHeroes().GuardFirst();
	    if(hero) Dialog::QuickInfo(*hero);
	}
    }
}

bool StatsCastlesList::ActionListCursor(CstlRow & row, const Point & cursor, s32 ox, s32 oy)
{
    if(row.armyBarGuard && (row.armyBarGuard->GetArea() & cursor) &&
	(row.armyBarGuest ? row.armyBarGuard->QueueEventProcessing(*row.armyBarGuest) : row.armyBarGuard->QueueEventProcessing()))
    {
	Cursor::Get().Hide();
	if(row.armyBarGuest && row.armyBarGuest->isSelected()) row.armyBarGuest->ResetSelected();
	return true;
    }
    else
    if(row.armyBarGuest && (row.armyBarGuest->GetArea() & cursor) &&
	(row.armyBarGuard ? row.armyBarGuest->QueueEventProcessing(*row.armyBarGuard) : row.armyBarGuest->QueueEventProcessing()))
    {
	Cursor::Get().Hide();
	if(row.armyBarGuard && row.armyBarGuard->isSelected()) row.armyBarGuard->ResetSelected();
	return true;
    }
    else
    if(row.dwellingsBar && (row.dwellingsBar->GetArea() & cursor) &&
	row.dwellingsBar->QueueEventProcessing())
    {
	Cursor::Get().Hide();
	if(row.armyBarGuest && row.armyBarGuest->isSelected()) row.armyBarGuest->ResetSelected();
	if(row.armyBarGuard && row.armyBarGuard->isSelected()) row.armyBarGuard->ResetSelected();
	return true;
    }

    return false;
}

void StatsCastlesList::RedrawItem(const CstlRow & row, s32 dstx, s32 dsty, bool current)
{
    if(row.castle)
    {
	Text text("", Font::SMALL);
	const Sprite & back = AGG::GetICN(ICN::OVERVIEW, 11);
	back.Blit(dstx, dsty);

	// base info
	Interface::RedrawCastleIcon(*row.castle, dstx + 17, dsty + 19);

	const Heroes* hero = row.castle->GetHeroes().GuardFirst();

	if(hero)
	{
	    Interface::RedrawHeroesIcon(*hero, dstx + 82, dsty + 19);
	    text.Set(hero->StringSkills("-"));
	    text.Blit(dstx + 104 - text.w() / 2, dsty + 43);
	}

	text.Set(row.castle->GetName());
	text.Blit(dstx + 72 - text.w() / 2, dsty + 62);

	// army info
	if(row.armyBarGuard)
	{
	    const_cast<ArmyBar*>(row.armyBarGuard)->SetPos(dstx + 146, row.armyBarGuest ? dsty : dsty + 20);
	    const_cast<ArmyBar*>(row.armyBarGuard)->Redraw();
	}

	if(row.armyBarGuest)
	{
	    const_cast<ArmyBar*>(row.armyBarGuest)->SetPos(dstx + 146, row.armyBarGuard ? dsty + 41 : dsty + 20);
	    const_cast<ArmyBar*>(row.armyBarGuest)->Redraw();
	}

	const_cast<DwellingsBar*>(row.dwellingsBar)->SetPos(dstx + 349, dsty + 15);
	const_cast<DwellingsBar*>(row.dwellingsBar)->Redraw();
    }
}

void StatsCastlesList::RedrawBackground(const Point & dst)
{
    Text text("", Font::SMALL);

    // header
    AGG::GetICN(ICN::OVERVIEW, 7).Blit(dst.x + 30, dst.y);

    text.Set(_("Town/Castle"));
    text.Blit(dst.x + 105 - text.w() / 2, dst.y + 1);

    text.Set(_("Garrison"));
    text.Blit(dst.x + 275 - text.w() / 2, dst.y + 1);

    text.Set(_("Available"));
    text.Blit(dst.x + 500 - text.w() / 2, dst.y + 1);

    // splitter background
    AGG::GetICN(ICN::OVERVIEW, 13).Blit(dst.x + 628, dst.y + 17);

    // items background
    for(u32 ii = 0; ii < maxItems; ++ii)
    {
	const Sprite & back = AGG::GetICN(ICN::OVERVIEW, 8);
	back.Blit(dst.x + 30, dst.y + 17 + ii * (back.h() + 4));
	// fix bar
	AGG::GetICN(ICN::OVERBACK, 0).Blit(Rect(28, 12, 599, 6), dst.x + 28, dst.y + 12 + ii * (back.h() + 4));
    }
}

std::string CapturedExtInfoString(int res, int color, const Funds & funds)
{
    std::ostringstream os;
    os << world.CountCapturedMines(res, color);
    const s32 vals = funds.Get(res);

    if(vals)
    {
	os << " " << "(";
	if(vals > 0) os << "+";
	os << vals << ")";
    }

    return os.str();
}

void RedrawIncomeInfo(const Point & pt, const Kingdom & myKingdom)
{
    const Funds income = myKingdom.GetIncome(INCOME_ARTIFACTS | INCOME_HEROSKILLS);
    Text text("", Font::SMALL);

    text.Set(CapturedExtInfoString(Resource::WOOD, myKingdom.GetColor(), income));
    text.Blit(pt.x + 54 - text.w() / 2, pt.y + 408);

    text.Set(CapturedExtInfoString(Resource::MERCURY, myKingdom.GetColor(), income));
    text.Blit(pt.x + 146 - text.w() / 2, pt.y + 408);

    text.Set(CapturedExtInfoString(Resource::ORE, myKingdom.GetColor(), income));
    text.Blit(pt.x + 228 - text.w() / 2, pt.y + 408);

    text.Set(CapturedExtInfoString(Resource::SULFUR, myKingdom.GetColor(), income));
    text.Blit(pt.x + 294 - text.w() / 2, pt.y + 408);

    text.Set(CapturedExtInfoString(Resource::CRYSTAL, myKingdom.GetColor(), income));
    text.Blit(pt.x + 360 - text.w() / 2, pt.y + 408);

    text.Set(CapturedExtInfoString(Resource::GEMS, myKingdom.GetColor(), income));
    text.Blit(pt.x + 428 - text.w() / 2, pt.y + 408);

    text.Set(CapturedExtInfoString(Resource::GOLD, myKingdom.GetColor(), income));
    text.Blit(pt.x + 494 - text.w() / 2, pt.y + 408);
}

void RedrawFundsInfo(const Point & pt, const Kingdom & myKingdom)
{
    const Funds & funds = myKingdom.GetFunds();
    Text text("", Font::SMALL);

    AGG::GetICN(ICN::OVERBACK, 0).Blit(Rect(4, 422, 530, 56), pt.x + 4, pt.y + 422);

    text.Set(GetString(funds.wood));
    text.Blit(pt.x + 56 - text.w() / 2, pt.y + 448);

    text.Set(GetString(funds.mercury));
    text.Blit(pt.x + 146 - text.w() / 2, pt.y + 448);

    text.Set(GetString(funds.ore));
    text.Blit(pt.x + 226 - text.w() / 2, pt.y + 448);

    text.Set(GetString(funds.sulfur));
    text.Blit(pt.x + 294 - text.w() / 2, pt.y + 448);

    text.Set(GetString(funds.crystal));
    text.Blit(pt.x + 362 - text.w() / 2, pt.y + 448);

    text.Set(GetString(funds.gems));
    text.Blit(pt.x + 428 - text.w() / 2, pt.y + 448);

    text.Set(GetString(funds.gold));
    text.Blit(pt.x + 496 - text.w() / 2, pt.y + 448);

    text.Set(_("Gold Per Day:") + std::string(" ") + GetString(myKingdom.GetIncome().Get(Resource::GOLD)));
    text.Blit(pt.x + 180, pt.y + 462);

    std::string msg = _("Day: %{day}");
    StringReplace(msg, "%{day}", world.GetDay());
    text.Set(msg);
    text.Blit(pt.x + 360, pt.y + 462);
}

void Kingdom::OverviewDialog(void)
{
    Display & display = Display::Get();
    Cursor & cursor = Cursor::Get();
    cursor.Hide();
    cursor.SetThemes(cursor.POINTER);

    Dialog::FrameBorder background(Size(640, 480));

    const Point & cur_pt = background.GetArea();
    Point dst_pt(cur_pt);

    AGG::GetICN(ICN::OVERBACK, 0).Blit(dst_pt);

    RedrawIncomeInfo(cur_pt, *this);
    RedrawFundsInfo(cur_pt, *this);

    StatsHeroesList listHeroes(dst_pt, heroes);
    StatsCastlesList listCastles(dst_pt, castles);

    // buttons
    dst_pt.x = cur_pt.x + 540;
    dst_pt.y = cur_pt.y + 360;
    Button buttonHeroes(dst_pt.x, dst_pt.y, ICN::OVERVIEW, 0, 1);

    dst_pt.x = cur_pt.x + 540;
    dst_pt.y = cur_pt.y + 405;
    Button buttonCastle(dst_pt.x, dst_pt.y, ICN::OVERVIEW, 2, 3);

    dst_pt.x = cur_pt.x + 540;
    dst_pt.y = cur_pt.y + 453;
    Button buttonExit(dst_pt.x, dst_pt.y, ICN::OVERVIEW, 4, 5);

    const Rect rectIncome(cur_pt.x + 1, cur_pt.y + 360, 535, 60);

    Interface::ListBasic* listStats = NULL;

    // set state view: castles
    if(Modes(OVERVIEWCSTL))
    {
	buttonCastle.Press();
	buttonHeroes.Release();
	listStats = &listCastles;
    }
    else
    // set state view: heroes
    {
	buttonHeroes.Press();
	buttonCastle.Release();
	listStats = &listHeroes;
    }

    listStats->Redraw();

    buttonHeroes.Draw();
    buttonCastle.Draw();
    buttonExit.Draw();

    cursor.Show();
    display.Flip();

    LocalEvent & le = LocalEvent::Get();
    bool redraw = true;

    // dialog menu loop
    while(le.HandleEvents())
    {
	le.MousePressLeft(buttonExit) ? buttonExit.PressDraw() : buttonExit.ReleaseDraw();

	// switch view: heroes/castle
	if(buttonHeroes.isReleased() &&
	    le.MouseClickLeft(buttonHeroes))
	{
	    cursor.Hide();
	    buttonHeroes.Press();
	    buttonCastle.Release();
	    buttonHeroes.Draw();
	    buttonCastle.Draw();
	    listStats = &listHeroes;
	    ResetModes(OVERVIEWCSTL);
	    redraw = true;
	}
	else
	if(buttonCastle.isReleased() &&
	    le.MouseClickLeft(buttonCastle))
	{
	    cursor.Hide();
	    buttonCastle.Press();
	    buttonHeroes.Release();
	    buttonHeroes.Draw();
	    buttonCastle.Draw();
	    listStats = &listCastles;
	    SetModes(OVERVIEWCSTL);
	    redraw = true;
	}

	// exit event
	if(le.MouseClickLeft(buttonExit) ||
	    Game::HotKeyPressEvent(Game::EVENT_DEFAULT_EXIT)) break;

	listStats->QueueEventProcessing();

        if(le.MouseClickLeft(rectIncome))
            Dialog::ResourceInfo("", "income:", GetIncome(INCOME_ALL), Dialog::OK);
        else
        if(le.MousePressRight(rectIncome))
            Dialog::ResourceInfo("", "income:", GetIncome(INCOME_ALL), 0);

	// redraw
	if(! cursor.isVisible() || redraw)
	{
	    listStats->Redraw();
	    RedrawFundsInfo(cur_pt, *this);
	    cursor.Show();
	    display.Flip();
	    redraw = false;
	}
    }
}
