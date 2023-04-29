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

#include "interface_icons.h"

#include "agg_image.h"
#include "dialog.h"
#include "game.h"
#include "game_interface.h"
#include "gamedefs.h"
#include "heroes.h"
#include "heroes_base.h"
#include "icn.h"
#include "kingdom.h"
#include "maps.h"
#include "screen.h"
#include "settings.h"
#include "ui_castle.h"
#include "ui_scrollbar.h"
#include "world.h"

class Castle;

namespace
{
    enum : int32_t
    {
        ICONS_WIDTH = 46,
        ICONS_HEIGHT = 22,
        ICONS_CURSOR_WIDTH = 56,
        ICONS_CURSOR_HEIGHT = 32
    };
}

bool Interface::IconsBar::IsVisible()
{
    const Settings & conf = Settings::Get();
    return !conf.isHideInterfaceEnabled() || conf.ShowIcons();
}

int32_t Interface::IconsBar::GetItemWidth()
{
    return ICONS_WIDTH;
}

int32_t Interface::IconsBar::GetItemHeight()
{
    return ICONS_HEIGHT;
}

void Interface::RedrawCastleIcon( const Castle & castle, int32_t sx, int32_t sy )
{
    fheroes2::drawCastleIcon( castle, fheroes2::Display::instance(), { sx, sy } );
}

void Interface::RedrawHeroesIcon( const Heroes & hero, int32_t sx, int32_t sy )
{
    hero.PortraitRedraw( sx, sy, PORT_SMALL, fheroes2::Display::instance() );
}

void Interface::IconsBar::redrawBackground( fheroes2::Image & output, const fheroes2::Point & offset, const int32_t validItemCount ) const
{
    const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

    const fheroes2::Sprite & icnadv = fheroes2::AGG::GetICN( isEvilInterface ? ICN::ADVBORDE : ICN::ADVBORD, 0 );
    fheroes2::Rect srcrt( icnadv.width() - RADARWIDTH - BORDERWIDTH, RADARWIDTH + 2 * BORDERWIDTH, RADARWIDTH / 2, 32 );

    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, output, offset.x, offset.y, srcrt.width, srcrt.height );

    srcrt.y = srcrt.y + srcrt.height;

    int32_t internalOffsetY = offset.y + srcrt.height;
    srcrt.height = 32;

    if ( iconsCount > 2 ) {
        for ( int32_t i = 0; i < iconsCount - 2; ++i ) {
            fheroes2::Blit( icnadv, srcrt.x, srcrt.y, output, offset.x, internalOffsetY, srcrt.width, srcrt.height );
            internalOffsetY += srcrt.height;
        }
    }

    srcrt.y = srcrt.y + 64;
    srcrt.height = 32;
    fheroes2::Blit( icnadv, srcrt.x, srcrt.y, output, offset.x, internalOffsetY, srcrt.width, srcrt.height );

    for ( int32_t i = validItemCount; i < iconsCount; ++i ) {
        const fheroes2::Sprite & background = fheroes2::AGG::GetICN( isEvilInterface ? ICN::LOCATORE : ICN::LOCATORS, 1 + i % 8 );
        fheroes2::Blit( background, output, offset.x + 5, offset.y + 5 + i * ( IconsBar::GetItemHeight() + 10 ) );
    }
}

void Interface::CastleIcons::RedrawItem( const CASTLE & item, int32_t ox, int32_t oy, bool current )
{
    if ( item && show ) {
        RedrawCastleIcon( *item, ox + 5, oy + 5 );

        if ( current )
            fheroes2::Blit( marker, fheroes2::Display::instance(), ox, oy );
    }
}

void Interface::CastleIcons::RedrawBackground( const fheroes2::Point & pos )
{
    redrawBackground( fheroes2::Display::instance(), pos, show ? _size() : 0 );
}

void Interface::CastleIcons::ActionCurrentUp()
{
    Interface::Basic::Get().SetFocus( GetCurrent() );
}

void Interface::CastleIcons::ActionCurrentDn()
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
        Dialog::QuickInfo( *item, { _topLeftCorner.x - 1, _topLeftCorner.y }, true );
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

void Interface::CastleIcons::SetPos( int32_t px, int32_t py )
{
    Castle * selectedCastle = isSelected() ? GetCurrent() : nullptr;

    const int icnscroll = Settings::Get().isEvilInterfaceEnabled() ? ICN::SCROLLE : ICN::SCROLL;

    _topLeftCorner = fheroes2::Point( px, py );
    SetTopLeft( _topLeftCorner );
    setScrollBarArea( { px + ICONS_CURSOR_WIDTH + 3, py + 19, 10, ICONS_CURSOR_HEIGHT * iconsCount - 38 } );

    KingdomCastles & castles = world.GetKingdom( Settings::Get().CurrentColor() ).GetCastles();

    const fheroes2::Sprite & originalSlider = fheroes2::AGG::GetICN( icnscroll, 4 );
    const fheroes2::Image scrollbarSlider
        = fheroes2::generateScrollbarSlider( originalSlider, false, ICONS_CURSOR_HEIGHT * iconsCount - 38, iconsCount, static_cast<int32_t>( castles.size() ),
                                             { 0, 0, originalSlider.width(), 8 }, { 0, 7, originalSlider.width(), 8 } );

    setScrollBarImage( scrollbarSlider );
    SetScrollButtonUp( icnscroll, 0, 1, { px + ICONS_CURSOR_WIDTH + 1, py + 1 } );
    SetScrollButtonDn( icnscroll, 2, 3, { px + ICONS_CURSOR_WIDTH + 1, py + iconsCount * ICONS_CURSOR_HEIGHT - 15 } );
    SetAreaMaxItems( iconsCount );
    SetAreaItems( { px, py, ICONS_CURSOR_WIDTH, iconsCount * ICONS_CURSOR_HEIGHT } );
    DisableHotkeys( true );

    SetListContent( castles );
    Reset();

    if ( selectedCastle ) {
        SetCurrent( selectedCastle );
    }
}

void Interface::HeroesIcons::RedrawItem( const HEROES & item, int32_t ox, int32_t oy, bool current )
{
    if ( item && show ) {
        RedrawHeroesIcon( *item, ox + 5, oy + 5 );

        if ( current )
            fheroes2::Blit( marker, fheroes2::Display::instance(), ox, oy );
    }
}

void Interface::HeroesIcons::RedrawBackground( const fheroes2::Point & pos )
{
    redrawBackground( fheroes2::Display::instance(), pos, show ? _size() : 0 );
}

void Interface::HeroesIcons::ActionCurrentUp()
{
    Interface::Basic::Get().SetFocus( GetCurrent(), false );
}

void Interface::HeroesIcons::ActionCurrentDn()
{
    Interface::Basic::Get().SetFocus( GetCurrent(), false );
}

void Interface::HeroesIcons::ActionListDoubleClick( HEROES & item )
{
    if ( item ) {
        Game::OpenHeroesDialog( *item, false, true );
    }
}

void Interface::HeroesIcons::ActionListSingleClick( HEROES & item )
{
    if ( item ) {
        Interface::Basic & I = Interface::Basic::Get();

        I.SetFocus( item, false );
        I.CalculateHeroPath( item, -1 );
        I.RedrawFocus();
    }
}

void Interface::HeroesIcons::ActionListPressRight( HEROES & item )
{
    if ( item ) {
        Dialog::QuickInfo( *item, { _topLeftCorner.x - 1, _topLeftCorner.y }, true );
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

void Interface::HeroesIcons::SetPos( int32_t px, int32_t py )
{
    Heroes * selectedHero = isSelected() ? GetCurrent() : nullptr;

    const int icnscroll = Settings::Get().isEvilInterfaceEnabled() ? ICN::SCROLLE : ICN::SCROLL;

    _topLeftCorner = fheroes2::Point( px, py );
    SetTopLeft( _topLeftCorner );
    setScrollBarArea( { px + ICONS_CURSOR_WIDTH + 3, py + 19, 10, ICONS_CURSOR_HEIGHT * iconsCount - 38 } );

    KingdomHeroes & heroes = world.GetKingdom( Settings::Get().CurrentColor() ).GetHeroes();

    const fheroes2::Sprite & originalSlider = fheroes2::AGG::GetICN( icnscroll, 4 );
    const fheroes2::Image scrollbarSlider
        = fheroes2::generateScrollbarSlider( originalSlider, false, ICONS_CURSOR_HEIGHT * iconsCount - 38, iconsCount, static_cast<int32_t>( heroes.size() ),
                                             { 0, 0, originalSlider.width(), 8 }, { 0, 7, originalSlider.width(), 8 } );

    setScrollBarImage( scrollbarSlider );
    SetScrollButtonUp( icnscroll, 0, 1, { px + ICONS_CURSOR_WIDTH + 1, py + 1 } );
    SetScrollButtonDn( icnscroll, 2, 3, { px + ICONS_CURSOR_WIDTH + 1, py + iconsCount * ICONS_CURSOR_HEIGHT - 15 } );
    SetAreaMaxItems( iconsCount );
    SetAreaItems( { px, py, ICONS_CURSOR_WIDTH, iconsCount * ICONS_CURSOR_HEIGHT } );
    DisableHotkeys( true );

    SetListContent( heroes );
    Reset();

    if ( selectedHero ) {
        SetCurrent( selectedHero );
    }
}

Interface::IconsPanel::IconsPanel( Basic & basic )
    : BorderWindow( { 0, 0, 144, 128 } )
    , interface( basic )
    , castleIcons( 4, sfMarker )
    , heroesIcons( 4, sfMarker )
{
    sfMarker.resize( ICONS_CURSOR_WIDTH, ICONS_CURSOR_HEIGHT );
    sfMarker.reset();
    fheroes2::DrawBorder( sfMarker, fheroes2::GetColorId( 0xA0, 0xE0, 0xE0 ) );
}

void Interface::IconsPanel::SavePosition()
{
    Settings & conf = Settings::Get();

    conf.SetPosIcons( GetRect().getPosition() );
    conf.Save( Settings::configFileName );
}

void Interface::IconsPanel::SetRedraw( const icons_t type ) const
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

void Interface::IconsPanel::SetRedraw() const
{
    SetRedraw( ICON_ANY );
}

void Interface::IconsPanel::SetPos( int32_t ox, int32_t oy )
{
    int32_t iconsCount = 0;

    if ( Settings::Get().isHideInterfaceEnabled() ) {
        iconsCount = 2;
    }
    else {
        const int32_t count_h = ( fheroes2::Display::instance().height() - fheroes2::Display::DEFAULT_HEIGHT ) / TILEWIDTH;
        iconsCount = count_h > 3 ? 8 : ( count_h < 3 ? 4 : 7 );
    }

    BorderWindow::SetPosition( ox, oy, 144, iconsCount * ICONS_CURSOR_HEIGHT );

    heroesIcons.SetIconsCount( iconsCount );
    castleIcons.SetIconsCount( iconsCount );

    const fheroes2::Rect & rect = GetArea();

    heroesIcons.SetPos( rect.x, rect.y );
    castleIcons.SetPos( rect.x + 72, rect.y );
}

void Interface::IconsPanel::Redraw()
{
    // is visible
    if ( IconsBar::IsVisible() ) {
        // redraw border
        if ( Settings::Get().isHideInterfaceEnabled() )
            BorderWindow::Redraw();

        heroesIcons.Redraw();
        castleIcons.Redraw();
    }
}

void Interface::IconsPanel::QueueEventProcessing()
{
    // Move border window
    if ( Settings::Get().ShowIcons() && BorderWindow::QueueEventProcessing() ) {
        SetRedraw();
    }
    else if ( heroesIcons.QueueEventProcessing() ) {
        if ( heroesIcons.isSelected() ) {
            castleIcons.Unselect();
        }

        SetRedraw();
    }
    else if ( castleIcons.QueueEventProcessing() ) {
        if ( castleIcons.isSelected() ) {
            heroesIcons.Unselect();
        }

        SetRedraw();
    }
}

void Interface::IconsPanel::Select( Heroes * const hr )
{
    castleIcons.Unselect();
    heroesIcons.SetCurrent( hr );
}

void Interface::IconsPanel::Select( Castle * const cs )
{
    heroesIcons.Unselect();
    castleIcons.SetCurrent( cs );
}

void Interface::IconsPanel::ResetIcons( const icons_t type )
{
    Kingdom & kingdom = world.GetKingdom( Settings::Get().CurrentColor() );

    if ( !kingdom.isControlAI() ) {
        const int icnscroll = Settings::Get().isEvilInterfaceEnabled() ? ICN::SCROLLE : ICN::SCROLL;

        const fheroes2::Sprite & originalSlider = fheroes2::AGG::GetICN( icnscroll, 4 );

        if ( type & ICON_HEROES ) {
            const fheroes2::Image scrollbarSlider = fheroes2::generateScrollbarSlider( originalSlider, false, ICONS_CURSOR_HEIGHT * heroesIcons.getIconsCount() - 38,
                                                                                       heroesIcons.getIconsCount(), static_cast<int32_t>( kingdom.GetHeroes().size() ),
                                                                                       { 0, 0, originalSlider.width(), 8 }, { 0, 7, originalSlider.width(), 8 } );
            heroesIcons.setScrollBarImage( scrollbarSlider );

            heroesIcons.SetListContent( kingdom.GetHeroes() );
            heroesIcons.Reset();
        }

        if ( type & ICON_CASTLES ) {
            const fheroes2::Image scrollbarSlider = fheroes2::generateScrollbarSlider( originalSlider, false, ICONS_CURSOR_HEIGHT * castleIcons.getIconsCount() - 38,
                                                                                       castleIcons.getIconsCount(), static_cast<int32_t>( kingdom.GetCastles().size() ),
                                                                                       { 0, 0, originalSlider.width(), 8 }, { 0, 7, originalSlider.width(), 8 } );
            castleIcons.setScrollBarImage( scrollbarSlider );

            castleIcons.SetListContent( kingdom.GetCastles() );
            castleIcons.Reset();
        }
    }
}

void Interface::IconsPanel::HideIcons( const icons_t type )
{
    if ( type & ICON_HEROES )
        heroesIcons.SetShow( false );
    if ( type & ICON_CASTLES )
        castleIcons.SetShow( false );
}

void Interface::IconsPanel::ShowIcons( const icons_t type )
{
    if ( type & ICON_HEROES )
        heroesIcons.SetShow( true );
    if ( type & ICON_CASTLES )
        castleIcons.SetShow( true );
}

void Interface::IconsPanel::RedrawIcons( const icons_t type )
{
    if ( type & ICON_HEROES )
        heroesIcons.Redraw();
    if ( type & ICON_CASTLES )
        castleIcons.Redraw();
}

bool Interface::IconsPanel::IsSelected( const icons_t type ) const
{
    if ( type & ICON_HEROES )
        return heroesIcons.isSelected();
    else if ( type & ICON_CASTLES )
        return castleIcons.isSelected();

    return false;
}
