/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "castle.h"
#include "dialog.h"
#include "game_delays.h"
#include "icn.h"
#include "translations.h"
#include "ui_dialog.h"
#include "ui_text.h"
#include "world.h"

void Castle::OpenTavern() const
{
    std::string body( _( "A generous tip for the barkeep yields the following rumor:" ) );
    body += "\n\n";
    body += world.getCurrentRumor();

    const fheroes2::AnimationDialogElement imageUI( ICN::TAVWIN, { 0, 1 }, 0, Game::getAnimationDelayValue( Game::CASTLE_TAVERN_DELAY ) );
    const fheroes2::TextDialogElement textBodyUI( std::make_shared<fheroes2::Text>( std::move( body ), fheroes2::FontType::normalWhite() ) );

    fheroes2::showStandardTextMessage( GetStringBuilding( BUILD_TAVERN ), {}, Dialog::OK, { &imageUI, &textBodyUI } );
}
