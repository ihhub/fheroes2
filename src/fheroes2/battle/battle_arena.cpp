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

#include <algorithm>
#include <cassert>

#include "agg.h"
#include "ai.h"
#include "army.h"
#include "army_troop.h"
#include "audio_mixer.h"
#include "audio_music.h"
#include "battle_arena.h"
#include "battle_army.h"
#include "battle_bridge.h"
#include "battle_catapult.h"
#include "battle_cell.h"
#include "battle_command.h"
#include "battle_interface.h"
#include "battle_tower.h"
#include "battle_troop.h"
#include "castle.h"
#include "ground.h"
#include "icn.h"
#include "logging.h"
#include "mus.h"
#include "race.h"
#include "settings.h"
#include "tools.h"
#include "world.h"

namespace Battle
{
    Arena * arena = nullptr;
}

int GetCovr( int ground, std::mt19937 & gen )
{
    std::vector<int> covrs;

    switch ( ground ) {
    case Maps::Ground::SNOW:
        covrs.push_back( ICN::COVR0007 );
        covrs.push_back( ICN::COVR0008 );
        covrs.push_back( ICN::COVR0009 );
        covrs.push_back( ICN::COVR0010 );
        covrs.push_back( ICN::COVR0011 );
        covrs.push_back( ICN::COVR0012 );
        break;

    case Maps::Ground::WASTELAND:
        covrs.push_back( ICN::COVR0019 );
        covrs.push_back( ICN::COVR0020 );
        covrs.push_back( ICN::COVR0021 );
        covrs.push_back( ICN::COVR0022 );
        covrs.push_back( ICN::COVR0023 );
        covrs.push_back( ICN::COVR0024 );
        break;

    case Maps::Ground::DIRT:
        covrs.push_back( ICN::COVR0013 );
        covrs.push_back( ICN::COVR0014 );
        covrs.push_back( ICN::COVR0015 );
        covrs.push_back( ICN::COVR0016 );
        covrs.push_back( ICN::COVR0017 );
        covrs.push_back( ICN::COVR0018 );
        break;

    case Maps::Ground::GRASS:
        covrs.push_back( ICN::COVR0001 );
        covrs.push_back( ICN::COVR0002 );
        covrs.push_back( ICN::COVR0003 );
        covrs.push_back( ICN::COVR0004 );
        covrs.push_back( ICN::COVR0005 );
        covrs.push_back( ICN::COVR0006 );
        break;

    default:
        break;
    }

    return covrs.empty() ? ICN::UNKNOWN : Rand::GetWithGen( covrs, gen );
}

bool Battle::TargetInfo::operator==( const TargetInfo & ta ) const
{
    return defender == ta.defender;
}

Battle::Arena * Battle::GetArena( void )
{
    return arena;
}

const Castle * Battle::Arena::GetCastle( void )
{
    return arena->castle;
}

Battle::Bridge * Battle::Arena::GetBridge( void )
{
    return arena->bridge;
}

Battle::Board * Battle::Arena::GetBoard( void )
{
    return &arena->board;
}

Battle::Graveyard * Battle::Arena::GetGraveyard( void )
{
    return &arena->graveyard;
}

Battle::Interface * Battle::Arena::GetInterface( void )
{
    return arena->interface;
}

Battle::Tower * Battle::Arena::GetTower( int type )
{
    switch ( type ) {
    case TWR_LEFT:
        return arena->towers[0];
    case TWR_CENTER:
        return arena->towers[1];
    case TWR_RIGHT:
        return arena->towers[2];
    default:
        break;
    }
    return nullptr;
}

Battle::Arena::Arena( Army & a1, Army & a2, s32 index, bool local )
    : army1( nullptr )
    , army2( nullptr )
    , armies_order( nullptr )
    , current_color( Color::NONE )
    , preferredColor( -1 ) // be aware of unknown color
    , castle( nullptr )
    , catapult( nullptr )
    , bridge( nullptr )
    , interface( nullptr )
    , icn_covr( ICN::UNKNOWN )
    , current_turn( 0 )
    , auto_battle( 0 )
    , end_turn( false )
{
    const Settings & conf = Settings::Get();
    usage_spells.reserve( 20 );

    assert( arena == nullptr );
    arena = this;
    army1 = new Force( a1, false );
    army2 = new Force( a2, true );

    // init castle (interface ahead)
    castle = world.GetCastle( Maps::GetPoint( index ) );

    if ( castle ) {
        CastleHeroes heroes = world.GetHeroes( *castle );

        // skip if present guard and guest
        if ( heroes.FullHouse() )
            castle = nullptr;

        // skip for town
        if ( castle && !castle->isCastle() )
            castle = nullptr;
    }

    // init interface
    if ( local ) {
        interface = new Interface( *this, index );
        board.SetArea( interface->GetArea() );

        if ( conf.Sound() )
            AGG::PlaySound( M82::PREBATTL );

        armies_order = new Units();
        armies_order->reserve( 25 );
        interface->SetArmiesOrder( armies_order );
    }
    else {
        // no interface - force auto battle mode for human player
        if ( a1.isControlHuman() ) {
            auto_battle |= a1.GetColor();
        }
        if ( a2.isControlHuman() ) {
            auto_battle |= a2.GetColor();
        }
    }

    towers[0] = nullptr;
    towers[1] = nullptr;
    towers[2] = nullptr;

    if ( castle ) {
        // init
        towers[0] = castle->isBuild( BUILD_LEFTTURRET ) ? new Tower( *castle, TWR_LEFT ) : nullptr;
        towers[1] = new Tower( *castle, TWR_CENTER );
        towers[2] = castle->isBuild( BUILD_RIGHTTURRET ) ? new Tower( *castle, TWR_RIGHT ) : nullptr;
        const bool fortification = ( Race::KNGT == castle->GetRace() ) && castle->isBuild( BUILD_SPEC );
        catapult = army1->GetCommander() ? new Catapult( *army1->GetCommander() ) : nullptr;
        bridge = new Bridge();

        // catapult cell
        board[77].SetObject( 1 );

        // wall (3,2,1,0)
        board[8].SetObject( fortification ? 3 : 2 );
        board[29].SetObject( fortification ? 3 : 2 );
        board[73].SetObject( fortification ? 3 : 2 );
        board[96].SetObject( fortification ? 3 : 2 );

        // tower
        board[40].SetObject( 2 );
        board[62].SetObject( 2 );

        // archers tower
        board[19].SetObject( 2 );
        board[85].SetObject( 2 );

        // bridge
        board[50].SetObject( 1 );
    }
    else
    // set obstacles
    {
        std::mt19937 seededGen( world.GetMapSeed() + static_cast<uint32_t>( index ) );

        icn_covr = Rand::GetWithGen( 0, 99, seededGen ) < 40 ? GetCovr( world.GetTiles( index ).GetGround(), seededGen ) : ICN::UNKNOWN;

        if ( icn_covr != ICN::UNKNOWN )
            board.SetCovrObjects( icn_covr );
        else
            board.SetCobjObjects( world.GetTiles( index ), seededGen );
    }

    if ( interface ) {
        fheroes2::Display & display = fheroes2::Display::instance();

        if ( conf.ExtGameUseFade() )
            fheroes2::FadeDisplay();

        interface->fullRedraw();
        display.render();

        // pause for play M82::PREBATTL
        if ( conf.Sound() )
            while ( LocalEvent::Get().HandleEvents() && Mixer::isPlaying( -1 ) )
                ;
    }
}

Battle::Arena::~Arena()
{
    delete army1;
    delete army2;
    delete towers[0];
    delete towers[1];
    delete towers[2];
    delete catapult;
    delete interface;
    delete armies_order;
    delete bridge;

    assert( arena == this );
    arena = nullptr;
}

void Battle::Arena::TurnTroop( Unit * troop, const Units & orderHistory )
{
    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, troop->String( true ) );

    if ( troop->isAffectedByMorale() ) {
        troop->SetRandomMorale();
    }

    end_turn = false;

    while ( !end_turn ) {
        Actions actions;

        if ( !troop->isValid() ) { // looks like the unit died
            end_turn = true;
        }
        else if ( troop->Modes( MORALE_BAD ) && !troop->Modes( TR_SKIPMOVE ) ) {
            // bad morale, happens only if the unit wasn't waiting for a turn
            actions.push_back( Command( MSG_BATTLE_MORALE, troop->GetUID(), false ) );
            end_turn = true;
        }
        else {
            // re-calculate possible paths in case unit moved or it's a new turn
            _pathfinder.calculate( *troop );

            // get task from player
            if ( troop->isControlRemote() )
                RemoteTurn( *troop, actions );
            else {
                if ( ( troop->GetCurrentControl() & CONTROL_AI ) || ( troop->GetCurrentColor() & auto_battle ) ) {
                    AI::Get().BattleTurn( *this, *troop, actions );
                }
                else {
                    HumanTurn( *troop, actions );
                }
            }
        }

        const bool troopHasAlreadySkippedMove = troop->Modes( TR_SKIPMOVE );

        // apply task
        while ( actions.size() ) {
            // apply action
            ApplyAction( actions.front() );
            actions.pop_front();

            if ( armies_order ) {
                // some spell could kill someone or affect the speed of some unit, update units order
                Force::UpdateOrderUnits( *army1, *army2, troop, preferredColor, orderHistory, *armies_order );
            }

            // check end battle
            if ( !BattleValid() ) {
                end_turn = true;
                break;
            }

            const bool isImmovable = troop->Modes( SP_BLIND | IS_PARALYZE_MAGIC );
            const bool troopSkipsMove = troopHasAlreadySkippedMove ? troop->Modes( TR_HARDSKIP ) : troop->Modes( TR_SKIPMOVE );

            // good morale
            if ( !end_turn && troop->isValid() && troop->Modes( TR_MOVED ) && troop->Modes( MORALE_GOOD ) && !isImmovable && !troopSkipsMove ) {
                actions.emplace_back( MSG_BATTLE_MORALE, troop->GetUID(), true );
            }
        }

        if ( troop->Modes( TR_MOVED ) || ( troop->Modes( TR_SKIPMOVE ) && !troopHasAlreadySkippedMove ) ) {
            end_turn = true;
        }

        board.Reset();

        if ( interface ) {
            fheroes2::delayforMs( 10 );
        }
    }
}

bool Battle::Arena::BattleValid( void ) const
{
    return army1->isValid() && army2->isValid() && 0 == result_game.army1 && 0 == result_game.army2;
}

void Battle::Arena::Turns( void )
{
    ++current_turn;

    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, current_turn );

    const Settings & conf = Settings::Get();

    if ( interface ) {
        interface->RedrawActionNewTurn();

        if ( conf.Music() && !Music::isPlaying() ) {
            AGG::PlayMusic( MUS::GetBattleRandom(), true, true );
        }
    }

    army1->NewTurn();
    army2->NewTurn();

    // order history on the current turn
    Units orderHistory;

    if ( armies_order ) {
        orderHistory.reserve( 25 );

        // build initial units order
        Force::UpdateOrderUnits( *army1, *army2, nullptr, preferredColor, orderHistory, *armies_order );
    }

    {
        bool tower_moved = false;
        bool catapult_moved = false;

        Unit * troop = nullptr;

        while ( BattleValid() && ( troop = Force::GetCurrentUnit( *army1, *army2, true, preferredColor ) ) != nullptr ) {
            current_color = troop->GetCurrentOrArmyColor();

            // switch preferred color for the next unit
            preferredColor = troop->GetArmyColor() == army1->GetColor() ? army2->GetColor() : army1->GetColor();

            if ( armies_order ) {
                // add unit to the order history
                orderHistory.push_back( troop );

                // update units order
                Force::UpdateOrderUnits( *army1, *army2, troop, preferredColor, orderHistory, *armies_order );
            }

            // first turn: castle and catapult action
            if ( castle ) {
                if ( !catapult_moved && troop->GetColor() == army1->GetColor() ) {
                    CatapultAction();
                    catapult_moved = true;
                }

                if ( !tower_moved && troop->GetColor() == army2->GetColor() ) {
                    if ( towers[1] && towers[1]->isValid() ) {
                        TowerAction( *towers[1] );

                        if ( armies_order ) {
                            // tower could kill someone, update units order
                            Force::UpdateOrderUnits( *army1, *army2, troop, preferredColor, orderHistory, *armies_order );
                        }
                    }
                    if ( towers[0] && towers[0]->isValid() ) {
                        TowerAction( *towers[0] );

                        if ( armies_order ) {
                            // tower could kill someone, update units order
                            Force::UpdateOrderUnits( *army1, *army2, troop, preferredColor, orderHistory, *armies_order );
                        }
                    }
                    if ( towers[2] && towers[2]->isValid() ) {
                        TowerAction( *towers[2] );

                        if ( armies_order ) {
                            // tower could kill someone, update units order
                            Force::UpdateOrderUnits( *army1, *army2, troop, preferredColor, orderHistory, *armies_order );
                        }
                    }
                    tower_moved = true;

                    // check dead last army from towers
                    if ( !BattleValid() )
                        break;
                }
            }

            // set bridge passable
            if ( bridge )
                bridge->SetPassable( *troop );

            // turn troop
            TurnTroop( troop, orderHistory );

            if ( armies_order ) {
                // if unit hasn't finished its turn yet, then remove it from the order history
                if ( troop->Modes( TR_SKIPMOVE ) && !troop->Modes( TR_MOVED ) ) {
                    orderHistory.pop_back();
                }
            }
        }
    }

    // can skip move ?
    if ( conf.ExtBattleSoftWait() ) {
        Unit * troop = nullptr;

        while ( BattleValid() && ( troop = Force::GetCurrentUnit( *army1, *army2, false, preferredColor ) ) != nullptr ) {
            current_color = troop->GetCurrentOrArmyColor();

            // switch preferred color for the next unit
            preferredColor = troop->GetArmyColor() == army1->GetColor() ? army2->GetColor() : army1->GetColor();

            if ( armies_order ) {
                // add unit to the order history
                orderHistory.push_back( troop );

                // update units order
                Force::UpdateOrderUnits( *army1, *army2, troop, preferredColor, orderHistory, *armies_order );
            }

            // set bridge passable
            if ( bridge )
                bridge->SetPassable( *troop );

            // turn troop
            TurnTroop( troop, orderHistory );
        }
    }

    // end turn: fix result
    if ( !army1->isValid() || ( result_game.army1 & ( RESULT_RETREAT | RESULT_SURRENDER ) ) ) {
        result_game.army1 |= RESULT_LOSS;
        if ( army2->isValid() )
            result_game.army2 = RESULT_WINS;
    }

    if ( !army2->isValid() || ( result_game.army2 & ( RESULT_RETREAT | RESULT_SURRENDER ) ) ) {
        result_game.army2 |= RESULT_LOSS;
        if ( army1->isValid() )
            result_game.army1 = RESULT_WINS;
    }

    // fix experience and killed
    if ( result_game.army1 || result_game.army2 ) {
        result_game.exp1 = army2->GetDeadHitPoints();
        result_game.exp2 = army1->GetDeadHitPoints();

        if ( army1->GetCommander() && !( result_game.army1 & ( RESULT_RETREAT | RESULT_SURRENDER ) ) ) {
            result_game.exp2 += 500;
        }
        if ( army2->GetCommander() && !( result_game.army2 & ( RESULT_RETREAT | RESULT_SURRENDER ) ) ) {
            result_game.exp1 += 500;
        }

        const Force * army_loss = ( result_game.army1 & RESULT_LOSS ? army1 : ( result_game.army2 & RESULT_LOSS ? army2 : nullptr ) );
        result_game.killed = army_loss ? army_loss->GetDeadCounts() : 0;
    }
}

void Battle::Arena::RemoteTurn( const Unit & b, Actions & a )
{
    DEBUG_LOG( DBG_BATTLE, DBG_WARN, "switch to AI turn" );
    AI::Get().BattleTurn( *this, b, a );
}

void Battle::Arena::HumanTurn( const Unit & b, Actions & a )
{
    if ( interface )
        interface->HumanTurn( b, a );
}

void Battle::Arena::TowerAction( const Tower & twr )
{
    board.Reset();
    board.SetEnemyQuality( twr );
    const Unit * enemy = GetEnemyMaxQuality( twr.GetColor() );

    if ( enemy ) {
        Command cmd( MSG_BATTLE_TOWER, twr.GetType(), enemy->GetUID() );
        ApplyAction( cmd );
    }
}

void Battle::Arena::CatapultAction( void )
{
    if ( catapult ) {
        u32 shots = catapult->GetShots();
        std::vector<u32> values( CAT_CENTRAL_TOWER + 1, 0 );

        values[CAT_WALL1] = GetCastleTargetValue( CAT_WALL1 );
        values[CAT_WALL2] = GetCastleTargetValue( CAT_WALL2 );
        values[CAT_WALL3] = GetCastleTargetValue( CAT_WALL3 );
        values[CAT_WALL4] = GetCastleTargetValue( CAT_WALL4 );
        values[CAT_TOWER1] = GetCastleTargetValue( CAT_TOWER1 );
        values[CAT_TOWER2] = GetCastleTargetValue( CAT_TOWER2 );
        values[CAT_BRIDGE] = GetCastleTargetValue( CAT_BRIDGE );
        values[CAT_CENTRAL_TOWER] = GetCastleTargetValue( CAT_CENTRAL_TOWER );

        Command cmd( MSG_BATTLE_CATAPULT );

        cmd << shots;

        while ( shots-- ) {
            const int target = catapult->GetTarget( values );
            const uint32_t damage = std::min( catapult->GetDamage(), values[target] );
            const bool hit = catapult->IsNextShotHit();

            cmd << target << damage << ( hit ? 1 : 0 );

            if ( hit ) {
                values[target] -= damage;
            }
        }

        // preserve the order of shots - command arguments will be extracted in reverse order
        std::reverse( cmd.begin(), cmd.end() );

        ApplyAction( cmd );
    }
}

Battle::Indexes Battle::Arena::GetPath( const Unit & b, const Position & dst ) const
{
    Indexes result = board.GetAStarPath( b, dst );

    if ( !result.empty() && IS_DEBUG( DBG_BATTLE, DBG_TRACE ) ) {
        std::stringstream ss;
        for ( u32 ii = 0; ii < result.size(); ++ii )
            ss << result[ii] << ", ";
        DEBUG_LOG( DBG_BATTLE, DBG_TRACE, ss.str() );
    }

    return result;
}

Battle::Indexes Battle::Arena::CalculateTwoMoveOverlap( int32_t indexTo, uint32_t movementRange ) const
{
    return _pathfinder.findTwoMovesOverlap( indexTo, movementRange );
}

std::pair<int, uint32_t> Battle::Arena::CalculateMoveToUnit( const Unit & target ) const
{
    std::pair<int, uint32_t> result = { -1, MAXU16 };

    const Position & pos = target.GetPosition();
    const Cell * head = pos.GetHead();
    const Cell * tail = pos.GetTail();

    if ( head ) {
        const ArenaNode & headNode = _pathfinder.getNode( head->GetIndex() );
        if ( headNode._from != -1 ) {
            result.first = headNode._from;
            result.second = headNode._cost;
        }
    }

    if ( tail ) {
        const ArenaNode & tailNode = _pathfinder.getNode( tail->GetIndex() );
        if ( tailNode._from != -1 && tailNode._cost < result.second ) {
            result.first = tailNode._from;
            result.second = tailNode._cost;
        }
    }

    return result;
}

uint32_t Battle::Arena::CalculateMoveDistance( int32_t indexTo ) const
{
    return Board::isValidIndex( indexTo ) ? _pathfinder.getDistance( indexTo ) : MAXU16;
}

bool Battle::Arena::hexIsPassable( int32_t indexTo ) const
{
    return Board::isValidIndex( indexTo ) && _pathfinder.hexIsPassable( indexTo );
}

Battle::Indexes Battle::Arena::getAllAvailableMoves( uint32_t moveRange ) const
{
    return _pathfinder.getAllAvailableMoves( moveRange );
}

Battle::Unit * Battle::Arena::GetTroopBoard( s32 index )
{
    return Board::isValidIndex( index ) ? board[index].GetUnit() : nullptr;
}

const Battle::Unit * Battle::Arena::GetTroopBoard( s32 index ) const
{
    return Board::isValidIndex( index ) ? board[index].GetUnit() : nullptr;
}

const HeroBase * Battle::Arena::GetCommander1( void ) const
{
    return army1->GetCommander();
}

const HeroBase * Battle::Arena::GetCommander2( void ) const
{
    return army2->GetCommander();
}

int Battle::Arena::GetArmyColor1( void ) const
{
    return army1->GetColor();
}

int Battle::Arena::GetArmyColor2( void ) const
{
    return army2->GetColor();
}

int Battle::Arena::GetCurrentColor( void ) const
{
    return current_color;
}

int Battle::Arena::GetOppositeColor( int col ) const
{
    return col == GetArmyColor1() ? GetArmyColor2() : GetArmyColor1();
}

Battle::Unit * Battle::Arena::GetTroopUID( u32 uid )
{
    Units::iterator it = std::find_if( army1->begin(), army1->end(), [uid]( const Unit * unit ) { return unit->isUID( uid ); } );

    if ( it != army1->end() )
        return *it;

    it = std::find_if( army2->begin(), army2->end(), [uid]( const Unit * unit ) { return unit->isUID( uid ); } );

    return it != army2->end() ? *it : nullptr;
}

const Battle::Unit * Battle::Arena::GetTroopUID( u32 uid ) const
{
    Units::const_iterator it = std::find_if( army1->begin(), army1->end(), [uid]( const Unit * unit ) { return unit->isUID( uid ); } );

    if ( it != army1->end() )
        return *it;

    it = std::find_if( army2->begin(), army2->end(), [uid]( const Unit * unit ) { return unit->isUID( uid ); } );

    return it != army2->end() ? *it : nullptr;
}

const Battle::Unit * Battle::Arena::GetEnemyMaxQuality( int my_color ) const
{
    const Unit * res = nullptr;
    s32 quality = 0;

    for ( Board::const_iterator it = board.begin(); it != board.end(); ++it ) {
        const Unit * enemy = ( *it ).GetUnit();

        if ( enemy && enemy->GetColor() != my_color && ( !enemy->isWide() || enemy->GetTailIndex() != ( *it ).GetIndex() ) && quality < ( *it ).GetQuality() ) {
            res = enemy;
            quality = ( *it ).GetQuality();
        }
    }

    return res;
}

void Battle::Arena::FadeArena( bool clearMessageLog ) const
{
    if ( interface )
        interface->FadeArena( clearMessageLog );
}

const SpellStorage & Battle::Arena::GetUsageSpells( void ) const
{
    return usage_spells;
}

s32 Battle::Arena::GetFreePositionNearHero( int color ) const
{
    const int cells1[] = {11, 22, 33};
    const int cells2[] = {21, 32, 43};
    const int * cells = nullptr;

    if ( army1->GetColor() == color )
        cells = cells1;
    else if ( army2->GetColor() == color )
        cells = cells2;

    if ( cells ) {
        for ( u32 ii = 0; ii < 3; ++ii ) {
            if ( board[cells[ii]].isPassable1( true ) && nullptr == board[cells[ii]].GetUnit() ) {
                return cells[ii];
            }
        }
    }

    return -1;
}

bool Battle::Arena::CanSurrenderOpponent( int color ) const
{
    const HeroBase * hero1 = GetCommander( color, true ); // enemy
    const HeroBase * hero2 = GetCommander( color, false );
    return hero1 && hero1->isHeroes() && hero2 && hero2->isHeroes() && !world.GetKingdom( hero2->GetColor() ).GetCastles().empty();
}

bool Battle::Arena::CanRetreatOpponent( int color ) const
{
    const HeroBase * hero = GetCommander( color );
    return hero && hero->isHeroes() && ( color == army1->GetColor() || hero->inCastle() == nullptr );
}

bool Battle::Arena::isSpellcastDisabled() const
{
    const HeroBase * hero1 = army1->GetCommander();
    const HeroBase * hero2 = army2->GetCommander();

    if ( ( hero1 && hero1->HasArtifact( Artifact::SPHERE_NEGATION ) ) || ( hero2 && hero2->HasArtifact( Artifact::SPHERE_NEGATION ) ) ) {
        return true;
    }
    return false;
}

bool Battle::Arena::isDisableCastSpell( const Spell & spell, std::string * msg )
{
    const HeroBase * current_commander = GetCurrentCommander();

    // check sphere negation (only for heroes)
    if ( isSpellcastDisabled() ) {
        if ( msg )
            *msg = _( "The Sphere of Negation artifact is in effect for this battle, disabling all combat spells." );
        return true;
    }

    // check casted
    if ( current_commander ) {
        if ( current_commander->Modes( Heroes::SPELLCASTED ) ) {
            if ( msg )
                *msg = _( "You have already cast a spell this round." );
            return true;
        }

        if ( spell == Spell::EARTHQUAKE && !castle ) {
            *msg = _( "That spell will affect no one!" );
            return true;
        }
        else if ( spell.isSummon() ) {
            const Unit * elem = GetCurrentForce().FindMode( CAP_SUMMONELEM );
            bool affect = true;

            if ( elem )
                switch ( spell() ) {
                case Spell::SUMMONEELEMENT:
                    if ( elem->GetID() != Monster::EARTH_ELEMENT )
                        affect = false;
                    break;
                case Spell::SUMMONAELEMENT:
                    if ( elem->GetID() != Monster::AIR_ELEMENT )
                        affect = false;
                    break;
                case Spell::SUMMONFELEMENT:
                    if ( elem->GetID() != Monster::FIRE_ELEMENT )
                        affect = false;
                    break;
                case Spell::SUMMONWELEMENT:
                    if ( elem->GetID() != Monster::WATER_ELEMENT )
                        affect = false;
                    break;
                default:
                    break;
                }
            if ( !affect ) {
                *msg = _( "You may only summon one type of elemental per combat." );
                return true;
            }

            if ( 0 > GetFreePositionNearHero( current_color ) ) {
                *msg = _( "There is no open space adjacent to your hero to summon an Elemental to." );
                return true;
            }
        }
        else if ( spell.isValid() ) {
            // check army
            for ( Board::const_iterator it = board.begin(); it != board.end(); ++it ) {
                const Battle::Unit * b = ( *it ).GetUnit();

                if ( b ) {
                    if ( b->AllowApplySpell( spell, current_commander, nullptr ) )
                        return false;
                }
                else
                    // check graveyard
                    if ( GraveyardAllowResurrect( ( *it ).GetIndex(), spell ) )
                    return false;
            }
            *msg = _( "That spell will affect no one!" );
            return true;
        }
    }

    // may be check other..
    /*
     */

    return false;
}

bool Battle::Arena::GraveyardAllowResurrect( s32 index, const Spell & spell ) const
{
    if ( !spell.isResurrect() )
        return false;

    const HeroBase * hero = GetCurrentCommander();
    if ( hero == nullptr )
        return false;

    const Unit * killed = GetTroopUID( graveyard.GetLastTroopUID( index ) );
    if ( killed == nullptr )
        return false;

    if ( !killed->AllowApplySpell( spell, hero, nullptr ) )
        return false;

    if ( Board::GetCell( index )->GetUnit() != nullptr )
        return false;

    if ( !killed->isWide() )
        return true;

    const int tailIndex = killed->GetTailIndex();
    const int headIndex = killed->GetHeadIndex();
    const int secondIndex = tailIndex == index ? headIndex : tailIndex;

    if ( Board::GetCell( secondIndex )->GetUnit() != nullptr )
        return false;

    return true;
}

const Battle::Unit * Battle::Arena::GraveyardLastTroop( s32 index ) const
{
    return GetTroopUID( graveyard.GetLastTroopUID( index ) );
}

std::vector<const Battle::Unit *> Battle::Arena::GetGraveyardTroops( const int32_t hexIndex ) const
{
    const TroopUIDs & ids = graveyard.GetTroopUIDs( hexIndex );

    std::vector<const Battle::Unit *> units( ids.size() );
    for ( size_t i = 0; i < ids.size(); ++i ) {
        units[i] = GetTroopUID( ids[i] );
    }

    return units;
}

Battle::Indexes Battle::Arena::GraveyardClosedCells( void ) const
{
    return graveyard.GetClosedCells();
}

void Battle::Arena::SetCastleTargetValue( int target, u32 value )
{
    switch ( target ) {
    case CAT_WALL1:
        board[8].SetObject( value );
        break;
    case CAT_WALL2:
        board[29].SetObject( value );
        break;
    case CAT_WALL3:
        board[73].SetObject( value );
        break;
    case CAT_WALL4:
        board[96].SetObject( value );
        break;

    case CAT_TOWER1:
        if ( towers[0] && towers[0]->isValid() )
            towers[0]->SetDestroy();
        break;
    case CAT_TOWER2:
        if ( towers[2] && towers[2]->isValid() )
            towers[2]->SetDestroy();
        break;
    case CAT_CENTRAL_TOWER:
        if ( towers[1] && towers[1]->isValid() )
            towers[1]->SetDestroy();
        break;

    case CAT_BRIDGE:
        if ( bridge->isValid() ) {
            if ( !bridge->isDown() ) {
                if ( interface ) {
                    interface->RedrawBridgeAnimation( true );
                }

                bridge->SetDown( true );
            }

            bridge->SetDestroy();
        }
        break;

    default:
        break;
    }
}

u32 Battle::Arena::GetCastleTargetValue( int target ) const
{
    switch ( target ) {
    case CAT_WALL1:
        return board[8].GetObject();
    case CAT_WALL2:
        return board[29].GetObject();
    case CAT_WALL3:
        return board[73].GetObject();
    case CAT_WALL4:
        return board[96].GetObject();

    case CAT_TOWER1:
        return towers[0] && towers[0]->isValid();
    case CAT_TOWER2:
        return towers[2] && towers[2]->isValid();
    case CAT_CENTRAL_TOWER:
        return towers[1] && towers[1]->isValid();

    case CAT_BRIDGE:
        return bridge->isValid();

    default:
        break;
    }
    return 0;
}

std::vector<int> Battle::Arena::GetCastleTargets( void ) const
{
    std::vector<int> targets;
    targets.reserve( 8 );

    // check walls
    if ( 0 != board[8].GetObject() )
        targets.push_back( CAT_WALL1 );
    if ( 0 != board[29].GetObject() )
        targets.push_back( CAT_WALL2 );
    if ( 0 != board[73].GetObject() )
        targets.push_back( CAT_WALL3 );
    if ( 0 != board[96].GetObject() )
        targets.push_back( CAT_WALL4 );

    // check right/left towers
    if ( towers[0] && towers[0]->isValid() )
        targets.push_back( CAT_TOWER1 );
    if ( towers[2] && towers[2]->isValid() )
        targets.push_back( CAT_TOWER2 );

    return targets;
}

const HeroBase * Battle::Arena::GetCommander( int color, bool invert ) const
{
    const HeroBase * commander = nullptr;

    if ( army1->GetColor() == color ) {
        commander = invert ? army2->GetCommander() : army1->GetCommander();
    }
    else {
        commander = invert ? army1->GetCommander() : army2->GetCommander();
    }

    return commander;
}

const HeroBase * Battle::Arena::GetCurrentCommander( void ) const
{
    return GetCommander( current_color );
}

Battle::Unit * Battle::Arena::CreateElemental( const Spell & spell )
{
    const HeroBase * hero = GetCurrentCommander();
    const s32 pos = GetFreePositionNearHero( current_color );

    if ( 0 > pos || !hero ) {
        DEBUG_LOG( DBG_BATTLE, DBG_WARN, "internal error" );
        return nullptr;
    }

    Force & army = GetCurrentForce();
    Unit * elem = army.FindMode( CAP_SUMMONELEM );
    bool affect = true;

    if ( elem )
        switch ( spell() ) {
        case Spell::SUMMONEELEMENT:
            if ( elem->GetID() != Monster::EARTH_ELEMENT )
                affect = false;
            break;
        case Spell::SUMMONAELEMENT:
            if ( elem->GetID() != Monster::AIR_ELEMENT )
                affect = false;
            break;
        case Spell::SUMMONFELEMENT:
            if ( elem->GetID() != Monster::FIRE_ELEMENT )
                affect = false;
            break;
        case Spell::SUMMONWELEMENT:
            if ( elem->GetID() != Monster::WATER_ELEMENT )
                affect = false;
            break;
        default:
            break;
        }

    if ( !affect ) {
        DEBUG_LOG( DBG_BATTLE, DBG_WARN, "other elemental summon" );
        return nullptr;
    }

    Monster mons( spell );

    if ( !mons.isValid() ) {
        DEBUG_LOG( DBG_BATTLE, DBG_WARN, "unknown id" );
        return nullptr;
    }

    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, mons.GetName() << ", position: " << pos );
    u32 count = spell.ExtraValue() * hero->GetPower();
    u32 acount = hero->HasArtifact( Artifact::BOOK_ELEMENTS );
    if ( acount )
        count *= acount * 2;

    elem = new Unit( Troop( mons, count ), pos, hero == army2->GetCommander() );

    if ( elem ) {
        elem->SetModes( CAP_SUMMONELEM );
        elem->SetArmy( hero->GetArmy() );
        army.push_back( elem );
    }
    else {
        DEBUG_LOG( DBG_BATTLE, DBG_WARN, "is nullptr" );
    }

    return elem;
}

Battle::Unit * Battle::Arena::CreateMirrorImage( Unit & b, s32 pos )
{
    Unit * image = new Unit( b, pos, b.isReflect() );

    if ( image ) {
        b.SetMirror( image );
        image->SetArmy( *b.GetArmy() );
        image->SetMirror( &b );
        image->SetModes( CAP_MIRRORIMAGE );
        b.SetModes( CAP_MIRROROWNER );

        GetCurrentForce().push_back( image );
    }
    else {
        DEBUG_LOG( DBG_BATTLE, DBG_WARN, "internal error" );
    }

    return image;
}

bool Battle::Arena::IsShootingPenalty( const Unit & attacker, const Unit & defender ) const
{
    if ( defender.Modes( CAP_TOWER ) || attacker.Modes( CAP_TOWER ) )
        return false;

    // check golden bow artifact
    const HeroBase * hero = attacker.GetCommander();
    if ( hero && hero->HasArtifact( Artifact::GOLDEN_BOW ) )
        return false;

    if ( castle == nullptr ) {
        return false;
    }

    // archery skill
    if ( hero && hero->GetLevelSkill( Skill::Secondary::ARCHERY ) != Skill::Level::NONE )
        return false;

    // attacker is castle owner
    if ( attacker.GetColor() == castle->GetColor() && !attacker.OutOfWalls() )
        return false;

    if ( defender.GetColor() == castle->GetColor() && defender.OutOfWalls() )
        return false;

    // check castle walls defensed
    const std::vector<fheroes2::Point> points = GetLinePoints( attacker.GetBackPoint(), defender.GetBackPoint(), CELLW / 3 );

    for ( std::vector<fheroes2::Point>::const_iterator it = points.begin(); it != points.end(); ++it ) {
        if ( 0 == board[8].GetObject() && ( board[8].GetPos() & *it ) )
            return false;
        else if ( 0 == board[29].GetObject() && ( board[29].GetPos() & *it ) )
            return false;
        else if ( 0 == board[73].GetObject() && ( board[73].GetPos() & *it ) )
            return false;
        else if ( 0 == board[96].GetObject() && ( board[96].GetPos() & *it ) )
            return false;
    }

    return true;
}

Battle::Force & Battle::Arena::GetForce1( void )
{
    return *army1;
}

Battle::Force & Battle::Arena::GetForce2( void )
{
    return *army2;
}

Battle::Force & Battle::Arena::GetForce( int color, bool invert )
{
    if ( army1->GetColor() == color )
        return invert ? *army2 : *army1;

    return invert ? *army1 : *army2;
}

Battle::Force & Battle::Arena::GetCurrentForce( void )
{
    return GetForce( current_color, false );
}

int Battle::Arena::GetICNCovr( void ) const
{
    return icn_covr;
}

u32 Battle::Arena::GetCurrentTurn( void ) const
{
    return current_turn;
}

Battle::Result & Battle::Arena::GetResult( void )
{
    return result_game;
}

bool Battle::Arena::CanBreakAutoBattle( void ) const
{
    return ( auto_battle & current_color ) && GetCurrentCommander() && !GetCurrentCommander()->isControlAI();
}

void Battle::Arena::BreakAutoBattle( void )
{
    auto_battle &= ~current_color;
}
