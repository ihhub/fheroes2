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
#ifndef H2KINGDOM_H
#define H2KINGDOM_H

#include <algorithm>
#include <cstdint>
#include <functional>
#include <list>
#include <set>

#include "bitmodes.h"
#include "castle.h"
#include "gamedefs.h"
#include "heroes.h"
#include "heroes_recruits.h"
#include "monster.h"
#include "mp2.h"
#include "pairs.h"
#include "players.h"
#include "puzzle.h"
#include "resource.h"

class StreamBase;

struct EventDate;

namespace Maps
{
    class Tiles;
}

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

    enum
    {
        INCOME_CAPTURED = 0x01,
        INCOME_CASTLES = 0x02,
        INCOME_ARTIFACTS = 0x04,
        INCOME_HERO_SKILLS = 0x08,
        INCOME_CAMPAIGN_BONUS = 0x10,
        INCOME_ALL = 0xFF
    };

    Kingdom();
    ~Kingdom() override = default;

    void Init( const int clr );
    void clear();

    void openOverviewDialog();

    bool isPlay() const;
    bool isLoss() const;
    bool AllowPayment( const Funds & ) const;
    bool AllowRecruitHero( bool check_payment ) const;

    // Returns true if this kingdom can recruit heroes, false otherwise. For example, this function returns false if there is only one town in the kingdom that cannot be
    // upgraded to a castle.
    bool canRecruitHeroes() const
    {
        return std::any_of( castles.begin(), castles.end(),
                            []( const Castle * castle ) { return ( castle->isCastle() || !castle->isBuildingDisabled( BUILD_CASTLE ) ); } );
    }

    // Returns true if this kingdom has any heroes, false otherwise.
    bool hasHeroes() const
    {
        return !heroes.empty();
    }

    void appendSurrenderedHero( Heroes & hero );

    Heroes * GetBestHero() const;

    Monster GetStrongestMonster() const;

    int GetControl() const override;
    int GetColor() const;
    int GetRace() const;

    const Funds & GetFunds() const
    {
        return resource;
    }

    Funds GetIncome( int type = INCOME_ALL ) const;

    double GetArmiesStrength() const;

    void AddFundsResource( const Funds & );
    void OddFundsResource( const Funds & );

    bool isLosingGame() const
    {
        return castles.empty();
    }

    uint32_t GetCountCastle() const;
    uint32_t GetCountTown() const;
    uint32_t GetCountMarketplace() const;
    uint32_t GetLostTownDays() const;
    uint32_t GetCountNecromancyShrineBuild() const;
    uint32_t GetCountBuilding( uint32_t ) const;
    uint32_t GetCountThievesGuild() const;

    uint32_t GetCountArtifacts() const;

    // Returns a reference to the pair of heroes available for recruitment,
    // updating it on the fly if necessary
    const Recruits & GetRecruits();
    // Returns a reference to the pair of heroes available for recruitment
    // without making any changes in it
    Recruits & GetCurrentRecruits();

    const VecHeroes & GetHeroes() const
    {
        return heroes;
    }
    const VecCastles & GetCastles() const
    {
        return castles;
    }

    VecHeroes & GetHeroes()
    {
        return heroes;
    }
    VecCastles & GetCastles()
    {
        return castles;
    }

    void AddHero( Heroes * hero );
    void RemoveHero( const Heroes * hero );
    void ApplyPlayWithStartingHero();

    void AddCastle( Castle * castle );
    void RemoveCastle( const Castle * );

    void ActionBeforeTurn();
    void ActionNewDay();
    void ActionNewWeek();
    void ActionNewMonth();
    void ActionNewDayResourceUpdate( const std::function<void( const EventDate & event, const Funds & funds )> & displayEventDialog );

    void SetVisited( int32_t index, const MP2::MapObjectType objectType );
    uint32_t CountVisitedObjects( const MP2::MapObjectType objectType ) const;
    bool isVisited( const MP2::MapObjectType objectType ) const;
    bool isVisited( const Maps::Tiles & ) const;
    bool isVisited( int32_t, const MP2::MapObjectType objectType ) const;

    bool isValidKingdomObject( const Maps::Tiles & tile, const MP2::MapObjectType objectType ) const;

    bool opponentsCanRecruitMoreHeroes() const;
    bool opponentsHaveHeroes() const;

    bool HeroesMayStillMove() const;

    Puzzle & PuzzleMaps();

    void SetVisitTravelersTent( int color );
    bool IsVisitTravelersTent( int ) const;

    void LossPostActions();

    // Checks whether this tile is visible to any hero who has an artifact with the VIEW_MONSTER_INFORMATION
    // bonus (for example, a Crystal Ball)
    bool IsTileVisibleFromCrystalBall( const int32_t dest ) const;

    static uint32_t GetMaxHeroes();

private:
    cost_t _getKingdomStartingResources( const int difficulty ) const;

    friend StreamBase & operator<<( StreamBase &, const Kingdom & );
    friend StreamBase & operator>>( StreamBase &, Kingdom & );

    int color;
    Funds resource;

    uint32_t lost_town_days;

    VecCastles castles;
    VecHeroes heroes;

    Recruits recruits;

    std::list<IndexObject> visit_object;

    Puzzle puzzle_maps;
    uint32_t visited_tents_colors;

    // Used to remember which item was selected in Kingdom View dialog.
    int _topCastleInKingdomView;
    int _topHeroInKingdomView;
};

class Kingdoms
{
public:
    Kingdoms() = default;

    void Init();
    void clear();

    void ApplyPlayWithStartingHero();

    void NewDay();
    void NewWeek();
    void NewMonth();

    Kingdom & GetKingdom( int color );
    const Kingdom & GetKingdom( int color ) const;

    int GetNotLossColors() const;
    int FindWins( int ) const;

    void AddHeroes( const AllHeroes & );
    void AddCastles( const AllCastles & castles );

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
