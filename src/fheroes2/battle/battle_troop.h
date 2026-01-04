/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2026                                             *
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

#pragma once

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
#include "players.h"
#include "resource.h"

class Spell;
class HeroBase;

enum class PlayerColor : uint8_t;

namespace Rand
{
    class PCG32;
}

namespace Battle
{
    struct TargetInfo;

    struct ModeDuration : public std::pair<uint32_t, uint32_t>
    {
        ModeDuration( const uint32_t mode, const uint32_t duration )
            : std::pair<uint32_t, uint32_t>( mode, duration )
        {
            // Do nothing.
        }

        bool isMode( const uint32_t mode ) const
        {
            return ( first & mode ) != 0;
        }

        bool isZeroDuration() const
        {
            return 0 == second;
        }

        void DecreaseDuration()
        {
            if ( second ) {
                --second;
            }
        }
    };

    struct ModesAffected : public std::vector<ModeDuration>
    {
        ModesAffected()
        {
            reserve( 3 );
        }

        uint32_t GetMode( const uint32_t mode ) const;
        void AddMode( const uint32_t mode, const uint32_t duration );
        void RemoveMode( const uint32_t mode );
        void DecreaseDuration();

        uint32_t FindZeroDuration() const;
    };

    class Unit : public ArmyTroop, public BitModes, public Control
    {
    public:
        Unit( const Troop & troop, const Position & pos, const bool isReflected, const uint32_t uid );

        Unit( const Unit & ) = delete;

        ~Unit() override = default;

        Unit & operator=( const Unit & ) = delete;

        bool isModes( const uint32_t modes_ ) const override;
        bool isBattle() const override;
        std::string GetShotString() const override;
        std::string GetSpeedString() const override;
        uint32_t GetHitPointsLeft() const override;
        virtual uint32_t GetMissingHitPoints() const;
        uint32_t GetAffectedDuration( const uint32_t mode ) const override;
        uint32_t GetSpeed() const override;
        int GetMorale() const override;

        Unit * GetMirror()
        {
            return _mirrorUnit;
        }

        void SetMirror( Unit * ptr )
        {
            _mirrorUnit = ptr;
        }

        void SetRandomMorale( Rand::PCG32 & randomGenerator );
        void SetRandomLuck( Rand::PCG32 & randomGenerator );
        void NewTurn();

        bool isFlying() const;
        bool isDoubleAttack() const;
        bool isRetaliationAllowed() const;
        // Checks whether this unit is forced to fight in melee (there is an enemy unit nearby)
        bool isHandFighting() const;

        bool isReflect() const
        {
            return _isReflected;
        }

        bool isHaveDamage() const;
        bool isOutOfCastleWalls() const;

        std::string String( const bool more = false ) const;

        uint32_t GetUID() const
        {
            return _uid;
        }

        bool isUID( const uint32_t uid ) const
        {
            return _uid == uid;
        }

        int32_t GetHeadIndex() const;
        int32_t GetTailIndex() const;

        const Position & GetPosition() const
        {
            return _position;
        }

        void SetPosition( const int32_t index );
        void SetPosition( const Position & pos );
        void SetReflection( const bool isReflected );

        uint32_t GetAttack() const override;
        uint32_t GetDefense() const override;

        PlayerColor GetArmyColor() const
        {
            return GetColor();
        }

        // Returns the current color of the unit according to its current combat state (the unit
        // may be under a spell that changes its affiliation).
        PlayerColor GetCurrentColor() const;
        // Returns the current unit color (if valid, the unit's color can be invalid if the unit
        // is under the Berserker spell), otherwise returns the color of the unit's army.
        PlayerColor GetCurrentOrArmyColor() const;

        int GetControl() const override;
        int GetCurrentControl() const;

        // Returns the current speed of the unit, optionally performing additional checks in accordance
        // with the call arguments. If 'skipStandingCheck' is set to false, then the method returns
        // Speed::STANDING if the unit is immovable due to spells cast on it or if this unit is dead
        // (contains 0 fighters). Additionally, if 'skipMovedCheck' is set to false, then this method
        // returns Speed::STANDING if the unit has already completed its turn. If 'skipStandingCheck'
        // is set to true, then the value of 'skipMovedCheck' doesn't matter.
        uint32_t GetSpeed( const bool skipStandingCheck, const bool skipMovedCheck ) const;

        uint32_t GetDamage( const Unit & enemy, Rand::PCG32 & randomGenerator ) const;

        // Returns the threat level of this unit, calculated as if it attacked the 'defender' unit. See
        // the implementation for details.
        double evaluateThreatForUnit( const Unit & defender ) const;

        uint32_t GetInitialCount() const
        {
            return _initialCount;
        }

        uint32_t GetMaxCount() const
        {
            return _maxCount;
        }

        uint32_t GetDead() const
        {
            return _deadCount;
        }

        uint32_t GetHitPoints() const
        {
            return _hitPoints;
        }

        // Returns the cost of this unit, suitable for calculating the cost of surrendering the army (see
        // the implementation for details). Discounts are not applied when calculating this cost.
        Funds GetSurrenderCost() const;

        uint32_t GetShots() const override
        {
            return _shotsLeft;
        }

        bool isImmovable() const;

        uint32_t ApplyDamage( Unit & enemy, const uint32_t dmg, uint32_t & killed, uint32_t * ptrResurrected );

        uint32_t CalculateMinDamage( const Unit & enemy ) const;
        uint32_t CalculateMaxDamage( const Unit & enemy ) const;
        uint32_t CalculateDamageUnit( const Unit & enemy, double dmg ) const;

        // Returns average estimated damage the current unit can do for the specific enemy unit.
        uint32_t getPotentialDamage( const Unit & enemy ) const;

        // Returns a very rough estimate of the retaliatory damage after this unit receives the damage of the specified value.
        // The returned value is not suitable for accurate calculations, but only for approximate comparison with other units
        // in similar circumstances.
        uint32_t EstimateRetaliatoryDamage( const uint32_t damageTaken ) const;

        bool ApplySpell( const Spell & spell, const HeroBase * applyingHero, TargetInfo & target );
        bool AllowApplySpell( const Spell & spell, const HeroBase * applyingHero, const bool forceApplyToAlly = false ) const;
        std::vector<Spell> getCurrentSpellEffects() const;

        void PostAttackAction( const Unit & enemy );

        // Sets whether a unit performs a retaliatory attack while being blinded (i.e. with reduced efficiency)
        void SetBlindRetaliation( const bool value )
        {
            _blindRetaliation = value;
        }

        uint32_t CalculateSpellDamage( const Spell & spell, uint32_t spellPower, const HeroBase * applyingHero, const uint32_t targetDamage,
                                       const bool ignoreDefendingHero ) const;

        // Returns true if the animation was correctly changed and if it is valid.
        bool SwitchAnimation( const int rule, const bool reverse = false );
        // Returns true if the animation was correctly changed and if it is valid.
        bool SwitchAnimation( const std::vector<int> & animationList, const bool reverse = false );

        void IncreaseAnimFrame( const bool loop = false )
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
            return _customAlphaMask;
        }

        void SetCustomAlpha( const uint8_t alpha )
        {
            _customAlphaMask = alpha;
        }

        fheroes2::Point GetStartMissileOffset( const size_t direction ) const;

        int M82Attk( const Unit & enemy ) const;
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
            return _position.GetRect();
        }

        uint32_t HowManyWillBeKilled( const uint32_t dmg ) const;

        void setRetaliationAsCompleted();

        void UpdateDirection();
        bool UpdateDirection( const fheroes2::Rect & pos );

        void PostKilledAction();

        uint32_t GetMagicResist( const Spell & spell, const HeroBase * applyingHero ) const;
        Spell GetSpellMagic( Rand::PCG32 & randomGenerator ) const;

        const HeroBase * GetCommander() const;
        // If the color of the current unit is valid (i.e. this unit is not under the influence of a Berserker spell), then returns the commander of the army with the
        // corresponding color. Otherwise, returns the commander of the unit's army.
        const HeroBase * GetCurrentOrArmyCommander() const;

        // Checks whether the attacker will fight the defender in melee.
        static bool isHandFighting( const Unit & attacker, const Unit & defender );

        int GetAnimationState() const
        {
            return animation.getCurrentState();
        }

        bool isIdling() const;

        bool checkIdleDelay()
        {
            return _idleTimer.checkDelay();
        }

        // Removes temporary affection(s) (usually spell effect(s)). Multiple affections can be removed using a single call.
        void removeAffection( const uint32_t mode );

        // TODO: find a better way to expose it without a million getters/setters
        AnimationState animation;

    private:
        // Returns the count of killed troops.
        uint32_t _applyDamage( const uint32_t dmg );
        uint32_t _resurrect( const uint32_t points, const bool allowToExceedMaxCount, const bool isTemporary );

        // Applies a damage-causing spell to this unit.
        void _spellApplyDamage( const Spell & spell, const uint32_t spellPower, const HeroBase * applyingHero, TargetInfo & target );
        // Applies a restoring or reviving spell to this unit.
        void _spellRestoreAction( const Spell & spell, const uint32_t spellPoints, const HeroBase * applyingHero );
        // Applies a spell to this unit that changes its parameters.
        void _spellModesAction( const Spell & spell, uint32_t duration, const HeroBase * applyingHero );

        // Adds a temporary affection (usually a spell effect) with the specified duration. Only one affection can be added.
        void _addAffection( const uint32_t mode, const uint32_t duration );

        // Replaces some temporary affection(s) with another affection. Multiple affections can be replaced by a new one (but
        // only one) with a single call.
        void _replaceAffection( const uint32_t modeToReplace, const uint32_t replacementMode, const uint32_t duration );

        const uint32_t _uid{ 0 };

        // The total number of hit points of the unit.
        uint32_t _hitPoints{ 0 };
        const uint32_t _initialCount{ 0 };
        uint32_t _maxCount{ 0 };
        uint32_t _deadCount{ 0 };
        uint32_t _shotsLeft{ 0 };
        uint32_t _disruptingRaysNum{ 0 };

        Position _position;
        ModesAffected _affected;
        Unit * _mirrorUnit{ nullptr };
        RandomizedDelay _idleTimer;

        uint8_t _customAlphaMask{ 255 };

        bool _isReflected{ false };
        // Whether a unit performs a retaliatory attack while being blinded (i.e. with reduced efficiency)
        bool _blindRetaliation{ false };
    };
}
