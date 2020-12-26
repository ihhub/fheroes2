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

#include "battle_arena.h"
#include "battle_bridge.h"
#include "battle_catapult.h"
#include "battle_cell.h"
#include "battle_command.h"
#include "battle_interface.h"
#include "battle_tower.h"
#include "battle_troop.h"
#include "kingdom.h"
#include "settings.h"
#include "spell.h"
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

        DEBUG( DBG_BATTLE, DBG_TRACE, "damage_range: unexpected spellPower value: " << spellPower << " for commander " << commander );
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

    TargetsInfo targets = GetTargetsForDamage( attacker, defender, dst );

    if ( Board::isReflectDirection( dir ) != attacker.isReflect() )
        attacker.UpdateDirection( board[dst].GetPos() );

    if ( interface )
        interface->RedrawActionAttackPart1( attacker, defender, targets );

    TargetsApplyDamage( attacker, defender, targets );
    if ( interface )
        interface->RedrawActionAttackPart2( attacker, targets );

    const Spell spell = attacker.GetSpellMagic();

    // magic attack
    if ( defender.isValid() && spell.isValid() ) {
        const std::string name( attacker.GetName() );
        targets = GetTargetsForSpells( attacker.GetCommander(), spell, defender.GetHeadIndex() );

        bool validSpell = true;
        if ( attacker == Monster::ARCHMAGE && !defender.Modes( IS_GOOD_MAGIC ) )
            validSpell = false;

        if ( targets.size() && validSpell ) {
            if ( interface )
                interface->RedrawActionSpellCastPart1( spell, defender.GetHeadIndex(), NULL, name, targets );

            if ( attacker == Monster::ARCHMAGE ) {
                if ( defender.Modes( IS_GOOD_MAGIC ) )
                    defender.ResetModes( IS_GOOD_MAGIC );
            }
            else {
                // magic attack not depends from hero
                TargetsApplySpell( NULL, spell, targets );
            }

            if ( interface )
                interface->RedrawActionSpellCastPart2( spell, targets );
            if ( interface )
                interface->RedrawActionMonsterSpellCastStatus( attacker, targets.front() );
        }
    }

    attacker.PostAttackAction( defender );
}

void Battle::Arena::ApplyAction( Command & cmd )
{
    switch ( cmd.GetType() ) {
    case MSG_BATTLE_CAST:
        ApplyActionSpellCast( cmd );
        break;
    case MSG_BATTLE_ATTACK:
        ApplyActionAttack( cmd );
        break;
    case MSG_BATTLE_MOVE:
        ApplyActionMove( cmd );
        break;
    case MSG_BATTLE_SKIP:
        ApplyActionSkip( cmd );
        break;
    case MSG_BATTLE_END_TURN:
        ApplyActionEnd( cmd );
        break;
    case MSG_BATTLE_MORALE:
        ApplyActionMorale( cmd );
        break;

    case MSG_BATTLE_TOWER:
        ApplyActionTower( cmd );
        break;
    case MSG_BATTLE_CATAPULT:
        ApplyActionCatapult( cmd );
        break;

    case MSG_BATTLE_RETREAT:
        ApplyActionRetreat( cmd );
        break;
    case MSG_BATTLE_SURRENDER:
        ApplyActionSurrender( cmd );
        break;

    case MSG_BATTLE_AUTO:
        ApplyActionAutoBattle( cmd );
        break;

    default:
        break;
    }
}

void Battle::Arena::ApplyActionSpellCast( Command & cmd )
{
    const Spell spell( cmd.GetValue() );
    HeroBase * current_commander = GetCurrentForce().GetCommander();

    if ( current_commander && current_commander->HaveSpellBook() && !current_commander->Modes( Heroes::SPELLCASTED ) && current_commander->CanCastSpell( spell )
         && spell.isCombat() ) {
        DEBUG( DBG_BATTLE, DBG_TRACE, current_commander->GetName() << ", color: " << Color::String( current_commander->GetColor() ) << ", spell: " << spell.GetName() );

        // uniq spells action
        switch ( spell() ) {
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

        current_commander->SetModes( Heroes::SPELLCASTED );
        current_commander->SpellCasted( spell );

        // save spell for "eagle eye" capability
        usage_spells.Append( spell );
    }
    else {
        DEBUG( DBG_BATTLE, DBG_INFO,
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
        DEBUG( DBG_BATTLE, DBG_TRACE, b1->String() << " to " << b2->String() );

        // reset blind
        if ( b2->Modes( SP_BLIND ) )
            b2->ResetBlind();

        const bool handfighting = Unit::isHandFighting( *b1, *b2 );
        // check position
        if ( b1->isArchers() || handfighting ) {
            // attack
            BattleProcess( *b1, *b2, dst, dir );

            if ( b2->isValid() ) {
                // defense answer
                if ( handfighting && !b1->ignoreRetaliation() && b2->AllowResponse() ) {
                    BattleProcess( *b2, *b1 );
                    b2->SetResponse();
                }

                // twice attack
                if ( b1->isValid() && b1->isTwiceAttack() && !b1->Modes( SP_BLIND | IS_PARALYZE_MAGIC ) ) {
                    DEBUG( DBG_BATTLE, DBG_TRACE, "twice attack" );
                    BattleProcess( *b1, *b2 );
                }
            }

            b1->UpdateDirection();
            b2->UpdateDirection();
        }
        else {
            DEBUG( DBG_BATTLE, DBG_WARN,
                   "incorrect param"
                       << ": " << b1->String( true ) << " and " << b2->String( true ) );
        }
    }
    else {
        DEBUG( DBG_BATTLE, DBG_WARN,
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
    const int32_t path_size = cmd.GetValue();

    Battle::Unit * b = GetTroopUID( uid );
    Cell * cell = Board::GetCell( dst );

    if ( b && b->isValid() && cell && cell->isPassable3( *b, false ) ) {
        Position pos1, pos2;
        const s32 head = b->GetHeadIndex();
        pos1 = Position::GetCorrect( *b, dst );

        DEBUG( DBG_BATTLE, DBG_TRACE,
               b->String() << ", dst: " << dst << ", (head: " << pos1.GetHead()->GetIndex() << ", tail: " << ( b->isWide() ? pos1.GetTail()->GetIndex() : -1 ) << ")" );

        if ( b->isFlying() ) {
            b->UpdateDirection( pos1.GetRect() );
            if ( b->isReflect() != pos1.isReflect() )
                pos1.Swap();
            if ( interface )
                interface->RedrawActionFly( *b, pos1 );
            pos2 = pos1;
        }
        else {
            Indexes path;

            // check path
            if ( 0 == path_size ) {
                path = GetPath( *b, pos1 );
                cmd = Command( MSG_BATTLE_MOVE, b->GetUID(), dst, path );
            }
            else
                for ( int index = 0; index < path_size; ++index )
                    path.push_back( cmd.GetValue() );

            if ( path.empty() ) {
                DEBUG( DBG_BATTLE, DBG_WARN,
                       "path empty, " << b->String() << " to "
                                      << "dst: " << dst );
                return;
            }

            if ( interface )
                interface->RedrawActionMove( *b, path );
            else if ( bridge ) {
                for ( Indexes::const_iterator pathIt = path.begin(); pathIt != path.end(); ++pathIt ) {
                    bool doMovement = false;

                    if ( bridge && bridge->NeedDown( *b, *pathIt ) )
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
                    if ( bridge && bridge->AllowUp() )
                        bridge->Action( *b, *pathIt );
                }
            }

            if ( b->isWide() ) {
                const s32 dst1 = path.back();
                const s32 dst2 = 1 < path.size() ? path[path.size() - 2] : head;

                pos2.Set( dst1, b->isWide(), RIGHT_SIDE & Board::GetDirection( dst1, dst2 ) );
            }
            else
                pos2.Set( path.back(), false, b->isReflect() );
        }

        b->SetPosition( pos2 );
        b->UpdateDirection();
    }
    else {
        DEBUG( DBG_BATTLE, DBG_WARN,
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
            if ( hard ) {
                battle->SetModes( TR_HARDSKIP );
                battle->SetModes( TR_SKIPMOVE );
                battle->SetModes( TR_MOVED );
                if ( Settings::Get().ExtBattleSkipIncreaseDefense() )
                    battle->SetModes( TR_DEFENSED );
            }
            else
                battle->SetModes( battle->Modes( TR_SKIPMOVE ) ? TR_MOVED : TR_SKIPMOVE );

            if ( interface )
                interface->RedrawActionSkipStatus( *battle );

            DEBUG( DBG_BATTLE, DBG_TRACE, battle->String() );
        }
        else {
            DEBUG( DBG_BATTLE, DBG_WARN, "uid: " << uid << " moved" );
        }
    }
    else {
        DEBUG( DBG_BATTLE, DBG_WARN,
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

            DEBUG( DBG_BATTLE, DBG_TRACE, battle->String() );
        }
        else {
            DEBUG( DBG_BATTLE, DBG_INFO, "uid: " << uid << " moved" );
        }
    }
    else {
        DEBUG( DBG_BATTLE, DBG_INFO,
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
            interface->RedrawActionMorale( *b, morale );

        DEBUG( DBG_BATTLE, DBG_TRACE, ( morale ? "good" : "bad" ) << " to " << b->String() );
    }
    else {
        DEBUG( DBG_BATTLE, DBG_WARN,
               "incorrect param"
                   << ": "
                   << "uid: "
                   << "0x" << std::setw( 8 ) << std::setfill( '0' ) << std::hex << uid );
    }
}

void Battle::Arena::ApplyActionRetreat( Command & /*cmd*/ )
{
    if ( CanRetreatOpponent( current_color ) ) {
        if ( army1->GetColor() == current_color ) {
            result_game.army1 = RESULT_RETREAT;
        }
        else if ( army2->GetColor() == current_color ) {
            result_game.army2 = RESULT_RETREAT;
        }
        DEBUG( DBG_BATTLE, DBG_TRACE, "color: " << Color::String( current_color ) );
    }
    else {
        DEBUG( DBG_BATTLE, DBG_WARN, "CanRetreatOpponent check failed" );
    }
}

void Battle::Arena::ApplyActionSurrender( Command & /*cmd*/ )
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
            DEBUG( DBG_BATTLE, DBG_TRACE, "color: " << Color::String( current_color ) );
        }
    }
    else {
        DEBUG( DBG_BATTLE, DBG_WARN, "incorrect param" );
    }
}

void Battle::Arena::TargetsApplyDamage( Unit & attacker, Unit & /*defender*/, TargetsInfo & targets )
{
    for ( TargetsInfo::iterator it = targets.begin(); it != targets.end(); ++it ) {
        TargetInfo & target = *it;
        if ( target.defender )
            target.killed = target.defender->ApplyDamage( attacker, target.damage );
    }
}

Battle::TargetsInfo Battle::Arena::GetTargetsForDamage( Unit & attacker, Unit & defender, s32 dst )
{
    TargetsInfo targets;
    targets.reserve( 8 );

    Unit * enemy = NULL;
    Cell * cell = NULL;
    TargetInfo res;

    // first target
    res.defender = &defender;
    res.damage = attacker.GetDamage( defender );

    // Genie special attack
    if ( attacker.GetID() == Monster::GENIE && Rand::Get( 1, 10 ) == 2 && defender.GetHitPoints() / 2 > res.damage ) {
        // Replaces the damage, not adding to it
        res.damage = defender.GetHitPoints() / 2;

        if ( Arena::GetInterface() ) {
            std::string str( _( "%{name} half the enemy troops!" ) );
            StringReplace( str, "%{name}", attacker.GetName() );
            Arena::GetInterface()->SetStatus( str, true );
        }
    }
    targets.push_back( res );

    // long distance attack
    if ( attacker.isDoubleCellAttack() ) {
        const int dir = Board::GetDirection( attacker.GetHeadIndex(), dst );
        if ( !defender.isWide() || 0 == ( ( RIGHT | LEFT ) & dir ) ) {
            if ( NULL != ( cell = Board::GetCell( dst, dir ) ) && NULL != ( enemy = cell->GetUnit() ) && enemy != &defender ) {
                res.defender = enemy;
                res.damage = attacker.GetDamage( *enemy );
                targets.push_back( res );
            }
        }
    }
    // around hydra
    else if ( attacker.GetID() == Monster::HYDRA ) {
        const Indexes around = Board::GetAroundIndexes( attacker );

        for ( Indexes::const_iterator it = around.begin(); it != around.end(); ++it ) {
            if ( NULL != ( enemy = Board::GetCell( *it )->GetUnit() ) && enemy != &defender && enemy->GetColor() != attacker.GetColor() ) {
                res.defender = enemy;
                res.damage = attacker.GetDamage( *enemy );
                targets.push_back( res );
            }
        }
    }
    // lich cloud damages
    else if ( ( attacker.GetID() == Monster::LICH || attacker.GetID() == Monster::POWER_LICH ) && !attacker.isHandFighting() ) {
        if ( defender.GetHeadIndex() == dst || defender.GetTailIndex() == dst ) {
            const Indexes around = Board::GetAroundIndexes( dst );

            for ( Indexes::const_iterator it = around.begin(); it != around.end(); ++it ) {
                if ( NULL != ( enemy = Board::GetCell( *it )->GetUnit() ) && enemy != &defender ) {
                    res.defender = enemy;
                    res.damage = attacker.GetDamage( *enemy );
                    targets.push_back( res );
                }
            }
        }
        else {
            DEBUG( DBG_BATTLE, DBG_TRACE, "Lich shot at a cell where no mosnter exists: " << dst );
        }
    }

    return targets;
}

void Battle::Arena::TargetsApplySpell( const HeroBase * hero, const Spell & spell, TargetsInfo & targets )
{
    DEBUG( DBG_BATTLE, DBG_TRACE, "targets: " << targets.size() );

    TargetsInfo::iterator it = targets.begin();

    for ( ; it != targets.end(); ++it ) {
        TargetInfo & target = *it;
        if ( target.defender )
            target.defender->ApplySpell( spell, hero, target );
    }
}

Battle::TargetsInfo Battle::Arena::GetTargetsForSpells( const HeroBase * hero, const Spell & spell, s32 dst )
{
    TargetsInfo targets;
    targets.reserve( 8 );

    TargetInfo res;
    Unit * target = GetTroopBoard( dst );

    // from spells
    switch ( spell() ) {
    case Spell::CHAINLIGHTNING:
    case Spell::COLDRING:
        // skip center
        target = NULL;
        break;

    default:
        break;
    }

    // first target
    if ( target && target->AllowApplySpell( spell, hero ) ) {
        res.defender = target;
        targets.push_back( res );
    }

    // resurrect spell? get target from graveyard
    if ( NULL == target && GraveyardAllowResurrect( dst, spell ) ) {
        target = GetTroopUID( graveyard.GetLastTroopUID( dst ) );

        if ( target && target->AllowApplySpell( spell, hero ) ) {
            res.defender = target;
            targets.push_back( res );
        }
    }
    else
        // check other spells
        switch ( spell() ) {
        case Spell::CHAINLIGHTNING: {
            uint32_t currentMonsterPos = dst;

            Indexes trgts;
            trgts.reserve( 12 );
            trgts.push_back( currentMonsterPos );

            Indexes ignoredMonster;
            ignoredMonster.push_back( currentMonsterPos );

            // find targets
            while ( trgts.size() < 4 ) {
                const Indexes nearestPosIds = board.GetNearestTroopIndexes( currentMonsterPos, &ignoredMonster );
                if ( nearestPosIds.empty() )
                    break;

                Indexes sortedIds;

                for ( size_t monsterId = 0; monsterId < nearestPosIds.size(); ++monsterId ) {
                    Unit * targetUnit = GetTroopBoard( nearestPosIds[monsterId] );
                    if ( targetUnit != NULL && ( targetUnit->GetMagicResist( spell, hero ? hero->GetPower() : 0 ) < 100 ) ) {
                        sortedIds.push_back( nearestPosIds[monsterId] );
                    }
                    ignoredMonster.push_back( nearestPosIds[monsterId] );
                }

                if ( sortedIds.empty() ) {
                    continue;
                }

                const uint32_t chosenMonsterPos = sortedIds.size() > 1 ? *Rand::Get( sortedIds ) : sortedIds.front();
                trgts.push_back( chosenMonsterPos );
                currentMonsterPos = chosenMonsterPos;
            }

            // save targets
            for ( Indexes::iterator it = trgts.begin(); it != trgts.end(); ++it ) {
                Unit * targetDefender = GetTroopBoard( *it );

                if ( targetDefender ) {
                    res.defender = targetDefender;
                    // store temp priority for calculate damage
                    res.damage = std::distance( trgts.begin(), it );
                    targets.push_back( res );
                }
            }
        } break;

        // check abroads
        case Spell::FIREBALL:
        case Spell::METEORSHOWER:
        case Spell::COLDRING:
        case Spell::FIREBLAST: {
            const Indexes positions = Board::GetDistanceIndexes( dst, ( spell == Spell::FIREBLAST ? 2 : 1 ) );

            for ( Indexes::const_iterator it = positions.begin(); it != positions.end(); ++it ) {
                Unit * targetUnit = GetTroopBoard( *it );
                if ( targetUnit && targetUnit->AllowApplySpell( spell, hero ) ) {
                    res.defender = targetUnit;
                    targets.push_back( res );
                }
            }

            // unique
            targets.resize( std::distance( targets.begin(), std::unique( targets.begin(), targets.end() ) ) );
        } break;

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
            for ( Board::iterator it = board.begin(); it != board.end(); ++it ) {
                target = ( *it ).GetUnit();
                if ( target && target->AllowApplySpell( spell, hero ) ) {
                    res.defender = target;
                    targets.push_back( res );
                }
            }

            // unique
            targets.resize( std::distance( targets.begin(), std::unique( targets.begin(), targets.end() ) ) );
        } break;

        default:
            break;
        }

    // remove resistent magic troop
    TargetsInfo::iterator it = targets.begin();
    while ( it != targets.end() ) {
        const u32 resist = ( *it ).defender->GetMagicResist( spell, hero ? hero->GetPower() : 0 );

        if ( 0 < resist && 100 > resist && resist >= Rand::Get( 1, 100 ) ) {
            if ( interface )
                interface->RedrawActionResistSpell( *( *it ).defender );

            it = targets.erase( it );
        }
        else
            ++it;
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
        DEBUG( DBG_BATTLE, DBG_TRACE, "tower: " << type << ", attack to " << b2->String() );

        TargetInfo target;
        target.defender = b2;
        target.damage = tower->GetDamage( *b2 );

        if ( interface )
            interface->RedrawActionTowerPart1( *tower, *b2 );
        target.killed = b2->ApplyDamage( *tower, target.damage );
        if ( interface )
            interface->RedrawActionTowerPart2( target );

        if ( b2->Modes( SP_BLIND ) )
            b2->ResetBlind();
    }
    else {
        DEBUG( DBG_BATTLE, DBG_WARN,
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
            u32 target = cmd.GetValue();
            u32 damage = cmd.GetValue();

            if ( target ) {
                if ( interface )
                    interface->RedrawActionCatapult( target );
                SetCastleTargetValue( target, GetCastleTargetValue( target ) - damage );
                DEBUG( DBG_BATTLE, DBG_TRACE, "target: " << target );
            }
        }
    }
    else {
        DEBUG( DBG_BATTLE, DBG_WARN, "incorrect param" );
    }
}

void Battle::Arena::ApplyActionAutoBattle( Command & cmd )
{
    const int color = cmd.GetValue();
    if ( current_color != color ) {
        DEBUG( DBG_BATTLE, DBG_WARN, "incorrect param" );
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

void Battle::Arena::ApplyActionSpellSummonElemental( Command & /*cmd*/, const Spell & spell )
{
    Unit * elem = CreateElemental( spell );
    if ( interface )
        interface->RedrawActionSummonElementalSpell( *elem );
}

void Battle::Arena::ApplyActionSpellDefaults( Command & cmd, const Spell & spell )
{
    const HeroBase * current_commander = GetCurrentCommander();
    if ( !current_commander )
        return;

    const int32_t dst = cmd.GetValue();

    TargetsInfo targets = GetTargetsForSpells( current_commander, spell, dst );
    if ( interface )
        interface->RedrawActionSpellCastPart1( spell, dst, current_commander, current_commander->GetName(), targets );

    TargetsApplySpell( current_commander, spell, targets );
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
        Position pos = Position::GetCorrect( *b, dst );
        if ( b->isReflect() != pos.isReflect() )
            pos.Swap();

        if ( interface )
            interface->RedrawActionTeleportSpell( *b, pos.GetHead()->GetIndex() );

        b->SetPosition( pos );
        b->UpdateDirection();

        DEBUG( DBG_BATTLE, DBG_TRACE, "spell: " << spell.GetName() << ", src: " << src << ", dst: " << dst );
    }
    else {
        DEBUG( DBG_BATTLE, DBG_WARN, "spell: " << spell.GetName() << " false" );
    }
}

void Battle::Arena::ApplyActionSpellEarthQuake( Command & /*cmd*/ )
{
    std::vector<int> targets = GetCastleTargets();
    if ( interface ) {
        interface->RedrawActionEarthQuakeSpell( targets );
    }

    const HeroBase * commander = GetCurrentCommander();
    const std::pair<int, int> range = commander ? getEarthquakeDamageRange( commander ) : std::make_pair( 0 , 0 );
    const std::vector<int> wallHexPositions = {FIRST_WALL_HEX_POSITION, SECOND_WALL_HEX_POSITION, THIRD_WALL_HEX_POSITION, FORTH_WALL_HEX_POSITION};
    for ( int position : wallHexPositions ) {
        if ( 0 != board[position].GetObject() ) {
            board[position].SetObject( Rand::Get( range.first, range.second ) );
        }
    }

    if ( towers[0] && towers[0]->isValid() && Rand::Get( 1 ) )
        towers[0]->SetDestroy();
    if ( towers[2] && towers[2]->isValid() && Rand::Get( 1 ) )
        towers[2]->SetDestroy();

    DEBUG( DBG_BATTLE, DBG_TRACE, "spell: " << Spell( Spell::EARTHQUAKE ).GetName() << ", targets: " << targets.size() );
}



void Battle::Arena::ApplyActionSpellMirrorImage( Command & cmd )
{
    const int32_t who = cmd.GetValue();
    Unit * troop = GetTroopBoard( who );

    if ( troop != NULL ) {
        Indexes distances = Board::GetDistanceIndexes( troop->GetHeadIndex(), 4 );

        const int32_t centerIndex = troop->GetHeadIndex();
        std::sort( distances.begin(), distances.end(), [centerIndex]( const int32_t index1, const int32_t index2 ) {
            return Battle::Board::GetDistance( centerIndex, index1 ) < Battle::Board::GetDistance( centerIndex, index2 );
        } );

        Indexes::const_iterator it
            = std::find_if( distances.begin(), distances.end(), [troop]( const int32_t v ) { return Battle::Board::isValidMirrorImageIndex( v, troop ); } );
        if ( it != distances.end() ) {
            const Position pos = Position::GetCorrect( *troop, *it );
            const s32 dst = pos.GetHead()->GetIndex();
            DEBUG( DBG_BATTLE, DBG_TRACE, "set position: " << dst );
            if ( interface )
                interface->RedrawActionMirrorImageSpell( *troop, pos );
            Unit * mirror = CreateMirrorImage( *troop, dst );
            if ( mirror )
                mirror->SetPosition( pos );
        }
        else {
            if ( interface )
                interface->SetStatus( _( "spell failed!" ), true );
            DEBUG( DBG_BATTLE, DBG_WARN, "new position not found!" );
        }
    }
    else {
        DEBUG( DBG_BATTLE, DBG_WARN, "spell: " << Spell( Spell::MIRRORIMAGE ).GetName() << " false" );
    }
}
