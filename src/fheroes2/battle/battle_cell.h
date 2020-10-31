/***************************************************************************
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#ifndef H2BATTLE_CELL_H
#define H2BATTLE_CELL_H

#include "gamedefs.h"

#define CELLW 44
#define CELLH 52
#define CELLH_VER_SIDE 32

namespace Battle
{
    class Unit;

    enum direction_t
    {
        UNKNOWN = 0x00,
        TOP_LEFT = 0x01,
        TOP_RIGHT = 0x02,
        RIGHT = 0x04,
        BOTTOM_RIGHT = 0x08,
        BOTTOM_LEFT = 0x10,
        LEFT = 0x20,
        CENTER = 0x40,
        RIGHT_SIDE = TOP_RIGHT | RIGHT | BOTTOM_RIGHT,
        LEFT_SIDE = TOP_LEFT | LEFT | BOTTOM_LEFT,
        AROUND = RIGHT_SIDE | LEFT_SIDE
    };

    class Cell
    {
    public:
        Cell();
        Cell( s32 );

        void ResetQuality( void );
        void ResetDirection( void );

        void SetObject( int );
        void SetDirection( int );
        void SetQuality( u32 );

        void SetArea( const Rect & );

        bool isPassable4( const Unit &, const Cell & ) const;
        bool isPassable3( const Unit &, bool check_reflect ) const;
        bool isPassable1( bool check_troop ) const;
        bool isPositionIncludePoint( const Point & ) const;

        s32 GetIndex( void ) const;
        const Rect & GetPos( void ) const;
        int GetObject( void ) const;
        int GetDirection( void ) const;
        s32 GetQuality( void ) const;
        direction_t GetTriangleDirection( const Point & ) const;

        const Unit * GetUnit( void ) const;
        Unit * GetUnit( void );
        void SetUnit( Unit * );

    private:
        friend StreamBase & operator<<( StreamBase &, const Cell & );
        friend StreamBase & operator>>( StreamBase &, Cell & );

        s32 index;
        Rect pos;
        int object;
        int direction;
        s32 quality;
        Unit * troop;
        Point coord[7];
    };

    StreamBase & operator<<( StreamBase &, const Cell & );
    StreamBase & operator>>( StreamBase &, Cell & );

    class Position : protected std::pair<Cell *, Cell *>
    {
    public:
        Position()
            : std::pair<Cell *, Cell *>( NULL, NULL )
        {}

        void Set( s32 head, bool wide, bool reflect );
        void Swap( void );
        bool isReflect( void ) const;
        bool isValid( void ) const;
        bool contains( int cellIndex ) const;

        static Position GetCorrect( const Unit &, s32 );

        Rect GetRect( void ) const;
        Cell * GetHead( void );
        const Cell * GetHead( void ) const;
        Cell * GetTail( void );
        const Cell * GetTail( void ) const;
    };
}

#endif
