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

#include "agg.h"
#include "army.h"
#include "army_troop.h"
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
#include "cursor.h"
#include "ground.h"
#include "mus.h"
#include "race.h"
#include "settings.h"
#include "speed.h"
#include "tools.h"
#include "world.h"

namespace Battle
{
    Arena * arena = NULL;
}

int GetCovr( int ground )
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

    return covrs.empty() ? ICN::UNKNOWN : *Rand::Get( covrs );
}

StreamBase & Battle::operator<<( StreamBase & msg, const TargetInfo & t )
{
    return msg << ( t.defender ? t.defender->GetUID() : static_cast<u32>( 0 ) ) << t.damage << t.killed << t.resist;
}

StreamBase & Battle::operator>>( StreamBase & msg, TargetInfo & t )
{
    u32 uid = 0;

    msg >> uid >> t.damage >> t.killed >> t.resist;

    t.defender = uid ? GetArena()->GetTroopUID( uid ) : NULL;

    return msg;
}

StreamBase & Battle::operator<<( StreamBase & msg, const TargetsInfo & ts )
{
    msg << static_cast<u32>( ts.size() );

    for ( TargetsInfo::const_iterator it = ts.begin(); it != ts.end(); ++it )
        msg << *it;

    return msg;
}

StreamBase & Battle::operator>>( StreamBase & msg, TargetsInfo & ts )
{
    u32 size = 0;

    msg >> size;
    ts.resize( size );

    for ( TargetsInfo::iterator it = ts.begin(); it != ts.end(); ++it )
        msg >> *it;

    return msg;
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
    return NULL;
}

Battle::Arena::Arena( Army & a1, Army & a2, s32 index, bool local )
    : army1( NULL )
    , army2( NULL )
    , armies_order( NULL )
    , castle( NULL )
    , current_color( 0 )
    , catapult( NULL )
    , bridge( NULL )
    , interface( NULL )
    , icn_covr( ICN::UNKNOWN )
    , current_turn( 0 )
    , auto_battle( 0 )
    , end_turn( false )
{
    const Settings & conf = Settings::Get();
    usage_spells.reserve( 20 );

    arena = this;
    army1 = new Force( a1, false );
    army2 = new Force( a2, true );

    // init castle (interface ahead)
    castle = world.GetCastle( Maps::GetPoint( index ) );

    if ( castle ) {
        CastleHeroes heroes = world.GetHeroes( *castle );

        // skip if present guard and guest
        if ( heroes.FullHouse() )
            castle = NULL;

        // skip for town
        if ( castle && !castle->isCastle() )
            castle = NULL;
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

    towers[0] = NULL;
    towers[1] = NULL;
    towers[2] = NULL;

    if ( castle ) {
        // init
        towers[0] = castle->isBuild( BUILD_LEFTTURRET ) ? new Tower( *castle, TWR_LEFT ) : NULL;
        towers[1] = new Tower( *castle, TWR_CENTER );
        towers[2] = castle->isBuild( BUILD_RIGHTTURRET ) ? new Tower( *castle, TWR_RIGHT ) : NULL;
        const bool fortification = ( Race::KNGT == castle->GetRace() ) && castle->isBuild( BUILD_SPEC );
        catapult = army1->GetCommander() ? new Catapult( *army1->GetCommander() ) : NULL;
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
        board[49].SetObject( 1 );
        board[50].SetObject( 1 );
    }
    else
    // set obstacles
    {
        icn_covr = Maps::ScanAroundObject( index, MP2::OBJ_CRATER ).size() ? GetCovr( world.GetTiles( index ).GetGround() ) : ICN::UNKNOWN;

        if ( icn_covr != ICN::UNKNOWN )
            board.SetCovrObjects( icn_covr );
        else
            board.SetCobjObjects( world.GetTiles( index ) );
    }


    if ( interface ) {
        Cursor & cursor = Cursor::Get();
        fheroes2::Display & display = fheroes2::Display::instance();

        cursor.Hide();
        cursor.SetThemes( Cursor::WAR_NONE );

        if ( conf.ExtGameUseFade() )
            fheroes2::FadeDisplay();

        interface->Redraw();
        cursor.Show();
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

    if ( towers[0] )
        delete towers[0];
    if ( towers[1] )
        delete towers[1];
    if ( towers[2] )
        delete towers[2];

    if ( catapult )
        delete catapult;
    if ( interface )
        delete interface;
    if ( armies_order )
        delete armies_order;
    if ( bridge )
        delete bridge;
}

void Battle::Arena::TurnTroop( Unit * current_troop )
{
    Actions actions;
    end_turn = false;
    const bool isImmovable = current_troop->Modes( SP_BLIND | IS_PARALYZE_MAGIC );

    DEBUG( DBG_BATTLE, DBG_TRACE, current_troop->String( true ) );

    // morale check right before the turn
    if ( !isImmovable ) {
        if ( current_troop->isAffectedByMorale() )
            current_troop->SetRandomMorale();
    }

    while ( !end_turn ) {
        if ( !current_troop->isValid() ) { // looks like the unit died
            end_turn = true;
        }
        else if ( current_troop->Modes( MORALE_BAD ) ) { // bad morale
            actions.push_back( Command( MSG_BATTLE_MORALE, current_troop->GetUID(), false ) );
            end_turn = true;
        }
        else {
            // re-calculate possible paths in case unit moved or it's a new turn
            _pathfinder.calculate( *current_troop );

            // turn opponents
            if ( current_troop->isControlRemote() )
                RemoteTurn( *current_troop, actions );
            else {
                if ( ( current_troop->GetCurrentControl() & CONTROL_AI ) || ( current_color & auto_battle ) ) {
                    AI::Get().BattleTurn( *this, *current_troop, actions );
                }
                else {
                    HumanTurn( *current_troop, actions );
                }
            }
        }

        // apply task
        while ( actions.size() ) {
            // apply action
            ApplyAction( actions.front() );
            actions.pop_front();

            // rescan orders
            if ( armies_order )
                Force::UpdateOrderUnits( *army1, *army2, *armies_order );

            // check end battle
            if ( !BattleValid() ) {
                end_turn = true;
                break;
            }

            // good morale
            if ( !end_turn && current_troop->isValid() && !current_troop->Modes( TR_SKIPMOVE ) && current_troop->Modes( TR_MOVED ) && current_troop->Modes( MORALE_GOOD )
                 && !isImmovable ) {
                actions.push_back( Command( MSG_BATTLE_MORALE, current_troop->GetUID(), true ) );
                end_turn = false;
            }
        }

        if ( current_troop->Modes( TR_SKIPMOVE | TR_MOVED ) )
            end_turn = true;

        board.Reset();

        DELAY( 10 );
    }
}

bool Battle::Arena::BattleValid( void ) const
{
    return army1->isValid() && army2->isValid() && 0 == result_game.army1 && 0 == result_game.army2;
}

void Battle::Arena::Turns( void )
{
    const Settings & conf = Settings::Get();

    ++current_turn;
    DEBUG( DBG_BATTLE, DBG_TRACE, current_turn );

    if ( interface && conf.Music() && !Music::isPlaying() )
        AGG::PlayMusic( MUS::GetBattleRandom() );

    army1->NewTurn();
    army2->NewTurn();

    bool tower_moved = false;
    bool catapult_moved = false;

    Unit * current_troop = NULL;

    // rescan orders
    if ( armies_order )
        Force::UpdateOrderUnits( *army1, *army2, *armies_order );

    while ( BattleValid() && NULL != ( current_troop = Force::GetCurrentUnit( *army1, *army2, current_troop, true ) ) ) {
        current_color = current_troop->GetArmyColor();

        // first turn: castle and catapult action
        if ( castle ) {
            if ( !catapult_moved && current_troop->GetColor() == army1->GetColor() ) {
                CatapultAction();
                catapult_moved = true;
            }

            if ( !tower_moved && current_troop->GetColor() == army2->GetColor() ) {
                if ( towers[1] && towers[1]->isValid() )
                    TowerAction( *towers[1] );
                if ( towers[0] && towers[0]->isValid() )
                    TowerAction( *towers[0] );
                if ( towers[2] && towers[2]->isValid() )
                    TowerAction( *towers[2] );
                tower_moved = true;

                // check dead last army from towers
                if ( !BattleValid() )
                    break;
            }
        }

        // set bridge passable
        if ( bridge && bridge->isValid() && !bridge->isDown() )
            bridge->SetPassable( *current_troop );

        // turn troop
        TurnTroop( current_troop );
    }

    current_troop = NULL;

    // can skip move ?
    if ( Settings::Get().ExtBattleSoftWait() ) {
        while ( BattleValid() && NULL != ( current_troop = Force::GetCurrentUnit( *army1, *army2, current_troop, false ) ) ) {
            current_color = current_troop->GetArmyColor();

            // set bridge passable
            if ( bridge && bridge->isValid() && !bridge->isDown() )
                bridge->SetPassable( *current_troop );

            // turn troop
            TurnTroop( current_troop );
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

        if ( army1->GetCommander() )
            result_game.exp2 += 500;
        if ( army2->GetCommander() )
            result_game.exp1 += 500;

        Force * army_loss = ( result_game.army1 & RESULT_LOSS ? army1 : ( result_game.army2 & RESULT_LOSS ? army2 : NULL ) );
        result_game.killed = army_loss ? army_loss->GetDeadCounts() : 0;
    }
}

void Battle::Arena::RemoteTurn( const Unit & b, Actions & a )
{
    DEBUG( DBG_BATTLE, DBG_WARN, "switch to AI turn" );
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
        std::vector<u32> values( CAT_MISS + 1, 0 );

        values[CAT_WALL1] = GetCastleTargetValue( CAT_WALL1 );
        values[CAT_WALL2] = GetCastleTargetValue( CAT_WALL2 );
        values[CAT_WALL3] = GetCastleTargetValue( CAT_WALL3 );
        values[CAT_WALL4] = GetCastleTargetValue( CAT_WALL4 );
        values[CAT_TOWER1] = GetCastleTargetValue( CAT_TOWER1 );
        values[CAT_TOWER2] = GetCastleTargetValue( CAT_TOWER2 );
        values[CAT_CENTRAL_TOWER] = GetCastleTargetValue( CAT_CENTRAL_TOWER );
        values[CAT_BRIDGE] = GetCastleTargetValue( CAT_BRIDGE );

        Command cmd( MSG_BATTLE_CATAPULT );

        while ( shots-- ) {
            int target = catapult->GetTarget( values );
            u32 damage = std::min( catapult->GetDamage(), values[target] );
            cmd << damage << target;
            values[target] -= damage;
        }

        cmd << catapult->GetShots();
        ApplyAction( cmd );
    }
}

Battle::Indexes Battle::Arena::GetPath( const Unit & b, const Position & dst )
{
    Indexes result = board.GetAStarPath( b, dst );

    if ( result.size() ) {
        if ( IS_DEBUG( DBG_BATTLE, DBG_TRACE ) ) {
            std::stringstream ss;
            for ( u32 ii = 0; ii < result.size(); ++ii )
                ss << result[ii] << ", ";
            DEBUG( DBG_BATTLE, DBG_TRACE, ss.str() );
        }
    }

    return result;
}

std::pair<int, uint32_t> Battle::Arena::CalculateMoveToUnit( const Unit & target )
{
    std::pair<int, uint32_t> result = {-1, MAXU16};

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

uint32_t Battle::Arena::CalculateMoveDistance( int32_t indexTo )
{
    return Board::isValidIndex( indexTo ) ? _pathfinder.getDistance( indexTo ) : MAXU16;
}

bool Battle::Arena::hexIsAccessible( int32_t indexTo )
{
    return Board::isValidIndex( indexTo ) && _pathfinder.hexIsAccessible( indexTo );
}

bool Battle::Arena::hexIsPassable( int32_t indexTo )
{
    return Board::isValidIndex( indexTo ) && _pathfinder.hexIsPassable( indexTo );
}

Battle::Unit * Battle::Arena::GetTroopBoard( s32 index )
{
    return Board::isValidIndex( index ) ? board[index].GetUnit() : NULL;
}

const Battle::Unit * Battle::Arena::GetTroopBoard( s32 index ) const
{
    return Board::isValidIndex( index ) ? board[index].GetUnit() : NULL;
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

    return it != army2->end() ? *it : NULL;
}

const Battle::Unit * Battle::Arena::GetTroopUID( u32 uid ) const
{
    Units::const_iterator it = std::find_if( army1->begin(), army1->end(), [uid]( const Unit * unit ) { return unit->isUID( uid ); } );

    if ( it != army1->end() )
        return *it;

    it = std::find_if( army2->begin(), army2->end(), [uid]( const Unit * unit ) { return unit->isUID( uid ); } );

    return it != army2->end() ? *it : NULL;
}

const Battle::Unit * Battle::Arena::GetEnemyMaxQuality( int my_color ) const
{
    const Unit * res = NULL;
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
    const int * cells = NULL;

    if ( army1->GetColor() == color )
        cells = cells1;
    else if ( army2->GetColor() == color )
        cells = cells2;

    if ( cells )
        for ( u32 ii = 0; ii < 3; ++ii )
            if ( board[cells[ii]].isPassable1( true ) && NULL == board[cells[ii]].GetUnit() )
                return cells[ii];

    return -1;
}

bool Battle::Arena::CanSurrenderOpponent( int color ) const
{
    const HeroBase * hero1 = GetCommander( color, false ); // enemy
    const HeroBase * hero2 = GetCommander( color, true );
    return hero1 && hero1->isHeroes() && hero2 && world.GetKingdom( hero2->GetColor() ).GetCastles().size();
}

bool Battle::Arena::CanRetreatOpponent( int color ) const
{
    const HeroBase * hero = army1->GetColor() == color ? army1->GetCommander() : army2->GetCommander();
    return hero && hero->isHeroes() && NULL == hero->inCastle() && world.GetKingdom( hero->GetColor() ).GetCastles().size();
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
                    if ( b->AllowApplySpell( spell, current_commander, NULL ) )
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
    if ( hero == NULL )
        return false;

    const Unit * killed = GetTroopUID( graveyard.GetLastTroopUID( index ) );
    if ( killed == NULL )
        return false;

    if ( !killed->AllowApplySpell( spell, hero, NULL ) )
        return false;

    if ( Board::GetCell( index )->GetUnit() != NULL )
        return false;

    if ( !killed->isWide() )
        return true;

    const int tailIndex = killed->GetTailIndex();
    const int headIndex = killed->GetHeadIndex();
    const int secondIndex = tailIndex == index ? headIndex : tailIndex;

    if ( Board::GetCell( secondIndex )->GetUnit() != NULL )
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
            if ( interface )
                interface->RedrawBridgeAnimation( true );
            bridge->SetDown( true );
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

StreamBase & Battle::operator<<( StreamBase & msg, const Arena & a )
{
    msg << a.current_turn << a.board << *a.army1 << *a.army2;

    const HeroBase * hero1 = a.army1->GetCommander();
    const HeroBase * hero2 = a.army2->GetCommander();

    if ( hero1 )
        msg << hero1->GetType() << *hero1;
    else
        msg << static_cast<int>( HeroBase::UNDEFINED );

    if ( hero2 )
        msg << hero2->GetType() << *hero2;
    else
        msg << static_cast<int>( HeroBase::UNDEFINED );

    return msg;
}

StreamBase & Battle::operator>>( StreamBase & msg, Arena & a )
{
    msg >> a.current_turn >> a.board >> *a.army1 >> *a.army2;

    int type;
    HeroBase * hero1 = a.army1->GetCommander();
    HeroBase * hero2 = a.army2->GetCommander();

    msg >> type;
    if ( hero1 && type == hero1->GetType() )
        msg >> *hero1;

    msg >> type;
    if ( hero2 && type == hero2->GetType() )
        msg >> *hero2;

    return msg;
}

const HeroBase * Battle::Arena::GetCommander( int color, bool invert ) const
{
    const HeroBase * commander = NULL;

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

bool Battle::Arena::NetworkTurn( void )
{
    return interface && interface->NetworkTurn( result_game );
}

Battle::Unit * Battle::Arena::CreateElemental( const Spell & spell )
{
    const HeroBase * hero = GetCurrentCommander();
    const s32 pos = GetFreePositionNearHero( current_color );

    if ( 0 > pos || !hero ) {
        DEBUG( DBG_BATTLE, DBG_WARN, "internal error" );
        return NULL;
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
        DEBUG( DBG_BATTLE, DBG_WARN, "other elemental summon" );
        return NULL;
    }

    Monster mons( spell );

    if ( !mons.isValid() ) {
        DEBUG( DBG_BATTLE, DBG_WARN, "unknown id" );
        return NULL;
    }

    DEBUG( DBG_BATTLE, DBG_TRACE, mons.GetName() << ", position: " << pos );
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
        DEBUG( DBG_BATTLE, DBG_WARN, "is NULL" );
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
        DEBUG( DBG_BATTLE, DBG_WARN, "internal error" );
    }

    return image;
}

u32 Battle::Arena::GetObstaclesPenalty( const Unit & attacker, const Unit & defender ) const
{
    if ( defender.Modes( CAP_TOWER ) || attacker.Modes( CAP_TOWER ) )
        return 0;

    // check golden bow artifact
    const HeroBase * enemy = attacker.GetCommander();
    if ( enemy && enemy->HasArtifact( Artifact::GOLDEN_BOW ) )
        return 0;

    u32 result = 0;
    const u32 step = CELLW / 3;

    if ( castle ) {
        // archery skill
        if ( enemy && Skill::Level::NONE != enemy->GetLevelSkill( Skill::Secondary::ARCHERY ) )
            return 0;

        // attacker is castle owner
        if ( attacker.GetColor() == castle->GetColor() && !attacker.OutOfWalls() )
            return 0;

        if ( defender.GetColor() == castle->GetColor() && defender.OutOfWalls() )
            return 0;

        // check castle walls defensed
        const Points points = GetLinePoints( attacker.GetBackPoint(), defender.GetBackPoint(), step );

        for ( Points::const_iterator it = points.begin(); it != points.end(); ++it ) {
            if ( 0 == board[8].GetObject() && ( board[8].GetPos() & *it ) )
                return 0;
            else if ( 0 == board[29].GetObject() && ( board[29].GetPos() & *it ) )
                return 0;
            else if ( 0 == board[73].GetObject() && ( board[73].GetPos() & *it ) )
                return 0;
            else if ( 0 == board[96].GetObject() && ( board[96].GetPos() & *it ) )
                return 0;
        }

        result = 1;
    }
    else if ( Settings::Get().ExtBattleObjectsArchersPenalty() ) {
        const Points points = GetLinePoints( attacker.GetBackPoint(), defender.GetBackPoint(), step );
        Indexes indexes;
        indexes.reserve( points.size() );

        for ( Points::const_iterator it = points.begin(); it != points.end(); ++it ) {
            const s32 index = board.GetIndexAbsPosition( *it );
            if ( Board::isValidIndex( index ) )
                indexes.push_back( index );
        }

        if ( indexes.size() ) {
            std::sort( indexes.begin(), indexes.end() );
            indexes.resize( std::distance( indexes.begin(), std::unique( indexes.begin(), indexes.end() ) ) );
        }

        for ( Indexes::const_iterator it = indexes.begin(); it != indexes.end(); ++it ) {
            // obstacles
            switch ( board[*it].GetObject() ) {
            // tree
            case 0x82:
            // trock
            case 0x85:
            // tree
            case 0x89:
            // tree
            case 0x8D:
            // rock
            case 0x95:
            case 0x96:
            // stub
            case 0x9A:
            // dead tree
            case 0x9B:
            // tree
            case 0x9C:
                ++result;
                break;

            default:
                break;
            }
        }

        if ( enemy ) {
            switch ( enemy->GetLevelSkill( Skill::Secondary::ARCHERY ) ) {
            case Skill::Level::BASIC:
                if ( result < 2 )
                    return 0;
                break;
            case Skill::Level::ADVANCED:
                if ( result < 3 )
                    return 0;
                break;
            case Skill::Level::EXPERT:
                return 0;
            default:
                break;
            }
        }
    }

    return result;
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
