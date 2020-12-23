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

#include "agg.h"
#include "cursor.h"
#include "dialog.h"
#include "interface_list.h"
#include "settings.h"
#include "text.h"
#include "ui_button.h"

class SettingsListBox : public Interface::ListBox<u32>
{
public:
    SettingsListBox( const Point & pt, bool f )
        : Interface::ListBox<u32>( pt )
        , readonly( f )
        , _restorer( fheroes2::Display::instance() )
    {}

    virtual void RedrawItem( const u32 &, s32, s32, bool ) override;
    virtual void RedrawBackground( const Point & ) override;

    virtual void ActionCurrentUp( void ) override {}
    virtual void ActionCurrentDn( void ) override {}
    virtual void ActionListDoubleClick( u32 & ) override;
    virtual void ActionListSingleClick( u32 & ) override;
    virtual void ActionListPressRight( u32 & ) override {}

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

    TextBox msg( conf.ExtName( item ), isActive ? Font::SMALL : Font::GRAY_SMALL, 250 );
    msg.SetAlign( ALIGN_LEFT );

    if ( 1 < msg.row() )
        msg.Blit( ox + cell.width() + 5, oy - 1 );
    else
        msg.Blit( ox + cell.width() + 5, oy + 4 );
}

void SettingsListBox::RedrawBackground( const Point & origin )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    const int window_h = 400;
    const int ah = window_h - 54;

    _restorer.restore();

    for ( int ii = 1; ii < ( window_h / 25 ); ++ii )
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::DROPLISL, 11 ), display, origin.x + 295, origin.y + 35 + ( 19 * ii ) );

    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::DROPLISL, 10 ), display, origin.x + 295, origin.y + 46 );

    const fheroes2::Sprite & lowerPart = fheroes2::AGG::GetICN( ICN::DROPLISL, 12 );
    fheroes2::Blit( lowerPart, display, origin.x + 295, origin.y + ah - lowerPart.height() );
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

        // depends
        switch ( item ) {
        case Settings::WORLD_1HERO_HIRED_EVERY_WEEK:
            conf.ExtResetModes( Settings::CASTLE_1HERO_HIRED_EVERY_WEEK );
            break;

        case Settings::CASTLE_1HERO_HIRED_EVERY_WEEK:
            conf.ExtResetModes( Settings::WORLD_1HERO_HIRED_EVERY_WEEK );
            break;

        case Settings::GAME_AUTOSAVE_BEGIN_DAY:
            if ( conf.ExtModes( Settings::GAME_AUTOSAVE_BEGIN_DAY ) )
                conf.ExtSetModes( Settings::GAME_AUTOSAVE_ON );
            else
                conf.ExtResetModes( Settings::GAME_AUTOSAVE_ON );
            break;

        case Settings::WORLD_NEW_VERSION_WEEKOF:
            if ( conf.ExtModes( Settings::WORLD_NEW_VERSION_WEEKOF ) )
                conf.ExtSetModes( Settings::WORLD_BAN_WEEKOF );
            else
                conf.ExtResetModes( Settings::WORLD_BAN_WEEKOF );
            break;

        default:
            break;
        }
    }
}

void Dialog::ExtSettings( bool readonly )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    const Settings & conf = Settings::Get();

    // cursor
    Cursor & cursor = Cursor::Get();
    cursor.Hide();
    cursor.SetThemes( cursor.POINTER );

    Dialog::FrameBorder frameborder( Size( 320, 400 ) );
    const Rect & area = frameborder.GetArea();

    Text text( "Experimental Game Settings", Font::YELLOW_BIG );
    text.Blit( area.x + ( area.w - text.w() ) / 2, area.y + 6 );

    std::vector<u32> states;
    states.reserve( 64 );

    states.push_back( Settings::GAME_SAVE_REWRITE_CONFIRM );
    states.push_back( Settings::GAME_REMEMBER_LAST_FOCUS );
    states.push_back( Settings::GAME_SHOW_SYSTEM_INFO );
    states.push_back( Settings::GAME_EVIL_INTERFACE );
    states.push_back( Settings::GAME_BATTLE_SHOW_DAMAGE );

    states.push_back( Settings::GAME_HIDE_INTERFACE );

    if ( !conf.PocketPC() )
        states.push_back( Settings::GAME_DYNAMIC_INTERFACE );

    states.push_back( Settings::GAME_AUTOSAVE_ON );
    states.push_back( Settings::GAME_AUTOSAVE_BEGIN_DAY );

    if ( conf.VideoMode() == Size( fheroes2::Display::DEFAULT_WIDTH, fheroes2::Display::DEFAULT_HEIGHT ) )
        states.push_back( Settings::GAME_USE_FADE );

    states.push_back( Settings::GAME_CONTINUE_AFTER_VICTORY );
    states.push_back( Settings::WORLD_SHOW_VISITED_CONTENT );
    states.push_back( Settings::WORLD_ABANDONED_MINE_RANDOM );
    states.push_back( Settings::WORLD_ALLOW_SET_GUARDIAN );
    states.push_back( Settings::WORLD_EXT_OBJECTS_CAPTURED );
    states.push_back( Settings::WORLD_SCOUTING_EXTENDED );
    states.push_back( Settings::WORLD_ARTIFACT_CRYSTAL_BALL );
    states.push_back( Settings::WORLD_ONLY_FIRST_MONSTER_ATTACK );
    states.push_back( Settings::WORLD_EYE_EAGLE_AS_SCHOLAR );
    states.push_back( Settings::WORLD_BAN_WEEKOF );
    states.push_back( Settings::WORLD_NEW_VERSION_WEEKOF );
    states.push_back( Settings::WORLD_BAN_PLAGUES );
    states.push_back( Settings::WORLD_BAN_MONTHOF_MONSTERS );
    states.push_back( Settings::WORLD_STARTHERO_LOSSCOND4HUMANS );
    states.push_back( Settings::WORLD_1HERO_HIRED_EVERY_WEEK );
    states.push_back( Settings::CASTLE_1HERO_HIRED_EVERY_WEEK );
    states.push_back( Settings::WORLD_SCALE_NEUTRAL_ARMIES );
    states.push_back( Settings::WORLD_USE_UNIQUE_ARTIFACTS_ML );
    states.push_back( Settings::WORLD_USE_UNIQUE_ARTIFACTS_RS );
    states.push_back( Settings::WORLD_USE_UNIQUE_ARTIFACTS_PS );
    states.push_back( Settings::WORLD_USE_UNIQUE_ARTIFACTS_SS );
    states.push_back( Settings::WORLD_DISABLE_BARROW_MOUNDS );
    states.push_back( Settings::HEROES_BUY_BOOK_FROM_SHRINES );
    states.push_back( Settings::HEROES_COST_DEPENDED_FROM_LEVEL );
    states.push_back( Settings::HEROES_REMEMBER_POINTS_RETREAT );
    states.push_back( Settings::HEROES_SURRENDERING_GIVE_EXP );
    states.push_back( Settings::HEROES_RECALCULATE_MOVEMENT );
    states.push_back( Settings::HEROES_TRANSCRIBING_SCROLLS );
    states.push_back( Settings::HEROES_ALLOW_BANNED_SECSKILLS );
    states.push_back( Settings::HEROES_ARENA_ANY_SKILLS );

    states.push_back( Settings::CASTLE_ALLOW_GUARDIANS );
    states.push_back( Settings::CASTLE_MAGEGUILD_POINTS_TURN );

    states.push_back( Settings::UNIONS_ALLOW_HERO_MEETINGS );
    states.push_back( Settings::UNIONS_ALLOW_CASTLE_VISITING );

    states.push_back( Settings::BATTLE_SHOW_ARMY_ORDER );
    states.push_back( Settings::BATTLE_SOFT_WAITING );
    states.push_back( Settings::BATTLE_OBJECTS_ARCHERS_PENALTY );
    states.push_back( Settings::BATTLE_SKIP_INCREASE_DEFENSE );
    states.push_back( Settings::BATTLE_REVERSE_WAIT_ORDER );

    if ( conf.PocketPC() ) {
        states.push_back( Settings::POCKETPC_TAP_MODE );
        states.push_back( Settings::POCKETPC_DRAG_DROP_SCROLL );
    }

    std::sort( states.begin(), states.end(),
               [&conf]( uint32_t first, uint32_t second ) { return std::string( conf.ExtName( first ) ) > std::string( conf.ExtName( second ) ); } );

    SettingsListBox listbox( area, readonly );

    listbox._restorer.update( area.x + 15, area.y + 25, 280, 336 );

    const int ah = 340;

    listbox.RedrawBackground( area );
    listbox.SetScrollButtonUp( ICN::DROPLISL, 6, 7, fheroes2::Point( area.x + 295, area.y + 25 ) );
    listbox.SetScrollButtonDn( ICN::DROPLISL, 8, 9, fheroes2::Point( area.x + 295, area.y + ah + 5 ) );
    listbox.SetScrollBar( fheroes2::AGG::GetICN( ICN::DROPLISL, 13 ), fheroes2::Rect( area.x + 300, area.y + 49, 12, ah - 46 ) );
    listbox.SetAreaMaxItems( ah / 40 );
    listbox.SetAreaItems( fheroes2::Rect( area.x + 10, area.y + 30, 290, ah + 5 ) );
    listbox.SetListContent( states );
    listbox.Redraw();

    LocalEvent & le = LocalEvent::Get();

    const fheroes2::Rect buttonsArea( area.x + 5, area.y, area.w - 10, area.h - 5 );

    const int buttonIcnId = conf.ExtGameEvilInterface() ? ICN::SPANBTNE : ICN::SPANBTN;
    const fheroes2::Sprite & buttonSprite = fheroes2::AGG::GetICN( buttonIcnId, 0 );

    fheroes2::Button buttonOk( buttonsArea.x + ( buttonsArea.width - buttonSprite.width() ) / 2, buttonsArea.y + buttonsArea.height - buttonSprite.height(), buttonIcnId,
                               0, 1 );

    buttonOk.draw();

    cursor.Show();
    display.render();

    // message loop
    while ( le.HandleEvents() ) {
        le.MousePressLeft( buttonOk.area() ) ? buttonOk.drawOnPress() : buttonOk.drawOnRelease();
        if ( le.MouseClickLeft( buttonOk.area() ) ) {
            break;
        }

        listbox.QueueEventProcessing();

        if ( !cursor.isVisible() ) {
            listbox.Redraw();
            cursor.Show();
            display.render();
        }
    }

    le.SetTapMode( conf.ExtPocketTapMode() );
    Settings::Get().BinarySave();
}
