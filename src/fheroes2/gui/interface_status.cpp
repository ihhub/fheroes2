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

#include "interface_status.h"

#include <cassert>
#include <string>
#include <utility>

#include "agg_image.h"
#include "army.h"
#include "castle.h"
#include "color.h"
#include "dialog.h"
#include "game.h"
#include "game_delays.h"
#include "game_interface.h"
#include "gamedefs.h"
#include "heroes.h"
#include "icn.h"
#include "image.h"
#include "interface_base.h"
#include "kingdom.h"
#include "localevent.h"
#include "math_base.h"
#include "players.h"
#include "resource.h"
#include "screen.h"
#include "settings.h"
#include "tools.h"
#include "translations.h"
#include "ui_dialog.h"
#include "ui_text.h"
#include "world.h"

namespace
{
    const uint32_t resourceWindowExpireTime = 2500;
}

Interface::StatusWindow::StatusWindow( BaseInterface & interface )
    : BorderWindow( { 0, 0, 144, 72 } )
    , _interface( interface )
    , _state( StatusType::STATUS_UNKNOWN )
    , lastResource( Resource::UNKNOWN )
    , countLastResource( 0 )
    , turn_progress( 0 )
    , showLastResourceDelay( resourceWindowExpireTime )
{}

void Interface::StatusWindow::Reset()
{
    _state = StatusType::STATUS_DAY;
    lastResource = Resource::UNKNOWN;
    countLastResource = 0;
}

void Interface::StatusWindow::SavePosition()
{
    Settings & conf = Settings::Get();

    conf.SetPosStatus( GetRect().getPosition() );
    conf.Save( Settings::configFileName );
}

void Interface::StatusWindow::SetRedraw() const
{
    _interface.setRedraw( REDRAW_STATUS );
}

void Interface::StatusWindow::SetPos( int32_t ox, int32_t oy )
{
    uint32_t ow = 144;
    uint32_t oh = 72;

    if ( !Settings::Get().isHideInterfaceEnabled() ) {
        oh = fheroes2::Display::instance().height() - oy - BORDERWIDTH;
    }

    BorderWindow::SetPosition( ox, oy, ow, oh );
}

void Interface::StatusWindow::SetState( const StatusType status )
{
    // SetResource() should be used to set this status
    assert( status != StatusType::STATUS_RESOURCE );

    if ( _state != StatusType::STATUS_RESOURCE ) {
        _state = status;
    }
}

void Interface::StatusWindow::_redraw() const
{
    const Settings & conf = Settings::Get();
    if ( conf.isHideInterfaceEnabled() && !conf.ShowStatus() ) {
        // The window is hidden.
        return;
    }

    const fheroes2::Rect & pos = GetArea();

    if ( conf.isHideInterfaceEnabled() ) {
        fheroes2::Fill( fheroes2::Display::instance(), pos.x, pos.y, pos.width, pos.height, fheroes2::GetColorId( 0x51, 0x31, 0x18 ) );
        BorderWindow::Redraw();
    }
    else {
        DrawBackground();
    }

    // Do not draw anything if the game hasn't really started yet
    if ( world.CountDay() == 0 ) {
        return;
    }

    // draw info: Day and Funds and Army
    const fheroes2::Sprite & ston = fheroes2::AGG::GetICN( conf.isEvilInterfaceEnabled() ? ICN::STONBAKE : ICN::STONBACK, 0 );
    const int32_t stonHeight = ston.height();

    if ( StatusType::STATUS_AITURN == _state ) {
        DrawAITurns();
    }
    else if ( StatusType::STATUS_UNKNOWN != _state && pos.height >= ( stonHeight * 3 + 15 ) ) {
        DrawDayInfo();

        if ( conf.CurrentColor() & Players::HumanColors() ) {
            DrawKingdomInfo( stonHeight + 5 );

            if ( _state != StatusType::STATUS_RESOURCE )
                DrawArmyInfo( 2 * stonHeight + 10 );
            else
                DrawResourceInfo( 2 * stonHeight + 10 );
        }
    }
    else if ( StatusType::STATUS_UNKNOWN != _state && pos.height >= ( stonHeight * 2 + 15 ) ) {
        DrawDayInfo();

        switch ( _state ) {
        case StatusType::STATUS_FUNDS:
            DrawKingdomInfo( stonHeight + 5 );
            break;
        case StatusType::STATUS_DAY:
        case StatusType::STATUS_ARMY:
            DrawArmyInfo( stonHeight + 5 );
            break;
        case StatusType::STATUS_RESOURCE:
            DrawResourceInfo( stonHeight + 5 );
            break;
        case StatusType::STATUS_UNKNOWN:
        case StatusType::STATUS_AITURN:
            assert( 0 ); // we shouldn't even reach this code
            break;
        default:
            break;
        }
    }
    else {
        switch ( _state ) {
        case StatusType::STATUS_DAY:
            DrawDayInfo();
            break;
        case StatusType::STATUS_FUNDS:
            DrawKingdomInfo();
            break;
        case StatusType::STATUS_ARMY:
            DrawArmyInfo();
            break;
        case StatusType::STATUS_RESOURCE:
            DrawResourceInfo();
            break;
        case StatusType::STATUS_UNKNOWN:
        case StatusType::STATUS_AITURN:
            assert( 0 ); // we shouldn't even reach this code
            break;
        default:
            break;
        }
    }
}

void Interface::StatusWindow::NextState()
{
    const int32_t areaHeight = GetArea().height;
    const fheroes2::Sprite & ston = fheroes2::AGG::GetICN( Settings::Get().isEvilInterfaceEnabled() ? ICN::STONBAKE : ICN::STONBACK, 0 );
    const int32_t stonHeight = ston.height();

    const bool skipDayStatus = areaHeight >= ( stonHeight * 2 + 15 ) && areaHeight < ( stonHeight * 3 + 15 );

    if ( StatusType::STATUS_DAY == _state )
        _state = StatusType::STATUS_FUNDS;
    else if ( StatusType::STATUS_FUNDS == _state )
        _state = ( GameFocus::UNSEL == GetFocusType() ? StatusType::STATUS_DAY : StatusType::STATUS_ARMY );
    else if ( StatusType::STATUS_ARMY == _state ) {
        if ( skipDayStatus ) {
            _state = StatusType::STATUS_FUNDS;
        }
        else {
            _state = StatusType::STATUS_DAY;
        }
    }
    else if ( StatusType::STATUS_RESOURCE == _state )
        _state = StatusType::STATUS_ARMY;

    if ( _state == StatusType::STATUS_ARMY ) {
        const Castle * castle = GetFocusCastle();

        // skip empty army for castle
        if ( castle && !castle->GetArmy().isValid() )
            NextState();
    }
}

void Interface::StatusWindow::DrawKingdomInfo( int oh ) const
{
    const fheroes2::Rect & pos = GetArea();
    const Kingdom & myKingdom = world.GetKingdom( Settings::Get().CurrentColor() );

    // sprite all resource
    fheroes2::Display & display = fheroes2::Display::instance();
    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::RESSMALL, 0 ), display, pos.x + 6, pos.y + 3 + oh );

    // count castle
    fheroes2::Text text( std::to_string( myKingdom.GetCountCastle() ), fheroes2::FontType::smallWhite() );
    text.draw( pos.x + 26 - text.width() / 2, pos.y + 30 + oh, display );
    // count town
    text.set( std::to_string( myKingdom.GetCountTown() ), fheroes2::FontType::smallWhite() );
    text.draw( pos.x + 78 - text.width() / 2, pos.y + 30 + oh, display );
    // count gold
    text.set( std::to_string( myKingdom.GetFunds().Get( Resource::GOLD ) ), fheroes2::FontType::smallWhite() );
    text.draw( pos.x + 122 - text.width() / 2, pos.y + 30 + oh, display );
    // count wood
    text.set( fheroes2::abbreviateNumber( myKingdom.GetFunds().Get( Resource::WOOD ) ), fheroes2::FontType::smallWhite() );
    text.draw( pos.x + 15 - text.width() / 2, pos.y + 60 + oh, display );
    // count mercury
    text.set( fheroes2::abbreviateNumber( myKingdom.GetFunds().Get( Resource::MERCURY ) ), fheroes2::FontType::smallWhite() );
    text.draw( pos.x + 37 - text.width() / 2, pos.y + 60 + oh, display );
    // count ore
    text.set( fheroes2::abbreviateNumber( myKingdom.GetFunds().Get( Resource::ORE ) ), fheroes2::FontType::smallWhite() );
    text.draw( pos.x + 60 - text.width() / 2, pos.y + 60 + oh, display );
    // count sulfur
    text.set( fheroes2::abbreviateNumber( myKingdom.GetFunds().Get( Resource::SULFUR ) ), fheroes2::FontType::smallWhite() );
    text.draw( pos.x + 84 - text.width() / 2, pos.y + 60 + oh, display );
    // count crystal
    text.set( fheroes2::abbreviateNumber( myKingdom.GetFunds().Get( Resource::CRYSTAL ) ), fheroes2::FontType::smallWhite() );
    text.draw( pos.x + 108 - text.width() / 2, pos.y + 60 + oh, display );
    // count gems
    text.set( fheroes2::abbreviateNumber( myKingdom.GetFunds().Get( Resource::GEMS ) ), fheroes2::FontType::smallWhite() );
    text.draw( pos.x + 130 - text.width() / 2, pos.y + 60 + oh, display );
}

void Interface::StatusWindow::DrawDayInfo( int oh ) const
{
    const fheroes2::Rect & pos = GetArea();

    const int dayOfWeek = world.GetDay();
    const int weekOfMonth = world.GetWeek();
    const uint32_t month = world.GetMonth();
    const int icnType = Settings::Get().isEvilInterfaceEnabled() ? ICN::SUNMOONE : ICN::SUNMOON;

    uint32_t icnId = dayOfWeek > 1 ? 0 : ( ( weekOfMonth - 1 ) % 4 ) + 1;
    // Special case
    if ( dayOfWeek == 1 && weekOfMonth == 1 && month == 1 ) {
        icnId = 0;
    }

    fheroes2::Display & display = fheroes2::Display::instance();
    fheroes2::Blit( fheroes2::AGG::GetICN( icnType, icnId ), display, pos.x, pos.y + 1 + oh );

    std::string message = _( "Month: %{month} Week: %{week}" );
    StringReplace( message, "%{month}", month );
    StringReplace( message, "%{week}", weekOfMonth );
    fheroes2::Text text( message, fheroes2::FontType::smallWhite() );
    text.draw( pos.x + ( pos.width - text.width() ) / 2, pos.y + 32 + oh, display );

    message = _( "Day: %{day}" );
    StringReplace( message, "%{day}", dayOfWeek );
    text.set( message, fheroes2::FontType::normalWhite() );
    text.draw( pos.x + ( pos.width - text.width() ) / 2, pos.y + 48 + oh, display );
}

void Interface::StatusWindow::SetResource( int res, uint32_t count )
{
    lastResource = res;
    countLastResource = count;
    _state = StatusType::STATUS_RESOURCE;

    showLastResourceDelay.reset();
}

void Interface::StatusWindow::DrawResourceInfo( int oh ) const
{
    const fheroes2::Rect & pos = GetArea();

    fheroes2::Display & display = fheroes2::Display::instance();

    std::string message = _( "You find a small\nquantity of %{resource}." );
    StringReplace( message, "%{resource}", Resource::String( lastResource ) );
    fheroes2::Text text{ std::move( message ), fheroes2::FontType::smallWhite() };
    text.draw( pos.x, pos.y + 6 + oh, pos.width, display );

    const fheroes2::Sprite & spr = fheroes2::AGG::GetICN( ICN::RESOURCE, Resource::getIconIcnIndex( lastResource ) );
    fheroes2::Blit( spr, display, pos.x + ( pos.width - spr.width() ) / 2, pos.y + 6 + oh + text.height( pos.width ) );

    text.set( std::to_string( countLastResource ), fheroes2::FontType::smallWhite() );
    text.draw( pos.x + ( pos.width - text.width() ) / 2, pos.y + oh + text.height() * 2 + spr.height() + 10, display );
}

void Interface::StatusWindow::DrawArmyInfo( int oh ) const
{
    const Army * armies = nullptr;

    if ( GetFocusHeroes() )
        armies = &GetFocusHeroes()->GetArmy();
    else if ( GetFocusCastle() )
        armies = &GetFocusCastle()->GetArmy();

    if ( armies ) {
        const fheroes2::Rect & pos = GetArea();
        Army::drawMultipleMonsterLines( *armies, pos.x + 4, pos.y + 1 + oh, 138, true, true );
    }
}

void Interface::StatusWindow::DrawAITurns() const
{
    // restore background
    DrawBackground();

    fheroes2::Display & display = fheroes2::Display::instance();

    const fheroes2::Sprite & glass = fheroes2::AGG::GetICN( ICN::HOURGLAS, 0 );
    const fheroes2::Rect & pos = GetArea();

    int32_t dst_x = pos.x + ( pos.width - glass.width() ) / 2;
    int32_t dst_y = pos.y + ( pos.height - glass.height() ) / 2;

    fheroes2::Blit( glass, display, dst_x, dst_y );

    int color_index = 0;

    const Settings & conf = Settings::Get();
    switch ( conf.CurrentColor() ) {
    case Color::BLUE:
        color_index = 0;
        break;
    case Color::GREEN:
        color_index = 1;
        break;
    case Color::RED:
        color_index = 2;
        break;
    case Color::YELLOW:
        color_index = 3;
        break;
    case Color::ORANGE:
        color_index = 4;
        break;
    case Color::PURPLE:
        color_index = 5;
        break;
    default:
        return;
    }

    const fheroes2::Sprite & crest = fheroes2::AGG::GetICN( ICN::BRCREST, color_index );

    dst_x += 2;
    dst_y += 2;

    fheroes2::Blit( crest, display, dst_x, dst_y );

    const fheroes2::Sprite & sand = fheroes2::AGG::GetICN( ICN::HOURGLAS, 1 + ( turn_progress % 10 ) );

    dst_x += ( glass.width() - sand.width() - sand.x() - 3 );
    dst_y += sand.y();

    fheroes2::Blit( sand, display, dst_x, dst_y );
}

void Interface::StatusWindow::DrawBackground() const
{
    fheroes2::Display & display = fheroes2::Display::instance();
    const fheroes2::Sprite & icnston = fheroes2::AGG::GetICN( Settings::Get().isEvilInterfaceEnabled() ? ICN::STONBAKE : ICN::STONBACK, 0 );
    const fheroes2::Rect & pos = GetArea();

    if ( !Settings::Get().isHideInterfaceEnabled() && display.height() - BORDERWIDTH - icnston.height() > pos.y ) {
        // top
        const int32_t startY = 11;
        const int32_t copyHeight = 46;
        fheroes2::Rect srcrt( 0, 0, icnston.width(), startY );
        fheroes2::Point dstpt( pos.x, pos.y );
        fheroes2::Blit( icnston, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );

        // middle
        srcrt = fheroes2::Rect( 0, startY, icnston.width(), copyHeight );
        const int32_t count = ( pos.height - ( icnston.height() - copyHeight ) ) / copyHeight;
        for ( int32_t i = 0; i < count; ++i ) {
            dstpt = fheroes2::Point( pos.x, pos.y + copyHeight * i + startY );
            fheroes2::Blit( icnston, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
        }

        // bottom
        srcrt = fheroes2::Rect( 0, startY, icnston.width(), icnston.height() - startY );
        dstpt = fheroes2::Point( pos.x, pos.y + pos.height - ( icnston.height() - startY ) );
        fheroes2::Blit( icnston, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
    }
    else {
        fheroes2::Blit( icnston, display, pos.x, pos.y );
    }
}

void Interface::StatusWindow::QueueEventProcessing()
{
    // Move border window
    if ( Settings::Get().ShowStatus() && BorderWindow::QueueEventProcessing() ) {
        SetRedraw();
        return;
    }

    LocalEvent & le = LocalEvent::Get();
    const fheroes2::Rect & drawnArea = GetArea();

    if ( le.MouseClickLeft( drawnArea ) ) {
        NextState();
        SetRedraw();
    }
    if ( le.isMouseRightButtonPressedInArea( GetRect() ) ) {
        const fheroes2::Sprite & ston = fheroes2::AGG::GetICN( Settings::Get().isEvilInterfaceEnabled() ? ICN::STONBAKE : ICN::STONBACK, 0 );
        const fheroes2::Rect & pos = GetArea();
        const bool isFullInfo = StatusType::STATUS_UNKNOWN != _state && pos.height >= ( ston.height() * 3 + 15 );
        if ( isFullInfo ) {
            fheroes2::showStandardTextMessage( _( "Status Window" ), _( "This window provides information on the status of your hero or kingdom, and shows the date." ),
                                               Dialog::ZERO );
        }
        else {
            fheroes2::showStandardTextMessage(
                _( "Status Window" ),
                _( "This window provides information on the status of your hero or kingdom, and shows the date. Left click here to cycle through these windows." ),
                Dialog::ZERO );
        }
    }
}

void Interface::StatusWindow::TimerEventProcessing()
{
    if ( _state != StatusType::STATUS_RESOURCE || !showLastResourceDelay.isPassed() ) {
        return;
    }

    switch ( GetFocusType() ) {
    case GameFocus::HEROES:
        _state = StatusType::STATUS_ARMY;
        break;
    case GameFocus::CASTLE:
        _state = StatusType::STATUS_FUNDS;
        break;
    default:
        _state = StatusType::STATUS_DAY;
        break;
    }

    SetRedraw();
}

void Interface::StatusWindow::DrawAITurnProgress( const uint32_t progressValue )
{
    // Process events if any before rendering a frame. For instance, updating a mouse cursor position.
    LocalEvent::Get().HandleEvents( false );

    turn_progress = progressValue;

    _interface.setRedraw( REDRAW_STATUS );

    if ( Game::validateAnimationDelay( Game::MAPS_DELAY ) ) {
        Game::updateAdventureMapAnimationIndex();

        _interface.redraw( REDRAW_GAMEAREA );
        fheroes2::Display::instance().render();
    }
}
