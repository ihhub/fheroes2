/*********[6~******************************************************************
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

#include <string>
#include <vector>

#include "agg.h"
#include "cursor.h"
#include "dialog.h"
#include "game.h"
#include "kingdom.h"
#include "marketplace.h"
#include "resource.h"
#include "settings.h"
#include "text.h"
#include "ui_button.h"
#include "ui_scrollbar.h"
#include "world.h"

void RedrawFromResource( const fheroes2::Point &, const Funds & );
void RedrawToResource( const fheroes2::Point &, bool showcost, bool tradingPost, int from_resource = 0 );
std::string GetStringTradeCosts( int rs_from, int rs_to, bool tradingPost );
u32 GetTradeCosts( int rs_from, int rs_to, bool tradingPost );

class TradeWindowGUI
{
public:
    TradeWindowGUI( const fheroes2::Rect & rt )
        : pos_rt( rt )
        , back( fheroes2::Display::instance() )
        , tradpost( Settings::Get().ExtGameEvilInterface() ? ICN::TRADPOSE : ICN::TRADPOST )
        , _singlePlayer( false )
    {
        Settings & conf = Settings::Get();

        back.update( rt.x - 5, rt.y + 15, rt.width + 10, 160 );

        buttonGift.setICNInfo( conf.ExtGameEvilInterface() ? ICN::BTNGIFT_EVIL : ICN::BTNGIFT_GOOD, 0, 1 );
        buttonTrade.setICNInfo( tradpost, 15, 16 );
        buttonLeft.setICNInfo( tradpost, 3, 4 );
        buttonRight.setICNInfo( tradpost, 5, 6 );

        buttonGift.setPosition( pos_rt.x + ( pos_rt.width - fheroes2::AGG::GetICN( tradpost, 17 ).width() ) / 2, pos_rt.y + 120 );
        buttonTrade.setPosition( pos_rt.x + ( pos_rt.width - fheroes2::AGG::GetICN( tradpost, 17 ).width() ) / 2, pos_rt.y + 150 );
        buttonLeft.setPosition( pos_rt.x + 11, pos_rt.y + 129 );
        buttonRight.setPosition( pos_rt.x + 220, pos_rt.y + 129 );

        _scrollbar.setImage( fheroes2::AGG::GetICN( tradpost, 2 ) );
        _scrollbar.setArea( fheroes2::Rect( pos_rt.x + ( pos_rt.width - fheroes2::AGG::GetICN( tradpost, 1 ).width() ) / 2 + 22, pos_rt.y + 131, 187, 11 ) );
        _scrollbar.hide();

        const TextBox text( _( "Please inspect our fine wares. If you feel like offering a trade, click on the items you wish to trade with and for." ), Font::BIG,
                            fheroes2::Rect( pos_rt.x, pos_rt.y + 30, pos_rt.width, 100 ) );

        textSell.SetFont( Font::SMALL );
        textBuy.SetFont( Font::SMALL );

        const std::vector<Player *> players = conf.GetPlayers();
        int playerCount = 0;
        for ( Players::const_iterator it = players.begin(); it != players.end(); ++it ) {
            if ( *it ) {
                const Player & player = ( **it );
                Kingdom & kingdom = world.GetKingdom( player.GetColor() );
                if ( kingdom.isPlay() )
                    ++playerCount;
            }
        }

        _singlePlayer = playerCount == 1;
    };

    void RedrawInfoBuySell( u32 count_sell, u32 count_buy, u32 max_sell, u32 orig_buy );
    void ShowTradeArea( int resourceFrom, int resourceTo, u32 max_buy, u32 max_sell, u32 count_buy, u32 count_sell, bool fromTradingPost );

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
    int tradpost;

    TextSprite textSell;
    TextSprite textBuy;
    bool _singlePlayer;
};

void TradeWindowGUI::ShowTradeArea( int resourceFrom, int resourceTo, u32 max_buy, u32 max_sell, u32 count_buy, u32 count_sell, bool fromTradingPost )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    Cursor & cursor = Cursor::Get();
    bool disable = world.GetKingdom( Settings::Get().CurrentColor() ).GetFunds().Get( resourceFrom ) <= 0;

    if ( disable || resourceFrom == resourceTo || ( Resource::GOLD != resourceTo && 0 == max_buy ) ) {
        cursor.Hide();
        _scrollbar.hide();
        back.restore();
        fheroes2::Rect dst_rt( pos_rt.x, pos_rt.y + 30, pos_rt.width, 100 );
        const TextBox displayMesssage( _( "You have received quite a bargain. I expect to make no profit on the deal. Can I interest you in any of my other wares?" ),
                                       Font::BIG, dst_rt );

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
        cursor.Hide();
        back.restore();

        const fheroes2::Sprite & bar = fheroes2::AGG::GetICN( tradpost, 1 );
        fheroes2::Point dst_pt( pos_rt.x + ( pos_rt.width - bar.width() ) / 2 - 2, pos_rt.y + 128 );
        fheroes2::Blit( bar, display, dst_pt.x, dst_pt.y );

        _scrollbar.setRange( 0, ( Resource::GOLD == resourceTo ? max_sell : max_buy ) );
        const uint32_t exchange_rate = GetTradeCosts( resourceFrom, resourceTo, fromTradingPost );
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
        const TextBox displayMessage( message, Font::BIG, fheroes2::Rect( pos_rt.x, pos_rt.y + 30, pos_rt.width, 100 ) );
        const fheroes2::Sprite & sprite_from = fheroes2::AGG::GetICN( ICN::RESOURCE, Resource::GetIndexSprite2( resourceFrom ) );
        dst_pt.x = pos_rt.x + ( pos_rt.width - sprite_from.width() ) / 2 - 70;
        dst_pt.y = pos_rt.y + 115 - sprite_from.height();
        fheroes2::Blit( sprite_from, display, dst_pt.x, dst_pt.y );
        const fheroes2::Sprite & sprite_to = fheroes2::AGG::GetICN( ICN::RESOURCE, Resource::GetIndexSprite2( resourceTo ) );
        dst_pt.x = pos_rt.x + ( pos_rt.width - sprite_to.width() ) / 2 + 70;
        dst_pt.y = pos_rt.y + 115 - sprite_to.height();
        fheroes2::Blit( sprite_to, display, dst_pt.x, dst_pt.y );
        const fheroes2::Sprite & sprite_fromto = fheroes2::AGG::GetICN( tradpost, 0 );
        dst_pt.x = pos_rt.x + ( pos_rt.width - sprite_fromto.width() ) / 2;
        dst_pt.y = pos_rt.y + 90;
        fheroes2::Blit( sprite_fromto, display, dst_pt.x, dst_pt.y );
        Text text( "max", Font::YELLOW_SMALL );
        dst_pt.x = pos_rt.x + ( pos_rt.width - text.w() ) / 2 - 5;
        dst_pt.y = pos_rt.y + 80;
        buttonMax = fheroes2::Rect( dst_pt.x, dst_pt.y, text.w(), text.h() );
        text.Blit( dst_pt.x, dst_pt.y );
        text.Set( "min", Font::YELLOW_SMALL );
        dst_pt.x = pos_rt.x + ( pos_rt.width - text.w() ) / 2 - 5;
        dst_pt.y = pos_rt.y + 103;
        buttonMin = fheroes2::Rect( dst_pt.x, dst_pt.y, text.w(), text.h() );
        text.Blit( dst_pt.x, dst_pt.y );
        text.Set( _( "Qty to trade" ), Font::SMALL );
        dst_pt.x = pos_rt.x + ( pos_rt.width - text.w() ) / 2;
        dst_pt.y = pos_rt.y + 115;
        text.Blit( dst_pt.x, dst_pt.y );

        buttonGift.disable();
        buttonTrade.enable();
        buttonLeft.enable();
        buttonRight.enable();

        buttonTrade.draw();
        buttonLeft.draw();
        buttonRight.draw();

        RedrawInfoBuySell( count_sell, count_buy, max_sell, world.GetKingdom( Settings::Get().CurrentColor() ).GetFunds().Get( resourceTo ) );
        _scrollbar.show();
    }

    cursor.Show();
    display.render();
}

void TradeWindowGUI::RedrawInfoBuySell( u32 count_sell, u32 count_buy, u32 max_sell, u32 orig_buy )
{
    fheroes2::Point dst_pt;

    _scrollbar.hide();

    textSell.Hide();
    textSell.SetText( std::string( "-" ) + GetString( count_sell ) + " " + "(" + GetString( max_sell - count_sell ) + ")" );
    dst_pt.x = pos_rt.x + pos_rt.width / 2 - 70 - textSell.w() / 2;
    dst_pt.y = pos_rt.y + 116;
    textSell.SetPos( dst_pt.x, dst_pt.y );
    textSell.Show();

    textBuy.Hide();
    textBuy.SetText( std::string( "+" ) + GetString( count_buy ) + " " + "(" + GetString( orig_buy + count_buy ) + ")" );
    dst_pt.x = pos_rt.x + pos_rt.width / 2 + 70 - textBuy.w() / 2;
    dst_pt.y = pos_rt.y + 116;
    textBuy.SetPos( dst_pt.x, dst_pt.y );
    textBuy.Show();

    _scrollbar.show();
}

void Dialog::Marketplace( bool fromTradingPost )
{
    fheroes2::Display & display = fheroes2::Display::instance();
    const int tradpost = Settings::Get().ExtGameEvilInterface() ? ICN::TRADPOSE : ICN::TRADPOST;
    const std::string & header = fromTradingPost ? _( "Trading Post" ) : _( "Marketplace" );

    Cursor & cursor = Cursor::Get();
    cursor.Hide();
    cursor.SetThemes( cursor.POINTER );

    Dialog::FrameBox box( 297, true );

    const fheroes2::Rect & pos_rt = box.GetArea();
    fheroes2::Point dst_pt( pos_rt.x, pos_rt.y );

    // header
    Text text;
    text.Set( header, Font::YELLOW_BIG );
    dst_pt.x = pos_rt.x + ( pos_rt.width - text.w() ) / 2;
    dst_pt.y = pos_rt.y;
    text.Blit( dst_pt.x, dst_pt.y );

    TradeWindowGUI gui( pos_rt );

    Kingdom & kingdom = world.GetKingdom( Settings::Get().CurrentColor() );

    const std::string & header_from = _( "Your Resources" );

    Funds fundsFrom = kingdom.GetFunds();
    int resourceFrom = 0;
    const fheroes2::Point pt1( pos_rt.x, pos_rt.y + 190 );
    std::vector<fheroes2::Rect> rectsFrom;
    rectsFrom.reserve( 7 );
    rectsFrom.push_back( fheroes2::Rect( pt1.x, pt1.y, 34, 34 ) ); // wood
    rectsFrom.push_back( fheroes2::Rect( pt1.x + 37, pt1.y, 34, 34 ) ); // mercury
    rectsFrom.push_back( fheroes2::Rect( pt1.x + 74, pt1.y, 34, 34 ) ); // ore
    rectsFrom.push_back( fheroes2::Rect( pt1.x, pt1.y + 37, 34, 34 ) ); // sulfur
    rectsFrom.push_back( fheroes2::Rect( pt1.x + 37, pt1.y + 37, 34, 34 ) ); // crystal
    rectsFrom.push_back( fheroes2::Rect( pt1.x + 74, pt1.y + 37, 34, 34 ) ); // gems
    rectsFrom.push_back( fheroes2::Rect( pt1.x + 37, pt1.y + 74, 34, 34 ) ); // gold

    fheroes2::MovableSprite cursorFrom( fheroes2::AGG::GetICN( tradpost, 14 ) );
    text.Set( header_from, Font::SMALL );
    dst_pt.x = pt1.x + ( 108 - text.w() ) / 2;
    dst_pt.y = pt1.y - 15;
    text.Blit( dst_pt.x, dst_pt.y );
    RedrawFromResource( pt1, fundsFrom );

    const std::string & header_to = _( "Available Trades" );

    Funds fundsTo;
    int resourceTo = 0;
    const fheroes2::Point pt2( 138 + pos_rt.x, pos_rt.y + 190 );
    std::vector<fheroes2::Rect> rectsTo;
    rectsTo.reserve( 7 );
    rectsTo.push_back( fheroes2::Rect( pt2.x, pt2.y, 34, 34 ) ); // wood
    rectsTo.push_back( fheroes2::Rect( pt2.x + 37, pt2.y, 34, 34 ) ); // mercury
    rectsTo.push_back( fheroes2::Rect( pt2.x + 74, pt2.y, 34, 34 ) ); // ore
    rectsTo.push_back( fheroes2::Rect( pt2.x, pt2.y + 37, 34, 34 ) ); // sulfur
    rectsTo.push_back( fheroes2::Rect( pt2.x + 37, pt2.y + 37, 34, 34 ) ); // crystal
    rectsTo.push_back( fheroes2::Rect( pt2.x + 74, pt2.y + 37, 34, 34 ) ); // gems
    rectsTo.push_back( fheroes2::Rect( pt2.x + 37, pt2.y + 74, 34, 34 ) ); // gold

    fheroes2::MovableSprite cursorTo( fheroes2::AGG::GetICN( tradpost, 14 ) );
    text.Set( header_to, Font::SMALL );
    dst_pt.x = pt2.x + ( 108 - text.w() ) / 2;
    dst_pt.y = pt2.y - 15;
    text.Blit( dst_pt.x, dst_pt.y );
    RedrawToResource( pt2, false, fromTradingPost );

    u32 count_sell = 0;
    u32 count_buy = 0;

    u32 max_sell = 0;
    u32 max_buy = 0;

    fheroes2::Rect & buttonMax = gui.buttonMax;
    fheroes2::Rect & buttonMin = gui.buttonMin;
    fheroes2::Button & buttonGift = gui.buttonGift;
    fheroes2::Button & buttonTrade = gui.buttonTrade;
    fheroes2::Button & buttonLeft = gui.buttonLeft;
    fheroes2::Button & buttonRight = gui.buttonRight;
    fheroes2::Scrollbar & scrollbar = gui._scrollbar;

    // button exit
    const fheroes2::Sprite & sprite_exit = fheroes2::AGG::GetICN( tradpost, 17 );
    dst_pt.x = pos_rt.x + ( pos_rt.width - sprite_exit.width() ) / 2;
    dst_pt.y = pos_rt.y + pos_rt.height - sprite_exit.height();
    fheroes2::Button buttonExit( dst_pt.x, dst_pt.y, tradpost, 17, 18 );

    buttonGift.draw();
    buttonExit.draw();
    cursor.Show();
    display.render();

    LocalEvent & le = LocalEvent::Get();

    // message loop
    while ( le.HandleEvents() ) {
        if ( buttonGift.isEnabled() )
            le.MousePressLeft( buttonGift.area() ) ? buttonGift.drawOnPress() : buttonGift.drawOnRelease();
        if ( buttonTrade.isEnabled() )
            le.MousePressLeft( buttonTrade.area() ) ? buttonTrade.drawOnPress() : buttonTrade.drawOnRelease();
        if ( buttonLeft.isEnabled() )
            le.MousePressLeft( buttonLeft.area() ) ? buttonLeft.drawOnPress() : buttonLeft.drawOnRelease();
        if ( buttonRight.isEnabled() )
            le.MousePressLeft( buttonRight.area() ) ? buttonRight.drawOnPress() : buttonRight.drawOnRelease();

        le.MousePressLeft( buttonExit.area() ) ? buttonExit.drawOnPress() : buttonExit.drawOnRelease();

        if ( le.MouseClickLeft( buttonExit.area() ) || HotKeyCloseWindow )
            break;

        if ( buttonGift.isEnabled() && le.MouseClickLeft( buttonGift.area() ) ) {
            cursor.Hide();
            Dialog::MakeGiftResource();
            fundsFrom = kingdom.GetFunds();
            RedrawFromResource( pt1, fundsFrom );
            cursor.Show();
            display.render();
        }

        // click from
        for ( u32 ii = 0; ii < rectsFrom.size(); ++ii ) {
            const fheroes2::Rect & rect_from = rectsFrom[ii];

            if ( le.MouseClickLeft( rect_from ) ) {
                resourceFrom = Resource::FromIndexSprite2( ii );
                max_sell = fundsFrom.Get( resourceFrom );

                if ( GetTradeCosts( resourceFrom, resourceTo, fromTradingPost ) ) {
                    max_buy = Resource::GOLD == resourceTo ? max_sell * GetTradeCosts( resourceFrom, resourceTo, fromTradingPost )
                                                           : max_sell / GetTradeCosts( resourceFrom, resourceTo, fromTradingPost );
                }

                count_sell = 0;
                count_buy = 0;

                cursor.Hide();
                cursorFrom.setPosition( rect_from.x - 2, rect_from.y - 2 );

                if ( resourceTo )
                    cursorTo.hide();
                RedrawToResource( pt2, true, fromTradingPost, resourceFrom );
                if ( resourceTo )
                    cursorTo.show();
                if ( resourceTo )
                    gui.ShowTradeArea( resourceFrom, resourceTo, max_buy, max_sell, count_buy, count_sell, fromTradingPost );

                cursor.Show();
                display.render();
            }
            else if ( le.MousePressRight( rect_from ) )
                Dialog::ResourceInfo( _( "Income" ), "", kingdom.GetIncome( INCOME_ALL ), 0 );
        }

        // click to
        for ( u32 ii = 0; ii < rectsTo.size(); ++ii ) {
            const Rect & rect_to = rectsTo[ii];

            if ( le.MouseClickLeft( rect_to ) ) {
                resourceTo = Resource::FromIndexSprite2( ii );

                if ( GetTradeCosts( resourceFrom, resourceTo, fromTradingPost ) ) {
                    max_buy = Resource::GOLD == resourceTo ? max_sell * GetTradeCosts( resourceFrom, resourceTo, fromTradingPost )
                                                           : max_sell / GetTradeCosts( resourceFrom, resourceTo, fromTradingPost );
                }

                count_sell = 0;
                count_buy = 0;

                cursor.Hide();
                cursorTo.setPosition( rect_to.x - 2, rect_to.y - 2 );

                if ( resourceFrom ) {
                    cursorTo.hide();
                    RedrawToResource( pt2, true, fromTradingPost, resourceFrom );
                    cursorTo.show();
                    gui.ShowTradeArea( resourceFrom, resourceTo, max_buy, max_sell, count_buy, count_sell, fromTradingPost );
                }
                cursor.Show();
                display.render();
            }
        }

        // Scrollbar
        if ( buttonLeft.isEnabled() && buttonRight.isEnabled() && max_buy && le.MousePressLeft( scrollbar.getArea() ) ) {
            const Point & mousePos = le.GetMouseCursor();
            scrollbar.moveToPos( fheroes2::Point( mousePos.x, mousePos.y ) );
            const int32_t seek = scrollbar.currentIndex();

            count_buy = seek * ( Resource::GOLD == resourceTo ? GetTradeCosts( resourceFrom, resourceTo, fromTradingPost ) : 1 );
            count_sell = seek * ( Resource::GOLD == resourceTo ? 1 : GetTradeCosts( resourceFrom, resourceTo, fromTradingPost ) );

            cursor.Hide();
            gui.RedrawInfoBuySell( count_sell, count_buy, max_sell, fundsFrom.Get( resourceTo ) );
            cursor.Show();
            display.render();
        }
        else
            // click max
            if ( buttonMax.width && max_buy && le.MouseClickLeft( buttonMax ) ) {
            const int32_t max = scrollbar.maxIndex();

            count_buy = max * ( Resource::GOLD == resourceTo ? GetTradeCosts( resourceFrom, resourceTo, fromTradingPost ) : 1 );
            count_sell = max * ( Resource::GOLD == resourceTo ? 1 : GetTradeCosts( resourceFrom, resourceTo, fromTradingPost ) );

            cursor.Hide();
            scrollbar.moveToIndex( max );
            gui.RedrawInfoBuySell( count_sell, count_buy, max_sell, fundsFrom.Get( resourceTo ) );
            cursor.Show();
            display.render();
        }
        // click min
        if ( buttonMin.width && max_buy && le.MouseClickLeft( buttonMin ) ) {
            const int32_t min = 1;

            count_buy = min * ( Resource::GOLD == resourceTo ? GetTradeCosts( resourceFrom, resourceTo, fromTradingPost ) : 1 );
            count_sell = min * ( Resource::GOLD == resourceTo ? 1 : GetTradeCosts( resourceFrom, resourceTo, fromTradingPost ) );

            cursor.Hide();
            scrollbar.moveToIndex( min );
            gui.RedrawInfoBuySell( count_sell, count_buy, max_sell, fundsFrom.Get( resourceTo ) );
            cursor.Show();
            display.render();
        }

        // trade
        if ( buttonTrade.isEnabled() && le.MouseClickLeft( buttonTrade.area() ) && count_sell && count_buy ) {
            kingdom.OddFundsResource( Funds( resourceFrom, count_sell ) );
            kingdom.AddFundsResource( Funds( resourceTo, count_buy ) );

            resourceTo = resourceFrom = Resource::UNKNOWN;
            gui.ShowTradeArea( resourceFrom, resourceTo, 0, 0, 0, 0, fromTradingPost );

            fundsFrom = kingdom.GetFunds();
            cursorTo.hide();
            cursorFrom.hide();
            RedrawFromResource( pt1, fundsFrom );
            RedrawToResource( pt2, false, fromTradingPost, resourceFrom );
            display.render();
        }

        // decrease trade resource
        if ( count_buy && ( ( buttonLeft.isEnabled() && le.MouseClickLeft( gui.buttonLeft.area() ) ) || le.MouseWheelDn( scrollbar.getArea() ) ) ) {
            count_buy -= Resource::GOLD == resourceTo ? GetTradeCosts( resourceFrom, resourceTo, fromTradingPost ) : 1;

            count_sell -= Resource::GOLD == resourceTo ? 1 : GetTradeCosts( resourceFrom, resourceTo, fromTradingPost );

            cursor.Hide();
            scrollbar.backward();
            gui.RedrawInfoBuySell( count_sell, count_buy, max_sell, fundsFrom.Get( resourceTo ) );
            cursor.Show();
            display.render();
        }

        // increase trade resource
        if ( count_buy < max_buy && ( ( buttonRight.isEnabled() && le.MouseClickLeft( buttonRight.area() ) ) || le.MouseWheelUp( scrollbar.getArea() ) ) ) {
            count_buy += Resource::GOLD == resourceTo ? GetTradeCosts( resourceFrom, resourceTo, fromTradingPost ) : 1;

            count_sell += Resource::GOLD == resourceTo ? 1 : GetTradeCosts( resourceFrom, resourceTo, fromTradingPost );

            cursor.Hide();
            scrollbar.forward();
            gui.RedrawInfoBuySell( count_sell, count_buy, max_sell, fundsFrom.Get( resourceTo ) );
            cursor.Show();
            display.render();
        }
    }
}

void RedrawResourceSprite( const fheroes2::Image & sf, s32 px, s32 py, s32 value )
{
    Text text;
    fheroes2::Point dst_pt( px, py );

    fheroes2::Blit( sf, fheroes2::Display::instance(), dst_pt.x, dst_pt.y );
    text.Set( GetString( value ), Font::SMALL );
    dst_pt.x += ( 34 - text.w() ) / 2;
    dst_pt.y += 21;
    text.Blit( dst_pt.x, dst_pt.y );
}

void RedrawFromResource( const fheroes2::Point & pt, const Funds & rs )
{
    const int tradpost = Settings::Get().ExtGameEvilInterface() ? ICN::TRADPOSE : ICN::TRADPOST;

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

void RedrawResourceSprite2( const fheroes2::Image & sf, s32 px, s32 py, bool show, int from, int res, bool trading )
{
    fheroes2::Point dst_pt( px, py );

    fheroes2::Blit( sf, fheroes2::Display::instance(), dst_pt.x, dst_pt.y );

    if ( show ) {
        Text text( GetStringTradeCosts( from, res, trading ), Font::SMALL );
        dst_pt.x += ( 34 - text.w() ) / 2;
        dst_pt.y += 21;
        text.Blit( dst_pt.x, dst_pt.y );
    }
}

void RedrawToResource( const fheroes2::Point & pt, bool showcost, bool tradingPost, int from_resource )
{
    const int tradpost = Settings::Get().ExtGameEvilInterface() ? ICN::TRADPOSE : ICN::TRADPOST;

    // wood
    RedrawResourceSprite2( fheroes2::AGG::GetICN( tradpost, 7 ), pt.x, pt.y, showcost, from_resource, Resource::WOOD, tradingPost );
    // mercury
    RedrawResourceSprite2( fheroes2::AGG::GetICN( tradpost, 8 ), pt.x + 37, pt.y, showcost, from_resource, Resource::MERCURY, tradingPost );
    // ore
    RedrawResourceSprite2( fheroes2::AGG::GetICN( tradpost, 9 ), pt.x + 74, pt.y, showcost, from_resource, Resource::ORE, tradingPost );
    // sulfur
    RedrawResourceSprite2( fheroes2::AGG::GetICN( tradpost, 10 ), pt.x, pt.y + 37, showcost, from_resource, Resource::SULFUR, tradingPost );
    // crystal
    RedrawResourceSprite2( fheroes2::AGG::GetICN( tradpost, 11 ), pt.x + 37, pt.y + 37, showcost, from_resource, Resource::CRYSTAL, tradingPost );
    // gems
    RedrawResourceSprite2( fheroes2::AGG::GetICN( tradpost, 12 ), pt.x + 74, pt.y + 37, showcost, from_resource, Resource::GEMS, tradingPost );
    // gold
    RedrawResourceSprite2( fheroes2::AGG::GetICN( tradpost, 13 ), pt.x + 37, pt.y + 74, showcost, from_resource, Resource::GOLD, tradingPost );
}

std::string GetStringTradeCosts( int rs_from, int rs_to, bool tradingPost )
{
    std::string res;

    if ( rs_from == rs_to ) {
        res = _( "n/a" );
    }
    else {
        res = "1/";
        res.append( GetString( GetTradeCosts( rs_from, rs_to, tradingPost ) ) );
    }

    return res;
}

u32 GetTradeCosts( int rs_from, int rs_to, bool tradingPost )
{
    const u32 markets = tradingPost ? 3 : world.GetKingdom( Settings::Get().CurrentColor() ).GetCountMarketplace();

    if ( rs_from == rs_to )
        return 0;

    switch ( rs_from ) {
    // uncostly
    case Resource::WOOD:
    case Resource::ORE:

        switch ( rs_to ) {
        // sale uncostly
        case Resource::GOLD:
            if ( 1 == markets )
                return SALE_UNCOSTLY1;
            else if ( 2 == markets )
                return SALE_UNCOSTLY2;
            else if ( 3 == markets )
                return SALE_UNCOSTLY3;
            else if ( 4 == markets )
                return SALE_UNCOSTLY4;
            else if ( 5 == markets )
                return SALE_UNCOSTLY5;
            else if ( 6 == markets )
                return SALE_UNCOSTLY6;
            else if ( 7 == markets )
                return SALE_UNCOSTLY7;
            else if ( 8 == markets )
                return SALE_UNCOSTLY8;
            else if ( 8 < markets )
                return SALE_UNCOSTLY9;
            break;

        // change uncostly to costly
        case Resource::MERCURY:
        case Resource::SULFUR:
        case Resource::CRYSTAL:
        case Resource::GEMS:
            if ( 1 == markets )
                return UNCOSTLY_COSTLY1;
            else if ( 2 == markets )
                return UNCOSTLY_COSTLY2;
            else if ( 3 == markets )
                return UNCOSTLY_COSTLY3;
            else if ( 4 == markets )
                return UNCOSTLY_COSTLY4;
            else if ( 5 == markets )
                return UNCOSTLY_COSTLY5;
            else if ( 6 == markets )
                return UNCOSTLY_COSTLY6;
            else if ( 7 == markets )
                return UNCOSTLY_COSTLY7;
            else if ( 8 == markets )
                return UNCOSTLY_COSTLY8;
            else if ( 8 < markets )
                return UNCOSTLY_COSTLY9;
            break;

        // change uncostly to uncostly
        case Resource::WOOD:
        case Resource::ORE:
            if ( 1 == markets )
                return COSTLY_COSTLY1;
            else if ( 2 == markets )
                return COSTLY_COSTLY2;
            else if ( 3 == markets )
                return COSTLY_COSTLY3;
            else if ( 4 == markets )
                return COSTLY_COSTLY4;
            else if ( 5 == markets )
                return COSTLY_COSTLY5;
            else if ( 6 == markets )
                return COSTLY_COSTLY6;
            else if ( 7 == markets )
                return COSTLY_COSTLY7;
            else if ( 8 == markets )
                return COSTLY_COSTLY8;
            else if ( 8 < markets )
                return COSTLY_COSTLY9;
            break;
        }
        break;

    // costly
    case Resource::MERCURY:
    case Resource::SULFUR:
    case Resource::CRYSTAL:
    case Resource::GEMS:

        switch ( rs_to ) {
        // sale costly
        case Resource::GOLD:
            if ( 1 == markets )
                return SALE_COSTLY1;
            else if ( 2 == markets )
                return SALE_COSTLY2;
            else if ( 3 == markets )
                return SALE_COSTLY3;
            else if ( 4 == markets )
                return SALE_COSTLY4;
            else if ( 5 == markets )
                return SALE_COSTLY5;
            else if ( 6 == markets )
                return SALE_COSTLY6;
            else if ( 7 == markets )
                return SALE_COSTLY7;
            else if ( 8 == markets )
                return SALE_COSTLY8;
            else if ( 8 < markets )
                return SALE_COSTLY9;
            break;

        // change costly to costly
        case Resource::MERCURY:
        case Resource::SULFUR:
        case Resource::CRYSTAL:
        case Resource::GEMS:
            if ( 1 == markets )
                return COSTLY_COSTLY1;
            else if ( 2 == markets )
                return COSTLY_COSTLY2;
            else if ( 3 == markets )
                return COSTLY_COSTLY3;
            else if ( 4 == markets )
                return COSTLY_COSTLY4;
            else if ( 5 == markets )
                return COSTLY_COSTLY5;
            else if ( 6 == markets )
                return COSTLY_COSTLY6;
            else if ( 7 == markets )
                return COSTLY_COSTLY7;
            else if ( 8 == markets )
                return COSTLY_COSTLY8;
            else if ( 8 < markets )
                return COSTLY_COSTLY9;
            break;

        // change costly to uncostly
        case Resource::WOOD:
        case Resource::ORE:
            if ( 1 == markets )
                return COSTLY_UNCOSTLY1;
            else if ( 2 == markets )
                return COSTLY_UNCOSTLY2;
            else if ( 3 == markets )
                return COSTLY_UNCOSTLY3;
            else if ( 4 == markets )
                return COSTLY_UNCOSTLY4;
            else if ( 5 == markets )
                return COSTLY_UNCOSTLY5;
            else if ( 6 == markets )
                return COSTLY_UNCOSTLY6;
            else if ( 7 == markets )
                return COSTLY_UNCOSTLY7;
            else if ( 8 == markets )
                return COSTLY_UNCOSTLY8;
            else if ( 8 < markets )
                return COSTLY_UNCOSTLY9;
            break;
        }
        break;

    // gold
    case Resource::GOLD:

        switch ( rs_to ) {
        default:
            break;

        // buy costly
        case Resource::MERCURY:
        case Resource::SULFUR:
        case Resource::CRYSTAL:
        case Resource::GEMS:
            if ( 1 == markets )
                return BUY_COSTLY1;
            else if ( 2 == markets )
                return BUY_COSTLY2;
            else if ( 3 == markets )
                return BUY_COSTLY3;
            else if ( 4 == markets )
                return BUY_COSTLY4;
            else if ( 5 == markets )
                return BUY_COSTLY5;
            else if ( 6 == markets )
                return BUY_COSTLY6;
            else if ( 7 == markets )
                return BUY_COSTLY7;
            else if ( 8 == markets )
                return BUY_COSTLY8;
            else if ( 8 < markets )
                return BUY_COSTLY9;
            break;

        // buy uncostly
        case Resource::WOOD:
        case Resource::ORE:
            if ( 1 == markets )
                return BUY_UNCOSTLY1;
            else if ( 2 == markets )
                return BUY_UNCOSTLY2;
            else if ( 3 == markets )
                return BUY_UNCOSTLY3;
            else if ( 4 == markets )
                return BUY_UNCOSTLY4;
            else if ( 5 == markets )
                return BUY_UNCOSTLY5;
            else if ( 6 == markets )
                return BUY_UNCOSTLY6;
            else if ( 7 == markets )
                return BUY_UNCOSTLY7;
            else if ( 8 == markets )
                return BUY_UNCOSTLY8;
            else if ( 8 < markets )
                return BUY_UNCOSTLY9;
            break;
        }
        break;

    // not select
    default:
        break;
    }

    return 0;
}
