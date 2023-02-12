/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2022 - 2023                                             *
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
#include "icn.h"
#include "image.h"
#include "kingdom.h"
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
        const std::string body( _( "For every lighthouse controlled, your ships will move further each day." ) );

        const Sprite & shadow_top = AGG::GetICN( ICN::OBJNMUL2, 60 );
        const Sprite & shadow_middle = AGG::GetICN( ICN::OBJNMUL2, 71 );
        const Sprite & shadow_bottom = AGG::GetICN( ICN::OBJNMUL2, 72 );
        const Sprite & lighthouse_top = AGG::GetICN( ICN::OBJNMUL2, 59 );
        const Sprite & lighthouse_middle = AGG::GetICN( ICN::OBJNMUL2, 61 );
        const Sprite & lighthouse_bottom = AGG::GetICN( ICN::OBJNMUL2, 73 );
        const Sprite & lighthouse_light = AGG::GetICN( ICN::OBJNMUL2, 62 );

        const int32_t top_offset = TILEWIDTH - lighthouse_top.height();

        Image combined( 5 * TILEWIDTH, 2 * TILEWIDTH + lighthouse_top.height() );
        combined.reset();

        Copy( shadow_top, 0, 0, combined, shadow_top.x(), TILEWIDTH + shadow_top.y() - top_offset, TILEWIDTH, TILEWIDTH );
        Blit( shadow_middle, combined, shadow_middle.x(), TILEWIDTH * 2 + shadow_middle.y() - top_offset );
        Blit( shadow_bottom, combined, shadow_bottom.x() + TILEWIDTH, TILEWIDTH * 2 + shadow_bottom.y() - top_offset );
        Blit( lighthouse_top, combined, lighthouse_top.x() + TILEWIDTH * 2, lighthouse_top.y() - top_offset );
        Blit( lighthouse_middle, combined, lighthouse_middle.x() + TILEWIDTH * 2, TILEWIDTH + lighthouse_middle.y() - top_offset );
        Blit( lighthouse_bottom, combined, lighthouse_bottom.x() + TILEWIDTH * 2, TILEWIDTH * 2 + lighthouse_bottom.y() - top_offset );
        Blit( lighthouse_light, combined, lighthouse_light.x() + TILEWIDTH * 2, TILEWIDTH + lighthouse_light.y() - top_offset );

        const CustomImageDialogElement lighthouseImageElement( combined );

        const Text lighthouseControlledText( std::to_string( lighthouseCount ), FontType::normalWhite() );
        const TextDialogElement lighthouseControlledElement( std::make_shared<Text>( lighthouseControlledText ) );

        showMessage( Text( StringObject( MP2::OBJ_LIGHTHOUSE, 2 ), FontType::normalYellow() ), Text( body, FontType::normalWhite() ), buttons,
                     { &lighthouseImageElement, &lighthouseControlledElement } );
    }
}
