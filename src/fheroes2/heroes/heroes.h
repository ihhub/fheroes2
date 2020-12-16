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

#ifndef H2HEROES_H
#define H2HEROES_H

#include <list>
#include <string>
#include <vector>

#include "army.h"
#include "gamedefs.h"
#include "heroes_base.h"
#include "pairs.h"
#include "route.h"
#include "visit.h"

class Recruits;

namespace Battle
{
    class Only;
}

class Heroes : public HeroBase, public ColorBase
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
        // from campain
        ROLAND,
        CORLAGON,
        ELIZA,
        ARCHIBALD,
        HALTON,
        BAX,
        // from extended
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
        SANDYSANDY,
        UNKNOWN
    };

    static fheroes2::Image GetPortrait( int heroid, int type );
    static const char * GetName( int heroid );

    enum flags_t
    {
        SHIPMASTER = 0x00000001,
        // UNUSED	= 0x00000002,
        SPELLCASTED = 0x00000004,
        ENABLEMOVE = 0x00000008,
        SAVE_SP_POINTS = 0x00000010,
        // UNUSED	= 0x00000020,
        // UNUSED	= 0x00000040,
        JAIL = 0x00000080,
        ACTION = 0x00000100,
        SAVE_MP_POINTS = 0x00000200,
        SLEEPER = 0x00000400,
        GUARDIAN = 0x00000800,
        NOTDEFAULTS = 0x00001000,
        NOTDISMISS = 0x00002000,
        VISIONS = 0x00004000,
        PATROL = 0x00008000,
        CUSTOMARMY = 0x00010000,
        CUSTOMSKILLS = 0x00020000
    };

    Heroes();
    Heroes( int heroid, int rc );

    bool isValid( void ) const;
    bool isFreeman( void ) const;
    void SetFreeman( int reason );

    const Castle * inCastle( void ) const;
    Castle * inCastle( void );

    void LoadFromMP2( s32 map_index, int cl, int rc, StreamBuf );
    void PostLoad( void );

    int GetRace( void ) const;
    const std::string & GetName( void ) const;
    int GetColor( void ) const;
    int GetType( void ) const;
    int GetControl( void ) const;

    int GetKillerColor( void ) const;
    void SetKillerColor( int );

    const Army & GetArmy( void ) const;
    Army & GetArmy( void );

    int GetID( void ) const;

    double getMeetingValue( const Heroes & otherHero ) const;
    double getRecruitValue() const;
    int getStatsValue() const;

    int GetAttack( void ) const;
    int GetDefense( void ) const;
    int GetPower( void ) const;
    int GetKnowledge( void ) const;

    int GetAttack( std::string * ) const;
    int GetDefense( std::string * ) const;
    int GetPower( std::string * ) const;
    int GetKnowledge( std::string * ) const;

    void IncreasePrimarySkill( int skill );

    int GetMorale( void ) const;
    int GetLuck( void ) const;
    int GetMoraleWithModificators( std::string * str = NULL ) const;
    int GetLuckWithModificators( std::string * str = NULL ) const;
    int GetLevel( void ) const;

    int GetMapsObject( void ) const;
    void SetMapsObject( int );

    const Point & GetCenterPatrol( void ) const;
    void SetCenterPatrol( const Point & );
    int GetSquarePatrol( void ) const;

    u32 GetMaxSpellPoints( void ) const;
    u32 GetMaxMovePoints( void ) const;

    u32 GetMovePoints( void ) const;
    void IncreaseMovePoints( u32 );
    bool MayStillMove( void ) const;
    void ResetMovePoints( void );
    void MovePointsScaleFixed( void );
    void RecalculateMovePoints( void );

    bool HasSecondarySkill( int ) const;
    bool HasMaxSecondarySkill( void ) const;
    int GetLevelSkill( int ) const;
    u32 GetSecondaryValues( int ) const;
    void LearnSkill( const Skill::Secondary & );
    Skill::SecSkills & GetSecondarySkills( void );

    bool PickupArtifact( const Artifact & );
    bool HasUltimateArtifact( void ) const;
    u32 GetCountArtifacts( void ) const;
    bool IsFullBagArtifacts( void ) const;

    int GetMobilityIndexSprite( void ) const;
    int GetManaIndexSprite( void ) const;

    int OpenDialog( bool readonly = false, bool fade = false );
    void MeetingDialog( Heroes & );

    bool Recruit( int col, const Point & pt );
    bool Recruit( const Castle & castle );

    void ActionNewDay( void );
    void ActionNewWeek( void );
    void ActionNewMonth( void );
    void ActionAfterBattle( void );
    void ActionPreBattle( void );

    bool BuySpellBook( const Castle *, int shrine = 0 );

    const Route::Path & GetPath( void ) const;
    Route::Path & GetPath( void );
    int GetRangeRouteDays( s32 ) const;
    void ShowPath( bool );
    void RescanPath( void );
    void RescanPathPassable( void );

    int GetDirection( void ) const;
    void setDirection( int directionToSet );

    void SetVisited( s32, Visit::type_t = Visit::LOCAL );
    void SetVisitedWideTile( s32, int object, Visit::type_t = Visit::LOCAL );
    bool isObjectTypeVisited( int object, Visit::type_t = Visit::LOCAL ) const;
    bool isVisited( const Maps::Tiles &, Visit::type_t = Visit::LOCAL ) const;
    bool hasMetWithHero( int heroID ) const;
    void markHeroMeeting( int heroID );

    bool Move( bool fast = false );
    void Move2Dest( const int32_t destination );
    bool isMoveEnabled( void ) const;
    bool CanMove( void ) const;
    void SetMove( bool );
    bool isAction( void ) const;
    void ResetAction( void );
    void Action( s32 );
    void ActionNewPosition( void );
    void ApplyPenaltyMovement( uint32_t penalty );
    bool ActionSpellCast( const Spell & );

    void Redraw( fheroes2::Image &, s32, s32, bool ) const;
    void PortraitRedraw( s32, s32, int type, fheroes2::Image & ) const;
    int GetSpriteIndex( void ) const;

    // These 2 methods must be used only for hero's animation. Please never use them anywhere else!
    void SetSpriteIndex( int index );
    void SetOffset( const fheroes2::Point & offset );

    void FadeOut( const Point & offset = Point() ) const;
    void FadeIn( const Point & offset = Point() ) const;
    void Scoute( void ) const;
    int GetScoute( void ) const;
    int CanScouteTile( s32 ) const;
    u32 GetVisionsDistance( void ) const;

    bool isShipMaster( void ) const;
    void SetShipMaster( bool );
    uint32_t lastGroundRegion() const;
    void setLastGroundRegion( uint32_t regionID );

    u32 GetExperience( void ) const;
    void IncreaseExperience( u32 );

    bool AllowBattle( bool attacker ) const;

    std::string String( void ) const;
    fheroes2::Image GetPortrait( int type ) const;

    static int GetLevelFromExperience( u32 );
    static u32 GetExperienceFromLevel( int );

    static void ScholarAction( Heroes &, Heroes & );

    Point MovementDirection() const;

private:
    friend StreamBase & operator<<( StreamBase &, const Heroes & );
    friend StreamBase & operator>>( StreamBase &, Heroes & );
#ifdef WITH_XML
    friend TiXmlElement & operator>>( TiXmlElement &, Heroes & );
#endif
    friend class Recruits;
    friend class Battle::Only;

    void LevelUp( bool skipsecondary, bool autoselect = false );
    int LevelUpPrimarySkill( void );
    void LevelUpSecondarySkill( int, bool autoselect = false );
    void AngleStep( int );
    bool MoveStep( bool fast = false );
    static void MoveStep( Heroes &, s32 to, bool newpos );
    static uint32_t GetStartingXp();
    bool isInVisibleMapArea() const;

    // This function is useful only in a situation when AI hero moves out of the fog
    // we don't update his direction during movement under the fog so there is a situation
    // when initial hero's sprite is set incorrectly. This function fixes it
    void SetValidDirectionSprite();

    uint32_t UpdateMovementPoints( const uint32_t movePoints, const int skill ) const;

    enum
    {
        SKILL_VALUE = 100
    };

    std::string name;
    ColorBase killer_color;
    u32 experience;
    s32 move_point_scale;

    Skill::SecSkills secondary_skills;

    Army army;

    int hid; /* hero id */
    int portrait; /* hero id */
    int race;
    int save_maps_object;

    Route::Path path;

    int direction;
    int sprite_index;
    fheroes2::Point _offset; // used only during hero's movement

    Point patrol_center;
    int patrol_square;

    std::list<IndexObject> visit_object;
    // persist this value later
    uint32_t _lastGroundRegion = 0;

    mutable int _alphaValue;

    enum
    {
        HERO_MOVE_STEP = 4 // in pixels
    };
};

struct VecHeroes : public std::vector<Heroes *>
{
    Heroes * Get( int /* hero id */ ) const;
    Heroes * Get( const Point & ) const;
};

struct AllHeroes : public VecHeroes
{
    AllHeroes();
    ~AllHeroes();

    void Init( void );
    void clear( void );

    void Scoute( int ) const;

    Heroes * GetGuest( const Castle & ) const;
    Heroes * GetGuard( const Castle & ) const;
    Heroes * GetFreeman( int race ) const;
    Heroes * FromJail( s32 ) const;

    bool HaveTwoFreemans( void ) const;
};

StreamBase & operator<<( StreamBase &, const VecHeroes & );
StreamBase & operator>>( StreamBase &, VecHeroes & );

StreamBase & operator<<( StreamBase &, const Heroes & );
StreamBase & operator>>( StreamBase &, Heroes & );

StreamBase & operator<<( StreamBase &, const AllHeroes & );
StreamBase & operator>>( StreamBase &, AllHeroes & );

#endif
