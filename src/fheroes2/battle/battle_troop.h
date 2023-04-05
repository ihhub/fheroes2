/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2010 by Andrey Afletdinov <fheroes2@gmail.com>          *
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

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "army_troop.h"
#include "battle_animation.h"
#include "battle_cell.h"
#include "bitmodes.h"
#include "math_base.h"
#include "payment.h"
#include "players.h"

class Spell;
class HeroBase;

namespace Rand
{
    class DeterministicRandomGenerator;
}

namespace Battle
{
    struct TargetInfo;

    struct ModeDuration : public std::pair<uint32_t, uint32_t>
    {
        ModeDuration( uint32_t, uint32_t );

        bool isMode( uint32_t ) const;
        bool isZeroDuration() const;
        void DecreaseDuration();
    };

    struct ModesAffected : public std::vector<ModeDuration>
    {
        ModesAffected();

        uint32_t GetMode( uint32_t ) const;
        void AddMode( uint32_t, uint32_t );
        void RemoveMode( uint32_t );
        void DecreaseDuration();

        uint32_t FindZeroDuration() const;
    };

    class Unit : public ArmyTroop, public BitModes, public Control
    {
    public:
        Unit( const Troop & t, const Position & pos, const bool ref, const Rand::DeterministicRandomGenerator & randomGenerator, const uint32_t uid );
        Unit( const Unit & ) = delete;

        Unit & operator=( const Unit & ) = delete;

        bool isModes( uint32_t v ) const override;
        bool isBattle() const override;
        std::string GetShotString() const override;
        std::string GetSpeedString() const override;
        uint32_t GetHitPointsLeft() const override;
        virtual uint32_t GetMissingHitPoints() const;
        uint32_t GetAffectedDuration( uint32_t mod ) const override;
        uint32_t GetSpeed() const override;
        int GetMorale() const override;

        Unit * GetMirror()
        {
            return mirror;
        }

        void SetMirror( Unit * ptr )
        {
            mirror = ptr;
        }

        void SetRandomMorale();
        void SetRandomLuck();
        void NewTurn();

        bool isFlying() const;
        bool isDoubleAttack() const;

        bool AllowResponse() const;
        // Checks whether this unit is forced to fight in melee (there is an enemy unit nearby)
        bool isHandFighting() const;

        bool isReflect() const
        {
            return reflect;
        }

        bool isHaveDamage() const;

        bool isMagicResist( const Spell & spell, const uint32_t attackingArmySpellPower, const HeroBase * attackingHero ) const
        {
            return 100 <= GetMagicResist( spell, attackingArmySpellPower, attackingHero );
        }

        bool OutOfWalls() const;
        bool canReach( int index ) const;
        bool canReach( const Unit & unit ) const;

        std::string String( bool more = false ) const;

        uint32_t GetUID() const
        {
            return _uid;
        }

        bool isUID( uint32_t v ) const
        {
            return _uid == v;
        }

        int32_t GetHeadIndex() const;
        int32_t GetTailIndex() const;

        const Position & GetPosition() const
        {
            return position;
        }

        void SetPosition( const int32_t idx );
        void SetPosition( const Position & pos );
        void SetReflection( bool );

        uint32_t GetAttack() const override;
        uint32_t GetDefense() const override;

        int GetArmyColor() const
        {
            return ArmyTroop::GetColor();
        }

        int GetColor() const override;
        int GetCurrentColor() const; // the unit can be under spell what changes its affiliation
        int GetCurrentOrArmyColor() const; // current unit color (if valid), color of the unit's army otherwise
        int GetCurrentControl() const;
        uint32_t GetMoveRange() const;
        uint32_t GetSpeed( bool skipStandingCheck, bool skipMovedCheck ) const;
        int GetControl() const override;
        uint32_t GetDamage( const Unit & ) const;
        int32_t GetScoreQuality( const Unit & ) const;
        uint32_t GetInitialCount() const;
        uint32_t GetDead() const;
        uint32_t GetHitPoints() const;
        payment_t GetSurrenderCost() const;

        uint32_t GetShots() const override
        {
            return shots;
        }

        uint32_t ApplyDamage( Unit & enemy, const uint32_t dmg, uint32_t & killed, uint32_t * ptrResurrected );
        uint32_t CalculateRetaliationDamage( uint32_t damageTaken ) const;
        uint32_t CalculateMinDamage( const Unit & ) const;
        uint32_t CalculateMaxDamage( const Unit & ) const;
        uint32_t CalculateDamageUnit( const Unit & enemy, double dmg ) const;
        bool ApplySpell( const Spell &, const HeroBase * hero, TargetInfo & );
        bool AllowApplySpell( const Spell &, const HeroBase * hero, std::string * msg = nullptr, bool forceApplyToAlly = false ) const;
        bool isUnderSpellEffect( const Spell & spell ) const;
        std::vector<Spell> getCurrentSpellEffects() const;
        void PostAttackAction();
        void SetBlindAnswer( bool value );
        void SpellModesAction( const Spell &, uint32_t, const HeroBase * );
        void SpellApplyDamage( const Spell & spell, uint32_t spellPoints, const HeroBase * hero, TargetInfo & target );
        uint32_t CalculateSpellDamage( const Spell & spell, uint32_t spellPoints, const HeroBase * hero, uint32_t targetDamage, bool ignoreDefendingHero ) const;
        void SpellRestoreAction( const Spell &, uint32_t, const HeroBase * );

        bool SwitchAnimation( int rule, bool reverse = false );
        bool SwitchAnimation( const std::vector<int> & animationList, bool reverse = false );

        void IncreaseAnimFrame( bool loop = false )
        {
            animation.playAnimation( loop );
        }

        bool isFinishAnimFrame() const
        {
            return animation.isLastFrame();
        }

        int GetFrame() const
        {
            return animation.getFrame();
        }

        uint8_t GetCustomAlpha() const
        {
            return customAlphaMask;
        }

        void SetCustomAlpha( uint8_t alpha )
        {
            customAlphaMask = alpha;
        }

        fheroes2::Point GetStartMissileOffset( size_t ) const;

        int M82Attk() const;
        int M82Kill() const;
        int M82Move() const;
        int M82Wnce() const;
        int M82Expl() const;
        int M82Tkof() const;
        int M82Land() const;

        fheroes2::Point GetBackPoint() const;
        fheroes2::Point GetCenterPoint() const;

        fheroes2::Rect GetRectPosition() const
        {
            return position.GetRect();
        }

        uint32_t HowManyWillKilled( uint32_t ) const;

        void SetResponse();
        void UpdateDirection();
        bool UpdateDirection( const fheroes2::Rect & );
        void PostKilledAction();

        uint32_t GetMagicResist( const Spell & spell, const uint32_t attackingArmySpellPower, const HeroBase * attackingHero ) const;
        int GetSpellMagic() const;

        const HeroBase * GetCommander() const;
        const HeroBase * GetCurrentOrArmyCommander() const; // commander of the army with the current unit color (if valid), commander of the unit's army otherwise

        // Checks whether the attacker will fight the defender in melee
        static bool isHandFighting( const Unit & attacker, const Unit & defender );

        int GetAnimationState() const
        {
            return animation.getCurrentState();
        }

        bool isIdling() const;

        bool checkIdleDelay()
        {
            return idleTimer.checkDelay();
        }

        // Remove temporary affection(s) (usually spell effect(s)). Multiple affections can be removed using a single call.
        void removeAffection( const uint32_t mode );

        // TODO: find a better way to expose it without a million getters/setters
        AnimationState animation;

    private:
        uint32_t ApplyDamage( const uint32_t dmg );
        uint32_t Resurrect( const uint32_t points, const bool allow_overflow, const bool skip_dead );

        // Add a temporary affection (usually a spell effect) with the specified duration. Only one affection can be added.
        void addAffection( const uint32_t mode, const uint32_t duration );

        // Replace some temporary affection(s) with another affection. Multiple affections can be replaced by a new one (but
        // only one) with a single call.
        void replaceAffection( const uint32_t modeToReplace, const uint32_t replacementMode, const uint32_t duration );

        const uint32_t _uid;
        uint32_t hp;
        uint32_t _initialCount;
        uint32_t dead;
        uint32_t shots;
        uint32_t disruptingray;
        bool reflect;

        Position position;
        ModesAffected affected;
        Unit * mirror;
        RandomizedDelay idleTimer;

        bool blindanswer;
        uint8_t customAlphaMask;

        const Rand::DeterministicRandomGenerator & _randomGenerator;
    };
}

#endif
