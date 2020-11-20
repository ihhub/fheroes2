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

enum building_t
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
    virtual ~Castle() {}
    void LoadFromMP2( StreamBuf );

    Captain & GetCaptain( void );
    const Captain & GetCaptain( void ) const;

    bool isCastle( void ) const;
    bool isCapital( void ) const;
    bool HaveNearlySea( void ) const;
    bool PresentBoat( void ) const;
    bool AllowBuyHero( const Heroes &, std::string * = NULL );
    bool isPosition( const Point & ) const;
    bool isNecromancyShrineBuild( void ) const;

    u32 CountBuildings( void ) const;

    Heroes * RecruitHero( Heroes * );
    CastleHeroes GetHeroes( void ) const;

    int GetRace( void ) const;
    const std::string & GetName( void ) const;
    int GetControl( void ) const;

    int GetLevelMageGuild( void ) const;
    const MageGuild & GetMageGuild( void ) const;
    bool HaveLibraryCapability( void ) const;
    bool isLibraryBuild( void ) const;
    void MageGuildEducateHero( HeroBase & ) const;

    const Army & GetArmy( void ) const;
    Army & GetArmy( void );
    const Army & GetActualArmy( void ) const;
    Army & GetActualArmy( void );
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

    void DrawImageCastle( const Point & pt ) const;

    int OpenDialog( bool readonly = false );

    int GetAttackModificator( std::string * ) const;
    int GetDefenseModificator( std::string * ) const;
    int GetPowerModificator( std::string * ) const;
    int GetKnowledgeModificator( std::string * ) const;
    int GetMoraleModificator( std::string * ) const;
    int GetLuckModificator( std::string * ) const;

    bool AllowBuild( void ) const;
    bool AllowBuyBuilding( u32 ) const;
    bool isBuild( u32 bd ) const;
    bool BuyBuilding( u32 );
    bool AllowBuyBoat( void ) const;
    bool BuyBoat( void );
    u32 GetBuildingRequirement( u32 ) const;

    int CheckBuyBuilding( u32 ) const;
    static int GetAllBuildingStatus( const Castle & );

    void Scoute( void ) const;

    std::string GetStringBuilding( u32 ) const;
    std::string GetDescriptionBuilding( u32 ) const;

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
    static u32 GetGrownWeekOf( const Monster & );
    static u32 GetGrownMonthOf( void );

    std::string String( void ) const;

    int DialogBuyHero( const Heroes * );
    int DialogBuyCastle( bool fixed = true ) const;

    void SwapCastleHeroes( CastleHeroes & );

private:
    u32 * GetDwelling( u32 dw );
    void EducateHeroes( void );
    Rect RedrawResourcePanel( const Point & ) const;
    u32 OpenTown( void );
    void OpenTavern( void );
    void OpenWell( void );
    void OpenMageGuild( const CastleHeroes & heroes );
    void WellRedrawInfoArea( const Point & cur_pt, const std::vector<RandomMonsterAnimation> & monsterAnimInfo );
    void JoinRNDArmy( void );
    void PostLoad( void );

private:
    friend StreamBase & operator<<( StreamBase &, const Castle & );
    friend StreamBase & operator>>( StreamBase &, Castle & );
#ifdef WITH_XML
    friend TiXmlElement & operator>>( TiXmlElement &, Castle & );
#endif

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
    struct builds_t
    {
        builds_t( building_t b, const Rect & r )
            : id( b )
            , coord( r ){};

        bool operator==( u32 b ) const
        {
            return b == static_cast<uint32_t>( id );
        }

        building_t id;
        Rect coord;
    };

    struct CacheBuildings : std::vector<builds_t>
    {
        CacheBuildings( const Castle &, const Point & );
        const Rect & GetRect( building_t ) const;
    };

    void RedrawAllBuilding( const Castle &, const Point &, const CacheBuildings & );
    void RedrawAnimationBuilding( const Castle &, const Point &, const CacheBuildings &, u32 build );
    void RedrawBuildingSpriteToArea( const fheroes2::Sprite &, s32, s32, const Rect &, uint8_t alpha = 255 );

    void CastleRedrawBuilding( const Castle &, const Point &, u32 build, u32 frame, uint8_t alpha = 255 );
    void CastleRedrawBuildingExtended( const Castle &, const Point &, u32 build, u32 frame, uint8_t alpha = 255 );

    bool RoadConnectionNeeded( const Castle & castle, const uint32_t buildId, const bool constructionInProgress );
    void RedrawRoadConnection( const Castle & castle, const Point & position, const uint32_t buildId, const uint8_t alpha = 255 );
}

struct VecCastles : public std::vector<Castle *>
{
    Castle * Get( const Point & ) const;
    Castle * GetFirstCastle( void ) const;

    void ChangeColors( int, int );
    void SortByBuildingValue();
};

struct AllCastles : public VecCastles
{
    AllCastles();
    ~AllCastles();

    void Init( void );
    void clear( void );

    void Scoute( int ) const;
};

StreamBase & operator<<( StreamBase &, const VecCastles & );
StreamBase & operator>>( StreamBase &, VecCastles & );

StreamBase & operator<<( StreamBase &, const AllCastles & );
StreamBase & operator>>( StreamBase &, AllCastles & );

StreamBase & operator<<( StreamBase &, const Castle & );
StreamBase & operator>>( StreamBase &, Castle & );

#endif
