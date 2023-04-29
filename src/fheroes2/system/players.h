/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
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

#ifndef H2PLAYERS_H
#define H2PLAYERS_H

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "bitmodes.h"
#include "color.h"

class StreamBase;

namespace Maps
{
    struct FileInfo;
}

namespace AI
{
    class Base;
}

class Castle;
class Heroes;

// control_t
enum
{
    CONTROL_NONE = 0,
    CONTROL_HUMAN = 1,
    CONTROL_REMOTE = 2, /*, CONTROL_LOCAL = CONTROL_AI | CONTROL_HUMAN */
    CONTROL_AI = 4
};
enum
{
    FOCUS_UNSEL = 0,
    FOCUS_HEROES = 1,
    FOCUS_CASTLE = 2
};

struct Focus : std::pair<int, void *>
{
    Focus()
        : std::pair<int, void *>( FOCUS_UNSEL, nullptr )
    {}

    void Reset()
    {
        first = FOCUS_UNSEL;
        second = nullptr;
    }
    void Set( Castle * ptr )
    {
        first = FOCUS_CASTLE;
        second = ptr;
    }
    void Set( Heroes * ptr )
    {
        first = FOCUS_HEROES;
        second = ptr;
    }

    Castle * GetCastle()
    {
        return first == FOCUS_CASTLE && second ? static_cast<Castle *>( second ) : nullptr;
    }
    Heroes * GetHeroes()
    {
        return first == FOCUS_HEROES && second ? static_cast<Heroes *>( second ) : nullptr;
    }
};

struct Control
{
    virtual int GetControl() const = 0;
    virtual ~Control() = default;

    bool isControlAI() const;
    bool isControlHuman() const;
    bool isControlRemote() const;
    bool isControlLocal() const;
};

class Player : public BitModes, public Control
{
public:
    enum class HandicapStatus : uint8_t
    {
        NONE, // No strings attached.
        MILD, // 15% fewer resources per turn
        SEVERE, // 30% fewer resources per turn
    };

    explicit Player( int col = Color::NONE );
    ~Player() override = default;

    bool isColor( int col ) const
    {
        return col == color;
    }

    bool isPlay() const;

    void SetColor( int cl )
    {
        color = cl;
    }

    void SetRace( int r )
    {
        race = r;
    }

    void SetControl( int ctl )
    {
        control = ctl;
    }

    void SetPlay( bool );

    void SetFriends( int f )
    {
        friends = f;
    }

    void SetName( const std::string & newName );

    int GetControl() const override;

    int GetColor() const
    {
        return color;
    }

    int GetRace() const
    {
        return race;
    }

    int GetFriends() const
    {
        return friends;
    }

    std::string GetDefaultName() const;
    std::string GetName() const;

    std::string GetPersonalityString() const;

    Focus & GetFocus()
    {
        return focus;
    }

    HandicapStatus getHandicapStatus() const
    {
        return _handicapStatus;
    }

    void setHandicapStatus( const HandicapStatus status );

    // This mode sets control from a human player to AI so the game will be continued by AI.
    void setAIAutoControlMode( const bool enable );

    bool isAIAutoControlMode() const
    {
        return _isAIAutoControlMode;
    }

protected:
    friend StreamBase & operator<<( StreamBase &, const Player & );
    friend StreamBase & operator>>( StreamBase &, Player & );

    int control;
    int color;
    int race;
    int friends;
    std::string name;
    uint32_t id;
    Focus focus;
    std::shared_ptr<AI::Base> _ai;
    HandicapStatus _handicapStatus;

    // This member should not be saved anywhere.
    bool _isAIAutoControlMode;
};

StreamBase & operator<<( StreamBase &, const Player & );
StreamBase & operator>>( StreamBase &, Player & );

class Players : public std::vector<Player *>
{
public:
    Players();
    Players( const Players & ) = delete;

    ~Players();

    Players & operator=( const Players & ) = delete;

    void Init( int colors );
    void Init( const Maps::FileInfo & );
    void clear();

    void SetStartGame();
    int GetColors( int control = 0xFF, bool strong = false ) const;
    int GetActualColors() const;
    std::string String() const;

    const std::vector<Player *> & getVector() const;

    Player * GetCurrent();
    const Player * GetCurrent() const;

    static void Set( const int color, Player * player );
    static Player * Get( int color );
    static int GetPlayerControl( int color );
    static int GetPlayerRace( int color );
    static int GetPlayerFriends( int color );
    static bool GetPlayerInGame( int color );
    static bool isFriends( int player, int colors );
    static void SetPlayerRace( int color, int race );
    static void SetPlayerControl( int color, int ctrl );
    static void SetPlayerInGame( int color, bool );
    static int HumanColors();
    // Return current player friends colors, if player does not exist he has no friends (returns 0).
    static int FriendColors();

    int getCurrentColor() const
    {
        return _currentColor;
    }

    // The color should belong to one player or be NONE (neutral player).
    void setCurrentColor( int color );

private:
    int _currentColor{ Color::NONE };
};

StreamBase & operator<<( StreamBase &, const Players & );
StreamBase & operator>>( StreamBase &, Players & );

#endif
