/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2022                                                    *
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

#include "ui_monster.h"

#include "agg_image.h"
#include "icn.h"
#include "image.h"
#include "monster.h"
#include "race.h"

namespace fheroes2
{
    void renderMonsterFrame( const Monster & monster, Image & output, const Point & offset )
    {
        switch ( monster.GetRace() ) {
        case Race::KNGT:
            Blit( AGG::GetICN( ICN::STRIP, 4 ), output, offset.x, offset.y );
            break;
        case Race::BARB:
            Blit( AGG::GetICN( ICN::STRIP, 5 ), output, offset.x, offset.y );
            break;
        case Race::SORC:
            Blit( AGG::GetICN( ICN::STRIP, 6 ), output, offset.x, offset.y );
            break;
        case Race::WRLK:
            Blit( AGG::GetICN( ICN::STRIP, 7 ), output, offset.x, offset.y );
            break;
        case Race::WZRD:
            Blit( AGG::GetICN( ICN::STRIP, 8 ), output, offset.x, offset.y );
            break;
        case Race::NECR:
            Blit( AGG::GetICN( ICN::STRIP, 9 ), output, offset.x, offset.y );
            break;
        default:
            Blit( AGG::GetICN( ICN::STRIP, 10 ), output, offset.x, offset.y );
            break;
        }

        const fheroes2::Sprite & monsterImage = fheroes2::AGG::GetICN( monster.ICNMonh(), 0 );
        fheroes2::Blit( monsterImage, output, offset.x + monsterImage.x(), offset.y + monsterImage.y() );
    }
}
