/***************************************************************************
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#ifndef H2BATTLE_TROOP_H
#define H2BATTLE_TROOP_H

#include <utility>
#include <vector>

#include "battle.h"
#include "battle_animation.h"
#include "battle_arena.h"
#include "battle_army.h"
#include "bitmodes.h"
#include "game_delays.h"

class Spell;
class HeroBase;

namespace Battle
{
    struct ModeDuration : public std::pair<u32, u32>
    {
        ModeDuration();
        ModeDuration( u32, u32 );

        bool isMode( u32 ) const;
        bool isZeroDuration( void ) const;
        void DecreaseDuration( void );
    };

    struct ModesAffected : public std::vector<ModeDuration>
    {
        ModesAffected();

        u32 GetMode( u32 ) const;
        void AddMode( u32, u32 );
        void RemoveMode( u32 );
        void DecreaseDuration( void );

        u32 FindZeroDuration( void ) const;
    };

    StreamBase & operator<<( StreamBase &, const ModesAffected & );
    StreamBase & operator>>( StreamBase &, ModesAffected & );

    enum
    {
        CONTOUR_MAIN = 0,
        CONTOUR_REFLECT = 0x01
    };

    // battle troop stats
    class Unit : public ArmyTroop, public BitModes, public Control
    {
    public:
        Unit( const Troop &, s32 pos, bool reflect );
        ~Unit();

        bool isModes( u32 ) const;
        bool isBattle( void ) const;
        std::string GetShotString( void ) const;
        std::string GetSpeedString( void ) const;
        u32 GetHitPointsLeft( void ) const;
        u32 GetAffectedDuration( u32 ) const;
        u32 GetSpeed( void ) const;

        Unit * GetMirror();
        void SetMirror( Unit * );
        void SetRandomMorale( void );
        void SetRandomLuck( void );
        void NewTurn( void );

        bool isValid( void ) const;
        bool isArchers( void ) const;
        bool isFlying( void ) const;
        bool isTwiceAttack( void ) const;

        bool AllowResponse( void ) const;
        bool isHandFighting( void ) const;
        bool isReflect( void ) const;
        bool isHaveDamage( void ) const;
        bool isMagicResist( const Spell &, u32 ) const;
        bool OutOfWalls( void ) const;
        bool canReach( int index ) const;
        bool canReach( const Unit & unit ) const;

        std::string String( bool more = false ) const;

        u32 GetUID( void ) const;
        bool isUID( u32 ) const;

        s32 GetHeadIndex( void ) const;
        s32 GetTailIndex( void ) const;
        const Position & GetPosition( void ) const;
        void SetPosition( s32 );
        void SetPosition( const Position & );
        void SetReflection( bool );

        u32 GetAttack( void ) const;
        u32 GetDefense( void ) const;
        int GetArmyColor( void ) const;
        int GetColor( void ) const;
        int GetCurrentColor() const; // the unit can be under spell what changes its affiliation
        int GetCurrentControl() const;
        u32 GetSpeed( bool skip_standing_check ) const;
        int GetControl( void ) const;
        u32 GetDamage( const Unit & ) const;
        s32 GetScoreQuality( const Unit & ) const;
        u32 GetDead( void ) const;
        u32 GetHitPoints( void ) const;
        u32 GetShots( void ) const;
        u32 ApplyDamage( Unit &, u32 );
        u32 ApplyDamage( u32 );
        uint32_t CalculateRetaliationDamage( uint32_t damageTaken ) const;
        u32 CalculateMinDamage( const Unit & ) const;
        u32 CalculateMaxDamage( const Unit & ) const;
        u32 CalculateDamageUnit( const Unit &, float ) const;
        bool ApplySpell( const Spell &, const HeroBase * hero, TargetInfo & );
        bool AllowApplySpell( const Spell &, const HeroBase * hero, std::string * msg = NULL, bool forceApplyToAlly = false ) const;
        void PostAttackAction( Unit & );
        void ResetBlind( void );
        void SpellModesAction( const Spell &, u32, const HeroBase * );
        void SpellApplyDamage( const Spell &, u32, const HeroBase *, TargetInfo & );
        void SpellRestoreAction( const Spell &, u32, const HeroBase * );
        u32 Resurrect( u32, bool, bool );

        bool SwitchAnimation( int rule, bool reverse = false );
        bool SwitchAnimation( const std::vector<int> & animationList, bool reverse = false );
        const AnimationState & GetFrameState( void ) const;
        AnimationSequence GetFrameState( int ) const;
        void SetDeathAnim();
        void IncreaseAnimFrame( bool loop = false );
        bool isStartAnimFrame( void ) const;
        bool isFinishAnimFrame( void ) const;
        int GetFrame( void ) const;
        int GetFrameStart( void ) const;
        int GetFrameCount( void ) const;
        uint32_t GetCustomAlpha() const;
        void SetCustomAlpha( uint32_t alpha );

        Point GetStartMissileOffset( size_t ) const;

        int M82Attk( void ) const;
        int M82Kill( void ) const;
        int M82Move( void ) const;
        int M82Wnce( void ) const;
        int M82Expl( void ) const;

        int ICNFile( void ) const;
        int ICNMiss( void ) const;

        Point GetBackPoint( void ) const;
        Point GetCenterPoint() const;
        Rect GetRectPosition( void ) const;

        u32 HowManyCanKill( const Unit & ) const;
        u32 HowManyWillKilled( u32 ) const;

        void SetResponse( void );
        void UpdateDirection( void );
        bool UpdateDirection( const Rect & );
        void PostKilledAction( void );

        u32 GetMagicResist( const Spell &, u32 ) const;
        int GetSpellMagic( bool force = false ) const;
        u32 GetObstaclesPenalty( const Unit & ) const;

        const HeroBase * GetCommander( void ) const;

        static bool isHandFighting( const Unit &, const Unit & );

        int GetAnimationState() const;
        bool isIdling() const;
        bool checkIdleDelay();

        // Find a better way to expose it without a million getters/setters
        AnimationState animation;

    private:
        friend StreamBase & operator<<( StreamBase &, const Unit & );
        friend StreamBase & operator>>( StreamBase &, Unit & );

        u32 uid;
        u32 hp;
        u32 count0;
        u32 dead;
        u32 shots;
        u32 disruptingray;
        bool reflect;

        Position position;
        ModesAffected affected;
        Unit * mirror;
        RandomizedDelay idleTimer;

        bool blindanswer;
        uint32_t customAlphaMask;
    };

    StreamBase & operator<<( StreamBase &, const Unit & );
    StreamBase & operator>>( StreamBase &, Unit & );
}

#endif
