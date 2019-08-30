/***************************************************************************
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of SDL++ Engine:                                                 *
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

#include <iterator>
#include <cctype>
#include <iostream>
#include <algorithm>

#include "tools.h"
#include "serialize.h"
#include "tinyconfig.h"

bool SpaceCompare(char a, char b)
{
    return std::isspace(a) && std::isspace(b);
}

std::string ModifyKey(const std::string & str)
{
    std::string key = StringTrim(StringLower(str));

    // remove multiple space
    std::string::iterator it = std::unique(key.begin(), key.end(), SpaceCompare);
    key.resize(it - key.begin());
    
    // change space
    std::replace_if(key.begin(), key.end(), ::isspace, 0x20);

    return key;
}

TinyConfig::TinyConfig(char sep, char com) : separator(sep), comment(com)
{
}

bool TinyConfig::Load(const std::string & cfile)
{
    StreamFile sf;
    if(! sf.open(cfile, "rb")) return false;

    std::list<std::string> rows = StringSplit(sf.toString(), "\n");

    for(std::list<std::string>::const_iterator
	it = rows.begin(); it != rows.end(); ++it)
    {
	std::string str = StringTrim(*it);
	if(str.empty() || str[0] == comment) continue;

        size_t pos = str.find(separator);
        if(std::string::npos != pos)
        {
            std::string left(str.substr(0, pos));
            std::string right(str.substr(pos + 1, str.length() - pos - 1));

	    left = StringTrim(left);
    	    right = StringTrim(right);

	    AddEntry(left, right, false);
        }
    }

    return true;
}

bool TinyConfig::Save(const std::string & cfile) const
{
    StreamFile sf;
    if(! sf.open(cfile, "wb")) return false;

    for(const_iterator
	it = begin(); it != end(); ++it)
	sf << it->first << " " << separator << " " << it->second << '\n';

    return true;
}

void TinyConfig::Clear(void)
{
    clear();
}

void TinyConfig::AddEntry(const std::string & key, const std::string & val, bool uniq)
{
    iterator it = end();

    if(uniq &&
	(end() != (it = find(ModifyKey(key)))))
	it->second = val;
    else
	insert(std::pair<std::string, std::string>(ModifyKey(key), val));
}

void TinyConfig::AddEntry(const std::string & key, int val, bool uniq)
{
    iterator it = end();

    if(uniq &&
	(end() != (it = find(ModifyKey(key)))))
	it->second = GetString(val);
    else
	insert(std::pair<std::string, std::string>(ModifyKey(key), GetString(val)));
}

int TinyConfig::IntParams(const std::string & key) const
{
    const_iterator it = find(ModifyKey(key));
    return it != end() ? GetInt(it->second) : 0;
}

std::string TinyConfig::StrParams(const std::string & key) const
{
    const_iterator it = find(ModifyKey(key));
    return it != end() ? it->second : "";
}

std::list<std::string> TinyConfig::ListStr(const std::string & key) const
{
    std::pair<const_iterator, const_iterator> ret = equal_range(ModifyKey(key));
    std::list<std::string> res;

    for(const_iterator
	it = ret.first; it != ret.second; ++it)
	res.push_back(it->second);

    return res;
}

std::list<int> TinyConfig::ListInt(const std::string & key) const
{
    std::pair<const_iterator, const_iterator> ret = equal_range(ModifyKey(key));
    std::list<int> res;

    for(const_iterator
	it = ret.first; it != ret.second; ++it)
	res.push_back(GetInt(it->second));

    return res;
}

bool TinyConfig::Exists(const std::string & key) const
{
    return end() != find(ModifyKey(key));
}
