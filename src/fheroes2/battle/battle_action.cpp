/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
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

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <ostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "battle.h"
#include "battle_arena.h"
#include "battle_army.h"
#include "battle_board.h"
#include "battle_bridge.h"
#include "battle_cell.h"
#include "battle_command.h"
#include "battle_grave.h"
#include "battle_interface.h"
#include "battle_tower.h"
#include "battle_troop.h"
#include "color.h"
#include "heroes.h"
#include "heroes_base.h"
#include "kingdom.h"
#include "logging.h"
#include "monster.h"
#include "monster_info.h"
#include "players.h"
#include "rand.h"
#include "resource.h"
#include "spell.h"
#include "spell_storage.h"
#include "tools.h"
#include "translations.h"
#include "world.h"

namespace
{
    std::pair<uint32_t, uint32_t> getEarthquakeDamageRange( const HeroBase * commander )
    {
        const int spellPower = commander->GetPower();
        if ( ( spellPower > 0 ) && ( spellPower < 3 ) ) {
            return { 0, 1 };
        }
        else if ( ( spellPower >= 3 ) && ( spellPower < 6 ) ) {
            return { 0, 2 };
        }
        else if ( ( spellPower >= 6 ) && ( spellPower < 10 ) ) {
            return { 0, 3 };
        }
        else if ( spellPower >= 10 ) {
            return { 1, 3 };
        }

        DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "unexpected spellPower value: " << spellPower << " for commander " << commander )
        return { 0, 0 };
    }
}

void Battle::Arena::BattleProcess( Unit & attacker, Unit & defender, int32_t dst /* = -1 */, int dir /* = -1 */ )
{
    auto calculateDst = []( const Unit & attackingUnit, const Unit & defendingUnit ) {
        // The defender's head cell is near the attacker's head cell
        if ( Board::isNearIndexes( attackingUnit.GetHeadIndex(), defendingUnit.GetHeadIndex() ) ) {
            return defendingUnit.GetHeadIndex();
        }
        // The defender's tail cell is near the attacker's head cell
        if ( defendingUnit.isWide() && Board::isNearIndexes( attackingUnit.GetHeadIndex(), defendingUnit.GetTailIndex() ) ) {
            return defendingUnit.GetTailIndex();
        }
        // The defender's head cell is near the attacker's tail cell
        if ( attackingUnit.isWide() && Board::isNearIndexes( attackingUnit.GetTailIndex(), defendingUnit.GetHeadIndex() ) ) {
            return defendingUnit.GetHeadIndex();
        }
        // The defender's tail cell is near the attacker's tail cell
        if ( attackingUnit.isWide() && defendingUnit.isWide() && Board::isNearIndexes( attackingUnit.GetTailIndex(), defendingUnit.GetTailIndex() ) ) {
            return defendingUnit.GetTailIndex();
        }
        // Units don't stand next to each other, this is most likely a shot
        return defendingUnit.GetHeadIndex();
    };

    auto calculateDir = []( const Unit & attackingUnit, const int32_t attackDst ) -> int {
        // The target cell of the attack is near the attacker's head cell
        if ( Board::isNearIndexes( attackingUnit.GetHeadIndex(), attackDst ) ) {
            return Board::GetDirection( attackingUnit.GetHeadIndex(), attackDst );
        }
        // The target cell of the attack is near the attacker's tail cell
        if ( attackingUnit.isWide() && Board::isNearIndexes( attackingUnit.GetTailIndex(), attackDst ) ) {
            return Board::GetDirection( attackingUnit.GetTailIndex(), attackDst );
        }
        // Units don't stand next to each other, this is most likely a shot
        return UNKNOWN;
    };

    if ( dst < 0 ) {
        dst = calculateDst( attacker, defender );
    }

    if ( dir < 0 ) {
        dir = calculateDir( attacker, dst );
    }

    // UNKNOWN attack direction is only allowed for archers
    assert( Unit::isHandFighting( attacker, defender ) ? dir > UNKNOWN : dir == UNKNOWN );

    // This is a direct attack, update the direction for both the attacker and the defender
    if ( dir ) {
        auto directionIsValidForAttack = []( const Unit & attackingUnit, const int32_t attackDst, const int attackDir ) {
            assert( attackingUnit.isWide() );

            const int32_t attackSrc = Board::GetIndexDirection( attackDst, Board::GetReflectDirection( attackDir ) );
            // Attacker should attack either from his head cell or from his tail cell, otherwise something strange happens
            assert( attackSrc == attackingUnit.GetHeadIndex() || attackSrc == attackingUnit.GetTailIndex() );

            return attackSrc == attackingUnit.GetHeadIndex();
        };

        if ( attacker.isWide() ) {
            if ( !directionIsValidForAttack( attacker, dst, dir ) ) {
                attacker.SetReflection( !attacker.isReflect() );
            }
        }
        else {
            attacker.UpdateDirection( board[dst].GetPos() );
        }

        if ( !attacker.ignoreRetaliation() && defender.AllowResponse() ) {
            const int32_t responseDst = calculateDst( defender, attacker );
            const int responseDir = calculateDir( defender, responseDst );

            if ( defender.isWide() ) {
                if ( !directionIsValidForAttack( defender, responseDst, responseDir ) ) {
                    defender.SetReflection( !defender.isReflect() );
                }
            }
            else {
                defender.UpdateDirection( board[responseDst].GetPos() );
            }
        }
    }
    // This is a shot, update the direction for the attacker only
    else {
        // For shooters we get the target position (not the 'dst') to take into account the wide units.
        attacker.UpdateDirection( defender.GetRectPosition() );
    }

    // Update the attacker's luck right before the attack
    attacker.SetRandomLuck();

    // Do damage first
    TargetsInfo attackTargets = GetTargetsForDamage( attacker, defender, dst, dir );

    if ( _interface ) {
        _interface->RedrawActionAttackPart1( attacker, defender, attackTargets );
    }

    uint32_t resurrected = 0;
    TargetsApplyDamage( attacker, attackTargets, resurrected );

    if ( _interface ) {
        _interface->RedrawActionAttackPart2( attacker, defender, attackTargets, resurrected );
    }

    // Then apply the attacker's built-in spell
    const Spell spell = attacker.GetSpellMagic();

    if ( spell.isValid() ) {
        // Only single target spells and special built-in only spells are allowed
        assert( spell.isSingleTarget() || spell.isBuiltinOnly() );

        TargetsInfo spellTargets;
        spellTargets.reserve( attackTargets.size() );

        // Filter out invalid targets and targets to which the spell cannot be applied
        for ( const TargetInfo & attackTarget : attackTargets ) {
            assert( attackTarget.defender != nullptr );

            if ( !attackTarget.defender->isValid() ) {
                continue;
            }
            if ( !attackTarget.defender->AllowApplySpell( spell, attackTarget.defender->GetCommander(), nullptr, true ) ) {
                continue;
            }

            spellTargets.emplace_back( attackTarget.defender );
        }

        if ( !spellTargets.empty() ) {
            // The built-in spell can only be applied to one target. If there are multiple
            // targets eligible for this spell, then we should randomly select only one.
            if ( spellTargets.size() > 1 ) {
                const Unit * selectedUnit = _randomGenerator.Get( spellTargets ).defender;

                spellTargets.erase( std::remove_if( spellTargets.begin(), spellTargets.end(),
                                                    [selectedUnit]( const TargetInfo & v ) { return v.defender != selectedUnit; } ),
                                    spellTargets.end() );
            }

            assert( spellTargets.size() == 1 );

            Unit * spellTargetUnit = spellTargets.front().defender;
            assert( spellTargetUnit != nullptr );

            // The built-in dispel should only remove beneficial spells from the target unit
            if ( spell.GetID() != Spell::DISPEL || spellTargetUnit->Modes( IS_GOOD_MAGIC ) ) {
                if ( _interface ) {
                    _interface->RedrawActionSpellCastStatus( spell, spellTargetUnit->GetHeadIndex(), attacker.GetName(), spellTargets );
                    _interface->RedrawActionSpellCastPart1( spell, spellTargetUnit->GetHeadIndex(), nullptr, spellTargets );
                }

                if ( spell.GetID() == Spell::DISPEL ) {
                    spellTargetUnit->removeAffection( IS_GOOD_MAGIC );
                }
                else {
                    // The unit's built-in spell efficiency does not depend on its commanding hero's skills
                    TargetsApplySpell( nullptr, spell, spellTargets );
                }

                if ( _interface ) {
                    _interface->RedrawActionSpellCastPart2( spell, spellTargets );
                    _interface->RedrawActionMonsterSpellCastStatus( spell, attacker, spellTargets.front() );
                }
            }
        }
    }

    attacker.PostAttackAction();
}

void Battle::Arena::ApplyAction( Command & cmd )
{
    switch ( cmd.GetType() ) {
    case CommandType::MSG_BATTLE_CAST:
        ApplyActionSpellCast( cmd );
        break;
    case CommandType::MSG_BATTLE_ATTACK:
        ApplyActionAttack( cmd );
        break;
    case CommandType::MSG_BATTLE_MOVE:
        ApplyActionMove( cmd );
        break;
    case CommandType::MSG_BATTLE_SKIP:
        ApplyActionSkip( cmd );
        break;
    case CommandType::MSG_BATTLE_END_TURN:
        ApplyActionEnd( cmd );
        break;
    case CommandType::MSG_BATTLE_MORALE:
        ApplyActionMorale( cmd );
        break;

    case CommandType::MSG_BATTLE_TOWER:
        ApplyActionTower( cmd );
        break;
    case CommandType::MSG_BATTLE_CATAPULT:
        ApplyActionCatapult( cmd );
        break;

    case CommandType::MSG_BATTLE_RETREAT:
        ApplyActionRetreat( cmd );
        break;
    case CommandType::MSG_BATTLE_SURRENDER:
        ApplyActionSurrender( cmd );
        break;

    case CommandType::MSG_BATTLE_AUTO_SWITCH:
        ApplyActionAutoSwitch( cmd );
        break;
    case CommandType::MSG_BATTLE_AUTO_FINISH:
        ApplyActionAutoFinish( cmd );
        break;

    default:
        break;
    }
}

void Battle::Arena::ApplyActionSpellCast( Command & cmd )
{
    const Spell spell( cmd.GetValue() );

    HeroBase * commander = GetCurrentForce().GetCommander();

    if ( spell.isCombat() && !isDisableCastSpell( spell ) && commander && commander->CanCastSpell( spell ) ) {
        DEBUG_LOG( DBG_BATTLE, DBG_TRACE, commander->GetName() << ", color: " << Color::String( commander->GetColor() ) << ", spell: " << spell.GetName() )

        // uniq spells action
        switch ( spell.GetID() ) {
        case Spell::TELEPORT:
            ApplyActionSpellTeleport( cmd );
            break;

        case Spell::EARTHQUAKE:
            ApplyActionSpellEarthQuake( cmd );
            break;

        case Spell::MIRRORIMAGE:
            ApplyActionSpellMirrorImage( cmd );
            break;

        case Spell::SUMMONEELEMENT:
        case Spell::SUMMONAELEMENT:
        case Spell::SUMMONFELEMENT:
        case Spell::SUMMONWELEMENT:
            ApplyActionSpellSummonElemental( cmd, spell );
            break;

        default:
            ApplyActionSpellDefaults( cmd, spell );
            break;
        }

        commander->SetModes( Heroes::SPELLCASTED );
        commander->SpellCasted( spell );

        // save spell for "eagle eye" capability
        usage_spells.Append( spell );
    }
    else {
        DEBUG_LOG( DBG_BATTLE, DBG_WARN, "incorrect param: " << spell.GetName() )
    }
}

void Battle::Arena::ApplyActionAttack( Command & cmd )
{
    const uint32_t attackerUID = cmd.GetValue();
    const uint32_t defenderUID = cmd.GetValue();
    const int32_t dst = cmd.GetValue();
    const int32_t dir = cmd.GetValue();

    Unit * attacker = GetTroopUID( attackerUID );
    Unit * defender = GetTroopUID( defenderUID );

    if ( attacker && attacker->isValid() && defender && defender->isValid() && ( attacker->GetCurrentColor() != defender->GetColor() ) ) {
        DEBUG_LOG( DBG_BATTLE, DBG_TRACE, attacker->String() << " to " << defender->String() )

        const bool handfighting = Unit::isHandFighting( *attacker, *defender );
        const bool doubleAttack = attacker->isDoubleAttack();

        if ( attacker->isArchers() || handfighting ) {
            defender->SetBlindAnswer( defender->Modes( SP_BLIND ) );

            BattleProcess( *attacker, *defender, dst, dir );

            if ( defender->isValid() ) {
                if ( handfighting && !attacker->ignoreRetaliation() && defender->AllowResponse() ) {
                    BattleProcess( *defender, *attacker );
                    defender->SetResponse();
                }

                defender->SetBlindAnswer( false );

                if ( doubleAttack && attacker->isValid() && !attacker->Modes( SP_BLIND | IS_PARALYZE_MAGIC ) ) {
                    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "double attack" )
                    BattleProcess( *attacker, *defender, dst, dir );
                }
            }

            // Reflect attacker only if he is alive.
            if ( attacker->isValid() ) {
                attacker->UpdateDirection();
            }

            // Reflect defender only if he is alive.
            if ( defender->isValid() ) {
                defender->UpdateDirection();
            }
        }
        else {
            DEBUG_LOG( DBG_BATTLE, DBG_WARN, "incorrect param: " << attacker->String( true ) << " and " << defender->String( true ) )
        }
    }
    else {
        DEBUG_LOG( DBG_BATTLE, DBG_WARN,
                   "incorrect param: "
                       << "uid: " << GetHexString( attackerUID ) << ", uid: " << GetHexString( defenderUID ) )
    }
}

void Battle::Arena::ApplyActionMove( Command & cmd )
{
    const uint32_t uid = cmd.GetValue();
    const int32_t dst = cmd.GetValue();

    Unit * unit = GetTroopUID( uid );
    const Cell * cell = Board::GetCell( dst );

    if ( unit && unit->isValid() && cell && cell->isPassableForUnit( *unit ) ) {
        const int32_t initialHead = unit->GetHeadIndex();

        Position pos = Position::GetPosition( *unit, dst );
        assert( pos.GetHead() != nullptr && ( !unit->isWide() || pos.GetTail() != nullptr ) );

        DEBUG_LOG( DBG_BATTLE, DBG_TRACE,
                   unit->String() << ", dst: " << dst << ", (head: " << pos.GetHead()->GetIndex() << ", tail: " << ( unit->isWide() ? pos.GetTail()->GetIndex() : -1 )
                                  << ")" )

        Position finalPos;

        if ( unit->isFlying() ) {
            unit->UpdateDirection( pos.GetRect() );

            if ( unit->isReflect() != pos.isReflect() ) {
                pos.Swap();
            }

            if ( _interface ) {
                _interface->RedrawActionFly( *unit, pos );
            }
            else if ( _bridge ) {
                const int32_t dstHead = pos.GetHead()->GetIndex();
                const int32_t dstTail = unit->isWide() ? pos.GetTail()->GetIndex() : -1;

                // Lower the bridge if the unit needs to land on it
                if ( _bridge->NeedDown( *unit, dstHead ) || ( unit->isWide() && _bridge->NeedDown( *unit, dstTail ) ) ) {
                    _bridge->ActionDown();
                }

                unit->SetPosition( pos );

                // Raise the bridge if possible after the unit has completed its movement
                if ( _bridge->AllowUp() ) {
                    _bridge->ActionUp();
                }
            }

            finalPos = pos;
        }
        else {
            const Indexes path = GetPath( *unit, pos );

            if ( path.empty() ) {
                DEBUG_LOG( DBG_BATTLE, DBG_WARN,
                           "path is empty: " << unit->String() << " to "
                                             << "dst: " << dst )
                return;
            }

            if ( _interface ) {
                _interface->RedrawActionMove( *unit, path );
            }
            else if ( _bridge ) {
                for ( const int32_t idx : path ) {
                    if ( _bridge->NeedDown( *unit, idx ) ) {
                        _bridge->ActionDown();
                    }

                    if ( unit->isWide() && unit->GetTailIndex() == idx ) {
                        unit->SetReflection( !unit->isReflect() );
                    }
                    else {
                        unit->SetPosition( idx );
                    }

                    if ( _bridge->AllowUp() ) {
                        _bridge->ActionUp();
                    }
                }
            }

            if ( unit->isWide() ) {
                const int32_t dstHead = path.back();
                const int32_t dstTail = path.size() > 1 ? path[path.size() - 2] : initialHead;

                finalPos.Set( dstHead, true, ( Board::GetDirection( dstHead, dstTail ) & RIGHT_SIDE ) != 0 );
            }
            else {
                finalPos.Set( path.back(), false, unit->isReflect() );
            }
        }

        unit->SetPosition( finalPos );
        unit->UpdateDirection();
    }
    else {
        DEBUG_LOG( DBG_BATTLE, DBG_WARN,
                   "incorrect param: "
                       << "uid: " << GetHexString( uid ) << ", dst: " << dst )
    }
}

void Battle::Arena::ApplyActionSkip( Command & cmd )
{
    const uint32_t uid = cmd.GetValue();

    Unit * unit = GetTroopUID( uid );

    if ( unit && unit->isValid() ) {
        if ( !unit->Modes( TR_MOVED ) ) {
            unit->SetModes( TR_SKIP );
            unit->SetModes( TR_MOVED );

            if ( _interface ) {
                _interface->RedrawActionSkipStatus( *unit );
            }

            DEBUG_LOG( DBG_BATTLE, DBG_TRACE, unit->String() )
        }
        else {
            DEBUG_LOG( DBG_BATTLE, DBG_WARN, "unit has already completed its turn: " << unit->String() )
        }
    }
    else {
        DEBUG_LOG( DBG_BATTLE, DBG_WARN,
                   "incorrect param: "
                       << "uid: " << GetHexString( uid ) )
    }
}

void Battle::Arena::ApplyActionEnd( Command & cmd )
{
    const uint32_t uid = cmd.GetValue();

    Unit * unit = GetTroopUID( uid );

    if ( unit ) {
        if ( !unit->Modes( TR_MOVED ) ) {
            unit->SetModes( TR_MOVED );

            DEBUG_LOG( DBG_BATTLE, DBG_TRACE, unit->String() )
        }
        else {
            DEBUG_LOG( DBG_BATTLE, DBG_INFO, "unit has already completed its turn: " << unit->String() )
        }
    }
    else {
        DEBUG_LOG( DBG_BATTLE, DBG_WARN,
                   "incorrect param: "
                       << "uid: " << GetHexString( uid ) )
    }
}

void Battle::Arena::ApplyActionMorale( Command & cmd )
{
    const uint32_t uid = cmd.GetValue();
    const int32_t morale = cmd.GetValue();

    Unit * unit = GetTroopUID( uid );

    if ( unit == nullptr || !unit->isValid() ) {
        DEBUG_LOG( DBG_BATTLE, DBG_WARN,
                   "incorrect param: "
                       << "uid: " << GetHexString( uid ) )

        return;
    }

    // Good morale
    if ( morale ) {
        if ( !unit->AllModes( TR_MOVED | MORALE_GOOD ) ) {
            DEBUG_LOG( DBG_BATTLE, DBG_WARN, "unit is in an invalid state: " << unit->String( true ) )

            return;
        }

        unit->ResetModes( TR_MOVED | MORALE_GOOD );
    }
    // Bad morale
    else {
        if ( !unit->Modes( MORALE_BAD ) || unit->Modes( TR_MOVED ) ) {
            DEBUG_LOG( DBG_BATTLE, DBG_WARN, "unit is in an invalid state: " << unit->String( true ) )

            return;
        }

        unit->ResetModes( MORALE_BAD );
        unit->SetModes( TR_MOVED );
    }

    if ( _interface ) {
        _interface->RedrawActionMorale( *unit, morale != 0 );
    }

    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, ( morale ? "good" : "bad" ) << " to " << unit->String() )
}

void Battle::Arena::ApplyActionRetreat( const Command & /*cmd*/ )
{
    if ( CanRetreatOpponent( current_color ) ) {
        if ( _army1->GetColor() == current_color ) {
            result_game.army1 = RESULT_RETREAT;
        }
        else if ( _army2->GetColor() == current_color ) {
            result_game.army2 = RESULT_RETREAT;
        }
        DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "color: " << Color::String( current_color ) )
    }
    else {
        DEBUG_LOG( DBG_BATTLE, DBG_WARN, "CanRetreatOpponent check failed" )
    }
}

void Battle::Arena::ApplyActionSurrender( const Command & /*cmd*/ )
{
    if ( CanSurrenderOpponent( current_color ) ) {
        Funds cost;

        if ( _army1->GetColor() == current_color )
            cost.gold = _army1->GetSurrenderCost();
        else if ( _army2->GetColor() == current_color )
            cost.gold = _army2->GetSurrenderCost();

        if ( world.GetKingdom( current_color ).AllowPayment( cost ) ) {
            if ( _army1->GetColor() == current_color ) {
                result_game.army1 = RESULT_SURRENDER;
                world.GetKingdom( current_color ).OddFundsResource( cost );
                world.GetKingdom( _army2->GetColor() ).AddFundsResource( cost );
            }
            else if ( _army2->GetColor() == current_color ) {
                result_game.army2 = RESULT_SURRENDER;
                world.GetKingdom( current_color ).OddFundsResource( cost );
                world.GetKingdom( _army1->GetColor() ).AddFundsResource( cost );
            }
            DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "color: " << Color::String( current_color ) )
        }
    }
    else {
        DEBUG_LOG( DBG_BATTLE, DBG_WARN, "incorrect param" )
    }
}

void Battle::Arena::TargetsApplyDamage( Unit & attacker, TargetsInfo & targets, uint32_t & resurrected )
{
    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "number of targets: " << targets.size() )

    for ( TargetInfo & target : targets ) {
        assert( target.defender != nullptr );

        uint32_t resurrectIncrease = 0;
        target.defender->ApplyDamage( attacker, target.damage, target.killed, &resurrectIncrease );
        resurrected += resurrectIncrease;
    }
}

Battle::TargetsInfo Battle::Arena::GetTargetsForDamage( const Unit & attacker, Unit & defender, const int32_t dst, const int dir ) const
{
    // The attacked unit should be located on the attacked cell
    assert( defender.GetHeadIndex() == dst || defender.GetTailIndex() == dst );

    TargetsInfo targets;
    targets.reserve( 8 );

    TargetInfo res;

    // first target
    res.defender = &defender;
    res.damage = attacker.GetDamage( defender );

    // Genie special attack
    if ( attacker.GetID() == Monster::GENIE && _randomGenerator.Get( 1, 10 ) == 2 && defender.GetHitPoints() / 2 > res.damage ) {
        // Replaces the damage, not adding to it
        if ( defender.GetCount() == 1 ) {
            res.damage = defender.GetHitPoints();
        }
        else {
            res.damage = defender.GetHitPoints() / 2;
        }

        if ( Arena::GetInterface() ) {
            std::string str( _n( "%{name} destroys half the enemy troops!", "%{name} destroy half the enemy troops!", attacker.GetCount() ) );
            StringReplace( str, "%{name}", attacker.GetName() );
            Arena::GetInterface()->SetStatus( str, true );
        }
    }

    targets.push_back( res );

    std::set<const Unit *> consideredTargets{ &defender };

    // long distance attack
    if ( attacker.isDoubleCellAttack() ) {
        Cell * cell = Board::GetCell( dst, dir );
        Unit * enemy = cell ? cell->GetUnit() : nullptr;

        if ( enemy && consideredTargets.insert( enemy ).second ) {
            res.defender = enemy;
            res.damage = attacker.GetDamage( *enemy );

            targets.push_back( res );
        }
    }
    // attack of all adjacent cells
    else if ( attacker.isAllAdjacentCellsAttack() ) {
        for ( const int32_t nearbyIdx : Board::GetAroundIndexes( attacker ) ) {
            assert( Board::GetCell( nearbyIdx ) != nullptr );

            Unit * enemy = Board::GetCell( nearbyIdx )->GetUnit();

            if ( enemy && enemy->GetColor() != attacker.GetCurrentColor() && consideredTargets.insert( enemy ).second ) {
                res.defender = enemy;
                res.damage = attacker.GetDamage( *enemy );

                targets.push_back( res );
            }
        }
    }
    // lich cloud damage
    else if ( attacker.isAbilityPresent( fheroes2::MonsterAbilityType::AREA_SHOT ) && !attacker.isHandFighting() ) {
        for ( const int32_t nearbyIdx : Board::GetAroundIndexes( dst ) ) {
            assert( Board::GetCell( nearbyIdx ) != nullptr );

            Unit * enemy = Board::GetCell( nearbyIdx )->GetUnit();

            if ( enemy && consideredTargets.insert( enemy ).second ) {
                res.defender = enemy;
                res.damage = attacker.GetDamage( *enemy );

                targets.push_back( res );
            }
        }
    }

    return targets;
}

void Battle::Arena::TargetsApplySpell( const HeroBase * hero, const Spell & spell, TargetsInfo & targets )
{
    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "number of targets: " << targets.size() )

    for ( TargetInfo & target : targets ) {
        assert( target.defender != nullptr );

        target.defender->ApplySpell( spell, hero, target );
    }
}

std::vector<Battle::Unit *> Battle::Arena::FindChainLightningTargetIndexes( const HeroBase * hero, Unit * firstUnit )
{
    std::vector<Unit *> result = { firstUnit };
    std::vector<Unit *> ignoredTroops = { firstUnit };

    std::vector<Unit *> foundTroops = board.GetNearestTroops( result.back(), ignoredTroops );

    const int heroSpellPower = hero ? hero->GetPower() : 0;

    // Filter those which are fully immuned
    for ( size_t i = 0; i < foundTroops.size(); ) {
        if ( foundTroops[i]->GetMagicResist( Spell::CHAINLIGHTNING, heroSpellPower, hero ) >= 100 ) {
            ignoredTroops.push_back( foundTroops[i] );
            foundTroops.erase( foundTroops.begin() + i );
        }
        else {
            ++i;
        }
    }

    while ( result.size() != CHAIN_LIGHTNING_CREATURE_COUNT && !foundTroops.empty() ) {
        bool targetFound = false;
        for ( size_t i = 0; i < foundTroops.size(); ++i ) {
            const int32_t resist = foundTroops[i]->GetMagicResist( Spell::CHAINLIGHTNING, heroSpellPower, hero );
            assert( resist >= 0 );
            if ( resist < static_cast<int32_t>( _randomGenerator.Get( 1, 100 ) ) ) {
                ignoredTroops.push_back( foundTroops[i] );
                result.push_back( foundTroops[i] );
                foundTroops.erase( foundTroops.begin() + i );
                targetFound = true;
                break;
            }
        }

        // All targets are resisted. Choosing the nearest one.
        if ( !targetFound ) {
            ignoredTroops.push_back( foundTroops.front() );
            result.push_back( foundTroops.front() );
            foundTroops.erase( foundTroops.begin() );
        }

        if ( result.size() != CHAIN_LIGHTNING_CREATURE_COUNT ) {
            foundTroops = board.GetNearestTroops( result.back(), ignoredTroops );
        }
    }

    return result;
}

Battle::TargetsInfo Battle::Arena::TargetsForChainLightning( const HeroBase * hero, int32_t attackedTroopIndex )
{
    Unit * unit = GetTroopBoard( attackedTroopIndex );
    if ( unit == nullptr ) {
        assert( 0 );
        return TargetsInfo();
    }

    TargetsInfo targets;

    const uint32_t firstUnitResist = unit->GetMagicResist( Spell::CHAINLIGHTNING, 0, hero );

    if ( firstUnitResist >= _randomGenerator.Get( 1, 100 ) ) {
        targets.emplace_back();
        TargetInfo & res = targets.back();
        res.defender = unit;
        res.resist = true;
        return targets;
    }

    const std::vector<Unit *> targetUnits = FindChainLightningTargetIndexes( hero, unit );
    for ( size_t i = 0; i < targetUnits.size(); ++i ) {
        targets.emplace_back();
        TargetInfo & res = targets.back();

        res.defender = targetUnits[i];
        // store temp priority for calculate damage
        res.damage = static_cast<uint32_t>( i );
    }
    return targets;
}

Battle::TargetsInfo Battle::Arena::GetTargetsForSpells( const HeroBase * hero, const Spell & spell, int32_t dest, bool * playResistSound /* = nullptr */ )
{
    TargetsInfo targets;
    targets.reserve( 8 );

    if ( playResistSound ) {
        *playResistSound = true;
    }

    Unit * target = GetTroopBoard( dest );

    // from spells
    switch ( spell.GetID() ) {
    case Spell::CHAINLIGHTNING:
    case Spell::COLDRING:
        // skip center
        target = nullptr;
        break;

    default:
        break;
    }

    std::set<const Unit *> consideredTargets;

    TargetInfo res;

    // first target
    if ( target && target->AllowApplySpell( spell, hero ) && consideredTargets.insert( target ).second ) {
        res.defender = target;

        targets.push_back( res );
    }

    bool ignoreMagicResistance = false;

    // resurrect spell? get target from graveyard
    if ( nullptr == target && GraveyardAllowResurrect( dest, spell ) ) {
        target = GetTroopUID( graveyard.GetLastTroopUID( dest ) );

        if ( target && target->AllowApplySpell( spell, hero ) && consideredTargets.insert( target ).second ) {
            res.defender = target;

            targets.push_back( res );
        }
    }
    else {
        // check other spells
        switch ( spell.GetID() ) {
        case Spell::CHAINLIGHTNING: {
            for ( const TargetInfo & spellTarget : TargetsForChainLightning( hero, dest ) ) {
                assert( spellTarget.defender != nullptr );

                if ( consideredTargets.insert( spellTarget.defender ).second ) {
                    targets.push_back( spellTarget );
                }
                else {
                    // TargetsForChainLightning() should never return duplicates
                    assert( 0 );
                }
            }

            ignoreMagicResistance = true;

            if ( playResistSound ) {
                *playResistSound = true;
            }
            break;
        }

        // check abroads
        case Spell::FIREBALL:
        case Spell::METEORSHOWER:
        case Spell::COLDRING:
        case Spell::FIREBLAST: {
            for ( const int32_t index : Board::GetDistanceIndexes( dest, ( spell == Spell::FIREBLAST ? 2 : 1 ) ) ) {
                Unit * targetUnit = GetTroopBoard( index );

                if ( targetUnit && targetUnit->AllowApplySpell( spell, hero ) && consideredTargets.insert( targetUnit ).second ) {
                    res.defender = targetUnit;

                    targets.push_back( res );
                }
            }

            if ( playResistSound ) {
                *playResistSound = false;
            }
            break;
        }

        // check all troops
        case Spell::DEATHRIPPLE:
        case Spell::DEATHWAVE:
        case Spell::ELEMENTALSTORM:
        case Spell::HOLYWORD:
        case Spell::HOLYSHOUT:
        case Spell::ARMAGEDDON:
        case Spell::MASSBLESS:
        case Spell::MASSCURE:
        case Spell::MASSCURSE:
        case Spell::MASSDISPEL:
        case Spell::MASSHASTE:
        case Spell::MASSSHIELD:
        case Spell::MASSSLOW: {
            for ( Cell & cell : board ) {
                target = cell.GetUnit();

                if ( target && target->AllowApplySpell( spell, hero ) && consideredTargets.insert( target ).second ) {
                    res.defender = target;

                    targets.push_back( res );
                }
            }

            if ( playResistSound ) {
                *playResistSound = false;
            }
            break;
        }

        default:
            break;
        }
    }

    if ( !ignoreMagicResistance ) {
        // Mark magically resistant troops
        for ( auto & tgt : targets ) {
            const uint32_t resist = tgt.defender->GetMagicResist( spell, hero ? hero->GetPower() : 0, hero );

            if ( 0 < resist && 100 > resist && resist >= _randomGenerator.Get( 1, 100 ) ) {
                tgt.resist = true;
            }
        }
    }

    return targets;
}

void Battle::Arena::ApplyActionTower( Command & cmd )
{
    const uint32_t type = cmd.GetValue();
    const uint32_t uid = cmd.GetValue();

    Tower * tower = GetTower( static_cast<TowerType>( type ) );
    Unit * unit = GetTroopUID( uid );

    if ( unit && unit->isValid() && tower ) {
        DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "tower: " << type << ", attack to " << unit->String() )

        TargetInfo target;
        target.defender = unit;
        target.damage = tower->GetDamage( *unit );

        if ( _interface )
            _interface->RedrawActionTowerPart1( *tower, *unit );
        unit->ApplyDamage( *tower, target.damage, target.killed, nullptr );
        if ( _interface )
            _interface->RedrawActionTowerPart2( *tower, target );
    }
    else {
        DEBUG_LOG( DBG_BATTLE, DBG_WARN,
                   "incorrect param: "
                       << "tower: " << type << ", uid: " << GetHexString( uid ) )
    }
}

void Battle::Arena::ApplyActionCatapult( Command & cmd )
{
    if ( _catapult ) {
        uint32_t shots = cmd.GetValue();

        while ( shots-- ) {
            const int target = cmd.GetValue();
            const uint32_t damage = cmd.GetValue();
            const bool hit = cmd.GetValue() != 0;

            if ( target ) {
                if ( _interface ) {
                    _interface->RedrawActionCatapultPart1( target, hit );
                }

                if ( hit ) {
                    SetCastleTargetValue( target, GetCastleTargetValue( target ) - damage );
                    if ( _interface ) {
                        // Continue animating the smoke cloud after changing the "health" of the building.
                        _interface->RedrawActionCatapultPart2( target );
                    }
                }

                DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "target: " << target << ", damage: " << damage << ", hit: " << hit )
            }
        }
    }
    else {
        DEBUG_LOG( DBG_BATTLE, DBG_WARN, "incorrect param" )
    }
}

void Battle::Arena::ApplyActionAutoSwitch( Command & cmd )
{
    const int color = cmd.GetValue();

    if ( ( color != GetArmy1Color() && color != GetArmy2Color() ) || ( getForce( color ).GetControl() & CONTROL_AI ) ) {
        DEBUG_LOG( DBG_BATTLE, DBG_WARN,
                   "incorrect param: "
                       << "color: " << Color::String( color ) << " (" << color << ")" )

        return;
    }

    _autoBattleColors ^= color;

    if ( _interface ) {
        const Player * player = Players::Get( color );
        assert( player );

        std::string msg = ( _autoBattleColors & color ) ? _( "%{name} has turned on the auto battle" ) : _( "%{name} has turned off the auto battle" );
        StringReplace( msg, "%{name}", player->GetName() );

        _interface->SetStatus( msg, true );
    }

    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "color: " << Color::String( color ) << ", status: " << ( ( _autoBattleColors & color ) ? "on" : "off" ) )
}

void Battle::Arena::ApplyActionAutoFinish( const Command & /* cmd */ )
{
    const int army1Control = GetForce1().GetControl();
    const int army2Control = GetForce2().GetControl();

    const int army1Color = GetArmy1Color();
    const int army2Color = GetArmy2Color();

    if ( army1Control & CONTROL_REMOTE ) {
        DEBUG_LOG( DBG_BATTLE, DBG_WARN, "remote player: " << Color::String( army1Color ) << ", auto finish disabled" )

        return;
    }
    if ( army2Control & CONTROL_REMOTE ) {
        DEBUG_LOG( DBG_BATTLE, DBG_WARN, "remote player: " << Color::String( army2Color ) << ", auto finish disabled" )

        return;
    }

    if ( !( army1Control & CONTROL_HUMAN ) && !( army2Control & CONTROL_HUMAN ) ) {
        DEBUG_LOG( DBG_BATTLE, DBG_WARN, "no local human-controlled player participates in the battle, auto finish disabled" )

        return;
    }

    if ( army1Control & CONTROL_HUMAN ) {
        _autoBattleColors |= army1Color;
    }
    if ( army2Control & CONTROL_HUMAN ) {
        _autoBattleColors |= army2Color;
    }

    _interface.reset();

    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "finishing the battle" )
}

void Battle::Arena::ApplyActionSpellSummonElemental( const Command & /*cmd*/, const Spell & spell )
{
    Unit * elem = CreateElemental( spell );
    assert( elem != nullptr );

    if ( _interface ) {
        const HeroBase * commander = GetCurrentCommander();
        assert( commander != nullptr );

        _interface->RedrawActionSpellCastStatus( spell, -1, commander->GetName(), {} );
        _interface->RedrawActionSummonElementalSpell( *elem );
    }
}

void Battle::Arena::ApplyActionSpellDefaults( Command & cmd, const Spell & spell )
{
    const HeroBase * commander = GetCurrentCommander();
    assert( commander != nullptr );

    const int32_t dst = cmd.GetValue();

    bool playResistSound = false;
    TargetsInfo targets = GetTargetsForSpells( commander, spell, dst, &playResistSound );
    TargetsInfo resistTargets;

    if ( _interface ) {
        _interface->RedrawActionSpellCastStatus( spell, dst, commander->GetName(), targets );

        for ( const auto & target : targets ) {
            if ( target.resist ) {
                resistTargets.push_back( target );
            }
        }
    }

    targets.erase( std::remove_if( targets.begin(), targets.end(), []( const TargetInfo & v ) { return v.resist; } ), targets.end() );

    if ( _interface ) {
        _interface->RedrawActionSpellCastPart1( spell, dst, commander, targets );
        for ( const TargetInfo & target : resistTargets ) {
            _interface->RedrawActionResistSpell( *target.defender, playResistSound );
        }
    }

    TargetsApplySpell( commander, spell, targets );

    if ( _interface )
        _interface->RedrawActionSpellCastPart2( spell, targets );
}

void Battle::Arena::ApplyActionSpellTeleport( Command & cmd )
{
    const int32_t src = cmd.GetValue();
    const int32_t dst = cmd.GetValue();

    Unit * unit = GetTroopBoard( src );
    const Cell * cell = Board::GetCell( dst );

    if ( unit && unit->isValid() && cell && cell->isPassableForUnit( *unit ) ) {
        const Position pos = Position::GetPosition( *unit, dst );
        assert( pos.GetHead() != nullptr && ( !unit->isWide() || pos.GetTail() != nullptr ) );

        if ( _interface ) {
            const HeroBase * commander = GetCurrentCommander();
            assert( commander != nullptr );

            TargetInfo targetInfo;
            targetInfo.defender = unit;

            TargetsInfo targetsInfo;
            targetsInfo.push_back( targetInfo );

            _interface->RedrawActionSpellCastStatus( Spell( Spell::TELEPORT ), src, commander->GetName(), targetsInfo );
            _interface->RedrawActionTeleportSpell( *unit, pos.GetHead()->GetIndex() );
        }

        unit->SetPosition( pos );

        DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "src: " << src << ", dst: " << dst )
    }
    else {
        DEBUG_LOG( DBG_BATTLE, DBG_WARN,
                   "incorrect param: "
                       << "src: " << src << ", dst: " << dst )
    }
}

void Battle::Arena::ApplyActionSpellEarthQuake( const Command & /*cmd*/ )
{
    const HeroBase * commander = GetCurrentCommander();
    assert( commander != nullptr );

    std::vector<int> targets = GetCastleTargets();

    if ( _interface ) {
        _interface->RedrawActionSpellCastStatus( Spell( Spell::EARTHQUAKE ), -1, commander->GetName(), {} );
        _interface->RedrawActionEarthQuakeSpell( targets );
    }

    const std::pair<uint32_t, uint32_t> range = getEarthquakeDamageRange( commander );
    const std::vector<int> wallHexPositions = { CASTLE_FIRST_TOP_WALL_POS, CASTLE_SECOND_TOP_WALL_POS, CASTLE_THIRD_TOP_WALL_POS, CASTLE_FOURTH_TOP_WALL_POS };
    for ( int position : wallHexPositions ) {
        const int wallCondition = board[position].GetObject();

        if ( wallCondition > 0 ) {
            uint32_t wallDamage = _randomGenerator.Get( range.first, range.second );

            if ( wallDamage > static_cast<uint32_t>( wallCondition ) ) {
                wallDamage = wallCondition;
            }

            board[position].SetObject( wallCondition - wallDamage );
        }
    }

    if ( _towers[0] && _towers[0]->isValid() && _randomGenerator.Get( 1 ) ) {
        _towers[0]->SetDestroy();
    }
    if ( _towers[2] && _towers[2]->isValid() && _randomGenerator.Get( 1 ) ) {
        _towers[2]->SetDestroy();
    }

    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "spell: " << Spell( Spell::EARTHQUAKE ).GetName() << ", targets: " << targets.size() )
}

void Battle::Arena::ApplyActionSpellMirrorImage( Command & cmd )
{
    const int32_t who = cmd.GetValue();
    Unit * unit = GetTroopBoard( who );

    if ( unit && unit->isValid() ) {
        Indexes distances = Board::GetDistanceIndexes( unit->GetHeadIndex(), 4 );

        const int32_t centerIndex = unit->GetHeadIndex();
        std::sort( distances.begin(), distances.end(), [centerIndex]( const int32_t index1, const int32_t index2 ) {
            return Board::GetDistance( centerIndex, index1 ) < Board::GetDistance( centerIndex, index2 );
        } );

        Indexes::const_iterator it = std::find_if( distances.begin(), distances.end(), [unit]( const int32_t v ) { return Board::isValidMirrorImageIndex( v, unit ); } );
        if ( it != distances.end() ) {
            const HeroBase * commander = GetCurrentCommander();
            assert( commander != nullptr );

            const Spell mirrorImageSpell( Spell::MIRRORIMAGE );

            TargetInfo targetInfo;
            targetInfo.defender = unit;

            TargetsInfo targetsInfo;
            targetsInfo.push_back( targetInfo );

            TargetsApplySpell( commander, mirrorImageSpell, targetsInfo );

            Unit * mirrorUnit = CreateMirrorImage( *unit );
            assert( mirrorUnit != nullptr );

            const Position pos = Position::GetPosition( *mirrorUnit, *it );
            assert( pos.GetHead() != nullptr && ( !mirrorUnit->isWide() || pos.GetTail() != nullptr ) );

            DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "set position: " << pos.GetHead()->GetIndex() )

            if ( _interface ) {
                _interface->RedrawActionSpellCastStatus( mirrorImageSpell, who, commander->GetName(), targetsInfo );
                _interface->RedrawActionMirrorImageSpell( *unit, pos );
            }

            mirrorUnit->SetPosition( pos );
        }
        else {
            DEBUG_LOG( DBG_BATTLE, DBG_WARN, "no suitable position found" )

            if ( _interface ) {
                _interface->SetStatus( _( "Spell failed!" ), true );
            }
        }
    }
    else {
        DEBUG_LOG( DBG_BATTLE, DBG_WARN,
                   "incorrect param: "
                       << "who: " << who )
    }
}
