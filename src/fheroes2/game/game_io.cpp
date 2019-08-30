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

#include <sstream>
#include <cstring>
#include <ctime>
#include "zzlib.h"
#include "text.h"
#include "settings.h"
#include "kingdom.h"
#include "heroes.h"
#include "castle.h"
#include "dialog.h"
#include "army.h"
#include "world.h"
#include "interface_gamearea.h"
#include "settings.h"
#include "tools.h"
#include "game.h"
#include "game_over.h"
#include "game_static.h"
#include "monster.h"
#include "game_io.h"

static u16 SAV2ID2 = 0xFF02;
static u16 SAV2ID3 = 0xFF03;

namespace Game
{
    struct HeaderSAV
    {
	enum { IS_COMPRESS = 0x8000, IS_LOYALTY = 0x4000 };

	HeaderSAV() : status(0)
	{
	}

	HeaderSAV(const Maps::FileInfo & fi, bool loyalty) : status(0), info(fi)
	{
	    time_t rawtime;
	    std::time(&rawtime);
	    info.localtime = rawtime;

	    if(loyalty)
		status |= IS_LOYALTY;

#ifdef WITH_ZLIB
	    status |= IS_COMPRESS;
#endif
	}

	u16		status;
	Maps::FileInfo	info;
    };

    StreamBase & operator<< (StreamBase & msg, const HeaderSAV & hdr)
    {
	return msg << hdr.status << hdr.info;
    }

    StreamBase & operator>> (StreamBase & msg, HeaderSAV & hdr)
    {
	return msg >> hdr.status >> hdr.info;
    }
}

bool Game::Save(const std::string &fn)
{
    DEBUG(DBG_GAME, DBG_INFO, fn);
    const bool autosave = (System::GetBasename(fn) == "autosave.sav");
    const Settings & conf = Settings::Get();

    // ask overwrite?
    if(System::IsFile(fn) &&
	((!autosave && conf.ExtGameRewriteConfirm()) || (autosave && Settings::Get().ExtGameAutosaveConfirm())) &&
	Dialog::NO == Dialog::Message("", _("Are you sure you want to overwrite the save with this name?"), Font::BIG, Dialog::YES|Dialog::NO))
    {
	return false;
    }

    StreamFile fs;
    fs.setbigendian(true);

    if(! fs.open(fn, "wb"))
    {
	DEBUG(DBG_GAME, DBG_INFO, fn << ", error open");
	return false;
    }

    u16 loadver = GetLoadVersion();
    if(! autosave) Game::SetLastSavename(fn);

    // raw info content
    fs << static_cast<char>(SAV2ID3 >> 8) << static_cast<char>(SAV2ID3) <<
	GetString(loadver) << loadver << HeaderSAV(conf.CurrentFileInfo(), conf.PriceLoyaltyVersion());
    fs.close();

    ZStreamFile fz;
    fz.setbigendian(true);

    // zip game data content
    fz << loadver << World::Get() << Settings::Get() <<
	GameOver::Result::Get() << GameStatic::Data::Get() << MonsterStaticData::Get() << SAV2ID3; // eof marker

    return !fz.fail() && fz.write(fn, true);
}

bool Game::Load(const std::string & fn)
{
    DEBUG(DBG_GAME, DBG_INFO, fn);
    Settings & conf = Settings::Get();
    // loading info
    Game::ShowLoadMapsText();

    StreamFile fs;
    fs.setbigendian(true);

    if(! fs.open(fn, "rb"))
    {
	DEBUG(DBG_GAME, DBG_INFO, fn << ", error open");
	return false;
    }

    char major, minor;
    fs >> major >> minor;
    const u16 savid = (static_cast<u16>(major) << 8) | static_cast<u16>(minor);

    // check version sav file
    if(savid != SAV2ID2 && savid != SAV2ID3)
    {
	DEBUG(DBG_GAME, DBG_INFO, fn << ", incorrect SAV2ID");
	return false;
    }

    std::string strver;
    u16 binver = 0;
    HeaderSAV header;

    // read raw info
    fs >> strver >> binver >> header;
    size_t offset = fs.tell();
    fs.close();

#ifndef WITH_ZLIB
    if(header.status & HeaderSAV::IS_COMPRESS)
    {
	DEBUG(DBG_GAME, DBG_INFO, fn << ", zlib: unsupported");
	return false;
    }
#endif

    ZStreamFile fz;
    fz.setbigendian(true);

    if(! fz.read(fn, offset))
    {
	DEBUG(DBG_GAME, DBG_INFO, ", uncompress: error");
	return false;
    }

    if((header.status & HeaderSAV::IS_LOYALTY) &&
	!conf.PriceLoyaltyVersion())
	Dialog::Message("Warning", _("This file is saved in the \"Price Loyalty\" version.\nSome items may be unavailable."), Font::BIG, Dialog::OK);

    //SaveMemToFile(std::vector<u8>(fz.data(), fz.data() + fz.size()), "gdata.bin");
    fz >> binver;

    // check version: false
    if(binver > CURRENT_FORMAT_VERSION || binver < LAST_FORMAT_VERSION)
    {
	std::ostringstream os;
	os << "usupported save format: " << binver << std::endl <<
     	"game version: " << CURRENT_FORMAT_VERSION << std::endl <<
     	"last version: " << LAST_FORMAT_VERSION;
 	Dialog::Message("Error", os.str(), Font::BIG, Dialog::OK);
 	return false;
    }

    DEBUG(DBG_GAME, DBG_TRACE, "load version: " << binver);
    SetLoadVersion(binver);
    u16 end_check = 0;

    fz >> World::Get() >> Settings::Get() >>
	GameOver::Result::Get() >> GameStatic::Data::Get() >> MonsterStaticData::Get() >> end_check;

    World::Get().PostFixLoad();

    if(fz.fail() || (end_check != SAV2ID2 && end_check != SAV2ID3))
    {
	DEBUG(DBG_GAME, DBG_WARN, "invalid load file: " << fn);
	return false;
    }

    SetLoadVersion(CURRENT_FORMAT_VERSION);

    Game::SetLastSavename(fn);
    conf.SetGameType(conf.GameType() | Game::TYPE_LOADFILE);

    return true;
}

bool Game::LoadSAV2FileInfo(const std::string & fn,  Maps::FileInfo & finfo)
{
    StreamFile fs;
    fs.setbigendian(true);

    if(! fs.open(fn, "rb"))
    {
	DEBUG(DBG_GAME, DBG_INFO, fn << ", error open");
	return false;
    }

    char major, minor;
    fs >> major >> minor;
    const u16 savid = (static_cast<u16>(major) << 8) | static_cast<u16>(minor);

    // check version sav file
    if(savid != SAV2ID2 && savid != SAV2ID3)
    {
	DEBUG(DBG_GAME, DBG_INFO, fn << ", incorrect SAV2ID");
	return false;
    }

    std::string strver;
    u16 binver = 0;
    HeaderSAV header;

    // read raw info
    fs >> strver >> binver >> header;

    // hide: unsupported version
    if(binver > CURRENT_FORMAT_VERSION || binver < LAST_FORMAT_VERSION)
	return false;

#ifndef WITH_ZLIB
    // check: compress game data
    if(header.status & HeaderSAV::IS_COMPRESS)
    {
	DEBUG(DBG_GAME, DBG_INFO, fn << ", zlib: unsupported");
	return false;
    }
#endif

    finfo = header.info;
    finfo.file = fn;

    return true;
}
