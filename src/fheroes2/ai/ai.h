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

#ifndef H2AI_H
#define H2AI_H

#include <optional>

#include "mp2.h"
#include "payment.h"
#include "rand.h"

class StreamBase;
class Funds;
class Castle;
class HeroBase;
class Heroes;
class Kingdom;
class Army;
class Spell;
struct VecHeroes;

namespace Maps
{
    class Tiles;
}

namespace Battle
{
    class Arena;
    class Unit;
    class Actions;
}

namespace AI
{
    enum class AI_TYPE : int
    {
        NORMAL
    };
    enum AI_PERSONALITY
    {
        NONE,
        WARRIOR,
        BUILDER,
        EXPLORER
    };

    // Although AI heroes are capable to find their own tasks strategic AI should be able to focus them on most critical tasks
    enum class PriorityTaskType : int
    {
        // AI will focus on siegeing or chasing the selected enemy castle or hero.
        ATTACK,

        // Target will usually be a friendly castle. AI will move heroes to defend and garrison it.
        DEFEND,

        // Target must be a friendly castle or hero. AI with such priority set should focus on bringing more troops to the target.
        REINFORCE
    };

    const double ARMY_ADVANTAGE_DESPERATE = 0.8;
    const double ARMY_ADVANTAGE_SMALL = 1.3;
    const double ARMY_ADVANTAGE_MEDIUM = 1.5;
    const double ARMY_ADVANTAGE_LARGE = 1.8;

    class Base
    {
    public:
        virtual void KingdomTurn( Kingdom & kingdom ) = 0;
        virtual void BattleTurn( Battle::Arena & arena, const Battle::Unit & unit, Battle::Actions & actions ) = 0;

        virtual void revealFog( const Maps::Tiles & tile, const Kingdom & kingdom ) = 0;

        virtual void HeroesAdd( const Heroes & hero );
        virtual void HeroesRemove( const Heroes & hero );
        virtual void HeroesBeginMovement( Heroes & hero );
        virtual void HeroesFinishMovement( Heroes & hero );
        virtual void HeroesPreBattle( HeroBase & hero, bool isAttacking );
        virtual void HeroesAfterBattle( HeroBase & hero, bool wasAttacking );
        virtual void HeroesPostLoad( Heroes & hero );
        virtual void HeroesActionComplete( Heroes & hero, int32_t tileIndex, const MP2::MapObjectType objectType );
        virtual void HeroesActionNewPosition( Heroes & hero );
        virtual void HeroesLevelUp( Heroes & hero );
        virtual std::string HeroesString( const Heroes & hero );

        virtual void CastleAdd( const Castle & castle );
        virtual void CastleRemove( const Castle & castle );
        virtual void CastlePreBattle( Castle & castle );
        virtual void CastleAfterBattle( Castle & castle, bool attackerWins );

        virtual int GetPersonality() const; // To be utilized in future.
        virtual std::string GetPersonalityString() const;

        virtual void Reset();
        virtual void resetPathfinder() = 0;
        virtual bool isValidHeroObject( const Heroes & hero, const int32_t index, const bool underHero ) = 0;

        // Should be called at the beginning of the battle even if no AI-controlled players are
        // involved in the battle - because of the possibility of using instant or auto battle
        virtual void battleBegins() = 0;

        virtual ~Base() = default;

        virtual void tradingPostVisitEvent( Kingdom & kingdom ) = 0;

    protected:
        int _personality = NONE;

        Base() = default;

    private:
        friend StreamBase & operator<<( StreamBase &, const AI::Base & );
        friend StreamBase & operator>>( StreamBase &, AI::Base & );
    };

    // AI type selector, can be used sometime in the future
    Base & Get( AI_TYPE type = AI_TYPE::NORMAL );

    // Definitions are in the ai_hero_action.cpp

    void HeroesAction( Heroes & hero, const int32_t dst_index );
    void HeroesMove( Heroes & hero );

    // Makes it so that the 'hero' casts the Dimension Door spell to the 'targetIndex'
    void HeroesCastDimensionDoor( Heroes & hero, const int32_t targetIndex );

    // Makes it so that the 'hero' casts the Summon Boat spell, summoning the boat at the 'boatDestinationIndex'.
    // Returns the index of the tile on which the boat was located before the summoning. It's the caller's
    // responsibility to make sure that 'hero' may cast this spell and there is a summonable boat on the map
    // before calling this function.
    int32_t HeroesCastSummonBoat( Heroes & hero, const int32_t boatDestinationIndex );

    bool HeroesCastAdventureSpell( Heroes & hero, const Spell & spell );

    // Definitions are in the ai_common.cpp

    // Builds the given building in the given castle if possible. If possible and necessary, obtains the resources
    // that are missing for construction through trading on the marketplace. Returns true if the construction was
    // successful, otherwise returns false.
    bool BuildIfPossible( Castle & castle, const int building );

    // Builds the given building in the given castle if possible and if there is a sufficient supply of resources
    // (see the implementation for details). If possible and necessary, obtains the resources that are missing for
    // construction through trading on the marketplace. Returns true if the construction was successful, otherwise
    // returns false.
    bool BuildIfEnoughFunds( Castle & castle, const int building, const int fundsMultiplier );

    void OptimizeTroopsOrder( Army & hero );
    bool CanPurchaseHero( const Kingdom & kingdom );

    // Calculates a marketplace transaction, which will be enough for the kingdom to be able to make a payment in the
    // amount of at least 'fundsToObtain'. Returns the corresponding transaction if it was found, otherwise returns an
    // empty result. In order to receive the necessary funds, the returned transaction must be deducted from the funds
    // of the kingdom.
    std::optional<Funds> getMarketplaceTransaction( const Kingdom & kingdom, const Funds & fundsToObtain );

    // Performs operations on the marketplace necessary for the kingdom to be able to make a payment in the amount
    // of at least 'fundsToObtain'. If the necessary funds were not obtained as a result of trading, then the current
    // funds of the kingdom remain unchanged. Returns true if the trade was successful, otherwise returns false.
    bool tradeAtMarketplace( Kingdom & kingdom, const Funds & fundsToObtain );

    StreamBase & operator<<( StreamBase &, const AI::Base & );
    StreamBase & operator>>( StreamBase &, AI::Base & );
}

#endif
