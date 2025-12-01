/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2025                                             *
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
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <ostream>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "battle.h"
#include "battle_arena.h" // IWYU pragma: associated
#include "battle_army.h"
#include "battle_board.h"
#include "battle_bridge.h"
#include "battle_catapult.h"
#include "battle_cell.h"
#include "battle_command.h"
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
    int32_t calculateAttackTarget( const Battle::Unit & attackingUnit, const Battle::Position & attackPosition, const Battle::Unit & defendingUnit )
    {
        const int32_t attackPositionHeadIdx = attackPosition.GetHead() ? attackPosition.GetHead()->GetIndex() : -1;
        const int32_t attackPositionTailIdx = attackPosition.GetTail() ? attackPosition.GetTail()->GetIndex() : -1;

        assert( attackPositionHeadIdx != -1 && ( attackingUnit.isWide() ? attackPositionTailIdx != -1 : attackPositionTailIdx == -1 ) );

        if ( Battle::Board::CanAttackFromCell( attackingUnit, attackPositionHeadIdx ) ) {
            // The defender's head cell is near the head cell of the attack position
            if ( Battle::Board::isNearIndexes( attackPositionHeadIdx, defendingUnit.GetHeadIndex() ) ) {
                return defendingUnit.GetHeadIndex();
            }

            // The defender's tail cell is near the head cell of the attack position
            if ( defendingUnit.isWide() && Battle::Board::isNearIndexes( attackPositionHeadIdx, defendingUnit.GetTailIndex() ) ) {
                return defendingUnit.GetTailIndex();
            }
        }

        if ( Battle::Board::CanAttackFromCell( attackingUnit, attackPositionTailIdx ) ) {
            // The defender's head cell is near the tail cell of the attack position
            if ( Battle::Board::isNearIndexes( attackPositionTailIdx, defendingUnit.GetHeadIndex() ) ) {
                return defendingUnit.GetHeadIndex();
            }

            // The defender's tail cell is near the tail cell of the attack position
            if ( defendingUnit.isWide() && Battle::Board::isNearIndexes( attackPositionTailIdx, defendingUnit.GetTailIndex() ) ) {
                return defendingUnit.GetTailIndex();
            }
        }

        // Attack position is not near the defender, this is most likely a shot
        return defendingUnit.GetHeadIndex();
    }

    Battle::CellDirection calculateAttackDirection( const Battle::Unit & attackingUnit, const Battle::Position & attackPosition, const int32_t attackTargetIdx )
    {
        const int32_t attackPositionHeadIdx = attackPosition.GetHead() ? attackPosition.GetHead()->GetIndex() : -1;
        const int32_t attackPositionTailIdx = attackPosition.GetTail() ? attackPosition.GetTail()->GetIndex() : -1;

        assert( attackPositionHeadIdx != -1 && ( attackingUnit.isWide() ? attackPositionTailIdx != -1 : attackPositionTailIdx == -1 ) );

        // The target cell of the attack is near the head cell of the attack position
        if ( Battle::Board::CanAttackFromCell( attackingUnit, attackPositionHeadIdx ) && Battle::Board::isNearIndexes( attackPositionHeadIdx, attackTargetIdx ) ) {
            return Battle::Board::GetDirection( attackPositionHeadIdx, attackTargetIdx );
        }

        // The target cell of the attack is near the tail cell of the attack position
        if ( Battle::Board::CanAttackFromCell( attackingUnit, attackPositionTailIdx ) && Battle::Board::isNearIndexes( attackPositionTailIdx, attackTargetIdx ) ) {
            return Battle::Board::GetDirection( attackPositionTailIdx, attackTargetIdx );
        }

        // Attack position is not near the defender, this is most likely a shot
        return Battle::CellDirection::UNKNOWN;
    }

    bool checkMoveParams( const Battle::Unit * unit, const int32_t dst )
    {
        assert( unit != nullptr && unit->isValid() );

        // "Moving" a unit to its current position is not allowed
        if ( unit->GetHeadIndex() == dst ) {
            return false;
        }

        const Battle::Position pos = Battle::Position::GetReachable( *unit, dst );
        if ( pos.GetHead() == nullptr ) {
            return false;
        }

        assert( pos.isValidForUnit( unit ) );

        // Index of the destination cell should correspond to the index of the head cell of the target position and nothing else
        return pos.GetHead()->GetIndex() == dst;
    }
}

void Battle::Arena::BattleProcess( Unit & attacker, Unit & defender, int32_t tgt /* = -1 */, int dir /* = -1 */ )
{
    if ( tgt < 0 ) {
        tgt = calculateAttackTarget( attacker, attacker.GetPosition(), defender );
    }

    const CellDirection cellDir = dir < 0 ? calculateAttackDirection( attacker, attacker.GetPosition(), tgt ) : static_cast<CellDirection>( dir );

    // UNKNOWN attack direction is only allowed for archers
    assert( Unit::isHandFighting( attacker, defender ) ? cellDir > Battle::CellDirection::UNKNOWN : cellDir == Battle::CellDirection::UNKNOWN );

    // This is a direct attack, update the direction for both the attacker and the defender
    if ( cellDir != Battle::CellDirection::UNKNOWN ) {
        const auto directionIsValidForAttack = []( const Unit & attackingUnit, const int32_t attackTgt, const Battle::CellDirection attackDir ) {
            assert( attackingUnit.isWide() );

            const int32_t attackSrc = Board::GetIndexDirection( attackTgt, Board::GetReflectDirection( attackDir ) );
            // Attacker should attack either from his head cell or from his tail cell, otherwise something strange happens
            assert( attackSrc == attackingUnit.GetHeadIndex() || attackSrc == attackingUnit.GetTailIndex() );

            return attackSrc == attackingUnit.GetHeadIndex();
        };

        if ( attacker.isWide() ) {
            if ( !directionIsValidForAttack( attacker, tgt, cellDir ) ) {
                attacker.SetReflection( !attacker.isReflect() );
            }
        }
        else {
            attacker.UpdateDirection( board[tgt].GetPos() );
        }

        if ( !attacker.isIgnoringRetaliation() && defender.isRetaliationAllowed() ) {
            const int32_t retaliationTgt = calculateAttackTarget( defender, defender.GetPosition(), attacker );
            const Battle::CellDirection retaliationDir = calculateAttackDirection( defender, defender.GetPosition(), retaliationTgt );

            if ( defender.isWide() ) {
                if ( !directionIsValidForAttack( defender, retaliationTgt, retaliationDir ) ) {
                    defender.SetReflection( !defender.isReflect() );
                }
            }
            else {
                defender.UpdateDirection( board[retaliationTgt].GetPos() );
            }
        }
    }
    // This is a shot, update the direction for the attacker only
    else {
        // For shooters we get the target position (not the 'tgt') to take into account the wide units.
        attacker.UpdateDirection( defender.GetRectPosition() );
    }

    // Update the attacker's luck right before the attack
    attacker.SetRandomLuck( _randomGenerator );

    // Do damage first
    TargetsInfo attackTargets = GetTargetsForDamage( attacker, defender, tgt, cellDir );

    if ( _interface ) {
        _interface->RedrawActionAttackPart1( attacker, defender, attackTargets );
    }

    uint32_t resurrected = 0;
    TargetsApplyDamage( attacker, attackTargets, resurrected );

    if ( _interface ) {
        _interface->RedrawActionAttackPart2( attacker, defender, attackTargets, resurrected );
    }

    // Then apply the attacker's built-in spell
    const Spell spell = attacker.GetSpellMagic( _randomGenerator );

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
            if ( !attackTarget.defender->AllowApplySpell( spell, nullptr ) ) {
                continue;
            }

            spellTargets.emplace_back( attackTarget.defender );
        }

        if ( !spellTargets.empty() ) {
            // The built-in spell can only be applied to one target. If there are multiple
            // targets eligible for this spell, then we should randomly select only one.
            if ( spellTargets.size() > 1 ) {
                const Unit * selectedUnit = Rand::GetWithGen( spellTargets, _randomGenerator ).defender;

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
                    _interface->redrawActionSpellCastStatus( spell, spellTargetUnit->GetHeadIndex(), attacker.GetName(), spellTargets );
                    _interface->redrawActionSpellCastPart1( spell, spellTargetUnit->GetHeadIndex(), nullptr, spellTargets );
                }

                if ( spell.GetID() == Spell::DISPEL ) {
                    spellTargetUnit->removeAffection( IS_GOOD_MAGIC );
                }
                else {
                    // The unit's built-in spell efficiency does not depend on its commanding hero's skills
                    TargetsApplySpell( nullptr, spell, spellTargets );
                }

                if ( _interface ) {
                    _interface->redrawActionSpellCastPart2( spell, spellTargets );
                    _interface->RedrawActionMonsterSpellCastStatus( spell, attacker, spellTargets.front() );
                }
            }
        }
    }

    attacker.PostAttackAction( defender );
}

void Battle::Arena::moveUnit( Unit * unit, const int32_t dst )
{
    if ( dst == -1 ) {
        return;
    }

    assert( checkMoveParams( unit, dst ) );

    Position pos = Position::GetReachable( *unit, dst );
    assert( pos.isValidForUnit( unit ) );

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
        const int32_t initialHead = unit->GetHeadIndex();

        const Indexes path = GetPath( *unit, pos );
        assert( !path.empty() );

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

            finalPos.Set( dstHead, true, isRightSide( Board::GetDirection( dstHead, dstTail ) ) );
        }
        else {
            finalPos.Set( path.back(), false, unit->isReflect() );
        }
    }

    unit->SetPosition( finalPos );
    unit->UpdateDirection();
}

void Battle::Arena::ApplyAction( Command & cmd )
{
    switch ( cmd.GetType() ) {
    case CommandType::SPELLCAST:
        ApplyActionSpellCast( cmd );
        break;
    case CommandType::ATTACK:
        ApplyActionAttack( cmd );
        break;
    case CommandType::MOVE:
        ApplyActionMove( cmd );
        break;
    case CommandType::SKIP:
        ApplyActionSkip( cmd );
        break;
    case CommandType::MORALE:
        ApplyActionMorale( cmd );
        break;

    case CommandType::TOWER:
        ApplyActionTower( cmd );
        break;
    case CommandType::CATAPULT:
        ApplyActionCatapult( cmd );
        break;

    case CommandType::RETREAT:
        ApplyActionRetreat( cmd );
        break;
    case CommandType::SURRENDER:
        ApplyActionSurrender( cmd );
        break;

    case CommandType::TOGGLE_AUTO_COMBAT:
        ApplyActionToggleAutoCombat( cmd );
        break;
    case CommandType::QUICK_COMBAT:
        ApplyActionQuickCombat( cmd );
        break;

    default:
        break;
    }
}

void Battle::Arena::ApplyActionSpellCast( Command & cmd )
{
    const auto checkParameters = []( const Spell & spell, const HeroBase * commander ) {
        if ( !spell.isCombat() ) {
            return false;
        }

        const Arena * arena = GetArena();
        assert( arena != nullptr );

        if ( arena->isDisableCastSpell( spell ) ) {
            return false;
        }

        if ( commander == nullptr || !commander->CanCastSpell( spell ) ) {
            return false;
        }

        return true;
    };

    const Spell spell( cmd.GetNextValue() );

    HeroBase * commander = GetCurrentForce().GetCommander();

    if ( !checkParameters( spell, commander ) ) {
        ERROR_LOG( "Invalid parameters: " << spell.GetName() )

#ifdef WITH_DEBUG
        assert( 0 );
#endif

        return;
    }

    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, commander->GetName() << ", color: " << Color::String( commander->GetColor() ) << ", spell: " << spell.GetName() )

    switch ( spell.GetID() ) {
    case Spell::TELEPORT:
        _applyActionSpellTeleport( cmd );
        break;

    case Spell::EARTHQUAKE:
        _applyActionSpellEarthquake();
        break;

    case Spell::MIRRORIMAGE:
        _applyActionSpellMirrorImage( cmd );
        break;

    case Spell::SUMMONEELEMENT:
    case Spell::SUMMONAELEMENT:
    case Spell::SUMMONFELEMENT:
    case Spell::SUMMONWELEMENT:
        _applyActionSpellSummonElemental( spell );
        break;

    default:
        _applyActionSpellDefaults( cmd, spell );
        break;
    }

    commander->SetModes( Heroes::SPELLCASTED );
    commander->SpellCasted( spell );

    // Save the spell for the Eagle Eye skill
    _usedSpells.Append( spell );
}

void Battle::Arena::ApplyActionAttack( Command & cmd )
{
    const auto checkParameters = []( const Unit * attacker, const Unit * defender, const int32_t dst, int32_t tgt, int dir ) {
        if ( attacker == nullptr || !attacker->isValid() ) {
            return false;
        }

        if ( defender == nullptr || !defender->isValid() ) {
            return false;
        }

        if ( attacker->Modes( TR_MOVED ) ) {
            return false;
        }

        if ( attacker->GetCurrentColor() == defender->GetColor() ) {
            return false;
        }

        // Attacker can attack from his current position without performing a move (in this case, the index of the destination cell should be -1)
        if ( dst != -1 && !checkMoveParams( attacker, dst ) ) {
            return false;
        }

        if ( attacker->isArchers() && !attacker->isHandFighting() ) {
            // Non-blocked archer can only attack by shooting from his current position
            if ( dst != -1 ) {
                return false;
            }

            if ( tgt < 0 ) {
                tgt = calculateAttackTarget( *attacker, attacker->GetPosition(), *defender );
            }

            const CellDirection cellDir = dir < 0 ? calculateAttackDirection( *attacker, attacker->GetPosition(), tgt ) : static_cast<CellDirection>( dir );

            if ( !defender->GetPosition().contains( tgt ) ) {
                return false;
            }

            // Non-blocked archers cannot attack "from a direction"
            if ( cellDir != CellDirection::UNKNOWN ) {
                return false;
            }

            return true;
        }

        const Position attackPos = ( dst == -1 ? attacker->GetPosition() : Position::GetReachable( *attacker, dst ) );
        if ( attackPos.GetHead() == nullptr ) {
            return false;
        }

        assert( attackPos.isValidForUnit( attacker ) );

        if ( tgt < 0 ) {
            tgt = calculateAttackTarget( *attacker, attackPos, *defender );
        }

        const CellDirection cellDir = dir < 0 ? calculateAttackDirection( *attacker, attackPos, tgt ) : static_cast<CellDirection>( dir );

        if ( !defender->GetPosition().contains( tgt ) ) {
            return false;
        }

        // Melee attacks are only possible from a certain direction
        if ( cellDir == CellDirection::UNKNOWN ) {
            return false;
        }

        const CellDirection reflectDir = Board::GetReflectDirection( cellDir );
        const int32_t attackIdx = ( Board::isValidDirection( tgt, reflectDir ) ? Board::GetIndexDirection( tgt, reflectDir ) : -1 );

        if ( !attackPos.contains( attackIdx ) ) {
            return false;
        }

        // Attack from a specified cell may be prohibited - for example, if this cell belongs to a castle moat
        if ( !Board::CanAttackFromCell( *attacker, attackIdx ) ) {
            return false;
        }

        return true;
    };

    const uint32_t attackerUID = cmd.GetNextValue();
    const uint32_t defenderUID = cmd.GetNextValue();
    const int32_t dst = cmd.GetNextValue();
    const int32_t tgt = cmd.GetNextValue();
    const int dir = cmd.GetNextValue();

    Unit * attacker = GetTroopUID( attackerUID );
    Unit * defender = GetTroopUID( defenderUID );

    if ( !checkParameters( attacker, defender, dst, tgt, dir ) ) {
        ERROR_LOG( "Invalid parameters: "
                   << "attacker uid: " << GetHexString( attackerUID ) << ", defender uid: " << GetHexString( defenderUID ) << ", dst: " << dst << ", tgt: " << tgt
                   << ", dir: " << dir )

#ifdef WITH_DEBUG
        assert( 0 );
#endif

        return;
    }

    moveUnit( attacker, dst );

    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, attacker->String() << " to " << defender->String() )

    const bool handfighting = Unit::isHandFighting( *attacker, *defender );
    const bool doubleAttack = attacker->isDoubleAttack();

    defender->SetBlindRetaliation( defender->Modes( SP_BLIND ) );

    BattleProcess( *attacker, *defender, tgt, dir );

    if ( defender->isValid() ) {
        if ( handfighting && !attacker->isIgnoringRetaliation() && defender->isRetaliationAllowed() ) {
            BattleProcess( *defender, *attacker );
            defender->setRetaliationAsCompleted();
        }

        if ( doubleAttack && attacker->isValid() && !attacker->isImmovable() ) {
            DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "double attack" )

            BattleProcess( *attacker, *defender, tgt, dir );
        }
    }

    defender->SetBlindRetaliation( false );

    // Berserk and Hypnotize spells should only be removed after an attack initiated by this unit, but not after its retaliatory attack
    attacker->removeAffection( SP_BERSERKER | SP_HYPNOTIZE );

    // Reflect attacker only if he is alive.
    if ( attacker->isValid() ) {
        attacker->UpdateDirection();
    }

    // Reflect defender only if he is alive.
    if ( defender->isValid() ) {
        defender->UpdateDirection();
    }

    attacker->SetModes( TR_MOVED );
}

void Battle::Arena::ApplyActionMove( Command & cmd )
{
    const auto checkParameters = []( const Unit * unit, const int32_t dst ) {
        if ( unit == nullptr || !unit->isValid() ) {
            return false;
        }

        if ( unit->Modes( TR_MOVED ) ) {
            return false;
        }

        if ( !checkMoveParams( unit, dst ) ) {
            return false;
        }

        return true;
    };

    const uint32_t uid = cmd.GetNextValue();
    const int32_t dst = cmd.GetNextValue();

    Unit * unit = GetTroopUID( uid );

    if ( !checkParameters( unit, dst ) ) {
        ERROR_LOG( "Invalid parameters: "
                   << "uid: " << GetHexString( uid ) << ", dst: " << dst )

#ifdef WITH_DEBUG
        assert( 0 );
#endif

        return;
    }

    moveUnit( unit, dst );

    unit->SetModes( TR_MOVED );
}

void Battle::Arena::ApplyActionSkip( Command & cmd )
{
    const auto checkParameters = []( const Unit * unit ) {
        if ( unit == nullptr || !unit->isValid() ) {
            return false;
        }

        if ( unit->Modes( TR_MOVED ) ) {
            return false;
        }

        return true;
    };

    const uint32_t uid = cmd.GetNextValue();

    Unit * unit = GetTroopUID( uid );

    if ( !checkParameters( unit ) ) {
        ERROR_LOG( "Invalid parameters: "
                   << "uid: " << GetHexString( uid ) )

#ifdef WITH_DEBUG
        assert( 0 );
#endif

        return;
    }

    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, unit->String() )

    unit->SetModes( TR_SKIP );
    unit->SetModes( TR_MOVED );

    if ( _interface ) {
        _interface->RedrawActionSkipStatus( *unit );
    }
}

void Battle::Arena::ApplyActionMorale( Command & cmd )
{
    const auto checkParameters = []( const Unit * unit, const bool morale ) {
        if ( unit == nullptr || !unit->isValid() ) {
            return false;
        }

        if ( morale ) {
            if ( !unit->AllModes( TR_MOVED | MORALE_GOOD ) ) {
                return false;
            }
        }
        else {
            if ( !unit->Modes( MORALE_BAD ) || unit->Modes( TR_MOVED ) ) {
                return false;
            }
        }

        return true;
    };

    const uint32_t uid = cmd.GetNextValue();
    const bool morale = cmd.GetNextValue();

    Unit * unit = GetTroopUID( uid );

    if ( !checkParameters( unit, morale ) ) {
        ERROR_LOG( "Invalid parameters: "
                   << "uid: " << GetHexString( uid ) << ", morale: " << ( morale ? "good" : "bad" ) )

#ifdef WITH_DEBUG
        assert( 0 );
#endif

        return;
    }

    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, ( morale ? "good" : "bad" ) << " to " << unit->String() )

    // Good morale
    if ( morale ) {
        unit->ResetModes( TR_MOVED | MORALE_GOOD );
    }
    // Bad morale
    else {
        unit->ResetModes( MORALE_BAD );
        unit->SetModes( TR_MOVED );
    }

    if ( _interface ) {
        _interface->RedrawActionMorale( *unit, morale != 0 );
    }
}

void Battle::Arena::ApplyActionRetreat( const Command & /* cmd */ )
{
    const PlayerColor currentColor = GetCurrentColor();

    if ( !CanRetreatOpponent( currentColor ) ) {
        ERROR_LOG( "Preconditions were not met" )

#ifdef WITH_DEBUG
        assert( 0 );
#endif

        return;
    }

    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "color: " << Color::String( currentColor ) )

    if ( _attackingArmy->GetColor() == currentColor ) {
        _battleResult.attacker = RESULT_RETREAT;
    }
    else if ( _defendingArmy->GetColor() == currentColor ) {
        _battleResult.defender = RESULT_RETREAT;
    }
    else {
        assert( 0 );
    }
}

void Battle::Arena::ApplyActionSurrender( const Command & /* cmd */ )
{
    const auto checkPreconditions = []( const Funds & cost ) {
        const Arena * arena = GetArena();
        assert( arena != nullptr );

        if ( !arena->CanSurrenderOpponent( arena->GetCurrentColor() ) ) {
            return false;
        }

        if ( !world.GetKingdom( arena->GetCurrentColor() ).AllowPayment( cost ) ) {
            return false;
        }

        return true;
    };

    const PlayerColor currentColor = GetCurrentColor();

    if ( _attackingArmy->GetColor() == currentColor ) {
        Funds cost;

        cost.gold = _attackingArmy->GetSurrenderCost();

        if ( !checkPreconditions( cost ) ) {
            ERROR_LOG( "Preconditions were not met" )

#ifdef WITH_DEBUG
            assert( 0 );
#endif

            return;
        }

        DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "color: " << Color::String( currentColor ) )

        world.GetKingdom( _attackingArmy->GetColor() ).OddFundsResource( cost );
        world.GetKingdom( _defendingArmy->GetColor() ).AddFundsResource( cost );

        _battleResult.attacker = RESULT_SURRENDER;
    }
    else if ( _defendingArmy->GetColor() == currentColor ) {
        Funds cost;

        cost.gold = _defendingArmy->GetSurrenderCost();

        if ( !checkPreconditions( cost ) ) {
            ERROR_LOG( "Preconditions were not met" )

#ifdef WITH_DEBUG
            assert( 0 );
#endif

            return;
        }

        DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "color: " << Color::String( currentColor ) )

        world.GetKingdom( _defendingArmy->GetColor() ).OddFundsResource( cost );
        world.GetKingdom( _attackingArmy->GetColor() ).AddFundsResource( cost );

        _battleResult.defender = RESULT_SURRENDER;
    }
    else {
        assert( 0 );
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

Battle::TargetsInfo Battle::Arena::GetTargetsForDamage( const Unit & attacker, Unit & defender, const int32_t dst, const Battle::CellDirection dir ) const
{
    // The attacked unit should be located on the attacked cell
    assert( defender.GetHeadIndex() == dst || defender.GetTailIndex() == dst );

    TargetsInfo targets;
    targets.reserve( 8 );

    TargetInfo res;

    // first target
    res.defender = &defender;
    res.damage = attacker.GetDamage( defender, _randomGenerator );

    // Genie special attack
    {
        const std::vector<fheroes2::MonsterAbility> & attackerAbilities = fheroes2::getMonsterData( attacker.GetID() ).battleStats.abilities;

        if ( const auto abilityIter = std::find( attackerAbilities.begin(), attackerAbilities.end(), fheroes2::MonsterAbilityType::ENEMY_HALVING );
             abilityIter != attackerAbilities.end() ) {
            const uint32_t halvingDamage = ( defender.GetCount() / 2 + defender.GetCount() % 2 ) * defender.Monster::GetHitPoints();
            if ( halvingDamage > res.damage && Rand::GetWithGen( 1, 100, _randomGenerator ) <= abilityIter->percentage ) {
                // Replaces damage, not adds extra damage
                res.damage = std::min( defender.GetHitPoints(), halvingDamage );

                Interface * iface = GetInterface();
                if ( iface ) {
                    std::string str( _n( "%{name} destroys half the enemy troops!", "%{name} destroy half the enemy troops!", attacker.GetCount() ) );
                    StringReplace( str, "%{name}", attacker.GetName() );

                    iface->setStatus( str, true );
                }
            }
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
            res.damage = attacker.GetDamage( *enemy, _randomGenerator );

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
                res.damage = attacker.GetDamage( *enemy, _randomGenerator );

                targets.push_back( res );
            }
        }
    }
    // lich cloud damage
    else if ( attacker.isAbilityPresent( fheroes2::MonsterAbilityType::AREA_SHOT ) && !Unit::isHandFighting( attacker, defender ) ) {
        for ( const int32_t nearbyIdx : Board::GetAroundIndexes( dst ) ) {
            assert( Board::GetCell( nearbyIdx ) != nullptr );

            Unit * enemy = Board::GetCell( nearbyIdx )->GetUnit();

            if ( enemy && consideredTargets.insert( enemy ).second ) {
                res.defender = enemy;
                res.damage = attacker.GetDamage( *enemy, _randomGenerator );

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

std::vector<Battle::Unit *> Battle::Arena::FindChainLightningTargetIndexes( const HeroBase * hero, Unit * firstUnit, const bool applyRandomMagicResistance )
{
    std::vector<Unit *> result = { firstUnit };
    std::vector<Unit *> ignoredTroops = { firstUnit };

    std::vector<Unit *> foundTroops = board.GetNearestTroops( result.back(), ignoredTroops );

    // Filter those which are fully immuned
    for ( size_t i = 0; i < foundTroops.size(); ) {
        if ( foundTroops[i]->GetMagicResist( Spell::CHAINLIGHTNING, hero ) >= 100 ) {
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
            const uint32_t resist = foundTroops[i]->GetMagicResist( Spell::CHAINLIGHTNING, hero );
            assert( resist < 100 );

            if ( !applyRandomMagicResistance || resist < Rand::GetWithGen( 1, 100, _randomGenerator ) ) {
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

Battle::TargetsInfo Battle::Arena::TargetsForChainLightning( const HeroBase * hero, const int32_t attackedTroopIndex, const bool applyRandomMagicResistance )
{
    Unit * unit = GetTroopBoard( attackedTroopIndex );
    if ( unit == nullptr ) {
        assert( 0 );
        return {};
    }

    TargetsInfo targets;

    const uint32_t firstUnitResist = unit->GetMagicResist( Spell::CHAINLIGHTNING, hero );

    if ( firstUnitResist >= 100 || ( applyRandomMagicResistance && firstUnitResist >= Rand::GetWithGen( 1, 100, _randomGenerator ) ) ) {
        targets.emplace_back();
        TargetInfo & res = targets.back();
        res.defender = unit;
        res.resist = true;
        return targets;
    }

    const std::vector<Unit *> targetUnits = FindChainLightningTargetIndexes( hero, unit, applyRandomMagicResistance );
    for ( size_t i = 0; i < targetUnits.size(); ++i ) {
        targets.emplace_back();
        TargetInfo & res = targets.back();

        res.defender = targetUnits[i];
        // store temp priority for calculate damage
        res.damage = static_cast<uint32_t>( i );
    }
    return targets;
}

Battle::TargetsInfo Battle::Arena::GetTargetsForSpell( const HeroBase * hero, const Spell & spell, const int32_t dst, bool applyRandomMagicResistance,
                                                       bool * playResistSound )
{
    TargetsInfo targets;
    targets.reserve( 8 );

    if ( playResistSound ) {
        *playResistSound = true;
    }

    Unit * target = GetTroopBoard( dst );

    switch ( spell.GetID() ) {
    case Spell::CHAINLIGHTNING:
    case Spell::COLDRING:
        target = nullptr;
        break;

    default:
        break;
    }

    std::set<const Unit *> consideredTargets;

    TargetInfo res;

    if ( target && target->AllowApplySpell( spell, hero ) && consideredTargets.insert( target ).second ) {
        res.defender = target;

        targets.push_back( res );
    }

    if ( target == nullptr && isAbleToResurrectFromGraveyard( dst, spell ) ) {
        target = getLastResurrectableUnitFromGraveyard( dst, spell );
        assert( target != nullptr && !target->isValid() );

        if ( consideredTargets.insert( target ).second ) {
            res.defender = target;

            targets.push_back( res );
        }
    }
    else {
        switch ( spell.GetID() ) {
        case Spell::CHAINLIGHTNING: {
            for ( const TargetInfo & spellTarget : TargetsForChainLightning( hero, dst, applyRandomMagicResistance ) ) {
                assert( spellTarget.defender != nullptr );

                if ( consideredTargets.insert( spellTarget.defender ).second ) {
                    targets.push_back( spellTarget );
                }
                else {
                    // TargetsForChainLightning() should never return duplicates
                    assert( 0 );
                }
            }

            // Magic resistance has already been applied in the process of selecting targets for Chain Lightning
            applyRandomMagicResistance = false;

            // TODO: remove this temporary assertion
            if ( playResistSound ) {
                assert( *playResistSound );
            }
            break;
        }

        case Spell::FIREBALL:
        case Spell::METEORSHOWER:
        case Spell::COLDRING:
        case Spell::FIREBLAST: {
            for ( const int32_t index : Board::GetDistanceIndexes( dst, ( spell == Spell::FIREBLAST ? 2 : 1 ) ) ) {
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

    // Mark magically resistant troops
    for ( auto & tgt : targets ) {
        const uint32_t resist = tgt.defender->GetMagicResist( spell, hero );
        assert( resist < 100 );

        if ( applyRandomMagicResistance && resist >= Rand::GetWithGen( 1, 100, _randomGenerator ) ) {
            tgt.resist = true;
        }
    }

    return targets;
}

void Battle::Arena::ApplyActionTower( Command & cmd )
{
    const auto checkParameters = []( const Tower * tower, const Unit * unit ) {
        if ( tower == nullptr || !tower->isValid() ) {
            return false;
        }

        if ( unit == nullptr || !unit->isValid() ) {
            return false;
        }

        return true;
    };

    const uint32_t type = cmd.GetNextValue();
    const uint32_t uid = cmd.GetNextValue();

    Tower * tower = GetTower( static_cast<TowerType>( type ) );
    Unit * unit = GetTroopUID( uid );

    if ( !checkParameters( tower, unit ) ) {
        ERROR_LOG( "Invalid parameters: "
                   << "tower: " << type << ", uid: " << GetHexString( uid ) )

#ifdef WITH_DEBUG
        assert( 0 );
#endif

        return;
    }

    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "tower: " << type << ", target: " << unit->String() )

    TargetInfo target;
    target.defender = unit;
    target.damage = tower->GetDamage( *unit, _randomGenerator );

    if ( _interface ) {
        _interface->RedrawActionTowerPart1( *tower, *unit );
    }

    unit->ApplyDamage( *tower, target.damage, target.killed, nullptr );

    if ( _interface ) {
        _interface->RedrawActionTowerPart2( *tower, target );
    }
}

void Battle::Arena::ApplyActionCatapult( Command & cmd )
{
    if ( _catapult == nullptr ) {
        ERROR_LOG( "Preconditions were not met" )

#ifdef WITH_DEBUG
        assert( 0 );
#endif

        return;
    }

    const auto checkParameters = [this]( const CastleDefenseStructure target, const int damage ) {
        if ( damage <= 0 ) {
            return false;
        }

        const std::vector<CastleDefenseStructure> & allowedTargets = Catapult::getAllowedTargets();

        if ( std::find( allowedTargets.begin(), allowedTargets.end(), target ) == allowedTargets.end() ) {
            return false;
        }

        return ( damage <= getCastleDefenseStructureCondition( target, SiegeWeaponType::Catapult ) );
    };

    uint32_t shots = cmd.GetNextValue();

    while ( shots-- ) {
        const CastleDefenseStructure target = static_cast<CastleDefenseStructure>( cmd.GetNextValue() );
        const int damage = cmd.GetNextValue();
        const bool hit = ( cmd.GetNextValue() != 0 );

        if ( target == CastleDefenseStructure::NONE ) {
            continue;
        }

        using TargetUnderlyingType = std::underlying_type_t<decltype( target )>;

        if ( !checkParameters( target, damage ) ) {
            ERROR_LOG( "Invalid parameters: "
                       << "target: " << static_cast<TargetUnderlyingType>( target ) << ", damage: " << damage << ", hit: " << ( hit ? "yes" : "no" ) )

#ifdef WITH_DEBUG
            assert( 0 );
#endif

            return;
        }

        DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "target: " << static_cast<TargetUnderlyingType>( target ) << ", damage: " << damage << ", hit: " << ( hit ? "yes" : "no" ) )

        if ( _interface ) {
            _interface->RedrawActionCatapultPart1( target, hit );
        }

        if ( !hit ) {
            continue;
        }

        applyDamageToCastleDefenseStructure( target, damage );

        if ( _interface ) {
            // Continue animating the smoke cloud after changing the "health" of the building.
            _interface->RedrawActionCatapultPart2( target );
        }
    }
}

void Battle::Arena::ApplyActionToggleAutoCombat( Command & cmd )
{
    const auto checkParameters = []( const PlayerColor color ) {
        const Arena * arena = GetArena();
        assert( arena != nullptr );

        if ( color != arena->getAttackingArmyColor() && color != arena->getDefendingArmyColor() ) {
            return false;
        }

        if ( arena->getForce( color ).GetControl() & CONTROL_AI ) {
            return false;
        }

        return true;
    };

    const PlayerColor color = static_cast<PlayerColor>( cmd.GetNextValue() );

    if ( !checkParameters( color ) ) {
        ERROR_LOG( "Invalid parameters: "
                   << "color: " << Color::String( color ) << " (" << static_cast<int>( color ) << ")" )

#ifdef WITH_DEBUG
        assert( 0 );
#endif

        return;
    }

    _autoCombatColors ^= color;

    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "color: " << Color::String( color ) << ", status: " << ( ( _autoCombatColors & color ) ? "on" : "off" ) )

    if ( _interface ) {
        const Player * player = Players::Get( color );
        assert( player );

        std::string msg = ( _autoCombatColors & color ) ? _( "%{name} has turned on the auto combat" ) : _( "%{name} has turned off the auto combat" );
        StringReplace( msg, "%{name}", player->GetName() );

        _interface->setStatus( msg, true );
    }
}

void Battle::Arena::ApplyActionQuickCombat( const Command & /* cmd */ )
{
    const int attackingArmyControl = getAttackingForce().GetControl();
    const int defendingArmyControl = getDefendingForce().GetControl();

    if ( !( attackingArmyControl & CONTROL_HUMAN ) && !( defendingArmyControl & CONTROL_HUMAN ) ) {
        ERROR_LOG( "Preconditions were not met" )

#ifdef WITH_DEBUG
        assert( 0 );
#endif

        return;
    }

    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "finishing the battle" )

    const PlayerColor attackingArmyColor = getAttackingArmyColor();
    const PlayerColor defendingArmyColor = getDefendingArmyColor();

    if ( attackingArmyControl & CONTROL_HUMAN ) {
        _autoCombatColors |= attackingArmyColor;
    }
    if ( defendingArmyControl & CONTROL_HUMAN ) {
        _autoCombatColors |= defendingArmyColor;
    }

    _interface.reset();
}

void Battle::Arena::_applyActionSpellSummonElemental( const Spell & spell )
{
    const auto checkPreconditions = []() {
        const Arena * arena = GetArena();
        assert( arena != nullptr );

        if ( arena->GetCurrentCommander() == nullptr ) {
            return false;
        }

        const int32_t idx = arena->GetFreePositionNearHero( arena->GetCurrentColor() );

        return Board::isValidIndex( idx );
    };

    if ( !checkPreconditions() ) {
        ERROR_LOG( "Preconditions were not met" )

#ifdef WITH_DEBUG
        assert( 0 );
#endif

        return;
    }

    Unit * elem = CreateElemental( spell );
    assert( elem != nullptr );

    if ( _interface ) {
        const HeroBase * commander = GetCurrentCommander();
        assert( commander != nullptr );

        // We are summoning a unit and need to set it fully transparent before the summon animation starts.
        elem->SetCustomAlpha( 0 );

        TargetsInfo targetsInfo;
        targetsInfo.emplace_back( elem );

        _interface->redrawActionSpellCastStatus( spell, -1, commander->GetName(), {} );
        _interface->redrawActionSpellCastPart1( spell, elem->GetHeadIndex(), commander, targetsInfo );
    }
}

void Battle::Arena::_applyActionSpellDefaults( Command & cmd, const Spell & spell )
{
    const int32_t dst = cmd.GetNextValue();

    const HeroBase * commander = GetCurrentCommander();
    if ( commander == nullptr ) {
        ERROR_LOG( "Preconditions were not met" )

#ifdef WITH_DEBUG
        assert( 0 );
#endif

        return;
    }

    bool playResistSound = false;
    TargetsInfo targets = GetTargetsForSpell( commander, spell, dst, true, &playResistSound );
    TargetsInfo resistTargets;

    if ( _interface ) {
        _interface->redrawActionSpellCastStatus( spell, dst, commander->GetName(), targets );

        for ( const auto & target : targets ) {
            if ( target.resist ) {
                resistTargets.push_back( target );
            }
        }
    }

    targets.erase( std::remove_if( targets.begin(), targets.end(), []( const TargetInfo & v ) { return v.resist; } ), targets.end() );

    if ( _interface ) {
        _interface->redrawActionSpellCastPart1( spell, dst, commander, targets );
        for ( const TargetInfo & target : resistTargets ) {
            _interface->RedrawActionResistSpell( *target.defender, playResistSound );
        }
    }

    TargetsApplySpell( commander, spell, targets );

    if ( _interface ) {
        _interface->redrawActionSpellCastPart2( spell, targets );
    }
}

void Battle::Arena::_applyActionSpellTeleport( Command & cmd )
{
    const auto checkParameters = []( const Unit * unit, const Cell * cell ) {
        if ( unit == nullptr || !unit->isValid() ) {
            return false;
        }

        if ( !cell->isPassableForUnit( *unit ) ) {
            return false;
        }

        const Arena * arena = GetArena();
        assert( arena != nullptr );

        return arena->GetCurrentCommander() != nullptr;
    };

    const int32_t src = cmd.GetNextValue();
    const int32_t dst = cmd.GetNextValue();

    Unit * unit = GetTroopBoard( src );
    const Cell * cell = Board::GetCell( dst );

    if ( !checkParameters( unit, cell ) ) {
        ERROR_LOG( "Invalid parameters: "
                   << "src: " << src << ", dst: " << dst )

#ifdef WITH_DEBUG
        assert( 0 );
#endif

        return;
    }

    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "src: " << src << ", dst: " << dst )

    const Position pos = Position::GetPosition( *unit, dst );
    assert( pos.isValidForUnit( unit ) );

    if ( _interface ) {
        const HeroBase * commander = GetCurrentCommander();
        assert( commander != nullptr );

        TargetsInfo targetsInfo;
        targetsInfo.emplace_back( unit );

        const Spell spell( Spell::TELEPORT );

        _interface->redrawActionSpellCastStatus( spell, src, commander->GetName(), targetsInfo );
        _interface->redrawActionSpellCastPart1( spell, pos.GetHead()->GetIndex(), commander, targetsInfo );
    }

    unit->SetPosition( pos );
}

void Battle::Arena::_applyActionSpellEarthquake()
{
    const HeroBase * commander = GetCurrentCommander();
    if ( commander == nullptr ) {
        ERROR_LOG( "Preconditions were not met" )

#ifdef WITH_DEBUG
        assert( 0 );
#endif

        return;
    }

    const auto [minDamage, maxDamage] = getEarthquakeDamageRange( commander );

    std::map<CastleDefenseStructure, int> earthquakeDamage;

    for ( const CastleDefenseStructure target : getEarthQuakeSpellTargets() ) {
        const int targetCondition = getCastleDefenseStructureCondition( target, SiegeWeaponType::EarthquakeSpell );
        assert( targetCondition >= 0 );

        if ( targetCondition == 0 ) {
            continue;
        }

        const int damage = [this, minDmg = minDamage, maxDmg = maxDamage, target]() {
            // Reduce the chance of bridge demolition by an extra 50% chance to "miss" it by the Earthquake spell.
            // It is done to be closer to the original game behavior where bridge demolition by this spell
            // is more rare than the demolition of the other structures.
            if ( target == CastleDefenseStructure::BRIDGE && Rand::GetWithGen( 0, 1, _randomGenerator ) == 0 ) {
                return 0;
            }

            return static_cast<int>( Rand::GetWithGen( minDmg, maxDmg, _randomGenerator ) );
        }();
        assert( damage >= 0 );

        if ( damage == 0 ) {
            continue;
        }

        if ( const auto [dummy, inserted] = earthquakeDamage.try_emplace( target, std::min( damage, targetCondition ) ); !inserted ) {
            assert( 0 );
        }
    }

    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "number of damaged targets: " << earthquakeDamage.size() )

    std::vector<CastleDefenseStructure> earthquakeTargets;

    if ( _interface ) {
        earthquakeTargets.reserve( earthquakeDamage.size() );

        for ( const auto & [target, dummy] : earthquakeDamage ) {
            earthquakeTargets.push_back( target );
        }

        _interface->redrawActionSpellCastStatus( Spell( Spell::EARTHQUAKE ), -1, commander->GetName(), {} );
        _interface->redrawActionEarthquakeSpellPart1( *commander, earthquakeTargets );
    }

    for ( const auto & [target, damage] : earthquakeDamage ) {
        applyDamageToCastleDefenseStructure( target, damage );
    }

    if ( _interface ) {
        // Render a second part of blast animation after targets are damaged or even destroyed.
        _interface->redrawActionEarthquakeSpellPart2( earthquakeTargets );
    }
}

void Battle::Arena::_applyActionSpellMirrorImage( Command & cmd )
{
    const auto checkParameters = []( const Unit * unit ) {
        if ( unit == nullptr || !unit->isValid() ) {
            return false;
        }

        const Arena * arena = GetArena();
        assert( arena != nullptr );

        return arena->GetCurrentCommander() != nullptr;
    };

    const int32_t targetUnitCellIndex = cmd.GetNextValue();

    Unit * unit = GetTroopBoard( targetUnitCellIndex );

    if ( !checkParameters( unit ) ) {
        ERROR_LOG( "Invalid parameters: "
                   << "cell index with original unit: " << targetUnitCellIndex )

#ifdef WITH_DEBUG
        assert( 0 );
#endif

        return;
    }

    Indexes distances = Board::GetDistanceIndexes( unit->GetHeadIndex(), 4 );

    const int32_t centerIndex = unit->GetHeadIndex();
    std::sort( distances.begin(), distances.end(), [centerIndex]( const int32_t index1, const int32_t index2 ) {
        return Board::GetDistance( centerIndex, index1 ) < Board::GetDistance( centerIndex, index2 );
    } );

    const Indexes::const_iterator it
        = std::find_if( distances.cbegin(), distances.cend(), [unit]( const int32_t v ) { return Board::isValidMirrorImageIndex( v, unit ); } );
    if ( it != distances.end() ) {
        const HeroBase * commander = GetCurrentCommander();
        assert( commander != nullptr );

        const Spell mirrorImageSpell( Spell::MIRRORIMAGE );

        TargetsInfo targetsInfo;
        targetsInfo.emplace_back( unit );

        TargetsApplySpell( commander, mirrorImageSpell, targetsInfo );

        Unit * mirrorUnit = CreateMirrorImage( *unit );
        assert( mirrorUnit != nullptr );

        const Position pos = Position::GetPosition( *mirrorUnit, *it );
        assert( pos.isValidForUnit( *mirrorUnit ) );

        if ( _interface ) {
            _interface->redrawActionSpellCastStatus( mirrorImageSpell, targetUnitCellIndex, commander->GetName(), targetsInfo );
        }

        DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "set position: " << pos.GetHead()->GetIndex() )
        mirrorUnit->SetPosition( pos );

        if ( _interface ) {
            _interface->redrawActionMirrorImageSpell( *commander, targetUnitCellIndex, *unit, *mirrorUnit );
        }
    }
    else {
        DEBUG_LOG( DBG_BATTLE, DBG_WARN, "no suitable position found" )

        if ( _interface ) {
            _interface->setStatus( _( "Spell failed!" ), true );
        }
    }
}
