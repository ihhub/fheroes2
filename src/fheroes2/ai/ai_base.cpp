/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2020                                                    *
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

#include "agg.h"
#include "battle.h"
#include "battle_arena.h"
#include "battle_command.h"
#include "battle_troop.h"
#include "castle.h"
#include "dialog.h"
#include "empty/ai_empty.h"
#include "game_interface.h"
#include "heroes.h"
#include "kingdom.h"
#include "mus.h"
#include "normal/ai_normal.h"
#include "settings.h"
#include "simple/ai_simple.h"

namespace AI
{
    // AI Selector here
    Base & Get( AI_TYPE type )
    {
        static AI::Empty empty;
        static AI::Simple simple;
        static AI::Normal normal;

        switch ( type ) {
        case AI::SIMPLE:
            return simple;
        case AI::NORMAL:
            return normal;
        }
        return empty;
    }

    const char * Base::Type() const
    {
        return "base";
    }

    const char * Base::License() const
    {
        return "GPL-2.0";
    }

    void Base::Reset() {}

    void Base::CastlePreBattle( Castle & ) {}

    void Base::CastleAfterBattle( Castle &, bool attacker_wins ) {}

    void Base::CastleTurn( Castle & ) {}

    void Base::CastleAdd( const Castle & ) {}

    void Base::CastleRemove( const Castle & ) {}

    void Base::HeroesAdd( const Heroes & ) {}

    void Base::HeroesRemove( const Heroes & ) {}

    void Base::HeroesPreBattle( HeroBase & ) {}

    void Base::HeroesAfterBattle( HeroBase & ) {}

    void Base::HeroesActionNewPosition( Heroes & ) {}

    void Base::HeroesClearTask( const Heroes & ) {}

    std::string Base::HeroesString( const Heroes & )
    {
        return "";
    }

    void Base::HeroesActionComplete( Heroes &, s32 ) {}

    void Base::HeroesLevelUp( Heroes & ) {}

    void Base::HeroesPostLoad( Heroes & ) {}

    bool Base::HeroesSkipFog()
    {
        return false;
    }

    bool Base::HeroesGetTask( Heroes & hero )
    {
        // stop hero
        hero.GetPath().Reset();
        return false;
    }

    bool Base::HeroesCanMove( const Heroes & hero )
    {
        return hero.MayStillMove() && !hero.Modes( HEROES_MOVED );
    }

    void Base::HeroesTurn( Heroes & hero )
    {
        Interface::StatusWindow & status = Interface::Basic::Get().GetStatusWindow();

        hero.ResetModes( HEROES_MOVED );

        while ( Base::HeroesCanMove( hero ) ) {
            // turn indicator
            status.RedrawTurnProgress( 3 );
            status.RedrawTurnProgress( 4 );

            // get task for heroes
            Base::HeroesGetTask( hero );

            // turn indicator
            status.RedrawTurnProgress( 5 );
            status.RedrawTurnProgress( 6 );

            // heroes AI turn
            AI::HeroesMove( hero );
            hero.SetModes( HEROES_MOVED );

            // turn indicator
            status.RedrawTurnProgress( 7 );
            status.RedrawTurnProgress( 8 );
        }

        DEBUG( DBG_AI, DBG_TRACE, hero.GetName() << ", end" );
    }

    void Base::KingdomTurn( Kingdom & kingdom )
    {
        KingdomHeroes & heroes = kingdom.GetHeroes();
        KingdomCastles & castles = kingdom.GetCastles();

        const int color = kingdom.GetColor();

        if ( kingdom.isLoss() || color == Color::NONE ) {
            kingdom.LossPostActions();
            return;
        }

        if ( !Settings::Get().MusicMIDI() )
            AGG::PlayMusic( MUS::COMPUTER_TURN );

        Interface::StatusWindow & status = Interface::Basic::Get().GetStatusWindow();

        // indicator
        status.RedrawTurnProgress( 0 );

        status.RedrawTurnProgress( 1 );

        // castles AI turn
        for ( KingdomCastles::iterator it = castles.begin(); it != castles.end(); ++it )
            if ( *it )
                CastleTurn( **it );

        status.RedrawTurnProgress( 3 );

        // heroes turns
        for ( KingdomHeroes::iterator it = heroes.begin(); it != heroes.end(); ++it )
            if ( *it )
                HeroesTurn( **it );

        status.RedrawTurnProgress( 6 );
        status.RedrawTurnProgress( 7 );
        status.RedrawTurnProgress( 8 );
        status.RedrawTurnProgress( 9 );

        DEBUG( DBG_AI, DBG_INFO, Color::String( color ) << " moved" );
    }

    void Base::BattleTurn( Battle::Arena &, const Battle::Unit & b, Battle::Actions & a )
    {
        // end action
        a.push_back( Battle::Command( Battle::MSG_BATTLE_END_TURN, b.GetUID() ) );
    }
}
