/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2025                                                    *
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

#include "ui_mage_guild.h"

#include <cassert>

#include "agg_image.h"
#include "screen.h"
#include "icn.h"
#include "image.h"
#include "pal.h"
#include "race.h"

namespace fheroes2
{
    void renderMageGuildBuilding( const int raceType, const int guildLevel, const Point offset )
    {
        int guildIcn = ICN::UNKNOWN;
        switch ( raceType ) {
        case Race::KNGT:
            guildIcn = ICN::MAGEGLDK;
            break;
        case Race::BARB:
            guildIcn = ICN::MAGEGLDB;
            break;
        case Race::SORC:
            guildIcn = ICN::MAGEGLDS;
            break;
        case Race::WRLK:
            guildIcn = ICN::MAGEGLDW;
            break;
        case Race::RAND:
        case Race::WZRD:
            guildIcn = ICN::MAGEGLDZ;
            break;
        case Race::NECR:
            guildIcn = ICN::MAGEGLDN;
            break;
        default:
            assert( 0 );
            break;
        }

        assert( guildLevel >= 1 && guildLevel <= 5 );
        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( guildIcn, guildLevel - 1 );

        const fheroes2::Rect area = fheroes2::GetActiveROI( sprite );

        fheroes2::Point inPos( 0, 0 );
        fheroes2::Point outPos( offset.x + 100 - area.x - area.width / 2, offset.y + 290 - sprite.height() );
        fheroes2::Size inSize( sprite.width(), sprite.height() );

        auto & display = Display::instance();

        if ( fheroes2::FitToRoi( sprite, inPos, display, outPos, inSize, { offset.x, offset.y, 200, fheroes2::Display::DEFAULT_HEIGHT } ) ) {
            if ( raceType == Race::RAND ) {
                fheroes2::Sprite guildSprite = sprite;
                fheroes2::ApplyPalette( guildSprite, PAL::GetPalette( PAL::PaletteType::PURPLE ) );
                fheroes2::Blit( guildSprite, inPos, display, outPos, inSize );
            }
            else {
                fheroes2::Blit( sprite, inPos, display, outPos, inSize );
            }
        }
    }
}
