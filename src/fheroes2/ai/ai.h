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

class Funds;
class Castle;
class HeroBase;
class Heroes;
class Kingdom;
namespace Maps
{
    class Tiles;
}
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
        NORMAL
    };
    enum AI_PERSONALITY
    {
        NONE,
        WARRIOR,
        BUILDER,
        EXPLORER
    };

    enum modes_t
    {
        HERO_SKIP_TURN = 0x02000000,
        HERO_WAITING = 0x04000000,
        HERO_MOVED = 0x08000000,
        HERO_SCOUT = 0x10000000,
        HERO_HUNTER = 0x20000000,
        HERO_COURIER = 0x40000000,
        HERO_CHAMPION = 0x80000000
    };

    const double ARMY_STRENGTH_ADVANTAGE_SMALL = 1.3;
    const double ARMY_STRENGTH_ADVANTAGE_MEDUIM = 1.5;
    const double ARMY_STRENGTH_ADVANTAGE_LARGE = 1.8;

    class Base
    {
    public:
        virtual void KingdomTurn( Kingdom & kingdom );
        virtual void CastleTurn( Castle & castle, bool defensive = false );
        virtual void BattleTurn( Battle::Arena & arena, const Battle::Unit & unit, Battle::Actions & actions );
        virtual void HeroTurn( Heroes & hero );

        virtual void revealFog( const Maps::Tiles & tile );

        virtual void HeroesAdd( const Heroes & hero );
        virtual void HeroesRemove( const Heroes & hero );
        virtual void HeroesPreBattle( HeroBase & hero );
        virtual void HeroesAfterBattle( HeroBase & hero );
        virtual void HeroesPostLoad( Heroes & hero );
        virtual bool HeroesCanMove( const Heroes & hero );
        virtual bool HeroesGetTask( Heroes & hero );
        virtual void HeroesActionComplete( Heroes & hero );
        virtual void HeroesActionNewPosition( Heroes & hero );
        virtual void HeroesClearTask( const Heroes & hero );
        virtual void HeroesLevelUp( Heroes & hero );
        virtual bool HeroesSkipFog();
        virtual std::string HeroesString( const Heroes & hero );

        virtual void CastleAdd( const Castle & castle );
        virtual void CastleRemove( const Castle & castle );
        virtual void CastlePreBattle( Castle & castle );
        virtual void CastleAfterBattle( Castle & castle, bool attackerWins );

        virtual const char * Type() const;
        virtual const char * License() const;
        virtual int GetPersonality() const;
        virtual std::string GetPersonalityString() const;

        virtual void Reset();
        virtual void resetPathfinder() = 0;

        virtual ~Base() {}

    protected:
        int _personality = NONE;

        Base() {}
    };

    Base & Get( AI_TYPE type = NORMAL );

    // functionality in ai_hero_action.cpp
    void HeroesAction( Heroes & hero, s32 dst_index );
    bool HeroesValidObject( const Heroes & hero, s32 index );
    void HeroesMove( Heroes & hero );

    // functionality in ai_common.cpp
    bool BuildIfAvailable( Castle & castle, int building );
    bool BuildIfEnoughResources( Castle & castle, int building, uint32_t minimumMultiplicator );
    uint32_t GetResourceMultiplier( const Castle & castle, uint32_t min, uint32_t max );
    void ReinforceHeroInCastle( Heroes & hero, Castle & castle, const Funds & budget );
}

#endif
