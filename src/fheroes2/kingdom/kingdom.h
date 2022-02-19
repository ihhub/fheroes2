/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2022                                                    *
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
#ifndef H2KINGDOM_H
#define H2KINGDOM_H

#include <set>

#include "castle.h"
#include "heroes_recruits.h"
#include "mp2.h"
#include "pairs.h"
#include "puzzle.h"

struct CapturedObjects;

struct KingdomCastles : public VecCastles
{};

struct KingdomHeroes : public VecHeroes
{};

class Kingdom : public BitModes, public Control
{
public:
    enum
    {
        // UNDEF      = 0x0001,
        IDENTIFYHERO = 0x0002,
        // UNUSED = 0x0004,
        KINGDOM_OVERVIEW_CASTLE_SELECTION = 0x0008
    };

    Kingdom();
    ~Kingdom() override = default;

    void Init( int color );
    void clear( void );

    void openOverviewDialog();

    bool isPlay( void ) const;
    bool isLoss( void ) const;
    bool AllowPayment( const Funds & ) const;
    bool AllowRecruitHero( bool check_payment, int level ) const;

    void SetLastBattleWinHero( const Heroes & hero );
    Heroes * GetLastBattleWinHero() const;

    void appendSurrenderedHero( Heroes & hero );

    Heroes * GetBestHero();

    Monster GetStrongestMonster() const;

    int GetControl( void ) const override;
    int GetColor( void ) const;
    int GetRace( void ) const;

    const Funds & GetFunds( void ) const
    {
        return resource;
    }
    Funds GetIncome( int = INCOME_ALL ) const;

    double GetArmiesStrength( void ) const;

    void AddFundsResource( const Funds & );
    void OddFundsResource( const Funds & );

    u32 GetCountCastle( void ) const;
    u32 GetCountTown( void ) const;
    u32 GetCountMarketplace( void ) const;
    u32 GetLostTownDays( void ) const;
    u32 GetCountNecromancyShrineBuild( void ) const;
    u32 GetCountBuilding( u32 ) const;
    uint32_t GetCountThievesGuild() const;

    uint32_t GetCountArtifacts() const;

    // Returns a reference to the pair of heroes available for recruitment,
    // updating it on the fly if necessary
    const Recruits & GetRecruits();
    // Returns a reference to the pair of heroes available for recruitment
    // without making any changes in it
    Recruits & GetCurrentRecruits();

    const KingdomHeroes & GetHeroes( void ) const
    {
        return heroes;
    }
    const KingdomCastles & GetCastles( void ) const
    {
        return castles;
    }

    KingdomHeroes & GetHeroes( void )
    {
        return heroes;
    }
    KingdomCastles & GetCastles( void )
    {
        return castles;
    }

    void AddHeroes( Heroes * );
    void RemoveHeroes( const Heroes * );
    void ApplyPlayWithStartingHero( void );

    void AddCastle( const Castle * );
    void RemoveCastle( const Castle * );

    void ActionBeforeTurn();
    void ActionNewDay( void );
    void ActionNewWeek( void );
    void ActionNewMonth( void );

    void SetVisited( s32 index, const MP2::MapObjectType objectType );
    uint32_t CountVisitedObjects( const MP2::MapObjectType objectType ) const;
    bool isVisited( const MP2::MapObjectType objectType ) const;
    bool isVisited( const Maps::Tiles & ) const;
    bool isVisited( s32, const MP2::MapObjectType objectType ) const;

    bool isValidKingdomObject( const Maps::Tiles & tile, const MP2::MapObjectType objectType ) const;

    bool HeroesMayStillMove( void ) const;

    Puzzle & PuzzleMaps( void );

    void SetVisitTravelersTent( int color );
    bool IsVisitTravelersTent( int ) const;

    void LossPostActions( void );

    bool IsTileVisibleFromCrystalBall( const int32_t dest ) const;

    static u32 GetMaxHeroes( void );

private:
    cost_t _getKingdomStartingResources( const int difficulty ) const;

    friend StreamBase & operator<<( StreamBase &, const Kingdom & );
    friend StreamBase & operator>>( StreamBase &, Kingdom & );

    int color;
    int _lastBattleWinHeroID;
    Funds resource;

    u32 lost_town_days;

    KingdomCastles castles;
    KingdomHeroes heroes;

    Recruits recruits;

    std::list<IndexObject> visit_object;

    Puzzle puzzle_maps;
    u32 visited_tents_colors;

    KingdomHeroes heroes_cond_loss;

    // Used to remember which item was selected in Kingdom View dialog.
    int _topItemInKingdomView;
};

class Kingdoms
{
public:
    Kingdoms() = default;

    void Init( void );
    void clear( void );

    void ApplyPlayWithStartingHero( void );

    void NewDay( void );
    void NewWeek( void );
    void NewMonth( void );

    Kingdom & GetKingdom( int color );
    const Kingdom & GetKingdom( int color ) const;

    int GetNotLossColors( void ) const;
    int FindWins( int ) const;

    void AddHeroes( const AllHeroes & );
    void AddCastles( const AllCastles & );

    void AddTributeEvents( CapturedObjects & captureobj, const uint32_t day, const MP2::MapObjectType objectType );

    // Resets recruits in all kingdoms and returns a set of heroes that are still available for recruitment
    // in the kingdoms
    std::set<Heroes *> resetRecruits();

private:
    friend StreamBase & operator<<( StreamBase &, const Kingdoms & );
    friend StreamBase & operator>>( StreamBase &, Kingdoms & );

    static constexpr uint32_t _size = KINGDOMMAX + 1;
    Kingdom kingdoms[_size];
};

StreamBase & operator<<( StreamBase &, const Kingdom & );
StreamBase & operator>>( StreamBase &, Kingdom & );

StreamBase & operator<<( StreamBase &, const Kingdoms & );
StreamBase & operator>>( StreamBase &, Kingdoms & );

#endif
