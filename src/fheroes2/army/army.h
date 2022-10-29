/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
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

#ifndef H2ARMY_H
#define H2ARMY_H

#include <string>
#include <utility>
#include <vector>

#include "monster.h"
#include "players.h"

class Castle;
class HeroBase;
class Heroes;
class Troop;

namespace Maps
{
    class Tiles;
}

class Troops : protected std::vector<Troop *>
{
public:
    Troops() = default;
    Troops( const Troops & troops );
    virtual ~Troops();
    Troops & operator=( const Troops & rhs );

    void Assign( const Troop *, const Troop * );
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
    uint32_t GetCountMonsters( const Monster & ) const;

    double getReinforcementValue( const Troops & reinforcement ) const;

    uint32_t GetOccupiedSlotCount() const;
    bool isValid() const;
    bool HasMonster( const Monster & ) const;

    bool areAllTroopsUnique() const;

    bool AllTroopsAreUndead() const;
    // Returns true if all valid troops have the same ID or if there are no troops, otherwise returns false
    bool AllTroopsAreTheSame() const;

    bool JoinTroop( const Troop & troop );
    bool JoinTroop( const Monster & mons, uint32_t count, bool emptySlotFirst );
    bool CanJoinTroop( const Monster & ) const;

    void JoinTroops( Troops & );
    bool CanJoinTroops( const Troops & ) const;

    // Implements the necessary logic to move unit stacks from army to army using the arrow buttons in the
    // hero's meeting dialog
    void MoveTroops( Troops & from, const int monsterIdToKeep );

    void MergeSameMonsterTroops();
    Troops GetOptimized() const;

    virtual double GetStrength() const;

    void Clean();
    void UpgradeTroops( const Castle & );

    Troop * GetFirstValid();
    Troop * GetWeakestTroop() const;
    Troop * GetSlowestTroop() const;

    void SortStrongest();

    void JoinStrongest( Troops & giverArmy, const bool keepAtLeastOneSlotForGiver, const bool priotirizeEmptySlots );

    void SplitTroopIntoFreeSlots( const Troop & troop, const Troop & selectedSlot, const uint32_t slots );
    void AssignToFirstFreeSlot( const Troop &, const uint32_t splitCount );
    void JoinAllTroopsOfType( const Troop & targetTroop );

    void addNewTroopsToFreeSlots( const Troop & troop, uint32_t maxSlots );

    bool isFullHouse() const
    {
        return GetOccupiedSlotCount() == size();
    }

    // If the army has no slot find 2 or more slots of the same monster which is the weakest and merge them releasing one slot in troops.
    bool mergeWeakestTroopsIfNeeded();
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

class Army : public Troops, public Control
{
public:
    static const size_t maximumTroopCount = 5;

    static std::string SizeString( uint32_t );
    static std::string TroopSizeString( const Troop & );

    static std::pair<uint32_t, uint32_t> SizeRange( const uint32_t count );

    // compare
    static bool WeakestTroop( const Troop *, const Troop * );
    static bool StrongestTroop( const Troop *, const Troop * );
    static bool SlowestTroop( const Troop *, const Troop * );
    static bool FastestTroop( const Troop *, const Troop * );
    static void SwapTroops( Troop &, Troop & );

    static NeutralMonsterJoiningCondition GetJoinSolution( const Heroes &, const Maps::Tiles &, const Troop & );

    static void drawMiniMonsLine( const Troops & troops, int32_t cx, int32_t cy, uint32_t width, uint32_t first = 0, uint32_t count = 0 );
    static void DrawMonsterLines( const Troops & troops, int32_t posX, int32_t posY, uint32_t lineWidth, uint32_t drawType, bool compact = true,
                                  bool isScouteView = true );

    explicit Army( HeroBase * s = nullptr );
    explicit Army( const Maps::Tiles & );
    Army( const Army & ) = delete;
    Army( Army && ) = delete;
    Army & operator=( const Army & ) = delete;
    Army & operator=( Army && ) = delete;
    ~Army() override;

    const Troops & getTroops() const;
    // Soft reset means reset to the default army (a few T1 and T2 units).
    // Hard reset means reset to the minimum army (strictly one T1 unit).
    void Reset( const bool soft = false );
    void setFromTile( const Maps::Tiles & tile );

    int GetColor() const;
    int GetControl() const override;
    uint32_t getTotalCount() const;

    double GetStrength() const override;
    bool isStrongerThan( const Army & target, double safetyRatio = 1.0 ) const;
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

    void JoinStrongestFromArmy( Army & giver, const bool priotirizeEmptySlots );

    void SetSpreadFormat( bool f )
    {
        combat_format = f;
    }

    bool isSpreadFormat() const
    {
        return combat_format;
    }

    bool SaveLastTroop() const;

    Monster GetStrongestMonster() const;

    void resetInvalidMonsters() const;

    // Optimizes the arrangement of troops to pass through the whirlpool (moves one weakest unit to a separate slot, if possible)
    void ArrangeForWhirlpool();

protected:
    friend StreamBase & operator<<( StreamBase &, const Army & );
    friend StreamBase & operator>>( StreamBase &, Army & );

    HeroBase * commander;
    bool combat_format;
    int color;

private:
    // Performs the pre-battle arrangement of given monsters in a given number, dividing them into a given number of stacks if possible
    void ArrangeForBattle( const Monster & monster, const uint32_t monstersCount, const uint32_t stacksCount );
    // Performs the pre-battle arrangement of given monsters in a given number, dividing them into a random number of stacks (seeded by
    // the tile index) with a random chance to get an upgraded stack of monsters in the center (if allowed)
    void ArrangeForBattle( const Monster & monster, const uint32_t monstersCount, const int32_t tileIndex, const bool allowUpgrade );
};

StreamBase & operator<<( StreamBase &, const Army & );
StreamBase & operator>>( StreamBase &, Army & );

#endif
