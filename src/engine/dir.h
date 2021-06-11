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
#ifndef H2DIR_H
#define H2DIR_H

#include <list>
#include <string>

struct ListFiles : public std::list<std::string>
{
    void Append( const ListFiles & files );
    void ReadDir( const std::string & path, const std::string & filter = "", bool sensitive = true );
    static bool IsEmpty( const std::string & path, const std::string & filter = "", bool sensitive = true );

    void FindFileInDir( const std::string & path, const std::string & fileName, bool sensitive );
};

struct ListDirs : public std::list<std::string>
{
    void Append( const std::list<std::string> & );
};

#endif
