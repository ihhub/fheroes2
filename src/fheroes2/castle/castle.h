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
#ifndef H2CASTLE_H
#define H2CASTLE_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "army.h"
#include "bitmodes.h"
#include "captain.h"
#include "color.h"
#include "mageguild.h"
#include "math_base.h"
#include "monster.h"
#include "players.h"
#include "position.h"

class IStreamBase;
class OStreamBase;

class HeroBase;
class Heroes;
class Troop;

struct Funds;

namespace fheroes2
{
    class RandomMonsterAnimation;
    class Image;
}

namespace Maps::Map_Format
{
    struct CastleMetadata;
}

enum BuildingType : uint32_t
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
    // Farm, Garbage Heap, Crystal Garden, Waterfall, Orchard, Skull Pile
    BUILD_WEL2 = 0x00000100,
    BUILD_MOAT = 0x00000200,
    // Fortifications, Coliseum, Rainbow, Dungeon, Library, Storm
    BUILD_SPEC = 0x00000400,
    BUILD_CASTLE = 0x00000800,
    BUILD_CAPTAIN = 0x00001000,
    BUILD_SHRINE = 0x00002000,
    BUILD_MAGEGUILD1 = 0x00004000,
    BUILD_MAGEGUILD2 = 0x00008000,
    BUILD_MAGEGUILD3 = 0x00010000,
    BUILD_MAGEGUILD4 = 0x00020000,
    BUILD_MAGEGUILD5 = 0x00040000,
    BUILD_MAGEGUILD = BUILD_MAGEGUILD1 | BUILD_MAGEGUILD2 | BUILD_MAGEGUILD3 | BUILD_MAGEGUILD4 | BUILD_MAGEGUILD5,
    BUILD_TENT = 0x00080000,
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
    // Black Dragons
    DWELLING_UPGRADE7 = 0x80000000,
    DWELLING_UPGRADES = DWELLING_UPGRADE2 | DWELLING_UPGRADE3 | DWELLING_UPGRADE4 | DWELLING_UPGRADE5 | DWELLING_UPGRADE6 | DWELLING_UPGRADE7
};

enum class BuildingStatus : int32_t
{
    UNKNOWN_COND,
    ALLOW_BUILD,
    NOT_TODAY,
    ALREADY_BUILT,
    NEED_CASTLE,
    BUILD_DISABLE,
    SHIPYARD_NOT_ALLOWED,
    UNKNOWN_UPGRADE,
    REQUIRES_BUILD,
    LACK_RESOURCES
};

class Castle final : public MapPosition, public BitModes, public ColorBase, public Control
{
public:
    // Maximum number of creature dwellings that can be built in a castle
    static constexpr int maxNumOfDwellings{ 6 };

    enum : uint32_t
    {
        UNUSED_ALLOW_CASTLE_CONSTRUCTION = ( 1 << 1 ),
        CUSTOM_ARMY = ( 1 << 2 ),
        ALLOW_TO_BUILD_TODAY = ( 1 << 3 )
    };

    enum class CastleDialogReturnValue : int
    {
        DoNothing,
        Close, // Close the dialog.
        NextCastle, // Open main dialog of the next castle.
        PreviousCastle, // Open main dialog of the previous castle.
        NextCostructionWindow, // Open construction dialog of the next castle.
        PreviousCostructionWindow // Open construction dialog of the previous castle.
    };

    Castle();
    Castle( int32_t cx, int32_t cy, int rc );
    Castle( const Castle & ) = delete;
    ~Castle() override = default;

    Castle & operator=( const Castle & ) = delete;

    void LoadFromMP2( const std::vector<uint8_t> & data );

    void loadFromResurrectionMap( const Maps::Map_Format::CastleMetadata & metadata );

    Captain & GetCaptain()
    {
        return captain;
    }

    const Captain & GetCaptain() const
    {
        return captain;
    }

    bool isCastle() const
    {
        return ( _constructedBuildings & BUILD_CASTLE ) != 0;
    }

    bool HasSeaAccess() const;
    bool HasBoatNearby() const;

    // Returns a tile ID where it is possible to place a boat or -1 if it is not.
    int32_t getTileIndexToPlaceBoat() const;

    bool AllowBuyHero( std::string * = nullptr ) const;
    bool isPosition( const fheroes2::Point & pt ) const override;
    bool isNecromancyShrineBuild() const;

    uint32_t CountBuildings() const;

    Heroes * RecruitHero( Heroes * );
    Heroes * GetHero() const;

    int GetRace() const
    {
        return race;
    }

    const std::string & GetName() const
    {
        return name;
    }

    // This method must be called only at the time of map loading and only for castles with empty names.
    void setName( const std::set<std::string, std::less<>> & usedNames );

    int GetControl() const override;

    int GetLevelMageGuild() const;

    const MageGuild & GetMageGuild() const
    {
        return mageguild;
    }

    bool HaveLibraryCapability() const;
    bool isLibraryBuild() const;
    void MageGuildEducateHero( HeroBase & ) const;

    bool isFortificationBuilt() const;

    const Army & GetArmy() const;
    Army & GetArmy();
    const Army & GetActualArmy() const;
    Army & GetActualArmy();
    uint32_t getMonstersInDwelling( uint32_t ) const;

    // Returns the garrison strength estimation calculated as if this castle had really been attacked, including
    // an estimate of the strength of the combined army consisting of the garrison and the guest hero's troops
    // (if present), castle-specific bonuses from moat, towers and so on, relative to the attacking hero's abilities
    // (if the 'attackingHero' is not 'nullptr'). See the implementation for details.
    double GetGarrisonStrength( const Heroes * attackingHero ) const;
    double GetGarrisonStrength( const Heroes & attackingHero ) const
    {
        return GetGarrisonStrength( &attackingHero );
    }

    // Returns the correct dwelling type available in the castle. BUILD_NOTHING is returned if this is not a dwelling.
    uint32_t GetActualDwelling( const uint32_t buildId ) const;

    // Returns true in case of successful recruitment.
    bool RecruitMonster( const Troop & troop, bool showDialog = true );

    void recruitBestAvailable( Funds budget );
    uint32_t getRecruitLimit( const Monster & monster, const Funds & budget ) const;

    int getBuildingValue() const;

    // Used only for AI.
    double getArmyRecruitmentValue() const;
    double getVisitValue( const Heroes & hero ) const;

    void ChangeColor( int );

    void ActionNewDay();
    void ActionNewWeek();
    void ActionNewMonth() const;

    void ActionPreBattle();
    void ActionAfterBattle( bool attacker_wins );

    void DrawImageCastle( const fheroes2::Point & pt ) const;

    CastleDialogReturnValue OpenDialog( const bool openConstructionWindow, const bool fade, const bool renderBackgroundDialog );

    int GetAttackModificator( const std::string * ) const;
    int GetDefenseModificator( const std::string * ) const;
    int GetPowerModificator( std::string * ) const;
    int GetKnowledgeModificator( const std::string * ) const;
    int GetMoraleModificator( std::string * ) const;
    int GetLuckModificator( std::string * ) const;

    bool AllowBuyBuilding( uint32_t ) const;

    bool isBuild( uint32_t bd ) const
    {
        return ( _constructedBuildings & bd ) != 0;
    }

    bool BuyBuilding( uint32_t );

    BuildingStatus CheckBuyBuilding( const uint32_t build ) const;
    static BuildingStatus GetAllBuildingStatus( const Castle & );

    bool AllowBuyBoat( const bool checkPayment ) const;
    bool BuyBoat() const;

    void Scout() const;

    std::string GetStringBuilding( uint32_t ) const;
    std::string GetDescriptionBuilding( uint32_t ) const;

    static const char * GetStringBuilding( uint32_t, int race );

    static int GetICNBuilding( uint32_t, int race );
    static int GetICNBoat( int race );
    uint32_t GetUpgradeBuilding( const uint32_t buildingId ) const;

    static bool PredicateIsCastle( const Castle * );
    static bool PredicateIsTown( const Castle * );
    static bool PredicateIsBuildBuilding( const Castle * castle, const uint32_t building );

    static uint32_t GetGrownWell();
    static uint32_t GetGrownWel2();
    static uint32_t GetGrownWeekOf();
    static uint32_t GetGrownMonthOf();

    std::string String() const;

    int DialogBuyHero( const Heroes * ) const;
    int DialogBuyCastle( bool fixed = true ) const;

    Troops getAvailableArmy( Funds potentialBudget ) const;

    bool isBuildingDisabled( const uint32_t buildingType ) const
    {
        return ( _disabledBuildings & buildingType ) != 0;
    }

private:
    enum class ConstructionDialogResult : int
    {
        DoNothing,
        NextConstructionWindow, // Open construction dialog for the next castle.
        PrevConstructionWindow, // Open construction dialog for the previous castle.
        Build, // Build something.
        RecruitHero // Recruit a hero.
    };

    // Checks whether this particular building is currently built in the castle (unlike
    // the isBuild(), upgraded versions of the same building are not taken into account)
    bool isExactBuildingBuilt( const uint32_t buildingToCheck ) const;

    uint32_t * GetDwelling( uint32_t dw );
    void EducateHeroes();

    ConstructionDialogResult openConstructionDialog( uint32_t & dwellingTobuild );

    void OpenTavern() const;
    void OpenWell();
    void OpenMageGuild( const Heroes * hero ) const;
    void JoinRNDArmy();
    void PostLoad();

    void _wellRedrawAvailableMonsters( const uint32_t dwellingType, const bool restoreBackground, fheroes2::Image & background ) const;
    void _wellRedrawBackground( fheroes2::Image & background ) const;
    void _wellRedrawMonsterAnimation( const fheroes2::Rect & roi, std::array<fheroes2::RandomMonsterAnimation, maxNumOfDwellings> & monsterAnimInfo ) const;

    void _setDefaultBuildings();

    // Recruit maximum monsters from the castle. Returns 'true' if the recruit was made.
    bool _recruitCastleMax( const Troops & currentCastleArmy );
    bool RecruitMonsterFromDwelling( uint32_t dw, uint32_t count, bool force = false );

    friend OStreamBase & operator<<( OStreamBase & stream, const Castle & castle );
    friend IStreamBase & operator>>( IStreamBase & stream, Castle & castle );

    int race;
    uint32_t _constructedBuildings;
    uint32_t _disabledBuildings;

    Captain captain;

    std::string name;

    MageGuild mageguild;
    std::array<uint32_t, maxNumOfDwellings> dwelling;
    Army army;
};

namespace CastleDialog
{
    // Class used for fading animation
    class FadeBuilding
    {
    public:
        FadeBuilding() = default;

        void StartFadeBuilding( const uint32_t building )
        {
            _alpha = 0;
            _building = building;
            _isOnlyBoat = false;
        }

        void StartFadeBoat()
        {
            _alpha = 0;
            _building = BUILD_SHIPYARD;
            _isOnlyBoat = true;
        }

        bool UpdateFade();

        bool IsFadeDone() const
        {
            return _alpha == 255;
        }

        void StopFade()
        {
            _alpha = 255;
            _building = BUILD_NOTHING;
            _isOnlyBoat = false;
        }

        uint8_t GetAlpha() const
        {
            return _alpha;
        }

        uint32_t GetBuilding() const
        {
            return _building;
        }

        bool isOnlyBoat() const
        {
            return _isOnlyBoat;
        }

    private:
        uint8_t _alpha{ 255 };
        uint32_t _building{ BUILD_NOTHING };
        bool _isOnlyBoat{ false };
    };

    struct BuildingRenderInfo
    {
        BuildingRenderInfo( BuildingType b, const fheroes2::Rect & r )
            : id( b )
            , coord( r )
        {}

        bool operator==( uint32_t b ) const
        {
            return b == static_cast<uint32_t>( id );
        }

        BuildingType id;
        fheroes2::Rect coord;
    };

    struct CacheBuildings : std::vector<BuildingRenderInfo>
    {
        CacheBuildings( const Castle & castle, const fheroes2::Point & top );
    };

    void RedrawAllBuildings( const Castle & castle, const fheroes2::Point & dst_pt, const CacheBuildings & orders, const CastleDialog::FadeBuilding & alphaBuilding,
                             const uint32_t animationIndex );
}

struct VecCastles : public std::vector<Castle *>
{
    VecCastles() = default;
    VecCastles( const VecCastles & ) = delete;

    ~VecCastles() = default;

    VecCastles & operator=( const VecCastles & ) = delete;
    VecCastles & operator=( VecCastles && ) = default;

    Castle * GetFirstCastle() const;
};

class AllCastles
{
public:
    AllCastles();
    AllCastles( const AllCastles & ) = delete;

    ~AllCastles() = default;

    AllCastles & operator=( const AllCastles & ) = delete;

    auto begin() const noexcept
    {
        return Iterator( _castles.begin() );
    }

    auto end() const noexcept
    {
        return Iterator( _castles.end() );
    }

    void Init()
    {
        Clear();
    }

    void Clear()
    {
        _castles.clear();
        _castleTiles.clear();
    }

    size_t Size() const
    {
        return _castles.size();
    }

    void AddCastle( std::unique_ptr<Castle> && castle );

    Castle * Get( const fheroes2::Point & position ) const;

    void Scout( const int colors ) const;

    void NewDay() const;
    void NewWeek() const;
    void NewMonth() const;

    template <typename BaseIterator>
    struct Iterator : public BaseIterator
    {
        explicit Iterator( BaseIterator && other ) noexcept
            : BaseIterator( std::move( other ) )
        {}

        auto * operator*() const noexcept
        {
            return BaseIterator::operator*().get();
        }
    };

private:
    std::vector<std::unique_ptr<Castle>> _castles;
    std::map<fheroes2::Point, size_t> _castleTiles;
};

OStreamBase & operator<<( OStreamBase & stream, const VecCastles & castles );
IStreamBase & operator>>( IStreamBase & stream, VecCastles & castles );

OStreamBase & operator<<( OStreamBase & stream, const AllCastles & castles );
IStreamBase & operator>>( IStreamBase & stream, AllCastles & castles );

#endif
