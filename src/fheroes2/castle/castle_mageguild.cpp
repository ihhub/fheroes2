/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
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
#include <string>
#include <vector>

#include "agg.h"
#include "castle.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "mageguild.h"
#include "race.h"
#include "settings.h"
#include "text.h"

RowSpells::RowSpells( const Point & pos, const Castle & castle, int lvl )
{
    const MageGuild & guild = castle.GetMageGuild();
    bool hide = castle.GetLevelMageGuild() < lvl;
    const fheroes2::Sprite & roll_show = fheroes2::AGG::GetICN( ICN::TOWNWIND, 0 );
    const fheroes2::Sprite & roll_hide = fheroes2::AGG::GetICN( ICN::TOWNWIND, 1 );
    const fheroes2::Sprite & roll = ( hide ? roll_hide : roll_show );

    u32 count = 0;

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

    for ( u32 ii = 0; ii < count; ++ii )
        coords.push_back( Rect( pos.x + coords.size() * 110 - roll.width() / 2, pos.y, roll.width(), roll.height() ) );

    if ( castle.HaveLibraryCapability() ) {
        if ( !hide && castle.isLibraryBuild() )
            coords.push_back( Rect( pos.x + coords.size() * 110 - roll_show.width() / 2, pos.y, roll_show.width(), roll_show.height() ) );
        else
            coords.push_back( Rect( pos.x + coords.size() * 110 - roll_hide.width() / 2, pos.y, roll_hide.width(), roll_hide.height() ) );
    }

    spells.reserve( 6 );
    spells = guild.GetSpells( castle.GetLevelMageGuild(), castle.isLibraryBuild(), lvl );
    spells.resize( coords.size(), Spell::NONE );
}

void RowSpells::Redraw( void )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    const fheroes2::Sprite & roll_show = fheroes2::AGG::GetICN( ICN::TOWNWIND, 0 );
    const fheroes2::Sprite & roll_hide = fheroes2::AGG::GetICN( ICN::TOWNWIND, 1 );

    for ( Rects::iterator it = coords.begin(); it != coords.end(); ++it ) {
        const Rect & dst = ( *it );
        const Spell & spell = spells[std::distance( coords.begin(), it )];

        // roll hide
        if ( dst.w < roll_show.width() || spell == Spell::NONE ) {
            fheroes2::Blit( roll_hide, display, dst.x, dst.y );
        }
        // roll show
        else {
            fheroes2::Blit( roll_show, display, dst.x, dst.y );

            const fheroes2::Sprite & icon = fheroes2::AGG::GetICN( ICN::SPELLS, spell.IndexSprite() );

            fheroes2::Blit( icon, display, dst.x + 3 + ( dst.w - icon.width() ) / 2, dst.y + 31 - icon.height() / 2 );

            TextBox text( std::string( spell.GetName() ) + " [" + GetString( spell.SpellPoint( NULL ) ) + "]", Font::SMALL, 78 );
            text.Blit( dst.x + 18, dst.y + 55 );
        }
    }
}

bool RowSpells::QueueEventProcessing( void )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    LocalEvent & le = LocalEvent::Get();
    Cursor & cursor = Cursor::Get();

    const s32 index = coords.GetIndex( le.GetMouseCursor() );

    if ( 0 <= index && ( le.MouseClickLeft() || le.MousePressRight() ) ) {
        const Spell & spell = spells[index];

        if ( spell != Spell::NONE ) {
            cursor.Hide();
            Dialog::SpellInfo( spell, !le.MousePressRight() );
            cursor.Show();
            display.render();
        }
    }

    return 0 <= index;
}

void Castle::OpenMageGuild( const CastleHeroes & heroes )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    Cursor & cursor = Cursor::Get();
    cursor.Hide();

    Dialog::FrameBorder frameborder( Display::GetDefaultSize() );
    const Point & cur_pt = frameborder.GetArea();

    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::STONEBAK, 0 ), display, cur_pt.x, cur_pt.y );

    // bar
    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::WELLXTRA, 2 ), display, cur_pt.x, cur_pt.y + 461 );

    // text bar
    Text text;
    if ( ( !heroes.Guard() || !heroes.Guard()->HaveSpellBook() ) && ( !heroes.Guest() || !heroes.Guest()->HaveSpellBook() ) )
        text.Set( _( "The above spells are available here." ), Font::BIG );
    else
        text.Set( _( "The above spells have been added to your book." ), Font::BIG );
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
    fheroes2::Blit( sprite, display, cur_pt.x + 90 - sprite.width() / 2, cur_pt.y + 290 - sprite.height() );

    RowSpells spells5( Point( cur_pt.x + 250, cur_pt.y + 5 ), *this, 5 );
    RowSpells spells4( Point( cur_pt.x + 250, cur_pt.y + 95 ), *this, 4 );
    RowSpells spells3( Point( cur_pt.x + 250, cur_pt.y + 185 ), *this, 3 );
    RowSpells spells2( Point( cur_pt.x + 250, cur_pt.y + 275 ), *this, 2 );
    RowSpells spells1( Point( cur_pt.x + 250, cur_pt.y + 365 ), *this, 1 );

    spells1.Redraw();
    spells2.Redraw();
    spells3.Redraw();
    spells4.Redraw();
    spells5.Redraw();

    // button exit
    fheroes2::Button buttonExit( cur_pt.x + 578, cur_pt.y + 461, ICN::WELLXTRA, 0, 1 );
    buttonExit.draw();

    cursor.Show();
    display.render();

    LocalEvent & le = LocalEvent::Get();

    // message loop
    while ( le.HandleEvents() ) {
        le.MousePressLeft( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();

        if ( le.MouseClickLeft( buttonExit.area() ) || HotKeyCloseWindow )
            break;

        if ( spells1.QueueEventProcessing() || spells2.QueueEventProcessing() || spells3.QueueEventProcessing() || spells4.QueueEventProcessing()
             || spells5.QueueEventProcessing() ) {
        }
    }
}
