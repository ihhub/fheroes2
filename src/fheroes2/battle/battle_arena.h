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

#ifndef H2BATTLE_ARENA_H
#define H2BATTLE_ARENA_H

#include <array>
#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include "battle.h"
#include "battle_board.h"
#include "battle_cell.h"
#include "battle_command.h"
#include "battle_grave.h"
#include "battle_pathfinding.h"
#include "icn.h"
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
    class Interface;
    class Status;
    class Tower;
    class Unit;
    class Units;

    enum class TowerType : uint8_t;

    enum class SiegeWeaponType
    {
        Catapult,
        EarthquakeSpell
    };

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
        Arena( Army & army1, Army & army2, const int32_t tileIndex, const bool isShowInterface, Rand::DeterministicRandomGenerator & randomGenerator );
        Arena( const Arena & ) = delete;
        Arena( Arena && ) = delete;

        ~Arena();

        Arena & operator=( const Arena & ) = delete;
        Arena & operator=( Arena && ) = delete;

        void Turns();
        bool BattleValid() const;

        bool AutoBattleInProgress() const;
        bool EnemyOfAIHasAutoBattleInProgress() const;
        bool CanToggleAutoBattle() const;

        uint32_t GetTurnNumber() const
        {
            return _turnNumber;
        }

        Result & GetResult();

        HeroBase * GetCommander1() const;
        HeroBase * GetCommander2() const;

        const HeroBase * getCommander( const int color ) const;
        const HeroBase * getEnemyCommander( const int color ) const;
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

        const SpellStorage & GetUsedSpells() const;

        bool DialogBattleSummary( const Result & res, const std::vector<Artifact> & artifacts, const bool allowToRestart ) const;
        int DialogBattleHero( HeroBase & hero, const bool buttons, Status & status ) const;
        static void DialogBattleNecromancy( const uint32_t raiseCount );

        void FadeArena( bool clearMessageLog ) const;

        // Checks whether the given position is reachable for the given unit, either on the current turn or in principle
        bool isPositionReachable( const Unit & unit, const Position & position, const bool isOnCurrentTurn )
        {
            return _battlePathfinder.isPositionReachable( unit, position, isOnCurrentTurn );
        }

        // Returns the cost of moving to the given position for the given unit. It's the caller's responsibility to make
        // sure that this position is reachable before calling this method.
        uint32_t CalculateMoveCost( const Unit & unit, const Position & position )
        {
            return _battlePathfinder.getCost( unit, position );
        }

        // Returns the distance to the given position (i.e. the number of movements that need to be performed to get to
        // this position, the reversal of a wide unit is not considered as a movement) for the given unit. For flying
        // units, this distance is estimated as the straight line distance to the given position. It's the caller's
        // responsibility to make sure that this position is reachable before calling this method.
        uint32_t CalculateMoveDistance( const Unit & unit, const Position & position )
        {
            return _battlePathfinder.getDistance( unit, position );
        }

        // Returns the path (or its part) for the given unit to the given position that can be traversed during the
        // current turn. If this position is unreachable by this unit, then an empty path is returned.
        Indexes GetPath( const Unit & unit, const Position & position );

        // Returns the indexes of all cells that can be occupied by the given unit's head on the current turn
        Indexes getAllAvailableMoves( const Unit & unit )
        {
            return _battlePathfinder.getAllAvailableMoves( unit );
        }

        // Returns the position on the path for the given unit to the given position, which is reachable on the current
        // turn and is as close as possible to the destination (excluding the current position of the unit). If the given
        // position is unreachable by the given unit, then an empty Position object is returned.
        Position getClosestReachablePosition( const Unit & unit, const Position & position )
        {
            return _battlePathfinder.getClosestReachablePosition( unit, position );
        }

        void ApplyAction( Command & );

        // Returns a list of targets that will be affected by the given spell casted by the given hero and applied
        // to a cell with a given index. This method can be used by external code to evaluate the applicability of
        // a spell, and does not use probabilistic mechanisms to determine units resisting the given spell.
        TargetsInfo GetTargetsForSpell( const HeroBase * hero, const Spell & spell, const int32_t dst )
        {
            return GetTargetsForSpell( hero, spell, dst, false, nullptr );
        }

        bool isSpellcastDisabled() const;
        bool isDisableCastSpell( const Spell & spell, std::string * msg = nullptr ) const;

        bool isAbleToResurrectFromGraveyard( const int32_t index, const Spell & spell ) const;

        Indexes getCellsOccupiedByGraveyard() const;
        std::vector<const Unit *> getGraveyardUnits( const int32_t index ) const;

        // Returns the unit that died last on the cell with the given index, or nullptr if there is no such unit.
        const Unit * getLastUnitFromGraveyard( const int32_t index ) const;

        // Returns the last dead unit on the cell with the given index, which can be potentially affected by any resurrection
        // spell during the current turn, or nullptr if there is no such unit.
        const Unit * getLastResurrectableUnitFromGraveyard( const int32_t index ) const;

        // Returns the last dead unit on the cell with the given index, which can be affected by the given resurrection spell
        // during the current turn, or nullptr if there is no such unit.
        Unit * getLastResurrectableUnitFromGraveyard( const int32_t index, const Spell & spell ) const;

        bool CanSurrenderOpponent( int color ) const;
        bool CanRetreatOpponent( int color ) const;

        bool IsShootingPenalty( const Unit &, const Unit & ) const;

        int GetICNCovr() const
        {
            return _covrIcnId;
        }

        int32_t GetFreePositionNearHero( const int heroColor ) const;

        static Board * GetBoard();
        static Tower * GetTower( const TowerType type );
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
        void UnitTurn( const Units & orderHistory );

        void TowerAction( const Tower & );
        void CatapultAction();

        // Returns the remaining number of hitpoints of the given castle defense structure from the point of view of the given siege weapon.
        int getCastleDefenseStructureCondition( const CastleDefenseStructure target, const SiegeWeaponType siegeWeapon ) const;

        // Applies the specified damage to the given castle defense structure. It's the caller's responsibility to make sure that this defense
        // structure still has enough hitpoints.
        void applyDamageToCastleDefenseStructure( const CastleDefenseStructure target, const int damage );

        TargetsInfo GetTargetsForDamage( const Unit & attacker, Unit & defender, const int32_t dst, const int dir ) const;
        TargetsInfo GetTargetsForSpell( const HeroBase * hero, const Spell & spell, const int32_t dst, bool applyRandomMagicResistance, bool * playResistSound );

        static void TargetsApplyDamage( Unit & attacker, TargetsInfo & targets, uint32_t & resurrected );
        static void TargetsApplySpell( const HeroBase * hero, const Spell & spell, TargetsInfo & targets );

        TargetsInfo TargetsForChainLightning( const HeroBase * hero, const int32_t attackedTroopIndex, const bool applyRandomMagicResistance );
        std::vector<Unit *> FindChainLightningTargetIndexes( const HeroBase * hero, Unit * firstUnit, const bool applyRandomMagicResistance );

        void ApplyActionRetreat( const Command & cmd );
        void ApplyActionSurrender( const Command & cmd );
        void ApplyActionAttack( Command & cmd );
        void ApplyActionMove( Command & cmd );
        void ApplyActionSkip( Command & cmd );
        void ApplyActionMorale( Command & cmd );
        void ApplyActionSpellCast( Command & cmd );
        void ApplyActionTower( Command & cmd );
        void ApplyActionCatapult( Command & cmd );
        void ApplyActionAutoSwitch( Command & cmd );
        void ApplyActionAutoFinish( const Command & cmd );

        void ApplyActionSpellSummonElemental( const Command & cmd, const Spell & spell );
        void ApplyActionSpellMirrorImage( Command & cmd );
        void ApplyActionSpellTeleport( Command & cmd );
        void ApplyActionSpellEarthquake( const Command & cmd );
        void ApplyActionSpellDefaults( Command & cmd, const Spell & spell );

        // Moves the given unit to a position where the index of the head cell is equal to 'dst'. If 'dst' is -1,
        // then this method does nothing. Otherwise, it's the caller's responsibility to make sure that this position
        // is reachable for the given unit on the current turn before calling this method.
        void moveUnit( Unit * unit, const int32_t dst );

        // Performs an actual attack of one unit (defender) by another unit (attacker), applying the attacker's
        // built-in magic, if necessary. If the specified index of the target cell of the attack (tgt) is negative,
        // then an attempt will be made to calculate it automatically based on the adjacency of the unit cells. If
        // the specified direction of the attack (dir) is negative, then an attempt will be made to calculate it
        // automatically. When an attack is made by firing a shot, the dir should be UNKNOWN (zero).
        void BattleProcess( Unit & attacker, Unit & defender, int32_t tgt = -1, int dir = -1 );

        // Creates and returns a fully combat-ready elemental, which will be already placed on the board. It's
        // the caller's responsibility to make sure that a given spell is capable of creating an elemental
        // before calling this method.
        Unit * CreateElemental( const Spell & spell );
        // Creates and returns a mirror image of a given unit. The returned mirror image will have an invalid
        // position, which should be updated separately.
        Unit * CreateMirrorImage( Unit & unit );

        std::unique_ptr<Force> _army1;
        std::unique_ptr<Force> _army2;
        std::shared_ptr<Units> _orderOfUnits;

        // The unit that is currently active. Please note that some battle actions (e.g. catapult or castle tower shots) can be performed without an active unit.
        Unit * _currentUnit{ nullptr };

        // The color of the army of the last unit that performed a full-fledged action (skipping a turn due to
        // bad morale is not considered as such).
        int _lastActiveUnitArmyColor{ -1 };

        const Castle * castle;
        // Is the battle taking place in a town or a castle
        const bool _isTown;

        std::array<std::unique_ptr<Tower>, 3> _towers;
        std::unique_ptr<Catapult> _catapult;
        std::unique_ptr<Bridge> _bridge;

        std::unique_ptr<Interface> _interface;
        Result result_game;

        Graveyard _graveyard;
        SpellStorage _usedSpells;

        Board board;
        BattlePathfinder _battlePathfinder;
        int _covrIcnId{ ICN::UNKNOWN };

        uint32_t _turnNumber{ 0 };
        // A set of colors of players for whom the auto-battle mode is enabled
        int _autoBattleColors{ 0 };

        // This random number generator should only be used in code that is equally used by both AI and the human
        // player - that is, in code related to the processing of battle commands. It cannot be safely used in other
        // places (for example, in code that performs situation assessment or AI decision-making) because in this
        // case the battles performed by AI will not be reproducible by a human player when performing exactly the
        // same actions.
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
