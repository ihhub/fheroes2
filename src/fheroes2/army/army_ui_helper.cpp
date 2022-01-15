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

#include "army_ui_helper.h"
#include "agg_image.h"
#include "army.h"
#include "army_troop.h"
#include "game.h"
#include "icn.h"
#include "image.h"
#include "screen.h"
#include "ui_text.h"

void fheroes2::DrawMons32Line( const Troops & troops, int32_t cx, int32_t cy, uint32_t width, uint32_t first, uint32_t count, uint32_t drawPower, bool compact,
                               bool isScouteView )
{
    if ( !troops.isValid() ) {
        return;
    }

    if ( 0 == count ) {
        count = troops.GetCount();
    }

    const int chunk = width / count;
    if ( !compact ) {
        cx += chunk / 2;
    }

    for ( auto it = troops.begin(); it <= troops.end(); ++it ) {
        if ( ( *it )->isValid() ) {
            if ( 0 == first && count ) {
                const fheroes2::Sprite & monster = fheroes2::AGG::GetICN( ICN::MONS32, ( *it )->GetSpriteIndex() );
                fheroes2::Text text( isScouteView ? Game::CountScoute( ( *it )->GetCount(), drawPower, compact )
                                                  : Game::CountThievesGuild( ( *it )->GetCount(), drawPower ),
                                     { fheroes2::FontSize::SMALL, fheroes2::FontColor::WHITE } );
                if ( compact ) {
                    const int offsetY = ( monster.height() < 37 ) ? 37 - monster.height() : 0;
                    int offset = ( chunk - monster.width() - text.width() ) / 2;
                    if ( offset < 0 )
                        offset = 0;
                    fheroes2::Blit( monster, fheroes2::Display::instance(), cx + offset, cy + offsetY + monster.y() );
                    text.draw( cx + chunk - text.width() - offset, cy + 23, fheroes2::Display::instance() );
                }
                else {
                    const int offsetY = 28 - monster.height();
                    fheroes2::Blit( monster, fheroes2::Display::instance(), cx - monster.width() / 2 + monster.x() + 2, cy + offsetY + monster.y() );
                    text.draw( cx - text.width() / 2, cy + 29, fheroes2::Display::instance() );
                }
                cx += chunk;
                --count;
            }
            else
                --first;
        }
    }
}
