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

#ifndef H2PLAYERS_H
#define H2PLAYERS_H

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "ai_personality.h"
#include "bitmodes.h"
#include "color.h"

class IStreamBase;
class OStreamBase;

namespace Maps
{
    struct FileInfo;
}

class Castle;
class Heroes;

// Maximum number of players on the map
inline constexpr int maxNumOfPlayers{ 6 };

enum
{
    CONTROL_NONE = 0,
    CONTROL_HUMAN = 1,
    // CONTROL_REMOTE = 2,
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

OStreamBase & operator<<( OStreamBase & stream, const Focus & focus );
IStreamBase & operator>>( IStreamBase & stream, Focus & focus );

struct Control
{
    virtual int GetControl() const = 0;
    virtual ~Control() = default;

    bool isControlAI() const;
    bool isControlHuman() const;
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

    explicit Player( const int col = Color::NONE );

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

    void SetPlay( const bool f );

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

#if defined( WITH_DEBUG )
    bool isAIAutoControlMode() const
    {
        return _isAIAutoControlMode;
    }

    // Sets whether a given human player is controlled by AI. See the implementation for details.
    void setAIAutoControlMode( const bool enable );

    // Turns the planned value of whether a given human player is controlled by AI into the actual value.
    // Should be called only if this mode is actually enabled.
    void commitAIAutoControlMode();
#endif

protected:
    friend OStreamBase & operator<<( OStreamBase & stream, const Player & player );
    friend IStreamBase & operator>>( IStreamBase & stream, Player & player );

    int control;
    int color;
    int race;
    int friends;
    std::string name;
    Focus focus;
    AI::Personality _aiPersonality{ AI::Personality::NONE };
    HandicapStatus _handicapStatus;

#if defined( WITH_DEBUG )
    // These members should not be saved anywhere

    // Actual value of whether a given human player is controlled by AI
    bool _isAIAutoControlMode;
    // Planned value of whether a given human player is controlled by AI (will become actual upon committing it)
    bool _isAIAutoControlModePlanned;
#endif
};

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
    static std::vector<int> getInPlayOpponents( const int color );
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
    void setCurrentColor( const int color );

private:
    int _currentColor{ Color::NONE };
};

OStreamBase & operator<<( OStreamBase & stream, const Players & players );
IStreamBase & operator>>( IStreamBase & stream, Players & players );

#endif
