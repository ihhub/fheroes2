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

#include "world.h"
#include "agg.h"
#include "cursor.h"
#include "settings.h"
#include "race.h"
#include "ground.h"
#include "game.h"
#include "game_interface.h"
#include "kingdom.h"
#include "maps_tiles.h"
#include "castle.h"
#include "direction.h"
#include "heroes.h"

bool ReflectSprite(int from);
void PlayWalkSound(int ground);
bool isNeedStayFrontObject(const Heroes & hero, const Maps::Tiles & next);

void PlayWalkSound(int ground)
{
    int wav = M82::UNKNOWN;
    const int speed = (4 > Settings::Get().HeroesMoveSpeed() ? 1 : (7 > Settings::Get().HeroesMoveSpeed() ? 2 : 3));

    // play sound
    switch(ground)
    {
    	case Maps::Ground::WATER:       wav = (1 == speed ? M82::WSND00 : (2 == speed ? M82::WSND10 : M82::WSND20)); break;
    	case Maps::Ground::GRASS:       wav = (1 == speed ? M82::WSND01 : (2 == speed ? M82::WSND11 : M82::WSND21)); break;
    	case Maps::Ground::WASTELAND:   wav = (1 == speed ? M82::WSND02 : (2 == speed ? M82::WSND12 : M82::WSND22)); break;
    	case Maps::Ground::SWAMP:
    	case Maps::Ground::BEACH:       wav = (1 == speed ? M82::WSND03 : (2 == speed ? M82::WSND13 : M82::WSND23)); break;
    	case Maps::Ground::LAVA:        wav = (1 == speed ? M82::WSND04 : (2 == speed ? M82::WSND14 : M82::WSND24)); break;
    	case Maps::Ground::DESERT:
    	case Maps::Ground::SNOW:        wav = (1 == speed ? M82::WSND05 : (2 == speed ? M82::WSND15 : M82::WSND25)); break;
    	case Maps::Ground::DIRT:        wav = (1 == speed ? M82::WSND06 : (2 == speed ? M82::WSND16 : M82::WSND26)); break;

    	default: break;
    }

    if(wav != M82::UNKNOWN) AGG::PlaySound(wav);
}

bool ReflectSprite(int from)
{
    switch(from)
    {
        case Direction::BOTTOM_LEFT:
        case Direction::LEFT:
        case Direction::TOP_LEFT:		return true;

        default: break;
    }

    return false;
}

Sprite SpriteHero(const Heroes & hero, int index, bool reflect, bool rotate)
{
    int icn_hero = ICN::UNKNOWN;
    int index_sprite = 0;

    if(hero.isShipMaster()) icn_hero = ICN::BOAT32;
    else
    switch(hero.GetRace())
    {
        case Race::KNGT: icn_hero = ICN::KNGT32; break;
        case Race::BARB: icn_hero = ICN::BARB32; break;
        case Race::SORC: icn_hero = ICN::SORC32; break;
        case Race::WRLK: icn_hero = ICN::WRLK32; break;
        case Race::WZRD: icn_hero = ICN::WZRD32; break;
        case Race::NECR: icn_hero = ICN::NECR32; break;

        default: DEBUG(DBG_GAME, DBG_WARN, "unknown race"); break;
    }

    if(rotate)				index_sprite = 45;
    else
    switch(hero.GetDirection())
    {
        case Direction::TOP:            index_sprite =  0; break;
        case Direction::TOP_RIGHT:      index_sprite =  9; break;
        case Direction::RIGHT:          index_sprite = 18; break;
        case Direction::BOTTOM_RIGHT:   index_sprite = 27; break;
        case Direction::BOTTOM:         index_sprite = 36; break;
        case Direction::BOTTOM_LEFT:    index_sprite = 27; break;
        case Direction::LEFT:           index_sprite = 18; break;
        case Direction::TOP_LEFT:       index_sprite =  9; break;

        default: DEBUG(DBG_GAME, DBG_WARN, "unknown direction"); break;
    }

    return AGG::GetICN(icn_hero, index_sprite + (index % 9), reflect);
}

Sprite SpriteFlag(const Heroes & hero, int index, bool reflect, bool rotate)
{
    int icn_flag = ICN::UNKNOWN;
    int index_sprite = 0;

    switch(hero.GetColor())
    {
        case Color::BLUE:       icn_flag = hero.isShipMaster() ? ICN::B_BFLG32 : ICN::B_FLAG32; break;
        case Color::GREEN:      icn_flag = hero.isShipMaster() ? ICN::G_BFLG32 : ICN::G_FLAG32; break;
        case Color::RED:        icn_flag = hero.isShipMaster() ? ICN::R_BFLG32 : ICN::R_FLAG32; break;
        case Color::YELLOW:     icn_flag = hero.isShipMaster() ? ICN::Y_BFLG32 : ICN::Y_FLAG32; break;
        case Color::ORANGE:     icn_flag = hero.isShipMaster() ? ICN::O_BFLG32 : ICN::O_FLAG32; break;
        case Color::PURPLE:     icn_flag = hero.isShipMaster() ? ICN::P_BFLG32 : ICN::P_FLAG32; break;

        default: DEBUG(DBG_GAME, DBG_WARN, "unknown color"); break;
    }

    if(rotate)				index_sprite = 45;
    else
    switch(hero.GetDirection())
    {
        case Direction::TOP:            index_sprite =  0; break;
        case Direction::TOP_RIGHT:      index_sprite =  9; break;
        case Direction::RIGHT:          index_sprite = 18; break;
        case Direction::BOTTOM_RIGHT:   index_sprite = 27; break;
        case Direction::BOTTOM:         index_sprite = 36; break;
        case Direction::BOTTOM_LEFT:    index_sprite = 27; break;
        case Direction::LEFT:           index_sprite = 18; break;
        case Direction::TOP_LEFT:       index_sprite =  9; break;

        default: DEBUG(DBG_GAME, DBG_WARN, "unknown direction"); break;
    }

    return AGG::GetICN(icn_flag, index_sprite + (index % 9), reflect);
}

Sprite SpriteShad(const Heroes & hero, int index)
{
    int icn_shad = hero.isShipMaster() ? ICN::BOATSHAD : ICN::SHADOW32;
    int index_sprite = 0;

    switch(hero.GetDirection())
    {
        case Direction::TOP:            index_sprite =  0; break;
        case Direction::TOP_RIGHT:      index_sprite =  9; break;
        case Direction::RIGHT:          index_sprite = 18; break;
        case Direction::BOTTOM_RIGHT:   index_sprite = 27; break;
        case Direction::BOTTOM:         index_sprite = 36; break;
        case Direction::BOTTOM_LEFT:    index_sprite = 45; break;
        case Direction::LEFT:           index_sprite = 54; break;
        case Direction::TOP_LEFT:       index_sprite = 63; break;

        default: DEBUG(DBG_GAME, DBG_WARN, "unknown direction"); break;
    }

    return AGG::GetICN(icn_shad, index_sprite + (index % 9));
}

Sprite SpriteFroth(const Heroes & hero, int index, bool reflect)
{
    int index_sprite = 0;

    switch(hero.GetDirection())
    {
        case Direction::TOP:            index_sprite =  0; break;
        case Direction::TOP_RIGHT:      index_sprite =  9; break;
        case Direction::RIGHT:          index_sprite = 18; break;
        case Direction::BOTTOM_RIGHT:   index_sprite = 27; break;
        case Direction::BOTTOM:         index_sprite = 36; break;
        case Direction::BOTTOM_LEFT:    index_sprite = 27; break;
        case Direction::LEFT:           index_sprite = 18; break;
        case Direction::TOP_LEFT:       index_sprite =  9; break;

        default: DEBUG(DBG_GAME, DBG_WARN, "unknown direction"); break;
    }

    return AGG::GetICN(ICN::FROTH, index_sprite + (index % 9), reflect);
}

bool isNeedStayFrontObject(const Heroes & hero, const Maps::Tiles & next)
{
    if(next.GetObject() == MP2::OBJ_CASTLE)
    {
	const Castle* castle = world.GetCastle(next.GetCenter());

	return (castle &&
		! hero.isFriends(castle->GetColor()));
    }
    else
    // to coast action
    if(hero.isShipMaster() &&
	next.GetObject() == MP2::OBJ_COAST)
	return true;

    return MP2::isNeedStayFront(next.GetObject());
}

void Heroes::Redraw(Surface & dst, bool with_shadow) const
{
    const Point & mp = GetCenter();
    const Interface::GameArea & gamearea = Interface::Basic::Get().GetGameArea();
    s32 dx = gamearea.GetMapsPos().x + TILEWIDTH * (mp.x - gamearea.GetRectMaps().x);
    s32 dy = gamearea.GetMapsPos().y + TILEWIDTH * (mp.y - gamearea.GetRectMaps().y);

    Redraw(dst, dx, dy, with_shadow);
}

void Heroes::Redraw(Surface & dst, s32 dx, s32 dy, bool with_shadow) const
{
    const Point & mp = GetCenter();
    const Interface::GameArea & gamearea = Interface::Basic::Get().GetGameArea();
    if(!(gamearea.GetRectMaps() & mp)) return;

    bool reflect = ReflectSprite(direction);

    const Sprite & sprite1 = SpriteHero(*this, sprite_index, reflect, false);
    const Sprite & sprite2 = SpriteFlag(*this, sprite_index, reflect, false);
    const Sprite & sprite3 = SpriteShad(*this, sprite_index);
    const Sprite & sprite4 = SpriteFroth(*this, sprite_index, reflect);

    Point dst_pt1(dx + (reflect ? TILEWIDTH - sprite1.x() - sprite1.w() : sprite1.x()), dy + sprite1.y() + TILEWIDTH);
    Point dst_pt2(dx + (reflect ? TILEWIDTH - sprite2.x() - sprite2.w() : sprite2.x()), dy + sprite2.y() + TILEWIDTH);
    Point dst_pt3(dx + sprite3.x(), dy + sprite3.y() + TILEWIDTH);
    Point dst_pt4(dx + (reflect ? TILEWIDTH - sprite4.x() - sprite4.w() : sprite4.x()), dy + sprite4.y() + TILEWIDTH);

    // apply offset
    if(sprite_index < 45)
    {
	s32 ox = 0;
	s32 oy = 0;
	int frame = (sprite_index % 9);

	switch(direction)
	{
    	    case Direction::TOP:            oy = -4 * frame; break;
    	    case Direction::TOP_RIGHT:      ox = 4 * frame; oy = -4 * frame; break;
    	    case Direction::TOP_LEFT:       ox = -4 * frame; oy = -4 * frame; break;
    	    case Direction::BOTTOM_RIGHT:   ox = 4 * frame; oy = 4 * frame; break;
    	    case Direction::BOTTOM:         oy = 4 * frame; break;
    	    case Direction::BOTTOM_LEFT:    ox = -4 * frame; oy = 4 * frame; break;
    	    case Direction::RIGHT:          ox = 4 * frame; break;
    	    case Direction::LEFT:           ox = -4 * frame; break;
    	    default: break;
	}

	dst_pt1.x += ox;
	dst_pt1.y += oy;
	dst_pt2.x += ox;
	dst_pt2.y += oy;
	dst_pt3.x += ox;
	dst_pt3.y += oy;
	dst_pt4.x += ox;
	dst_pt4.y += oy;
    }

    if(isShipMaster())
    {
	dst_pt1.y -= 15;
	dst_pt2.y -= 15;
	dst_pt3.y -= 15;
	dst_pt4.y -= 15;

	sprite4.Blit(gamearea.RectFixed(dst_pt4, sprite4.w(), sprite4.h()), dst_pt4, dst);
    }

    // redraw sprites for shadow
    if(with_shadow)
	sprite3.Blit(gamearea.RectFixed(dst_pt3, sprite3.w(), sprite3.h()), dst_pt3, dst);

    // redraw sprites hero and flag
    sprite1.Blit(gamearea.RectFixed(dst_pt1, sprite1.w(), sprite1.h()), dst_pt1, dst);
    sprite2.Blit(gamearea.RectFixed(dst_pt2, sprite2.w(), sprite2.h()), dst_pt2, dst);

    // redraw dependences tiles
    Maps::Tiles & tile = world.GetTiles(center.x, center.y);
    const s32 centerIndex = GetIndex();
    bool skip_ground = MP2::isActionObject(tile.GetObject(false), isShipMaster());

    tile.RedrawTop(dst);

    if(Maps::isValidDirection(centerIndex, Direction::TOP))
	world.GetTiles(Maps::GetDirectionIndex(centerIndex, Direction::TOP)).RedrawTop4Hero(dst, skip_ground);

    if(Maps::isValidDirection(centerIndex, Direction::BOTTOM))
    {
	Maps::Tiles & tile_bottom = world.GetTiles(Maps::GetDirectionIndex(centerIndex, Direction::BOTTOM));
	tile_bottom.RedrawBottom4Hero(dst);
	tile_bottom.RedrawTop(dst);
    }

    if(45 > GetSpriteIndex())
    {
	if(Direction::BOTTOM != direction &&
	    Direction::TOP != direction &&
	    Maps::isValidDirection(centerIndex, direction))
	{
	    if(Maps::isValidDirection(Maps::GetDirectionIndex(centerIndex, direction), Direction::BOTTOM))
	    {
		Maps::Tiles & tile_dir_bottom = world.GetTiles(Maps::GetDirectionIndex(Maps::GetDirectionIndex(centerIndex, direction), Direction::BOTTOM));
    		tile_dir_bottom.RedrawBottom4Hero(dst);
		tile_dir_bottom.RedrawTop(dst);
	    }
	    if(Maps::isValidDirection(Maps::GetDirectionIndex(centerIndex, direction), Direction::TOP))
	    {
		Maps::Tiles & tile_dir_top = world.GetTiles(Maps::GetDirectionIndex(Maps::GetDirectionIndex(centerIndex, direction), Direction::TOP));
		tile_dir_top.RedrawTop4Hero(dst, skip_ground);
	    }
	}

	if(Maps::isValidDirection(centerIndex, Direction::BOTTOM))
	{
	    Maps::Tiles & tile_bottom = world.GetTiles(Maps::GetDirectionIndex(centerIndex, Direction::BOTTOM));

	    if(tile_bottom.GetObject() == MP2::OBJ_BOAT)
    		tile_bottom.RedrawObjects(dst);
	}
    }

    if(Maps::isValidDirection(centerIndex, direction))
    {
	if(Direction::TOP == direction)
	    world.GetTiles(Maps::GetDirectionIndex(centerIndex, direction)).RedrawTop4Hero(dst, skip_ground);
	else
	    world.GetTiles(Maps::GetDirectionIndex(centerIndex, direction)).RedrawTop(dst);
    }
}

void Heroes::MoveStep(Heroes & hero, s32 index_from, s32 index_to, bool newpos)
{
    if(newpos)
    {
	hero.Move2Dest(index_to);
	hero.GetPath().PopFront();

	// possible hero is die
	if(!hero.isFreeman() &&
	    index_to == hero.GetPath().GetDestinationIndex())
	{
	    hero.GetPath().Reset();
	    hero.Action(index_to);
	    hero.SetMove(false);
	}
    }
    else
    {
	hero.ApplyPenaltyMovement();
	hero.GetPath().Reset();
	hero.Action(index_to);
	hero.SetMove(false);
    }
}

bool Heroes::MoveStep(bool fast)
{
    s32 index_from = GetIndex();
    s32 index_to = Maps::GetDirectionIndex(index_from, path.GetFrontDirection());
    s32 index_dst = path.GetDestinationIndex();
    const Point & mp = GetCenter();

    if(fast)
    {
	if(index_to == index_dst && isNeedStayFrontObject(*this, world.GetTiles(index_to)))
	    MoveStep(*this, index_from, index_to, false);
	else
	    MoveStep(*this, index_from, index_to, true);

	return true;
    }
    else
    if(0 == sprite_index % 9)
    {
	if(index_to == index_dst && isNeedStayFrontObject(*this, world.GetTiles(index_to)))
	{
	    MoveStep(*this, index_from, index_to, false);

	    return true;
	}
	else
	{
	    // play sound
	    if(GetKingdom().isControlHuman())
		PlayWalkSound(world.GetTiles(mp.x, mp.y).GetGround());
	}
    }
    else
    if(8 == sprite_index % 9)
    {
	sprite_index -= 8;
	MoveStep(*this, index_from, index_to, true);

	return true;
    }

    ++sprite_index;

    return false;
}

void Heroes::AngleStep(int to_direct)
{
    //bool check = false;
    bool clockwise = Direction::ShortDistanceClockWise(direction, to_direct);

    // start index
    if(45 > sprite_index && 0 == sprite_index % 9)
    {
	switch(direction)
	{
    	    case Direction::TOP:		sprite_index = 45; break;
    	    case Direction::TOP_RIGHT:		sprite_index = clockwise ? 47 : 46; break;
    	    case Direction::TOP_LEFT:		sprite_index = clockwise ? 46 : 47; break;
    	    case Direction::RIGHT:		sprite_index = clockwise ? 49 : 48; break;
    	    case Direction::LEFT:		sprite_index = clockwise ? 48 : 49; break;
    	    case Direction::BOTTOM_RIGHT:	sprite_index = clockwise ? 51 : 50; break;
    	    case Direction::BOTTOM_LEFT:	sprite_index = clockwise ? 50 : 51; break;
    	    case Direction::BOTTOM:		sprite_index = clockwise ? 52 : 53; break;

	    default: break;
	}
    }
    // animation process
    else
    {
	switch(direction)
	{
    	    case Direction::TOP_RIGHT:
    	    case Direction::RIGHT:
    	    case Direction::BOTTOM_RIGHT:
		clockwise ? ++sprite_index : --sprite_index;
		break;

    	    case Direction::TOP:
		++sprite_index;
		break;

    	    case Direction::TOP_LEFT:
    	    case Direction::LEFT:
    	    case Direction::BOTTOM_LEFT:
		clockwise ? --sprite_index : ++sprite_index;
		break;

    	    case Direction::BOTTOM:
		--sprite_index;
		break;

	    default: break;
	}

	bool end = false;
	int next = Direction::UNKNOWN;

	switch(direction)
	{
    	    case Direction::TOP:		next = clockwise ? Direction::TOP_RIGHT : Direction::TOP_LEFT; break;
    	    case Direction::TOP_RIGHT:		next = clockwise ? Direction::RIGHT : Direction::TOP; break;
    	    case Direction::TOP_LEFT:		next = clockwise ? Direction::TOP : Direction::LEFT; break;
    	    case Direction::RIGHT:		next = clockwise ? Direction::BOTTOM_RIGHT : Direction::TOP_RIGHT; break;
    	    case Direction::LEFT:		next = clockwise ? Direction::TOP_LEFT : Direction::BOTTOM_LEFT; break;
    	    case Direction::BOTTOM_RIGHT:	next = clockwise ? Direction::BOTTOM : Direction::RIGHT; break;
    	    case Direction::BOTTOM_LEFT:	next = clockwise ? Direction::LEFT : Direction::BOTTOM; break;
    	    case Direction::BOTTOM:		next = clockwise ? Direction::BOTTOM_LEFT : Direction::BOTTOM_RIGHT; break;

	    default: break;
	}

	switch(next)
	{
    	    case Direction::TOP:		end = (sprite_index == 44); break;
    	    case Direction::TOP_RIGHT:		end = (sprite_index == (clockwise ? 47 : 46)); break;
    	    case Direction::TOP_LEFT:		end = (sprite_index == (clockwise ? 46 : 47)); break;
    	    case Direction::RIGHT:		end = (sprite_index == (clockwise ? 49 : 48)); break;
    	    case Direction::LEFT:		end = (sprite_index == (clockwise ? 48 : 49)); break;
    	    case Direction::BOTTOM_RIGHT:	end = (sprite_index == (clockwise ? 51 : 50)); break;
    	    case Direction::BOTTOM_LEFT:	end = (sprite_index == (clockwise ? 50 : 51)); break;
    	    case Direction::BOTTOM:		end = (sprite_index == 53); break;

	    default: break;
	}

	if(end)
	{
	    switch(next)
	    {
    		case Direction::TOP:            sprite_index = 0; break;
    		case Direction::BOTTOM:         sprite_index = 36; break;
    		case Direction::TOP_RIGHT:
    		case Direction::TOP_LEFT:       sprite_index = 9; break;
    		case Direction::BOTTOM_RIGHT:
    		case Direction::BOTTOM_LEFT:    sprite_index = 27; break;
    		case Direction::RIGHT:
    		case Direction::LEFT:           sprite_index = 18; break;

		default: break;
	    }

	    direction = next;
	}
    }
}

void Heroes::FadeOut(void) const
{
    const Point & mp = GetCenter();
    const Interface::GameArea & gamearea = Interface::Basic::Get().GetGameArea();

    if(!(gamearea.GetRectMaps() & mp)) return;

    Display & display = Display::Get();

    bool reflect = ReflectSprite(direction);

    s32 dx = gamearea.GetMapsPos().x + TILEWIDTH * (mp.x - gamearea.GetRectMaps().x);
    s32 dy = gamearea.GetMapsPos().y + TILEWIDTH * (mp.y - gamearea.GetRectMaps().y);

    Sprite sphero = SpriteHero(*this, sprite_index, reflect, false);
    Sprite sprite1 = Sprite(sphero.GetSurface(), sphero.x(), sphero.y());

    Point dst_pt1(dx + (reflect ? TILEWIDTH - sprite1.x() - sprite1.w() : sprite1.x()), dy + sprite1.y() + TILEWIDTH);
    const Rect src_rt = gamearea.RectFixed(dst_pt1, sprite1.w(), sprite1.h());

    LocalEvent & le = LocalEvent::Get();
    int alpha = 250;

    while(le.HandleEvents() && alpha > 0)
    {
        if(Game::AnimateInfrequentDelay(Game::HEROES_FADE_DELAY))
        {
            Cursor::Get().Hide();

	    for(s32 y = mp.y - 1; y <= mp.y + 1; ++y)
		for(s32 x = mp.x - 1; x <= mp.x + 1; ++x)
    		    if(Maps::isValidAbsPoint(x, y))
	    {
        	const Maps::Tiles & tile = world.GetTiles(Maps::GetIndexFromAbsPoint(x, y));

        	tile.RedrawTile(display);
        	tile.RedrawBottom(display);
        	tile.RedrawObjects(display);
    	    }

    	    sprite1.SetAlphaMod(alpha);
    	    sprite1.Blit(src_rt, dst_pt1, display);

	    for(s32 y = mp.y - 1; y <= mp.y + 1; ++y)
		for(s32 x = mp.x - 1; x <= mp.x + 1; ++x)
    		    if(Maps::isValidAbsPoint(x, y))
	    {
        	const Maps::Tiles & tile = world.GetTiles(Maps::GetIndexFromAbsPoint(x, y));

        	tile.RedrawTop(display);
    	    }

	    Cursor::Get().Show();
	    display.Flip();
            alpha -= 10;
        }
    }
}

void Heroes::FadeIn(void) const
{
    const Point & mp = GetCenter();
    const Interface::GameArea & gamearea = Interface::Basic::Get().GetGameArea();

    if(!(gamearea.GetRectMaps() & mp)) return;

    Display & display = Display::Get();

    bool reflect = ReflectSprite(direction);

    s32 dx = gamearea.GetMapsPos().x + TILEWIDTH * (mp.x - gamearea.GetRectMaps().x);
    s32 dy = gamearea.GetMapsPos().y + TILEWIDTH * (mp.y - gamearea.GetRectMaps().y);

    Sprite sphero = SpriteHero(*this, sprite_index, reflect, false);
    Sprite sprite1 = Sprite(sphero.GetSurface(), sphero.x(), sphero.y());

    Point dst_pt1(dx + (reflect ? TILEWIDTH - sprite1.x() - sprite1.w() : sprite1.x()), dy + sprite1.y() + TILEWIDTH);
    const Rect src_rt = gamearea.RectFixed(dst_pt1, sprite1.w(), sprite1.h());

    LocalEvent & le = LocalEvent::Get();
    int alpha = 0;

    while(le.HandleEvents() && alpha < 250)
    {
        if(Game::AnimateInfrequentDelay(Game::HEROES_FADE_DELAY))
        {
            Cursor::Get().Hide();

	    for(s32 y = mp.y - 1; y <= mp.y + 1; ++y)
		for(s32 x = mp.x - 1; x <= mp.x + 1; ++x)
    		    if(Maps::isValidAbsPoint(x, y))
	    {
        	const Maps::Tiles & tile = world.GetTiles(Maps::GetIndexFromAbsPoint(x, y));

        	tile.RedrawTile(display);
        	tile.RedrawBottom(display);
        	tile.RedrawObjects(display);
    	    }

    	    sprite1.SetAlphaMod(alpha);
    	    sprite1.Blit(src_rt, dst_pt1, display);

	    for(s32 y = mp.y - 1; y <= mp.y + 1; ++y)
		for(s32 x = mp.x - 1; x <= mp.x + 1; ++x)
    		    if(Maps::isValidAbsPoint(x, y))
	    {
        	const Maps::Tiles & tile = world.GetTiles(Maps::GetIndexFromAbsPoint(x, y));

        	tile.RedrawTop(display);
    	    }

	    Cursor::Get().Show();
	    display.Flip();
            alpha += 10;
        }
    }
}

bool Heroes::Move(bool fast)
{
    if(Modes(ACTION)) ResetModes(ACTION);

    // move hero
    if(path.isValid() &&
           (isEnableMove() || (GetSpriteIndex() < 45 && GetSpriteIndex() % 9) || GetSpriteIndex() >= 45))
    {
	// fast move for hide AI
	if(fast)
	{
	    direction = path.GetFrontDirection();
    	    MoveStep(true);

	    return true;
	}
	else
	{
    	    // if need change through the circle
	    if(GetDirection() != path.GetFrontDirection())
    	    {
                AngleStep(path.GetFrontDirection());
            }
            else
            // move
    	    if(MoveStep())
    	    {
    		if(isFreeman()) return false;

		return true;
    	    }
	}
    }
    else
    {
	SetMove(false);
    }

    return false;
}
