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
#include "castle.h"
#include "cursor.h"
#include "game.h"
#include "game_interface.h"
#include "heroes.h"
#include "interface_icons.h"
#include "kingdom.h"
#include "race.h"
#include "settings.h"
#include "world.h"

#define ICONS_WIDTH 46
#define ICONS_HEIGHT 22
#define ICONS_CURSOR_WIDTH 56
#define ICONS_CURSOR_HEIGHT 32

bool Interface::IconsBar::IsVisible( void )
{
    const Settings & conf = Settings::Get();
    return !conf.ExtGameHideInterface() || conf.ShowIcons();
}

u32 Interface::IconsBar::GetItemWidth( void )
{
    return ICONS_WIDTH;
}

u32 Interface::IconsBar::GetItemHeight( void )
{
    return ICONS_HEIGHT;
}

void Interface::RedrawCastleIcon( const Castle & castle, s32 sx, s32 sy )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    const bool evil = Settings::Get().ExtGameEvilInterface();
    u32 index_sprite = 1;

    switch ( castle.GetRace() ) {
    case Race::KNGT:
        index_sprite = castle.isCastle() ? 9 : 15;
        break;
    case Race::BARB:
        index_sprite = castle.isCastle() ? 10 : 16;
        break;
    case Race::SORC:
        index_sprite = castle.isCastle() ? 11 : 17;
        break;
    case Race::WRLK:
        index_sprite = castle.isCastle() ? 12 : 18;
        break;
    case Race::WZRD:
        index_sprite = castle.isCastle() ? 13 : 19;
        break;
    case Race::NECR:
        index_sprite = castle.isCastle() ? 14 : 20;
        break;
    default:
        DEBUG( DBG_ENGINE, DBG_WARN, "unknown race" );
    }

    fheroes2::Blit( fheroes2::AGG::GetICN( evil ? ICN::LOCATORE : ICN::LOCATORS, index_sprite ), display, sx, sy );

    // castle build marker
    switch ( Castle::GetAllBuildingStatus( castle ) ) {
    // white marker
    case UNKNOWN_COND:
    case NOT_TODAY:
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::CSLMARKER, 0 ), display, sx + 40, sy );
        break;

    // green marker
    // case LACK_RESOURCES:
    //    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::CSLMARKER, 2 ), display, sx + 40, sy );
    //    break;

    // red marker
    case REQUIRES_BUILD:
        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::CSLMARKER, 1 ), display, sx + 40, sy );
        break;

    default:
        break;
    }
}

void Interface::RedrawHeroesIcon( const Heroes & hero, s32 sx, s32 sy )
{
    hero.PortraitRedraw( sx, sy, PORT_SMALL, fheroes2::Display::instance() );
}

void Interface::IconsBar::RedrawBackground( const Point & pos )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    const fheroes2::Sprite & icnadv = fheroes2::AGG::GetICN( Settings::Get().ExtGameEvilInterface() ? ICN::ADVBORDE : ICN::ADVBORD, 0 );
    const fheroes2::Sprite & back = fheroes2::AGG::GetICN( Settings::Get().ExtGameEvilInterface() ? ICN::LOCATORE : ICN::LOCATORS, 1 );
    fheroes2::Rect srcrt;
    fheroes2::Point dstpt;

    srcrt.x = icnadv.width() - RADARWIDTH - BORDERWIDTH;
    srcrt.y = RADARWIDTH + 2 * BORDERWIDTH;
    srcrt.width = RADARWIDTH / 2;
    srcrt.height = 32;

    dstpt.x = pos.x;
    dstpt.y = pos.y;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );

    srcrt.y = srcrt.y + srcrt.height;
    dstpt.y = dstpt.y + srcrt.height;
    srcrt.height = 32;

    if ( 2 < iconsCount ) {
        for ( u32 ii = 0; ii < iconsCount - 2; ++ii ) {
            fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
            dstpt.y += srcrt.height;
        }
    }

    srcrt.y = srcrt.y + 64;
    srcrt.height = 32;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );

    for ( u32 ii = 0; ii < iconsCount; ++ii )
        fheroes2::Blit( back, display, pos.x + 5, pos.y + 5 + ii * ( IconsBar::GetItemHeight() + 10 ) );
}

/* Interface::CastleIcons */
void Interface::CastleIcons::RedrawItem( const CASTLE & item, s32 ox, s32 oy, bool current )
{
    if ( item && show ) {
        RedrawCastleIcon( *item, ox + 5, oy + 5 );

        if ( current )
            fheroes2::Blit( marker, fheroes2::Display::instance(), ox, oy );
    }
}

void Interface::CastleIcons::RedrawBackground( const Point & pos )
{
    IconsBar::RedrawBackground( pos );
}

void Interface::CastleIcons::ActionCurrentUp( void )
{
    Interface::Basic::Get().SetFocus( GetCurrent() );
}

void Interface::CastleIcons::ActionCurrentDn( void )
{
    Interface::Basic::Get().SetFocus( GetCurrent() );
}

void Interface::CastleIcons::ActionListDoubleClick( CASTLE & item )
{
    if ( item ) {
        Game::OpenCastleDialog( *item );
    }
}

void Interface::CastleIcons::ActionListSingleClick( CASTLE & item )
{
    if ( item ) {
        Interface::Basic & I = Interface::Basic::Get();

        I.SetFocus( item );
        I.RedrawFocus();
    }
}

void Interface::CastleIcons::ActionListPressRight( CASTLE & item )
{
    if ( item ) {
        Cursor::Get().Hide();
        Dialog::QuickInfo( *item );
    }
}

void Interface::CastleIcons::SetShow( bool f )
{
    IconsBar::SetShow( f );

    if ( IconsBar::IsVisible() ) {
        if ( f )
            GetScrollbar().show();
        else
            GetScrollbar().hide();
    }
}

void Interface::CastleIcons::SetPos( s32 px, s32 py )
{
    const int icnscroll = Settings::Get().ExtGameEvilInterface() ? ICN::SCROLLE : ICN::SCROLL;

    SetTopLeft( Point( px, py ) );
    SetScrollBar( fheroes2::AGG::GetICN( icnscroll, 4 ), fheroes2::Rect( px + ICONS_CURSOR_WIDTH + 3, py + 19, 10, ICONS_CURSOR_HEIGHT * iconsCount - 38 ) );
    SetScrollButtonUp( icnscroll, 0, 1, fheroes2::Point( px + ICONS_CURSOR_WIDTH + 1, py + 1 ) );
    SetScrollButtonDn( icnscroll, 2, 3, fheroes2::Point( px + ICONS_CURSOR_WIDTH + 1, py + iconsCount * ICONS_CURSOR_HEIGHT - 15 ) );
    SetAreaMaxItems( iconsCount );
    SetAreaItems( fheroes2::Rect( px, py, ICONS_CURSOR_WIDTH, iconsCount * ICONS_CURSOR_HEIGHT ) );
    DisableHotkeys( true );

    SetListContent( world.GetKingdom( Settings::Get().CurrentColor() ).GetCastles() );
    Reset();
}

/* Interface::HeroesIcons */
void Interface::HeroesIcons::RedrawItem( const HEROES & item, s32 ox, s32 oy, bool current )
{
    if ( item && show ) {
        RedrawHeroesIcon( *item, ox + 5, oy + 5 );

        if ( current )
            fheroes2::Blit( marker, fheroes2::Display::instance(), ox, oy );
    }
}

void Interface::HeroesIcons::RedrawBackground( const Point & pos )
{
    IconsBar::RedrawBackground( pos );
}

void Interface::HeroesIcons::ActionCurrentUp( void )
{
    Interface::Basic::Get().SetFocus( GetCurrent() );
}

void Interface::HeroesIcons::ActionCurrentDn( void )
{
    Interface::Basic::Get().SetFocus( GetCurrent() );
}

void Interface::HeroesIcons::ActionListDoubleClick( HEROES & item )
{
    if ( item ) {
        if ( item->Modes( Heroes::GUARDIAN ) ) {
            Castle * castle = world.GetCastle( item->GetCenter() );
            if ( castle )
                Game::OpenCastleDialog( *castle );
        }
        else
            Game::OpenHeroesDialog( *item, false );
    }
}

void Interface::HeroesIcons::ActionListSingleClick( HEROES & item )
{
    if ( item ) {
        Interface::Basic & I = Interface::Basic::Get();

        I.SetFocus( item );
        I.CalculateHeroPath( item, -1 );
        I.RedrawFocus();
    }
}

void Interface::HeroesIcons::ActionListPressRight( HEROES & item )
{
    if ( item ) {
        Cursor::Get().Hide();
        Dialog::QuickInfo( *item );
    }
}

void Interface::HeroesIcons::SetShow( bool f )
{
    IconsBar::SetShow( f );

    if ( IconsBar::IsVisible() ) {
        if ( f )
            GetScrollbar().show();
        else
            GetScrollbar().hide();
    }
}

void Interface::HeroesIcons::SetPos( s32 px, s32 py )
{
    const int icnscroll = Settings::Get().ExtGameEvilInterface() ? ICN::SCROLLE : ICN::SCROLL;

    SetTopLeft( Point( px, py ) );
    SetScrollBar( fheroes2::AGG::GetICN( icnscroll, 4 ), fheroes2::Rect( px + ICONS_CURSOR_WIDTH + 3, py + 19, 10, ICONS_CURSOR_HEIGHT * iconsCount - 38 ) );
    SetScrollButtonUp( icnscroll, 0, 1, fheroes2::Point( px + ICONS_CURSOR_WIDTH + 1, py + 1 ) );
    SetScrollButtonDn( icnscroll, 2, 3, fheroes2::Point( px + ICONS_CURSOR_WIDTH + 1, py + iconsCount * ICONS_CURSOR_HEIGHT - 15 ) );
    SetAreaMaxItems( iconsCount );
    SetAreaItems( fheroes2::Rect( px, py, ICONS_CURSOR_WIDTH, iconsCount * ICONS_CURSOR_HEIGHT ) );
    DisableHotkeys( true );

    SetListContent( world.GetKingdom( Settings::Get().CurrentColor() ).GetHeroes() );
    Reset();
}

/* Interface::IconsPanel */
Interface::IconsPanel::IconsPanel( Basic & basic )
    : BorderWindow( Rect( 0, 0, 144, 128 ) )
    , interface( basic )
    , castleIcons( 4, sfMarker )
    , heroesIcons( 4, sfMarker )
{
    sfMarker.resize( ICONS_CURSOR_WIDTH, ICONS_CURSOR_HEIGHT );
    sfMarker.reset();
    fheroes2::DrawBorder( sfMarker, fheroes2::GetColorId( 0xA0, 0xE0, 0xE0 ) );
}

u32 Interface::IconsPanel::CountIcons( void ) const
{
    return castleIcons.CountIcons();
}

void Interface::IconsPanel::SavePosition( void )
{
    Settings::Get().SetPosIcons( GetRect() );
}

void Interface::IconsPanel::SetRedraw( icons_t type ) const
{
    // is visible
    if ( IconsBar::IsVisible() ) {
        switch ( type ) {
        case ICON_HEROES:
            interface.SetRedraw( REDRAW_HEROES );
            break;
        case ICON_CASTLES:
            interface.SetRedraw( REDRAW_CASTLES );
            break;
        default:
            break;
        }

        interface.SetRedraw( REDRAW_ICONS );
    }
}

void Interface::IconsPanel::SetRedraw( void ) const
{
    SetRedraw( ICON_ANY );
}

void Interface::IconsPanel::SetPos( s32 ox, s32 oy )
{
    u32 iconsCount = 0;

    if ( Settings::Get().ExtGameHideInterface() ) {
        iconsCount = 2;
    }
    else {
        const u32 count_h = ( fheroes2::Display::instance().height() - fheroes2::Display::DEFAULT_HEIGHT ) / TILEWIDTH;
        iconsCount = count_h > 3 ? 8 : ( count_h < 3 ? 4 : 7 );
    }

    BorderWindow::SetPosition( ox, oy, 144, iconsCount * ICONS_CURSOR_HEIGHT );

    const Rect & rect = GetArea();

    heroesIcons.SetIconsCount( iconsCount );
    castleIcons.SetIconsCount( iconsCount );

    heroesIcons.SetPos( rect.x, rect.y );
    castleIcons.SetPos( rect.x + 72, rect.y );
}

void Interface::IconsPanel::Redraw( void )
{
    // is visible
    if ( IconsBar::IsVisible() ) {
        // redraw border
        if ( Settings::Get().ExtGameHideInterface() )
            BorderWindow::Redraw();

        heroesIcons.Redraw();
        castleIcons.Redraw();
    }
}

void Interface::IconsPanel::QueueEventProcessing( void )
{
    if ( Settings::Get().ShowIcons() &&
         // move border window
         BorderWindow::QueueEventProcessing() ) {
        interface.RedrawFocus();
        SetRedraw();
    }
    else if ( heroesIcons.QueueEventProcessing() ) {
        if ( heroesIcons.isSelected() )
            castleIcons.Unselect();

        SetRedraw();
    }
    else if ( castleIcons.QueueEventProcessing() ) {
        if ( castleIcons.isSelected() )
            heroesIcons.Unselect();

        SetRedraw();
    }
}

void Interface::IconsPanel::Select( const Heroes & hr )
{
    castleIcons.Unselect();
    heroesIcons.SetCurrent( (const HEROES)&hr );
}

void Interface::IconsPanel::Select( const Castle & cs )
{
    heroesIcons.Unselect();
    castleIcons.SetCurrent( (const CASTLE)&cs );
}

void Interface::IconsPanel::ResetIcons( icons_t type )
{
    Kingdom & kingdom = world.GetKingdom( Settings::Get().CurrentColor() );

    if ( !kingdom.isControlAI() ) {
        if ( type & ICON_HEROES ) {
            heroesIcons.SetListContent( kingdom.GetHeroes() );
            heroesIcons.Reset();
        }

        if ( type & ICON_CASTLES ) {
            castleIcons.SetListContent( kingdom.GetCastles() );
            castleIcons.Reset();
        }
    }
}

void Interface::IconsPanel::HideIcons( icons_t type )
{
    if ( type & ICON_HEROES )
        heroesIcons.SetShow( false );
    if ( type & ICON_CASTLES )
        castleIcons.SetShow( false );
}

void Interface::IconsPanel::ShowIcons( icons_t type )
{
    if ( type & ICON_HEROES )
        heroesIcons.SetShow( true );
    if ( type & ICON_CASTLES )
        castleIcons.SetShow( true );
}

void Interface::IconsPanel::SetCurrentVisible( void )
{
    if ( heroesIcons.isSelected() ) {
        heroesIcons.SetCurrentVisible();
        heroesIcons.Redraw();
    }
    else if ( castleIcons.isSelected() ) {
        castleIcons.SetCurrentVisible();
        castleIcons.Redraw();
    }
}

void Interface::IconsPanel::RedrawIcons( icons_t type )
{
    if ( type & ICON_HEROES )
        heroesIcons.Redraw();
    if ( type & ICON_CASTLES )
        castleIcons.Redraw();
}

bool Interface::IconsPanel::IsSelected( icons_t type ) const
{
    if ( type & ICON_HEROES )
        return heroesIcons.isSelected();
    else if ( type & ICON_CASTLES )
        return castleIcons.isSelected();

    return false;
}
