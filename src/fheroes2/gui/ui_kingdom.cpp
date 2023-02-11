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
#include "tools.h"
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
        const std::string header( _( "Lighthouses" ) );
        const std::string body( _( "For every lighthouse controlled, your ships will move further each day." ) );

        Image combined( 128, 69 );
        combined.reset();

        // shadow top
        Blit( AGG::GetICN( ICN::OBJNMUL2, 60 ), combined, 2, 33 );
        // shadow middle
        Blit( AGG::GetICN( ICN::OBJNMUL2, 71 ), combined, 3, 37 );
        // shadow bottom
        Blit( AGG::GetICN( ICN::OBJNMUL2, 72 ), combined, 17, 39 );

        // lighthouse top
        Blit( AGG::GetICN( ICN::OBJNMUL2, 59 ), combined, 63, 0 );
        // lighthouse middle
        Blit( AGG::GetICN( ICN::OBJNMUL2, 61 ), combined, 57, 5 );
        // lighthouse bottom
        Blit( AGG::GetICN( ICN::OBJNMUL2, 73 ), combined, 49, 37 );
        // lighthouse light
        Blit( AGG::GetICN( ICN::OBJNMUL2, 62 ), combined, 61, 10 );

        const CustomImageDialogElement lighthouseImageElement( combined );

        std::string lighthouseControlledString = _( "%{count}" );
        StringReplace( lighthouseControlledString, "%{count}", std::to_string( lighthouseCount ) );
        const Text lighthouseControlledText( lighthouseControlledString, FontType::normalWhite() );
        const TextDialogElement lighthouseControlledElement( std::make_shared<Text>( lighthouseControlledText ) );

        showMessage( Text( header, FontType::normalYellow() ), Text( body, FontType::normalWhite() ), buttons,
                     { &lighthouseImageElement, &lighthouseControlledElement } );
    }
}
