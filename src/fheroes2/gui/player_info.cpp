/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2020                                                    *
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
#include "dialog.h"
#include "game.h"
#include "icn.h"
#include "localevent.h"
#include "player_info.h"
#include "race.h"
#include "settings.h"
#include "text.h"
#include "tools.h"

Interface::PlayersInfo::PlayersInfo( bool name, bool race, bool swap )
    : show_name( name )
    , show_race( race )
    , show_swap( swap )
    , currentSelectedPlayer( nullptr )
{
    reserve( KINGDOMMAX );
}

void Interface::PlayersInfo::UpdateInfo( Players & players, const fheroes2::Point & pt1, const fheroes2::Point & pt2 )
{
    const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::NGEXTRA, 3 );

    clear();

    const int32_t playerCount = static_cast<int32_t>( players.size() ); // safe to cast as the number of players <= 8.

    for ( int32_t i = 0; i < playerCount; ++i ) {
        PlayerInfo info;

        info.player = players[i];
        info.rect1 = fheroes2::Rect( pt1.x + Game::GetStep4Player( i, sprite.width(), playerCount ), pt1.y, sprite.width(), sprite.height() );
        info.rect2 = fheroes2::Rect( pt2.x + Game::GetStep4Player( i, sprite.width(), playerCount ), pt2.y, sprite.width(), sprite.height() );

        emplace_back( std::move( info ) );
    }

    for ( iterator it = begin(); it != end(); ++it ) {
        if ( ( it + 1 ) != end() ) {
            const fheroes2::Rect & rect1 = ( *it ).rect2;
            const fheroes2::Rect & rect2 = ( *( it + 1 ) ).rect2;
            const fheroes2::Sprite & iconSprite = fheroes2::AGG::GetICN( ICN::ADVMCO, 8 );

            ( *it ).rect3 = fheroes2::Rect( rect1.x + rect1.width + ( rect2.x - ( rect1.x + rect1.width ) ) / 2 - 5, rect1.y + rect1.height + 20, iconSprite.width(),
                                            iconSprite.height() );
        }
    }
}

bool Interface::PlayersInfo::SwapPlayers( Player & player1, Player & player2 ) const
{
    const Settings & conf = Settings::Get();
    const Maps::FileInfo & fi = conf.CurrentFileInfo();

    const int player1Color = player1.GetColor();
    const int player2Color = player2.GetColor();

    bool swap = false;

    if ( player1.isControlAI() == player2.isControlAI() ) {
        swap = true;
    }
    else if ( ( player1Color & fi.AllowCompHumanColors() ) && ( player2Color & fi.AllowCompHumanColors() ) ) {
        const int humans = conf.GetPlayers().GetColors( CONTROL_HUMAN, true );

        if ( humans & player1Color ) {
            Players::SetPlayerControl( player1Color, CONTROL_AI | CONTROL_HUMAN );
            Players::SetPlayerControl( player2Color, CONTROL_HUMAN );
        }
        else {
            Players::SetPlayerControl( player2Color, CONTROL_AI | CONTROL_HUMAN );
            Players::SetPlayerControl( player1Color, CONTROL_HUMAN );
        }

        swap = true;
    }

    if ( swap ) {
        const int player1Race = player1.GetRace();
        const int player2Race = player2.GetRace();

        if ( player1Race != player2Race && conf.AllowChangeRace( player1Color ) && conf.AllowChangeRace( player2Color ) ) {
            player1.SetRace( player2Race );
            player2.SetRace( player1Race );
        }

        const std::string player1Name = player1.GetName();
        const std::string player2Name = player2.GetName();

        const std::string player1DefaultName = player1.GetDefaultName();
        const std::string player2DefaultName = player2.GetDefaultName();

        if ( player2Name == player2DefaultName ) {
            player1.SetName( player1DefaultName );
        }
        else {
            player1.SetName( player2Name );
        }
        if ( player1Name == player1DefaultName ) {
            player2.SetName( player2DefaultName );
        }
        else {
            player2.SetName( player1Name );
        }
    }

    return swap;
}

Player * Interface::PlayersInfo::GetFromOpponentClick( const fheroes2::Point & pt )
{
    for ( iterator it = begin(); it != end(); ++it )
        if ( ( *it ).rect1 & pt )
            return ( *it ).player;

    return nullptr;
}

Player * Interface::PlayersInfo::GetFromOpponentNameClick( const fheroes2::Point & pt )
{
    for ( iterator it = begin(); it != end(); ++it )
        if ( fheroes2::Rect( ( *it ).rect1.x, ( *it ).rect1.y + ( *it ).rect1.height, ( *it ).rect1.width, 10 ) & pt )
            return ( *it ).player;

    return nullptr;
}

Player * Interface::PlayersInfo::GetFromOpponentChangeClick( const fheroes2::Point & pt )
{
    for ( iterator it = begin(); it != end(); ++it )
        if ( ( *it ).rect3 & pt )
            return ( *it ).player;

    return nullptr;
}

Player * Interface::PlayersInfo::GetFromClassClick( const fheroes2::Point & pt )
{
    for ( iterator it = begin(); it != end(); ++it )
        if ( ( *it ).rect2 & pt )
            return ( *it ).player;

    return nullptr;
}

void Interface::PlayersInfo::RedrawInfo( bool show_play_info ) const /* show_play_info: show game info with color status (play/not play) */
{
    const Settings & conf = Settings::Get();
    fheroes2::Display & display = fheroes2::Display::instance();
    const Maps::FileInfo & fi = conf.CurrentFileInfo();

    const u32 humans_colors = conf.GetPlayers().GetColors( CONTROL_HUMAN, true );
    u32 index = 0;

    for ( const_iterator it = begin(); it != end(); ++it ) {
        const Player & player = *( ( *it ).player );
        const fheroes2::Rect & rect1 = ( *it ).rect1;
        const fheroes2::Rect & rect2 = ( *it ).rect2;
        const fheroes2::Rect & rect3 = ( *it ).rect3;

        // 1. redraw opponents

        // current human
        if ( humans_colors & player.GetColor() )
            index = 9 + Color::GetIndex( player.GetColor() );
        else
            // comp only
            if ( fi.ComputerOnlyColors() & player.GetColor() ) {
            if ( show_play_info ) {
                index = ( player.isPlay() ? 3 : 15 ) + Color::GetIndex( player.GetColor() );
            }
            else
                index = 15 + Color::GetIndex( player.GetColor() );
        }
        else
        // comp/human
        {
            if ( show_play_info ) {
                index = ( player.isPlay() ? 3 : 15 ) + Color::GetIndex( player.GetColor() );
            }
            else
                index = 3 + Color::GetIndex( player.GetColor() );
        }

        // wide sprite offset
        if ( show_name )
            index += 24;

        const fheroes2::Sprite & playerIcon = fheroes2::AGG::GetICN( ICN::NGEXTRA, index );
        fheroes2::Blit( playerIcon, display, rect1.x, rect1.y );
        if ( currentSelectedPlayer != nullptr && it->player == currentSelectedPlayer ) {
            fheroes2::Image selection( playerIcon.width(), playerIcon.height() );
            selection.reset();
            fheroes2::DrawBorder( selection, 214 );
            fheroes2::Blit( selection, display, rect1.x, rect1.y );
        }

        if ( show_name ) {
            // draw player name
            Text name( player.GetName(), Font::SMALL );

            const int32_t maximumTextWidth = playerIcon.width() - 4;
            const int32_t fitWidth = Text::getFitWidth( player.GetName(), Font::SMALL, maximumTextWidth );
            name.Blit( rect1.x + 2 + ( maximumTextWidth - fitWidth ) / 2, rect1.y + rect1.height - 1, maximumTextWidth );
        }

        // 2. redraw class
        bool class_color = conf.AllowChangeRace( player.GetColor() );

        if ( show_play_info ) {
            class_color = player.isPlay();
        }

        switch ( player.GetRace() ) {
        case Race::KNGT:
            index = class_color ? 51 : 70;
            break;
        case Race::BARB:
            index = class_color ? 52 : 71;
            break;
        case Race::SORC:
            index = class_color ? 53 : 72;
            break;
        case Race::WRLK:
            index = class_color ? 54 : 73;
            break;
        case Race::WZRD:
            index = class_color ? 55 : 74;
            break;
        case Race::NECR:
            index = class_color ? 56 : 75;
            break;
        case Race::MULT:
            index = 76;
            break;
        case Race::RAND:
            index = 58;
            break;
        default:
            continue;
        }

        fheroes2::Blit( fheroes2::AGG::GetICN( ICN::NGEXTRA, index ), display, rect2.x, rect2.y );

        if ( show_race ) {
            const std::string & name = ( Race::NECR == player.GetRace() ? _( "Necroman" ) : Race::String( player.GetRace() ) );
            Text text( name, Font::SMALL );
            text.Blit( rect2.x + ( rect2.width - text.w() ) / 2, rect2.y + rect2.height + 2 );
        }

        // "swap" sprite
        if ( show_swap && ( it + 1 ) != end() ) {
            fheroes2::Blit( fheroes2::AGG::GetICN( ICN::ADVMCO, 8 ), display, rect3.x, rect3.y );
        }
    }
}

void Interface::PlayersInfo::resetSelection()
{
    currentSelectedPlayer = nullptr;
}

bool Interface::PlayersInfo::QueueEventProcessing( void )
{
    Settings & conf = Settings::Get();
    LocalEvent & le = LocalEvent::Get();
    Player * player = nullptr;

    if ( le.MousePressRight() ) {
        // opponent
        if ( nullptr != ( player = GetFromOpponentClick( le.GetMouseCursor() ) ) )
            Dialog::Message(
                _( "Opponents" ),
                _( "This lets you change player starting positions and colors. A particular color will always start in a particular location. Some positions may only be played by a computer player or only by a human player." ),
                Font::BIG );
        // class
        else if ( nullptr != ( player = GetFromClassClick( le.GetMouseCursor() ) ) )
            Dialog::Message(
                _( "Class" ),
                _( "This lets you change the class of a player. Classes are not always changeable. Depending on the scenario, a player may receive additional towns and/or heroes not of their primary alignment." ),
                Font::BIG );
    }
    // le.MouseClickLeft()
    else {
        // select opponent
        if ( nullptr != ( player = GetFromOpponentClick( le.GetMouseCursor() ) ) ) {
            const Maps::FileInfo & fi = conf.CurrentFileInfo();

            if ( conf.IsGameType( Game::TYPE_MULTI ) ) {
                if ( currentSelectedPlayer == nullptr ) {
                    currentSelectedPlayer = player;
                }
                else if ( currentSelectedPlayer == player ) {
                    currentSelectedPlayer = nullptr;
                }
                else if ( SwapPlayers( *player, *currentSelectedPlayer ) ) {
                    currentSelectedPlayer = nullptr;
                }
            }
            else {
                const int playerColor = player->GetColor();

                if ( playerColor & fi.AllowHumanColors() ) {
                    const int human = conf.GetPlayers().GetColors( CONTROL_HUMAN, true );

                    if ( playerColor != human ) {
                        Players::SetPlayerControl( human, CONTROL_AI | CONTROL_HUMAN );
                        Players::SetPlayerControl( playerColor, CONTROL_HUMAN );
                    }
                }
            }
        }
        // modify name
        else if ( show_name && nullptr != ( player = GetFromOpponentNameClick( le.GetMouseCursor() ) ) ) {
            std::string res;
            std::string str = _( "%{color} player" );
            StringReplace( str, "%{color}", Color::String( player->GetColor() ) );

            if ( Dialog::InputString( str, res ) && !res.empty() )
                player->SetName( res );
        }
        // select class
        else if ( nullptr != ( player = GetFromClassClick( le.GetMouseCursor() ) ) ) {
            if ( conf.AllowChangeRace( player->GetColor() ) ) {
                switch ( player->GetRace() ) {
                case Race::KNGT:
                    player->SetRace( Race::BARB );
                    break;
                case Race::BARB:
                    player->SetRace( Race::SORC );
                    break;
                case Race::SORC:
                    player->SetRace( Race::WRLK );
                    break;
                case Race::WRLK:
                    player->SetRace( Race::WZRD );
                    break;
                case Race::WZRD:
                    player->SetRace( Race::NECR );
                    break;
                case Race::NECR:
                    player->SetRace( Race::RAND );
                    break;
                case Race::RAND:
                    player->SetRace( Race::KNGT );
                    break;
                default:
                    break;
                }
            }
        }
        // swap players
        else if ( show_swap && nullptr != ( player = GetFromOpponentChangeClick( le.GetMouseCursor() ) ) ) {
            iterator it = std::find_if( begin(), end(), [player]( const PlayerInfo & pi ) { return pi.player == player; } );
            if ( it != end() && ( it + 1 ) != end() ) {
                Players & players = conf.GetPlayers();
                Players::iterator it1 = std::find_if( players.begin(), players.end(), [&it]( const Player * p ) { return p == ( *it ).player; } );
                Players::iterator it2 = std::find_if( players.begin(), players.end(), [&it]( const Player * p ) { return p == ( *( it + 1 ) ).player; } );

                if ( it1 != players.end() && it2 != players.end() ) {
                    SwapPlayers( **it1, **it2 );
                }
            }
            else
                player = nullptr;
        }
    }

    return player != nullptr;
}
