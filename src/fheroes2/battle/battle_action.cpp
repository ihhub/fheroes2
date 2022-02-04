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
#include <iomanip>
#include <set>

#include "battle_arena.h"
#include "battle_army.h"
#include "battle_bridge.h"
#include "battle_cell.h"
#include "battle_command.h"
#include "battle_interface.h"
#include "battle_tower.h"
#include "battle_troop.h"
#include "kingdom.h"
#include "logging.h"
#include "rand.h"
#include "spell.h"
#include "tools.h"
#include "translations.h"
#include "world.h"

namespace
{
    std::pair<int, int> getEarthquakeDamageRange( const HeroBase * commander )
    {
        const int spellPower = commander->GetPower();
        if ( ( spellPower > 0 ) && ( spellPower < 3 ) ) {
            return std::make_pair( 0, 1 );
        }
        else if ( ( spellPower >= 3 ) && ( spellPower < 6 ) ) {
            return std::make_pair( 0, 2 );
        }
        else if ( ( spellPower >= 6 ) && ( spellPower < 10 ) ) {
            return std::make_pair( 0, 3 );
        }
        else if ( spellPower >= 10 ) {
            return std::make_pair( 1, 3 );
        }

        DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "damage_range: unexpected spellPower value: " << spellPower << " for commander " << commander );
        return std::make_pair( 0, 0 );
    }
}

void Battle::Arena::BattleProcess( Unit & attacker, Unit & defender, s32 dst, int dir )
{
    if ( 0 > dst )
        dst = defender.GetHeadIndex();

    if ( dir ) {
        if ( attacker.isWide() ) {
            if ( !Board::isNearIndexes( attacker.GetHeadIndex(), dst ) )
                attacker.UpdateDirection( board[dst].GetPos() );
            if ( defender.AllowResponse() )
                defender.UpdateDirection( board[attacker.GetHeadIndex()].GetPos() );
        }
        else {
            attacker.UpdateDirection( board[dst].GetPos() );
            if ( defender.AllowResponse() )
                defender.UpdateDirection( board[attacker.GetHeadIndex()].GetPos() );
        }
    }
    else {
        attacker.UpdateDirection( board[dst].GetPos() );
    }

    // check luck right before the attack
    attacker.SetRandomLuck();

    TargetsInfo targets = GetTargetsForDamage( attacker, defender, dst, dir );

    if ( Board::isReflectDirection( dir ) != attacker.isReflect() )
        attacker.UpdateDirection( board[dst].GetPos() );

    if ( interface )
        interface->RedrawActionAttackPart1( attacker, defender, targets );

    TargetsApplyDamage( attacker, defender, targets );
    if ( interface )
        interface->RedrawActionAttackPart2( attacker, targets );

    if ( defender.isValid() ) {
        const Spell spell = attacker.GetSpellMagic();

        // magic attack
        if ( spell.isValid() ) {
            const std::string name( attacker.GetName() );

            targets = GetTargetsForSpells( attacker.GetCommander(), spell, defender.GetHeadIndex() );

            bool validSpell = true;
            if ( attacker == Monster::ARCHMAGE && !defender.Modes( IS_GOOD_MAGIC ) )
                validSpell = false;

            if ( !targets.empty() && validSpell ) {
                if ( interface ) {
                    interface->RedrawActionSpellCastStatus( spell, defender.GetHeadIndex(), name, targets );
                    interface->RedrawActionSpellCastPart1( spell, defender.GetHeadIndex(), nullptr, targets );
                }

                if ( attacker == Monster::ARCHMAGE ) {
                    if ( defender.Modes( IS_GOOD_MAGIC ) )
                        defender.ResetModes( IS_GOOD_MAGIC );
                }
                else {
                    // magic attack not depends from hero
                    TargetsApplySpell( nullptr, spell, targets );
                }

                if ( interface ) {
                    interface->RedrawActionSpellCastPart2( spell, targets );
                    interface->RedrawActionMonsterSpellCastStatus( attacker, targets.front() );
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

    case CommandType::MSG_BATTLE_AUTO:
        ApplyActionAutoBattle( cmd );
        break;

    default:
        break;
    }
}

void Battle::Arena::ApplyActionSpellCast( Command & cmd )
{
    const Spell spell( cmd.GetValue() );

    HeroBase * commander = GetCurrentForce().GetCommander();

    if ( commander && commander->HaveSpellBook() && !commander->Modes( Heroes::SPELLCASTED ) && commander->CanCastSpell( spell ) && spell.isCombat() ) {
        DEBUG_LOG( DBG_BATTLE, DBG_TRACE, commander->GetName() << ", color: " << Color::String( commander->GetColor() ) << ", spell: " << spell.GetName() );

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
        DEBUG_LOG( DBG_BATTLE, DBG_WARN,
                   spell.GetName() << ", "
                                   << "incorrect param" );
    }
}

void Battle::Arena::ApplyActionAttack( Command & cmd )
{
    const uint32_t uid1 = cmd.GetValue();
    const uint32_t uid2 = cmd.GetValue();
    const int32_t dst = cmd.GetValue();
    const int32_t dir = cmd.GetValue();

    Battle::Unit * b1 = GetTroopUID( uid1 );
    Battle::Unit * b2 = GetTroopUID( uid2 );

    if ( b1 && b1->isValid() && b2 && b2->isValid() && ( b1->GetCurrentColor() != b2->GetColor() ) ) {
        DEBUG_LOG( DBG_BATTLE, DBG_TRACE, b1->String() << " to " << b2->String() );

        const bool handfighting = Unit::isHandFighting( *b1, *b2 );
        // check position
        if ( b1->isArchers() || handfighting ) {
            b2->SetBlindAnswer( b2->Modes( SP_BLIND ) );

            // attack
            BattleProcess( *b1, *b2, dst, dir );

            if ( b2->isValid() ) {
                // defense answer
                if ( handfighting && !b1->ignoreRetaliation() && b2->AllowResponse() ) {
                    BattleProcess( *b2, *b1 );
                    b2->SetResponse();
                }
                b2->SetBlindAnswer( false );

                // twice attack
                if ( b1->isValid() && b1->isTwiceAttack() && !b1->Modes( SP_BLIND | IS_PARALYZE_MAGIC ) ) {
                    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "twice attack" );
                    BattleProcess( *b1, *b2, dst, dir );
                }
            }

            b1->UpdateDirection();
            b2->UpdateDirection();
        }
        else {
            DEBUG_LOG( DBG_BATTLE, DBG_WARN,
                       "incorrect param"
                           << ": " << b1->String( true ) << " and " << b2->String( true ) );
        }
    }
    else {
        DEBUG_LOG( DBG_BATTLE, DBG_WARN,
                   "incorrect param"
                       << ": "
                       << "uid: "
                       << "0x" << std::setw( 8 ) << std::setfill( '0' ) << std::hex << uid1 << ", "
                       << "uid: "
                       << "0x" << std::setw( 8 ) << std::setfill( '0' ) << std::hex << uid2 );
    }
}

void Battle::Arena::ApplyActionMove( Command & cmd )
{
    const uint32_t uid = cmd.GetValue();
    const int32_t dst = cmd.GetValue();

    Battle::Unit * b = GetTroopUID( uid );
    const Cell * cell = Board::GetCell( dst );

    if ( b && b->isValid() && cell && cell->isPassable3( *b, false ) ) {
        Position pos2;
        const s32 head = b->GetHeadIndex();
        Position pos1 = Position::GetPositionWhenMoved( *b, dst );

        DEBUG_LOG( DBG_BATTLE, DBG_TRACE,
                   b->String() << ", dst: " << dst << ", (head: " << pos1.GetHead()->GetIndex() << ", tail: " << ( b->isWide() ? pos1.GetTail()->GetIndex() : -1 )
                               << ")" );

        if ( b->isFlying() ) {
            b->UpdateDirection( pos1.GetRect() );
            if ( b->isReflect() != pos1.isReflect() )
                pos1.Swap();

            if ( interface ) {
                interface->RedrawActionFly( *b, pos1 );
            }
            else if ( bridge ) {
                const int32_t dstHead = pos1.GetHead()->GetIndex();
                const int32_t dstTail = b->isWide() ? pos1.GetTail()->GetIndex() : -1;

                // open the bridge if the unit should land on it
                if ( bridge->NeedDown( *b, dstHead ) ) {
                    bridge->Action( *b, dstHead );
                }
                else if ( b->isWide() && bridge->NeedDown( *b, dstTail ) ) {
                    bridge->Action( *b, dstTail );
                }

                b->SetPosition( pos1 );

                // check for possible bridge close action, after unit's end of movement
                if ( bridge->AllowUp() ) {
                    bridge->Action( *b, dstHead );
                }
            }

            pos2 = pos1;
        }
        else {
            const Indexes path = GetPath( *b, pos1 );

            if ( path.empty() ) {
                DEBUG_LOG( DBG_BATTLE, DBG_WARN,
                           "path empty, " << b->String() << " to "
                                          << "dst: " << dst );
                return;
            }

            if ( interface )
                interface->RedrawActionMove( *b, path );
            else if ( bridge ) {
                for ( Indexes::const_iterator pathIt = path.begin(); pathIt != path.end(); ++pathIt ) {
                    bool doMovement = false;

                    if ( bridge->NeedDown( *b, *pathIt ) )
                        bridge->Action( *b, *pathIt );

                    if ( b->isWide() ) {
                        if ( b->GetTailIndex() == *pathIt )
                            b->SetReflection( !b->isReflect() );
                        else
                            doMovement = true;
                    }
                    else {
                        doMovement = true;
                    }

                    if ( doMovement )
                        b->SetPosition( *pathIt );

                    // check for possible bridge close action, after unit's end of movement
                    if ( bridge->AllowUp() )
                        bridge->Action( *b, *pathIt );
                }
            }

            if ( b->isWide() ) {
                const s32 dst1 = path.back();
                const s32 dst2 = 1 < path.size() ? path[path.size() - 2] : head;

                pos2.Set( dst1, b->isWide(), ( RIGHT_SIDE & Board::GetDirection( dst1, dst2 ) ) != 0 );
            }
            else
                pos2.Set( path.back(), false, b->isReflect() );
        }

        b->SetPosition( pos2 );
        b->UpdateDirection();
    }
    else {
        DEBUG_LOG( DBG_BATTLE, DBG_WARN,
                   "incorrect param"
                       << ": "
                       << "uid: " << uid << ", dst: " << dst );
    }
}

void Battle::Arena::ApplyActionSkip( Command & cmd )
{
    const uint32_t uid = cmd.GetValue();
    const int32_t hard = cmd.GetValue();

    Battle::Unit * battle = GetTroopUID( uid );
    if ( battle && battle->isValid() ) {
        if ( !battle->Modes( TR_MOVED ) ) {
            if ( hard || battle->Modes( TR_SKIPMOVE ) ) {
                battle->SetModes( TR_HARDSKIP );
                battle->SetModes( TR_MOVED );
            }

            battle->SetModes( TR_SKIPMOVE );

            if ( interface )
                interface->RedrawActionSkipStatus( *battle );

            DEBUG_LOG( DBG_BATTLE, DBG_TRACE, battle->String() );
        }
        else {
            DEBUG_LOG( DBG_BATTLE, DBG_WARN, "uid: " << uid << " moved" );
        }
    }
    else {
        DEBUG_LOG( DBG_BATTLE, DBG_WARN,
                   "incorrect param"
                       << ": "
                       << "uid: " << uid );
    }
}

void Battle::Arena::ApplyActionEnd( Command & cmd )
{
    const uint32_t uid = cmd.GetValue();

    Battle::Unit * battle = GetTroopUID( uid );

    if ( battle ) {
        if ( !battle->Modes( TR_MOVED ) ) {
            battle->SetModes( TR_MOVED );

            if ( battle->Modes( TR_SKIPMOVE ) && interface )
                interface->RedrawActionSkipStatus( *battle );

            DEBUG_LOG( DBG_BATTLE, DBG_TRACE, battle->String() );
        }
        else {
            DEBUG_LOG( DBG_BATTLE, DBG_INFO, "uid: " << uid << " moved" );
        }
    }
    else {
        DEBUG_LOG( DBG_BATTLE, DBG_WARN,
                   "incorrect param"
                       << ": "
                       << "uid: "
                       << "0x" << std::setw( 8 ) << std::setfill( '0' ) << std::hex << uid );
    }
}

void Battle::Arena::ApplyActionMorale( Command & cmd )
{
    const uint32_t uid = cmd.GetValue();
    const int32_t morale = cmd.GetValue();

    Battle::Unit * b = GetTroopUID( uid );

    if ( b && b->isValid() ) {
        // good morale
        if ( morale && b->Modes( TR_MOVED ) && b->Modes( MORALE_GOOD ) ) {
            b->ResetModes( TR_MOVED );
            b->ResetModes( MORALE_GOOD );
            end_turn = false;
        }
        // bad morale
        else if ( !morale && !b->Modes( TR_MOVED ) && b->Modes( MORALE_BAD ) ) {
            b->SetModes( TR_MOVED );
            b->ResetModes( MORALE_BAD );
            end_turn = true;
        }

        if ( interface )
            interface->RedrawActionMorale( *b, morale != 0 );

        DEBUG_LOG( DBG_BATTLE, DBG_TRACE, ( morale ? "good" : "bad" ) << " to " << b->String() );
    }
    else {
        DEBUG_LOG( DBG_BATTLE, DBG_WARN,
                   "incorrect param"
                       << ": "
                       << "uid: "
                       << "0x" << std::setw( 8 ) << std::setfill( '0' ) << std::hex << uid );
    }
}

void Battle::Arena::ApplyActionRetreat( const Command & /*cmd*/ )
{
    if ( CanRetreatOpponent( current_color ) ) {
        if ( army1->GetColor() == current_color ) {
            result_game.army1 = RESULT_RETREAT;
        }
        else if ( army2->GetColor() == current_color ) {
            result_game.army2 = RESULT_RETREAT;
        }
        DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "color: " << Color::String( current_color ) );
    }
    else {
        DEBUG_LOG( DBG_BATTLE, DBG_WARN, "CanRetreatOpponent check failed" );
    }
}

void Battle::Arena::ApplyActionSurrender( const Command & /*cmd*/ )
{
    if ( CanSurrenderOpponent( current_color ) ) {
        Funds cost;

        if ( army1->GetColor() == current_color )
            cost.gold = army1->GetSurrenderCost();
        else if ( army2->GetColor() == current_color )
            cost.gold = army2->GetSurrenderCost();

        if ( world.GetKingdom( current_color ).AllowPayment( cost ) ) {
            if ( army1->GetColor() == current_color ) {
                result_game.army1 = RESULT_SURRENDER;
                world.GetKingdom( current_color ).OddFundsResource( cost );
                world.GetKingdom( army2->GetColor() ).AddFundsResource( cost );
            }
            else if ( army2->GetColor() == current_color ) {
                result_game.army2 = RESULT_SURRENDER;
                world.GetKingdom( current_color ).OddFundsResource( cost );
                world.GetKingdom( army1->GetColor() ).AddFundsResource( cost );
            }
            DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "color: " << Color::String( current_color ) );
        }
    }
    else {
        DEBUG_LOG( DBG_BATTLE, DBG_WARN, "incorrect param" );
    }
}

void Battle::Arena::TargetsApplyDamage( Unit & attacker, const Unit & /*defender*/, TargetsInfo & targets ) const
{
    for ( TargetsInfo::iterator it = targets.begin(); it != targets.end(); ++it ) {
        TargetInfo & target = *it;
        if ( target.defender )
            target.killed = target.defender->ApplyDamage( attacker, target.damage );
    }
}

Battle::TargetsInfo Battle::Arena::GetTargetsForDamage( const Unit & attacker, Unit & defender, const int32_t dst, const int dir )
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
    if ( attacker.GetID() == Monster::GENIE && Rand::Get( 1, 10 ) == 2 && defender.GetHitPoints() / 2 > res.damage ) {
        // Replaces the damage, not adding to it
        if ( defender.GetCount() == 1 ) {
            res.damage = defender.GetHitPoints();
        }
        else {
            res.damage = defender.GetHitPoints() / 2;
        }

        if ( Arena::GetInterface() ) {
            std::string str( _( "%{name} half the enemy troops!" ) );
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
        for ( const int32_t aroundIdx : Board::GetAroundIndexes( attacker ) ) {
            assert( Board::GetCell( aroundIdx ) != nullptr );

            Unit * enemy = Board::GetCell( aroundIdx )->GetUnit();

            if ( enemy && enemy->GetColor() != attacker.GetCurrentColor() && consideredTargets.insert( enemy ).second ) {
                res.defender = enemy;
                res.damage = attacker.GetDamage( *enemy );

                targets.push_back( res );
            }
        }
    }
    // lich cloud damage
    else if ( attacker.isAbilityPresent( fheroes2::MonsterAbilityType::AREA_SHOT ) && !attacker.isHandFighting() ) {
        for ( const int32_t aroundIdx : Board::GetAroundIndexes( dst ) ) {
            assert( Board::GetCell( aroundIdx ) != nullptr );

            Unit * enemy = Board::GetCell( aroundIdx )->GetUnit();

            if ( enemy && consideredTargets.insert( enemy ).second ) {
                res.defender = enemy;
                res.damage = attacker.GetDamage( *enemy );

                targets.push_back( res );
            }
        }
    }

    return targets;
}

void Battle::Arena::TargetsApplySpell( const HeroBase * hero, const Spell & spell, TargetsInfo & targets ) const
{
    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "targets: " << targets.size() );

    TargetsInfo::iterator it = targets.begin();

    for ( ; it != targets.end(); ++it ) {
        TargetInfo & target = *it;
        if ( target.defender )
            target.defender->ApplySpell( spell, hero, target );
    }
}

std::vector<Battle::Unit *> Battle::Arena::FindChainLightningTargetIndexes( const HeroBase * hero, Unit * firstUnit )
{
    std::vector<Battle::Unit *> result = { firstUnit };
    std::vector<Battle::Unit *> ignoredTroops = { firstUnit };

    std::vector<Battle::Unit *> foundTroops = board.GetNearestTroops( result.back(), ignoredTroops );

    const int heroSpellPower = hero ? hero->GetPower() : 0;

    // Filter those which are fully immuned
    for ( size_t i = 0; i < foundTroops.size(); ) {
        if ( foundTroops[i]->GetMagicResist( Spell::CHAINLIGHTNING, heroSpellPower ) >= 100 ) {
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
            const int32_t resist = foundTroops[i]->GetMagicResist( Spell::CHAINLIGHTNING, heroSpellPower );
            assert( resist >= 0 );
            if ( resist < static_cast<int32_t>( Rand::Get( 1, 100 ) ) ) {
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
                *playResistSound = false;
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
        // Mark magically resistant troops (should be ignored in case of built-in creature spells)
        for ( auto & tgt : targets ) {
            const uint32_t resist = tgt.defender->GetMagicResist( spell, hero ? hero->GetPower() : 0 );

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

    Tower * tower = GetTower( type );
    Battle::Unit * b2 = GetTroopUID( uid );

    if ( b2 && b2->isValid() && tower ) {
        DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "tower: " << type << ", attack to " << b2->String() );

        TargetInfo target;
        target.defender = b2;
        target.damage = tower->GetDamage( *b2 );

        if ( interface )
            interface->RedrawActionTowerPart1( *tower, *b2 );
        target.killed = b2->ApplyDamage( *tower, target.damage );
        if ( interface )
            interface->RedrawActionTowerPart2( *tower, target );
    }
    else {
        DEBUG_LOG( DBG_BATTLE, DBG_WARN,
                   "incorrect param"
                       << ": "
                       << "tower: " << type << ", uid: "
                       << "0x" << std::setw( 8 ) << std::setfill( '0' ) << std::hex << uid );
    }
}

void Battle::Arena::ApplyActionCatapult( Command & cmd )
{
    if ( catapult ) {
        u32 shots = cmd.GetValue();

        while ( shots-- ) {
            const int target = cmd.GetValue();
            const uint32_t damage = cmd.GetValue();
            const bool hit = cmd.GetValue() != 0;

            if ( target ) {
                if ( interface ) {
                    interface->RedrawActionCatapult( target, hit );
                }

                if ( hit ) {
                    SetCastleTargetValue( target, GetCastleTargetValue( target ) - damage );
                }

                DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "target: " << target << ", damage: " << damage << ", hit: " << hit );
            }
        }
    }
    else {
        DEBUG_LOG( DBG_BATTLE, DBG_WARN, "incorrect param" );
    }
}

void Battle::Arena::ApplyActionAutoBattle( Command & cmd )
{
    const int color = cmd.GetValue();
    if ( current_color != color ) {
        DEBUG_LOG( DBG_BATTLE, DBG_WARN, "incorrect param" );
        return;
    }

    if ( auto_battle & color ) {
        if ( interface )
            interface->SetStatus( _( "Set auto battle off" ), true );
        auto_battle &= ~color;
    }
    else {
        if ( interface )
            interface->SetStatus( _( "Set auto battle on" ), true );
        auto_battle |= color;
    }
}

void Battle::Arena::ApplyActionSpellSummonElemental( const Command & /*cmd*/, const Spell & spell )
{
    Unit * elem = CreateElemental( spell );
    assert( elem != nullptr );

    if ( interface ) {
        const HeroBase * commander = GetCurrentCommander();
        assert( commander != nullptr );

        interface->RedrawActionSpellCastStatus( spell, -1, commander->GetName(), {} );
        interface->RedrawActionSummonElementalSpell( *elem );
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

    if ( interface ) {
        interface->RedrawActionSpellCastStatus( spell, dst, commander->GetName(), targets );

        for ( const auto & target : targets ) {
            if ( target.resist ) {
                resistTargets.push_back( target );
            }
        }
    }

    targets.erase( std::remove_if( targets.begin(), targets.end(), []( const TargetInfo & v ) { return v.resist; } ), targets.end() );

    if ( interface ) {
        interface->RedrawActionSpellCastPart1( spell, dst, commander, targets );
        for ( const TargetInfo & target : resistTargets ) {
            interface->RedrawActionResistSpell( *target.defender, playResistSound );
        }
    }

    TargetsApplySpell( commander, spell, targets );

    if ( interface )
        interface->RedrawActionSpellCastPart2( spell, targets );
}

void Battle::Arena::ApplyActionSpellTeleport( Command & cmd )
{
    const int32_t src = cmd.GetValue();
    const int32_t dst = cmd.GetValue();

    Unit * b = GetTroopBoard( src );
    const Spell spell( Spell::TELEPORT );

    if ( b ) {
        Position pos = Position::GetPositionWhenMoved( *b, dst );
        if ( b->isReflect() != pos.isReflect() )
            pos.Swap();

        if ( interface ) {
            const HeroBase * commander = GetCurrentCommander();
            assert( commander != nullptr );

            TargetInfo targetInfo;
            targetInfo.defender = b;

            TargetsInfo targetsInfo;
            targetsInfo.push_back( targetInfo );

            interface->RedrawActionSpellCastStatus( spell, src, commander->GetName(), targetsInfo );
            interface->RedrawActionTeleportSpell( *b, pos.GetHead()->GetIndex() );
        }

        b->SetPosition( pos );
        b->UpdateDirection();

        DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "spell: " << spell.GetName() << ", src: " << src << ", dst: " << dst );
    }
    else {
        DEBUG_LOG( DBG_BATTLE, DBG_WARN, "spell: " << spell.GetName() << " false" );
    }
}

void Battle::Arena::ApplyActionSpellEarthQuake( const Command & /*cmd*/ )
{
    const HeroBase * commander = GetCurrentCommander();
    assert( commander != nullptr );

    std::vector<int> targets = GetCastleTargets();

    if ( interface ) {
        interface->RedrawActionSpellCastStatus( Spell( Spell::EARTHQUAKE ), -1, commander->GetName(), {} );
        interface->RedrawActionEarthQuakeSpell( targets );
    }

    const std::pair<int, int> range = getEarthquakeDamageRange( commander );
    const std::vector<int> wallHexPositions = { CASTLE_FIRST_TOP_WALL_POS, CASTLE_SECOND_TOP_WALL_POS, CASTLE_THIRD_TOP_WALL_POS, CASTLE_FOURTH_TOP_WALL_POS };
    for ( int position : wallHexPositions ) {
        const int wallCondition = board[position].GetObject();

        if ( wallCondition > 0 ) {
            uint32_t wallDamage = Rand::Get( range.first, range.second );

            if ( wallDamage > static_cast<uint32_t>( wallCondition ) ) {
                wallDamage = wallCondition;
            }

            board[position].SetObject( wallCondition - wallDamage );
        }
    }

    if ( towers[0] && towers[0]->isValid() && Rand::Get( 1 ) )
        towers[0]->SetDestroy();
    if ( towers[2] && towers[2]->isValid() && Rand::Get( 1 ) )
        towers[2]->SetDestroy();

    DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "spell: " << Spell( Spell::EARTHQUAKE ).GetName() << ", targets: " << targets.size() );
}

void Battle::Arena::ApplyActionSpellMirrorImage( Command & cmd )
{
    const int32_t who = cmd.GetValue();
    Unit * troop = GetTroopBoard( who );

    if ( troop != nullptr ) {
        Indexes distances = Board::GetDistanceIndexes( troop->GetHeadIndex(), 4 );

        const int32_t centerIndex = troop->GetHeadIndex();
        std::sort( distances.begin(), distances.end(), [centerIndex]( const int32_t index1, const int32_t index2 ) {
            return Battle::Board::GetDistance( centerIndex, index1 ) < Battle::Board::GetDistance( centerIndex, index2 );
        } );

        Indexes::const_iterator it
            = std::find_if( distances.begin(), distances.end(), [troop]( const int32_t v ) { return Battle::Board::isValidMirrorImageIndex( v, troop ); } );
        if ( it != distances.end() ) {
            const Position pos = Position::GetPositionWhenMoved( *troop, *it );
            const s32 dst = pos.GetHead()->GetIndex();

            DEBUG_LOG( DBG_BATTLE, DBG_TRACE, "set position: " << dst );

            if ( interface ) {
                const HeroBase * commander = GetCurrentCommander();
                assert( commander != nullptr );

                TargetInfo targetInfo;
                targetInfo.defender = troop;

                TargetsInfo targetsInfo;
                targetsInfo.push_back( targetInfo );

                interface->RedrawActionSpellCastStatus( Spell( Spell::MIRRORIMAGE ), who, commander->GetName(), targetsInfo );
                interface->RedrawActionMirrorImageSpell( *troop, pos );
            }

            Unit * mirror = CreateMirrorImage( *troop, dst );
            if ( mirror ) {
                mirror->SetPosition( pos );
            }
        }
        else {
            DEBUG_LOG( DBG_BATTLE, DBG_WARN, "new position not found!" );

            if ( interface ) {
                interface->SetStatus( _( "Spell failed!" ), true );
            }
        }
    }
    else {
        DEBUG_LOG( DBG_BATTLE, DBG_WARN, "spell: " << Spell( Spell::MIRRORIMAGE ).GetName() << " false" );
    }
}
