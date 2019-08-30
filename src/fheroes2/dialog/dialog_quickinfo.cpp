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
#include "maps.h"
#include "text.h"
#include "army.h"
#include "heroes.h"
#include "castle.h"
#include "monster.h"
#include "race.h"
#include "ground.h"
#include "interface_gamearea.h"
#include "game_interface.h"
#include "cursor.h"
#include "world.h"
#include "settings.h"
#include "kingdom.h"
#include "profit.h"
#include "spell.h"
#include "game.h"
#include "dialog.h"

std::string GetMinesIncomeString(int type)
{
    const payment_t income = ProfitConditions::FromMine(type);
    const s32 value = income.Get(type);
    std::string res;

    if(value)
    {
	res.append(" ");
        res.append("(");
        res.append(value > 0 ? "+" : "-");
        res.append(GetString(value));
        res.append(")");
    }

    return res;
}

std::string ShowGuardiansInfo(const Maps::Tiles & tile, int scoute)
{
    std::string str;
    const Troop & troop = tile.QuantityTroop();

    if(MP2::OBJ_MINES == tile.GetObject())
    {
        str = Maps::GetMinesName(tile.QuantityResourceCount().first);
	str.append(GetMinesIncomeString(tile.QuantityResourceCount().first));
    }
    else
	str = MP2::StringObject(tile.GetObject());

    if(troop.isValid())
    {
	str.append("\n");
	str.append(_("guarded by %{count} of %{monster}"));

	StringReplace(str, "%{monster}", StringLower(troop.GetMultiName()));
	StringReplace(str, "%{count}", Game::CountScoute(troop.GetCount(), scoute));
    }

    return str;
}

std::string ShowMonsterInfo(const Maps::Tiles & tile, int scoute)
{
    std::string str;
    const Troop & troop = tile.QuantityTroop();

    if(scoute)
    {
        str = "%{count} %{monster}";
	StringReplace(str, "%{count}", Game::CountScoute(troop.GetCount(), scoute));
	StringReplace(str, "%{monster}", StringLower(troop.GetMultiName()));
    }
    else
	str = Army::TroopSizeString(troop);

    return str;
}

std::string ShowArtifactInfo(const Maps::Tiles & tile, bool show)
{
    std::string str = MP2::StringObject(tile.GetObject());

    if(show)
    {
	str.append("\n(");
	str.append(tile.QuantityArtifact().GetName());
	str.append(")");
    }

    return str;
}

std::string ShowResourceInfo(const Maps::Tiles & tile, bool show, int scoute)
{
    std::string str = MP2::StringObject(tile.GetObject());

    if(show)
    {
	const ResourceCount & rc = tile.QuantityResourceCount();

	str.append("\n(");
	str.append(Resource::String(rc.first));

	if(scoute)
	{
	    str.append(": ");
	    str.append(Game::CountScoute(rc.second, scoute));
	}
	str.append(")");
    }

    return str;
}

std::string ShowDwellingInfo(const Maps::Tiles & tile, int scoute)
{
    std::string str = MP2::StringObject(tile.GetObject());

    if(scoute)
    {
	str.append("\n");
	const Troop & troop = tile.QuantityTroop();
	if(troop.isValid())
	{
	    str.append(_("(available: %{count})"));
	    StringReplace(str, "%{count}", Game::CountScoute(troop.GetCount(), scoute));
	}
	else
	    str.append("(empty)");
    }

    return str;
}

std::string ShowShrineInfo(const Maps::Tiles & tile, const Heroes* hero, int scoute)
{
    std::string str = MP2::StringObject(tile.GetObject());
    bool show = false;

    switch(tile.GetObject())
    {
	case MP2::OBJ_SHRINE1: show = scoute >= Skill::Level::BASIC; break;
	case MP2::OBJ_SHRINE2: show = scoute >= Skill::Level::ADVANCED; break;
	case MP2::OBJ_SHRINE3: show = scoute == Skill::Level::EXPERT; break;
	default: break;
    }

    if(show)
    {
	const Spell & spell = tile.QuantitySpell();
	str.append("\n(");
	str.append(spell.GetName());
	str.append(")");
	if(hero && hero->HaveSpell(spell))
	{
	    str.append("\n(");
	    str.append(_("already learned"));
	    str.append(")");
	}
    }

    return str;
}

std::string ShowWitchHutInfo(const Maps::Tiles & tile, const Heroes* hero, bool show)
{
    std::string str = MP2::StringObject(tile.GetObject());

    if(show)
    {
	const Skill::Secondary & skill = tile.QuantitySkill();
	str.append("\n(");
	str.append(Skill::Secondary::String(skill.Skill()));
	str.append(")");

	if(hero)
	{
	    if(hero->HasSecondarySkill(skill.Skill()))
	    {
		str.append("\n(");
		str.append(_("already knows this skill"));
		str.append(")");
	    }
	    else
	    if(hero->HasMaxSecondarySkill())
	    {
		str.append("\n(");
		str.append(_("already has max skills"));
		str.append(")");
	    }
	}
    }

    return str;
}

std::string ShowLocalVisitTileInfo(const Maps::Tiles & tile, const Heroes* hero)
{
    std::string str = MP2::StringObject(tile.GetObject());
    if(hero)
    {
	str.append("\n");
	str.append(hero->isVisited(tile) ? _("(already visited)") : _("(not visited)"));
    }

    return str;
}

std::string ShowLocalVisitObjectInfo(const Maps::Tiles & tile, const Heroes* hero)
{
    std::string str = MP2::StringObject(tile.GetObject());
    if(hero)
    {
	str.append("\n");
	str.append(hero->isVisited(tile.GetObject()) ? _("(already visited)") : _("(not visited)"));
    }

    return str;
}

std::string ShowGlobalVisitInfo(const Maps::Tiles & tile, const Kingdom & kingdom)
{
    std::string str = MP2::StringObject(tile.GetObject());
    str.append("\n");
    str.append(kingdom.isVisited(tile) ? _("(already visited)") : _("(not visited)"));

    return str;
}

std::string ShowGlobalVisitInfo(const Maps::Tiles & tile, const Kingdom & kingdom, bool ext)
{
    std::string str = MP2::StringObject(tile.GetObject());
    if(ext && kingdom.isVisited(tile))
    {
	str.append("\n");
	str.append(_("(already visited)"));
    }

    return str;
}

std::string ShowBarrierTentInfo(const Maps::Tiles & tile, const Kingdom & kingdom)
{
    std::string str = BarrierColor::String(tile.QuantityColor());
    str.append(" ");
    str.append(MP2::StringObject(tile.GetObject()));

    if(MP2::OBJ_TRAVELLERTENT == tile.GetObject() &&
	kingdom.IsVisitTravelersTent(tile.QuantityColor()))
    {
	str.append("\n");
	str.append(_("(already visited)"));
    }

    return str;
}

std::string ShowGroundInfo(const Maps::Tiles & tile, bool show, const Heroes* hero)
{
    std::string str = Maps::Ground::String(tile.GetGround());

    if(show && hero)
    {
	int dir = Direction::Get(hero->GetIndex(), tile.GetIndex());
	if(dir != Direction::UNKNOWN)
	{
	    u32 cost = Maps::Ground::GetPenalty(tile.GetIndex(), Direction::Reflect(dir),
				    hero->GetLevelSkill(Skill::Secondary::PATHFINDING));
	    if(cost)
	    {
		str.append("\n");
		str.append(_("penalty: %{cost}"));
		StringReplace(str, "%{cost}", cost);
	    }
	}
    }

    return str;
}

void Dialog::QuickInfo(const Maps::Tiles & tile)
{
    // check
    switch(tile.GetObject())
    {
	case MP2::OBJN_MINES:
	case MP2::OBJN_ABANDONEDMINE:
	case MP2::OBJN_SAWMILL:
	case MP2::OBJN_ALCHEMYLAB:
	{
	    const Maps::Tiles & left  = world.GetTiles(tile.GetIndex() - 1);
	    const Maps::Tiles & right = world.GetTiles(tile.GetIndex() + 1);
	    const Maps::Tiles* center = NULL;

	    if(MP2::isGroundObject(left.GetObject()))  center = &left;
	    else
	    if(MP2::isGroundObject(right.GetObject())) center = &right;

	    if(center)
	    {
		QuickInfo(*center);
		return;
	    }
	}
	break;

	default: break;
    }

    const Settings & settings = Settings::Get();
    Display & display = Display::Get();
    Cursor & cursor = Cursor::Get();
    cursor.Hide();

    // preload
    const int qwikinfo = ICN::QWIKINFO;

    // image box
    const Sprite &box = AGG::GetICN(qwikinfo, 0);
    const Interface::GameArea & gamearea = Interface::Basic::Get().GetGameArea();
    const Rect ar(BORDERWIDTH, BORDERWIDTH, gamearea.GetArea().w, gamearea.GetArea().h);

    LocalEvent & le = LocalEvent::Get();
    const Point & mp = le.GetMouseCursor();

    Rect pos;
    s32 mx = (mp.x - BORDERWIDTH) / TILEWIDTH;
    mx *= TILEWIDTH;
    s32 my = (mp.y - BORDERWIDTH) / TILEWIDTH;
    my *= TILEWIDTH;

    // top left
    if(mx <= ar.x + ar.w / 2 && my <= ar.y + ar.h / 2)
	pos = Rect(mx + TILEWIDTH, my + TILEWIDTH, box.w(), box.h());
    else
    // top right
    if(mx > ar.x + ar.w / 2 && my <= ar.y + ar.h / 2)
	pos = Rect(mx - box.w(), my + TILEWIDTH, box.w(), box.h());
    else
    // bottom left
    if(mx <= ar.x + ar.w / 2 && my > ar.y + ar.h / 2)
	pos = Rect(mx + TILEWIDTH, my - box.h(), box.w(), box.h());
    else
    // bottom right
	pos = Rect(mx - box.w(), my - box.h(), box.w(), box.h());

    SpriteBack back(pos);
    box.Blit(pos.x, pos.y);

    std::string name_object;

    const Heroes* from_hero = Interface::GetFocusHeroes();
    const Kingdom & kingdom = world.GetKingdom(settings.CurrentColor());
    int scoute = from_hero ? from_hero->CanScouteTile(tile.GetIndex()) : 0;
    const bool show = settings.ExtWorldShowVisitedContent();

    if(tile.isFog(settings.CurrentColor()))
	name_object = _("Unchartered Territory");
    else
    // check guardians mine
    if(MP2::OBJ_ABANDONEDMINE == tile.GetObject() ||
	tile.CaptureObjectIsProtection())
    {
	name_object = ShowGuardiansInfo(tile,
		(settings.CurrentColor() == tile.QuantityColor() ? Skill::Level::EXPERT : scoute));
    }
    else
    switch(tile.GetObject())
    {
        case MP2::OBJ_MONSTER:
	    name_object = ShowMonsterInfo(tile, scoute);
    	    break;

        case MP2::OBJ_EVENT:
        case MP2::OBJ_ZERO:
    	    name_object = ShowGroundInfo(tile, show, from_hero);
    	    break;

	case MP2::OBJ_DERELICTSHIP:
	case MP2::OBJ_SHIPWRECK:
	case MP2::OBJ_GRAVEYARD:
	case MP2::OBJ_DAEMONCAVE:
	case MP2::OBJ_PYRAMID:
	case MP2::OBJ_WAGON:
	case MP2::OBJ_SKELETON:
	case MP2::OBJ_LEANTO:
	    name_object = ShowGlobalVisitInfo(tile, kingdom, show);
	    break;

	case MP2::OBJ_WINDMILL:
	case MP2::OBJ_WATERWHEEL:
	case MP2::OBJ_MAGICGARDEN:
	    name_object = Settings::Get().ExtWorldExtObjectsCaptured() ?
		    MP2::StringObject(tile.GetObject()) : ShowGlobalVisitInfo(tile, kingdom, show);
	    break;

	case MP2::OBJ_CAMPFIRE:
	    name_object = ShowResourceInfo(tile, scoute, scoute);
	    break;

	case MP2::OBJ_RESOURCE:
	    name_object = ShowResourceInfo(tile, show || scoute, scoute);
	    break;

	case MP2::OBJ_ARTIFACT:
	    name_object = ShowArtifactInfo(tile, show || scoute);
	    break;

	case MP2::OBJ_MINES:
	    name_object = Maps::GetMinesName(tile.QuantityResourceCount().first);
	    if(settings.CurrentColor() == tile.QuantityColor())
		name_object.append(GetMinesIncomeString(tile.QuantityResourceCount().first));
	    break;

	case MP2::OBJ_ALCHEMYLAB:
	case MP2::OBJ_SAWMILL:
	    name_object = MP2::StringObject(tile.GetObject());
	    if(settings.CurrentColor() == tile.QuantityColor())
		name_object.append(GetMinesIncomeString(tile.QuantityResourceCount().first));
	    break;

        // join army
        case MP2::OBJ_WATCHTOWER:
        case MP2::OBJ_EXCAVATION:
        case MP2::OBJ_CAVE:
        case MP2::OBJ_TREEHOUSE:
        case MP2::OBJ_ARCHERHOUSE:
        case MP2::OBJ_GOBLINHUT:
        case MP2::OBJ_DWARFCOTT:
        case MP2::OBJ_HALFLINGHOLE:
        case MP2::OBJ_PEASANTHUT:
        case MP2::OBJ_THATCHEDHUT:
        // recruit army
        case MP2::OBJ_RUINS:
        case MP2::OBJ_TREECITY:
        case MP2::OBJ_WAGONCAMP:
        case MP2::OBJ_DESERTTENT:
        // battle and recruit army
        case MP2::OBJ_DRAGONCITY:
        case MP2::OBJ_CITYDEAD:
        case MP2::OBJ_TROLLBRIDGE:
	    name_object = ShowDwellingInfo(tile, (kingdom.isVisited(tile) ? Skill::Level::EXPERT : scoute));
	    break;

	case MP2::OBJ_GAZEBO:
	case MP2::OBJ_FORT:
	case MP2::OBJ_XANADU:
	case MP2::OBJ_MERCENARYCAMP:
	case MP2::OBJ_DOCTORHUT:
	case MP2::OBJ_STANDINGSTONES:
	case MP2::OBJ_ARTESIANSPRING:
	case MP2::OBJ_TREEKNOWLEDGE:
	    name_object = ShowLocalVisitTileInfo(tile, from_hero);
	    break;

	case MP2::OBJ_MAGICWELL:
	case MP2::OBJ_FOUNTAIN:
	case MP2::OBJ_FAERIERING:
	case MP2::OBJ_IDOL:
	case MP2::OBJ_OASIS:
	case MP2::OBJ_TEMPLE:
	case MP2::OBJ_BUOY:
	case MP2::OBJ_MERMAID:
	case MP2::OBJ_WATERINGHOLE:
	case MP2::OBJ_ARENA:
	case MP2::OBJ_STABLES:
	case MP2::OBJ_SIRENS:
	    name_object = ShowLocalVisitObjectInfo(tile, from_hero);
	    break;

	case MP2::OBJ_SHRINE1:
	case MP2::OBJ_SHRINE2:
	case MP2::OBJ_SHRINE3:
	    name_object = ShowShrineInfo(tile, from_hero,
			    (show && kingdom.isVisited(tile) ? Skill::Level::EXPERT : scoute));
	    break;

	case MP2::OBJ_WITCHSHUT:
	    name_object = ShowWitchHutInfo(tile, from_hero,
			    ((show && kingdom.isVisited(tile)) || scoute == Skill::Level::EXPERT));
	    break;

        case MP2::OBJ_OBELISK:
	    name_object = ShowGlobalVisitInfo(tile, kingdom);
            break;

        case MP2::OBJ_BARRIER:
        case MP2::OBJ_TRAVELLERTENT:
	    name_object = ShowBarrierTentInfo(tile, kingdom);
	    break;

        default:
	    name_object = MP2::StringObject(tile.GetObject());
	    break;
    }

    TextBox text(name_object, Font::SMALL, 118);
    text.Blit(pos.x + BORDERWIDTH + (pos.w - BORDERWIDTH - text.w()) / 2, pos.y + (pos.h - BORDERWIDTH - text.h()) / 2);

    cursor.Show();
    display.Flip();

    // quick info loop
    while(le.HandleEvents() && le.MousePressRight());

    // restore background
    cursor.Hide();
    back.Restore();
    cursor.Show();
    display.Flip();
}

void Dialog::QuickInfo(const Castle & castle)
{
    Display & display = Display::Get();

    Cursor & cursor = Cursor::Get();
    cursor.Hide();

    const int qwiktown = ICN::QWIKTOWN;

    // image box
    const Sprite &box = AGG::GetICN(qwiktown, 0);
    const Interface::GameArea & gamearea = Interface::Basic::Get().GetGameArea();
    const Rect ar(BORDERWIDTH, BORDERWIDTH, gamearea.GetArea().w, gamearea.GetArea().h);

    LocalEvent & le = LocalEvent::Get();
    const Point & mp = le.GetMouseCursor();

    Rect cur_rt;
    s32 mx = (mp.x - BORDERWIDTH) / TILEWIDTH;
    mx *= TILEWIDTH;
    s32 my = (mp.y - BORDERWIDTH) / TILEWIDTH;
    my *= TILEWIDTH;

    // top left
    if(mx <= ar.x + ar.w / 2 && my <= ar.y + ar.h / 2)
	cur_rt = Rect(mx + TILEWIDTH, my + TILEWIDTH, box.w(), box.h());
    else
    // top right
    if(mx > ar.x + ar.w / 2 && my <= ar.y + ar.h / 2)
	cur_rt = Rect(mx - box.w(), my + TILEWIDTH, box.w(), box.h());
    else
    // bottom left
    if(mx <= ar.x + ar.w / 2 && my > ar.y + ar.h / 2)
	cur_rt = Rect(mx + TILEWIDTH, my - box.h(), box.w(), box.h());
    else
    // bottom right
	cur_rt = Rect(mx - box.w(), my - box.h(), box.w(), box.h());

    if(Settings::Get().QVGA())
    {
	cur_rt = Rect((display.w() - box.w()) / 2, (display.h() - box.h()) / 2, box.w(), box.h());
    }

    SpriteBack back(cur_rt);
    box.Blit(cur_rt.x, cur_rt.y);

    cur_rt = Rect(back.GetPos().x + 28 , back.GetPos().y + 12, 178, 140);
    Point dst_pt;
    Text text;

    // castle name
    text.Set(castle.GetName(), Font::SMALL);
    dst_pt.x = cur_rt.x + (cur_rt.w - text.w()) / 2;
    dst_pt.y = cur_rt.y + 5;
    text.Blit(dst_pt);

    u32 index = 0;

    switch(castle.GetRace())
    {
	case Race::KNGT: index = (castle.isCastle() ?  9 : 15); break;
	case Race::BARB: index = (castle.isCastle() ? 10 : 16); break;
	case Race::SORC: index = (castle.isCastle() ? 11 : 17); break;
	case Race::WRLK: index = (castle.isCastle() ? 12 : 18); break;
	case Race::WZRD: index = (castle.isCastle() ? 13 : 19); break;
	case Race::NECR: index = (castle.isCastle() ? 14 : 20); break;
	default: DEBUG(DBG_GAME, DBG_WARN, "unknown race"); return;
    }

    // castle icon
    const Sprite & sprite = AGG::GetICN(ICN::LOCATORS, index);

    dst_pt.x = cur_rt.x + (cur_rt.w - sprite.w()) / 2;
    dst_pt.y += 18;
    sprite.Blit(dst_pt);

    // color flags
    switch(castle.GetColor())
    {
	case Color::BLUE:	index = 0; break;
	case Color::GREEN:	index = 2; break;
	case Color::RED:	index = 4; break;
	case Color::YELLOW:	index = 6; break;
	case Color::ORANGE:	index = 8; break;
	case Color::PURPLE:	index = 10; break;
	case Color::NONE:	index = 12; break;
	default: break;
    }

    const Sprite & l_flag = AGG::GetICN(ICN::FLAG32, index);
    dst_pt.x = cur_rt.x + (cur_rt.w - 60) / 2 - l_flag.w();
    l_flag.Blit(dst_pt);

    const Sprite & r_flag = AGG::GetICN(ICN::FLAG32, index + 1);
    dst_pt.x = cur_rt.x + (cur_rt.w + 60) / 2;
    r_flag.Blit(dst_pt);

    // info
    text.Set(_("Defenders:"));
    dst_pt.x = cur_rt.x + (cur_rt.w - text.w()) / 2;
    dst_pt.y += sprite.h() + 5;
    text.Blit(dst_pt);

    //
    u32 count = castle.GetArmy().GetCount();
    const Settings & conf = Settings::Get();

    const Heroes* from_hero = Interface::GetFocusHeroes();
    const Heroes* guardian = castle.GetHeroes().Guard();

    // draw guardian portrait
    if(guardian &&
	// my  colors
	(castle.isFriends(conf.CurrentColor()) ||
	// show guardians (scouting: advanced)
	(from_hero &&
	    Skill::Level::ADVANCED <= from_hero->GetSecondaryValues(Skill::Secondary::SCOUTING))))
    {
	// heroes name
	text.Set(guardian->GetName(), Font::SMALL);
	dst_pt.x = cur_rt.x + (cur_rt.w - text.w()) / 2;
	dst_pt.y += 10;
	text.Blit(dst_pt);

	// mini port heroes
	Surface port = guardian->GetPortrait(PORT_SMALL);
	if(port.isValid())
	{
	    dst_pt.x = cur_rt.x + (cur_rt.w - port.w()) / 2;
	    dst_pt.y += 15;
	    port.Blit(dst_pt, display);
	}
    }

    // draw defenders
    if(! count)
    {
	text.Set(_("None"));
	dst_pt.x = cur_rt.x + (cur_rt.w - text.w()) / 2;
	dst_pt.y += 45;
	text.Blit(dst_pt);
    }
    else
    if(castle.isFriends(conf.CurrentColor()))
	// show all
	Army::DrawMons32Line(castle.GetArmy(), cur_rt.x - 5, cur_rt.y + 100, 192);
    else
	// show limited
	Army::DrawMons32LineWithScoute(castle.GetArmy(), cur_rt.x - 5, cur_rt.y + 100, 192, 0, 0,
				(from_hero && from_hero->CanScouteTile(castle.GetIndex()) ? from_hero->GetSecondaryValues(Skill::Secondary::SCOUTING) : Skill::Level::NONE));

    cursor.Show();
    display.Flip();

    // quick info loop
    while(le.HandleEvents() && le.MousePressRight());

    // restore background
    cursor.Hide();
    back.Restore();
    cursor.Show();
    display.Flip();
}

void Dialog::QuickInfo(const Heroes & hero)
{
    Display & display = Display::Get();
    const Settings & conf = Settings::Get();

    Cursor & cursor = Cursor::Get();
    cursor.Hide();

    const int qwikhero = ICN::QWIKHERO;

    // image box
    const Sprite &box = AGG::GetICN(qwikhero, 0);
    const Interface::GameArea & gamearea = Interface::Basic::Get().GetGameArea();
    const Rect ar(BORDERWIDTH, BORDERWIDTH, gamearea.GetArea().w, gamearea.GetArea().h);

    LocalEvent & le = LocalEvent::Get();
    const Point & mp = le.GetMouseCursor();

    Rect cur_rt;
    s32 mx = (mp.x - BORDERWIDTH) / TILEWIDTH;
    mx *= TILEWIDTH;
    s32 my = (mp.y - BORDERWIDTH) / TILEWIDTH;
    my *= TILEWIDTH;

    // top left
    if(mx <= ar.x + ar.w / 2 && my <= ar.y + ar.h / 2)
	cur_rt = Rect(mx + TILEWIDTH, my + TILEWIDTH, box.w(), box.h());
    else
    // top right
    if(mx > ar.x + ar.w / 2 && my <= ar.y + ar.h / 2)
	cur_rt = Rect(mx - box.w(), my + TILEWIDTH, box.w(), box.h());
    else
    // bottom left
    if(mx <= ar.x + ar.w / 2 && my > ar.y + ar.h / 2)
	cur_rt = Rect(mx + TILEWIDTH, my - box.h(), box.w(), box.h());
    else
    // bottom right
	cur_rt = Rect(mx - box.w(), my - box.h(), box.w(), box.h());

    if(Settings::Get().QVGA())
    {
	cur_rt = Rect((display.w() - box.w()) / 2, (display.h() - box.h()) / 2, box.w(), box.h());
    }

    SpriteBack back(cur_rt);
    box.Blit(cur_rt.x, cur_rt.y);

    cur_rt = Rect(back.GetPos().x + 28 , back.GetPos().y + 10, 146, 144);
    Point dst_pt;
    Text text;
    std::string message;

    // heroes name
    if(hero.isFriends(conf.CurrentColor()))
    {
	message = _("%{name} ( Level %{level} )");
	StringReplace(message, "%{name}", hero.GetName());
	StringReplace(message, "%{level}", hero.GetLevel());
    }
    else
	message = hero.GetName();
    text.Set(message, Font::SMALL);
    dst_pt.x = cur_rt.x + (cur_rt.w - text.w()) / 2;
    dst_pt.y = cur_rt.y;
    text.Blit(dst_pt);

    // mini port heroes
    Surface port = hero.GetPortrait(PORT_SMALL);
    if(port.isValid())
    {
	dst_pt.x = cur_rt.x + (cur_rt.w - port.w()) / 2;
	dst_pt.y = cur_rt.y + 13;
	port.Blit(dst_pt, display);
    }

    // luck
    if(hero.isFriends(conf.CurrentColor()))
    {
	const s32 luck = hero.GetLuckWithModificators(NULL);
	const Sprite & sprite = AGG::GetICN(ICN::MINILKMR, (0 > luck ? 0 : (0 < luck ? 1 : 2)));
	u32 count = (0 == luck ? 1 : std::abs(luck));
	dst_pt.x = cur_rt.x + 120;
	dst_pt.y = cur_rt.y + (count == 1 ? 20 : 13);

	while(count--)
	{
    	    sprite.Blit(dst_pt.x, dst_pt.y);
    	    dst_pt.y += sprite.h() - 1;
	}
    }

    // morale
    if(hero.isFriends(conf.CurrentColor()))
    {
	const s32 morale = hero.GetMoraleWithModificators(NULL);
	const Sprite & sprite = AGG::GetICN(ICN::MINILKMR, (0 > morale ? 3 : (0 < morale ? 4 : 5)));
	u32 count = (0 == morale ? 1 : std::abs(morale));
	dst_pt.x = cur_rt.x + 10;
	dst_pt.y = cur_rt.y + (count == 1 ? 20 : 13);

	while(count--)
	{
    	    sprite.Blit(dst_pt.x, dst_pt.y);
    	    dst_pt.y += sprite.h() - 1;
	}
    }

    // color flags
    u32 index = 0;

    switch(hero.GetColor())
    {
	case Color::BLUE:	index = 0; break;
	case Color::GREEN:	index = 2; break;
	case Color::RED:	index = 4; break;
	case Color::YELLOW:	index = 6; break;
	case Color::ORANGE:	index = 8; break;
	case Color::PURPLE:	index = 10; break;
	case Color::NONE:	index = 12; break;
	default: break;
    }

    dst_pt.y = cur_rt.y + 13;

    const Sprite & l_flag = AGG::GetICN(ICN::FLAG32, index);
    dst_pt.x = cur_rt.x + (cur_rt.w - 40) / 2 - l_flag.w();
    l_flag.Blit(dst_pt);

    const Sprite & r_flag = AGG::GetICN(ICN::FLAG32, index + 1);
    dst_pt.x = cur_rt.x + (cur_rt.w + 40) / 2;
    r_flag.Blit(dst_pt);

    // attack
    text.Set(std::string(_("Attack")) + ":");
    dst_pt.x = cur_rt.x + 10;
    dst_pt.y += port.h();
    text.Blit(dst_pt);

    text.Set(GetString(hero.GetAttack()));
    dst_pt.x += 75;
    text.Blit(dst_pt);

    // defense
    text.Set(std::string(_("Defense")) + ":");
    dst_pt.x = cur_rt.x + 10;
    dst_pt.y += 12;
    text.Blit(dst_pt);

    text.Set(GetString(hero.GetDefense()));
    dst_pt.x += 75;
    text.Blit(dst_pt);

    // power
    text.Set(std::string(_("Spell Power")) + ":");
    dst_pt.x = cur_rt.x + 10;
    dst_pt.y += 12;
    text.Blit(dst_pt);

    text.Set(GetString(hero.GetPower()));
    dst_pt.x += 75;
    text.Blit(dst_pt);

    // knowledge
    text.Set(std::string(_("Knowledge")) + ":");
    dst_pt.x = cur_rt.x + 10;
    dst_pt.y += 12;
    text.Blit(dst_pt);

    text.Set(GetString(hero.GetKnowledge()));
    dst_pt.x += 75;
    text.Blit(dst_pt);

    // spell point
    text.Set(std::string(_("Spell Points")) + ":");
    dst_pt.x = cur_rt.x + 10;
    dst_pt.y += 12;
    text.Blit(dst_pt);

    text.Set(GetString(hero.GetSpellPoints()) + "/" + GetString(hero.GetMaxSpellPoints()));
    dst_pt.x += 75;
    text.Blit(dst_pt);

    // move point
    text.Set(std::string(_("Move Points")) + ":");
    dst_pt.x = cur_rt.x + 10;
    dst_pt.y += 12;
    text.Blit(dst_pt);

    text.Set(GetString(hero.GetMobilityIndexSprite()) + "/" + GetString(hero.GetMovePoints()) + "/" + GetString(hero.GetMaxMovePoints()));
    dst_pt.x += 75;
    text.Blit(dst_pt);

    // draw monster sprite in one string
    const Heroes* from_hero = Interface::GetFocusHeroes();

    if(hero.isFriends(conf.CurrentColor()))
	// show all
	Army::DrawMons32Line(hero.GetArmy(), cur_rt.x - 5, cur_rt.y + 114, 160);
    else
	// show limited
	Army::DrawMons32LineWithScoute(hero.GetArmy(), cur_rt.x - 5, cur_rt.y + 114, 160, 0, 0,
				(from_hero && from_hero->CanScouteTile(hero.GetIndex()) ? from_hero->GetSecondaryValues(Skill::Secondary::SCOUTING) : Skill::Level::NONE));

    cursor.Show();
    display.Flip();

    // quick info loop
    while(le.HandleEvents() && le.MousePressRight());

    // restore background
    cursor.Hide();
    back.Restore();
    cursor.Show();
    display.Flip();
}
