/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
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

#include "agg_image.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "icn.h"
#include "interface_list.h"
#include "localevent.h"
#include "pal.h"
#include "settings.h"
#include "text.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_window.h"

class SettingsListBox : public Interface::ListBox<u32>
{
public:
    SettingsListBox( const fheroes2::Point & pt, bool f )
        : Interface::ListBox<u32>( pt )
        , readonly( f )
        , _restorer( fheroes2::Display::instance() )
    {}

    void RedrawItem( const u32 &, s32, s32, bool ) override;
    void RedrawBackground( const fheroes2::Point & ) override;

    void ActionCurrentUp() override
    {
        // Do nothing.
    }

    void ActionCurrentDn() override
    {
        // Do nothing.
    }

    void ActionListDoubleClick( u32 & ) override;

    void ActionListSingleClick( u32 & ) override;

    void ActionListPressRight( u32 & ) override
    {
        // Do nothing.
    }

    bool readonly;
    fheroes2::ImageRestorer _restorer;
};

void SettingsListBox::RedrawItem( const u32 & item, s32 ox, s32 oy, bool /*current*/ )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    const Settings & conf = Settings::Get();

    const bool isActive = !readonly || conf.CanChangeInGame( item );

    const fheroes2::Sprite & cell = fheroes2::AGG::GetICN( ICN::CELLWIN, 1 );
    const fheroes2::Sprite & mark = fheroes2::AGG::GetICN( ICN::CELLWIN, 2 );

    fheroes2::Blit( cell, display, ox, oy );
    if ( conf.ExtModes( item ) )
        fheroes2::Blit( mark, display, ox + 3, oy + 2 );

    if ( !isActive ) {
        // Gray out the option icon.
        fheroes2::ApplyPalette( display, ox, oy, display, ox, oy, cell.width(), cell.height(), PAL::GetPalette( PAL::PaletteType::GRAY ) );
    }

    TextBox msg( Settings::ExtName( item ), isActive ? Font::SMALL : Font::GRAY_SMALL, 250 );
    msg.SetAlign( ALIGN_LEFT );

    if ( 1 < msg.row() )
        msg.Blit( ox + cell.width() + 5, oy - 1 );
    else
        msg.Blit( ox + cell.width() + 5, oy + 4 );
}

void SettingsListBox::RedrawBackground( const fheroes2::Point & origin )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    const int windowHeight = 400;
    const int scrollbarHeight = windowHeight - 64 - 2 * 16; // up/down buttons are 16px tall

    _restorer.restore();

    const fheroes2::Image & sprite = fheroes2::AGG::GetICN( ICN::ESCROLL, 1 );
    fheroes2::Blit( sprite, 0, 0, display, origin.x + 295, origin.y + 41, 16, scrollbarHeight / 2 );
    fheroes2::Blit( sprite, 0, sprite.height() - scrollbarHeight / 2, display, origin.x + 295, origin.y + 41 + scrollbarHeight / 2, 16, scrollbarHeight / 2 );
}

void SettingsListBox::ActionListDoubleClick( u32 & item )
{
    ActionListSingleClick( item );
}

void SettingsListBox::ActionListSingleClick( u32 & item )
{
    Settings & conf = Settings::Get();

    if ( !readonly || conf.CanChangeInGame( item ) ) {
        conf.ExtModes( item ) ? conf.ExtResetModes( item ) : conf.ExtSetModes( item );
    }
}

void Dialog::ExtSettings( bool readonly )
{
    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    const fheroes2::StandardWindow frameborder( 320, 400 );
    const fheroes2::Rect area( frameborder.activeArea() );

    Text text( _( "Experimental Game Settings" ), Font::YELLOW_BIG );
    text.Blit( area.x + ( area.width - text.w() ) / 2, area.y + 6 );

    std::vector<u32> states;
    states.reserve( 32 );

    states.push_back( Settings::GAME_SAVE_REWRITE_CONFIRM );
    states.push_back( Settings::GAME_REMEMBER_LAST_FOCUS );
    states.push_back( Settings::GAME_SHOW_SYSTEM_INFO );
    states.push_back( Settings::GAME_BATTLE_SHOW_DAMAGE );
    states.push_back( Settings::GAME_AUTOSAVE_BEGIN_DAY );
    states.push_back( Settings::GAME_CONTINUE_AFTER_VICTORY );
    states.push_back( Settings::WORLD_SHOW_TERRAIN_PENALTY );
    states.push_back( Settings::WORLD_ALLOW_SET_GUARDIAN );
    states.push_back( Settings::WORLD_EXT_OBJECTS_CAPTURED );
    states.push_back( Settings::WORLD_SCOUTING_EXTENDED );
    states.push_back( Settings::WORLD_ARTIFACT_CRYSTAL_BALL );
    states.push_back( Settings::WORLD_SCALE_NEUTRAL_ARMIES );
    states.push_back( Settings::HEROES_BUY_BOOK_FROM_SHRINES );
    states.push_back( Settings::HEROES_REMEMBER_POINTS_RETREAT );
    states.push_back( Settings::HEROES_TRANSCRIBING_SCROLLS );
    states.push_back( Settings::HEROES_ARENA_ANY_SKILLS );
    states.push_back( Settings::CASTLE_ALLOW_GUARDIANS );
    states.push_back( Settings::BATTLE_SOFT_WAITING );
    states.push_back( Settings::BATTLE_REVERSE_WAIT_ORDER );
    states.push_back( Settings::BATTLE_DETERMINISTIC_RESULT );

    std::sort( states.begin(), states.end(), []( uint32_t first, uint32_t second ) { return Settings::ExtName( first ) > Settings::ExtName( second ); } );

    SettingsListBox listbox( area.getPosition(), readonly );

    listbox._restorer.update( area.x + 15, area.y + 25, 280, 336 );

    const int ah = 340;

    listbox.RedrawBackground( area.getPosition() );
    listbox.SetScrollButtonUp( ICN::ESCROLL, 4, 5, { area.x + 295, area.y + 25 } );
    listbox.SetScrollButtonDn( ICN::ESCROLL, 6, 7, { area.x + 295, area.y + ah + 5 } );

    const fheroes2::Sprite & originalSilder = fheroes2::AGG::GetICN( ICN::ESCROLL, 3 );
    const fheroes2::Image scrollbarSlider
        = fheroes2::generateScrollbarSlider( originalSilder, false, ah - 42, ah / 40, static_cast<int32_t>( states.size() ), { 0, 0, originalSilder.width(), 8 },
                                             { 0, 7, originalSilder.width(), 8 } );

    listbox.setScrollBarArea( { area.x + 298, area.y + 44, 10, ah - 42 } );
    listbox.setScrollBarImage( scrollbarSlider );
    listbox.SetAreaMaxItems( ah / 40 );
    listbox.SetAreaItems( fheroes2::Rect( area.x + 10, area.y + 30, 290, ah + 5 ) );
    listbox.SetListContent( states );
    listbox.Redraw();

    const fheroes2::Rect buttonsArea( area.x + 5, area.y, area.width - 10, area.height - 5 );

    const int buttonIcnId = Settings::Get().ExtGameEvilInterface() ? ICN::SPANBTNE : ICN::SPANBTN;
    const fheroes2::Sprite & buttonSprite = fheroes2::AGG::GetICN( buttonIcnId, 0 );

    fheroes2::Display & display = fheroes2::Display::instance();

    fheroes2::ButtonSprite buttonOk
        = fheroes2::makeButtonWithShadow( buttonsArea.x + ( buttonsArea.width - buttonSprite.width() ) / 2, buttonsArea.y + buttonsArea.height - buttonSprite.height(),
                                          fheroes2::AGG::GetICN( buttonIcnId, 0 ), fheroes2::AGG::GetICN( buttonIcnId, 1 ), display );
    buttonOk.draw();

    display.render();

    // message loop
    LocalEvent & le = LocalEvent::Get();
    while ( le.HandleEvents() ) {
        le.MousePressLeft( buttonOk.area() ) ? buttonOk.drawOnPress() : buttonOk.drawOnRelease();

        if ( le.MouseClickLeft( buttonOk.area() ) || HotKeyCloseWindow ) {
            break;
        }

        listbox.QueueEventProcessing();

        if ( !listbox.IsNeedRedraw() ) {
            continue;
        }

        listbox.Redraw();
        display.render();
    }

    Settings::Get().BinarySave();
}
