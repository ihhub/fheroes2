/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
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
    void PushBack( const Monster &, u32 );
    void PopBack( void );

    size_t Size() const
    {
        return size();
    }

    Troop * GetTroop( size_t );
    const Troop * GetTroop( size_t ) const;

    void UpgradeMonsters( const Monster & );
    u32 GetCountMonsters( const Monster & ) const;

    double getReinforcementValue( const Troops & reinforcement ) const;

    u32 GetCount( void ) const;
    bool isValid( void ) const;
    bool HasMonster( const Monster & ) const;

    bool AllTroopsAreUndead( void ) const;
    bool AllTroopsAreTheSame( void ) const;

    bool JoinTroop( const Troop & );
    bool JoinTroop( const Monster & mons, uint32_t count, bool emptySlotFirst = false );
    bool CanJoinTroop( const Monster & ) const;

    void JoinTroops( Troops & );
    bool CanJoinTroops( const Troops & ) const;

    // Used only for moving full army in hero's meeting dialog.
    void MoveTroops( const Troops & from );

    void MergeTroops();
    Troops GetOptimized( void ) const;

    virtual double GetStrength() const;

    void Clean( void );
    void UpgradeTroops( const Castle & );

    Troop * GetFirstValid( void );
    Troop * GetWeakestTroop() const;
    const Troop * GetSlowestTroop() const;

    void SortStrongest();
    void ArrangeForBattle( bool = false );
    // Optimizes the arrangement of troops to pass through the whirlpool (moves one weakest unit
    // to a separate slot, if possible)
    void ArrangeForWhirlpool();

    void JoinStrongest( Troops &, bool );

    void DrawMons32Line( int32_t, int32_t, uint32_t, uint32_t, uint32_t, uint32_t, bool, bool ) const;
    void SplitTroopIntoFreeSlots( const Troop & troop, const Troop & selectedSlot, const uint32_t slots );
    void AssignToFirstFreeSlot( const Troop &, const uint32_t splitCount );
    void JoinAllTroopsOfType( const Troop & targetTroop );
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
    static std::string SizeString( u32 );
    static std::string TroopSizeString( const Troop & );

    // compare
    static bool WeakestTroop( const Troop *, const Troop * );
    static bool StrongestTroop( const Troop *, const Troop * );
    static bool SlowestTroop( const Troop *, const Troop * );
    static bool FastestTroop( const Troop *, const Troop * );
    static void SwapTroops( Troop &, Troop & );

    static NeutralMonsterJoiningCondition GetJoinSolution( const Heroes &, const Maps::Tiles &, const Troop & );

    static void DrawMons32Line( const Troops &, s32, s32, u32, u32 = 0, u32 = 0 );
    static void DrawMonsterLines( const Troops & troops, int32_t posX, int32_t posY, uint32_t lineWidth, uint32_t drawPower, bool compact = true,
                                  bool isScouteView = true );

    explicit Army( HeroBase * s = nullptr );
    explicit Army( const Maps::Tiles & );
    Army( const Army & ) = delete;
    Army( Army && ) = delete;
    Army & operator=( const Army & ) = delete;
    Army & operator=( Army && ) = delete;
    ~Army() override;

    const Troops & getTroops() const;
    void Reset( bool = false ); // reset: soft or hard
    void setFromTile( const Maps::Tiles & tile );

    int GetColor( void ) const;
    int GetControl( void ) const override;
    uint32_t getTotalCount() const;

    double GetStrength() const override;
    bool isStrongerThan( const Army & target, double safetyRatio = 1.0 ) const;
    bool isMeleeDominantArmy() const;

    void SetColor( int cl )
    {
        color = cl;
    }

    int GetMorale( void ) const;
    int GetLuck( void ) const;
    int GetMoraleModificator( std::string * ) const;
    int GetLuckModificator( const std::string * ) const;
    uint32_t ActionToSirens() const;

    const HeroBase * GetCommander( void ) const;
    HeroBase * GetCommander( void );

    void SetCommander( HeroBase * c )
    {
        commander = c;
    }

    const Castle * inCastle( void ) const;

    std::string String( void ) const;

    void JoinStrongestFromArmy( Army & );

    void SetSpreadFormat( bool f )
    {
        combat_format = f;
    }

    bool isSpreadFormat() const
    {
        return combat_format;
    }

    bool isFullHouse() const
    {
        return GetCount() == size();
    }

    bool SaveLastTroop( void ) const;

    Monster GetStrongestMonster() const;

    void resetInvalidMonsters() const;

protected:
    friend StreamBase & operator<<( StreamBase &, const Army & );
    friend StreamBase & operator>>( StreamBase &, Army & );

    HeroBase * commander;
    bool combat_format;
    int color;
};

StreamBase & operator<<( StreamBase &, const Army & );
StreamBase & operator>>( StreamBase &, Army & );

#endif
