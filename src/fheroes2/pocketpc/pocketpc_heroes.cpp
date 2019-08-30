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
#include "cursor.h"
#include "text.h"
#include "button.h"
#include "dialog.h"
#include "heroes.h"
#include "game.h"
#include "heroes_indicator.h"
#include "army_bar.h"
#include "world.h"
#include "race.h"
#include "kingdom.h"
#include "pocketpc.h"

int PocketPC::HeroesOpenDialog(Heroes & hero, bool readonly)
{
    Cursor & cursor = Cursor::Get();
    Display & display = Display::Get();
    LocalEvent & le = LocalEvent::Get();

    cursor.Hide();
    cursor.SetThemes(cursor.POINTER);

    Dialog::FrameBorder frameborder(Size(320, 224));
    const Rect & dst_rt = frameborder.GetArea();
    const Sprite & backSprite = AGG::GetICN(ICN::SWAPWIN, 0);

    // portrait
    AGG::GetICN(ICN::BRCREST, 6).Blit(dst_rt.x + 8, dst_rt.y, display);
    hero.PortraitRedraw(dst_rt.x + 12, dst_rt.y + 4, PORT_MEDIUM, display);

    // name
    std::string message = _("%{name} the %{race} ( Level %{level} )");
    StringReplace(message, "%{name}", hero.GetName());
    StringReplace(message, "%{race}", Race::String(hero.GetRace()));
    StringReplace(message, "%{level}", hero.GetLevel());
    Text text(message, Font::SMALL);
    text.Blit(dst_rt.x + 73, dst_rt.y + 1);

    // experience
    ExperienceIndicator experienceInfo(hero);
    experienceInfo.SetPos(Point(dst_rt.x + 205, dst_rt.y + 14));
    experienceInfo.Redraw();

    // spell points
    SpellPointsIndicator spellPointsInfo(hero);
    spellPointsInfo.SetPos(Point(dst_rt.x + 238, dst_rt.y + 16));
    spellPointsInfo.Redraw();

    // morale
    MoraleIndicator moraleIndicator(hero);
    moraleIndicator.SetPos(Point(dst_rt.x + 280, dst_rt.y + 20));
    moraleIndicator.Redraw();

    // luck
    LuckIndicator luckIndicator(hero);
    luckIndicator.SetPos(Point(dst_rt.x + 280, dst_rt.y + 60));
    luckIndicator.Redraw();

    // prim skill
    PrimarySkillsBar primskill_bar(&hero, true);
    primskill_bar.SetColRows(4, 1);
    primskill_bar.SetHSpace(-1);
    primskill_bar.SetTextOff(0, -1);
    primskill_bar.SetPos(dst_rt.x + 74, dst_rt.y + 14);
    primskill_bar.Redraw();

    // sec skill
    backSprite.Blit(Rect(21, 198, 267, 36), dst_rt.x + 7, dst_rt.y + 57);
    // secondary skill
    SecondarySkillsBar secskill_bar;
    secskill_bar.SetColRows(8, 1);
    secskill_bar.SetHSpace(-1);
    secskill_bar.SetContent(hero.GetSecondarySkills().ToVector());
    secskill_bar.SetPos(dst_rt.x + 8, dst_rt.y + 58);
    secskill_bar.Redraw();

    // army bar
    ArmyBar selectArmy(&hero.GetArmy(), true, readonly);
    selectArmy.SetColRows(5, 1);
    selectArmy.SetPos(dst_rt.x + 51, dst_rt.y + 170);
    selectArmy.SetHSpace(-1);
    selectArmy.Redraw();
            
    // art bar
    ArtifactsBar selectArtifacts(&hero, true, readonly);
    selectArtifacts.SetColRows(7, 2);
    selectArtifacts.SetHSpace(2);
    selectArtifacts.SetVSpace(2);
    selectArtifacts.SetContent(hero.GetBagArtifacts());
    selectArtifacts.SetPos(dst_rt.x + 37, dst_rt.y + 95);
    selectArtifacts.Redraw();

    Button buttonDismiss(dst_rt.x + dst_rt.w / 2 - 160, dst_rt.y + dst_rt.h - 125, ICN::HSBTNS, 0, 1);
    Button buttonExit(dst_rt.x + dst_rt.w / 2 + 130, dst_rt.y + dst_rt.h - 125, ICN::HSBTNS, 2, 3);

    Button buttonPrev(dst_rt.x + 34, dst_rt.y + 200, ICN::TRADPOST, 3, 4);
    Button buttonNext(dst_rt.x + 275, dst_rt.y + 200, ICN::TRADPOST, 5, 6);

    if(hero.inCastle() || readonly)
    {
	buttonDismiss.Press();
        buttonDismiss.SetDisable(true);
    }

    if(readonly || 2 > hero.GetKingdom().GetHeroes().size())
    {
	buttonNext.Press();
	buttonPrev.Press();
        buttonNext.SetDisable(true);
        buttonPrev.SetDisable(true);
    }

    buttonDismiss.Draw();
    buttonExit.Draw();
    buttonNext.Draw();
    buttonPrev.Draw();

    cursor.Show();
    display.Flip();

    while(le.HandleEvents())
    {
        le.MousePressLeft(buttonNext) ? buttonNext.PressDraw() : buttonNext.ReleaseDraw();
        le.MousePressLeft(buttonPrev) ? buttonPrev.PressDraw() : buttonPrev.ReleaseDraw();
        le.MousePressLeft(buttonExit) ? buttonExit.PressDraw() : buttonExit.ReleaseDraw();
        if(buttonDismiss.isEnable()) le.MousePressLeft(buttonDismiss) ? buttonDismiss.PressDraw() : buttonDismiss.ReleaseDraw();

        if(buttonNext.isEnable() && le.MouseClickLeft(buttonNext)) return Dialog::NEXT;
        else
        if(buttonPrev.isEnable() && le.MouseClickLeft(buttonPrev)) return Dialog::PREV;
	else
        // exit
        if(le.MouseClickLeft(buttonExit) ||
		Game::HotKeyPressEvent(Game::EVENT_DEFAULT_EXIT)) return Dialog::CANCEL;
	else
        // dismiss
	if(buttonDismiss.isEnable() && le.MouseClickLeft(buttonDismiss) &&
	    Dialog::YES == Dialog::Message(hero.GetName(), _("Are you sure you want to dismiss this Hero?"), Font::BIG, Dialog::YES | Dialog::NO))
        { return Dialog::DISMISS; }

	// skills click
	if(le.MouseCursor(primskill_bar.GetArea()) && primskill_bar.QueueEventProcessing())
	{
            cursor.Show();
    	    display.Flip();
        }
	else
	if(le.MouseCursor(secskill_bar.GetArea()) && secskill_bar.QueueEventProcessing())
	{
            cursor.Show();
    	    display.Flip();
        }

        // selector troops event
        if(le.MouseCursor(selectArmy.GetArea()) && selectArmy.QueueEventProcessing())
	{
	    if(selectArtifacts.isSelected()) selectArtifacts.ResetSelected();
    	    moraleIndicator.Redraw();
            luckIndicator.Redraw();
	    selectArmy.Redraw();
            cursor.Show();
    	    display.Flip();
        }

        // selector artifacts event
        if(le.MouseCursor(selectArtifacts.GetArea()) && selectArtifacts.QueueEventProcessing())
	{
    	    if(selectArmy.isSelected()) selectArmy.ResetSelected();
	    selectArtifacts.Redraw();
    	    cursor.Show();
    	    display.Flip();
        }

        if(le.MouseCursor(moraleIndicator.GetArea())) MoraleIndicator::QueueEventProcessing(moraleIndicator);
        else
        if(le.MouseCursor(luckIndicator.GetArea())) LuckIndicator::QueueEventProcessing(luckIndicator);
        else
	if(le.MouseCursor(experienceInfo.GetArea())) experienceInfo.QueueEventProcessing();
        else
	if(le.MouseCursor(spellPointsInfo.GetArea())) spellPointsInfo.QueueEventProcessing();
    }

    return Dialog::ZERO;
}
