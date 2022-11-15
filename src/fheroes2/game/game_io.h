/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
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

#ifndef H2GAMEIO_H
#define H2GAMEIO_H

#include <string>

#include "game_mode.h"

namespace Maps
{
    struct FileInfo;
}

namespace Game
{
    bool AutoSaveAtTheBeginningOfTheDay();
    bool AutoSaveAtTheEndOfTheDay();

    bool Save( const std::string & fn, const bool autoSave = false );

    // Returns GameMode::CANCEL in case of failure.
    fheroes2::GameMode Load( const std::string & fn );

    bool LoadSAV2FileInfo( const std::string &, Maps::FileInfo & );

    bool SaveCompletedCampaignScenario();
}

#endif
