/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
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
#ifndef H2KINGDOM_H
#define H2KINGDOM_H

#include <map>
#include <vector>

#include "castle.h"
#include "heroes.h"
#include "heroes_recruits.h"
#include "mp2.h"
#include "pairs.h"
#include "puzzle.h"

class Castle;
class Heroes;
struct AllHeroes;
struct VecHeroes;
struct AllCastles;
struct VecCastles;
struct CapturedObjects;

struct LastLoseHero
{
    LastLoseHero()
        : id( Heroes::UNKNOWN )
        , date( 0 )
    {}
    int id;
    u32 date;
};

StreamBase & operator>>( StreamBase &, LastLoseHero & );

struct KingdomCastles : public VecCastles
{};

struct KingdomHeroes : public VecHeroes
{};

class Kingdom : public BitModes, public Control
{
public:
    enum
    {
        // UNDEF	     = 0x0001,
        IDENTIFYHERO = 0x0002,
        DISABLEHIRES = 0x0004,
        OVERVIEWCSTL = 0x0008
    };

    Kingdom();
    virtual ~Kingdom() {}

    void Init( int color );
    void clear( void );

    void OverviewDialog( void );

    void UpdateStartingResource( void );
    bool isPlay( void ) const;
    bool isLoss( void ) const;
    bool AllowPayment( const Funds & ) const;
    bool AllowRecruitHero( bool check_payment, int level ) const;

    void SetLastLostHero( Heroes & );
    void ResetLastLostHero( void );
    void AddHeroStartCondLoss( Heroes * );
    std::string GetNamesHeroStartCondLoss( void ) const;

    Heroes * GetLastLostHero( void ) const;

    const Heroes * GetFirstHeroStartCondLoss( void ) const;
    Heroes * GetBestHero();

    int GetControl( void ) const;
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
    u32 GetCountCapital( void ) const;
    u32 GetLostTownDays( void ) const;
    u32 GetCountNecromancyShrineBuild( void ) const;
    u32 GetCountBuilding( u32 ) const;
    uint32_t GetCountThievesGuild() const;

    Recruits & GetRecruits( void );

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
    void HeroesActionNewPosition( void );

    void AddCastle( const Castle * );
    void RemoveCastle( const Castle * );

    void ActionBeforeTurn( void );
    void ActionNewDay( void );
    void ActionNewWeek( void );
    void ActionNewMonth( void );

    void SetVisited( s32 index, int object = MP2::OBJ_ZERO );
    u32 CountVisitedObjects( int object ) const;
    bool isVisited( int object ) const;
    bool isVisited( const Maps::Tiles & ) const;
    bool isVisited( s32, int obj ) const;

    bool isValidKingdomObject( const Maps::Tiles & tile, int objectID ) const;

    bool HeroesMayStillMove( void ) const;

    const Puzzle & PuzzleMaps( void ) const;
    Puzzle & PuzzleMaps( void );

    void SetVisitTravelersTent( int color );
    bool IsVisitTravelersTent( int ) const;

    void UpdateRecruits( void );
    void LossPostActions( void );

    static u32 GetMaxHeroes( void );

private:
    friend StreamBase & operator<<( StreamBase &, const Kingdom & );
    friend StreamBase & operator>>( StreamBase &, Kingdom & );

    int color;
    Funds resource;

    u32 lost_town_days;

    KingdomCastles castles;
    KingdomHeroes heroes;

    Recruits recruits;
    LastLoseHero lost_hero;

    std::list<IndexObject> visit_object;

    Puzzle puzzle_maps;
    u32 visited_tents_colors;

    KingdomHeroes heroes_cond_loss;
};

class Kingdoms
{
public:
    Kingdoms();

    void Init( void );
    void clear( void );

    void ApplyPlayWithStartingHero( void );

    void NewDay( void );
    void NewWeek( void );
    void NewMonth( void );

    Kingdom & GetKingdom( int color );
    const Kingdom & GetKingdom( int color ) const;

    int GetLossColors( void ) const;
    int GetNotLossColors( void ) const;
    int FindWins( int ) const;

    void AddHeroes( const AllHeroes & );
    void AddCastles( const AllCastles & );

    void AddCondLossHeroes( const AllHeroes & );
    void AddTributeEvents( CapturedObjects &, u32 day, int obj );

    u32 size( void ) const;

private:
    friend StreamBase & operator<<( StreamBase &, const Kingdoms & );
    friend StreamBase & operator>>( StreamBase &, Kingdoms & );

    Kingdom kingdoms[KINGDOMMAX + 1];
};

StreamBase & operator<<( StreamBase &, const Kingdom & );
StreamBase & operator>>( StreamBase &, Kingdom & );

StreamBase & operator<<( StreamBase &, const Kingdoms & );
StreamBase & operator>>( StreamBase &, Kingdoms & );

StreamBase & operator<<( StreamBase & sb, const LastLoseHero & hero );
StreamBase & operator>>( StreamBase & sb, LastLoseHero & hero );

#endif
