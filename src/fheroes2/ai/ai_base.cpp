/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2022                                             *
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
#include "ai.h"
#include "battle_arena.h"
#include "battle_command.h"
#include "battle_troop.h"
#include "game_interface.h"
#include "heroes.h"
#include "kingdom.h"
#include "logging.h"
#include "mus.h"
#include "serialize.h"
#include "translations.h"

namespace AI
{
    const char * Base::Type() const
    {
        return "base";
    }

    int Base::GetPersonality() const
    {
        return _personality;
    }

    std::string Base::GetPersonalityString() const
    {
        switch ( _personality ) {
        case WARRIOR:
            return _( "Warrior" );
        case BUILDER:
            return _( "Builder" );
        case EXPLORER:
            return _( "Explorer" );
        default:
            break;
        }

        return Type();
    }

    void Base::Reset()
    {
        // Do nothing.
    }

    void Base::CastlePreBattle( Castle & )
    {
        // Do nothing.
    }

    void Base::CastleAfterBattle( Castle &, bool )
    {
        // Do nothing.
    }

    void Base::CastleTurn( Castle &, bool )
    {
        // Do nothing.
    }

    void Base::CastleAdd( const Castle & )
    {
        // Do nothing.
    }

    void Base::CastleRemove( const Castle & )
    {
        // Do nothing.
    }

    void Base::HeroesAdd( const Heroes & )
    {
        // Do nothing.
    }

    void Base::HeroesRemove( const Heroes & )
    {
        // Do nothing.
    }

    void Base::HeroesPreBattle( HeroBase &, bool )
    {
        // Do nothing.
    }

    void Base::HeroesAfterBattle( HeroBase &, bool )
    {
        // Do nothing.
    }

    void Base::HeroesActionNewPosition( Heroes & )
    {
        // Do nothing.
    }

    void Base::HeroesClearTask( const Heroes & )
    {
        // Do nothing.
    }

    void Base::revealFog( const Maps::Tiles & )
    {
        // Do nothing.
    }

    std::string Base::HeroesString( const Heroes & )
    {
        return std::string();
    }

    void Base::HeroesActionComplete( Heroes & )
    {
        // Do nothing.
    }

    void Base::HeroesLevelUp( Heroes & )
    {
        // Do nothing.
    }

    void Base::HeroesPostLoad( Heroes & )
    {
        // Do nothing.
    }

    bool Base::HeroesGetTask( Heroes & hero )
    {
        // stop hero
        hero.GetPath().Reset();
        return false;
    }

    bool Base::HeroesCanMove( const Heroes & hero )
    {
        return hero.MayStillMove( false, false ) && !hero.Modes( Heroes::MOVED );
    }

    void Base::HeroTurn( Heroes & hero )
    {
        Interface::StatusWindow & status = Interface::Basic::Get().GetStatusWindow();

        hero.ResetModes( Heroes::MOVED );

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
            hero.SetModes( Heroes::MOVED );

            // turn indicator
            status.RedrawTurnProgress( 7 );
            status.RedrawTurnProgress( 8 );
        }

        DEBUG_LOG( DBG_AI, DBG_TRACE, hero.GetName() << ", end" )
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

        AGG::PlayMusic( MUS::COMPUTER_TURN, true, true );

        Interface::StatusWindow & status = Interface::Basic::Get().GetStatusWindow();

        // indicator
        status.RedrawTurnProgress( 0 );

        status.RedrawTurnProgress( 1 );

        // castles AI turn
        for ( KingdomCastles::iterator it = castles.begin(); it != castles.end(); ++it )
            if ( *it )
                CastleTurn( **it, false );

        status.RedrawTurnProgress( 3 );

        // heroes turns
        for ( KingdomHeroes::iterator it = heroes.begin(); it != heroes.end(); ++it )
            if ( *it )
                HeroTurn( **it );

        status.RedrawTurnProgress( 6 );
        status.RedrawTurnProgress( 7 );
        status.RedrawTurnProgress( 8 );
        status.RedrawTurnProgress( 9 );

        DEBUG_LOG( DBG_AI, DBG_INFO, Color::String( color ) << " moved" )
    }

    void Base::BattleTurn( Battle::Arena &, const Battle::Unit & currentUnit, Battle::Actions & actions )
    {
        // end action
        actions.emplace_back( Battle::CommandType::MSG_BATTLE_END_TURN, currentUnit.GetUID() );
    }

    StreamBase & operator<<( StreamBase & msg, const AI::Base & instance )
    {
        return msg << instance._personality;
    }

    StreamBase & operator>>( StreamBase & msg, AI::Base & instance )
    {
        return msg >> instance._personality;
    }
}
