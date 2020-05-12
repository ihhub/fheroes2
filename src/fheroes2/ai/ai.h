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

#ifndef H2AI_H
#define H2AI_H

#include "gamedefs.h"

class Castle;
class HeroBase;
class Heroes;
class Kingdom;
namespace Battle
{
    class Arena;
    class Unit;
    class Actions;
}

namespace AI
{
    enum AI_TYPE
    {
        EMPTY,
        SIMPLE,
        NORMAL
    };

    enum modes_t
    {
        HEROES_MOVED = 0x08000000,
        HEROES_SCOUTER = 0x10000000,
        HEROES_HUNTER = 0x20000000,
        HEROES_WAITING = 0x40000000,
        HEROES_STUPID = 0x80000000
    };

    class Base
    {
    public:
        virtual void KingdomTurn( Kingdom & );
        virtual void CastleTurn( Castle & );
        virtual void BattleTurn( Battle::Arena &, const Battle::Unit &, Battle::Actions & );
        virtual void HeroesTurn( Heroes & );

        virtual void HeroesAdd( const Heroes & );
        virtual void HeroesRemove( const Heroes & );
        virtual void HeroesPreBattle( HeroBase & );
        virtual void HeroesAfterBattle( HeroBase & );
        virtual void HeroesPostLoad( Heroes & );
        virtual bool HeroesCanMove( const Heroes & hero );
        virtual bool HeroesGetTask( Heroes & );
        virtual void HeroesActionComplete( Heroes &, s32 );
        virtual void HeroesActionNewPosition( Heroes & );
        virtual void HeroesClearTask( const Heroes & );
        virtual void HeroesLevelUp( Heroes & );
        virtual bool HeroesSkipFog( void );
        virtual std::string HeroesString( const Heroes & );

        virtual void CastleAdd( const Castle & );
        virtual void CastleRemove( const Castle & );
        virtual void CastlePreBattle( Castle & );
        virtual void CastleAfterBattle( Castle &, bool attacker_wins );

        virtual const char * Type( void ) const;
        virtual const char * License( void ) const;
        virtual void Reset();
    };

    Base & Get( AI_TYPE type = SIMPLE );

    void HeroesAction( Heroes & hero, s32 dst_index );
    bool HeroesValidObject( const Heroes & hero, s32 index );
    void HeroesMove( Heroes & hero );
}

#endif
