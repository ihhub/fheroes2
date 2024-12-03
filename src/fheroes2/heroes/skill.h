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

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

class IStreamBase;
class OStreamBase;

class Heroes;
class HeroBase;

namespace Skill
{
    // Total number of primary skills available in the game
    inline constexpr int numOfPrimarySkills{ 4 };
    // Total number of secondary skills available in the game
    inline constexpr int numOfSecondarySkills{ 14 };

    class Secondary;

    int GetLeadershipModifiers( int level, std::string * strs );
    int GetLuckModifiers( int level, std::string * strs );

    uint32_t GetNecromancyBonus( const HeroBase & hero );
    uint32_t GetNecromancyPercent( const HeroBase & hero );
    uint32_t GetDiplomacySurrenderCostDiscount( const int level );

    namespace Level
    {
        enum
        {
            NONE = 0,
            BASIC = 1,
            ADVANCED = 2,
            EXPERT = 3
        };

        const char * String( int level );
        std::string StringWithBonus( const Heroes & hero, const Secondary & skill );
    }

    class Secondary final : public std::pair<int, int>
    {
    public:
        enum
        {
            UNKNOWN = 0,
            PATHFINDING = 1,
            ARCHERY = 2,
            LOGISTICS = 3,
            SCOUTING = 4,
            DIPLOMACY = 5,
            NAVIGATION = 6,
            LEADERSHIP = 7,
            WISDOM = 8,
            MYSTICISM = 9,
            LUCK = 10,
            BALLISTICS = 11,
            EAGLE_EYE = 12,
            NECROMANCY = 13,
            ESTATES = 14
        };

        Secondary();
        Secondary( int skill, int level );

        void Reset();
        void Set( const Secondary & );
        void SetSkill( int );
        void SetLevel( int );
        void NextLevel();

        int Level() const
        {
            return second;
        }

        int Skill() const
        {
            return first;
        }

        bool isSkill( int skill ) const
        {
            return skill == first;
        }

        bool isValid() const;

        std::string GetName() const;
        std::string GetNameWithBonus( const Heroes & hero ) const;
        std::string GetDescription( const Heroes & hero ) const;
        uint32_t GetValue() const;

        // Returns the sprite index from SECSKILL
        int GetIndexSprite1() const;
        // Returns the sprite index from MINISS
        int GetIndexSprite2() const;

        static int RandForWitchsHut();
        static const char * String( int );
    };

    class SecSkills final : protected std::vector<Secondary>
    {
    public:
        SecSkills();
        explicit SecSkills( const int race );

        int Count() const;
        int GetLevel( int skill ) const;
        int GetTotalLevel() const;
        uint32_t GetValue( int skill ) const;

        Secondary * FindSkill( int );

        void AddSkill( const Skill::Secondary & );
        void FillMax( const Skill::Secondary & );

        std::pair<Secondary, Secondary> FindSkillsForLevelUp( const int race, const uint32_t firstSkillSeed, uint32_t const secondSkillSeed ) const;

        std::string String() const;

        std::vector<Secondary> & ToVector();
        const std::vector<Secondary> & ToVector() const;

        friend OStreamBase & operator<<( OStreamBase & stream, const SecSkills & ss );
        friend IStreamBase & operator>>( IStreamBase & stream, SecSkills & ss );
    };

    class Primary
    {
    public:
        virtual ~Primary() = default;

        enum
        {
            UNKNOWN = 0,
            ATTACK = 1,
            DEFENSE = 2,
            POWER = 3,
            KNOWLEDGE = 4
        };

        virtual int GetAttack() const = 0;
        virtual int GetDefense() const = 0;
        virtual int GetPower() const = 0;
        virtual int GetKnowledge() const = 0;
        virtual int GetMorale() const = 0;
        virtual int GetLuck() const = 0;
        virtual int GetRace() const = 0;

        int LevelUp( int race, int level, uint32_t seed );

        // Returns the sum of the values of the four primary skills (attack, defense, power and knowledge), belonging directly to the hero (i.e. excluding artifacts)
        int getTotalPrimarySkillLevel() const
        {
            return attack + defense + power + knowledge;
        }

        static const char * String( const int skillType );
        static std::string StringDescription( int, const Heroes * );
        static int GetInitialSpell( int race );
        static int getHeroDefaultSkillValue( const int skill, const int race );

    protected:
        void LoadDefaults( int type, int race );

        friend OStreamBase & operator<<( OStreamBase & stream, const Primary & skill );
        friend IStreamBase & operator>>( IStreamBase & stream, Primary & skill );

        int attack{ 0 };
        int defense{ 0 };
        int power{ 0 };
        int knowledge{ 0 };
    };

    OStreamBase & operator<<( OStreamBase & stream, const SecSkills & ss );
    IStreamBase & operator>>( IStreamBase & stream, SecSkills & ss );

    OStreamBase & operator<<( OStreamBase & stream, const Primary & skill );
    IStreamBase & operator>>( IStreamBase & stream, Primary & skill );
}
