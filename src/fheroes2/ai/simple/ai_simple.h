/********************************************************************************
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>               *
 *   All rights reserved.                                                       *
 *                                                                              *
 *   Part of the Free Heroes2 Engine:                                           *
 *   http://sourceforge.net/projects/fheroes2                                   *
 *                                                                              *
 *   Redistribution and use in source and binary forms, with or without         *
 *   modification, are permitted provided that the following conditions         *
 *   are met:                                                                   *
 *   - Redistributions may not be sold, nor may they be used in a               *
 *     commercial product or activity.                                          *
 *   - Redistributions of source code and/or in binary form must reproduce      *
 *     the above copyright notice, this list of conditions and the              *
 *     following disclaimer in the documentation and/or other materials         *
 *     provided with the distribution.                                          *
 *                                                                              *
 * THIS SOFTWARE IS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,   *
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS    *
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT     *
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,        *
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, *
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;  *
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,     *
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE         *
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,            *
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                           *
 *******************************************************************************/

#ifndef H2AI_SIMPLE_H
#define H2AI_SIMPLE_H

#include <list>
#include <map>
#include <vector>

#include "ai.h"
#include "pairs.h"

namespace AI
{
    struct Queue : public std::list<IndexDistance>
    {
        bool isPresent( s32 ) const;
    };

    struct IndexObjectMap : public std::map<s32, int>
    {
        void DumpObjects( const IndexDistance & id );
    };

    struct AIKingdom
    {
        AIKingdom()
            : capital( NULL ){};
        void Reset( void );

        Castle * capital;
        IndexObjectMap scans;
    };

    struct AIHero
    {
        AIHero();
        void ClearTasks();
        void Reset();

        Queue sheduled_visit;
        s32 primary_target;
        u32 fix_loop;
    };

    class Simple : public Base
    {
    public:
        Simple();

        // Overwrites for Base AI
        void KingdomTurn( Kingdom & );
        void CastleTurn( Castle & );
        void BattleTurn( Battle::Arena &, const Battle::Unit &, Battle::Actions & );
        void HeroesTurn( Heroes & );

        void HeroesPreBattle( HeroBase & );
        void HeroesAfterBattle( HeroBase & );
        void HeroesPostLoad( Heroes & );
        bool HeroesGetTask( Heroes & );
        void HeroesActionNewPosition( Heroes & );
        void HeroesClearTask( const Heroes & );
        void HeroesLevelUp( Heroes & );
        bool HeroesSkipFog( void );
        std::string HeroesString( const Heroes & );

        void CastleRemove( const Castle & );
        void CastlePreBattle( Castle & );

        const char * Type( void ) const;
        const char * License( void ) const;

        void Reset();
        AIKingdom & GetKingdom( int color );
        AIHero & GetHero( const Heroes & );

    private:
        std::vector<AIKingdom> _kingdoms;
        std::vector<AIHero> _heroes;

        // Additional methods called only internally
        bool BattleMagicTurn( Battle::Arena &, const Battle::Unit &, Battle::Actions &, const Battle::Unit * );

        void HeroesAddedRescueTask( Heroes & hero );
        void HeroesAddedTask( Heroes & hero );
        void HeroesTurnEnd( Heroes * hero );
        void HeroesSetHunterWithTarget( Heroes * hero, s32 dst );
        void HeroesCaptureNearestTown( Heroes * hero );
        bool HeroesScheduledVisit( const Kingdom & kingdom, s32 index );

        bool IsPriorityAndNotVisitAndNotPresent( const std::pair<s32, int> & indexObj, const Heroes * hero );
    };
}
#endif
