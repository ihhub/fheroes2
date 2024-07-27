/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include "battle_arena.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <map>
#include <numeric>
#include <ostream>
#include <random>
#include <type_traits>
#include <utility>

#include "ai_battle.h"
#include "army.h"
#include "army_troop.h"
#include "artifact.h"
#include "artifact_info.h"
#include "audio.h"
#include "battle_army.h"
#include "battle_bridge.h"
#include "battle_catapult.h"
#include "battle_cell.h"
#include "battle_command.h"
#include "battle_interface.h"
#include "battle_tower.h"
#include "battle_troop.h"
#include "castle.h"
#include "color.h"
#include "ground.h"
#include "heroes.h"
#include "heroes_base.h"
#include "icn.h"
#include "localevent.h"
#include "logging.h"
#include "maps.h"
#include "maps_tiles.h"
#include "math_base.h"
#include "monster.h"
#include "players.h"
#include "rand.h"
#include "skill.h"
#include "speed.h"
#include "spell_info.h"
#include "tools.h"
#include "translations.h"
#include "world.h"

namespace
{
    Battle::Arena * arena = nullptr;

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

    Battle::Unit * GetCurrentUnit( Battle::Units & units1, Battle::Units & units2, const bool units1GoFirst, const bool ordersMode )
    {
        Battle::Unit * result = nullptr;

        const auto unitFilter = []( const Battle::Unit * unit ) { return unit->GetSpeed() > Speed::STANDING; };

        Battle::Units::iterator it1 = std::find_if( units1.begin(), units1.end(), unitFilter );
        Battle::Units::iterator it2 = std::find_if( units2.begin(), units2.end(), unitFilter );

        if ( it1 != units1.end() && it2 != units2.end() ) {
            if ( ( *it1 )->GetSpeed() == ( *it2 )->GetSpeed() ) {
                result = units1GoFirst ? *it1 : *it2;
            }
            else {
                result = ( ( *it1 )->GetSpeed() > ( *it2 )->GetSpeed() ) ? *it1 : *it2;
            }
        }
        else if ( it1 != units1.end() ) {
            result = *it1;
        }
        else if ( it2 != units2.end() ) {
            result = *it2;
        }

        if ( result && ordersMode ) {
            if ( it1 != units1.end() && result == *it1 ) {
                units1.erase( it1 );
            }
            else if ( it2 != units2.end() && result == *it2 ) {
                units2.erase( it2 );
            }
        }

        return result;
    }

    Battle::Unit * GetCurrentUnit( const Battle::Force & army1, const Battle::Force & army2, const int preferredColor )
    {
        Battle::Units units1( army1.getUnits(), Battle::Units::REMOVE_INVALID_UNITS );
        Battle::Units units2( army2.getUnits(), Battle::Units::REMOVE_INVALID_UNITS );

        units1.SortFastest();
        units2.SortFastest();

        Battle::Unit * result = GetCurrentUnit( units1, units2, preferredColor != army2.GetColor(), false );
        if ( result == nullptr ) {
            return result;
        }

        assert( result->isValid() );

        return result;
    }

    void UpdateOrderOfUnits( const Battle::Force & army1, const Battle::Force & army2, const Battle::Unit * currentUnit, int preferredColor,
                             const Battle::Units & orderHistory, Battle::Units & orderOfUnits )
    {
        orderOfUnits.assign( orderHistory.begin(), orderHistory.end() );

        Battle::Units units1( army1.getUnits(), Battle::Units::REMOVE_INVALID_UNITS );
        Battle::Units units2( army2.getUnits(), Battle::Units::REMOVE_INVALID_UNITS );

        units1.SortFastest();
        units2.SortFastest();

        while ( true ) {
            Battle::Unit * unit = GetCurrentUnit( units1, units2, preferredColor != army2.GetColor(), true );
            if ( unit == nullptr ) {
                break;
            }

            assert( unit->isValid() );

            if ( unit == currentUnit ) {
                continue;
            }

            preferredColor = ( unit->GetArmyColor() == army1.GetColor() ) ? army2.GetColor() : army1.GetColor();

            orderOfUnits.push_back( unit );
        }
    }
}

Battle::Arena * Battle::GetArena()
{
    return arena;
}

const Castle * Battle::Arena::GetCastle()
{
    assert( arena != nullptr );

    return arena->castle;
}

Battle::Bridge * Battle::Arena::GetBridge()
{
    assert( arena != nullptr );

    return arena->_bridge.get();
}

Battle::Board * Battle::Arena::GetBoard()
{
    assert( arena != nullptr );

    return &arena->board;
}

Battle::Graveyard * Battle::Arena::GetGraveyard()
{
    assert( arena != nullptr );

    return &arena->graveyard;
}

Battle::Interface * Battle::Arena::GetInterface()
{
    assert( arena != nullptr );

    return arena->_interface.get();
}

Battle::Tower * Battle::Arena::GetTower( const TowerType type )
{
    assert( arena != nullptr );

    switch ( type ) {
    case TowerType::TWR_LEFT:
        return arena->_towers[0].get();
    case TowerType::TWR_CENTER:
        return arena->_towers[1].get();
    case TowerType::TWR_RIGHT:
        return arena->_towers[2].get();
    default:
        break;
    }
    return nullptr;
}

bool Battle::Arena::isAnyTowerPresent()
{
    assert( arena != nullptr );

    return std::any_of( arena->_towers.begin(), arena->_towers.end(), []( const auto & twr ) { return twr && twr->isValid(); } );
}

Battle::Arena::Arena( Army & army1, Army & army2, const int32_t tileIndex, const bool isShowInterface, Rand::DeterministicRandomGenerator & randomGenerator )
    : castle( world.getCastleEntrance( Maps::GetPoint( tileIndex ) ) )
    , _isTown( castle != nullptr )
    , _randomGenerator( randomGenerator )
{
    usage_spells.reserve( 20 );

    assert( arena == nullptr );
    arena = this;

    _army1 = std::make_unique<Force>( army1, false, _uidGenerator );
    _army2 = std::make_unique<Force>( army2, true, _uidGenerator );

    // If this is a siege of a town, then there is in fact no castle
    if ( castle && !castle->isCastle() ) {
        castle = nullptr;
    }

    // init interface
    if ( isShowInterface ) {
        _interface = std::make_unique<Interface>( *this, tileIndex );
        board.SetArea( _interface->GetArea() );

        _orderOfUnits = std::make_shared<Units>();
        _orderOfUnits->reserve( 25 );
        _interface->SetOrderOfUnits( _orderOfUnits );
    }
    else {
        // no interface - force auto battle mode for human player
        if ( army1.isControlHuman() ) {
            _autoBattleColors |= army1.GetColor();
        }
        if ( army2.isControlHuman() ) {
            _autoBattleColors |= army2.GetColor();
        }
    }

    if ( castle ) {
        if ( castle->isBuild( BUILD_LEFTTURRET ) ) {
            _towers[0] = std::make_unique<Tower>( *castle, TowerType::TWR_LEFT, _uidGenerator.GetUnique() );
        }

        _towers[1] = std::make_unique<Tower>( *castle, TowerType::TWR_CENTER, _uidGenerator.GetUnique() );

        if ( castle->isBuild( BUILD_RIGHTTURRET ) ) {
            _towers[2] = std::make_unique<Tower>( *castle, TowerType::TWR_RIGHT, _uidGenerator.GetUnique() );
        }

        if ( _army1->GetCommander() ) {
            _catapult = std::make_unique<Catapult>( *_army1->GetCommander() );
        }

        _bridge = std::make_unique<Bridge>();

        // catapult cell
        board[CATAPULT_POS].SetObject( 1 );

        // wall (3,2,1,0)
        const int wallObject = castle->isFortificationBuilt() ? 3 : 2;
        board[CASTLE_FIRST_TOP_WALL_POS].SetObject( wallObject );
        board[CASTLE_SECOND_TOP_WALL_POS].SetObject( wallObject );
        board[CASTLE_THIRD_TOP_WALL_POS].SetObject( wallObject );
        board[CASTLE_FOURTH_TOP_WALL_POS].SetObject( wallObject );

        // tower
        board[CASTLE_TOP_GATE_TOWER_POS].SetObject( 2 );
        board[CASTLE_BOTTOM_GATE_TOWER_POS].SetObject( 2 );

        // archers tower
        board[CASTLE_TOP_ARCHER_TOWER_POS].SetObject( 2 );
        board[CASTLE_BOTTOM_ARCHER_TOWER_POS].SetObject( 2 );

        // bridge
        board[CASTLE_GATE_POS].SetObject( 1 );
    }
    else
    // set obstacles
    {
        std::mt19937 seededGen( world.GetMapSeed() + static_cast<uint32_t>( tileIndex ) );

        _covrIcnId = Rand::GetWithGen( 0, 99, seededGen ) < 40 ? GetCovr( world.GetTiles( tileIndex ).GetGround(), seededGen ) : ICN::UNKNOWN;

        if ( _covrIcnId != ICN::UNKNOWN ) {
            board.SetCovrObjects( _covrIcnId );
        }

        board.SetCobjObjects( world.GetTiles( tileIndex ), seededGen );
    }

    AI::BattlePlanner::Get().battleBegins();

    if ( _interface ) {
        _interface->fullRedraw();

        // Wait for the end of M82::PREBATTL playback. Make sure that we check the music status first as HandleEvents() call is not instant.
        LocalEvent & le = LocalEvent::Get();
        while ( Mixer::isPlaying( -1 ) && le.HandleEvents() ) {
            if ( le.isKeyPressed( fheroes2::Key::KEY_ESCAPE ) || le.MouseClickMiddle() || le.MouseClickRight() ) {
                // Cancel waiting for M82::PREBATTL to over and start the battle.
                break;
            }
        }
    }
}

Battle::Arena::~Arena()
{
    assert( arena == this );
    arena = nullptr;
}

void Battle::Arena::UnitTurn( const Units & orderHistory )
{
    assert( _currentUnit && _currentUnit->isValid() );

    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, _currentUnit->String( true ) )

    if ( _currentUnit->isAffectedByMorale() ) {
        _currentUnit->SetRandomMorale( _randomGenerator );
    }

    assert( !_currentUnit->AllModes( MORALE_GOOD | MORALE_BAD ) );

    bool endOfTurn = false;

    while ( !endOfTurn ) {
        // There should be no dead units on the board at the beginning of each iteration
        assert( std::all_of( board.begin(), board.end(), []( const Cell & cell ) { return ( cell.GetUnit() == nullptr || cell.GetUnit()->isValid() ); } ) );

        Actions actions;

        if ( _interface ) {
            _interface->getPendingActions( actions );
        }

        if ( !actions.empty() ) {
            // Pending actions from the user interface (such as toggling auto battle) have "already occurred" and
            // therefore should be handled first, before any other actions. Just skip the rest of the branches.
        }
        else if ( _currentUnit->GetSpeed() == Speed::STANDING ) {
            // Unit has either finished its turn, is dead, or has become immovable due to some spell. Even if the
            // unit died or has become immovable during its turn without performing any action, its turn is still
            // considered completed.
            _currentUnit->SetModes( TR_MOVED );

            endOfTurn = true;
        }
        else if ( _currentUnit->Modes( MORALE_BAD ) ) {
            actions.emplace_back( Command::MORALE, _currentUnit->GetUID(), false );
        }
        else {
            // This unit will certainly perform at least one full-fledged action
            _lastActiveUnitArmyColor = _currentUnit->GetArmyColor();

            if ( _bridge ) {
                _bridge->SetPassability( *_currentUnit );
            }

            if ( ( _currentUnit->GetCurrentControl() & CONTROL_AI ) || ( _currentUnit->GetCurrentColor() & _autoBattleColors ) ) {
                AI::BattlePlanner::Get().BattleTurn( *this, *_currentUnit, actions );
            }
            else {
                assert( _interface != nullptr );

                _interface->HumanTurn( *_currentUnit, actions );
            }
        }

        const uint32_t newSeed = std::accumulate( actions.cbegin(), actions.cend(), _randomGenerator.GetSeed(),
                                                  []( const uint32_t seed, const Command & cmd ) { return cmd.updateSeed( seed ); } );
        _randomGenerator.UpdateSeed( newSeed );

        while ( !actions.empty() ) {
            ApplyAction( actions.front() );
            actions.pop_front();

            board.removeDeadUnits();

            if ( _orderOfUnits ) {
                // Applied action could kill someone or affect the speed of some unit, update the order of units
                UpdateOrderOfUnits( *_army1, *_army2, _currentUnit, GetOppositeColor( _currentUnit->GetArmyColor() ), orderHistory, *_orderOfUnits );
            }

            if ( !BattleValid() ) {
                endOfTurn = true;
                break;
            }

            if ( _currentUnit->AllModes( TR_MOVED | MORALE_GOOD ) && !_currentUnit->Modes( TR_SKIP ) && _currentUnit->GetSpeed( false, true ) > Speed::STANDING ) {
                actions.emplace_back( Command::MORALE, _currentUnit->GetUID(), true );
            }
        }

        // There should be no dead units on the board at the end of each iteration
        assert( std::all_of( board.begin(), board.end(), []( const Cell & cell ) { return ( cell.GetUnit() == nullptr || cell.GetUnit()->isValid() ); } ) );
    }
}

bool Battle::Arena::BattleValid() const
{
    return _army1->isValid() && _army2->isValid() && 0 == result_game.army1 && 0 == result_game.army2;
}

void Battle::Arena::Turns()
{
    ++_turnNumber;

    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, _turnNumber )

    if ( _interface ) {
        _interface->RedrawActionNewTurn();
    }

    _army1->NewTurn();
    _army2->NewTurn();

    // History of unit order on the current turn
    Units orderHistory;

    if ( _orderOfUnits ) {
        orderHistory.reserve( 25 );

        // Build the initial order of units
        UpdateOrderOfUnits( *_army1, *_army2, nullptr, GetOppositeColor( _lastActiveUnitArmyColor ), orderHistory, *_orderOfUnits );
    }

    {
        bool towersActed = false;
        bool catapultActed = false;

        while ( BattleValid() ) {
            // We can get the nullptr here if there are no units left waiting for their turn
            _currentUnit = GetCurrentUnit( *_army1, *_army2, GetOppositeColor( _lastActiveUnitArmyColor ) );

            if ( _orderOfUnits ) {
                // Add unit to the history
                if ( _currentUnit ) {
                    orderHistory.push_back( _currentUnit );
                }

                // Update the order of units
                UpdateOrderOfUnits( *_army1, *_army2, _currentUnit, GetOppositeColor( _currentUnit ? _currentUnit->GetArmyColor() : _lastActiveUnitArmyColor ),
                                    orderHistory, *_orderOfUnits );
            }

            if ( castle ) {
                // Catapult acts either during the turn of the first unit from the attacking army, or at the end of the
                // turn if none of the units from the attacking army are able to act (for example, all are blinded)
                if ( !catapultActed && ( _currentUnit == nullptr || _currentUnit->GetColor() == _army1->GetColor() ) ) {
                    CatapultAction();

                    catapultActed = true;
                }

                // Castle towers act either during the turn of the first unit from the defending army, or at the end of
                // the turn if none of the units from the defending army are able to act (for example, all are blinded)
                if ( !towersActed && ( _currentUnit == nullptr || _currentUnit->GetColor() == _army2->GetColor() ) ) {
                    const auto towerAction = [this, &orderHistory]( const size_t idx ) {
                        assert( idx < std::size( _towers ) );

                        if ( _towers[idx] == nullptr || !_towers[idx]->isValid() ) {
                            return;
                        }

                        TowerAction( *_towers[idx] );

                        if ( _orderOfUnits ) {
                            // Tower could kill someone, update the order of units
                            UpdateOrderOfUnits( *_army1, *_army2, _currentUnit,
                                                GetOppositeColor( _currentUnit ? _currentUnit->GetArmyColor() : _lastActiveUnitArmyColor ), orderHistory,
                                                *_orderOfUnits );
                        }
                    };

                    towerAction( 1 );
                    towerAction( 0 );
                    towerAction( 2 );

                    towersActed = true;

                    // If the towers have killed the last enemy unit, the battle is over
                    if ( !BattleValid() ) {
                        break;
                    }
                }
            }

            if ( _currentUnit == nullptr ) {
                // There are no units left waiting for their turn
                break;
            }

            UnitTurn( orderHistory );
        }
    }

    // Check if the battle is over
    if ( !_army1->isValid() || ( result_game.army1 & ( RESULT_RETREAT | RESULT_SURRENDER ) ) ) {
        result_game.army1 |= RESULT_LOSS;
        // Check if any of the original troops in the army2 are still alive
        result_game.army2 = _army2->isValid( false ) ? RESULT_WINS : RESULT_LOSS;
    }
    else if ( !_army2->isValid() || ( result_game.army2 & ( RESULT_RETREAT | RESULT_SURRENDER ) ) ) {
        result_game.army2 |= RESULT_LOSS;
        // Check if any of the original troops in the army1 are still alive
        result_game.army1 = _army1->isValid( false ) ? RESULT_WINS : RESULT_LOSS;
    }

    // If the battle is over, calculate the experience and the number of units killed
    if ( result_game.army1 || result_game.army2 ) {
        result_game.exp1 = _army2->GetDeadHitPoints();
        result_game.exp2 = _army1->GetDeadHitPoints();

        const HeroBase * army1Commander = _army1->GetCommander();
        const HeroBase * army2Commander = _army2->GetCommander();

        // Attacker (or defender) gets an experience bonus if the enemy army was under the command of a hero who was defeated (i.e. did not retreat or surrender)
        if ( army1Commander && army1Commander->isHeroes() && !( result_game.army1 & ( RESULT_RETREAT | RESULT_SURRENDER ) ) ) {
            result_game.exp2 += 500;
        }
        if ( army2Commander && army2Commander->isHeroes() && !( result_game.army2 & ( RESULT_RETREAT | RESULT_SURRENDER ) ) ) {
            result_game.exp1 += 500;
        }

        // Attacker gets an additional experience bonus after successfully besieging a town or castle
        if ( _isTown ) {
            result_game.exp1 += 500;
        }

        const Force * army_loss = ( result_game.army1 & RESULT_LOSS ? _army1.get() : ( result_game.army2 & RESULT_LOSS ? _army2.get() : nullptr ) );
        result_game.killed = army_loss ? army_loss->GetDeadCounts() : 0;
    }
}

void Battle::Arena::TowerAction( const Tower & twr )
{
    // There should be no dead units on the board at this moment
    assert( std::all_of( board.begin(), board.end(), []( const Cell & cell ) { return ( cell.GetUnit() == nullptr || cell.GetUnit()->isValid() ); } ) );

    // Target unit and its threat level
    std::pair<const Unit *, int32_t> targetInfo{ nullptr, INT32_MIN };

    for ( const Cell & cell : board ) {
        const Unit * unit = cell.GetUnit();

        if ( unit == nullptr || unit->GetColor() == twr.GetColor() || ( unit->isWide() && unit->GetTailIndex() == cell.GetIndex() ) ) {
            continue;
        }

        const int32_t unitThreatLevel = unit->evaluateThreatForUnit( twr );

        if ( targetInfo.first == nullptr || targetInfo.second < unitThreatLevel ) {
            targetInfo = { unit, unitThreatLevel };
        }
    }

    if ( targetInfo.first == nullptr ) {
        DEBUG_LOG( DBG_BATTLE, DBG_INFO, "No target found for the tower" )

        return;
    }

    using TowerGetTypeUnderlyingType = typename std::underlying_type_t<decltype( twr.GetType() )>;
    static_assert( std::is_same_v<TowerGetTypeUnderlyingType, uint8_t>, "Type of Tower::GetType() has been changed, check the logic below" );

    Command cmd( Command::TOWER, static_cast<TowerGetTypeUnderlyingType>( twr.GetType() ), targetInfo.first->GetUID() );

    ApplyAction( cmd );

    board.removeDeadUnits();
}

void Battle::Arena::CatapultAction()
{
    if ( _catapult ) {
        uint32_t shots = _catapult->GetShots();

        std::map<CastleDefenseElement, uint32_t> stateOfCatapultTargets
            = { { CastleDefenseElement::WALL1, GetCastleTargetValue( CastleDefenseElement::WALL1 ) },
                { CastleDefenseElement::WALL2, GetCastleTargetValue( CastleDefenseElement::WALL2 ) },
                { CastleDefenseElement::WALL3, GetCastleTargetValue( CastleDefenseElement::WALL3 ) },
                { CastleDefenseElement::WALL4, GetCastleTargetValue( CastleDefenseElement::WALL4 ) },
                { CastleDefenseElement::TOWER1, GetCastleTargetValue( CastleDefenseElement::TOWER1 ) },
                { CastleDefenseElement::TOWER2, GetCastleTargetValue( CastleDefenseElement::TOWER2 ) },
                { CastleDefenseElement::BRIDGE, GetCastleTargetValue( CastleDefenseElement::BRIDGE ) },
                { CastleDefenseElement::CENTRAL_TOWER, GetCastleTargetValue( CastleDefenseElement::CENTRAL_TOWER ) } };

        Command cmd( Command::CATAPULT );

        cmd << shots;

        while ( shots-- ) {
            const CastleDefenseElement target = Catapult::GetTarget( stateOfCatapultTargets, _randomGenerator );
            const uint32_t damage = std::min( _catapult->GetDamage( _randomGenerator ), stateOfCatapultTargets[target] );
            const bool hit = _catapult->IsNextShotHit( _randomGenerator );

            using TargetUnderlyingType = std::underlying_type_t<decltype( target )>;

            cmd << static_cast<TargetUnderlyingType>( target ) << damage << ( hit ? 1 : 0 );

            if ( hit ) {
                stateOfCatapultTargets[target] -= damage;
            }
        }

        // Preserve the order of shots - command arguments will be extracted in reverse order
        std::reverse( cmd.begin(), cmd.end() );

        ApplyAction( cmd );
    }
}

Battle::Indexes Battle::Arena::GetPath( const Unit & unit, const Position & position )
{
    const Indexes result = _battlePathfinder.buildPath( unit, position );

    if ( IS_DEBUG( DBG_BATTLE, DBG_TRACE ) && !result.empty() ) {
        std::string pathStr;
        pathStr.reserve( Speed::INSTANT * 4 );

        std::for_each( result.begin(), result.end(), [&pathStr]( const int32_t item ) {
            pathStr += std::to_string( item );
            pathStr += ", ";
        } );

        DEBUG_LOG( DBG_BATTLE, DBG_TRACE, pathStr )
    }

    return result;
}

Battle::Unit * Battle::Arena::GetTroopBoard( int32_t index )
{
    return Board::isValidIndex( index ) ? board[index].GetUnit() : nullptr;
}

const Battle::Unit * Battle::Arena::GetTroopBoard( int32_t index ) const
{
    return Board::isValidIndex( index ) ? board[index].GetUnit() : nullptr;
}

const HeroBase * Battle::Arena::GetCommander1() const
{
    return _army1->GetCommander();
}

const HeroBase * Battle::Arena::GetCommander2() const
{
    return _army2->GetCommander();
}

int Battle::Arena::GetArmy1Color() const
{
    return _army1->GetColor();
}

int Battle::Arena::GetArmy2Color() const
{
    return _army2->GetColor();
}

int Battle::Arena::GetCurrentColor() const
{
    // This method should never be called in cases where there may not be an active unit
    if ( _currentUnit == nullptr ) {
        assert( 0 );
        return Color::NONE;
    }

    return _currentUnit->GetCurrentOrArmyColor();
}

int Battle::Arena::GetOppositeColor( const int col ) const
{
    return col == GetArmy1Color() ? GetArmy2Color() : GetArmy1Color();
}

Battle::Unit * Battle::Arena::GetTroopUID( uint32_t uid )
{
    Units::iterator it = std::find_if( _army1->begin(), _army1->end(), [uid]( const Unit * unit ) { return unit->isUID( uid ); } );

    if ( it != _army1->end() )
        return *it;

    it = std::find_if( _army2->begin(), _army2->end(), [uid]( const Unit * unit ) { return unit->isUID( uid ); } );

    return it != _army2->end() ? *it : nullptr;
}

const Battle::Unit * Battle::Arena::GetTroopUID( uint32_t uid ) const
{
    Units::const_iterator it = std::find_if( _army1->begin(), _army1->end(), [uid]( const Unit * unit ) { return unit->isUID( uid ); } );

    if ( it != _army1->end() )
        return *it;

    it = std::find_if( _army2->begin(), _army2->end(), [uid]( const Unit * unit ) { return unit->isUID( uid ); } );

    return it != _army2->end() ? *it : nullptr;
}

void Battle::Arena::FadeArena( bool clearMessageLog ) const
{
    if ( _interface )
        _interface->FadeArena( clearMessageLog );
}

const SpellStorage & Battle::Arena::GetUsageSpells() const
{
    return usage_spells;
}

int32_t Battle::Arena::GetFreePositionNearHero( const int heroColor ) const
{
    std::vector<int> cellIds;
    if ( _army1->GetColor() == heroColor ) {
        cellIds = { 11, 22, 33 };
    }
    else if ( _army2->GetColor() == heroColor ) {
        cellIds = { 21, 32, 43 };
    }
    else {
        // Some third color?
        return -1;
    }

    assert( !cellIds.empty() );

    for ( const int cellId : cellIds ) {
        if ( board[cellId].isPassable( true ) ) {
            // TODO: remove this temporary assertion
            assert( board[cellId].GetUnit() == nullptr );

            return cellId;
        }
    }

    return -1;
}

bool Battle::Arena::CanSurrenderOpponent( int color ) const
{
    const HeroBase * hero = getCommander( color );
    const HeroBase * enemyHero = getEnemyCommander( color );
    return hero && hero->isHeroes() && enemyHero && ( enemyHero->isHeroes() || enemyHero->isCaptain() );
}

bool Battle::Arena::CanRetreatOpponent( int color ) const
{
    const HeroBase * hero = getCommander( color );
    return hero && hero->isHeroes() && ( color == _army1->GetColor() || hero->inCastle() == nullptr );
}

bool Battle::Arena::isSpellcastDisabled() const
{
    const HeroBase * hero1 = _army1->GetCommander();
    if ( hero1 != nullptr && hero1->GetBagArtifacts().isArtifactBonusPresent( fheroes2::ArtifactBonusType::DISABLE_ALL_SPELL_COMBAT_CASTING ) ) {
        return true;
    }

    const HeroBase * hero2 = _army2->GetCommander();
    if ( hero2 != nullptr && hero2->GetBagArtifacts().isArtifactBonusPresent( fheroes2::ArtifactBonusType::DISABLE_ALL_SPELL_COMBAT_CASTING ) ) {
        return true;
    }

    return false;
}

bool Battle::Arena::isDisableCastSpell( const Spell & spell, std::string * msg /* = nullptr */ ) const
{
    // check sphere negation (only for heroes)
    if ( isSpellcastDisabled() ) {
        if ( msg ) {
            *msg = _( "The Sphere of Negation artifact is in effect for this battle, disabling all combat spells." );
        }
        return true;
    }

    const HeroBase * current_commander = GetCurrentCommander();

    // check casted
    if ( current_commander ) {
        if ( current_commander->Modes( Heroes::SPELLCASTED ) ) {
            if ( msg ) {
                *msg = _( "You have already cast a spell this round." );
            }
            return true;
        }

        if ( spell == Spell::EARTHQUAKE && !castle ) {
            if ( msg ) {
                *msg = _( "That spell will have no effect!" );
            }
            return true;
        }
        else if ( spell.isSummon() ) {
            const Monster mons( spell );
            assert( mons.isValid() && mons.isElemental() );

            const Unit * elem = GetCurrentForce().FindMode( CAP_SUMMONELEM );
            if ( elem && elem->GetID() != mons.GetID() ) {
                if ( msg ) {
                    *msg = _( "You may only summon one type of elemental per combat." );
                }
                return true;
            }

            if ( 0 > GetFreePositionNearHero( GetCurrentColor() ) ) {
                if ( msg ) {
                    *msg = _( "There is no open space adjacent to your hero where you can summon an Elemental to." );
                }
                return true;
            }
        }
        else if ( spell.isValid() ) {
            // check army
            for ( Board::const_iterator it = board.begin(); it != board.end(); ++it ) {
                const Battle::Unit * b = ( *it ).GetUnit();

                if ( b ) {
                    if ( b->AllowApplySpell( spell, current_commander ) ) {
                        return false;
                    }
                }
                else {
                    // check graveyard
                    if ( GraveyardAllowResurrect( ( *it ).GetIndex(), spell ) ) {
                        return false;
                    }
                }
            }

            if ( msg ) {
                *msg = _( "That spell will have no effect!" );
            }
            return true;
        }
    }

    return false;
}

bool Battle::Arena::GraveyardAllowResurrect( const int32_t index, const Spell & spell ) const
{
    if ( !spell.isResurrect() ) {
        return false;
    }

    const HeroBase * hero = GetCurrentCommander();
    if ( hero == nullptr ) {
        return false;
    }

    const Unit * unit = GraveyardLastTroop( index );
    if ( unit == nullptr ) {
        return false;
    }

    if ( !unit->AllowApplySpell( spell, hero ) ) {
        return false;
    }

    const int headIndex = unit->GetHeadIndex();
    assert( Board::isValidIndex( headIndex ) );

    // No other unit should stand on a corpse
    if ( Board::GetCell( headIndex )->GetUnit() != nullptr ) {
        return false;
    }

    if ( !unit->isWide() ) {
        return true;
    }

    const int tailIndex = unit->GetTailIndex();
    assert( Board::isValidIndex( tailIndex ) );

    // No other unit should stand on a corpse
    if ( Board::GetCell( tailIndex )->GetUnit() != nullptr ) {
        return false;
    }

    return true;
}

const Battle::Unit * Battle::Arena::GraveyardLastTroop( const int32_t index ) const
{
    return GetTroopUID( graveyard.GetLastTroopUID( index ) );
}

std::vector<const Battle::Unit *> Battle::Arena::GetGraveyardTroops( const int32_t index ) const
{
    const TroopUIDs troopUIDs = graveyard.GetTroopUIDs( index );

    std::vector<const Unit *> result;
    result.reserve( troopUIDs.size() );

    std::for_each( troopUIDs.begin(), troopUIDs.end(), [this, &result]( const uint32_t uid ) {
        const Unit * unit = GetTroopUID( uid );
        assert( unit != nullptr );

        result.push_back( unit );
    } );

    return result;
}

Battle::Indexes Battle::Arena::GraveyardOccupiedCells() const
{
    return graveyard.GetOccupiedCells();
}

void Battle::Arena::SetCastleTargetValue( const CastleDefenseElement target, const uint32_t value )
{
    switch ( target ) {
    case CastleDefenseElement::WALL1:
        board[CASTLE_FIRST_TOP_WALL_POS].SetObject( value );
        break;
    case CastleDefenseElement::WALL2:
        board[CASTLE_SECOND_TOP_WALL_POS].SetObject( value );
        break;
    case CastleDefenseElement::WALL3:
        board[CASTLE_THIRD_TOP_WALL_POS].SetObject( value );
        break;
    case CastleDefenseElement::WALL4:
        board[CASTLE_FOURTH_TOP_WALL_POS].SetObject( value );
        break;

    case CastleDefenseElement::TOWER1:
        if ( _towers[0] && _towers[0]->isValid() ) {
            _towers[0]->SetDestroy();
        }
        break;
    case CastleDefenseElement::TOWER2:
        if ( _towers[2] && _towers[2]->isValid() ) {
            _towers[2]->SetDestroy();
        }
        break;
    case CastleDefenseElement::CENTRAL_TOWER:
        if ( _towers[1] && _towers[1]->isValid() ) {
            _towers[1]->SetDestroy();
        }
        break;

    case CastleDefenseElement::BRIDGE:
        if ( _bridge->isValid() ) {
            _bridge->SetDestroyed();
        }
        break;

    default:
        break;
    }
}

uint32_t Battle::Arena::GetCastleTargetValue( const CastleDefenseElement target ) const
{
    switch ( target ) {
    case CastleDefenseElement::WALL1:
        return board[CASTLE_FIRST_TOP_WALL_POS].GetObject();
    case CastleDefenseElement::WALL2:
        return board[CASTLE_SECOND_TOP_WALL_POS].GetObject();
    case CastleDefenseElement::WALL3:
        return board[CASTLE_THIRD_TOP_WALL_POS].GetObject();
    case CastleDefenseElement::WALL4:
        return board[CASTLE_FOURTH_TOP_WALL_POS].GetObject();

    case CastleDefenseElement::TOWER1:
        return _towers[0] && _towers[0]->isValid() ? 1 : 0;
    case CastleDefenseElement::TOWER2:
        return _towers[2] && _towers[2]->isValid() ? 1 : 0;
    case CastleDefenseElement::CENTRAL_TOWER:
        return _towers[1] && _towers[1]->isValid() ? 1 : 0;

    case CastleDefenseElement::BRIDGE:
        return _bridge->isValid() ? 1 : 0;

    default:
        break;
    }
    return 0;
}

std::vector<Battle::CastleDefenseElement> Battle::Arena::GetEarthQuakeTargets() const
{
    std::vector<CastleDefenseElement> targets;
    targets.reserve( 8 );

    if ( board[CASTLE_FIRST_TOP_WALL_POS].GetObject() > 0 ) {
        targets.push_back( CastleDefenseElement::WALL1 );
    }
    if ( board[CASTLE_SECOND_TOP_WALL_POS].GetObject() > 0 ) {
        targets.push_back( CastleDefenseElement::WALL2 );
    }
    if ( board[CASTLE_THIRD_TOP_WALL_POS].GetObject() > 0 ) {
        targets.push_back( CastleDefenseElement::WALL3 );
    }
    if ( board[CASTLE_FOURTH_TOP_WALL_POS].GetObject() > 0 ) {
        targets.push_back( CastleDefenseElement::WALL4 );
    }

    if ( _towers[0] && _towers[0]->isValid() ) {
        targets.push_back( CastleDefenseElement::TOWER1 );
    }
    if ( _towers[2] && _towers[2]->isValid() ) {
        targets.push_back( CastleDefenseElement::TOWER2 );
    }

    return targets;
}

const HeroBase * Battle::Arena::getCommander( const int color ) const
{
    return ( _army1->GetColor() == color ) ? _army1->GetCommander() : _army2->GetCommander();
}

const HeroBase * Battle::Arena::getEnemyCommander( const int color ) const
{
    return ( _army1->GetColor() == color ) ? _army2->GetCommander() : _army1->GetCommander();
}

const HeroBase * Battle::Arena::GetCurrentCommander() const
{
    return getCommander( GetCurrentColor() );
}

Battle::Unit * Battle::Arena::CreateElemental( const Spell & spell )
{
    // TODO: this assertion is here to thoroughly check all the complex limitations of the Summon Elemental spell
    assert( !isDisableCastSpell( spell ) );

    const HeroBase * hero = GetCurrentCommander();
    assert( hero != nullptr );

    const int32_t idx = GetFreePositionNearHero( GetCurrentColor() );
    assert( Board::isValidIndex( idx ) );

    const Monster mons( spell );
    assert( mons.isValid() && mons.isElemental() && !mons.isWide() );

    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, mons.GetName() << ", position: " << idx )

    const bool reflect = ( hero == _army2->GetCommander() );
    const uint32_t count = fheroes2::getSummonMonsterCount( spell, hero->GetPower(), hero );

    Position pos;
    pos.Set( idx, mons.isWide(), reflect );

    // An elemental could not be a wide unit
    assert( pos.GetHead() != nullptr && pos.GetTail() == nullptr );

    Unit * elem = new Unit( Troop( mons, count ), pos, reflect, _uidGenerator.GetUnique() );

    elem->SetModes( CAP_SUMMONELEM );
    elem->SetArmy( hero->GetArmy() );

    GetCurrentForce().push_back( elem );

    return elem;
}

Battle::Unit * Battle::Arena::CreateMirrorImage( Unit & unit )
{
    Unit * mirrorUnit = new Unit( unit, {}, unit.isReflect(), _uidGenerator.GetUnique() );

    mirrorUnit->SetArmy( *unit.GetArmy() );
    mirrorUnit->SetMirror( &unit );
    mirrorUnit->SetModes( CAP_MIRRORIMAGE );

    unit.SetMirror( mirrorUnit );
    unit.SetModes( CAP_MIRROROWNER );

    GetCurrentForce().push_back( mirrorUnit );

    return mirrorUnit;
}

bool Battle::Arena::IsShootingPenalty( const Unit & attacker, const Unit & defender ) const
{
    // no castle - no castle walls, penalty does not apply
    if ( castle == nullptr ) {
        return false;
    }

    // penalty does not apply to towers
    if ( defender.Modes( CAP_TOWER ) || attacker.Modes( CAP_TOWER ) )
        return false;

    // penalty does not apply if the attacker's hero has certain artifacts or skills
    const HeroBase * hero = attacker.GetCommander();
    if ( hero ) {
        // golden bow artifact
        if ( hero->GetBagArtifacts().isArtifactBonusPresent( fheroes2::ArtifactBonusType::NO_SHOOTING_PENALTY ) )
            return false;

        // archery skill
        if ( hero->GetLevelSkill( Skill::Secondary::ARCHERY ) != Skill::Level::NONE )
            return false;
    }

    // penalty does not apply if the attacking unit (be it a castle attacker or a castle defender) is inside the castle walls
    if ( !attacker.isOutOfCastleWalls() ) {
        return false;
    }

    // penalty does not apply if both units are on the same side relative to the castle walls
    if ( attacker.isOutOfCastleWalls() == defender.isOutOfCastleWalls() ) {
        return false;
    }

    // penalty does not apply if the target unit is exposed due to the broken castle wall
    const std::vector<fheroes2::Point> points = GetLinePoints( attacker.GetBackPoint(), defender.GetBackPoint(), CELLW / 3 );

    for ( std::vector<fheroes2::Point>::const_iterator it = points.begin(); it != points.end(); ++it ) {
        if ( ( 0 == board[CASTLE_FIRST_TOP_WALL_POS].GetObject() && ( board[CASTLE_FIRST_TOP_WALL_POS].GetPos() & *it ) )
             || ( 0 == board[CASTLE_SECOND_TOP_WALL_POS].GetObject() && ( board[CASTLE_SECOND_TOP_WALL_POS].GetPos() & *it ) )
             || ( 0 == board[CASTLE_THIRD_TOP_WALL_POS].GetObject() && ( board[CASTLE_THIRD_TOP_WALL_POS].GetPos() & *it ) )
             || ( 0 == board[CASTLE_FOURTH_TOP_WALL_POS].GetObject() && ( board[CASTLE_FOURTH_TOP_WALL_POS].GetPos() & *it ) ) )
            return false;
    }

    return true;
}

Battle::Force & Battle::Arena::GetForce1() const
{
    return *_army1;
}

Battle::Force & Battle::Arena::GetForce2() const
{
    return *_army2;
}

Battle::Force & Battle::Arena::getForce( const int color ) const
{
    return ( _army1->GetColor() == color ) ? *_army1 : *_army2;
}

Battle::Force & Battle::Arena::getEnemyForce( const int color ) const
{
    return ( _army1->GetColor() == color ) ? *_army2 : *_army1;
}

Battle::Force & Battle::Arena::GetCurrentForce() const
{
    return getForce( GetCurrentColor() );
}

Battle::Result & Battle::Arena::GetResult()
{
    return result_game;
}

bool Battle::Arena::AutoBattleInProgress() const
{
    if ( _currentUnit == nullptr ) {
        return false;
    }

    if ( _autoBattleColors & GetCurrentColor() ) {
        // Auto battle mode cannot be enabled for a player controlled by AI
        assert( !( GetCurrentForce().GetControl() & CONTROL_AI ) );

        return true;
    }

    return false;
}

bool Battle::Arena::EnemyOfAIHasAutoBattleInProgress() const
{
    if ( _currentUnit == nullptr ) {
        return false;
    }

    if ( !( GetCurrentForce().GetControl() & CONTROL_AI ) ) {
        return false;
    }

    const Force & enemyForce = getEnemyForce( GetCurrentColor() );

    if ( enemyForce.GetControl() & CONTROL_AI ) {
        return false;
    }

    return ( _autoBattleColors & enemyForce.GetColor() );
}

bool Battle::Arena::CanToggleAutoBattle() const
{
    if ( _currentUnit == nullptr ) {
        return false;
    }

    return !( GetCurrentForce().GetControl() & CONTROL_AI );
}
