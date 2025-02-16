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

#include "players.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <sstream>

#include "castle.h"
#include "game.h"
#include "game_io.h"
#include "heroes.h"
#include "logging.h"
#include "maps.h"
#include "maps_fileinfo.h"
#include "race.h"
#include "rand.h"
#include "save_format_version.h"
#include "serialize.h"
#include "settings.h"
#include "world.h"

namespace
{
    std::array<Player *, maxNumOfPlayers + 1> playersArray{};
    int humanColors{ Color::NONE };

    enum
    {
        ST_INGAME = 0x2000
    };

    void resetFocus( Player * player )
    {
        if ( player == nullptr ) {
            return;
        }

        player->GetFocus().Reset();
    }

    void fixMultiControl( Player * player )
    {
        if ( player == nullptr ) {
            return;
        }

        if ( player->GetControl() != ( CONTROL_HUMAN | CONTROL_AI ) ) {
            return;
        }

        player->SetControl( CONTROL_AI );
    }

    void removeAlreadySelectedRaces( const Player * player, std::vector<int> & availableRaces )
    {
        const int raceToRemove = player->GetRace();

        availableRaces.erase( remove_if( availableRaces.begin(), availableRaces.end(), [raceToRemove]( const int race ) { return raceToRemove == race; } ),
                              availableRaces.end() );
    }

    void fixRandomRace( Player * player, std::vector<int> & availableRaces )
    {
        if ( player == nullptr ) {
            return;
        }

        if ( player->GetRace() != Race::RAND ) {
            return;
        }

        if ( availableRaces.empty() ) {
            player->SetRace( Race::Rand() );

            return;
        }

        const size_t raceIndex = Rand::Get( 0, static_cast<uint32_t>( availableRaces.size() - 1 ) );

        player->SetRace( availableRaces[raceIndex] );

        availableRaces.erase( availableRaces.begin() + raceIndex );
    }
}

bool Control::isControlAI() const
{
    return ( CONTROL_AI & GetControl() ) != 0;
}

bool Control::isControlHuman() const
{
    return ( CONTROL_HUMAN & GetControl() ) != 0;
}

Player::Player( const int col /* = Color::NONE */ )
    : control( CONTROL_NONE )
    , color( col )
    , race( Race::NONE )
    , friends( col )
    , _aiPersonality( AI::getRandomPersonality() )
    , _handicapStatus( HandicapStatus::NONE )
#if defined( WITH_DEBUG )
    , _isAIAutoControlMode( false )
    , _isAIAutoControlModePlanned( false )
#endif
{}

std::string Player::GetDefaultName() const
{
    return Color::String( color );
}

std::string Player::GetName() const
{
    if ( name.empty() ) {
        return GetDefaultName();
    }

    return name;
}

int Player::GetControl() const
{
#if defined( WITH_DEBUG )
    if ( _isAIAutoControlMode ) {
        assert( ( control & CONTROL_HUMAN ) == CONTROL_HUMAN );
        return CONTROL_AI;
    }
#endif

    return control;
}

std::string Player::GetPersonalityString() const
{
    return AI::getPersonalityString( _aiPersonality );
}

bool Player::isPlay() const
{
    return Modes( ST_INGAME );
}

void Player::SetName( const std::string & newName )
{
    if ( newName == GetDefaultName() ) {
        name.clear();
    }
    else {
        name = newName;
    }
}

void Player::SetPlay( const bool f )
{
    if ( f ) {
        SetModes( ST_INGAME );
    }
    else {
        ResetModes( ST_INGAME );
    }
}

void Player::setHandicapStatus( const HandicapStatus status )
{
    if ( status == HandicapStatus::NONE ) {
        _handicapStatus = status;
        return;
    }

    assert( !( control & CONTROL_AI ) );

    _handicapStatus = status;
}

#if defined( WITH_DEBUG )
void Player::setAIAutoControlMode( const bool enable )
{
    assert( ( control & CONTROL_HUMAN ) == CONTROL_HUMAN );

    // If this mode should be enabled, then it happens immediately
    if ( enable ) {
        _isAIAutoControlMode = enable;
    }
    // Otherwise, the change is first planned and then committed, in which case this mode should be actually enabled
    else {
        assert( _isAIAutoControlMode );
    }

    _isAIAutoControlModePlanned = enable;
}

void Player::commitAIAutoControlMode()
{
    assert( ( control & CONTROL_HUMAN ) == CONTROL_HUMAN );

    // If this method has been called, then this mode should be actually enabled
    assert( _isAIAutoControlMode );

    _isAIAutoControlMode = _isAIAutoControlModePlanned;
}
#endif

OStreamBase & operator<<( OStreamBase & stream, const Focus & focus )
{
    stream << focus.first;

    switch ( focus.first ) {
    case FOCUS_HEROES:
        stream << static_cast<Heroes *>( focus.second )->GetIndex();
        break;
    case FOCUS_CASTLE:
        stream << static_cast<Castle *>( focus.second )->GetIndex();
        break;
    default:
        stream << static_cast<int32_t>( -1 );
        break;
    }

    return stream;
}

IStreamBase & operator>>( IStreamBase & stream, Focus & focus )
{
    int32_t index;
    stream >> focus.first >> index;

    switch ( focus.first ) {
    case FOCUS_HEROES:
        focus.second = world.GetHeroes( Maps::GetPoint( index ) );
        break;
    case FOCUS_CASTLE:
        focus.second = world.getCastle( Maps::GetPoint( index ) );
        break;
    default:
        focus.second = nullptr;
        break;
    }

    return stream;
}

OStreamBase & operator<<( OStreamBase & stream, const Player & player )
{
    const BitModes & modes = player;

    return stream << modes << player.control << player.color << player.race << player.friends << player.name << player.focus << player._aiPersonality
                  << player._handicapStatus;
}

IStreamBase & operator>>( IStreamBase & stream, Player & player )
{
    BitModes & modes = player;

    stream >> modes;

    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_1009_RELEASE, "Remove the logic below." );
    if ( Game::GetVersionOfCurrentSaveFile() < FORMAT_VERSION_1009_RELEASE ) {
        uint32_t temp;
        stream >> temp;
    }

    return stream >> player.control >> player.color >> player.race >> player.friends >> player.name >> player.focus >> player._aiPersonality >> player._handicapStatus;
}

Players::Players()
{
    reserve( maxNumOfPlayers );
}

Players::~Players()
{
    clear();
}

void Players::clear()
{
    std::for_each( begin(), end(), []( Player * player ) { delete player; } );

    std::vector<Player *>::clear();

    _currentColor = Color::NONE;

    playersArray = {};
    humanColors = Color::NONE;
}

void Players::Init( int colors )
{
    clear();

    const Colors vcolors( colors );

    for ( Colors::const_iterator it = vcolors.begin(); it != vcolors.end(); ++it ) {
        push_back( new Player( *it ) );
        playersArray[Color::GetIndex( *it )] = back();
    }

    DEBUG_LOG( DBG_GAME, DBG_INFO, "Players: " << String() )
}

void Players::Init( const Maps::FileInfo & fi )
{
    if ( fi.kingdomColors == 0 ) {
        DEBUG_LOG( DBG_GAME, DBG_INFO, "No players are set." )
        return;
    }

    clear();
    const Colors vcolors( fi.kingdomColors );

    Player * first = nullptr;

    for ( const int color : vcolors ) {
        Player * player = new Player( color );
        player->SetRace( fi.KingdomRace( color ) );
        player->SetControl( CONTROL_AI );
        player->SetFriends( color | fi.unions[Color::GetIndex( color )] );

        if ( ( color & fi.HumanOnlyColors() ) && Settings::Get().IsGameType( Game::TYPE_MULTI ) )
            player->SetControl( CONTROL_HUMAN );
        else if ( color & fi.colorsAvailableForHumans )
            player->SetControl( player->GetControl() | CONTROL_HUMAN );

        if ( !first && ( player->GetControl() & CONTROL_HUMAN ) )
            first = player;

        push_back( player );
        playersArray[Color::GetIndex( color )] = back();
    }

    if ( first )
        first->SetControl( CONTROL_HUMAN );

    DEBUG_LOG( DBG_GAME, DBG_INFO, "Players: " << String() )
}

void Players::Set( const int color, Player * player )
{
    playersArray[Color::GetIndex( color )] = player;
}

Player * Players::Get( int color )
{
    return playersArray[Color::GetIndex( color )];
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

int Players::GetActualColors() const
{
    int res = 0;

    for ( const_iterator it = begin(); it != end(); ++it )
        if ( ( *it )->isPlay() )
            res |= ( *it )->GetColor();

    return res;
}

const std::vector<Player *> & Players::getVector() const
{
    return *this;
}

Player * Players::GetCurrent()
{
    return Get( _currentColor );
}

const Player * Players::GetCurrent() const
{
    return Get( _currentColor );
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

std::vector<int> Players::getInPlayOpponents( const int color )
{
    std::vector<int> opponentColors;

    const Player * playerOfColor = Players::Get( color );
    assert( playerOfColor != nullptr );

    const int friends = playerOfColor->GetFriends();

    for ( const Player * player : Settings::Get().GetPlayers() ) {
        assert( player != nullptr );

        const int currentColor = player->GetColor();

        if ( player->isPlay() && ( ( currentColor & friends ) == 0 ) ) {
            opponentColors.emplace_back( currentColor );
        }
    }

    return opponentColors;
}

void Players::SetPlayerInGame( int color, bool f )
{
    Player * player = Get( color );
    if ( player == nullptr ) {
        return;
    }

    player->SetPlay( f );
}

void Players::SetStartGame()
{
    std::vector<int> races = { Race::KNGT, Race::BARB, Race::SORC, Race::WRLK, Race::WZRD, Race::NECR };

    for_each( begin(), end(), []( Player * player ) { player->SetPlay( true ); } );
    for_each( begin(), end(), []( Player * player ) { resetFocus( player ); } );
    for_each( begin(), end(), [&races]( const Player * player ) { removeAlreadySelectedRaces( player, races ); } );
    for_each( begin(), end(), [&races]( Player * player ) { fixRandomRace( player, races ); } );
    for_each( begin(), end(), []( Player * player ) { fixMultiControl( player ); } );

    _currentColor = Color::NONE;
    humanColors = Color::NONE;

    DEBUG_LOG( DBG_GAME, DBG_INFO, String() )
}

int Players::HumanColors()
{
    if ( humanColors == Color::NONE ) {
        humanColors = Settings::Get().GetPlayers().GetColors( CONTROL_HUMAN, true );
    }

    return humanColors;
}

int Players::FriendColors()
{
    const Player * player = Settings::Get().GetPlayers().GetCurrent();
    if ( player ) {
        return player->GetFriends();
    }

    return 0;
}

void Players::setCurrentColor( const int color )
{
    // We can set only one of 6 player colors ( BLUE | GREEN | RED | YELLOW | ORANGE | PURPLE ) or NONE (neutral player).
    assert( Color::Count( color ) == 1 || color == Color::NONE );

    _currentColor = color;
}

std::string Players::String() const
{
    std::ostringstream os;
    os << "Players: ";

    for ( const_iterator it = begin(); it != end(); ++it ) {
        os << Color::String( ( *it )->GetColor() ) << "(" << Race::String( ( *it )->GetRace() ) << ", ";

        switch ( ( *it )->GetControl() ) {
        case CONTROL_AI | CONTROL_HUMAN:
            os << "ai|human, " << ( *it )->GetPersonalityString();
            break;

        case CONTROL_AI:
            os << "ai, " << ( *it )->GetPersonalityString();
            break;

        case CONTROL_HUMAN:
            os << "human";
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

OStreamBase & operator<<( OStreamBase & stream, const Players & players )
{
    stream << players.GetColors() << players.getCurrentColor();

    std::for_each( players.begin(), players.end(), [&stream]( const Player * player ) {
        assert( player != nullptr );

        stream << *player;
    } );

    return stream;
}

IStreamBase & operator>>( IStreamBase & stream, Players & players )
{
    int colors{ 0 };
    int current{ 0 };
    stream >> colors >> current;

    players.clear();
    players.setCurrentColor( current );

    const Colors vcolors( colors );
    std::for_each( vcolors.begin(), vcolors.end(), [&stream, &players]( const int /* color */ ) {
        Player * player = new Player();
        stream >> *player;

        Players::Set( player->GetColor(), player );

        players.push_back( player );
    } );

    return stream;
}
