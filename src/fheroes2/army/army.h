/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2024                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "monster.h"
#include "players.h"

class IStreamBase;
class OStreamBase;

class Castle;
class HeroBase;
class Heroes;
class Troop;

namespace Maps
{
    class Tile;
}

class Troops : protected std::vector<Troop *>
{
public:
    Troops() = default;

    Troops( const Troops & troops );
    Troops( Troops && ) = default;

    virtual ~Troops();

    Troops & operator=( const Troops & ) = delete;

    void Assign( const Troop * itbeg, const Troop * itend );
    void Assign( const Troops & );
    void Insert( const Troops & );
    void PushBack( const Monster &, uint32_t );
    void PopBack();

    size_t Size() const
    {
        return size();
    }

    Troop * GetTroop( size_t );
    const Troop * GetTroop( size_t ) const;

    void UpgradeMonsters( const Monster & );
    uint32_t GetCountMonsters( const Monster & mons ) const;

    double getReinforcementValue( const Troops & reinforcement ) const;

    uint32_t GetOccupiedSlotCount() const;
    bool isValid() const;
    bool HasMonster( const Monster & ) const;

    bool areAllTroopsUnique() const;

    bool AllTroopsAreUndead() const;
    // Returns true if all valid troops have the same ID or if there are no troops, otherwise returns false
    bool AllTroopsAreTheSame() const;

    bool JoinTroop( const Troop & troop );
    bool JoinTroop( const Monster & mons, const uint32_t count, const bool emptySlotFirst );
    bool CanJoinTroop( const Monster & ) const;

    virtual double GetStrength() const;

    uint32_t getTotalHP() const;

    void Clean();
    void UpgradeTroops( const Castle & castle ) const;

    Troop * GetFirstValid();
    Troop * GetWeakestTroop() const;
    Troop * GetSlowestTroop() const;

    void SortStrongest();

    void SplitTroopIntoFreeSlots( const Troop & troop, const Troop & selectedSlot, const uint32_t slots );
    void AssignToFirstFreeSlot( const Troop & troopToAssign, const uint32_t count ) const;
    void JoinAllTroopsOfType( const Troop & targetTroop ) const;

    void addNewTroopsToFreeSlots( const Troop & troop, uint32_t maxSlots );

    bool isFullHouse() const
    {
        return GetOccupiedSlotCount() == size();
    }

    // If the army has no slot find 2 or more slots of the same monster which is the weakest and merge them releasing one slot in troops.
    bool mergeWeakestTroopsIfNeeded();

    // Splits the stack consisting of the weakest units into free slots (if any), 1 monster per free slot
    void splitStackOfWeakestUnitsIntoFreeSlots();

    // Combines all stacks consisting of identical monsters
    void MergeSameMonsterTroops();

protected:
    void JoinStrongest( Troops & giverArmy, const bool keepAtLeastOneSlotForGiver );

    // Combines two stacks consisting of identical monsters. Returns true if there was something to combine, otherwise returns false.
    bool MergeSameMonsterOnce();
    // Returns an optimized version of this Troops instance, i.e. all stacks of identical monsters are combined and there are no empty slots
    Troops GetOptimized() const;

private:
    // Returns the stack that best matches the specified condition or nullptr if there are no valid stacks
    Troop * getBestMatchToCondition( const std::function<bool( const Troop *, const Troop * )> & condition ) const;
};

struct NeutralMonsterJoiningCondition
{
    enum class Reason : int
    {
        None,
        Free,
        ForMoney,
        RunAway,
        Alliance,
        Bane
    };

    Reason reason;
    uint32_t monsterCount;

    // These messages are used only for Alliance and Bane reasons.
    const char * joiningMessage;
    const char * fleeingMessage;
};

class Army final : public Troops, public Control
{
public:
    static const size_t maximumTroopCount = 5;

    static std::string SizeString( uint32_t );
    static std::string TroopSizeString( const Troop & );

    // Comparison functions
    static bool WeakestTroop( const Troop *, const Troop * );
    static bool StrongestTroop( const Troop *, const Troop * );
    static bool SlowestTroop( const Troop *, const Troop * );
    static bool FastestTroop( const Troop *, const Troop * );

    static void SwapTroops( Troop &, Troop & );

    static NeutralMonsterJoiningCondition GetJoinSolution( const Heroes &, const Maps::Tile &, const Troop & );

    static void drawSingleDetailedMonsterLine( const Troops & troops, int32_t cx, int32_t cy, int32_t width );
    static void drawMultipleMonsterLines( const Troops & troops, int32_t posX, int32_t posY, int32_t lineWidth, bool isCompact, const bool isDetailedView,
                                          const bool isGarrisonView = false, const uint32_t thievesGuildsCount = 0 );

    explicit Army( HeroBase * cmdr = nullptr );
    explicit Army( const Maps::Tile & tile );

    Army( const Army & ) = delete;

    ~Army() override = default;

    Army & operator=( const Army & ) = delete;

    const Troops & getTroops() const;

    // Resets the army. If the army doesn't have a commanding hero, then it makes the army empty. Otherwise, if 'defaultArmy' is set to true, then it creates a default
    // army of the commanding hero's faction (several units of level 1 and 2). Otherwise, a minimum army is created, consisting of exactly one monster of the first level
    // of the commanding hero's faction.
    void Reset( const bool defaultArmy = false );
    void setFromTile( const Maps::Tile & tile );

    int GetColor() const;
    int GetControl() const override;
    uint32_t getTotalCount() const;

    double GetStrength() const override;
    bool isStrongerThan( const Army & target, const double safetyRatio = 1.0 ) const;
    bool isMeleeDominantArmy() const;

    void SetColor( int cl )
    {
        color = cl;
    }

    int GetMorale() const;
    int GetLuck() const;
    int GetMoraleModificator( std::string * strs ) const;
    int GetLuckModificator( std::string * strs ) const;
    uint32_t ActionToSirens() const;

    const HeroBase * GetCommander() const;
    HeroBase * GetCommander();

    void SetCommander( HeroBase * c )
    {
        commander = c;
    }

    const Castle * inCastle() const;

    std::string String() const;

    void JoinStrongestFromArmy( Army & giver );

    // Implements the necessary logic to move unit stacks from army to army in the hero's meeting dialog and in the castle dialog
    void MoveTroops( Army & from, const int monsterIdToKeep );

    void SetSpreadFormation( const bool spread )
    {
        _isSpreadCombatFormation = spread;
    }

    bool isSpreadFormation() const
    {
        return _isSpreadCombatFormation;
    }

    bool SaveLastTroop() const;

    Monster GetStrongestMonster() const;

    void resetInvalidMonsters() const;

    // Performs the pre-battle arrangement for the castle (or town) defense, trying to add reinforcements from the garrison. Returns
    // true if at least one unit from the garrison was moved to the army as reinforcements, otherwise returns false. The logic of
    // combining troops for the castle defense differs from the original game, see the implementation for details.
    bool ArrangeForCastleDefense( Army & garrison );
    // Optimizes the arrangement of troops to pass through the whirlpool (moves one weakest unit to a separate slot, if possible)
    void ArrangeForWhirlpool();

private:
    friend OStreamBase & operator<<( OStreamBase & stream, const Army & army );
    friend IStreamBase & operator>>( IStreamBase & stream, Army & army );

    // Performs the pre-battle arrangement of given monsters in a given number, dividing them into a given number of stacks if possible
    void ArrangeForBattle( const Monster & monster, const uint32_t monstersCount, const uint32_t stacksCount );
    // Performs the pre-battle arrangement of given monsters in a given number, dividing them into a random number of stacks (seeded by
    // the tile index) with a random chance to get an upgraded stack of monsters in the center (if allowed)
    void ArrangeForBattle( const Monster & monster, const uint32_t monstersCount, const int32_t tileIndex, const bool allowUpgrade );

    HeroBase * commander;
    bool _isSpreadCombatFormation;
    int color;
};
