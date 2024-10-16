/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#ifndef H2AGG_H
#define H2AGG_H

#include <cstdint>
#include <string>
#include <vector>

namespace AGG
{
    class AGGInitializer
    {
    public:
        AGGInitializer();
        AGGInitializer( const AGGInitializer & ) = delete;
        AGGInitializer & operator=( const AGGInitializer & ) = delete;

        ~AGGInitializer() = default;

        const std::string & getOriginalAGGFilePath() const
        {
            return _originalAGGFilePath;
        }

        const std::string & getExpansionAGGFilePath() const
        {
            return _expansionAGGFilePath;
        }

    private:
        bool init();

        std::string _originalAGGFilePath;
        std::string _expansionAGGFilePath;
    };

    std::vector<uint8_t> getDataFromAggFile( const std::string & key, const bool ignoreExpansion );
}

#endif
