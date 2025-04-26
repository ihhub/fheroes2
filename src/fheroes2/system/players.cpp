/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2025                                             *
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
#include "rand.h"
#include "save_format_version.h"
#include "serialize.h"
#include "settings.h"
#include "world.h"

namespace
{
    std::array<Player *, maxNumOfPlayers + 1> playersArray{};
    Color::PlayerColor humanColors{ Color::PlayerColor::NONE };

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

Player::Player( const Color::PlayerColor color /* = Color::PlayerColor::NONE */ )
    : _aiPersonality( AI::getRandomPersonality() )
    , _color( color )
    , _friendsColors( color )
{
    // Do nothing.
}

std::string Player::GetName() const
{
    if ( _name.empty() ) {
        return GetDefaultName();
    }

    return _name;
}

int Player::GetControl() const
{
#if defined( WITH_DEBUG )
    if ( _isAIAutoControlMode ) {
        assert( ( _control & CONTROL_HUMAN ) == CONTROL_HUMAN );
        return CONTROL_AI;
    }
#endif

    return _control;
}

std::string Player::GetPersonalityString() const
{
    return AI::getPersonalityString( _aiPersonality );
}

bool Player::isPlay() const
{
    return Modes( ST_INGAME );
}

void Player::SetName( std::string newName )
{
    if ( newName == GetDefaultName() ) {
        _name.clear();
    }
    else {
        _name = std::move( newName );
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

    assert( !( _control & CONTROL_AI ) );

    _handicapStatus = status;
}

#if defined( WITH_DEBUG )
void Player::setAIAutoControlMode( const bool enable )
{
    assert( ( _control & CONTROL_HUMAN ) == CONTROL_HUMAN );

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
    assert( ( _control & CONTROL_HUMAN ) == CONTROL_HUMAN );

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

    return stream << modes << player._control << player._color << player._race << player._friendsColors << player._name << player._focus << player._aiPersonality
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

    stream >> player._control;

    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_1109_RELEASE, "Remove the logic below." );
    if ( Game::GetVersionOfCurrentSaveFile() < FORMAT_VERSION_1109_RELEASE ) {
        int playerColor;
        int friendsColors;

        stream >> playerColor >> player._race >> friendsColors;

        player._color = static_cast<Color::PlayerColor>( playerColor );
        player._friendsColors = static_cast<Color::PlayerColor>( friendsColors );
    }
    else {
        stream >> player._color >> player._race >> player._friendsColors;
    }

    return stream >> player._name >> player._focus >> player._aiPersonality >> player._handicapStatus;
}

void Players::clear()
{
    std::for_each( begin(), end(), []( Player * player ) { delete player; } );

    std::vector<Player *>::clear();

    _currentColor = Color::PlayerColor::NONE;

    playersArray = {};
    humanColors = Color::PlayerColor::NONE;
}

void Players::Init( const Color::PlayerColor colors )
{
    clear();

    const Color::PlayerColors vcolors( colors );

    for ( const Color::PlayerColor color : vcolors ) {
        push_back( new Player( color ) );
        playersArray[Color::GetIndex( color )] = back();
    }

    DEBUG_LOG( DBG_GAME, DBG_INFO, "Players: " << String() )
}

void Players::Init( const Maps::FileInfo & fi )
{
    if ( fi.kingdomColors == Color::PlayerColor::NONE ) {
        DEBUG_LOG( DBG_GAME, DBG_INFO, "No players are set." )
        return;
    }

    clear();
    const Color::PlayerColors vcolors( fi.kingdomColors );

    Player * first = nullptr;

    for ( const Color::PlayerColor color : vcolors ) {
        Player * player = new Player( color );
        player->SetRace( fi.KingdomRace( color ) );
        player->SetControl( CONTROL_AI );
        player->SetFriends( color | fi.unions[Color::GetIndex( color )] );

        if ( Color::haveCommonColors( fi.HumanOnlyColors(), color ) && Settings::Get().IsGameType( Game::TYPE_MULTI ) ) {
            player->SetControl( CONTROL_HUMAN );
        }
        else if ( Color::haveCommonColors( fi.colorsAvailableForHumans, color ) ) {
            player->SetControl( player->GetControl() | CONTROL_HUMAN );
        }

        if ( !first && ( player->GetControl() & CONTROL_HUMAN ) ) {
            first = player;
        }

        push_back( player );
        playersArray[Color::GetIndex( color )] = back();
    }

    if ( first ) {
        first->SetControl( CONTROL_HUMAN );
    }

    DEBUG_LOG( DBG_GAME, DBG_INFO, "Players: " << String() )
}

void Players::Set( const Color::PlayerColor color, Player * player )
{
    playersArray[Color::GetIndex( color )] = player;
}

Player * Players::Get( const Color::PlayerColor color )
{
    return playersArray[Color::GetIndex( color )];
}

bool Players::isFriends( const Color::PlayerColor playerColor, Color::PlayerColor colors )
{
    const Player * ptr = Get( playerColor );
    return ptr ? ( ptr->GetFriends() & colors ) != Color::PlayerColor::NONE : false;
}

void Players::SetPlayerRace( const Color::PlayerColor color, const int race )
{
    Player * player = Get( color );

    if ( player ) {
        player->SetRace( race );
    }
}

void Players::SetPlayerControl( const Color::PlayerColor color, const int control )
{
    Player * player = Get( color );

    if ( player ) {
        player->SetControl( control );
    }
}

Color::PlayerColor Players::GetColors( const int control, const bool strong ) const
{
    Color::PlayerColor res = Color::PlayerColor::NONE;

    for ( const Player * player : *this ) {
        if ( control == 0xFF || ( strong && player->GetControl() == control ) || ( !strong && ( player->GetControl() & control ) ) ) {
            res |= player->GetColor();
        }
    }

    return res;
}

Player * Players::GetCurrent()
{
    return Get( _currentColor );
}

const Player * Players::GetCurrent() const
{
    return Get( _currentColor );
}

Color::PlayerColor Players::GetActualColors() const
{
    Color::PlayerColor res = Color::PlayerColor::NONE;

    for ( const Player * player : *this ) {
        if ( player->isPlay() ) {
            res |= player->GetColor();
        }
    }

    return res;
}

Color::PlayerColor Players::GetPlayerFriends( const Color::PlayerColor color )
{
    const Player * player = Get( color );
    return player ? player->GetFriends() : Color::PlayerColor::NONE;
}

int Players::GetPlayerControl( const Color::PlayerColor color )
{
    const Player * player = Get( color );
    return player ? player->GetControl() : CONTROL_NONE;
}

int Players::GetPlayerRace( const Color::PlayerColor color )
{
    const Player * player = Get( color );
    return player ? player->GetRace() : Race::NONE;
}

bool Players::GetPlayerInGame( const Color::PlayerColor color )
{
    const Player * player = Get( color );
    return player && player->isPlay();
}

std::vector<Color::PlayerColor> Players::getInPlayOpponents( const Color::PlayerColor color )
{
    std::vector<Color::PlayerColor> opponentColors;

    const Player * playerOfColor = Players::Get( color );
    assert( playerOfColor != nullptr );

    const Color::PlayerColor friends = playerOfColor->GetFriends();

    for ( const Player * player : Settings::Get().GetPlayers() ) {
        assert( player != nullptr );

        const Color::PlayerColor currentColor = player->GetColor();

        if ( player->isPlay() && ( ( currentColor & friends ) == Color::PlayerColor::NONE ) ) {
            opponentColors.emplace_back( currentColor );
        }
    }

    return opponentColors;
}

void Players::SetPlayerInGame( const Color::PlayerColor color, bool isPlay )
{
    Player * player = Get( color );
    if ( player == nullptr ) {
        return;
    }

    player->SetPlay( isPlay );
}

void Players::SetStartGame()
{
    std::vector<int> races = { Race::KNGT, Race::BARB, Race::SORC, Race::WRLK, Race::WZRD, Race::NECR };

    for_each( begin(), end(), []( Player * player ) { player->SetPlay( true ); } );
    for_each( begin(), end(), []( Player * player ) { resetFocus( player ); } );
    for_each( begin(), end(), [&races]( const Player * player ) { removeAlreadySelectedRaces( player, races ); } );
    for_each( begin(), end(), [&races]( Player * player ) { fixRandomRace( player, races ); } );
    for_each( begin(), end(), []( Player * player ) { fixMultiControl( player ); } );

    _currentColor = Color::PlayerColor::NONE;
    humanColors = Color::PlayerColor::NONE;

    DEBUG_LOG( DBG_GAME, DBG_INFO, String() )
}

Color::PlayerColor Players::HumanColors()
{
    if ( humanColors == Color::PlayerColor::NONE ) {
        humanColors = Settings::Get().GetPlayers().GetColors( CONTROL_HUMAN, true );
    }

    return humanColors;
}

Color::PlayerColor Players::FriendColors()
{
    const Player * player = Settings::Get().GetPlayers().GetCurrent();
    if ( player ) {
        return player->GetFriends();
    }

    return Color::PlayerColor::NONE;
}

void Players::setCurrentColor( const Color::PlayerColor color )
{
    // We can set only one of 6 player colors ( BLUE | GREEN | RED | YELLOW | ORANGE | PURPLE ) or NONE (neutral player).
    assert( Color::Count( color ) == 1 || color == Color::PlayerColor::NONE );

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
    Color::PlayerColor colors;
    Color::PlayerColor current;

    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_1109_RELEASE, "Remove the logic below." );
    if ( Game::GetVersionOfCurrentSaveFile() < FORMAT_VERSION_1109_RELEASE ) {
        int colorsTemp{ 0 };
        int currentTemp{ 0 };

        stream >> colorsTemp >> currentTemp;

        colors = static_cast<Color::PlayerColor>( colorsTemp );
        current = static_cast<Color::PlayerColor>( currentTemp );
    }
    else {
        stream >> colors >> current;
    }

    players.clear();
    players.setCurrentColor( current );

    const Color::PlayerColors vcolors( colors );
    std::for_each( vcolors.begin(), vcolors.end(), [&stream, &players]( const Color::PlayerColor /* color */ ) {
        Player * player = new Player();
        stream >> *player;

        Players::Set( player->GetColor(), player );

        players.push_back( player );
    } );

    return stream;
}
