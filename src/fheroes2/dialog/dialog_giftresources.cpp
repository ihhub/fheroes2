/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2011 by Andrey Afletdinov <fheroes2@gmail.com>          *
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
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "agg_image.h"
#include "color.h"
#include "cursor.h"
#include "dialog.h"
#include "icn.h"
#include "image.h"
#include "kingdom.h"
#include "localevent.h"
#include "math_base.h"
#include "players.h"
#include "resource.h"
#include "screen.h"
#include "settings.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_dialog.h"
#include "ui_text.h"
#include "ui_window.h"
#include "world.h"

namespace
{
    const fheroes2::Size giftDialogSize( 320, 234 );
}

int32_t GetIndexClickRects( const std::vector<fheroes2::Rect> & rects )
{
    LocalEvent & le = LocalEvent::Get();

    const fheroes2::Point & pos = le.getMouseCursorPos();
    const fheroes2::Point position( pos.x, pos.y );

    for ( size_t i = 0; i < rects.size(); ++i ) {
        if ( rects[i] & position ) {
            if ( le.MouseClickLeft() )
                return static_cast<int32_t>( i );
            else
                return -1;
        }
    }

    return -1;
}

struct SelectRecipientsColors
{
    static constexpr int recipientSpacing = 22;
    const Colors colors;
    int recipients;
    std::vector<fheroes2::Rect> positions;

    SelectRecipientsColors( const fheroes2::Point & pos, int senderColor )
        : colors( Settings::Get().GetPlayers().GetActualColors() & ~senderColor )
        , recipients( 0 )
    {
        positions.reserve( colors.size() );
        const fheroes2::Display & display = fheroes2::Display::instance();
        const fheroes2::Rect box( ( display.width() - giftDialogSize.width ) / 2, ( display.height() - giftDialogSize.height ) / 2, giftDialogSize.width,
                                  giftDialogSize.height );

        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::CELLWIN, 43 );
        const int32_t colorCount = static_cast<int32_t>( colors.size() ); // safe to cast as the number of players <= 8.
        const int32_t playerContainerWidth = colorCount * sprite.width() + ( colorCount - 1 ) * recipientSpacing;
        const int32_t startX = box.x + ( giftDialogSize.width - playerContainerWidth ) / 2;
        for ( int32_t i = 0; i < colorCount; ++i ) {
            const int32_t posX = startX + i * ( recipientSpacing + sprite.width() );
            positions.emplace_back( posX, pos.y, sprite.width(), sprite.height() );
        }
    }

    int32_t GetIndexClick() const
    {
        return GetIndexClickRects( positions );
    }

    void Redraw() const
    {
        fheroes2::Display & display = fheroes2::Display::instance();

        for ( Colors::const_iterator it = colors.begin(); it != colors.end(); ++it ) {
            const fheroes2::Rect & pos = positions[std::distance( colors.begin(), it )];

            fheroes2::Blit( fheroes2::AGG::GetICN( ICN::CELLWIN, 43 + Color::GetIndex( *it ) ), display, pos.x, pos.y );
            if ( recipients & *it )
                fheroes2::Blit( fheroes2::AGG::GetICN( ICN::CELLWIN, 2 ), display, pos.x + 2, pos.y + 2 );
        }
    }

    bool QueueEventProcessing()
    {
        const int32_t index = GetIndexClick();

        if ( index >= 0 ) {
            const int cols = colors[index];

            if ( recipients & cols )
                recipients &= ~cols;
            else
                recipients |= cols;

            return true;
        }

        return false;
    }
};

struct ResourceBar
{
    Funds & resource;
    std::vector<fheroes2::Rect> positions;

    ResourceBar( Funds & funds, int32_t posx, int32_t posy )
        : resource( funds )
    {
        positions.reserve( 7 );
        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::TRADPOST, 7 );

        positions.emplace_back( posx, posy, sprite.width(), sprite.height() );
        positions.emplace_back( posx + 40, posy, sprite.width(), sprite.height() );
        positions.emplace_back( posx + 80, posy, sprite.width(), sprite.height() );
        positions.emplace_back( posx + 120, posy, sprite.width(), sprite.height() );
        positions.emplace_back( posx + 160, posy, sprite.width(), sprite.height() );
        positions.emplace_back( posx + 200, posy, sprite.width(), sprite.height() );
        positions.emplace_back( posx + 240, posy, sprite.width(), sprite.height() );
    }

    static void RedrawResource( int type, int32_t count, int32_t posx, int32_t posy )
    {
        fheroes2::Display & display = fheroes2::Display::instance();
        fheroes2::Text text( std::to_string( count ), fheroes2::FontType::smallWhite() );
        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::TRADPOST, 7 + Resource::getIconIcnIndex( type ) );
        fheroes2::Blit( sprite, display, posx, posy );
        text.draw( posx + ( sprite.width() - text.width() ) / 2, posy + sprite.height() - 10, display );
    }

    void Redraw( const Funds * res = nullptr ) const
    {
        if ( !res )
            res = &resource;

        for ( size_t i = 0; i < positions.size(); ++i ) {
            const int rs = Resource::getResourceTypeFromIconIndex( static_cast<uint32_t>( i ) );
            RedrawResource( rs, res->Get( rs ), positions[i].x, positions[i].y );
        }
    }

    int32_t GetIndexClick() const
    {
        return GetIndexClickRects( positions );
    }

    bool QueueEventProcessing( Funds & funds, uint32_t mul )
    {
        const int32_t index = GetIndexClick();

        if ( index >= 0 ) {
            int rs = Resource::getResourceTypeFromIconIndex( index );
            const int32_t step = ( rs == Resource::GOLD ) ? 100 : 1;

            int32_t cur = resource.Get( rs );
            int32_t sel = cur;
            const int32_t max = mul > 1 ? ( funds.Get( rs ) + resource.Get( rs ) ) / static_cast<int32_t>( mul ) : funds.Get( rs ) + resource.Get( rs );
            if ( 0 == mul ) {
                fheroes2::showStandardTextMessage( {}, _( "First select recipients!" ), Dialog::OK );
            }
            else if ( 0 == max ) {
                std::string msg = _( "You cannot select %{resource}!" );
                StringReplace( msg, "%{resource}", Resource::String( rs ) );
                fheroes2::showStandardTextMessage( {}, std::move( msg ), Dialog::OK );
            }
            else {
                std::string msg = _( "Select count %{resource}:" );
                StringReplace( msg, "%{resource}", Resource::String( rs ) );

                if ( Dialog::SelectCount( std::move( msg ), 0, max, sel, step ) && cur != sel ) {
                    int32_t * from = funds.GetPtr( rs );
                    int32_t * to = resource.GetPtr( rs );

                    if ( from && to ) {
                        int32_t count = sel - cur;

                        *from -= mul > 1 ? count * static_cast<int32_t>( mul ) : count;
                        *to += count;

                        return true;
                    }
                }
            }
        }

        return false;
    }
};

void Dialog::MakeGiftResource( Kingdom & kingdom )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    LocalEvent & le = LocalEvent::Get();

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    const fheroes2::StandardWindow frameborder( 320, 234, true );
    const fheroes2::Rect box( frameborder.activeArea() );

    Funds funds1( kingdom.GetFunds() );
    Funds funds2;

    const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::TRADPOST, 7 );
    const int32_t posX = box.x + ( giftDialogSize.width - ( sprite.width() + 240 ) ) / 2;

    const fheroes2::FontType normalWhite = fheroes2::FontType::normalWhite();
    fheroes2::Text text( _( "Select Recipients" ), normalWhite );
    text.draw( box.x + ( box.width - text.width() ) / 2, box.y + 7, display );
    SelectRecipientsColors selector( fheroes2::Point( box.x + 65, box.y + 28 ), kingdom.GetColor() );
    selector.Redraw();

    text.set( _( "Your Funds" ), normalWhite );
    text.draw( box.x + ( box.width - text.width() ) / 2, box.y + 57, display );
    ResourceBar info1( funds1, posX, box.y + 80 );
    info1.Redraw();

    text.set( _( "Planned Gift" ), normalWhite );
    text.draw( box.x + ( box.width - text.width() ) / 2, box.y + 127, display );
    ResourceBar info2( funds2, posX, box.y + 150 );
    info2.Redraw();

    const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();
    const int okIcnId = isEvilInterface ? ICN::BUTTON_SMALL_OKAY_EVIL : ICN::BUTTON_SMALL_OKAY_GOOD;
    const int cancelIcnId = isEvilInterface ? ICN::BUTTON_SMALL_CANCEL_EVIL : ICN::BUTTON_SMALL_CANCEL_GOOD;
    const fheroes2::Sprite & buttonOkSprite = fheroes2::AGG::GetICN( okIcnId, 0 );
    const fheroes2::Sprite & buttonCancelSprite = fheroes2::AGG::GetICN( cancelIcnId, 0 );

    const int32_t border = 10;
    fheroes2::ButtonGroup btnGroup;
    btnGroup.addButton( fheroes2::makeButtonWithShadow( box.x + border, box.y + box.height - border - buttonOkSprite.height(), buttonOkSprite,
                                                        fheroes2::AGG::GetICN( okIcnId, 1 ), display ),
                        Dialog::OK );
    btnGroup.addButton( fheroes2::makeButtonWithShadow( box.x + box.width - border - buttonCancelSprite.width(),
                                                        box.y + box.height - border - buttonCancelSprite.height(), buttonCancelSprite,
                                                        fheroes2::AGG::GetICN( cancelIcnId, 1 ), display ),
                        Dialog::CANCEL );
    btnGroup.button( 0 ).disable();

    btnGroup.draw();

    display.render();

    uint32_t count = Color::Count( selector.recipients );

    // message loop
    int result = Dialog::ZERO;
    while ( result == Dialog::ZERO && le.HandleEvents() ) {
        result = btnGroup.processEvents();

        if ( selector.QueueEventProcessing() ) {
            uint32_t new_count = Color::Count( selector.recipients );

            if ( 0 == new_count || 0 == funds2.GetValidItemsCount() )
                btnGroup.button( 0 ).disable();
            else
                btnGroup.button( 0 ).enable();

            if ( count != new_count ) {
                funds1 = kingdom.GetFunds();
                funds2.Reset();
                info1.Redraw();
                info2.Redraw();
                count = new_count;
            }

            btnGroup.draw();
            selector.Redraw();
            display.render();
        }
        else if ( info2.QueueEventProcessing( funds1, count ) ) {
            if ( 0 == Color::Count( selector.recipients ) || 0 == funds2.GetValidItemsCount() )
                btnGroup.button( 0 ).disable();
            else
                btnGroup.button( 0 ).enable();

            info1.Redraw();
            info2.Redraw();
            btnGroup.draw();
            display.render();
        }
    }

    if ( Dialog::OK == result ) {
        EventDate event;

        event.resource = funds2;
        event.isApplicableForAIPlayers = true;
        event.firstOccurrenceDay = world.CountDay() + 1;
        event.repeatPeriodInDays = 0;
        event.colors = selector.recipients;
        event.message = _( "Gift from %{name}" );
        const Player * player = Players::Get( kingdom.GetColor() );
        if ( player )
            StringReplace( event.message, "%{name}", player->GetName() );

        world.AddEventDate( event );

        if ( 1 < count )
            funds2 *= count;
        kingdom.OddFundsResource( funds2 );
    }
}
