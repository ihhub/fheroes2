/***************************************************************************
 *   Copyright (C) 2011 by Andrey Afletdinov <fheroes2@gmail.com>          *
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
#include <assert.h>

#include "game.h"
#include "maps_fileinfo.h"
#include "players.h"
#include "race.h"
#include "world.h"

namespace
{
    const int playersSize = KINGDOMMAX + 1;
    Player * _players[playersSize] = {NULL};
    int human_colors = 0;

    enum
    {
        ST_INGAME = 0x2000
    };
}

void PlayerFocusReset( Player * player )
{
    if ( player )
        player->GetFocus().Reset();
}

void PlayerFixMultiControl( Player * player )
{
    if ( player && player->GetControl() == ( CONTROL_HUMAN | CONTROL_AI ) )
        player->SetControl( CONTROL_AI );
}

void PlayerFixRandomRace( Player * player )
{
    if ( player && player->GetRace() == Race::RAND )
        player->SetRace( Race::Rand() );
}

bool Control::isControlAI( void ) const
{
    return ( CONTROL_AI & GetControl() ) != 0;
}

bool Control::isControlHuman( void ) const
{
    return ( CONTROL_HUMAN & GetControl() ) != 0;
}

bool Control::isControlLocal( void ) const
{
    return !isControlRemote();
}

bool Control::isControlRemote( void ) const
{
    return ( CONTROL_REMOTE & GetControl() ) != 0;
}

Player::Player( int col )
    : control( CONTROL_NONE )
    , color( col )
    , race( Race::NONE )
    , friends( col )
    , id( World::GetUniq() )
{
    name = Color::String( color );
}

const std::string & Player::GetName( void ) const
{
    return name;
}

Focus & Player::GetFocus( void )
{
    return focus;
}

const Focus & Player::GetFocus( void ) const
{
    return focus;
}

int Player::GetControl( void ) const
{
    return control;
}

int Player::GetColor( void ) const
{
    return color;
}

int Player::GetRace( void ) const
{
    return race;
}

int Player::GetFriends( void ) const
{
    return friends;
}

int Player::GetID( void ) const
{
    return id;
}

bool Player::isID( u32 id2 ) const
{
    return id2 == id;
}

bool Player::isColor( int col ) const
{
    return col == color;
}

bool Player::isName( const std::string & str ) const
{
    return str == name;
}

bool Player::isPlay( void ) const
{
    return Modes( ST_INGAME );
}

void Player::SetFriends( int f )
{
    friends = f;
}

void Player::SetName( const std::string & n )
{
    name = n;
}

void Player::SetControl( int ctl )
{
    control = ctl;
}

void Player::SetColor( int cl )
{
    color = cl;
}

void Player::SetRace( int r )
{
    race = r;
}

void Player::SetPlay( bool f )
{
    if ( f )
        SetModes( ST_INGAME );
    else
        ResetModes( ST_INGAME );
}

StreamBase & operator<<( StreamBase & msg, const Focus & focus )
{
    msg << focus.first;

    switch ( focus.first ) {
    case FOCUS_HEROES:
        msg << reinterpret_cast<Heroes *>( focus.second )->GetIndex();
        break;
    case FOCUS_CASTLE:
        msg << reinterpret_cast<Castle *>( focus.second )->GetIndex();
        break;
    default:
        msg << static_cast<s32>( -1 );
        break;
    }

    return msg;
}

StreamBase & operator>>( StreamBase & msg, Focus & focus )
{
    s32 index;
    msg >> focus.first >> index;

    switch ( focus.first ) {
    case FOCUS_HEROES:
        focus.second = world.GetHeroes( Maps::GetPoint( index ) );
        break;
    case FOCUS_CASTLE:
        focus.second = world.GetCastle( Maps::GetPoint( index ) );
        break;
    default:
        focus.second = NULL;
        break;
    }

    return msg;
}

StreamBase & operator<<( StreamBase & msg, const Player & player )
{
    const BitModes & modes = player;

    return msg << modes << player.id << player.control << player.color << player.race << player.friends << player.name << player.focus;
}

StreamBase & operator>>( StreamBase & msg, Player & player )
{
    BitModes & modes = player;

    return msg >> modes >> player.id >> player.control >> player.color >> player.race >> player.friends >> player.name >> player.focus;
}

Players::Players()
    : current_color( 0 )
{
    reserve( KINGDOMMAX );
}

Players::~Players()
{
    clear();
}

void Players::clear( void )
{
    for ( iterator it = begin(); it != end(); ++it )
        delete *it;

    std::vector<Player *>::clear();

    for ( u32 ii = 0; ii < KINGDOMMAX + 1; ++ii )
        _players[ii] = NULL;

    current_color = 0;
    human_colors = 0;
}

void Players::Init( int colors )
{
    clear();

    const Colors vcolors( colors );

    for ( Colors::const_iterator it = vcolors.begin(); it != vcolors.end(); ++it ) {
        push_back( new Player( *it ) );
        _players[Color::GetIndex( *it )] = back();
    }

    DEBUG( DBG_GAME, DBG_INFO, "Players: " << String() );
}

void Players::Init( const Maps::FileInfo & fi )
{
    if ( fi.kingdom_colors ) {
        clear();
        const Colors vcolors( fi.kingdom_colors );

        Player * first = NULL;

        for ( Colors::const_iterator it = vcolors.begin(); it != vcolors.end(); ++it ) {
            Player * player = new Player( *it );
            player->SetRace( fi.KingdomRace( *it ) );
            player->SetControl( CONTROL_AI );
            player->SetFriends( *it | fi.unions[Color::GetIndex( *it )] );

            if ( ( *it & fi.HumanOnlyColors() ) && Settings::Get().IsGameType( Game::TYPE_MULTI ) )
                player->SetControl( CONTROL_HUMAN );
            else if ( *it & fi.AllowHumanColors() )
                player->SetControl( player->GetControl() | CONTROL_HUMAN );

            if ( !first && ( player->GetControl() & CONTROL_HUMAN ) )
                first = player;

            push_back( player );
            _players[Color::GetIndex( *it )] = back();
        }

        if ( first )
            first->SetControl( CONTROL_HUMAN );

        DEBUG( DBG_GAME, DBG_INFO, "Players: " << String() );
    }
    else {
        DEBUG( DBG_GAME, DBG_INFO,
               "Players: "
                   << "unknown colors" );
    }
}

void Players::Set( const int color, Player * player )
{
    assert( color >= 0 && color < playersSize );
    _players[color] = player;
}

Player * Players::Get( int color )
{
    return _players[Color::GetIndex( color )];
}

bool Players::isFriends( int player, int colors )
{
    const Player * ptr = Get( player );
    return ptr ? ( ptr->GetFriends() & colors ) != 0 : false;
}

void Players::SetPlayerRace( int color, int race )
{
    Player * player = Get( color );

    if ( player )
        player->SetRace( race );
}

void Players::SetPlayerControl( int color, int ctrl )
{
    Player * player = Get( color );

    if ( player )
        player->SetControl( ctrl );
}

int Players::GetColors( int control, bool strong ) const
{
    int res = 0;

    for ( const_iterator it = begin(); it != end(); ++it )
        if ( control == 0xFF || ( strong && ( *it )->GetControl() == control ) || ( !strong && ( ( *it )->GetControl() & control ) ) )
            res |= ( *it )->GetColor();

    return res;
}

int Players::GetActualColors( void ) const
{
    int res = 0;

    for ( const_iterator it = begin(); it != end(); ++it )
        if ( ( *it )->isPlay() )
            res |= ( *it )->GetColor();

    return res;
}

Player * Players::GetCurrent( void )
{
    return Get( current_color );
}

const Player * Players::GetCurrent( void ) const
{
    return Get( current_color );
}

int Players::GetPlayerFriends( int color )
{
    const Player * player = Get( color );
    return player ? player->GetFriends() : 0;
}

int Players::GetPlayerControl( int color )
{
    const Player * player = Get( color );
    return player ? player->GetControl() : CONTROL_NONE;
}

int Players::GetPlayerRace( int color )
{
    const Player * player = Get( color );
    return player ? player->GetRace() : Race::NONE;
}

bool Players::GetPlayerInGame( int color )
{
    const Player * player = Get( color );
    return player && player->isPlay();
}

void Players::SetPlayerInGame( int color, bool f )
{
    Player * player = Get( color );
    if ( player )
        player->SetPlay( f );
}

void Players::SetStartGame( void )
{
    for_each( begin(), end(), []( Player * player ) { player->SetPlay( true ); } );
    for_each( begin(), end(), []( Player * player ) { PlayerFocusReset( player ); } );
    for_each( begin(), end(), []( Player * player ) { PlayerFixRandomRace( player ); } );
    for_each( begin(), end(), []( Player * player ) { PlayerFixMultiControl( player ); } );

    current_color = Color::NONE;
    human_colors = Color::NONE;

    DEBUG( DBG_GAME, DBG_INFO, String() );
}

int Players::HumanColors( void )
{
    if ( 0 == human_colors )
        human_colors = Settings::Get().GetPlayers().GetColors( CONTROL_HUMAN, true );
    return human_colors;
}

int Players::FriendColors( void )
{
    int colors = 0;
    const Players & players = Settings::Get().GetPlayers();

    if ( players.current_color & Players::HumanColors() ) {
        const Player * player = players.GetCurrent();
        if ( player )
            colors = player->GetFriends();
    }
    else
        colors = Players::HumanColors();

    return colors;
}

std::string Players::String( void ) const
{
    std::ostringstream os;
    os << "Players: ";

    for ( const_iterator it = begin(); it != end(); ++it ) {
        os << Color::String( ( *it )->GetColor() ) << "(" << Race::String( ( *it )->GetRace() ) << ", ";

        switch ( ( *it )->GetControl() ) {
        case CONTROL_AI | CONTROL_HUMAN:
            os << "ai|human";
            break;

        case CONTROL_AI:
            os << "ai";
            break;

        case CONTROL_HUMAN:
            os << "human";
            break;

        case CONTROL_REMOTE:
            os << "remote";
            break;

        default:
            os << "unknown";
            break;
        }

        os << ")"
           << ", ";
    }

    return os.str();
}

StreamBase & operator<<( StreamBase & msg, const Players & players )
{
    msg << players.GetColors() << players.current_color;

    for ( Players::const_iterator it = players.begin(); it != players.end(); ++it )
        msg << ( **it );

    return msg;
}

StreamBase & operator>>( StreamBase & msg, Players & players )
{
    int colors, current;
    msg >> colors >> current;

    players.clear();
    players.current_color = current;
    const Colors vcolors( colors );

    for ( u32 ii = 0; ii < vcolors.size(); ++ii ) {
        Player * player = new Player();
        msg >> *player;
        Players::Set( Color::GetIndex( player->GetColor() ), player );
        players.push_back( player );
    }

    return msg;
}
