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

#include <cassert>

#include "agg_image.h"
#include "army.h"
#include "castle.h"
#include "game_interface.h"
#include "heroes.h"
#include "icn.h"
#include "interface_status.h"
#include "kingdom.h"
#include "resource.h"
#include "settings.h"
#include "text.h"
#include "tools.h"
#include "world.h"

namespace
{
    const uint32_t resourceWindowExpireTime = 2500;
}

Interface::StatusWindow::StatusWindow( Basic & basic )
    : BorderWindow( fheroes2::Rect( 0, 0, 144, 72 ) )
    , interface( basic )
    , _state( StatusType::STATUS_UNKNOWN )
    , _oldState( StatusType::STATUS_UNKNOWN )
    , lastResource( Resource::UNKNOWN )
    , countLastResource( 0 )
    , turn_progress( 0 )
{}

void Interface::StatusWindow::Reset()
{
    _state = StatusType::STATUS_DAY;
    _oldState = StatusType::STATUS_UNKNOWN;
    lastResource = Resource::UNKNOWN;
    countLastResource = 0;
    ResetTimer();
}

u32 Interface::StatusWindow::ResetResourceStatus( u32 /*tick*/, void * ptr )
{
    if ( ptr ) {
        Interface::StatusWindow * status = reinterpret_cast<Interface::StatusWindow *>( ptr );
        if ( StatusType::STATUS_RESOURCE == status->_state ) {
            status->_state = status->_oldState;
            Interface::Basic::Get().SetRedraw( REDRAW_STATUS );
        }
        else {
            status->timerShowLastResource.Remove();
        }
    }

    return 0;
}

void Interface::StatusWindow::SavePosition( void )
{
    Settings::Get().SetPosStatus( GetRect().getPosition() );
}

void Interface::StatusWindow::SetRedraw( void ) const
{
    interface.SetRedraw( REDRAW_STATUS );
}

void Interface::StatusWindow::SetPos( s32 ox, s32 oy )
{
    u32 ow = 144;
    u32 oh = 72;

    if ( !Settings::Get().ExtGameHideInterface() ) {
        oh = fheroes2::Display::instance().height() - oy - BORDERWIDTH;
    }

    BorderWindow::SetPosition( ox, oy, ow, oh );
}

void Interface::StatusWindow::SetState( const StatusType status )
{
    if ( StatusType::STATUS_RESOURCE != _state )
        _state = status;
}

void Interface::StatusWindow::Redraw( void ) const
{
    const Settings & conf = Settings::Get();
    const fheroes2::Rect & pos = GetArea();

    if ( !conf.ExtGameHideInterface() || conf.ShowStatus() ) {
        if ( conf.ExtGameHideInterface() ) {
            fheroes2::Fill( fheroes2::Display::instance(), pos.x, pos.y, pos.width, pos.height, fheroes2::GetColorId( 0x51, 0x31, 0x18 ) );
            BorderWindow::Redraw();
        }
        else {
            DrawBackground();
        }

        // draw info: Day and Funds and Army
        const fheroes2::Sprite & ston = fheroes2::AGG::GetICN( Settings::Get().ExtGameEvilInterface() ? ICN::STONBAKE : ICN::STONBACK, 0 );
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
}

void Interface::StatusWindow::NextState( void )
{
    const int32_t areaHeight = GetArea().height;
    const fheroes2::Sprite & ston = fheroes2::AGG::GetICN( Settings::Get().ExtGameEvilInterface() ? ICN::STONBAKE : ICN::STONBACK, 0 );
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
    fheroes2::Blit( fheroes2::AGG::GetICN( ICN::RESSMALL, 0 ), fheroes2::Display::instance(), pos.x + 6, pos.y + 3 + oh );

    // count castle
    Text text( std::to_string( myKingdom.GetCountCastle() ), Font::SMALL );
    text.Blit( pos.x + 26 - text.w() / 2, pos.y + 28 + oh );
    // count town
    text.Set( std::to_string( myKingdom.GetCountTown() ) );
    text.Blit( pos.x + 78 - text.w() / 2, pos.y + 28 + oh );
    // count gold
    text.Set( std::to_string( myKingdom.GetFunds().Get( Resource::GOLD ) ) );
    text.Blit( pos.x + 122 - text.w() / 2, pos.y + 28 + oh );
    // count wood
    text.Set( GetStringShort( myKingdom.GetFunds().Get( Resource::WOOD ) ) );
    text.Blit( pos.x + 15 - text.w() / 2, pos.y + 58 + oh );
    // count mercury
    text.Set( GetStringShort( myKingdom.GetFunds().Get( Resource::MERCURY ) ) );
    text.Blit( pos.x + 37 - text.w() / 2, pos.y + 58 + oh );
    // count ore
    text.Set( GetStringShort( myKingdom.GetFunds().Get( Resource::ORE ) ) );
    text.Blit( pos.x + 60 - text.w() / 2, pos.y + 58 + oh );
    // count sulfur
    text.Set( GetStringShort( myKingdom.GetFunds().Get( Resource::SULFUR ) ) );
    text.Blit( pos.x + 84 - text.w() / 2, pos.y + 58 + oh );
    // count crystal
    text.Set( GetStringShort( myKingdom.GetFunds().Get( Resource::CRYSTAL ) ) );
    text.Blit( pos.x + 108 - text.w() / 2, pos.y + 58 + oh );
    // count gems
    text.Set( GetStringShort( myKingdom.GetFunds().Get( Resource::GEMS ) ) );
    text.Blit( pos.x + 130 - text.w() / 2, pos.y + 58 + oh );
}

void Interface::StatusWindow::DrawDayInfo( int oh ) const
{
    const fheroes2::Rect & pos = GetArea();

    const int dayOfWeek = world.GetDay();
    const int weekOfMonth = world.GetWeek();
    const int month = world.GetMonth();
    const int icnType = Settings::Get().ExtGameEvilInterface() ? ICN::SUNMOONE : ICN::SUNMOON;
    uint32_t icnId = dayOfWeek > 1 ? 0 : ( ( weekOfMonth - 1 ) % 4 ) + 1;
    if ( dayOfWeek == 1 && weekOfMonth == 1 && month == 1 ) { // special case
        icnId = 0;
    }

    fheroes2::Blit( fheroes2::AGG::GetICN( icnType, icnId ), fheroes2::Display::instance(), pos.x, pos.y + 1 + oh );

    std::string message = _( "Month: %{month} Week: %{week}" );
    StringReplace( message, "%{month}", world.GetMonth() );
    StringReplace( message, "%{week}", world.GetWeek() );
    Text text( message, Font::SMALL );
    text.Blit( pos.x + ( pos.width - text.w() ) / 2, pos.y + 30 + oh );

    message = _( "Day: %{day}" );
    StringReplace( message, "%{day}", world.GetDay() );
    text.Set( message, Font::BIG );
    text.Blit( pos.x + ( pos.width - text.w() ) / 2, pos.y + 46 + oh );
}

void Interface::StatusWindow::SetResource( int res, u32 count )
{
    lastResource = res;
    countLastResource = count;

    if ( timerShowLastResource.IsValid() )
        timerShowLastResource.Remove();
    else
        _oldState = _state;

    _state = StatusType::STATUS_RESOURCE;
    timerShowLastResource.Run( resourceWindowExpireTime, ResetResourceStatus, this );
}

void Interface::StatusWindow::ResetTimer( void )
{
    StatusWindow & window = Interface::Basic::Get().GetStatusWindow();

    if ( window.timerShowLastResource.IsValid() ) {
        window.timerShowLastResource.Remove();
        ResetResourceStatus( 0, &window );
    }
}

void Interface::StatusWindow::DrawResourceInfo( int oh ) const
{
    const fheroes2::Rect & pos = GetArea();

    std::string message = _( "You find a small\nquantity of %{resource}." );
    StringReplace( message, "%{resource}", Resource::String( lastResource ) );
    TextBox text( message, Font::SMALL, pos.width );
    text.Blit( pos.x, pos.y + 4 + oh );

    const fheroes2::Sprite & spr = fheroes2::AGG::GetICN( ICN::RESOURCE, Resource::GetIndexSprite2( lastResource ) );
    fheroes2::Blit( spr, fheroes2::Display::instance(), pos.x + ( pos.width - spr.width() ) / 2, pos.y + 6 + oh + text.h() );

    text.Set( std::to_string( countLastResource ), Font::SMALL, pos.width );
    text.Blit( pos.x + ( pos.width - text.w() ) / 2, pos.y + oh + text.h() * 2 + spr.height() + 8 );
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
        Army::DrawMonsterLines( *armies, pos.x + 4, pos.y + 1 + oh, 138, Skill::Level::EXPERT );
    }
}

void Interface::StatusWindow::DrawAITurns( void ) const
{
    const Settings & conf = Settings::Get();

    if ( !conf.ExtGameHideInterface() || conf.ShowStatus() ) {
        // restore background
        DrawBackground();

        fheroes2::Display & display = fheroes2::Display::instance();

        const fheroes2::Sprite & glass = fheroes2::AGG::GetICN( ICN::HOURGLAS, 0 );
        const fheroes2::Rect & pos = GetArea();

        s32 dst_x = pos.x + ( pos.width - glass.width() ) / 2;
        s32 dst_y = pos.y + ( pos.height - glass.height() ) / 2;

        fheroes2::Blit( glass, display, dst_x, dst_y );

        int color_index = 0;

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
}

void Interface::StatusWindow::DrawBackground( void ) const
{
    fheroes2::Display & display = fheroes2::Display::instance();
    const fheroes2::Sprite & icnston = fheroes2::AGG::GetICN( Settings::Get().ExtGameEvilInterface() ? ICN::STONBAKE : ICN::STONBACK, 0 );
    const fheroes2::Rect & pos = GetArea();

    if ( !Settings::Get().ExtGameHideInterface() && display.height() - BORDERWIDTH - icnston.height() > pos.y ) {
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

        // botom
        srcrt = fheroes2::Rect( 0, startY, icnston.width(), icnston.height() - startY );
        dstpt = fheroes2::Point( pos.x, pos.y + pos.height - ( icnston.height() - startY ) );
        fheroes2::Blit( icnston, srcrt.x, srcrt.y, display, dstpt.x, dstpt.y, srcrt.width, srcrt.height );
    }
    else {
        fheroes2::Blit( icnston, display, pos.x, pos.y );
    }
}

void Interface::StatusWindow::QueueEventProcessing( void )
{
    if ( Settings::Get().ShowStatus() && BorderWindow::QueueEventProcessing() ) {
        return;
    }

    LocalEvent & le = LocalEvent::Get();
    const fheroes2::Rect & drawnArea = GetArea();

    if ( le.MouseClickLeft( drawnArea ) ) {
        NextState();
        Redraw();
        fheroes2::Display::instance().render();
    }
    if ( le.MousePressRight( GetRect() ) ) {
        const fheroes2::Sprite & ston = fheroes2::AGG::GetICN( Settings::Get().ExtGameEvilInterface() ? ICN::STONBAKE : ICN::STONBACK, 0 );
        const fheroes2::Rect & pos = GetArea();
        const bool isFullInfo = StatusType::STATUS_UNKNOWN != _state && pos.height >= ( ston.height() * 3 + 15 );
        if ( isFullInfo ) {
            Dialog::Message( _( "Status Window" ), _( "This window provides information on the status of your hero or kingdom, and shows the date." ), Font::BIG );
        }
        else {
            Dialog::Message(
                _( "Status Window" ),
                _( "This window provides information on the status of your hero or kingdom, and shows the date. Left click here to cycle throungh these windows." ),
                Font::BIG );
        }
    }
}

void Interface::StatusWindow::RedrawTurnProgress( u32 v )
{
    turn_progress = v;
    SetRedraw();

    interface.Redraw();
    fheroes2::Display::instance().render();
}
