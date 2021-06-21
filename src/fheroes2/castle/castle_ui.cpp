/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2021                                                    *
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

#include "castle_ui.h"
#include "agg_image.h"
#include "castle.h"
#include "icn.h"
#include "image.h"
#include "logging.h"
#include "race.h"
#include "settings.h"

#include <cassert>

namespace fheroes2
{
    void drawCastleIcon( const Castle & castle, Image & output, const Point & offset )
    {
        uint32_t icnIndex = 1;

        switch ( castle.GetRace() ) {
        case Race::KNGT:
            icnIndex = castle.isCastle() ? 9 : 15;
            break;
        case Race::BARB:
            icnIndex = castle.isCastle() ? 10 : 16;
            break;
        case Race::SORC:
            icnIndex = castle.isCastle() ? 11 : 17;
            break;
        case Race::WRLK:
            icnIndex = castle.isCastle() ? 12 : 18;
            break;
        case Race::WZRD:
            icnIndex = castle.isCastle() ? 13 : 19;
            break;
        case Race::NECR:
            icnIndex = castle.isCastle() ? 14 : 20;
            break;
        default:
            assert( 0 );
            DEBUG_LOG( DBG_ENGINE, DBG_WARN, "unknown race" );
        }

        const Sprite & castleImage = fheroes2::AGG::GetICN( Settings::Get().ExtGameEvilInterface() ? ICN::LOCATORE : ICN::LOCATORS, icnIndex );
        fheroes2::Blit( castleImage, output, offset.x, offset.y );

        // Draw castle's marker.
        switch ( Castle::GetAllBuildingStatus( castle ) ) {
        case UNKNOWN_COND:
        case NOT_TODAY:
            fheroes2::Blit( fheroes2::AGG::GetICN( ICN::CSLMARKER, 0 ), output, offset.x + 40, offset.y );
            break;
        case REQUIRES_BUILD:
            fheroes2::Blit( fheroes2::AGG::GetICN( ICN::CSLMARKER, 1 ), output, offset.x + 40, offset.y );
            break;
        default:
            break;
        }
    }
}
