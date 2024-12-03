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
#include "race.h"

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

    Castle() = default;
    Castle( const int32_t posX, const int32_t posY, int race );

    Castle( const Castle & ) = delete;

    ~Castle() override = default;

    Castle & operator=( const Castle & ) = delete;

    void LoadFromMP2( const std::vector<uint8_t> & data );

    void loadFromResurrectionMap( const Maps::Map_Format::CastleMetadata & metadata );

    Captain & GetCaptain()
    {
        return _captain;
    }

    const Captain & GetCaptain() const
    {
        return _captain;
    }

    bool isCastle() const
    {
        return ( _constructedBuildings & BUILD_CASTLE ) != 0;
    }

    bool HasSeaAccess() const;
    bool HasBoatNearby() const;

    // Returns a tile ID where it is possible to place a boat or -1 if it is not.
    int32_t getTileIndexToPlaceBoat() const;

    bool AllowBuyHero( std::string * msg = nullptr ) const;
    bool isPosition( const fheroes2::Point & pt ) const override;
    bool isNecromancyShrineBuild() const
    {
        return _race == Race::NECR && ( BUILD_SHRINE & _constructedBuildings );
    }

    uint32_t CountBuildings() const;

    Heroes * RecruitHero( Heroes * hero );
    Heroes * GetHero() const;

    int GetRace() const
    {
        return _race;
    }

    const std::string & GetName() const
    {
        return _name;
    }

    // This method must be called only at the time of map loading and only for castles with empty names.
    void setName( const std::set<std::string, std::less<>> & usedNames );

    int GetControl() const override;

    int GetLevelMageGuild() const;

    const MageGuild & GetMageGuild() const
    {
        return _mageGuild;
    }

    bool HaveLibraryCapability() const
    {
        return _race == Race::WZRD;
    }

    bool isLibraryBuild() const
    {
        return _race == Race::WZRD && isBuild( BUILD_SPEC );
    }

    void MageGuildEducateHero( HeroBase & hero ) const
    {
        _mageGuild.educateHero( hero, GetLevelMageGuild(), isLibraryBuild() );
    }

    bool isFortificationBuilt() const
    {
        return _race == Race::KNGT && isBuild( BUILD_SPEC );
    }

    const Army & GetArmy() const
    {
        return _army;
    }

    Army & GetArmy()
    {
        return _army;
    }

    const Army & GetActualArmy() const;
    Army & GetActualArmy();

    // Returns current monsters count in dwelling.
    uint32_t getMonstersInDwelling( const uint32_t buildingType ) const;

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

    void ChangeColor( const int newColor );

    void ActionNewDay();
    void ActionNewWeek();
    void ActionNewMonth() const
    {
        // Do nothing.
    }

    void ActionPreBattle();
    void ActionAfterBattle( const bool attackerWins );

    void DrawImageCastle( const fheroes2::Point & pt ) const;

    CastleDialogReturnValue OpenDialog( const bool openConstructionWindow, const bool fade, const bool renderBackgroundDialog );

    int GetAttackModificator( const std::string * /* unused */ ) const
    {
        return 0;
    }

    int GetDefenseModificator( const std::string * /* unused */ ) const
    {
        return 0;
    }

    int GetPowerModificator( std::string * strs ) const;

    int GetKnowledgeModificator( const std::string * /* unused */ ) const
    {
        return 0;
    }

    int GetMoraleModificator( std::string * strs ) const;
    int GetLuckModificator( std::string * strs ) const;

    bool AllowBuyBuilding( const uint32_t buildingType ) const
    {
        return BuildingStatus::ALLOW_BUILD == CheckBuyBuilding( buildingType );
    }

    bool isBuild( const uint32_t buildingType ) const
    {
        return ( _constructedBuildings & buildingType ) != 0;
    }

    bool BuyBuilding( const uint32_t buildingType );

    BuildingStatus CheckBuyBuilding( const uint32_t build ) const;
    static BuildingStatus GetAllBuildingStatus( const Castle & castle );

    bool AllowBuyBoat( const bool checkPayment ) const;
    bool BuyBoat() const;

    void Scout() const;

    std::string GetStringBuilding( const uint32_t buildingType ) const
    {
        return GetStringBuilding( buildingType, _race );
    }

    std::string GetDescriptionBuilding( const uint32_t buildingType ) const;

    static const char * GetStringBuilding( const uint32_t buildingType, const int race );

    // Get building ICN ID for given race and building type.
    static int GetICNBuilding( const uint32_t buildingType, const int race );
    static int GetICNBoat( const int race );
    uint32_t GetUpgradeBuilding( const uint32_t buildingId ) const;

    static bool PredicateIsCastle( const Castle * castle )
    {
        return castle && castle->isCastle();
    }

    static bool PredicateIsTown( const Castle * castle )
    {
        return castle && !castle->isCastle();
    }

    static bool PredicateIsBuildBuilding( const Castle * castle, const uint32_t buildingType )
    {
        return castle && castle->isBuild( buildingType );
    }

    static uint32_t GetGrownWell();
    static uint32_t GetGrownWel2();
    static uint32_t GetGrownWeekOf();
    static uint32_t GetGrownMonthOf();

    std::string String() const;

    int DialogBuyHero( const Heroes * hero ) const;
    int DialogBuyCastle( const bool hasButtons = true ) const;

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
    bool _isExactBuildingBuilt( const uint32_t buildingToCheck ) const;

    uint32_t * _getDwelling( const uint32_t buildingType );
    void _educateHeroes();

    ConstructionDialogResult _openConstructionDialog( uint32_t & dwellingTobuild );

    void _openTavern() const;
    void _openWell();
    void _openMageGuild( const Heroes * hero ) const;
    void _joinRNDArmy();
    void _postLoad();

    void _wellRedrawAvailableMonsters( const uint32_t dwellingType, const bool restoreBackground, fheroes2::Image & background ) const;
    void _wellRedrawBackground( fheroes2::Image & background ) const;
    void _wellRedrawMonsterAnimation( const fheroes2::Rect & roi, std::array<fheroes2::RandomMonsterAnimation, maxNumOfDwellings> & monsterAnimInfo ) const;

    void _setDefaultBuildings();

    // Recruit maximum monsters from the castle. Returns 'true' if the recruit was made.
    bool _recruitCastleMax( const Troops & currentCastleArmy );
    bool _recruitMonsterFromDwelling( const uint32_t buildingType, const uint32_t count, const bool force = false );

    friend OStreamBase & operator<<( OStreamBase & stream, const Castle & castle );
    friend IStreamBase & operator>>( IStreamBase & stream, Castle & castle );

    int _race{ Race::NONE };
    uint32_t _constructedBuildings{ 0 };
    uint32_t _disabledBuildings{ 0 };

    Captain _captain{ *this };

    std::string _name;

    MageGuild _mageGuild;
    std::array<uint32_t, maxNumOfDwellings> _dwelling{ 0 };
    Army _army{ &_captain };
};

namespace CastleDialog
{
    // Class used for fading animation
    class FadeBuilding
    {
    public:
        FadeBuilding() = default;

        void startFadeBuilding( const uint32_t building )
        {
            _alpha = 0;
            _building = building;
            _isOnlyBoat = false;
        }

        void startFadeBoat()
        {
            _alpha = 0;
            _building = BUILD_SHIPYARD;
            _isOnlyBoat = true;
        }

        bool updateFadeAlpha();

        bool isFadeDone() const
        {
            return _alpha == 255;
        }

        void stopFade()
        {
            _alpha = 255;
            _building = BUILD_NOTHING;
            _isOnlyBoat = false;
        }

        uint8_t getAlpha() const
        {
            return _alpha;
        }

        uint32_t getBuilding() const
        {
            return _building;
        }

        bool isOnlyBoat() const
        {
            return _isOnlyBoat;
        }

    private:
        uint32_t _building{ BUILD_NOTHING };
        uint8_t _alpha{ 255 };
        bool _isOnlyBoat{ false };
    };

    struct BuildingRenderInfo
    {
        BuildingRenderInfo( const BuildingType buildingType, const fheroes2::Rect & buildingRect )
            : id( buildingType )
            , coord( buildingRect )
        {
            // Do nothing.
        }

        bool operator==( const uint32_t buildingType ) const
        {
            return buildingType == static_cast<uint32_t>( id );
        }

        BuildingType id;
        fheroes2::Rect coord;
    };

    struct BuildingsRenderQueue : std::vector<BuildingRenderInfo>
    {
        BuildingsRenderQueue( const Castle & castle, const fheroes2::Point & top );
    };

    void redrawAllBuildings( const Castle & castle, const fheroes2::Point & offset, const BuildingsRenderQueue & buildings,
                             const CastleDialog::FadeBuilding & alphaBuilding, const uint32_t animationIndex );
}

struct VecCastles : public std::vector<Castle *>
{
    VecCastles() = default;

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
