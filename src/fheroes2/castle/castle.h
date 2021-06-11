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
#ifndef H2CASTLE_H
#define H2CASTLE_H

#include <string>
#include <vector>

#include "army.h"
#include "bitmodes.h"
#include "captain.h"
#include "castle_heroes.h"
#include "heroes.h"
#include "mageguild.h"
#include "position.h"
#include "ui_button.h"

class Heroes;

class MeetingButton : public fheroes2::ButtonSprite
{
public:
    MeetingButton( s32, s32 );
};

class SwapButton : public fheroes2::ButtonSprite
{
public:
    SwapButton( s32, s32 );
};

enum building_t : uint32_t
{
    BUILD_NOTHING = 0x00000000,
    BUILD_THIEVESGUILD = 0x00000001,
    BUILD_TAVERN = 0x00000002,
    BUILD_SHIPYARD = 0x00000004,
    BUILD_WELL = 0x00000008,
    BUILD_STATUE = 0x00000010,
    BUILD_LEFTTURRET = 0x00000020,
    BUILD_RIGHTTURRET = 0x00000040,
    BUILD_MARKETPLACE = 0x00000080,
    BUILD_WEL2 = 0x00000100, // Farm, Garbage He, Crystal Gar, Waterfall, Orchard, Skull Pile
    BUILD_MOAT = 0x00000200,
    BUILD_SPEC = 0x00000400, // Fortification, Coliseum, Rainbow, Dungeon, Library, Storm
    BUILD_CASTLE = 0x00000800,
    BUILD_CAPTAIN = 0x00001000,
    BUILD_SHRINE = 0x00002000,
    BUILD_MAGEGUILD1 = 0x00004000,
    BUILD_MAGEGUILD2 = 0x00008000,
    BUILD_MAGEGUILD3 = 0x00010000,
    BUILD_MAGEGUILD4 = 0x00020000,
    BUILD_MAGEGUILD5 = 0x00040000,
    BUILD_MAGEGUILD = BUILD_MAGEGUILD1 | BUILD_MAGEGUILD2 | BUILD_MAGEGUILD3 | BUILD_MAGEGUILD4 | BUILD_MAGEGUILD5,
    BUILD_TENT = 0x00080000, // deprecated
    DWELLING_MONSTER1 = 0x00100000,
    DWELLING_MONSTER2 = 0x00200000,
    DWELLING_MONSTER3 = 0x00400000,
    DWELLING_MONSTER4 = 0x00800000,
    DWELLING_MONSTER5 = 0x01000000,
    DWELLING_MONSTER6 = 0x02000000,
    DWELLING_MONSTERS = DWELLING_MONSTER1 | DWELLING_MONSTER2 | DWELLING_MONSTER3 | DWELLING_MONSTER4 | DWELLING_MONSTER5 | DWELLING_MONSTER6,
    DWELLING_UPGRADE2 = 0x04000000,
    DWELLING_UPGRADE3 = 0x08000000,
    DWELLING_UPGRADE4 = 0x10000000,
    DWELLING_UPGRADE5 = 0x20000000,
    DWELLING_UPGRADE6 = 0x40000000,
    DWELLING_UPGRADE7 = 0x80000000, // black dragon
    DWELLING_UPGRADES = DWELLING_UPGRADE2 | DWELLING_UPGRADE3 | DWELLING_UPGRADE4 | DWELLING_UPGRADE5 | DWELLING_UPGRADE6 | DWELLING_UPGRADE7
};

enum buildcond_t
{
    NOT_TODAY = -1,
    ALREADY_BUILT = -2,
    NEED_CASTLE = -3,
    BUILD_DISABLE = -4,
    UNKNOWN_UPGRADE = -5,
    REQUIRES_BUILD = -6,
    LACK_RESOURCES = -7,
    UNKNOWN_COND = 0,
    ALLOW_BUILD = 1
};

class Castle : public MapPosition, public BitModes, public ColorBase, public Control
{
public:
    enum flags_t
    {
        ALLOWCASTLE = 0x0002,
        CUSTOMARMY = 0x0004,
        ALLOWBUILD = 0x0008,
        DISABLEHIRES = 0x0010,
        CAPITAL = 0x0020
    };

    Castle();
    Castle( s32, s32, int rs );
    ~Castle() override = default;

    void LoadFromMP2( StreamBuf );

    Captain & GetCaptain( void );
    const Captain & GetCaptain( void ) const;

    bool isCastle( void ) const;
    bool isCapital( void ) const;
    bool HaveNearlySea( void ) const;
    bool PresentBoat( void ) const;
    bool AllowBuyHero( const Heroes &, std::string * = NULL ) const;
    bool isPosition( const fheroes2::Point & ) const;
    bool isNecromancyShrineBuild( void ) const;

    u32 CountBuildings( void ) const;

    Heroes * RecruitHero( Heroes * );
    CastleHeroes GetHeroes( void ) const;

    int GetRace( void ) const;
    const std::string & GetName( void ) const;
    int GetControl( void ) const override;

    int GetLevelMageGuild( void ) const;
    const MageGuild & GetMageGuild( void ) const;
    bool HaveLibraryCapability( void ) const;
    bool isLibraryBuild( void ) const;
    void MageGuildEducateHero( HeroBase & ) const;

    const Army & GetArmy( void ) const;
    Army & GetArmy( void );
    const Army & GetActualArmy( void ) const;
    Army & GetActualArmy( void );
    double GetGarrisonStrength( const Heroes * attackingHero ) const;
    u32 getMonstersInDwelling( u32 ) const;
    u32 GetActualDwelling( u32 ) const;

    bool RecruitMonsterFromDwelling( uint32_t dw, uint32_t count, bool force = false );
    bool RecruitMonster( const Troop & troop, bool showDialog = true );
    void recruitBestAvailable( Funds budget );
    uint32_t getRecruitLimit( const Monster & monster, const Funds & budget ) const;

    int getBuildingValue() const;
    double getVisitValue( const Heroes & hero ) const;

    void ChangeColor( int );

    void ActionNewDay( void );
    void ActionNewWeek( void );
    void ActionNewMonth( void );
    void ActionPreBattle( void );
    void ActionAfterBattle( bool attacker_wins );

    void DrawImageCastle( const fheroes2::Point & pt ) const;

    int OpenDialog( bool readonly = false );

    int GetAttackModificator( const std::string * ) const;
    int GetDefenseModificator( const std::string * ) const;
    int GetPowerModificator( std::string * ) const;
    int GetKnowledgeModificator( const std::string * ) const;
    int GetMoraleModificator( std::string * ) const;
    int GetLuckModificator( std::string * ) const;

    bool AllowBuyBuilding( u32 ) const;
    bool isBuild( u32 bd ) const;
    bool BuyBuilding( u32 );
    bool AllowBuyBoat( void ) const;
    bool BuyBoat( void ) const;
    u32 GetBuildingRequirement( u32 ) const;

    int CheckBuyBuilding( u32 ) const;
    static int GetAllBuildingStatus( const Castle & );

    void Scoute( void ) const;

    std::string GetStringBuilding( u32 ) const;
    std::string GetDescriptionBuilding( u32 ) const;

    // Returns message displayed in the status bar on the castle view
    // when hover over the building
    std::string buildingStatusMessage( const uint32_t buildingId ) const;

    static const char * GetStringBuilding( u32, int race );
    static const char * GetDescriptionBuilding( u32, int race );

    static int GetICNBuilding( u32, int race );
    static int GetICNBoat( int race );
    u32 GetUpgradeBuilding( u32 ) const;

    static bool PredicateIsCastle( const Castle * );
    static bool PredicateIsTown( const Castle * );
    static bool PredicateIsCapital( const Castle * );
    static bool PredicateIsBuildBuilding( const Castle * castle, const uint32_t building );

    static u32 GetGrownWell( void );
    static u32 GetGrownWel2( void );
    static u32 GetGrownWeekOf();
    static u32 GetGrownMonthOf( void );

    std::string String( void ) const;

    int DialogBuyHero( const Heroes * ) const;
    int DialogBuyCastle( bool fixed = true ) const;

    void SwapCastleHeroes( CastleHeroes & );

private:
    u32 * GetDwelling( u32 dw );
    void EducateHeroes( void );
    fheroes2::Rect RedrawResourcePanel( const fheroes2::Point & ) const;
    u32 OpenTown( void );
    void OpenTavern( void ) const;
    void OpenWell( void );
    void OpenMageGuild( const CastleHeroes & heroes ) const;
    void WellRedrawInfoArea( const fheroes2::Point & cur_pt, const std::vector<RandomMonsterAnimation> & monsterAnimInfo ) const;
    void JoinRNDArmy( void );
    void PostLoad( void );

private:
    friend StreamBase & operator<<( StreamBase &, const Castle & );
    friend StreamBase & operator>>( StreamBase &, Castle & );

    int race;
    u32 building;
    Captain captain;

    std::string name;

    MageGuild mageguild;
    u32 dwelling[CASTLEMAXMONSTER];
    Army army;
};

namespace CastleDialog
{
    // Class used for fading animation
    class FadeBuilding
    {
    public:
        FadeBuilding()
            : _alpha( 255 )
            , _build( BUILD_NOTHING )
        {}

        void StartFadeBuilding( const uint32_t build );

        bool UpdateFadeBuilding();

        bool IsFadeDone() const
        {
            return _alpha >= 255;
        }

        void StopFadeBuilding();

        uint32_t GetAlpha() const
        {
            return _alpha;
        }

        uint32_t GetBuild() const
        {
            return _build;
        }

    private:
        uint32_t _alpha;
        uint32_t _build;
    };

    struct builds_t
    {
        builds_t( building_t b, const fheroes2::Rect & r )
            : id( b )
            , coord( r )
        {}

        bool operator==( u32 b ) const
        {
            return b == static_cast<uint32_t>( id );
        }

        building_t id;
        fheroes2::Rect coord;
    };

    struct CacheBuildings : std::vector<builds_t>
    {
        CacheBuildings( const Castle &, const fheroes2::Point & );
    };

    void RedrawAllBuilding( const Castle & castle, const fheroes2::Point & dst_pt, const CacheBuildings & orders, const CastleDialog::FadeBuilding & alphaBuilding );
    void RedrawBuildingSpriteToArea( const fheroes2::Sprite &, s32, s32, const fheroes2::Rect &, uint8_t alpha = 255 );

    void CastleRedrawBuilding( const Castle &, const fheroes2::Point &, u32 build, u32 frame, uint8_t alpha = 255 );
    void CastleRedrawBuildingExtended( const Castle &, const fheroes2::Point &, u32 build, u32 frame, uint8_t alpha = 255 );

    bool RoadConnectionNeeded( const Castle & castle, const uint32_t buildId, const bool constructionInProgress );
    void RedrawRoadConnection( const Castle & castle, const fheroes2::Point & position, const uint32_t buildId, const uint8_t alpha = 255 );
}

struct VecCastles : public std::vector<Castle *>
{
    Castle * GetFirstCastle( void ) const;

    void ChangeColors( int, int );
    void SortByBuildingValue();
};

class AllCastles
{
public:
    AllCastles();
    AllCastles( AllCastles & ) = delete;

    ~AllCastles();

    AllCastles & operator=( const AllCastles & ) = delete;

    void Init( void );
    void Clear( void );

    void AddCastle( Castle * castle );

    Castle * Get( const fheroes2::Point & position ) const;

    void Scoute( int ) const;

    // begin/end methods so we can iterate through the elements
    std::vector<Castle *>::const_iterator begin() const
    {
        return _castles.begin();
    }

    std::vector<Castle *>::const_iterator end() const
    {
        return _castles.end();
    }

    size_t Size() const
    {
        return _castles.size();
    }

private:
    std::vector<Castle *> _castles;
    std::map<fheroes2::Point, size_t> _castleTiles;
};

StreamBase & operator<<( StreamBase &, const VecCastles & );
StreamBase & operator>>( StreamBase &, VecCastles & );

StreamBase & operator<<( StreamBase &, const AllCastles & );
StreamBase & operator>>( StreamBase &, AllCastles & );

StreamBase & operator<<( StreamBase &, const Castle & );
StreamBase & operator>>( StreamBase &, Castle & );

#endif
