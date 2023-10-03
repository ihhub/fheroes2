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

void fheroes2::drawMiniMonsters( const Troops & troops, int32_t cx, const int32_t cy, const uint32_t width, uint32_t first, uint32_t count, const bool isCompact,
                                 const bool isDetailedView, const bool isGarrisonView, const uint32_t thievesGuildsCount, Image & output )
{
    if ( !troops.isValid() ) {
        return;
    }

    if ( 0 == count ) {
        count = troops.GetOccupiedSlotCount();
    }

    int slotsToSkip = 1;
    if ( count <= 2 ) {
        slotsToSkip = 2;
    }
    else if ( count >= 7 ) {
        slotsToSkip = 0;
    }

    int marginLeft = 18;
    const int chunk = ( width - marginLeft ) / (count + ( slotsToSkip * 2 ) );
    Troops reversedTroops = troops.GetReversed();
    const size_t slots = reversedTroops.Size() + slotsToSkip;

    if ( !isCompact ) {
        cx -= chunk / 2;
        cx += width;
    }

    const int slotOffset = slotsToSkip;

    for ( size_t slot = 0; slot < slots; ++slot ) {
        //Skip the monster handling when a slot is empty
        if ( slotsToSkip > 0 ) {
            slotsToSkip--;
            cx -= chunk;
            continue;
        }
 
        const Troop * troop = reversedTroops.GetTroop( slot - slotOffset );

        if ( troop == nullptr || !troop->isValid() ) {
            continue;
        }
        if ( first != 0 || count == 0 ) {
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
        fheroes2::Text text( std::move( monstersCountRepresentation ), fheroes2::FontType::smallWhite() );

        // This is the drawing of army troops in compact form in the small info window beneath resources
        if ( isCompact ) {
            const int offsetY = ( monster.height() < 37 ) ? 37 - monster.height() : 0;
            int offset = ( chunk - monster.width() - text.width() ) / 2;
            if ( offset < 0 )
                offset = 0;
            fheroes2::Blit( monster, output, cx + offset, cy + offsetY + monster.y() );
            text.draw( cx + chunk - text.width() - offset, cy + 23, output );
        }
        else {
            if ( slotsToSkip == 0 ) {
                const int offsetY = 28 - monster.height();
                int OffsetX = -14;    

                //Center the monster if there is only one
                if ( count == 1 && slot == 1 ) {
                    OffsetX = -10;
                }

                int x = ( cx - ( monster.width() / 2 ) ) + OffsetX;
                int y = cy + offsetY + monster.y(); 
                fheroes2::Blit( monster, output, x, y);
                text.draw( ( cx - text.width() / 2 ) +  OffsetX , cy + 29, output );
            }
        }
        cx -= chunk;
        --count;
    }
}
