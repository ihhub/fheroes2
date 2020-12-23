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

#ifndef H2PLAYERS_H
#define H2PLAYERS_H

#include <string>
#include <vector>

#include "bitmodes.h"
#include "color.h"

namespace Maps
{
    class FileInfo;
}

class Castle;
class Heroes;

// control_t
enum
{
    CONTROL_NONE = 0,
    CONTROL_HUMAN = 1,
    CONTROL_AI = 4,
    CONTROL_REMOTE = 2 /*, CONTROL_LOCAL = CONTROL_AI | CONTROL_HUMAN */
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
        : std::pair<int, void *>( FOCUS_UNSEL, NULL )
    {}

    bool isValid( void ) const
    {
        return first != FOCUS_UNSEL && second;
    }

    void Reset( void )
    {
        first = FOCUS_UNSEL;
        second = NULL;
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

    Castle * GetCastle( void )
    {
        return first == FOCUS_CASTLE && second ? reinterpret_cast<Castle *>( second ) : NULL;
    }
    Heroes * GetHeroes( void )
    {
        return first == FOCUS_HEROES && second ? reinterpret_cast<Heroes *>( second ) : NULL;
    }
};

struct Control
{
    virtual int GetControl( void ) const = 0;
    virtual ~Control() {}
    bool isControlAI( void ) const;
    bool isControlHuman( void ) const;
    bool isControlRemote( void ) const;
    bool isControlLocal( void ) const;
};

class Player : public BitModes, public Control
{
public:
    Player( int col = Color::NONE );
    virtual ~Player() {}

    bool isID( u32 ) const;
    bool isColor( int ) const;
    bool isName( const std::string & ) const;
    bool isPlay( void ) const;

    void SetColor( int );
    void SetRace( int );
    void SetControl( int );
    void SetPlay( bool );
    void SetFriends( int );
    void SetName( const std::string & );

    int GetControl( void ) const;
    int GetColor( void ) const;
    int GetRace( void ) const;
    int GetFriends( void ) const;
    int GetID( void ) const;

    const std::string & GetName( void ) const;
    Focus & GetFocus( void );
    const Focus & GetFocus( void ) const;

protected:
    friend StreamBase & operator<<( StreamBase &, const Player & );
    friend StreamBase & operator>>( StreamBase &, Player & );

    int control;
    int color;
    int race;
    int friends;
    std::string name;
    u32 id;
    Focus focus;
};

StreamBase & operator<<( StreamBase &, const Player & );
StreamBase & operator>>( StreamBase &, Player & );

class Players : public std::vector<Player *>
{
public:
    Players();
    ~Players();

    void Init( int colors );
    void Init( const Maps::FileInfo & );
    void clear( void );

    void SetStartGame( void );
    int GetColors( int control = 0xFF, bool strong = false ) const;
    int GetActualColors( void ) const;
    std::string String( void ) const;

    Player * GetCurrent( void );
    const Player * GetCurrent( void ) const;

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
    static int HumanColors( void );
    static int FriendColors( void );

    int current_color;
};

StreamBase & operator<<( StreamBase &, const Players & );
StreamBase & operator>>( StreamBase &, Players & );

#endif
