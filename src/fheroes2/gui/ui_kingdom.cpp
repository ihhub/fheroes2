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

#include "ui_kingdom.h"
#include "agg_image.h"
#include "icn.h"
#include "kingdom.h"
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
        const std::string body( _( "For every lighthouse controlled, your heroes moves further on water." ) );

        const CustomImageDialogElement lighthouseImageElement( AGG::GetICN( ICN::OBJNMUL2, 61 ) );
        const Text lighthouseControlledText( _( "Controlled: " + std::to_string( lighthouseCount ) ), FontType::smallWhite() );
        const TextDialogElement lighthouseControlledElement( std::make_shared<Text>( lighthouseControlledText ) );
        showMessage( Text( header, FontType::normalYellow() ), Text( body, FontType::normalWhite() ), buttons, { &lighthouseImageElement, &lighthouseControlledElement } );
    }
}
