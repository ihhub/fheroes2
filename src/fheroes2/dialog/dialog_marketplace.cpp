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

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "agg_image.h"
#include "cursor.h"
#include "dialog.h"
#include "game_hotkeys.h"
#include "icn.h"
#include "image.h"
#include "kingdom.h"
#include "localevent.h"
#include "math_base.h"
#include "players.h"
#include "resource.h"
#include "resource_trading.h"
#include "screen.h"
#include "settings.h"
#include "tools.h"
#include "translations.h"
#include "ui_button.h"
#include "ui_kingdom.h"
#include "ui_scrollbar.h"
#include "ui_text.h"
#include "ui_tool.h"
#include "world.h"

void RedrawFromResource( const fheroes2::Point &, const Funds & );
void RedrawToResource( const fheroes2::Point & pt, bool showcost, const Kingdom & kingdom, bool tradingPost, int from_resource = 0 );
std::string GetStringTradeCosts( const Kingdom & kingdom, int rs_from, int rs_to, bool tradingPost );
uint32_t GetTradeCosts( const Kingdom & kingdom, int rs_from, int rs_to, bool tradingPost );

class TradeWindowGUI
{
public:
    explicit TradeWindowGUI( const fheroes2::Rect & rt )
        : pos_rt( rt )
        , back( fheroes2::Display::instance() )
        , tradpostIcnId( Settings::Get().isEvilInterfaceEnabled() ? ICN::TRADPOSE : ICN::TRADPOST )
        , textSell( fheroes2::Display::instance() )
        , textBuy( fheroes2::Display::instance() )
        , _singlePlayer( false )
    {
        Settings & conf = Settings::Get();

        back.update( rt.x - 5, rt.y + 15, rt.width + 10, 160 );

        const bool isEvilInterface = conf.isEvilInterfaceEnabled();

        const int tradeButtonIcnID = isEvilInterface ? ICN::BUTTON_SMALL_TRADE_EVIL : ICN::BUTTON_SMALL_TRADE_GOOD;
        const int giftButtonIcnID = isEvilInterface ? ICN::BTNGIFT_EVIL : ICN::BTNGIFT_GOOD;

        buttonGift.setICNInfo( giftButtonIcnID, 0, 1 );
        buttonTrade.setICNInfo( tradeButtonIcnID, 0, 1 );
        buttonLeft.setICNInfo( tradpostIcnId, 3, 4 );
        buttonRight.setICNInfo( tradpostIcnId, 5, 6 );

        const fheroes2::Sprite & spriteGift = fheroes2::AGG::GetICN( giftButtonIcnID, 0 );
        const fheroes2::Sprite & spriteTrade = fheroes2::AGG::GetICN( tradeButtonIcnID, 0 );

        buttonGift.setPosition( pos_rt.x - 68 + ( pos_rt.width - spriteGift.width() ) / 2, pos_rt.y + pos_rt.height - spriteGift.height() );
        buttonTrade.setPosition( pos_rt.x + ( pos_rt.width - spriteTrade.width() ) / 2, pos_rt.y + 150 );
        buttonLeft.setPosition( pos_rt.x + 11, pos_rt.y + 129 );
        buttonRight.setPosition( pos_rt.x + 220, pos_rt.y + 129 );
        _scrollbar.setImage( fheroes2::AGG::GetICN( tradpostIcnId, 2 ) );
        _scrollbar.setArea( { pos_rt.x + ( pos_rt.width - fheroes2::AGG::GetICN( tradpostIcnId, 1 ).width() ) / 2 + 22, pos_rt.y + 131, 187, 11 } );
        _scrollbar.hide();

        const fheroes2::Text text( _( "Please inspect our fine wares. If you feel like offering a trade, click on the items you wish to trade with and for." ),
                                   fheroes2::FontType::normalWhite() );
        text.draw( pos_rt.x, pos_rt.y + 32, pos_rt.width, fheroes2::Display::instance() );

        const Players & players = conf.GetPlayers();
        int playerCount = 0;
        for ( const Player * player : players ) {
            if ( player != nullptr ) {
                const Kingdom & kingdom = world.GetKingdom( player->GetColor() );
                if ( kingdom.isPlay() )
                    ++playerCount;
            }
        }

        _singlePlayer = playerCount == 1;
    }

    void RedrawInfoBuySell( uint32_t count_sell, uint32_t count_buy, uint32_t max_sell, uint32_t orig_buy );
    void ShowTradeArea( const Kingdom & kingdom, int resourceFrom, int resourceTo, uint32_t max_buy, uint32_t max_sell, uint32_t count_buy, uint32_t count_sell,
                        const bool fromTradingPost, const bool firstExchange );

    fheroes2::Rect buttonMax;
    fheroes2::Rect buttonMin;
    fheroes2::Button buttonTrade;
    fheroes2::Button buttonLeft;
    fheroes2::Button buttonRight;
    fheroes2::Button buttonGift;
    fheroes2::Scrollbar _scrollbar;

private:
    fheroes2::Rect pos_rt;
    fheroes2::ImageRestorer back;
    const int tradpostIcnId;

    fheroes2::MovableText textSell;
    fheroes2::MovableText textBuy;
    bool _singlePlayer;
};

void TradeWindowGUI::ShowTradeArea( const Kingdom & kingdom, int resourceFrom, int resourceTo, uint32_t max_buy, uint32_t max_sell, uint32_t count_buy,
                                    uint32_t count_sell, const bool fromTradingPost, const bool firstExchange )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    bool disable = kingdom.GetFunds().Get( resourceFrom ) <= 0;

    if ( disable || resourceFrom == resourceTo || ( Resource::GOLD != resourceTo && 0 == max_buy ) ) {
        _scrollbar.hide();
        back.restore();
        fheroes2::Rect dst_rt( pos_rt.x, pos_rt.y + 30, pos_rt.width, 100 );
        std::string message = firstExchange && ( resourceFrom == resourceTo || 0 == max_buy )
                                  ? _( "Please inspect our fine wares. If you feel like offering a trade, click on the items you wish to trade with and for." )
                                  : _( "You have received quite a bargain. I expect to make no profit on the deal. Can I interest you in any of my other wares?" );

        const fheroes2::Text displayMessage( std::move( message ), fheroes2::FontType::normalWhite() );
        displayMessage.draw( dst_rt.x, dst_rt.y + 2, dst_rt.width, display );

        if ( !_singlePlayer ) {
            buttonGift.enable();
        }
        buttonTrade.disable();
        buttonLeft.disable();
        buttonRight.disable();
        buttonGift.draw();
        buttonMax = fheroes2::Rect();
        buttonMin = fheroes2::Rect();
    }
    else {
        back.restore();

        const fheroes2::Sprite & bar = fheroes2::AGG::GetICN( tradpostIcnId, 1 );
        fheroes2::Point dst_pt( pos_rt.x + ( pos_rt.width - bar.width() ) / 2 - 2, pos_rt.y + 128 );
        fheroes2::Blit( bar, display, dst_pt.x, dst_pt.y );

        const uint32_t maximumValue = ( Resource::GOLD == resourceTo ) ? max_sell : max_buy;

        const fheroes2::Sprite & originalSlider = fheroes2::AGG::GetICN( tradpostIcnId, 2 );
        const fheroes2::Image scrollbarSlider = fheroes2::generateScrollbarSlider( originalSlider, true, 187, 1, static_cast<int32_t>( maximumValue + 1 ),
                                                                                   { 0, 0, 2, originalSlider.height() }, { 2, 0, 8, originalSlider.height() } );
        _scrollbar.setImage( scrollbarSlider );

        _scrollbar.setRange( 0, maximumValue );

        const uint32_t exchange_rate = GetTradeCosts( kingdom, resourceFrom, resourceTo, fromTradingPost );
        std::string message;
        if ( Resource::GOLD == resourceTo ) {
            message = _( "I can offer you %{count} for 1 unit of %{resfrom}." );
            StringReplace( message, "%{count}", exchange_rate );
            StringReplace( message, "%{resfrom}", Resource::String( resourceFrom ) );
        }
        else {
            message = _( "I can offer you 1 unit of %{resto} for %{count} units of %{resfrom}." );
            StringReplace( message, "%{resto}", Resource::String( resourceTo ) );
            StringReplace( message, "%{resfrom}", Resource::String( resourceFrom ) );
            StringReplace( message, "%{count}", exchange_rate );
        }

        const fheroes2::Text displayMessage( std::move( message ), fheroes2::FontType::normalWhite() );
        displayMessage.draw( pos_rt.x, pos_rt.y + 32, pos_rt.width, display );

        const fheroes2::Sprite & sprite_from = fheroes2::AGG::GetICN( ICN::RESOURCE, Resource::getIconIcnIndex( resourceFrom ) );
        dst_pt.x = pos_rt.x + ( pos_rt.width - sprite_from.width() + 1 ) / 2 - 70;
        dst_pt.y = pos_rt.y + 115 - sprite_from.height();
        fheroes2::Blit( sprite_from, display, dst_pt.x, dst_pt.y );
        const fheroes2::Sprite & sprite_to = fheroes2::AGG::GetICN( ICN::RESOURCE, Resource::getIconIcnIndex( resourceTo ) );
        dst_pt.x = pos_rt.x + ( pos_rt.width - sprite_to.width() + 1 ) / 2 + 70;
        dst_pt.y = pos_rt.y + 115 - sprite_to.height();
        fheroes2::Blit( sprite_to, display, dst_pt.x, dst_pt.y );
        const fheroes2::Sprite & sprite_fromto = fheroes2::AGG::GetICN( tradpostIcnId, 0 );
        dst_pt.x = pos_rt.x + ( pos_rt.width - sprite_fromto.width() ) / 2;
        dst_pt.y = pos_rt.y + 90;
        fheroes2::Blit( sprite_fromto, display, dst_pt.x, dst_pt.y );
        fheroes2::Text text( _( "Max" ), fheroes2::FontType::smallYellow() );
        dst_pt.x = pos_rt.x + ( pos_rt.width - text.width() ) / 2 - 5;
        dst_pt.y = pos_rt.y + 80;
        buttonMax = fheroes2::Rect( dst_pt.x, dst_pt.y, text.width(), text.height() );
        text.draw( dst_pt.x, dst_pt.y + 2, display );
        text.set( _( "Min" ), fheroes2::FontType::smallYellow() );
        dst_pt.x = pos_rt.x + ( pos_rt.width - text.width() ) / 2 - 5;
        dst_pt.y = pos_rt.y + 103;
        buttonMin = fheroes2::Rect( dst_pt.x, dst_pt.y, text.width(), text.height() );
        text.draw( dst_pt.x, dst_pt.y + 2, display );
        text.set( _( "Qty to trade" ), fheroes2::FontType::smallWhite() );
        dst_pt.x = pos_rt.x + ( pos_rt.width - text.width() ) / 2;
        dst_pt.y = pos_rt.y + 115;
        text.draw( dst_pt.x, dst_pt.y + 2, display );

        buttonGift.enable();
        buttonTrade.enable();
        buttonLeft.enable();
        buttonRight.enable();

        buttonTrade.draw();
        buttonLeft.draw();
        buttonRight.draw();

        RedrawInfoBuySell( count_sell, count_buy, max_sell, kingdom.GetFunds().Get( resourceTo ) );
        _scrollbar.show();
    }

    display.render();
}

void TradeWindowGUI::RedrawInfoBuySell( uint32_t count_sell, uint32_t count_buy, uint32_t max_sell, uint32_t orig_buy )
{
    fheroes2::Point dst_pt;

    _scrollbar.hide();

    auto text = std::make_unique<fheroes2::Text>( std::string( "-" ) + std::to_string( count_sell ) + " " + "(" + std::to_string( max_sell - count_sell ) + ")",
                                                  fheroes2::FontType::smallWhite() );

    int32_t textWidth = text->width();

    textSell.update( std::move( text ) );
    dst_pt.x = pos_rt.x + pos_rt.width / 2 - 70 - ( textWidth + 1 ) / 2;
    dst_pt.y = pos_rt.y + 116;
    textSell.draw( dst_pt.x, dst_pt.y );

    text = std::make_unique<fheroes2::Text>( std::string( "+" ) + std::to_string( count_buy ) + " " + "(" + std::to_string( orig_buy + count_buy ) + ")",
                                             fheroes2::FontType::smallWhite() );

    textWidth = text->width();

    textBuy.update( std::move( text ) );
    dst_pt.x = pos_rt.x + pos_rt.width / 2 + 70 - ( textWidth + 1 ) / 2;
    dst_pt.y = pos_rt.y + 116;
    textBuy.draw( dst_pt.x, dst_pt.y );

    _scrollbar.show();
}

void Dialog::Marketplace( Kingdom & kingdom, bool fromTradingPost )
{
    fheroes2::Display & display = fheroes2::Display::instance();

    const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();
    const int tradpost = isEvilInterface ? ICN::TRADPOSE : ICN::TRADPOST;
    const std::string & header = fromTradingPost ? _( "Trading Post" ) : _( "Marketplace" );

    // setup cursor
    const CursorRestorer cursorRestorer( true, Cursor::POINTER );

    Dialog::FrameBox box( 297, true );

    const fheroes2::Rect & pos_rt = box.GetArea();
    fheroes2::Point dst_pt( pos_rt.x, pos_rt.y );

    // header
    fheroes2::Text text{ header, fheroes2::FontType::normalYellow() };
    dst_pt.x = pos_rt.x + ( pos_rt.width - text.width() ) / 2;
    dst_pt.y = pos_rt.y;
    text.draw( dst_pt.x, dst_pt.y + 2, display );

    TradeWindowGUI gui( pos_rt );

    Funds fundsFrom = kingdom.GetFunds();
    int resourceFrom = 0;
    const fheroes2::Point pt1( pos_rt.x, pos_rt.y + 190 );
    std::vector<fheroes2::Rect> rectsFrom;
    rectsFrom.reserve( 7 );
    rectsFrom.emplace_back( pt1.x, pt1.y, 34, 34 ); // wood
    rectsFrom.emplace_back( pt1.x + 37, pt1.y, 34, 34 ); // mercury
    rectsFrom.emplace_back( pt1.x + 74, pt1.y, 34, 34 ); // ore
    rectsFrom.emplace_back( pt1.x, pt1.y + 37, 34, 34 ); // sulfur
    rectsFrom.emplace_back( pt1.x + 37, pt1.y + 37, 34, 34 ); // crystal
    rectsFrom.emplace_back( pt1.x + 74, pt1.y + 37, 34, 34 ); // gems
    rectsFrom.emplace_back( pt1.x + 37, pt1.y + 74, 34, 34 ); // gold

    fheroes2::MovableSprite cursorFrom( fheroes2::AGG::GetICN( tradpost, 14 ) );
    text.set( _( "Your Resources" ), fheroes2::FontType::smallWhite() );
    dst_pt.x = pt1.x + ( 108 - text.width() ) / 2;
    dst_pt.y = pt1.y - 15;
    text.draw( dst_pt.x, dst_pt.y + 2, display );
    RedrawFromResource( pt1, fundsFrom );

    int resourceTo = 0;
    const fheroes2::Point pt2( 136 + pos_rt.x, pos_rt.y + 190 );
    std::vector<fheroes2::Rect> rectsTo;
    rectsTo.reserve( 7 );
    rectsTo.emplace_back( pt2.x, pt2.y, 34, 34 ); // wood
    rectsTo.emplace_back( pt2.x + 37, pt2.y, 34, 34 ); // mercury
    rectsTo.emplace_back( pt2.x + 74, pt2.y, 34, 34 ); // ore
    rectsTo.emplace_back( pt2.x, pt2.y + 37, 34, 34 ); // sulfur
    rectsTo.emplace_back( pt2.x + 37, pt2.y + 37, 34, 34 ); // crystal
    rectsTo.emplace_back( pt2.x + 74, pt2.y + 37, 34, 34 ); // gems
    rectsTo.emplace_back( pt2.x + 37, pt2.y + 74, 34, 34 ); // gold

    fheroes2::MovableSprite cursorTo( fheroes2::AGG::GetICN( tradpost, 14 ) );
    text.set( _( "Available Trades" ), fheroes2::FontType::smallWhite() );
    dst_pt.x = pt2.x + ( 108 - text.width() ) / 2;
    dst_pt.y = pt2.y - 15;
    text.draw( dst_pt.x, dst_pt.y + 2, display );
    RedrawToResource( pt2, false, kingdom, fromTradingPost );

    uint32_t count_sell = 0;
    uint32_t count_buy = 0;

    uint32_t max_sell = 0;
    uint32_t max_buy = 0;

    const fheroes2::Rect & buttonMax = gui.buttonMax;
    const fheroes2::Rect & buttonMin = gui.buttonMin;
    fheroes2::Button & buttonGift = gui.buttonGift;
    fheroes2::Button & buttonTrade = gui.buttonTrade;
    fheroes2::Button & buttonLeft = gui.buttonLeft;
    fheroes2::Button & buttonRight = gui.buttonRight;

    fheroes2::TimedEventValidator timedButtonLeft( [&buttonLeft]() { return buttonLeft.isPressed(); } );
    fheroes2::TimedEventValidator timedButtonRight( [&buttonRight]() { return buttonRight.isPressed(); } );
    buttonLeft.subscribe( &timedButtonLeft );
    buttonRight.subscribe( &timedButtonRight );

    fheroes2::Scrollbar & scrollbar = gui._scrollbar;

    // button exit
    const int exitButtonIcnID = isEvilInterface ? ICN::UNIFORM_EVIL_EXIT_BUTTON : ICN::UNIFORM_GOOD_EXIT_BUTTON;
    const fheroes2::Sprite & spriteExit = fheroes2::AGG::GetICN( exitButtonIcnID, 0 );

    dst_pt.x = pos_rt.x + 68 + ( pos_rt.width - spriteExit.width() ) / 2;
    dst_pt.y = pos_rt.y + pos_rt.height - spriteExit.height();
    fheroes2::Button buttonExit( dst_pt.x, dst_pt.y, exitButtonIcnID, 0, 1 );

    buttonGift.draw();
    buttonExit.draw();
    buttonTrade.disable();
    display.render();

    LocalEvent & le = LocalEvent::Get();

    bool firstExchange = true;

    // message loop
    while ( le.HandleEvents() ) {
        if ( buttonGift.isEnabled() )
            le.isMouseLeftButtonPressedInArea( buttonGift.area() ) ? buttonGift.drawOnPress() : buttonGift.drawOnRelease();
        if ( buttonTrade.isEnabled() )
            le.isMouseLeftButtonPressedInArea( buttonTrade.area() ) ? buttonTrade.drawOnPress() : buttonTrade.drawOnRelease();
        if ( buttonLeft.isEnabled() )
            le.isMouseLeftButtonPressedInArea( buttonLeft.area() ) ? buttonLeft.drawOnPress() : buttonLeft.drawOnRelease();
        if ( buttonRight.isEnabled() )
            le.isMouseLeftButtonPressedInArea( buttonRight.area() ) ? buttonRight.drawOnPress() : buttonRight.drawOnRelease();

        le.isMouseLeftButtonPressedInArea( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();

        if ( le.MouseClickLeft( buttonExit.area() ) || Game::HotKeyCloseWindow() )
            break;

        // gift resources
        if ( buttonGift.isEnabled() && le.MouseClickLeft( buttonGift.area() ) ) {
            Dialog::MakeGiftResource( kingdom );

            resourceTo = Resource::UNKNOWN;
            resourceFrom = Resource::UNKNOWN;
            gui.ShowTradeArea( kingdom, resourceFrom, resourceTo, 0, 0, 0, 0, fromTradingPost, firstExchange );

            cursorTo.hide();
            cursorFrom.hide();

            fundsFrom = kingdom.GetFunds();
            RedrawFromResource( pt1, fundsFrom );

            display.render();
        }

        // click from
        for ( uint32_t ii = 0; ii < rectsFrom.size(); ++ii ) {
            const fheroes2::Rect & rect_from = rectsFrom[ii];

            if ( le.MouseClickLeft( rect_from ) ) {
                resourceFrom = Resource::getResourceTypeFromIconIndex( ii );
                max_sell = fundsFrom.Get( resourceFrom );

                const uint32_t tradeCost = GetTradeCosts( kingdom, resourceFrom, resourceTo, fromTradingPost );

                if ( tradeCost ) {
                    max_buy = Resource::GOLD == resourceTo ? max_sell * tradeCost : max_sell / tradeCost;
                }

                count_sell = 0;
                count_buy = 0;

                cursorFrom.show();
                cursorFrom.setPosition( rect_from.x - 2, rect_from.y - 2 );

                if ( resourceTo ) {
                    cursorTo.hide();
                }
                RedrawToResource( pt2, true, kingdom, fromTradingPost, resourceFrom );
                if ( resourceTo ) {
                    cursorTo.show();
                    gui.ShowTradeArea( kingdom, resourceFrom, resourceTo, max_buy, max_sell, count_buy, count_sell, fromTradingPost, firstExchange );
                }

                display.render();
            }
            else if ( le.isMouseRightButtonPressedInArea( rect_from ) ) {
                fheroes2::showKingdomIncome( kingdom, 0 );
            }
        }

        // click to
        for ( uint32_t ii = 0; ii < rectsTo.size(); ++ii ) {
            const fheroes2::Rect & rect_to = rectsTo[ii];

            if ( le.MouseClickLeft( rect_to ) ) {
                resourceTo = Resource::getResourceTypeFromIconIndex( ii );

                const uint32_t tradeCost = GetTradeCosts( kingdom, resourceFrom, resourceTo, fromTradingPost );

                if ( tradeCost ) {
                    max_buy = Resource::GOLD == resourceTo ? max_sell * tradeCost : max_sell / tradeCost;
                }

                count_sell = 0;
                count_buy = 0;

                cursorTo.show();
                cursorTo.setPosition( rect_to.x - 2, rect_to.y - 2 );

                if ( resourceFrom ) {
                    cursorTo.hide();
                    RedrawToResource( pt2, true, kingdom, fromTradingPost, resourceFrom );
                    cursorTo.show();
                    gui.ShowTradeArea( kingdom, resourceFrom, resourceTo, max_buy, max_sell, count_buy, count_sell, fromTradingPost, firstExchange );
                }
                display.render();
            }
        }

        // Scrollbar
        if ( buttonLeft.isEnabled() && buttonRight.isEnabled() && max_buy && le.isMouseLeftButtonPressedInArea( scrollbar.getArea() ) ) {
            const fheroes2::Point & mousePos = le.getMouseCursorPos();
            scrollbar.moveToPos( mousePos );
            const int32_t seek = scrollbar.currentIndex();

            count_buy = seek * ( Resource::GOLD == resourceTo ? GetTradeCosts( kingdom, resourceFrom, resourceTo, fromTradingPost ) : 1 );
            count_sell = seek * ( Resource::GOLD == resourceTo ? 1 : GetTradeCosts( kingdom, resourceFrom, resourceTo, fromTradingPost ) );

            gui.RedrawInfoBuySell( count_sell, count_buy, max_sell, fundsFrom.Get( resourceTo ) );
            display.render();
        }
        else if ( scrollbar.updatePosition() ) {
            display.render();
        }

        // click max
        if ( buttonMax.width && max_buy && le.MouseClickLeft( buttonMax ) ) {
            const int32_t max = scrollbar.maxIndex();

            count_buy = max * ( Resource::GOLD == resourceTo ? GetTradeCosts( kingdom, resourceFrom, resourceTo, fromTradingPost ) : 1 );
            count_sell = max * ( Resource::GOLD == resourceTo ? 1 : GetTradeCosts( kingdom, resourceFrom, resourceTo, fromTradingPost ) );

            scrollbar.moveToIndex( max );
            gui.RedrawInfoBuySell( count_sell, count_buy, max_sell, fundsFrom.Get( resourceTo ) );
            display.render();
        }

        // click min
        if ( buttonMin.width && max_buy && le.MouseClickLeft( buttonMin ) ) {
            const int32_t min = 1;

            count_buy = min * ( Resource::GOLD == resourceTo ? GetTradeCosts( kingdom, resourceFrom, resourceTo, fromTradingPost ) : 1 );
            count_sell = min * ( Resource::GOLD == resourceTo ? 1 : GetTradeCosts( kingdom, resourceFrom, resourceTo, fromTradingPost ) );

            scrollbar.moveToIndex( min );
            gui.RedrawInfoBuySell( count_sell, count_buy, max_sell, fundsFrom.Get( resourceTo ) );
            display.render();
        }

        // trade
        if ( buttonTrade.isEnabled() && le.MouseClickLeft( buttonTrade.area() ) && count_sell && count_buy ) {
            kingdom.OddFundsResource( Funds( resourceFrom, count_sell ) );
            kingdom.AddFundsResource( Funds( resourceTo, count_buy ) );

            firstExchange = false;

            resourceTo = Resource::UNKNOWN;
            resourceFrom = Resource::UNKNOWN;
            gui.ShowTradeArea( kingdom, resourceFrom, resourceTo, 0, 0, 0, 0, fromTradingPost, firstExchange );

            fundsFrom = kingdom.GetFunds();
            cursorTo.hide();
            cursorFrom.hide();
            RedrawFromResource( pt1, fundsFrom );
            RedrawToResource( pt2, false, kingdom, fromTradingPost, resourceFrom );
            display.render();
        }

        // decrease trade resource
        if ( count_buy
             && ( ( buttonLeft.isEnabled() && ( le.MouseClickLeft( gui.buttonLeft.area() ) || timedButtonLeft.isDelayPassed() ) )
                  || le.isMouseWheelDownInArea( scrollbar.getArea() ) ) ) {
            count_buy -= Resource::GOLD == resourceTo ? GetTradeCosts( kingdom, resourceFrom, resourceTo, fromTradingPost ) : 1;

            count_sell -= Resource::GOLD == resourceTo ? 1 : GetTradeCosts( kingdom, resourceFrom, resourceTo, fromTradingPost );

            scrollbar.backward();
            gui.RedrawInfoBuySell( count_sell, count_buy, max_sell, fundsFrom.Get( resourceTo ) );
            display.render();
        }

        // increase trade resource
        if ( count_buy < max_buy
             && ( ( buttonRight.isEnabled() && ( le.MouseClickLeft( buttonRight.area() ) || timedButtonRight.isDelayPassed() ) )
                  || le.isMouseWheelUpInArea( scrollbar.getArea() ) ) ) {
            count_buy += Resource::GOLD == resourceTo ? GetTradeCosts( kingdom, resourceFrom, resourceTo, fromTradingPost ) : 1;

            count_sell += Resource::GOLD == resourceTo ? 1 : GetTradeCosts( kingdom, resourceFrom, resourceTo, fromTradingPost );

            scrollbar.forward();
            gui.RedrawInfoBuySell( count_sell, count_buy, max_sell, fundsFrom.Get( resourceTo ) );
            display.render();
        }
    }
}

void RedrawResourceSprite( const fheroes2::Image & sf, int32_t px, int32_t py, int32_t value )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    fheroes2::Blit( sf, display, px, py );

    const fheroes2::Text text( std::to_string( value ), fheroes2::FontType::smallWhite() );
    text.draw( px + ( 34 - text.width() ) / 2, py + 23, display );
}

void RedrawFromResource( const fheroes2::Point & pt, const Funds & rs )
{
    const int tradpost = Settings::Get().isEvilInterfaceEnabled() ? ICN::TRADPOSE : ICN::TRADPOST;

    // wood
    RedrawResourceSprite( fheroes2::AGG::GetICN( tradpost, 7 ), pt.x, pt.y, rs.wood );
    // mercury
    RedrawResourceSprite( fheroes2::AGG::GetICN( tradpost, 8 ), pt.x + 37, pt.y, rs.mercury );
    // ore
    RedrawResourceSprite( fheroes2::AGG::GetICN( tradpost, 9 ), pt.x + 74, pt.y, rs.ore );
    // sulfur
    RedrawResourceSprite( fheroes2::AGG::GetICN( tradpost, 10 ), pt.x, pt.y + 37, rs.sulfur );
    // crystal
    RedrawResourceSprite( fheroes2::AGG::GetICN( tradpost, 11 ), pt.x + 37, pt.y + 37, rs.crystal );
    // gems
    RedrawResourceSprite( fheroes2::AGG::GetICN( tradpost, 12 ), pt.x + 74, pt.y + 37, rs.gems );
    // gold
    RedrawResourceSprite( fheroes2::AGG::GetICN( tradpost, 13 ), pt.x + 37, pt.y + 74, rs.gold );
}

void RedrawResourceSprite2( const fheroes2::Image & sf, int32_t px, int32_t py, bool show, const Kingdom & kingdom, int from, int res, bool trading )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    fheroes2::Blit( sf, display, px, py );

    if ( show ) {
        const fheroes2::Text text( GetStringTradeCosts( kingdom, from, res, trading ), fheroes2::FontType::smallWhite() );
        text.draw( px + ( 34 - text.width() ) / 2, py + 23, display );
    }
}

void RedrawToResource( const fheroes2::Point & pt, bool showcost, const Kingdom & kingdom, bool tradingPost, int from_resource )
{
    const int tradpost = Settings::Get().isEvilInterfaceEnabled() ? ICN::TRADPOSE : ICN::TRADPOST;

    // wood
    RedrawResourceSprite2( fheroes2::AGG::GetICN( tradpost, 7 ), pt.x, pt.y, showcost, kingdom, from_resource, Resource::WOOD, tradingPost );
    // mercury
    RedrawResourceSprite2( fheroes2::AGG::GetICN( tradpost, 8 ), pt.x + 37, pt.y, showcost, kingdom, from_resource, Resource::MERCURY, tradingPost );
    // ore
    RedrawResourceSprite2( fheroes2::AGG::GetICN( tradpost, 9 ), pt.x + 74, pt.y, showcost, kingdom, from_resource, Resource::ORE, tradingPost );
    // sulfur
    RedrawResourceSprite2( fheroes2::AGG::GetICN( tradpost, 10 ), pt.x, pt.y + 37, showcost, kingdom, from_resource, Resource::SULFUR, tradingPost );
    // crystal
    RedrawResourceSprite2( fheroes2::AGG::GetICN( tradpost, 11 ), pt.x + 37, pt.y + 37, showcost, kingdom, from_resource, Resource::CRYSTAL, tradingPost );
    // gems
    RedrawResourceSprite2( fheroes2::AGG::GetICN( tradpost, 12 ), pt.x + 74, pt.y + 37, showcost, kingdom, from_resource, Resource::GEMS, tradingPost );
    // gold
    RedrawResourceSprite2( fheroes2::AGG::GetICN( tradpost, 13 ), pt.x + 37, pt.y + 74, showcost, kingdom, from_resource, Resource::GOLD, tradingPost );
}

std::string GetStringTradeCosts( const Kingdom & kingdom, int rs_from, int rs_to, bool tradingPost )
{
    std::string res;

    if ( rs_from == rs_to ) {
        res = _( "n/a" );
    }
    else {
        res = "1/";
        res.append( std::to_string( GetTradeCosts( kingdom, rs_from, rs_to, tradingPost ) ) );
    }

    return res;
}

uint32_t GetTradeCosts( const Kingdom & kingdom, int rs_from, int rs_to, bool tradingPost )
{
    return fheroes2::getTradeCost( ( tradingPost ? 3 : kingdom.GetCountMarketplace() ), rs_from, rs_to );
}
