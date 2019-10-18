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

#ifndef TINYCONFIG_H
#define TINYCONFIG_H

#include <ostream>
#include <string>
#include <list>
#include <map>

class TinyConfig : protected std::multimap<std::string, std::string>
{
public:
    TinyConfig(char sep = '=', char com = ';');

    bool	Load(const std::string &);
    bool	Save(const std::string &) const;
    void	Clear(void);

    void	AddEntry(const std::string &, const std::string &, bool uniq = true);
    void	AddEntry(const std::string &, int, bool uniq = true);

    bool	Exists(const std::string &) const;

    int		IntParams(const std::string &) const;
    std::string StrParams(const std::string &) const;

    std::list<std::string> ListStr(const std::string &) const;
    std::list<int>         ListInt(const std::string &) const;

protected:
    char	separator;
    char	comment;
};

#endif
