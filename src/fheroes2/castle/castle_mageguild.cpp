/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
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

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <vector>

#include "agg_image.h"
#include "castle.h"
#include "cursor.h"
#include "dialog.h"
#include "game_hotkeys.h"
#include "heroes.h"
#include "icn.h"
#include "image.h"
#include "localevent.h"
#include "mageguild.h"
#include "math_base.h"
#include "race.h"
#include "screen.h"
#include "settings.h"
#include "spell.h"
#include "spell_storage.h"
#include "text.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_text.h"

namespace
{
    const int32_t bottomBarOffsetY = 461;
    const int32_t exitButtonOffsetX = 578;

    class RowSpells
    {
    public:
        RowSpells( const fheroes2::Point & pos, const Castle & castle, const int lvl );
        void Redraw( fheroes2::Image & output );
        bool QueueEventProcessing();

    private:
        std::vector<fheroes2::Rect> coords;
        SpellStorage spells;
    };
}

RowSpells::RowSpells( const fheroes2::Point & pos, const Castle & castle, const int lvl )
{
    const bool hide = castle.GetLevelMageGuild() < lvl;
    const fheroes2::Sprite & roll_show = fheroes2::AGG::GetICN( ICN::TOWNWIND, 0 );
    const fheroes2::Sprite & roll_hide = fheroes2::AGG::GetICN( ICN::TOWNWIND, 1 );
    const fheroes2::Sprite & roll = ( hide ? roll_hide : roll_show );

    int32_t count = 0;

    switch ( lvl ) {
    case 1:
    case 2:
        count = 3;
        break;
    case 3:
    case 4:
        count = 2;
        break;
    case 5:
        count = 1;
        break;
    default:
        break;
    }

    for ( int32_t i = 0; i < count; ++i )
        coords.emplace_back( pos.x + i * 110 - roll.width() / 2, pos.y, roll.width(), roll.height() );

    if ( castle.HaveLibraryCapability() ) {
        if ( !hide && castle.isLibraryBuild() )
            coords.emplace_back( pos.x + count * 110 - roll_show.width() / 2, pos.y, roll_show.width(), roll_show.height() );
        else
            coords.emplace_back( pos.x + count * 110 - roll_hide.width() / 2, pos.y, roll_hide.width(), roll_hide.height() );
    }

    spells.reserve( 6 );
    spells = castle.GetMageGuild().GetSpells( castle.GetLevelMageGuild(), castle.isLibraryBuild(), lvl );
    spells.resize( coords.size(), Spell::NONE );
}

void RowSpells::Redraw( fheroes2::Image & output )
{
    const fheroes2::Sprite & roll_show = fheroes2::AGG::GetICN( ICN::TOWNWIND, 0 );

    for ( std::vector<fheroes2::Rect>::iterator it = coords.begin(); it != coords.end(); ++it ) {
        const fheroes2::Rect & dst = ( *it );
        const Spell & spell = spells[std::distance( coords.begin(), it )];

        // roll hide
        if ( dst.width < roll_show.width() || spell == Spell::NONE ) {
            const fheroes2::Sprite & roll_hide = fheroes2::AGG::GetICN( ICN::TOWNWIND, 1 );
            fheroes2::Blit( roll_hide, output, dst.x, dst.y );
        }
        // roll show
        else {
            fheroes2::Blit( roll_show, output, dst.x, dst.y );

            const fheroes2::Sprite & icon = fheroes2::AGG::GetICN( ICN::SPELLS, spell.IndexSprite() );
            fheroes2::Blit( icon, output, dst.x + 3 + ( dst.width - icon.width() ) / 2, dst.y + 31 - icon.height() / 2 );

            TextBox text( spell.GetName(), Font::SMALL, 78 );
            text.Blit( dst.x + 18, dst.y + 55 );
        }
    }
}

bool RowSpells::QueueEventProcessing()
{
    LocalEvent & le = LocalEvent::Get();

    const int32_t index = GetRectIndex( coords, le.GetMouseCursor() );

    if ( 0 <= index && ( le.MouseClickLeft() || le.MousePressRight() ) ) {
        const Spell & spell = spells[index];

        if ( spell != Spell::NONE ) {
            fheroes2::SpellDialogElement( spell, nullptr ).showPopup( le.MousePressRight() ? Dialog::ZERO : Dialog::OK );
        }
    }

    return 0 <= index;
}

void Castle::OpenMageGuild( const Heroes * hero ) const
{
    fheroes2::Display & display = fheroes2::Display::instance();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    const fheroes2::ImageRestorer restorer( display, ( display.width() - fheroes2::Display::DEFAULT_WIDTH ) / 2,
                                            ( display.height() - fheroes2::Display::DEFAULT_HEIGHT ) / 2, fheroes2::Display::DEFAULT_WIDTH,
                                            fheroes2::Display::DEFAULT_HEIGHT );

    const fheroes2::Point cur_pt( restorer.x(), restorer.y() );
    fheroes2::Point dst_pt( cur_pt.x, cur_pt.y );

    const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

    fheroes2::Blit( fheroes2::AGG::GetICN( isEvilInterface ? ICN::STONEBAK_EVIL : ICN::STONEBAK, 0 ), display, cur_pt.x, cur_pt.y );

    // The original ICN::WELLEXTRA image does not have a yellow outer frame.
    const int32_t allowedBottomBarWidth = exitButtonOffsetX;
    const fheroes2::Sprite & bottomBar = fheroes2::AGG::GetICN( ICN::SMALLBAR, 0 );

    // ICN::SMALLBAR image's first column contains all black pixels. This should not be drawn.
    fheroes2::Blit( bottomBar, 1, 0, display, cur_pt.x, cur_pt.y + bottomBarOffsetY, allowedBottomBarWidth / 2, bottomBar.height() );
    fheroes2::Blit( bottomBar, bottomBar.width() - ( allowedBottomBarWidth - allowedBottomBarWidth / 2 ) - 1, 0, display, cur_pt.x + allowedBottomBarWidth / 2,
                    cur_pt.y + bottomBarOffsetY, allowedBottomBarWidth - allowedBottomBarWidth / 2, bottomBar.height() );

    // text bar
    Text text;
    if ( hero == nullptr || !hero->HaveSpellBook() ) {
        text.Set( _( "The above spells are available here." ), Font::BIG );
    }
    else {
        text.Set( _( "The above spells have been added to your book." ), Font::BIG );
    }
    text.Blit( cur_pt.x + 280 - text.w() / 2, cur_pt.y + 463 );

    const int level = GetLevelMageGuild();
    // sprite
    int icn = ICN::UNKNOWN;
    switch ( race ) {
    case Race::KNGT:
        icn = ICN::MAGEGLDK;
        break;
    case Race::BARB:
        icn = ICN::MAGEGLDB;
        break;
    case Race::SORC:
        icn = ICN::MAGEGLDS;
        break;
    case Race::WRLK:
        icn = ICN::MAGEGLDW;
        break;
    case Race::WZRD:
        icn = ICN::MAGEGLDZ;
        break;
    case Race::NECR:
        icn = ICN::MAGEGLDN;
        break;
    default:
        break;
    }
    const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( icn, level - 1 );
    const fheroes2::Rect area = fheroes2::GetActiveROI( sprite );

    fheroes2::Point inPos( 0, 0 );
    fheroes2::Point outPos( cur_pt.x + 100 - area.x - area.width / 2, cur_pt.y + 290 - sprite.height() );
    fheroes2::Size inSize( sprite.width(), sprite.height() );

    if ( fheroes2::FitToRoi( sprite, inPos, display, outPos, inSize, { cur_pt.x, cur_pt.y, 200, fheroes2::Display::DEFAULT_HEIGHT } ) ) {
        fheroes2::Blit( sprite, inPos, display, outPos, inSize );
    }

    RowSpells spells5( { cur_pt.x + 250, cur_pt.y + 5 }, *this, 5 );
    RowSpells spells4( { cur_pt.x + 250, cur_pt.y + 95 }, *this, 4 );
    RowSpells spells3( { cur_pt.x + 250, cur_pt.y + 185 }, *this, 3 );
    RowSpells spells2( { cur_pt.x + 250, cur_pt.y + 275 }, *this, 2 );
    RowSpells spells1( { cur_pt.x + 250, cur_pt.y + 365 }, *this, 1 );

    spells1.Redraw( display );
    spells2.Redraw( display );
    spells3.Redraw( display );
    spells4.Redraw( display );
    spells5.Redraw( display );

    fheroes2::Button buttonExit( cur_pt.x + exitButtonOffsetX, cur_pt.y + bottomBarOffsetY, ICN::WELLXTRA, 0, 1 );
    buttonExit.draw();

    display.render();

    LocalEvent & le = LocalEvent::Get();

    // message loop
    while ( le.HandleEvents() ) {
        le.MousePressLeft( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();

        if ( le.MouseClickLeft( buttonExit.area() ) || Game::HotKeyCloseWindow() )
            break;

        spells1.QueueEventProcessing() || spells2.QueueEventProcessing() || spells3.QueueEventProcessing() || spells4.QueueEventProcessing()
            || spells5.QueueEventProcessing();

        if ( le.MousePressRight( buttonExit.area() ) ) {
            fheroes2::Text header( _( "Exit" ), fheroes2::FontType::normalYellow() );
            fheroes2::Text body( _( "Exit this menu." ), fheroes2::FontType::normalWhite() );

            fheroes2::showMessage( header, body, 0 );
        }
    }
}
