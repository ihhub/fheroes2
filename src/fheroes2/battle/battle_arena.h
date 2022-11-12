/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
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

#ifndef H2BATTLE_ARENA_H
#define H2BATTLE_ARENA_H

#include <array>
#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "battle.h"
#include "battle_board.h"
#include "battle_command.h"
#include "battle_grave.h"
#include "battle_pathfinding.h"
#include "battle_tower.h"
#include "spell.h"
#include "spell_storage.h"

class Army;
class Artifact;
class Castle;
class HeroBase;

namespace Rand
{
    class DeterministicRandomGenerator;
}

namespace Battle
{
    class Bridge;
    class Catapult;
    class Force;
    class Position;
    class Units;
    class Unit;
    class Interface;
    class Status;

    class Actions : public std::list<Command>
    {};

    class TroopsUidGenerator
    {
    public:
        TroopsUidGenerator() = default;
        TroopsUidGenerator( const TroopsUidGenerator & ) = delete;

        TroopsUidGenerator & operator=( const TroopsUidGenerator & ) = delete;

        uint32_t GetUnique()
        {
            return _id++;
        }

    private:
        uint32_t _id{ 1 };
    };

    class Arena
    {
    public:
        Arena( Army & a1, Army & a2, int32_t index, bool local, Rand::DeterministicRandomGenerator & randomGenerator );
        Arena( const Arena & ) = delete;
        Arena( Arena && ) = delete;

        ~Arena();

        Arena & operator=( const Arena & ) = delete;
        Arena & operator=( Arena && ) = delete;

        void Turns();
        bool BattleValid() const;

        bool AutoBattleInProgress() const;
        bool CanToggleAutoBattle() const;

        uint32_t GetCurrentTurn() const;
        Result & GetResult();

        const HeroBase * getCommander( const int color ) const;
        const HeroBase * getEnemyCommander( const int color ) const;
        const HeroBase * GetCommander1() const;
        const HeroBase * GetCommander2() const;
        const HeroBase * GetCurrentCommander() const;

        Force & GetForce1() const;
        Force & GetForce2() const;
        Force & getForce( const int color ) const;
        Force & getEnemyForce( const int color ) const;
        Force & GetCurrentForce() const;

        int GetArmy1Color() const;
        int GetArmy2Color() const;
        int GetCurrentColor() const;
        // Returns the color of the army opposite to the army of the given color. If there is no army of the given color,
        // returns the color of the attacking army.
        int GetOppositeColor( const int col ) const;

        Unit * GetTroopBoard( int32_t );
        const Unit * GetTroopBoard( int32_t ) const;

        Unit * GetTroopUID( uint32_t );
        const Unit * GetTroopUID( uint32_t ) const;

        const SpellStorage & GetUsageSpells() const;

        bool DialogBattleSummary( const Result & res, const std::vector<Artifact> & artifacts, bool allowToCancel ) const;
        int DialogBattleHero( const HeroBase & hero, const bool buttons, Status & status ) const;
        void DialogBattleNecromancy( const uint32_t raiseCount, const uint32_t raisedMonsterType ) const;

        void FadeArena( bool clearMessageLog ) const;

        // returns pair with move cell index and distance
        std::pair<int, uint32_t> CalculateMoveToUnit( const Unit & target ) const;

        uint32_t CalculateMoveDistance( int32_t indexTo ) const;
        bool hexIsPassable( int32_t indexTo ) const;
        Indexes getAllAvailableMoves( uint32_t moveRange ) const;
        Indexes CalculateTwoMoveOverlap( int32_t indexTo, uint32_t movementRange = 0 ) const;
        Indexes GetPath( const Unit &, const Position & ) const;

        // Returns the cell nearest to the end of the path to the cell with the given index (according to the AIBattlePathfinder)
        // and reachable for the current unit (to which the current board passability information relates) or -1 if the cell with
        // the given index is unreachable in principle
        int32_t GetNearestReachableCell( const Unit & currentUnit, const int32_t dst ) const;

        void ApplyAction( Command & );

        TargetsInfo GetTargetsForSpells( const HeroBase * hero, const Spell & spell, int32_t dest, bool * playResistSound = nullptr );

        bool isSpellcastDisabled() const;
        bool isDisableCastSpell( const Spell &, std::string * msg = nullptr );

        bool GraveyardAllowResurrect( int32_t, const Spell & ) const;
        const Unit * GraveyardLastTroop( int32_t ) const;
        std::vector<const Unit *> GetGraveyardTroops( const int32_t hexIndex ) const;
        Indexes GraveyardClosedCells() const;

        bool CanSurrenderOpponent( int color ) const;
        bool CanRetreatOpponent( int color ) const;

        void ApplyActionSpellSummonElemental( const Command &, const Spell & );
        void ApplyActionSpellMirrorImage( Command & );
        void ApplyActionSpellTeleport( Command & );
        void ApplyActionSpellEarthQuake( const Command & );
        void ApplyActionSpellDefaults( Command &, const Spell & );

        bool IsShootingPenalty( const Unit &, const Unit & ) const;
        int GetICNCovr() const;

        uint32_t GetCastleTargetValue( int ) const;

        int32_t GetFreePositionNearHero( const int heroColor ) const;

        const Rand::DeterministicRandomGenerator & GetRandomGenerator() const;

        static Board * GetBoard();
        static Tower * GetTower( int );
        static Bridge * GetBridge();
        static const Castle * GetCastle();
        static Interface * GetInterface();
        static Graveyard * GetGraveyard();

        static bool isAnyTowerPresent();

        enum
        {
            CATAPULT_POS = 77,
            CASTLE_GATE_POS = 50,
            CASTLE_FIRST_TOP_WALL_POS = 8,
            CASTLE_SECOND_TOP_WALL_POS = 29,
            CASTLE_THIRD_TOP_WALL_POS = 73,
            CASTLE_FOURTH_TOP_WALL_POS = 96,
            CASTLE_TOP_ARCHER_TOWER_POS = 19,
            CASTLE_BOTTOM_ARCHER_TOWER_POS = 85,
            CASTLE_TOP_GATE_TOWER_POS = 40,
            CASTLE_BOTTOM_GATE_TOWER_POS = 62
        };

    private:
        void RemoteTurn( const Unit &, Actions & );
        void HumanTurn( const Unit &, Actions & );

        void TurnTroop( Unit * troop, const Units & orderHistory );
        void TowerAction( const Tower & );

        void SetCastleTargetValue( int, uint32_t );
        void CatapultAction();

        TargetsInfo GetTargetsForDamage( const Unit & attacker, Unit & defender, const int32_t dst, const int dir ) const;

        static void TargetsApplyDamage( Unit & attacker, TargetsInfo & targets );
        static void TargetsApplySpell( const HeroBase * hero, const Spell & spell, TargetsInfo & targets );

        std::vector<int> GetCastleTargets() const;
        TargetsInfo TargetsForChainLightning( const HeroBase * hero, int32_t attackedTroopIndex );
        std::vector<Unit *> FindChainLightningTargetIndexes( const HeroBase * hero, Unit * firstUnit );

        void ApplyActionRetreat( const Command & );
        void ApplyActionSurrender( const Command & );
        void ApplyActionAttack( Command & );
        void ApplyActionMove( Command & );
        void ApplyActionEnd( Command & );
        void ApplyActionSkip( Command & );
        void ApplyActionMorale( Command & );
        void ApplyActionSpellCast( Command & );
        void ApplyActionTower( Command & );
        void ApplyActionCatapult( Command & );
        void ApplyActionAutoSwitch( Command & cmd );
        void ApplyActionAutoFinish( const Command & cmd );

        // Performs an actual attack of one unit (defender) by another unit (attacker), applying the attacker's
        // built-in magic, if necessary. If the given index of the target cell of the attack (dst) is negative,
        // then an attempt will be made to calculate it automatically based on the adjacency of the unit cells.
        // If the given direction of the attack (dir) is negative, then an attempt will be made to calculate it
        // automatically. When an attack is made by firing a shot, the dir should be UNKNOWN (zero).
        void BattleProcess( Unit & attacker, Unit & defender, int32_t dst = -1, int dir = -1 );

        // Creates and returns a fully combat-ready elemental, which will be already placed on the board. It's
        // the caller's responsibility to make sure that this elemental can be created using the given spell
        // before calling this method.
        Unit * CreateElemental( const Spell & spell );
        // Creates and returns a mirror image of the given unit. The returned mirror image will have an invalid
        // position, which should be updated separately.
        Unit * CreateMirrorImage( Unit & unit );

        std::unique_ptr<Force> _army1;
        std::unique_ptr<Force> _army2;
        std::shared_ptr<Units> _orderOfUnits;

        int current_color;
        // The color of the army of the last unit that performed a full-fledged action (skipping a turn due to
        // bad morale is not considered as such)
        int _lastActiveUnitArmyColor;

        const Castle * castle;
        const bool _isTown; // If the battle is in town (village or castle).

        std::array<std::unique_ptr<Tower>, 3> _towers;
        std::unique_ptr<Catapult> _catapult;
        std::unique_ptr<Bridge> _bridge;

        std::unique_ptr<Interface> _interface;
        Result result_game;

        Graveyard graveyard;
        SpellStorage usage_spells;

        Board board;
        AIBattlePathfinder _globalAIPathfinder;
        int icn_covr;

        uint32_t current_turn;
        // A set of colors of players for whom the auto-battle mode is enabled
        int _autoBattleColors;

        Rand::DeterministicRandomGenerator & _randomGenerator;

        TroopsUidGenerator _uidGenerator;

        enum
        {
            CHAIN_LIGHTNING_CREATURE_COUNT = 4
        };
    };

    Arena * GetArena();
}

#endif
