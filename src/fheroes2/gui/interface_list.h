/***************************************************************************
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#ifndef H2INTERFACE_LIST_H
#define H2INTERFACE_LIST_H

#include <algorithm>

#include "icn.h"
#include "button.h"
#include "splitter.h"
#include "settings.h"
#include "cursor.h"
#include "sprite.h"
#include "gamedefs.h"

namespace Interface
{
    struct ListBasic
    {
	virtual ~ListBasic(){}
	virtual void Redraw(void) = 0;
	virtual bool QueueEventProcessing(void) = 0;
    };

    template<class Item> 
    class ListBox : public ListBasic
    {
    public:
	typedef std::vector<Item> Items;
	typedef typename std::vector<Item>::iterator ItemsIterator;

	ListBox(const Point & pt) : ptRedraw(pt), maxItems(0), useHotkeys(true), content(NULL) {}
	ListBox() : maxItems(0), useHotkeys(true), content(NULL) {}
	virtual ~ListBox(){}

	virtual void RedrawItem(const Item &, s32 ox, s32 oy, bool current) = 0;
	virtual void RedrawBackground(const Point &) = 0;

	virtual void ActionCurrentUp(void) = 0;
	virtual void ActionCurrentDn(void) = 0;

	virtual void ActionListDoubleClick(Item &) = 0;
	virtual void ActionListSingleClick(Item &) = 0;
	virtual void ActionListPressRight(Item &) = 0;

	virtual void ActionListDoubleClick(Item & item, const Point & cursor, s32 ox, s32 oy){ ActionListDoubleClick(item); };
	virtual void ActionListSingleClick(Item & item, const Point & cursor, s32 ox, s32 oy){ ActionListSingleClick(item); };
	virtual void ActionListPressRight(Item & item, const Point & cursor, s32 ox, s32 oy){ ActionListPressRight(item); };
	virtual bool ActionListCursor(Item & item, const Point & cursor, s32 ox, s32 oy){ return false; };

	/*
	void SetTopLeft(const Point & top);
	void SetScrollButtonUp(int, u32, u32, const Point &);
	void SetScrollButtonDn(int, u32, u32, const Point &);
	void SetScrollSplitter(const Sprite &, const Rect &);
	void SetAreaMaxItems(u32);
	void SetAreaItems(const Rect &);
	void SetListContent(std::vector<Item> &);
	void Redraw(void);
	bool QueueEventProcessing(void);
	Item & GetCurrent(void);
	void SetCurrent(size_t);
	void SetCurrent(const Item &);
	void SetCurrentVisible(void);
	void RemoveSelected(void);
	void DisableHotkeys(bool);
	bool isSelected(void) const;
	void Unselect(void);
	void Reset(void);
	*/

	void SetTopLeft(const Point & tl)
	{
	    ptRedraw = tl;
	}

	void SetScrollButtonUp(int icn, u32 index1, u32 index2, const Point & pos)
	{
	    buttonPgUp.SetSprite(icn, index1, index2);
    	    buttonPgUp.SetPos(pos);
        }

	void SetScrollButtonDn(int icn, u32 index1, u32 index2, const Point & pos)
	{
	    buttonPgDn.SetSprite(icn, index1, index2);
    	    buttonPgDn.SetPos(pos);
        }

	void SetScrollSplitter(const Sprite & sp, const Rect & area)
	{
	    splitter.SetArea(area);
	    splitter.SetSprite(sp);
	}

	Splitter & GetSplitter(void)
	{
	    return splitter;
	}

	void SetAreaMaxItems(int max)
	{
	    maxItems = max;
	}

	void SetAreaItems(const Rect & rt)
	{
	    rtAreaItems = rt;
	}

	void SetListContent(std::vector<Item> & list)
	{
	    content = &list;
	    cur = content->begin();
	    top = content->begin();
	    if(maxItems < list.size())
	    {
		splitter.SetRange(0, list.size() - maxItems);
		splitter.MoveIndex(0);
	    }
	    else
	    {
		splitter.SetRange(0, 0);
		splitter.MoveCenter();
	    }
	}

	void Reset(void)
	{
	    if(content)
	    {
		cur = content->end();
		top = content->begin();
		UpdateSplitterRange();
		splitter.MoveCenter();

		if(maxItems < content->size())
		    splitter.MoveIndex(0);
		else
		    splitter.MoveCenter();
	    }
	}

	void DisableHotkeys(bool f)
	{
	    useHotkeys = !f;
	}

	void Redraw(void)
	{
	    Cursor::Get().Hide();

            RedrawBackground(ptRedraw);

            buttonPgUp.Draw();
            buttonPgDn.Draw();
            splitter.RedrawCursor();

            ItemsIterator curt = top;
            ItemsIterator last = top + maxItems < content->end() ? top + maxItems : content->end();
            for(; curt != last; ++curt)
                RedrawItem(*curt, rtAreaItems.x, rtAreaItems.y + (curt - top) * rtAreaItems.h / maxItems, curt == cur);
        }

	Item & GetCurrent(void)
	{
	    return *cur;
	}

	Item* GetFromPosition(const Point & mp)
	{
	    ItemsIterator click = content->end();
	    float offset = (mp.y - rtAreaItems.y) * maxItems / rtAreaItems.h;
	    click = top + static_cast<size_t>(offset);
	    return click < content->begin() || click >= content->end() ? NULL : &(*click);
	}

	void SetCurrent(size_t pos)
	{
	    if(pos < content->size())
		cur = content->begin() + pos;

	    SetCurrentVisible();
	}

	void SetCurrentVisible(void)
	{
	    if(top > cur || top + maxItems <= cur)
	    {
		top = cur + maxItems > content->end() ? content->end() - maxItems : cur;
		if(top < content->begin()) top = content->begin();
		UpdateSplitterRange();
    		splitter.MoveIndex(top - content->begin());
	    }
	}

	void SetCurrent(const Item & item)
	{
	    cur = std::find(content->begin(), content->end(), item);
	    SetCurrentVisible();
	}

	void RemoveSelected(void)
	{
	    if(content && cur != content->end()) content->erase(cur);
	}

	bool isSelected(void) const
	{
	    return content && cur != content->end();
	}

	void Unselect(void)
	{
	    if(content) cur = content->end();
	}

	bool QueueEventProcessing(void)
	{
	    LocalEvent & le = LocalEvent::Get();
	    Cursor & cursor = Cursor::Get();

	    le.MousePressLeft(buttonPgUp) ? buttonPgUp.PressDraw() : buttonPgUp.ReleaseDraw();
	    le.MousePressLeft(buttonPgDn) ? buttonPgDn.PressDraw() : buttonPgDn.ReleaseDraw();
    
	    if(!content) return false;

	    if((le.MouseClickLeft(buttonPgUp) || (useHotkeys && le.KeyPress(KEY_PAGEUP))) &&
		    (top > content->begin()))
	    {
		cursor.Hide();
		top = (top - content->begin() > static_cast<int>(maxItems) ? top - maxItems : content->begin());
		UpdateSplitterRange();
    		splitter.MoveIndex(top - content->begin());
		return true;
	    }
	    else
	    if((le.MouseClickLeft(buttonPgDn) || (useHotkeys && le.KeyPress(KEY_PAGEDOWN))) &&
		    (top + maxItems < content->end()))
	    {
		cursor.Hide();
		top += maxItems;
		if(top + maxItems > content->end()) top = content->end() - maxItems;
		UpdateSplitterRange();
		splitter.MoveIndex(top - content->begin());
		return true;
	    }
	    else
	    if(useHotkeys && le.KeyPress(KEY_UP) && (cur > content->begin()))
	    {
		cursor.Hide();
		--cur;
		SetCurrentVisible();
		ActionCurrentUp();
		return true;
	    }
	    else
	    if(useHotkeys && le.KeyPress(KEY_DOWN) && (cur < (content->end() - 1)))
	    {
		cursor.Hide();
		++cur;
		SetCurrentVisible();
		ActionCurrentDn();
		return true;
	    }
	    else
	    if((le.MouseWheelUp(rtAreaItems) || le.MouseWheelUp(splitter.GetRect())) && (top > content->begin()))
	    {
		cursor.Hide();
		--top;
		splitter.Backward();
		return true;
	    }
	    else
	    if((le.MouseWheelDn(rtAreaItems) || le.MouseWheelDn(splitter.GetRect())) && (top < (content->end() - maxItems)))
	    {
		cursor.Hide();
    		++top;
		splitter.Forward();
		return true;
	    }
	    else
	    if(le.MousePressLeft(splitter.GetRect()) && (content->size() > maxItems))
	    {
		cursor.Hide();
		UpdateSplitterRange();
		s32 seek = (le.GetMouseCursor().y - splitter.GetRect().y) * 100 / splitter.GetStep();
		if(seek < splitter.Min()) seek = splitter.Min();
		else
		if(seek > splitter.Max()) seek = splitter.Max();
		top = content->begin() + seek;
		splitter.MoveIndex(seek);
		return true;
	    }

	    if(content->size())
	    {
		float offset = (le.GetMouseCursor().y - rtAreaItems.y) * maxItems / rtAreaItems.h;

		if(offset >= 0)
		{
		    cursor.Hide();

		    ItemsIterator pos = top + static_cast<size_t>(offset);

		    if(pos >= content->begin() && pos < content->end())
		    {
			const s32 posy = rtAreaItems.y + (pos - top) * rtAreaItems.h / maxItems;

			if(ActionListCursor(*pos, le.GetMouseCursor(), rtAreaItems.x, posy))
			    return true;

			if(le.MouseClickLeft(rtAreaItems))
			{
			    if(pos == cur)
			    {
				ActionListDoubleClick(*cur, le.GetMouseCursor(), rtAreaItems.x, posy);
			    }
			    else
			    {
				cur = pos;
				ActionListSingleClick(*cur, le.GetMouseCursor(), rtAreaItems.x, posy);
			    }
			    return true;
			}
			else
			if(le.MousePressRight(rtAreaItems))
			{
			    ActionListPressRight(*pos, le.GetMouseCursor(), rtAreaItems.x, posy);
			    return true;
			}
		    }

		    cursor.Show();
		}
	    }

	    return false;
	}

    protected:
	void UpdateSplitterRange(void)
	{
	    int max = content && maxItems < content->size() ? content->size() - maxItems : 0;

	    if(splitter.Max() != max)
		splitter.SetRange(0, max);
	}

	Point		ptRedraw;
	Rect		rtAreaItems;

	Button		buttonPgUp;
	Button		buttonPgDn;

	Splitter	splitter;

	u32		maxItems;
	bool		useHotkeys;

	Items*		content;
	ItemsIterator	cur;
	ItemsIterator	top;
    };
}

#endif
