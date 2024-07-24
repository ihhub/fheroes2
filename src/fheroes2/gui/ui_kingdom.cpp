/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2022 - 2024                                             *
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

#include "ui_kingdom.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>

#include "agg_image.h"
#include "game_delays.h"
#include "icn.h"
#include "image.h"
#include "kingdom.h"
#include "maps.h"
#include "mp2.h"
#include "translations.h"
#include "ui_dialog.h"
#include "ui_text.h"
#include "world.h"

namespace fheroes2
{
    void showKingdomIncome( const Kingdom & kingdom, const int buttons )
    {
        const Text header( _( "Kingdom Income" ), FontType::normalYellow() );
        const Text body( _( "Kingdom Income per day." ), FontType::normalWhite() );

        showResourceMessage( header, body, buttons, kingdom.GetIncome( Kingdom::INCOME_ALL ) );
    }

    void showLighthouseInfo( const Kingdom & kingdom, const int buttons )
    {
        const uint32_t lighthouseCount = world.CountCapturedObject( MP2::OBJ_LIGHTHOUSE, kingdom.GetColor() );

        const Sprite & shadowTop = AGG::GetICN( ICN::OBJNMUL2, 60 );
        const Sprite & shadowMiddle = AGG::GetICN( ICN::OBJNMUL2, 71 );
        const Sprite & shadowBottom = AGG::GetICN( ICN::OBJNMUL2, 72 );
        const Sprite & lighthouseTop = AGG::GetICN( ICN::OBJNMUL2, 59 );
        const Sprite & lighthouseMiddle = AGG::GetICN( ICN::OBJNMUL2, 61 );
        const Sprite & lighthouseBottom = AGG::GetICN( ICN::OBJNMUL2, 73 );
        const Sprite & lighthouseLight = AGG::GetICN( ICN::OBJNMUL2, 62 );

        const int32_t topOffset = TILEWIDTH - lighthouseTop.height();

        Image combined( 5 * TILEWIDTH, 2 * TILEWIDTH + lighthouseTop.height() );
        combined.reset();

        Copy( shadowTop, 0, 0, combined, shadowTop.x(), TILEWIDTH + shadowTop.y() - topOffset, TILEWIDTH, TILEWIDTH );
        Copy( shadowMiddle, 0, 0, combined, shadowMiddle.x(), TILEWIDTH * 2 + shadowMiddle.y() - topOffset, TILEWIDTH, TILEWIDTH );
        Copy( shadowBottom, 0, 0, combined, TILEWIDTH + shadowBottom.x(), TILEWIDTH * 2 + shadowBottom.y() - topOffset, TILEWIDTH, TILEWIDTH );
        Copy( lighthouseTop, 0, 0, combined, TILEWIDTH * 2 + lighthouseTop.x(), lighthouseTop.y() - topOffset, TILEWIDTH, TILEWIDTH );
        Copy( lighthouseMiddle, 0, 0, combined, TILEWIDTH * 2 + lighthouseMiddle.x(), TILEWIDTH + lighthouseMiddle.y() - topOffset, TILEWIDTH, TILEWIDTH );
        Copy( lighthouseBottom, 0, 0, combined, TILEWIDTH * 2 + lighthouseBottom.x(), TILEWIDTH * 2 + lighthouseBottom.y() - topOffset, TILEWIDTH, TILEWIDTH );

        const TextDialogElement lighthouseControlledElement( std::make_shared<Text>( std::to_string( lighthouseCount ), FontType::normalWhite() ) );

        // Use MAPS_DELAY for animation delay since the lighthouse is a map object
        // 61 (0x3D) references the icn offset for the lighthouse animation in icn.cpp
        const CustomAnimationDialogElement lighthouseCustomDynamicImageElement( ICN::OBJNMUL2, combined,
                                                                                { TILEWIDTH * 2 + lighthouseLight.x(), TILEWIDTH + lighthouseLight.y() - topOffset }, 61,
                                                                                getAnimationDelayValue( Game::MAPS_DELAY ) );

        // StringObject on OBJ_LIGHTHOUSE with count 2 for the plural of lighthouse
        showStandardTextMessage( StringObject( MP2::OBJ_LIGHTHOUSE, 2 ), _( "For every lighthouse controlled, your ships will move further each day." ), buttons,
                                 { &lighthouseCustomDynamicImageElement, &lighthouseControlledElement } );
    }
}
