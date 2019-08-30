/****************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
 *                                                                          *
 *   This program is free software; you can redistribute it and/or modify   *
 *   it under the terms of the GNU General Public License as published by   *
 *   the Free Software Foundation; either version 2 of the License, or      *
 *   (at your option) any later version.                                    *
 *                                                                          *
 *   This program is distributed in the hope that it will be useful,        *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU General Public License for more details.                           *
 *                                                                          *
 *   You should have received a copy of the GNU General Public License      *
 *   along with this program; if not, write to the                          *
 *   Free Software Foundation, Inc.,                                        *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.              *
 ****************************************************************************/

#include <string>
#include <algorithm>
#include "agg.h"
#include "button.h"
#include "cursor.h"
#include "settings.h"
#include "text.h"
#include "army.h"
#include "heroes.h"
#include "army_bar.h"
#include "heroes_indicator.h"
#include "pocketpc.h"
#include "game.h"
#include "game_interface.h"

void RedrawPrimarySkillInfo(const Point &, PrimarySkillsBar*, PrimarySkillsBar*);

void Heroes::MeetingDialog(Heroes & heroes2)
{
    if(Settings::Get().QVGA()) return PocketPC::HeroesMeeting(*this, heroes2);

    Display & display = Display::Get();

    // cursor
    Cursor & cursor = Cursor::Get();
    cursor.Hide();
    cursor.SetThemes(cursor.POINTER);

    const Sprite &backSprite = AGG::GetICN(ICN::SWAPWIN, 0);
    const Point cur_pt((display.w() - backSprite.w()) / 2, (display.h() - backSprite.h()) / 2);
    SpriteBack background(Rect(cur_pt, backSprite.w(), backSprite.h()));
    Point dst_pt(cur_pt);
    std::string message;

    Rect src_rt(0, 0, 640, 480);

    // background
    dst_pt.x = cur_pt.x;
    dst_pt.y = cur_pt.y;
    backSprite.Blit(src_rt, dst_pt);

    // header
    message = _("%{name1} meets %{name2}");
    StringReplace(message, "%{name1}", GetName());
    StringReplace(message, "%{name2}", heroes2.GetName());
    Text text(message, Font::BIG);
    text.Blit(cur_pt.x + 320 - text.w() / 2, cur_pt.y + 26);

    // portrait
    dst_pt.x = cur_pt.x + 93;
    dst_pt.y = cur_pt.y + 72;
    PortraitRedraw(dst_pt.x, dst_pt.y, PORT_BIG, display);

    dst_pt.x = cur_pt.x + 445;
    dst_pt.y = cur_pt.y + 72;
    heroes2.PortraitRedraw(dst_pt.x, dst_pt.y, PORT_BIG, display);

    dst_pt.x = cur_pt.x + 34;
    dst_pt.y = cur_pt.y + 75;
    MoraleIndicator moraleIndicator1(*this);
    moraleIndicator1.SetPos(dst_pt);
    moraleIndicator1.Redraw();

    dst_pt.x = cur_pt.x + 34;
    dst_pt.y = cur_pt.y + 115;
    LuckIndicator luckIndicator1(*this);
    luckIndicator1.SetPos(dst_pt);
    luckIndicator1.Redraw();

    dst_pt.x = cur_pt.x + 566;
    dst_pt.y = cur_pt.y + 75;
    MoraleIndicator moraleIndicator2(heroes2);
    moraleIndicator2.SetPos(dst_pt);
    moraleIndicator2.Redraw();

    dst_pt.x = cur_pt.x + 566;
    dst_pt.y = cur_pt.y + 115;
    LuckIndicator luckIndicator2(heroes2);
    luckIndicator2.SetPos(dst_pt);
    luckIndicator2.Redraw();

    // primary skill
    SpriteBack backPrimary(Rect(cur_pt.x + 255, cur_pt.y + 50, 130, 135));

    PrimarySkillsBar primskill_bar1(this, true);
    primskill_bar1.SetColRows(1, 4);
    primskill_bar1.SetVSpace(-1);
    primskill_bar1.SetTextOff(70, -25);
    primskill_bar1.SetPos(cur_pt.x + 216, cur_pt.y + 51);

    PrimarySkillsBar primskill_bar2(&heroes2, true);
    primskill_bar2.SetColRows(1, 4);
    primskill_bar2.SetVSpace(-1);
    primskill_bar2.SetTextOff(-70, -25);
    primskill_bar2.SetPos(cur_pt.x + 389, cur_pt.y + 51);

    RedrawPrimarySkillInfo(cur_pt, &primskill_bar1, &primskill_bar2);

    // secondary skill
    SecondarySkillsBar secskill_bar1;
    secskill_bar1.SetColRows(8, 1);
    secskill_bar1.SetHSpace(-1);
    secskill_bar1.SetContent(secondary_skills.ToVector());
    secskill_bar1.SetPos(cur_pt.x + 22, cur_pt.y + 199);
    secskill_bar1.Redraw();

    SecondarySkillsBar secskill_bar2;
    secskill_bar2.SetColRows(8, 1);
    secskill_bar2.SetHSpace(-1);
    secskill_bar2.SetContent(heroes2.GetSecondarySkills().ToVector());
    secskill_bar2.SetPos(cur_pt.x + 353, cur_pt.y + 199);
    secskill_bar2.Redraw();

    // army
    dst_pt.x = cur_pt.x + 36;
    dst_pt.y = cur_pt.y + 267;

    ArmyBar selectArmy1(&GetArmy(), true, false);
    selectArmy1.SetColRows(5, 1);
    selectArmy1.SetPos(dst_pt.x, dst_pt.y);
    selectArmy1.SetHSpace(2);
    selectArmy1.Redraw();

    dst_pt.x = cur_pt.x + 381;
    dst_pt.y = cur_pt.y + 267;

    ArmyBar selectArmy2(&heroes2.GetArmy(), true, false);
    selectArmy2.SetColRows(5, 1);
    selectArmy2.SetPos(dst_pt.x, dst_pt.y);
    selectArmy2.SetHSpace(2);
    selectArmy2.Redraw();

    // artifact
    dst_pt.x = cur_pt.x + 23;
    dst_pt.y = cur_pt.y + 347;

    ArtifactsBar selectArtifacts1(this, true, false);
    selectArtifacts1.SetColRows(7, 2);
    selectArtifacts1.SetHSpace(2);
    selectArtifacts1.SetVSpace(2);
    selectArtifacts1.SetContent(GetBagArtifacts());
    selectArtifacts1.SetPos(dst_pt.x, dst_pt.y);
    selectArtifacts1.Redraw();

    dst_pt.x = cur_pt.x + 367;
    dst_pt.y = cur_pt.y + 347;

    ArtifactsBar selectArtifacts2(&heroes2, true, false);
    selectArtifacts2.SetColRows(7, 2);
    selectArtifacts2.SetHSpace(2);
    selectArtifacts2.SetVSpace(2);
    selectArtifacts2.SetContent(heroes2.GetBagArtifacts());
    selectArtifacts2.SetPos(dst_pt.x, dst_pt.y);
    selectArtifacts2.Redraw();

    // button exit
    dst_pt.x = cur_pt.x + 280;
    dst_pt.y = cur_pt.y + 428;
    Button buttonExit(dst_pt.x, dst_pt.y, ICN::SWAPBTN, 0, 1);

    buttonExit.Draw();

    cursor.Show();
    display.Flip();

    MovePointsScaleFixed();
    heroes2.MovePointsScaleFixed();

    // scholar action
    if(Settings::Get().ExtWorldEyeEagleAsScholar())
	Heroes::ScholarAction(*this, heroes2);

    LocalEvent & le = LocalEvent::Get();

    // message loop
    while(le.HandleEvents())
    {
        le.MousePressLeft(buttonExit) ? buttonExit.PressDraw() : buttonExit.ReleaseDraw();
        if(le.MouseClickLeft(buttonExit) || HotKeyCloseWindow) break;

	// selector troops event
	if((le.MouseCursor(selectArmy1.GetArea()) &&
    	    selectArmy1.QueueEventProcessing(selectArmy2)) ||
	   (le.MouseCursor(selectArmy2.GetArea()) &&
    	    selectArmy2.QueueEventProcessing(selectArmy1)))
	{
	    cursor.Hide();

	    if(selectArtifacts1.isSelected()) selectArtifacts1.ResetSelected();
	    else
	    if(selectArtifacts2.isSelected()) selectArtifacts2.ResetSelected();

	    selectArmy1.Redraw();
	    selectArmy2.Redraw();

	    moraleIndicator1.Redraw();
	    moraleIndicator2.Redraw();
	    luckIndicator1.Redraw();
	    luckIndicator2.Redraw();
	    cursor.Show();
	    display.Flip();
	}

	// selector artifacts event
	if((le.MouseCursor(selectArtifacts1.GetArea()) &&
    	    selectArtifacts1.QueueEventProcessing(selectArtifacts2)) ||
	   (le.MouseCursor(selectArtifacts2.GetArea()) &&
    	    selectArtifacts2.QueueEventProcessing(selectArtifacts1)))
	{
	    cursor.Hide();

	    if(selectArmy1.isSelected()) selectArmy1.ResetSelected();
	    else
	    if(selectArmy2.isSelected()) selectArmy2.ResetSelected();

	    selectArtifacts1.Redraw();
	    selectArtifacts2.Redraw();

	    backPrimary.Restore();
	    RedrawPrimarySkillInfo(cur_pt, &primskill_bar1, &primskill_bar2);
	    moraleIndicator1.Redraw();
	    moraleIndicator2.Redraw();
	    luckIndicator1.Redraw();
	    luckIndicator2.Redraw();
	    cursor.Show();
	    display.Flip();
	}

        if((le.MouseCursor(primskill_bar1.GetArea()) && primskill_bar1.QueueEventProcessing()) ||
           (le.MouseCursor(primskill_bar2.GetArea()) && primskill_bar2.QueueEventProcessing()) ||
           (le.MouseCursor(secskill_bar1.GetArea()) && secskill_bar1.QueueEventProcessing()) ||
           (le.MouseCursor(secskill_bar2.GetArea()) && secskill_bar2.QueueEventProcessing()))
	{
	    cursor.Show();
	    display.Flip();
	}

        if(le.MouseCursor(moraleIndicator1.GetArea())) MoraleIndicator::QueueEventProcessing(moraleIndicator1);
        else
        if(le.MouseCursor(moraleIndicator2.GetArea())) MoraleIndicator::QueueEventProcessing(moraleIndicator2);
	else
        if(le.MouseCursor(luckIndicator1.GetArea())) LuckIndicator::QueueEventProcessing(luckIndicator1);
        else
        if(le.MouseCursor(luckIndicator2.GetArea())) LuckIndicator::QueueEventProcessing(luckIndicator2);
    }

    if(Settings::Get().ExtHeroRecalculateMovement())
    {
	RecalculateMovePoints();
	heroes2.RecalculateMovePoints();
    }

    cursor.Hide();
    background.Restore();
    cursor.Show();
    display.Flip();
}

void RedrawPrimarySkillInfo(const Point & cur_pt, PrimarySkillsBar* bar1, PrimarySkillsBar* bar2)
{
    // attack skill
    Text text(_("Attack Skill"), Font::SMALL);
    text.Blit(cur_pt.x + 320 - text.w() / 2, cur_pt.y + 64);

    // defense skill
    text.Set(_("Defense Skill"));
    text.Blit(cur_pt.x + 320 - text.w() / 2, cur_pt.y + 96);

    // spell power
    text.Set(_("Spell Power"));
    text.Blit(cur_pt.x + 320 - text.w() / 2, cur_pt.y + 128);

    // knowledge
    text.Set(_("Knowledge"));
    text.Blit(cur_pt.x + 320 - text.w() / 2, cur_pt.y + 160);

    if(bar1) bar1->Redraw();
    if(bar2) bar2->Redraw();
}

// spell_book.cpp
struct HeroesCanTeachSpell : std::binary_function<const HeroBase*, Spell, bool>
{
    bool operator() (const HeroBase* hero, Spell spell) const { return hero->CanTeachSpell(spell); };
};

struct HeroesHaveSpell : std::binary_function<const HeroBase*, Spell, bool>
{
    bool operator() (const HeroBase* hero, Spell spell) const { return hero->HaveSpell(spell); };
};

void Heroes::ScholarAction(Heroes & hero1, Heroes & hero2)
{
    if(! hero1.HaveSpellBook() || ! hero2.HaveSpellBook())
    {
	DEBUG(DBG_GAME, DBG_INFO, "spell_book disabled");
	return;
    }
    else
    if(! Settings::Get().ExtWorldEyeEagleAsScholar())
    {
	DEBUG(DBG_GAME, DBG_WARN, "EyeEagleAsScholar settings disabled");
	return;
    }

    const int scholar1 = hero1.GetLevelSkill(Skill::Secondary::EAGLEEYE);
    const int scholar2 = hero2.GetLevelSkill(Skill::Secondary::EAGLEEYE);
    int scholar = 0;

    Heroes* teacher = NULL;
    Heroes* learner = NULL;

    if(scholar1 && scholar1 >= scholar2)
    {
	teacher = &hero1;
	learner = &hero2;
	scholar = scholar1;
    }
    else
    if(scholar2 && scholar2 >= scholar1)
    {
	teacher = &hero2;
	learner = &hero1;
	scholar = scholar2;
    }
    else
    {
	DEBUG(DBG_GAME, DBG_WARN, "Eagle Eye skill not found");
	return;
    }

    // skip bag artifacts
    SpellStorage teach = teacher->spell_book.SetFilter(SpellBook::ALL);
    SpellStorage learn = learner->spell_book.SetFilter(SpellBook::ALL);

    // remove_if for learn spells
    if(learn.size())
    {
	SpellStorage::iterator
	    res = std::remove_if(learn.begin(), learn.end(), std::bind1st(HeroesHaveSpell(), teacher));
	learn.resize(std::distance(learn.begin(), res));
    }

    if(learn.size())
    {
	SpellStorage::iterator
	    res = std::remove_if(learn.begin(), learn.end(), std::not1(std::bind1st(HeroesCanTeachSpell(), teacher)));
	learn.resize(std::distance(learn.begin(), res));
    }

    // remove_if for teach spells
    if(teach.size())
    {
	SpellStorage::iterator
	    res = std::remove_if(teach.begin(), teach.end(), std::bind1st(HeroesHaveSpell(), learner));
	teach.resize(std::distance(teach.begin(), res));
    }

    if(teach.size())
    {
	SpellStorage::iterator
	    res = std::remove_if(teach.begin(), teach.end(), std::not1(std::bind1st(HeroesCanTeachSpell(), teacher)));
	teach.resize(std::distance(teach.begin(), res));
    }

    std::string message, spells1, spells2;

    // learning
    for(SpellStorage::const_iterator
	it = learn.begin(); it != learn.end(); ++it)
    {
	teacher->AppendSpellToBook(*it);
	if(spells1.size())
	    spells1.append(it + 1 == learn.end() ? _(" and ") : ", ");
	spells1.append((*it).GetName());
    }

    // teacher
    for(SpellStorage::const_iterator
	it = teach.begin(); it != teach.end(); ++it)
    {
	learner->AppendSpellToBook(*it);
	if(spells2.size())
	    spells2.append(it + 1 == teach.end() ? _(" and ") : ", ");
	spells2.append((*it).GetName());
    }


    if(teacher->isControlHuman() || learner->isControlHuman())
    {
	if(spells1.size() && spells2.size())
	    message = _("%{teacher}, whose %{level} %{scholar} knows many magical secrets, learns %{spells1} from %{learner}, and teaches %{spells2} to %{learner}.");
	else
	if(spells1.size())
	    message = _("%{teacher}, whose %{level} %{scholar} knows many magical secrets, learns %{spells1} from %{learner}.");
	else
	if(spells2.size())
	    message = _("%{teacher}, whose %{level} %{scholar} knows many magical secrets, teaches %{spells2} to %{learner}.");

	if(message.size())
	{
	    StringReplace(message, "%{teacher}", teacher->GetName());
	    StringReplace(message, "%{learner}", learner->GetName());
	    StringReplace(message, "%{level}", Skill::Level::String(scholar));
	    StringReplace(message, "%{scholar}", Skill::Secondary::String(Skill::Secondary::EAGLEEYE));
	    StringReplace(message, "%{spells1}", spells1);
	    StringReplace(message, "%{spells2}", spells2);

	    Dialog::Message(_("Scholar Ability"), message, Font::BIG, Dialog::OK);
	}
    }
}
