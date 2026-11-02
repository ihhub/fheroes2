/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2026                                                    *
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

#include "game_tools.h"

#include "tools.h"
#include "translations.h"

namespace Game
{
    std::string getDateDescription( const int32_t day )
    {
        std::string message = _( "Day: %{day} Week: %{week} Month: %{month}" );
        int32_t days = day - 1;
        const int32_t month = days / ( 7 * 4 );
        days -= month * ( 7 * 4 );

        StringReplace( message, "%{day}", ( days % 7 ) + 1 );
        StringReplace( message, "%{week}", ( days / 7 ) + 1 );
        StringReplace( message, "%{month}", month + 1 );

        return message;
    }
}
