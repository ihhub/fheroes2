/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2025                                             *
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

#include <cassert> // IWYU pragma: keep
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <list>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "army.h"
#include "artifact.h"
#include "color.h"
#include "direction.h"
#include "heroes_base.h"
#include "math_base.h"
#include "mp2.h"
#include "pairs.h"
#include "race.h"
#include "route.h"
#include "skill.h"
#include "spell.h"
#include "visit.h"

class IStreamBase;
class OStreamBase;

class Castle;

namespace Battle
{
    class Only;
}

namespace Maps
{
    class Tile;

    namespace Map_Format
    {
        struct HeroMetadata;
    }
}

namespace fheroes2
{
    class Image;
    class Sprite;

    enum class SupportedLanguage : uint8_t;
}

class Heroes final : public HeroBase, public ColorBase
{
public:
    friend class Battle::Only;

    // Maximum number of hero's secondary skills
    static constexpr int maxNumOfSecSkills{ 8 };

    enum : int32_t
    {
        // Unknown / undefined hero.
        UNKNOWN,

        // Knight heroes from The Succession Wars.
        LORDKILBURN,
        SIRGALLANTH,
        ECTOR,
        GVENNETH,
        TYRO,
        AMBROSE,
        RUBY,
        MAXIMUS,
        DIMITRY,

        // Barbarian heroes from The Succession Wars.
        THUNDAX,
        FINEOUS,
        JOJOSH,
        CRAGHACK,
        JEZEBEL,
        JACLYN,
        ERGON,
        TSABU,
        ATLAS,

        // Sorceress heroes from The Succession Wars.
        ASTRA,
        NATASHA,
        TROYAN,
        VATAWNA,
        REBECCA,
        GEM,
        ARIEL,
        CARLAWN,
        LUNA,

        // Warlock heroes from The Succession Wars.
        ARIE,
        ALAMAR,
        VESPER,
        CRODO,
        BAROK,
        KASTORE,
        AGAR,
        FALAGAR,
        WRATHMONT,

        // Wizard heroes from The Succession Wars.
        MYRA,
        FLINT,
        DAWN,
        HALON,
        MYRINI,
        WILFREY,
        SARAKIN,
        KALINDRA,
        MANDIGAL,

        // Necromancer heroes from The Succession Wars.
        ZOM,
        DARLANA,
        ZAM,
        RANLOO,
        CHARITY,
        RIALDO,
        ROXANA,
        SANDRO,
        CELIA,

        // The Succession Wars campaign heroes.
        ROLAND,
        CORLAGON,
        ELIZA,
        ARCHIBALD,
        HALTON,
        BRAX,

        // The Price of Loyalty expansion heroes.
        SOLMYR,
        DAINWIN,
        MOG,
        UNCLEIVAN,
        JOSEPH,
        GALLAVANT,
        ELDERIAN,
        CEALLACH,
        DRAKONIA,
        MARTINE,
        JARKONAS,

        // Debug hero. Should not be used anywhere outside the development!
        DEBUG_HERO,

        // Resurrection expansion heroes.

        // IMPORTANT! Put all new heroes just above this line.
        HEROES_COUNT
    };

    enum : uint32_t
    {
        SHIPMASTER = 0x00000001,

        // UNUSED = 0x00000002,

        SPELLCASTED = 0x00000004,
        ENABLEMOVE = 0x00000008,

        // UNUSED = 0x00000010,
        // UNUSED = 0x00000020,

        // Hero is available for recruitment in any kingdom
        RECRUIT = 0x00000040,
        JAIL = 0x00000080,
        ACTION = 0x00000100,
        // Hero must retain his movement points if he retreated or surrendered and was then rehired on the same day
        SAVEMP = 0x00000200,
        SLEEPER = 0x00000400,

        // UNUSED = 0x00000800,

        // Hero has non-standard properties that were set using the map editor
        CUSTOM = 0x00001000,
        NOTDISMISS = 0x00002000,
        VISIONS = 0x00004000,
        PATROL = 0x00008000
    };

    // Types of hero roles. They are only for AI as humans are smart enough to manage heroes by themselves.
    // The order of roles is important as it is used to identify more valuable heroes among others.
    enum class Role : int
    {
        // The main goal for Scout is to discover new areas so he should run towards the fog of war to expand the visible territory.
        // These heroes usually appear when either no tasks exist on the map or when AI has too many heroes.
        SCOUT,

        // Courier's life is to deliver things from one place to another. Usually they bring an army for Fighters or Champions from
        // dwellings, castles or from one hero to another.
        COURIER,

        // The most ordinary hero's role with no any specialization. This type does everything what a hero can do:
        // collecting resources, fighting (mostly weak) monsters, claiming towns and mines and expanding the visible territory.
        HUNTER,

        // The type of hero with a skew towards fights. His main priority is to kill monsters and enemies, capture castles and guarded mines.
        // This type still can capture valuable mines or dwellings if they're on the way to something better.
        FIGHTER,

        // The mightiest hero among others. The main purpose of this type is to run over the enemy's territory and defeat all heroes there while
        // capturing all castles and towns. This type of hero is set when one (or few) heroes are too strong in comparison to others.
        // A hero to be defeated as a winning condition for human must be marked as this type of role.
        CHAMPION
    };

    // This class is used to update a flag for an AI hero to make him available to meet other heroes.
    // Such cases happen after battles, reinforcements or collecting artifacts.
    class AIHeroMeetingUpdater
    {
    public:
        explicit AIHeroMeetingUpdater( Heroes & hero )
            : _hero( hero )
            , _initialArmyStrength( hero.GetArmy().GetStrength() )
        {
            // Do nothing.
        }

        AIHeroMeetingUpdater( const AIHeroMeetingUpdater & ) = delete;

        ~AIHeroMeetingUpdater()
        {
            double currentArmyStrength = 0;

            try {
                // Army::GetStrength() could potentially throw an exception, and SonarQube complains about a potentially uncaught exception
                // in the destructor. This is not a problem per se, because calling std::terminate() is OK, so let's just do this ourselves.
                currentArmyStrength = _hero.GetArmy().GetStrength();
            }
            catch ( ... ) {
                // This should never happen
                assert( 0 );
                std::terminate();
            }

            if ( std::fabs( _initialArmyStrength - currentArmyStrength ) > 0.001 ) {
                _hero.unmarkHeroMeeting();
            }
        }

        AIHeroMeetingUpdater & operator=( const AIHeroMeetingUpdater & ) = delete;

    private:
        Heroes & _hero;
        const double _initialArmyStrength;
    };

    static const int heroFrameCountPerTile{ 9 };

    Heroes() = default;
    Heroes( const int heroId, const int race );
    Heroes( const int heroId, const int race, const uint32_t additionalExperience )
        : Heroes( heroId, race )
    {
        IncreaseExperience( additionalExperience, true );
    }

    Heroes( const Heroes & ) = delete;

    ~Heroes() override = default;

    Heroes & operator=( const Heroes & ) = delete;

    bool isValid() const override;
    // Returns true if the hero is active on the adventure map (i.e. has a valid ID, is not imprisoned, and is hired by
    // some kingdom), otherwise returns false
    bool isActive() const;

    // Returns true if the hero is available for hire (i.e. has a valid ID, is not imprisoned, and is not hired by any
    // kingdom), otherwise returns false
    bool isAvailableForHire() const;
    // Dismisses the hero (makes him available for hire) because of a 'reason'. See the implementation for details.
    void Dismiss( const int reason );

    bool isLosingGame() const;
    const Castle * inCastle() const override;
    Castle * inCastleMutable() const;

    void LoadFromMP2( const int32_t mapIndex, const PlayerColor colorType, const int raceType, const bool isInJail, const std::vector<uint8_t> & data );

    void applyHeroMetadata( const Maps::Map_Format::HeroMetadata & heroMetadata, const bool isInJail, const bool isEditor );
    // Updates data in heroMetadata and returns true if it has changes.
    Maps::Map_Format::HeroMetadata getHeroMetadata() const;

    int GetRace() const override;
    const std::string & GetName() const override;
    PlayerColor GetColor() const override;
    int GetType() const override;
    int GetControl() const override;

    const Army & GetArmy() const override;
    Army & GetArmy() override;

    int GetID() const
    {
        return _id;
    }

    double getMeetingValue( const Heroes & receivingHero ) const;
    double getRecruitValue() const;
    int getStatsValue() const;

    void setAttackBaseValue( const int baseValue )
    {
        attack = baseValue;
    }

    void setDefenseBaseValue( const int baseValue )
    {
        defense = baseValue;
    }

    void setPowerBaseValue( const int baseValue )
    {
        power = baseValue;
    }

    void setKnowledgeBaseValue( const int baseValue )
    {
        knowledge = baseValue;
    }

    // Get hero's Attack skill base value without any modificators.
    int getAttackBaseValue() const
    {
        return attack;
    }

    // Get hero's Defense skill base value without any modificators.
    int getDefenseBaseValue() const
    {
        return defense;
    }

    // Get hero's Spell Power skill base value without any modificators.
    int getPowerBaseValue() const
    {
        return power;
    }

    // Get hero's Knowledge skill base value without any modificators.
    int getKnowledgeBaseValue() const
    {
        return knowledge;
    }

    int GetAttack() const override;
    int GetDefense() const override;
    int GetPower() const override;
    int GetKnowledge() const override;

    int GetAttack( std::string * text ) const;
    int GetDefense( std::string * text ) const;
    int GetPower( std::string * text ) const;
    int GetKnowledge( std::string * text ) const;

    void IncreasePrimarySkill( const int skill );

    int GetMorale() const override;
    int GetLuck() const override;
    int getMoraleWithModifiers( std::string * text ) const;
    int getLuckWithModifiers( std::string * text ) const;

    int GetLevel() const
    {
        return GetLevelFromExperience( _experience );
    }

    MP2::MapObjectType getObjectTypeUnderHero() const
    {
        return _objectTypeUnderHero;
    }

    void setObjectTypeUnderHero( const MP2::MapObjectType objectType )
    {
        _objectTypeUnderHero = ( ( objectType != MP2::OBJ_HERO ) ? objectType : MP2::OBJ_NONE );
    }

    const fheroes2::Point & GetPatrolCenter() const
    {
        return _patrolCenter;
    }

    void SetPatrolCenter( const fheroes2::Point & pos )
    {
        _patrolCenter = pos;
    }

    uint32_t GetPatrolDistance() const
    {
        return _patrolDistance;
    }

    uint32_t GetMaxSpellPoints() const override;

    // Returns the maximum number of hero movement points, depending on the surface type on which the hero is currently located
    uint32_t GetMaxMovePoints() const
    {
        return GetMaxMovePoints( isShipMaster() );
    }

    // Returns the maximum number of hero movement points, depending on the specified surface type (water or land)
    uint32_t GetMaxMovePoints( const bool onWater ) const;

    uint32_t GetMovePoints() const
    {
        return move_point;
    }

    void IncreaseMovePoints( const uint32_t point )
    {
        move_point += point;
    }

    bool MayStillMove( const bool ignorePath, const bool ignoreSleeper ) const;

    void ResetMovePoints()
    {
        move_point = 0;
    }

    bool HasSecondarySkill( const int skill ) const;
    bool HasMaxSecondarySkill() const;
    int GetLevelSkill( const int skill ) const override;
    uint32_t GetSecondarySkillValue( const int skill ) const override;
    void LearnSkill( const Skill::Secondary & skill );

    Skill::SecSkills & GetSecondarySkills()
    {
        return _secondarySkills;
    }

    bool PickupArtifact( const Artifact & art );

    bool HasUltimateArtifact() const
    {
        return bag_artifacts.ContainUltimateArtifact();
    }

    uint32_t GetCountArtifacts() const
    {
        return bag_artifacts.CountArtifacts();
    }

    bool IsFullBagArtifacts() const
    {
        return bag_artifacts.isFull();
    }

    uint32_t GetMobilityIndexSprite() const;

    // Returns the relative height of mana column near hero's portrait in heroes panel. Returned value will be in range [0; 25].
    uint32_t GetManaIndexSprite() const;

    int OpenDialog( const bool readonly, const bool fade, const bool disableDismiss, const bool disableSwitch, const bool renderBackgroundDialog, const bool isEditor,
                    const fheroes2::SupportedLanguage language );
    void MeetingDialog( Heroes & );

    bool Recruit( const PlayerColor col, const fheroes2::Point & pt );
    bool Recruit( const Castle & castle );

    void ActionNewDay();
    void ActionNewWeek();
    void ActionAfterBattle() override;
    void ActionPreBattle() override;

    bool BuySpellBook( const Castle & castle );

    const Route::Path & GetPath() const
    {
        return _path;
    }

    Route::Path & GetPath()
    {
        return _path;
    }

    // Returns the number of travel days to the tile with the dstIdx index using the pathfinder from the World global
    // object, or zero if the destination tile is unreachable. The number of days returned is limited, see the source
    // of this method.
    int getNumOfTravelDays( const int32_t dstIdx ) const;

    void ShowPath( const bool show )
    {
        show ? _path.Show() : _path.Hide();
    }

    // Calculates the hero's path to the tile with the dstIdx index using the pathfinder from the World global object.
    // Recalculates the existing path if dstIdx is negative. Not applicable if you want to use a pathfinder other than
    // PlayerWorldPathfinder.
    void calculatePath( int32_t dstIdx );

    int GetDirection() const
    {
        return _direction;
    }

    void setDirection( const int directionToSet )
    {
        if ( directionToSet != Direction::UNKNOWN ) {
            _direction = directionToSet;
        }
    }

    // Set the visited state for the tile with the given index
    void SetVisited( const int32_t tileIndex, const Visit::Type type = Visit::LOCAL );

    // Set the global visited state for the tile with the given index both for this hero's kingdom and its allies
    void setVisitedForAllies( const int32_t tileIndex ) const;

    bool isObjectTypeVisited( const MP2::MapObjectType object, const Visit::Type = Visit::LOCAL ) const;
    bool isVisited( const Maps::Tile & tile, const Visit::Type = Visit::LOCAL ) const;

    std::set<MP2::MapObjectType> getAllVisitedObjectTypes() const;

    // These methods are used only for AI.
    bool hasMetWithHero( const int heroId ) const;
    void markHeroMeeting( const int heroId );

    // Do not call this method directly. It is used by AIHeroMeetingUpdater class.
    void unmarkHeroMeeting();

    bool Move( const bool jumpToNextTile = false );
    void Move2Dest( const int32_t destinationIndex );
    bool isMoveEnabled() const;
    bool CanMove() const;
    void SetMove( const bool enable );

    bool isAction() const
    {
        return Modes( ACTION );
    }

    void ResetAction()
    {
        ResetModes( ACTION );
    }

    void Action( const int tileIndex );
    void ActionNewPosition( const bool allowMonsterAttack );
    void ActionSpellCast( const Spell & spell );

    // Update map in the scout area around the Hero on radar (mini-map).
    void ScoutRadar() const;

    bool MayCastAdventureSpells() const;

    void PortraitRedraw( const int32_t px, const int32_t py, const PortraitType type, fheroes2::Image & dstsf ) const override;

    int GetSpriteIndex() const
    {
        return _spriteIndex;
    }

    // These 2 methods must be used only for hero's animation. Please never use them anywhere else!
    void SetSpriteIndex( const int index )
    {
        _spriteIndex = index;
    }

    void SetOffset( const fheroes2::Point & offset )
    {
        _offset = offset;
    }

    fheroes2::Point getCurrentPixelOffset() const;

    // Performs a hero fade-out animation with the given speed multiplier and an optional offset
    void FadeOut( const int animSpeedMultiplier, const fheroes2::Point & offset = {} ) const;

    // Performs a hero fade-in animation with the given speed multiplier and an optional offset
    void FadeIn( const int animSpeedMultiplier, const fheroes2::Point & offset = {} ) const;

    // Performs a hero fade-out animation with an optional offset at the lowest possible speed
    void FadeOut( const fheroes2::Point & offset = {} ) const
    {
        FadeOut( 1, offset );
    }

    // Performs a hero fade-in animation with an optional offset at the lowest possible speed
    void FadeIn( const fheroes2::Point & offset = {} ) const
    {
        FadeIn( 1, offset );
    }

    void Scout( const int tileIndex ) const;
    int32_t GetScoutingDistance() const;

    // Returns the area in map tiles around hero's position in his scout range.
    fheroes2::Rect GetScoutRoi() const;

    static uint32_t GetVisionsDistance();

    bool isShipMaster() const
    {
        return Modes( SHIPMASTER );
    }

    void SetShipMaster( const bool isShipMaster );

    void setLastGroundRegion( const uint32_t regionID )
    {
        _lastGroundRegion = regionID;
    }

    uint32_t GetExperience() const
    {
        return _experience;
    }

    void IncreaseExperience( const uint32_t amount, const bool autoselect = false );

    std::string String() const;

    const fheroes2::Sprite & GetPortrait( const int type ) const
    {
        return Heroes::GetPortrait( _portrait, type );
    }

    int getPortraitId() const
    {
        return _portrait;
    }

    bool isPoLPortrait() const
    {
        return ( _portrait >= SOLMYR && _portrait <= JARKONAS );
    }

    fheroes2::Point MovementDirection() const;

    int GetAttackedMonsterTileIndex() const
    {
        return _attackedMonsterTileIndex;
    }

    void SetAttackedMonsterTileIndex( const int tileIndex )
    {
        _attackedMonsterTileIndex = tileIndex;
    }

    void setAIRole( const Role role )
    {
        _aiRole = role;
    }

    Role getAIRole() const
    {
        return _aiRole;
    }

    void setDimensionDoorUsage( const uint32_t newUsage )
    {
        _dimensionDoorsUsed = newUsage;
    }

    uint32_t getDimensionDoorUses() const
    {
        return _dimensionDoorsUsed;
    }

    uint8_t getAlphaValue() const
    {
        return static_cast<uint8_t>( _alphaValue );
    }

    double getAIMinimumJoiningArmyStrength() const;

    uint32_t getDailyRestoredSpellPoints() const;

    bool isInDeepOcean() const;

    static int GetLevelFromExperience( const uint32_t experience );
    static uint32_t GetExperienceFromLevel( const int level );

    static uint32_t getExperienceMaxValue();

    static const fheroes2::Sprite & GetPortrait( const int heroId, const int portraitType );
    static const char * getDefaultName( const int heroId );

    static bool isValidId( const int32_t heroId )
    {
        return heroId > UNKNOWN && heroId < HEROES_COUNT;
    }

    void resetHeroSprite();

    // Update French language-specific characters to match CP1252.
    // Call this method only when loading maps made with original French editor.
    void fixFrenchCharactersInName();

private:
    friend OStreamBase & operator<<( OStreamBase & stream, const Heroes & hero );
    friend IStreamBase & operator>>( IStreamBase & stream, Heroes & hero );

    enum
    {
        SKILL_VALUE = 100
    };

    struct HeroSeedsForLevelUp
    {
        uint32_t seedPrimarySkill{ 0 };
        uint32_t seedSecondarySkill1{ 0 };
        uint32_t seedSecondarySkill2{ 0 };
        uint32_t seedSecondarySkillRandomChoose{ 0 };
    };

    HeroSeedsForLevelUp _getSeedsForLevelUp() const;
    void _levelUp( const bool skipSecondary, const bool autoselect = false );
    void _levelUpSecondarySkill( const HeroSeedsForLevelUp & seeds, const int primary, const bool autoselect = false );
    void _angleStep( const int targetDirection );
    void _applyMovementPenalty( const uint32_t penalty );
    bool _moveStep( const bool jumpToNextTile );
    bool _isInVisibleMapArea() const;

    // This function is useful only in a situation when AI hero moves out of the fog
    // we don't update his direction during movement under the fog so there is a situation
    // when initial hero's sprite is set incorrectly. This function fixes it
    void _setValidDirectionSprite();

    uint32_t _updateMovementPoints( const uint32_t movePoints, const int skill ) const;

    // Daily replenishment of spell points
    void _replenishSpellPoints();

    static uint32_t _getStartingXp();

    uint32_t _experience{ 0 };
    std::string _name;

    Skill::SecSkills _secondarySkills;

    Army _army{ this };

    // Hero ID
    int _id{ UNKNOWN };
    // Corresponds to the ID of the hero whose portrait is applied. Usually equal to the
    // ID of this hero, unless a custom portrait is applied.
    int _portrait{ UNKNOWN };
    int _race{ Race::NONE };

    MP2::MapObjectType _objectTypeUnderHero{ MP2::OBJ_NONE };

    Route::Path _path{ *this };

    std::list<IndexObject> _visitedObjects;

    // Hero's direction on adventure map.
    int _direction{ Direction::RIGHT };
    // Hero's sprite index on adventure map.
    int _spriteIndex{ 18 };
    // Hero's sprite offset on adventure map, used only during hero's movement.
    fheroes2::Point _offset;

    fheroes2::Point _patrolCenter;
    uint32_t _patrolDistance{ 0 };

    uint32_t _lastGroundRegion{ 0 };

    // Tracking how many spells this hero used this turn
    uint32_t _dimensionDoorsUsed{ 0 };

    mutable int _alphaValue{ 255 };

    // Used only when hero attacks a group of wandering monsters.
    int _attackedMonsterTileIndex{ -1 };

    // This value should NOT be saved in save file as it's dynamically set during AI turn.
    Role _aiRole{ Role::HUNTER };
};

struct VecHeroes final : public std::vector<Heroes *>
{
    VecHeroes() = default;
};

class AllHeroes
{
public:
    AllHeroes() = default;
    AllHeroes( const AllHeroes & ) = delete;

    ~AllHeroes() = default;

    AllHeroes & operator=( const AllHeroes & ) = delete;

    auto begin() const noexcept
    {
        return Iterator( _heroes.begin() );
    }

    auto end() const noexcept
    {
        return Iterator( _heroes.end() );
    }

    void Init();

    void Clear()
    {
        _heroes.clear();
    }

    // Return the maximum allowed heroes on map limited by the count of hero portraits
    // and by the count of her default names.
    static constexpr size_t getMaximumAllowedHeroes()
    {
        // We exclude the "UNKNOWN" hero, the "debug" hero, all 17 heroes from campaign.
        return Heroes::HEROES_COUNT - 2 - 17;
    }

    Heroes * Get( const int heroId ) const;
    Heroes * Get( const fheroes2::Point & center ) const;

    void Scout( const PlayerColorsSet colors ) const;

    void ResetModes( const uint32_t modes ) const;

    void NewDay() const;
    void NewWeek() const;

    Heroes * GetHeroForHire( const int race, const int heroIDToIgnore ) const;
    Heroes * FromJail( const int32_t tileIndex ) const;

    template <typename BaseIterator>
    struct Iterator : public BaseIterator
    {
        explicit Iterator( BaseIterator && other ) noexcept
            : BaseIterator( std::move( other ) )
        {
            // Do nothing.
        }

        auto * operator*() const noexcept
        {
            return BaseIterator::operator*().get();
        }
    };

private:
    friend OStreamBase & operator<<( OStreamBase & stream, const AllHeroes & heroes );
    friend IStreamBase & operator>>( IStreamBase & stream, AllHeroes & heroes );

    std::vector<std::unique_ptr<Heroes>> _heroes;
};

OStreamBase & operator<<( OStreamBase & stream, const VecHeroes & heroes );
IStreamBase & operator>>( IStreamBase & stream, VecHeroes & heroes );
