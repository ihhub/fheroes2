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
#include <iterator>
#include <ctime>
#include <cctype>
#include <sstream>
#include <string>

#include "system.h"
#include "dir.h"
#include "agg.h"
#include "text.h"
#include "button.h"
#include "cursor.h"
#include "settings.h"
#include "maps_fileinfo.h"
#include "interface_list.h"
#include "pocketpc.h"
#include "world.h"
#include "game.h"
#include "dialog.h"

std::string SelectFileListSimple(const std::string &, const std::string &, bool);
bool RedrawExtraInfo(const Point &, const std::string &, const std::string &, const Rect &);

class FileInfoListBox : public Interface::ListBox<Maps::FileInfo>
{
public:
    FileInfoListBox(const Point & pt, bool & edit) : Interface::ListBox<Maps::FileInfo>(pt), edit_mode(edit) {};

    void RedrawItem(const Maps::FileInfo &, s32, s32, bool);
    void RedrawBackground(const Point &);

    void ActionCurrentUp(void);
    void ActionCurrentDn(void);
    void ActionListDoubleClick(Maps::FileInfo &);
    void ActionListSingleClick(Maps::FileInfo &);
    void ActionListPressRight(Maps::FileInfo &){};

    bool & edit_mode;
};

void FileInfoListBox::RedrawItem(const Maps::FileInfo & info, s32 dstx, s32 dsty, bool current)
{
    char short_date[20];
    time_t timeval = info.localtime;

    std::fill(short_date, ARRAY_COUNT_END(short_date), 0);
    std::strftime(short_date, ARRAY_COUNT(short_date) - 1, "%b %d, %H:%M", std::localtime(&timeval));
    std::string savname(System::GetBasename(info.file));

    if(savname.size())
    {
	Text text;
	const size_t dotpos = savname.size() - 4;
    	if(StringLower(savname.substr(dotpos)) == ".sav") savname.erase(dotpos);

	text.Set(savname, (current ? Font::YELLOW_BIG : Font::BIG));
	text.Blit(dstx + 5, dsty, (Settings::Get().QVGA() ? 190 : 155));

	text.Set(short_date, (current ? Font::YELLOW_BIG : Font::BIG));
	text.Blit(dstx + 265 - text.w(), dsty);
    }
}

void FileInfoListBox::RedrawBackground(const Point & dst)
{
    const Sprite & panel = AGG::GetICN(ICN::REQBKG, 0);

    if(Settings::Get().QVGA())
    {
	panel.Blit(Rect(0, 0, panel.w(), 120), dst.x, dst.y);
	panel.Blit(Rect(0, panel.h() - 120, panel.w(), 120), dst.x, dst.y + 224 - 120);
    }
    else
	panel.Blit(dst);
}

void FileInfoListBox::ActionCurrentUp(void)
{
    edit_mode = false;
}

void FileInfoListBox::ActionCurrentDn(void)
{
    edit_mode = false;
}

void FileInfoListBox::ActionListDoubleClick(Maps::FileInfo &)
{
    edit_mode = false;
}

void FileInfoListBox::ActionListSingleClick(Maps::FileInfo &)
{
    edit_mode = false;
}

std::string ResizeToShortName(const std::string & str)
{
    std::string res = System::GetBasename(str);
    size_t it = res.find('.');
    if(std::string::npos != it) res.resize(it);
    return res;
}

size_t GetInsertPosition(const std::string & name, s32 cx, s32 posx)
{
    if(name.size())
    {
	s32 tw = Text::width(name, Font::SMALL);
	if(cx <= posx)
	    return 0;
	else
	if(cx >= posx + tw)
	    return name.size();
	else
	{
	    float cw = tw / name.size();
	    return static_cast<size_t>((cx - posx) / cw);
	}
    }
    return 0;
}

MapsFileInfoList GetSortedMapsFileInfoList(void)
{
    ListFiles list1;
    list1.ReadDir(Settings::GetSaveDir(), ".sav", false);

    MapsFileInfoList list2(list1.size());
    int ii = 0;
    for(ListFiles::const_iterator itd = list1.begin(); itd != list1.end(); ++itd, ++ii) if(!list2[ii].ReadSAV(*itd)) --ii;
    if(static_cast<size_t>(ii) != list2.size()) list2.resize(ii);
    std::sort(list2.begin(), list2.end(), Maps::FileInfo::FileSorting);

    return list2;
}

bool IsPunct(char c)
{
    return true;
}

std::string Dialog::SelectFileSave(void)
{
    const Settings & conf = Settings::Get();
    const std::string & name = conf.CurrentFileInfo().name;

    std::string base = name.size() ? name : "newgame";
    base.resize(std::distance(base.begin(), std::find_if(base.begin(), base.end(), ::ispunct)));
    std::replace_if(base.begin(), base.end(), ::isspace, '_');
    std::ostringstream os;

    os << System::ConcatePath(Settings::GetSaveDir(), base) <<
	    // add postfix:
	    '_' << std::setw(4) << std::setfill('0') << world.CountDay() << ".sav";
    std::string lastfile = os.str();
    return SelectFileListSimple(_("File to Save:"), lastfile, true);
}

std::string Dialog::SelectFileLoad(void)
{
    const std::string & lastfile = Game::GetLastSavename();
    return SelectFileListSimple(_("File to Load:"), (lastfile.size() ? lastfile : ""), false);
}

std::string SelectFileListSimple(const std::string & header, const std::string & lastfile, bool editor)
{
    Display & display = Display::Get();
    Cursor & cursor = Cursor::Get();
    LocalEvent & le = LocalEvent::Get();

    cursor.Hide();
    cursor.SetThemes(cursor.POINTER);

    const Sprite & sprite = AGG::GetICN(ICN::REQBKG, 0);
    Size panel(sprite.w(), sprite.h());
    bool pocket = Settings::Get().QVGA();
    if(pocket) panel = Size(sprite.w(), 224);

    SpriteBack back(Rect((display.w() - panel.w) / 2, (display.h() - panel.h) / 2, panel.w, panel.h));

    const Rect & rt = back.GetArea();
    const Rect enter_field(rt.x + 42, rt.y + (pocket ? 148 : 286), 260, 16);

    Button buttonOk(rt.x + 34, rt.y + (pocket ? 176 : 315), ICN::REQUEST, 1, 2);
    Button buttonCancel(rt.x + 244, rt.y + (pocket ? 176 : 315), ICN::REQUEST, 3, 4);

    bool edit_mode = false;

    MapsFileInfoList lists = GetSortedMapsFileInfoList();
    FileInfoListBox listbox(rt, edit_mode);

    listbox.RedrawBackground(rt);
    listbox.SetScrollButtonUp(ICN::REQUESTS, 5, 6, Point(rt.x + 327, rt.y + 55));
    listbox.SetScrollButtonDn(ICN::REQUESTS, 7, 8, Point(rt.x + 327, rt.y + (pocket ? 117 : 257)));
    listbox.SetScrollSplitter(AGG::GetICN(ICN::ESCROLL, 3), Rect(rt.x + 328, rt.y + 73, 12, (pocket ? 40 : 180)));
    listbox.SetAreaMaxItems(pocket ? 5 : 11);
    listbox.SetAreaItems(Rect(rt.x + 40, rt.y + 55, 265, (pocket ? 78 : 215)));
    listbox.SetListContent(lists);

    std::string filename;
    size_t charInsertPos = 0;

    if(lastfile.size())
    {
	filename = ResizeToShortName(lastfile);
	charInsertPos = filename.size();

	MapsFileInfoList::iterator it = lists.begin();
	for(; it != lists.end(); ++it) if((*it).file == lastfile) break;

	if(it != lists.end())
	    listbox.SetCurrent(std::distance(lists.begin(), it));
	else
    	    listbox.Unselect();
    }

    if(!editor && lists.empty())
    	buttonOk.SetDisable(true);

    if(filename.empty() && listbox.isSelected())
    {
        filename = ResizeToShortName(listbox.GetCurrent().file);
	charInsertPos = filename.size();
    }

    listbox.Redraw();
    RedrawExtraInfo(rt, header, filename, enter_field);

    buttonOk.Draw();
    buttonCancel.Draw();

    cursor.Show();
    display.Flip();

    std::string result;
    bool is_limit = false;

    while(le.HandleEvents() && result.empty())
    {
        le.MousePressLeft(buttonOk) && buttonOk.isEnable() ? buttonOk.PressDraw() : buttonOk.ReleaseDraw();
        le.MousePressLeft(buttonCancel) ? buttonCancel.PressDraw() : buttonCancel.ReleaseDraw();

	listbox.QueueEventProcessing();

        if((buttonOk.isEnable() && le.MouseClickLeft(buttonOk)) || Game::HotKeyPressEvent(Game::EVENT_DEFAULT_READY))
        {
    	    if(filename.size())
		result = System::ConcatePath(Settings::GetSaveDir(), filename + ".sav");
    	    else
    	    if(listbox.isSelected())
    		result = listbox.GetCurrent().file;
    	}
    	else
        if(le.MouseClickLeft(buttonCancel) || Game::HotKeyPressEvent(Game::EVENT_DEFAULT_EXIT))
        {
    	    break;
	}
	else
        if(le.MouseClickLeft(enter_field) && editor)
	{
	    edit_mode = true;
	    charInsertPos = GetInsertPosition(filename, le.GetMouseCursor().x, enter_field.x);
	    if(Settings::Get().PocketPC())
		PocketPC::KeyboardDialog(filename);
    	    buttonOk.SetDisable(filename.empty());
	    cursor.Hide();
	}
	else
	if(edit_mode && le.KeyPress() &&
	    (!is_limit || KEY_BACKSPACE == le.KeyValue()))
	{
	    charInsertPos = InsertKeySym(filename, charInsertPos, le.KeyValue(), le.KeyMod());
	    buttonOk.SetDisable(filename.empty());
	    cursor.Hide();
	}
	if((le.KeyPress(KEY_DELETE) || (pocket && le.MousePressRight())) && listbox.isSelected())
	{
	    std::string msg(_("Are you sure you want to delete file:"));
	    msg.append("\n \n");
	    msg.append(System::GetBasename(listbox.GetCurrent().file));
	    if(Dialog::YES == Dialog::Message(_("Warning!"), msg, Font::BIG, Dialog::YES | Dialog::NO))
	    {
		System::Unlink(listbox.GetCurrent().file);
		listbox.RemoveSelected();
		if(lists.empty() || filename.empty()) buttonOk.SetDisable(true);
		listbox.SetListContent(lists);
	    }
	    cursor.Hide();
	}

	if(! cursor.isVisible())
	{
	    listbox.Redraw();

	    if(edit_mode && editor)
		is_limit = RedrawExtraInfo(rt, header, InsertString(filename, charInsertPos, "_"), enter_field);
	    else
	    if(listbox.isSelected())
	    {
		filename = ResizeToShortName(listbox.GetCurrent().file);
		charInsertPos = filename.size();
		is_limit = RedrawExtraInfo(rt, header, filename, enter_field);
	    }
	    else
		is_limit = RedrawExtraInfo(rt, header, filename, enter_field);

	    buttonOk.Draw();
	    buttonCancel.Draw();
	    cursor.Show();
	    display.Flip();
	}
    }

    cursor.Hide();
    back.Restore();

    return result;
}

bool RedrawExtraInfo(const Point & dst, const std::string & header, const std::string & filename, const Rect & field)
{
    Text text(header, Font::BIG);
    text.Blit(dst.x + 175 - text.w() / 2, dst.y + 30);

    if(filename.size())
    {
	text.Set(filename, Font::BIG);
	text.Blit(field.x, field.y + 1, field.w);
    }

    return text.w() + 10 > field.w;
}
