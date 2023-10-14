/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2023                                             *
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

#include "army_ui_helper.h"

#include <cassert>
#include <cstddef>
#include <string>
#include <utility>

#include "agg_image.h"
#include "army.h"
#include "army_troop.h"
#include "game.h"
#include "icn.h"
#include "image.h"
#include "ui_text.h"

void fheroes2::drawMiniMonsters( const Troops & troops, int32_t cx, const int32_t cy, const int32_t width, uint32_t first, uint32_t count, const bool isCompact,
                                 const bool isDetailedView, const bool isGarrisonView, const uint32_t thievesGuildsCount, Image & output )
{
    if ( !troops.isValid() ) {
        return;
    }

    bool isRightToLeftRender = false;

    if ( count == 0 ) {
        // If this function was called with 'count == 0' than the count is determined by the number of troop slots.
        count = troops.GetOccupiedSlotCount();

        if ( first == 0 ) {
            // This is the case when we render all the troops and do it from left to right.
            // It is done to make troop sprites overlapping more appealing when troops are close to each other.
            // This case may occur if many troops were killed during the battle (lots of summoned elementals and/or mirror image troops).
            isRightToLeftRender = true;
        }
    }

    const double chunk = width / static_cast<double>( count );

    if ( !isCompact ) {
        cx += static_cast<int32_t>( chunk / 2 );
    }
    if ( !isRightToLeftRender ) {
        cx += width;
    }

    const size_t slots = troops.Size();
    for ( size_t slot = 0; slot < slots; ++slot ) {
        if ( count == 0 ) {
            // We have rendered the given count of troops. There is nothing more to do.
            break;
        }

        const Troop * troop = troops.GetTroop( isRightToLeftRender ? ( slots - slot - 1 ) : slot );

        if ( troop == nullptr || !troop->isValid() ) {
            continue;
        }

        if ( first != 0 ) {
            --first;
            continue;
        }

        std::string monstersCountRepresentation;

        if ( isDetailedView || !isGarrisonView ) {
            monstersCountRepresentation = Game::formatMonsterCount( troop->GetCount(), isDetailedView, isCompact );
        }
        else {
            assert( thievesGuildsCount > 0 );

            if ( thievesGuildsCount == 1 ) {
                monstersCountRepresentation = "???";
            }
            else {
                monstersCountRepresentation = Army::SizeString( troop->GetCount() );
            }
        }

        const fheroes2::Sprite & monster = fheroes2::AGG::GetICN( ICN::MONS32, troop->GetSpriteIndex() );
        const fheroes2::Text text( std::move( monstersCountRepresentation ), fheroes2::FontType::smallWhite() );

        const int32_t posX = isRightToLeftRender ? ( cx + static_cast<int32_t>( chunk * ( count - 1 ) ) ) : ( cx - static_cast<int32_t>( chunk * count ) );

        // This is the drawing of army troops in compact form in the small info window beneath resources
        if ( isCompact ) {
            const int32_t offsetY = ( monster.height() < 37 ) ? 37 - monster.height() : 0;
            int32_t offset = ( static_cast<int32_t>( chunk ) - monster.width() - text.width() ) / 2;
            if ( offset < 0 ) {
                offset = 0;
            }
            fheroes2::Blit( monster, output, posX + offset, cy + offsetY + monster.y() );
            text.draw( posX - text.width() - offset + static_cast<int32_t>( chunk ), cy + 23, output );
        }
        else {
            const int32_t offsetY = 28 - monster.height() + monster.y();
            fheroes2::Blit( monster, output, posX - monster.width() / 2 + monster.x() + 2, cy + offsetY );
            text.draw( posX - text.width() / 2, cy + 29, output );
        }

        --count;
    }
}
