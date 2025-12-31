/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2026                                             *
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

#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "ai_personality.h"
#include "bitmodes.h"
#include "color.h"
#include "race.h"

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

struct Focus : std::pair<int32_t, void *>
{
    Focus()
        : std::pair<int32_t, void *>( FOCUS_UNSEL, nullptr )
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

    bool isControlAI() const
    {
        return ( CONTROL_AI & GetControl() ) != 0;
    }

    bool isControlHuman() const
    {
        return ( CONTROL_HUMAN & GetControl() ) != 0;
    }
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

    explicit Player( const PlayerColor color = PlayerColor::NONE );

    ~Player() override = default;

    bool isColor( const PlayerColor color ) const
    {
        return color == _color;
    }

    bool isPlay() const;

    void SetColor( const PlayerColor color )
    {
        _color = color;
    }

    void SetRace( const int race )
    {
        _race = race;
    }

    void SetControl( const int control )
    {
        _control = control;
    }

    void SetPlay( const bool f );

    void SetFriends( const PlayerColorsSet friendsColors )
    {
        _friendsColors = friendsColors;
    }

    void SetName( std::string newName );

    int GetControl() const override;

    PlayerColor GetColor() const
    {
        return _color;
    }

    int GetRace() const
    {
        return _race;
    }

    PlayerColorsSet GetFriends() const
    {
        return _friendsColors;
    }

    std::string GetDefaultName() const
    {
        return Color::String( _color );
    }

    std::string GetName() const;

    bool isDefaultName() const
    {
        return _name.empty();
    }

    std::string GetPersonalityString() const;

    Focus & GetFocus()
    {
        return _focus;
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

    std::string _name;
    Focus _focus;
    AI::Personality _aiPersonality{ AI::Personality::NONE };
    int32_t _control{ CONTROL_NONE };
    int32_t _race{ Race::NONE };
    PlayerColor _color;
    PlayerColorsSet _friendsColors;
    HandicapStatus _handicapStatus{ HandicapStatus::NONE };

#if defined( WITH_DEBUG )
    // These members should not be saved anywhere

    // Actual value of whether a given human player is controlled by AI
    bool _isAIAutoControlMode{ false };
    // Planned value of whether a given human player is controlled by AI (will become actual upon committing it)
    bool _isAIAutoControlModePlanned{ false };
#endif
};

class Players : public std::vector<Player *>
{
public:
    Players()
    {
        reserve( maxNumOfPlayers );
    }

    Players( const Players & ) = delete;

    ~Players()
    {
        clear();
    }

    Players & operator=( const Players & ) = delete;

    void Init( const PlayerColorsSet colors );
    void Init( const Maps::FileInfo & fi );
    void clear();

    void SetStartGame();
    PlayerColorsSet GetColors( const int control = 0xFF, const bool strong = false ) const;
    PlayerColorsSet GetActualColors() const;
    std::string String() const;

    const std::vector<Player *> & getVector() const
    {
        return *this;
    }

    Player * GetCurrent();
    const Player * GetCurrent() const;

    static void Set( const PlayerColor color, Player * player );
    static Player * Get( const PlayerColor color );
    static int GetPlayerControl( const PlayerColor color );
    static int GetPlayerRace( const PlayerColor color );
    static PlayerColorsSet GetPlayerFriends( const PlayerColor color );
    static bool GetPlayerInGame( const PlayerColor color );
    static std::vector<PlayerColor> getInPlayOpponents( const PlayerColor color );
    static bool isFriends( const PlayerColor playerColor, const PlayerColorsSet colors );
    static void SetPlayerRace( const PlayerColor color, const int race );
    static void SetPlayerControl( const PlayerColor color, const int control );
    static void SetPlayerInGame( const PlayerColor color, const bool isPlay );
    static PlayerColorsSet HumanColors();
    // Return current player friends colors, if player does not exist he has no friends (returns 0).
    static PlayerColorsSet FriendColors();

    PlayerColor getCurrentColor() const
    {
        return _currentColor;
    }

    // The color should belong to one player or be NONE (neutral player).
    void setCurrentColor( const PlayerColor color );

private:
    PlayerColor _currentColor{ PlayerColor::NONE };
};

OStreamBase & operator<<( OStreamBase & stream, const Players & players );
IStreamBase & operator>>( IStreamBase & stream, Players & players );
