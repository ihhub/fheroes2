/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2020                                                    *
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

#include "game_credits.h"
#include "agg.h"
#include "dialog.h"
#include "mus.h"
#include "screen.h"
#include "settings.h"
#include "text.h"

void Game::ShowCredits()
{
    fheroes2::Display & display = fheroes2::Display::instance();
    Dialog::FrameBorder border( Size( display.DEFAULT_WIDTH, display.DEFAULT_HEIGHT ) );

    const fheroes2::Point screenOffset( ( display.width() - display.DEFAULT_WIDTH ) / 2, ( display.height() - display.DEFAULT_HEIGHT ) / 2 );

    const fheroes2::Sprite & background = fheroes2::AGG::GetICN( ICN::CBKGLAVA, 0 );
    fheroes2::Blit( background, display, screenOffset.x, screenOffset.y + display.DEFAULT_HEIGHT - background.height() );

    fheroes2::Fill( display, screenOffset.x, screenOffset.y, display.DEFAULT_WIDTH, display.DEFAULT_HEIGHT - background.height(), 0 );

    Text caption( "Free Heroes of Might and Magic II (" + Settings::GetVersion() + ")", Font::YELLOW_BIG );
    caption.Blit( screenOffset.x + display.DEFAULT_WIDTH / 2 - caption.w() / 2, screenOffset.y + 15 );

    const int32_t columnStep = 210;
    const int32_t textInitialOffsetY = 40;
    const int32_t textWidth = 200;

    int32_t offsetY = screenOffset.y + textInitialOffsetY;

    TextBox title( "Project Coordination and Core Development", Font::YELLOW_BIG, textWidth );
    TextBox name( "Ihar Hubchyk", Font::BIG, textWidth );
    title.Blit( screenOffset.x + ( columnStep - title.w() ) / 2, offsetY );
    name.Blit( screenOffset.x + ( columnStep - name.w() ) / 2, offsetY + title.h() );
    offsetY += title.h() + name.h() + 10;

    const fheroes2::Sprite & blackDragon = fheroes2::AGG::GetICN( ICN::DRAGBLAK, 5 );
    fheroes2::Blit( blackDragon, display, screenOffset.x + ( columnStep - blackDragon.width() ) / 2, offsetY );
    offsetY += blackDragon.height();

    title.Set( "QA and Support", Font::YELLOW_BIG, textWidth );
    name.Set( "Igor Tsivilko", Font::BIG, textWidth );
    title.Blit( screenOffset.x + ( columnStep - title.w() ) / 2, offsetY );
    name.Blit( screenOffset.x + ( columnStep - name.w() ) / 2, offsetY + title.h() );
    offsetY += title.h() + name.h() + 10;

    const fheroes2::Sprite & cyclop = fheroes2::AGG::GetICN( ICN::CYCLOPS, 38 );
    fheroes2::Blit( cyclop, display, screenOffset.x + ( columnStep - cyclop.width() ) / 2, offsetY );
    offsetY += cyclop.height();

    title.Set( "Development", Font::YELLOW_BIG, textWidth );
    name.Set( "Ivan Shibanov", Font::BIG, textWidth );
    title.Blit( screenOffset.x + ( columnStep - title.w() ) / 2, offsetY );
    name.Blit( screenOffset.x + ( columnStep - name.w() ) / 2, offsetY + title.h() );
    offsetY += title.h() + name.h() + 10;

    const fheroes2::Sprite & crusader = fheroes2::AGG::GetICN( ICN::PALADIN2, 23 );
    fheroes2::Blit( crusader, display, screenOffset.x + ( columnStep - crusader.width() ) / 2, offsetY );
    offsetY += crusader.height();

    offsetY += 10;

    const Text websiteInto( "Visit us at ", Font::BIG );
    const Text website( "https://github.com/ihhub/fheroes2", Font::YELLOW_BIG );
    const int32_t websiteOffsetX = screenOffset.x + ( display.DEFAULT_WIDTH - websiteInto.w() - website.w() ) / 2;
    websiteInto.Blit( websiteOffsetX, offsetY );
    website.Blit( websiteOffsetX + websiteInto.w(), offsetY );

    const fheroes2::Sprite & missile = fheroes2::AGG::GetICN( ICN::ARCH_MSL, 4 );
    fheroes2::Blit( missile, display, websiteOffsetX - 10 - missile.width(), offsetY + website.h() / 2 - missile.height() / 2 );
    fheroes2::Blit( missile, display, websiteOffsetX + websiteInto.w() + website.w() + 10, offsetY + website.h() / 2 - missile.height() / 2, true );

    offsetY = screenOffset.y + textInitialOffsetY;

    title.Set( "Special Thanks to", Font::YELLOW_BIG, textWidth );
    title.Blit( screenOffset.x + 2 * columnStep + ( columnStep - title.w() ) / 2, offsetY );
    offsetY += title.h();

    const std::string contributors( "LeHerosInconnu\nshprotru\nundef21\nAndrey Starodubtsev\neos428\nVasilenko Alexey\nvincent-grosbois\nemotionalamoeba\nOroty\n"
                                    "and many other contributors!" );

    name.Set( contributors, Font::BIG, textWidth );
    name.Blit( screenOffset.x + 2 * columnStep + ( columnStep - name.w() ) / 2, offsetY );
    offsetY += name.h() + 10;

    const fheroes2::Sprite & hydra = fheroes2::AGG::GetICN( ICN::HYDRA, 11 );
    fheroes2::Blit( hydra, display, screenOffset.x + 2 * columnStep + ( columnStep - hydra.width() ) / 2, offsetY );
    offsetY += hydra.height();

    title.Set( "Original project before 0.7", Font::YELLOW_SMALL, textWidth );
    title.Blit( screenOffset.x + 2 * columnStep + ( columnStep - title.w() ) / 2, offsetY );
    offsetY += title.h();

    name.Set( "Andrey Afletdinov\nhttps://sourceforge.net/\nprojects/fheroes2/", Font::SMALL, textWidth );
    name.Blit( screenOffset.x + 2 * columnStep + ( columnStep - name.w() ) / 2, offsetY );
    offsetY += name.h();

    const fheroes2::Sprite & goblin = fheroes2::AGG::GetICN( ICN::GOBLIN, 27 );
    fheroes2::Blit( goblin, display, screenOffset.x + ( display.DEFAULT_WIDTH - goblin.width() ) / 2, screenOffset.y + ( display.DEFAULT_HEIGHT - goblin.height() ) / 2 );

    AGG::PlayMusic( MUS::VICTORY );

    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents() ) {
        if ( le.KeyPress() || le.MouseClickLeft() || le.MouseClickMiddle() || le.MouseClickRight() )
            break;
    }
}
