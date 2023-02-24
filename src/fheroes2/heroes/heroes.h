/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
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

#ifndef H2HEROES_H
#define H2HEROES_H

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <exception>
#include <list>
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
#include "route.h"
#include "skill.h"
#include "spell.h"
#include "visit.h"

class Castle;
class StreamBase;
class StreamBuf;

namespace Battle
{
    class Only;
}

namespace Maps
{
    class Tiles;
}

namespace fheroes2
{
    class Image;
    class Sprite;
    struct ObjectRenderingInfo;
}

struct HeroSeedsForLevelUp
{
    uint32_t seedPrimarySkill = 0;
    uint32_t seedSecondaySkill1 = 0;
    uint32_t seedSecondaySkill2 = 0;
    uint32_t seedSecondaySkillRandomChoose = 0;
};

class Heroes final : public HeroBase, public ColorBase
{
public:
    enum
    {
        // knight
        LORDKILBURN,
        SIRGALLANTH,
        ECTOR,
        GVENNETH,
        TYRO,
        AMBROSE,
        RUBY,
        MAXIMUS,
        DIMITRY,
        // barbarian
        THUNDAX,
        FINEOUS,
        JOJOSH,
        CRAGHACK,
        JEZEBEL,
        JACLYN,
        ERGON,
        TSABU,
        ATLAS,
        // sorceress
        ASTRA,
        NATASHA,
        TROYAN,
        VATAWNA,
        REBECCA,
        GEM,
        ARIEL,
        CARLAWN,
        LUNA,
        // warlock
        ARIE,
        ALAMAR,
        VESPER,
        CRODO,
        BAROK,
        KASTORE,
        AGAR,
        FALAGAR,
        WRATHMONT,
        // wizard
        MYRA,
        FLINT,
        DAWN,
        HALON,
        MYRINI,
        WILFREY,
        SARAKIN,
        KALINDRA,
        MANDIGAL,
        // necromancer
        ZOM,
        DARLANA,
        ZAM,
        RANLOO,
        CHARITY,
        RIALDO,
        ROXANA,
        SANDRO,
        CELIA,
        // From The Succession Wars campaign.
        ROLAND,
        CORLAGON,
        ELIZA,
        ARCHIBALD,
        HALTON,
        BAX,
        // From The Price of Loyalty expansion.
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
        // debugger
        DEBUG_HERO,
        UNKNOWN
    };

    enum flags_t : uint32_t
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

        NOTDEFAULTS = 0x00001000,
        NOTDISMISS = 0x00002000,
        VISIONS = 0x00004000,
        PATROL = 0x00008000,

        // UNUSED = 0x00010000,

        CUSTOMSKILLS = 0x00020000
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
        AIHeroMeetingUpdater( AIHeroMeetingUpdater && ) = delete;

        AIHeroMeetingUpdater & operator=( const AIHeroMeetingUpdater & ) = delete;
        AIHeroMeetingUpdater & operator=( AIHeroMeetingUpdater && ) = delete;

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

    private:
        Heroes & _hero;
        const double _initialArmyStrength;
    };

    Heroes();
    Heroes( int heroid, int rc );
    Heroes( const int heroID, const int race, const uint32_t additionalExperience );
    Heroes( const Heroes & ) = delete;

    ~Heroes() override = default;

    Heroes & operator=( const Heroes & ) = delete;

    static const fheroes2::Sprite & GetPortrait( int heroid, int type );
    static const char * GetName( int heroid );

    bool isValid() const override;
    bool isFreeman() const;
    void SetFreeman( int reason );

    bool isLosingGame() const;
    const Castle * inCastle() const override;
    Castle * inCastleMutable() const;

    void LoadFromMP2( int32_t map_index, int cl, int rc, StreamBuf );
    void PostLoad();

    int GetRace() const override;
    const std::string & GetName() const override;
    int GetColor() const override;
    int GetType() const override;
    int GetControl() const override;

    const Army & GetArmy() const override;
    Army & GetArmy() override;

    int GetID() const
    {
        return hid;
    }

    double getMeetingValue( const Heroes & otherHero ) const;
    double getRecruitValue() const;
    int getStatsValue() const;

    int GetAttack() const override;
    int GetDefense() const override;
    int GetPower() const override;
    int GetKnowledge() const override;

    int GetAttack( std::string * ) const;
    int GetDefense( std::string * ) const;
    int GetPower( std::string * ) const;
    int GetKnowledge( std::string * ) const;

    void IncreasePrimarySkill( int skill );

    int GetMorale() const override;
    int GetLuck() const override;
    int GetMoraleWithModificators( std::string * str = nullptr ) const;
    int GetLuckWithModificators( std::string * str = nullptr ) const;

    int GetLevel() const
    {
        return GetLevelFromExperience( experience );
    }

    MP2::MapObjectType GetMapsObject() const;
    void SetMapsObject( const MP2::MapObjectType objectType );

    const fheroes2::Point & GetCenterPatrol() const
    {
        return patrol_center;
    }

    void SetCenterPatrol( const fheroes2::Point & pos )
    {
        patrol_center = pos;
    }

    int GetSquarePatrol() const
    {
        return patrol_square;
    }

    uint32_t GetMaxSpellPoints() const override;
    uint32_t GetMaxMovePoints() const;

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

    bool HasSecondarySkill( int ) const;
    bool HasMaxSecondarySkill() const;
    int GetLevelSkill( int ) const override;
    uint32_t GetSecondaryValues( int skill ) const override;
    void LearnSkill( const Skill::Secondary & );
    Skill::SecSkills & GetSecondarySkills();

    bool PickupArtifact( const Artifact & );
    bool HasUltimateArtifact() const;
    uint32_t GetCountArtifacts() const;
    bool IsFullBagArtifacts() const;

    int GetMobilityIndexSprite() const;

    // Returns the relative height of mana column near hero's portrait in heroes panel. Returned value will be in range [0; 25].
    int GetManaIndexSprite() const;

    int OpenDialog( const bool readonly, const bool fade, const bool disableDismiss, const bool disableSwitch, const bool renderBackgroundDialog = false );
    void MeetingDialog( Heroes & );

    bool Recruit( const int col, const fheroes2::Point & pt );
    bool Recruit( const Castle & castle );

    void ActionNewDay();
    void ActionNewWeek();
    void ActionNewMonth();
    void ActionAfterBattle() override;
    void ActionPreBattle() override;

    bool BuySpellBook( const Castle * castle );

    const Route::Path & GetPath() const
    {
        return path;
    }

    Route::Path & GetPath()
    {
        return path;
    }

    // Returns the number of travel days to the tile with the dstIdx index using the pathfinder from the World global
    // object, or zero if the destination tile is unreachable. The number of days returned is limited, see the source
    // of this method.
    int getNumOfTravelDays( int32_t dstIdx ) const;

    void ShowPath( const bool show )
    {
        show ? path.Show() : path.Hide();
    }

    // Calculates the hero's path to the tile with the dstIdx index using the pathfinder from the World global object.
    // Recalculates the existing path if dstIdx is negative. Not applicable if you want to use a pathfinder other than
    // PlayerWorldPathfinder.
    void calculatePath( int32_t dstIdx );

    int GetDirection() const
    {
        return direction;
    }

    void setDirection( const int directionToSet )
    {
        if ( directionToSet != Direction::UNKNOWN ) {
            direction = directionToSet;
        }
    }

    // set visited cell
    void SetVisited( int32_t, Visit::type_t = Visit::LOCAL );

    // Set global visited state for itself and for allies.
    void setVisitedForAllies( const int32_t tileIndex ) const;

    void SetVisitedWideTile( int32_t, const MP2::MapObjectType objectType, Visit::type_t = Visit::LOCAL );
    bool isObjectTypeVisited( const MP2::MapObjectType object, Visit::type_t = Visit::LOCAL ) const;
    bool isVisited( const Maps::Tiles &, Visit::type_t = Visit::LOCAL ) const;

    // These methods are used only for AI.
    bool hasMetWithHero( int heroID ) const;
    void markHeroMeeting( int heroID );

    // Do not call this method directly. It is used by AIHeroMeetingUpdater class.
    void unmarkHeroMeeting();

    bool Move( bool fast = false );
    void Move2Dest( const int32_t destination );
    bool isMoveEnabled() const;
    bool CanMove() const;
    void SetMove( bool );
    bool isAction() const;
    void ResetAction();
    void Action( int tileIndex, bool isDestination );
    void ActionNewPosition( const bool allowMonsterAttack );
    void ApplyPenaltyMovement( uint32_t penalty );
    void ActionSpellCast( const Spell & spell );

    // Update map in the scout area around the Hero on radar (mini-map).
    void ScoutRadar() const;

    bool MayCastAdventureSpells() const;

    // Since heroes sprite are much bigger than a tile we need to 'cut' the sprite and the shadow's sprite into pieces. Each piece is for a separate tile.
    std::vector<fheroes2::ObjectRenderingInfo> getHeroSpritesPerTile() const;
    std::vector<fheroes2::ObjectRenderingInfo> getHeroShadowSpritesPerTile() const;

    void PortraitRedraw( const int32_t px, const int32_t py, const PortraitType type, fheroes2::Image & dstsf ) const override;

    int GetSpriteIndex() const
    {
        return sprite_index;
    }

    // These 2 methods must be used only for hero's animation. Please never use them anywhere else!
    void SetSpriteIndex( const int index )
    {
        sprite_index = index;
    }

    void SetOffset( const fheroes2::Point & offset )
    {
        _offset = offset;
    }

    fheroes2::Point getCurrentPixelOffset() const;

    void FadeOut( const fheroes2::Point & offset = fheroes2::Point() ) const;
    void FadeIn( const fheroes2::Point & offset = fheroes2::Point() ) const;
    void Scoute( const int tileIndex ) const;
    int GetScoute() const;

    // Returns the area in map tiles around hero's position in his scout range.
    // For non-diagonal hero move the area is set only in move direction and one tile behind (to clear Hero's previous position).
    fheroes2::Rect GetScoutRoi( const bool ignoreDirection = false ) const;

    uint32_t GetVisionsDistance() const;

    bool isShipMaster() const;
    void SetShipMaster( bool );

    void setLastGroundRegion( const uint32_t regionID )
    {
        _lastGroundRegion = regionID;
    }

    uint32_t GetExperience() const
    {
        return experience;
    }

    void IncreaseExperience( const uint32_t amount, const bool autoselect = false );

    std::string String() const;

    const fheroes2::Sprite & GetPortrait( const int type ) const
    {
        return Heroes::GetPortrait( portrait, type );
    }

    int getPortraitId() const
    {
        return portrait;
    }

    static int GetLevelFromExperience( uint32_t );
    static uint32_t GetExperienceFromLevel( int );

    fheroes2::Point MovementDirection() const;

    int GetAttackedMonsterTileIndex() const
    {
        return _attackedMonsterTileIndex;
    }

    void SetAttackedMonsterTileIndex( const int idx )
    {
        _attackedMonsterTileIndex = idx;
    }

    void setAIRole( const Role role )
    {
        _aiRole = role;
    }

    Role getAIRole() const
    {
        return _aiRole;
    }

    uint8_t getAlphaValue() const
    {
        return static_cast<uint8_t>( _alphaValue );
    }

    double getAIMininumJoiningArmyStrength() const;

private:
    friend StreamBase & operator<<( StreamBase &, const Heroes & );
    friend StreamBase & operator>>( StreamBase &, Heroes & );

    friend class Recruits;
    friend class Battle::Only;

    HeroSeedsForLevelUp GetSeedsForLevelUp() const;
    void LevelUp( bool skipsecondary, bool autoselect = false );
    void LevelUpSecondarySkill( const HeroSeedsForLevelUp & seeds, int primary, bool autoselect = false );
    void AngleStep( int );
    bool MoveStep( bool fast = false );
    static void MoveStep( Heroes &, int32_t to, bool newpos );
    static uint32_t GetStartingXp();
    bool isInVisibleMapArea() const;

    // This function is useful only in a situation when AI hero moves out of the fog
    // we don't update his direction during movement under the fog so there is a situation
    // when initial hero's sprite is set incorrectly. This function fixes it
    void SetValidDirectionSprite();

    uint32_t UpdateMovementPoints( const uint32_t movePoints, const int skill ) const;

    // Daily replenishment of spell points
    void ReplenishSpellPoints();

    bool isInDeepOcean() const;

    enum
    {
        SKILL_VALUE = 100
    };

    std::string name;
    uint32_t experience;

    Skill::SecSkills secondary_skills;

    Army army;

    // Hero ID
    int hid;
    // Corresponds to the ID of the hero whose portrait is applied. Usually equal to the
    // ID of this hero, unless a custom portrait is applied.
    int portrait;
    int _race;
    int save_maps_object;

    Route::Path path;

    int direction;
    int sprite_index;
    fheroes2::Point _offset; // used only during hero's movement

    fheroes2::Point patrol_center;
    int patrol_square;

    std::list<IndexObject> visit_object;
    uint32_t _lastGroundRegion = 0;

    mutable int _alphaValue;

    int _attackedMonsterTileIndex; // used only when hero attacks a group of wandering monsters

    // This value should NOT be saved in save file as it's dynamically set during AI turn.
    Role _aiRole;

    enum
    {
        HERO_MOVE_STEP = 4 // in pixels
    };
};

struct VecHeroes : public std::vector<Heroes *>
{
    Heroes * Get( int /* hero id */ ) const;
    Heroes * Get( const fheroes2::Point & ) const;
};

struct AllHeroes : public VecHeroes
{
    AllHeroes();
    AllHeroes( const AllHeroes & ) = delete;

    ~AllHeroes();

    AllHeroes & operator=( const AllHeroes & ) = delete;

    void Init();
    void clear();

    void Scoute( int ) const;

    void ResetModes( const uint32_t modes ) const
    {
        std::for_each( begin(), end(), [modes]( Heroes * hero ) { hero->ResetModes( modes ); } );
    }

    void NewDay()
    {
        std::for_each( begin(), end(), []( Heroes * hero ) { hero->ActionNewDay(); } );
    }

    void NewWeek()
    {
        std::for_each( begin(), end(), []( Heroes * hero ) { hero->ActionNewWeek(); } );
    }

    void NewMonth()
    {
        std::for_each( begin(), end(), []( Heroes * hero ) { hero->ActionNewMonth(); } );
    }

    Heroes * GetHero( const Castle & castle ) const;
    Heroes * GetFreeman( const int race, const int heroIDToIgnore ) const;
    Heroes * FromJail( int32_t ) const;
};

StreamBase & operator<<( StreamBase &, const VecHeroes & );
StreamBase & operator>>( StreamBase &, VecHeroes & );

StreamBase & operator<<( StreamBase &, const Heroes & );
StreamBase & operator>>( StreamBase &, Heroes & );

StreamBase & operator<<( StreamBase &, const AllHeroes & );
StreamBase & operator>>( StreamBase &, AllHeroes & );

#endif
