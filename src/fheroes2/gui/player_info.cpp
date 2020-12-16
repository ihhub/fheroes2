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

#include "agg.h"
#include "dialog.h"
#include "game.h"
#include "player_info.h"
#include "race.h"
#include "settings.h"
#include "text.h"

bool Interface::PlayerInfo::operator==( const Player * p ) const
{
    return player == p;
}

Interface::PlayersInfo::PlayersInfo( bool name, bool race, bool swap )
    : show_name( name )
    , show_race( race )
    , show_swap( swap )
    , currentSelectedPlayer( nullptr )
{
    reserve( KINGDOMMAX );
}

void Interface::PlayersInfo::UpdateInfo( Players & players, const Point & pt1, const Point & pt2 )
{
    const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::NGEXTRA, 3 );

    clear();

    for ( Players::iterator it = players.begin(); it != players.end(); ++it ) {
        const u32 current = std::distance( players.begin(), it );
        PlayerInfo info;

        info.player = *it;
        info.rect1 = Rect( pt1.x + Game::GetStep4Player( current, sprite.width(), players.size() ), pt1.y, sprite.width(), sprite.height() );
        info.rect2 = Rect( pt2.x + Game::GetStep4Player( current, sprite.width(), players.size() ), pt2.y, sprite.width(), sprite.height() );

        push_back( info );
    }

    for ( iterator it = begin(); it != end(); ++it ) {
        if ( ( it + 1 ) != end() ) {
            const Rect & rect1 = ( *it ).rect2;
            const Rect & rect2 = ( *( it + 1 ) ).rect2;
            const fheroes2::Sprite & iconSprite = fheroes2::AGG::GetICN( ICN::ADVMCO, 8 );

            ( *it ).rect3 = Rect( rect1.x + rect1.w + ( rect2.x - ( rect1.x + rect1.w ) ) / 2 - 5, rect1.y + rect1.h + 20, iconSprite.width(), iconSprite.height() );
        }
    }
}

Player * Interface::PlayersInfo::GetFromOpponentClick( const Point & pt )
{
    for ( iterator it = begin(); it != end(); ++it )
        if ( ( *it ).rect1 & pt )
            return ( *it ).player;

    return NULL;
}

Player * Interface::PlayersInfo::GetFromOpponentNameClick( const Point & pt )
{
    for ( iterator it = begin(); it != end(); ++it )
        if ( Rect( ( *it ).rect1.x, ( *it ).rect1.y + ( *it ).rect1.h, ( *it ).rect1.w, 10 ) & pt )
            return ( *it ).player;

    return NULL;
}

Player * Interface::PlayersInfo::GetFromOpponentChangeClick( const Point & pt )
{
    for ( iterator it = begin(); it != end(); ++it )
        if ( ( *it ).rect3 & pt )
            return ( *it ).player;

    return NULL;
}

Player * Interface::PlayersInfo::GetFromClassClick( const Point & pt )
{
    for ( iterator it = begin(); it != end(); ++it )
        if ( ( *it ).rect2 & pt )
            return ( *it ).player;

    return NULL;
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
        const Rect & rect1 = ( *it ).rect1;
        const Rect & rect2 = ( *it ).rect2;
        const Rect & rect3 = ( *it ).rect3;

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
            name.Blit( rect1.x + 2 + ( maximumTextWidth - fitWidth ) / 2, rect1.y + rect1.h - show_name, maximumTextWidth );
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
            text.Blit( rect2.x + ( rect2.w - text.w() ) / 2, rect2.y + rect2.h + 2 );
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
    Player * player = NULL;

    if ( le.MousePressRight() ) {
        // opponent
        if ( NULL != ( player = GetFromOpponentClick( le.GetMouseCursor() ) ) )
            Dialog::Message(
                _( "Opponents" ),
                _( "This lets you change player starting positions and colors. A particular color will always start in a particular location. Some positions may only be played by a computer player or only by a human player." ),
                Font::BIG );
        else
            // class
            if ( NULL != ( player = GetFromClassClick( le.GetMouseCursor() ) ) )
            Dialog::Message(
                _( "Class" ),
                _( "This lets you change the class of a player. Classes are not always changeable. Depending on the scenario, a player may receive additional towns and/or heroes not of their primary alignment." ),
                Font::BIG );
    }
    else
    // if(le.MouseClickLeft())
    {
        // select opponent
        if ( NULL != ( player = GetFromOpponentClick( le.GetMouseCursor() ) ) ) {
            const Maps::FileInfo & fi = conf.CurrentFileInfo();
            Players & players = conf.GetPlayers();
            if ( ( player->GetColor() & fi.AllowHumanColors() )
                 && ( !Settings::Get().IsGameType( Game::TYPE_MULTI ) || !( player->GetColor() & fi.HumanOnlyColors() ) ) ) {
                u32 humans = players.GetColors( CONTROL_HUMAN, true );

                if ( conf.IsGameType( Game::TYPE_MULTI ) ) {
                    if ( currentSelectedPlayer == nullptr ) {
                        currentSelectedPlayer = player;
                    }
                    else if ( currentSelectedPlayer == player ) {
                        currentSelectedPlayer = nullptr;
                    }
                    else if ( player->isControlAI() != currentSelectedPlayer->isControlAI() ) {
                        if ( !( humans & player->GetColor() ) ) {
                            player->SetControl( CONTROL_HUMAN );
                            players.SetPlayerControl( currentSelectedPlayer->GetColor(), CONTROL_AI | CONTROL_HUMAN );
                        }
                        else {
                            currentSelectedPlayer->SetControl( CONTROL_HUMAN );
                            players.SetPlayerControl( player->GetColor(), CONTROL_AI | CONTROL_HUMAN );
                        }

                        currentSelectedPlayer = nullptr;
                    }
                }
                else
                // single play
                {
                    players.SetPlayerControl( humans, CONTROL_AI | CONTROL_HUMAN );
                    player->SetControl( CONTROL_HUMAN );
                }
            }
        }
        else
            // modify name
            if ( show_name && NULL != ( player = GetFromOpponentNameClick( le.GetMouseCursor() ) ) ) {
            std::string res;
            std::string str = _( "%{color} player" );
            StringReplace( str, "%{color}", Color::String( player->GetColor() ) );

            if ( Dialog::InputString( str, res ) && !res.empty() )
                player->SetName( res );
        }
        else
            // select class
            if ( NULL != ( player = GetFromClassClick( le.GetMouseCursor() ) ) ) {
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
        else
            // change players
            if ( show_swap && NULL != ( player = GetFromOpponentChangeClick( le.GetMouseCursor() ) ) ) {
            iterator it = std::find_if( begin(), end(), [player]( const PlayerInfo & pi ) { return pi.player == player; } );
            if ( it != end() && ( it + 1 ) != end() ) {
                Players & players = conf.GetPlayers();
                Players::iterator it1 = std::find_if( players.begin(), players.end(), [it]( Player * p ) { return p == ( *it ).player; } );
                Players::iterator it2 = std::find_if( players.begin(), players.end(), [it]( Player * p ) { return p == ( *( it + 1 ) ).player; } );

                if ( it1 != players.end() && it2 != players.end() ) {
                    std::swap( ( *it ).player, ( *( it + 1 ) ).player );
                    std::swap( *it1, *it2 );
                }
            }
            else
                player = NULL;
        }
    }

    return player != NULL;
}
