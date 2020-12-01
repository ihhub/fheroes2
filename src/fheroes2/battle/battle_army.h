/***************************************************************************
 *   Copyright (C) 2012 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#ifndef H2BATTLE_ARMY_H
#define H2BATTLE_ARMY_H

#include "army.h"
#include "bitmodes.h"

namespace Battle
{
    class Unit;

    class Units : public std::vector<Unit *>
    {
    public:
        Units();
        Units( const Units &, bool filter = false );
        Units( const Units &, const Units & );
        virtual ~Units();

        Units & operator=( const Units & );

        Unit * FindMode( u32 );
        Unit * FindUID( u32 );

        void SortSlowest( bool );
        void SortFastest( bool );
        void SortStrongest( void );
        void SortWeakest( void );
        void SortArchers();
    };

    class Force : public Units, public BitModes
    {
    public:
        Force( Army &, bool );
        ~Force();

        HeroBase * GetCommander( void );
        const HeroBase * GetCommander( void ) const;

        bool isValid( void ) const;
        bool HasMonster( const Monster & ) const;
        u32 GetDeadHitPoints( void ) const;
        u32 GetDeadCounts( void ) const;
        int GetColor( void ) const;
        int GetControl( void ) const;
        uint32_t GetSurrenderCost( void ) const;
        Troops GetKilledTroops( void ) const;
        bool animateIdleUnits();
        void resetIdleAnimation();

        void NewTurn( void );
        void SyncArmyCount( bool checkResurrected );

        static Unit * GetCurrentUnit( const Force &, const Force &, Unit * last, Units * all, bool part1 );
        static Unit * GetCurrentUnit( const Force &, const Force &, Unit * last, bool part1 );
        static void UpdateOrderUnits( const Force &, const Force &, Units & );

    private:
        Army & army;
        std::vector<u32> uids;
    };

    StreamBase & operator<<( StreamBase &, const Force & );
    StreamBase & operator>>( StreamBase &, Force & );
}

#endif
