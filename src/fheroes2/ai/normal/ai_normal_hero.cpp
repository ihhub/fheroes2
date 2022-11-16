/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2022                                             *
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

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <ostream>
#include <utility>
#include <vector>

#include "ai.h"
#include "ai_normal.h"
#include "army.h"
#include "army_troop.h"
#include "artifact.h"
#include "castle.h"
#include "color.h"
#include "game_over.h"
#include "game_static.h"
#include "gamedefs.h"
#include "ground.h"
#include "heroes.h"
#include "kingdom.h"
#include "logging.h"
#include "luck.h"
#include "maps.h"
#include "maps_tiles.h"
#include "math_base.h"
#include "monster.h"
#include "morale.h"
#include "mp2.h"
#include "pairs.h"
#include "payment.h"
#include "players.h"
#include "rand.h"
#include "resource.h"
#include "route.h"
#include "settings.h"
#include "skill.h"
#include "spell.h"
#include "visit.h"
#include "world.h"
#include "world_pathfinding.h"

namespace
{
    bool isHeroWhoseDefeatIsVictoryConditionForHumanInCastle( const Castle * castle )
    {
        assert( castle != nullptr );

        const Heroes * hero = castle->GetHero();
        if ( hero && hero == world.GetHeroesCondWins() ) {
            return true;
        }

        return false;
    }

    bool isFindArtifactVictoryConditionForHuman( const Artifact & art )
    {
        assert( art.isValid() );

        const Settings & conf = Settings::Get();

        if ( ( conf.ConditionWins() & GameOver::WINS_ARTIFACT ) == 0 ) {
            return false;
        }

        if ( conf.WinsFindUltimateArtifact() ) {
            return art.isUltimate();
        }

        return ( art.GetID() == conf.WinsFindArtifactID() );
    }

    bool isCastleLossConditionForHuman( const Castle * castle )
    {
        assert( castle != nullptr );

        const Settings & conf = Settings::Get();
        const bool isSinglePlayer = ( Colors( Players::HumanColors() ).size() == 1 );

        if ( isSinglePlayer && ( conf.ConditionLoss() & GameOver::LOSS_TOWN ) != 0 && castle->GetCenter() == conf.LossMapsPositionObject() ) {
            // It is a loss town condition for human.
            return true;
        }

        if ( conf.WinsCompAlsoWins() && ( conf.ConditionWins() & GameOver::WINS_TOWN ) != 0 && castle->GetCenter() == conf.WinsMapsPositionObject() ) {
            // It is a town capture winning condition for AI.
            return true;
        }

        return false;
    }

    bool AIShouldVisitCastle( const Heroes & hero, int castleIndex, const double heroArmyStrength )
    {
        const Castle * castle = world.getCastleEntrance( Maps::GetPoint( castleIndex ) );
        if ( castle == nullptr ) {
            return false;
        }

        if ( hero.GetColor() == castle->GetColor() ) {
            return castle->GetHero() == nullptr;
        }

        if ( hero.isFriends( castle->GetColor() ) ) {
            return false;
        }

        // WINS_HERO victory condition does not apply to AI-controlled players, we have to ignore the castle with this hero
        // to keep him alive for the human player
        if ( isHeroWhoseDefeatIsVictoryConditionForHumanInCastle( castle ) ) {
            return false;
        }

        const double advantage = hero.isLosingGame() ? AI::ARMY_ADVANTAGE_DESPERATE : AI::ARMY_ADVANTAGE_MEDIUM;
        return heroArmyStrength > castle->GetGarrisonStrength( &hero ) * advantage;
    }

    bool isHeroStrongerThan( const Maps::Tiles & tile, const MP2::MapObjectType objectType, AI::Normal & ai, const double heroArmyStrength,
                             const double targetStrengthMultiplier )
    {
        return heroArmyStrength > ai.getTargetArmyStrength( tile, objectType ) * targetStrengthMultiplier;
    }

    bool isArmyValuableToObtain( const Troop & monster, double armyStrengthThreshold, const bool armyHasMonster )
    {
        if ( armyHasMonster ) {
            // Since the army has the same monster the limit must be reduced.
            armyStrengthThreshold /= 2;
        }

        // Do not even care about this monster if it brings no visible advantage to the army.
        return monster.GetStrength() > armyStrengthThreshold;
    }

    bool HeroesValidObject( const Heroes & hero, const double heroArmyStrength, const int32_t index, const AIWorldPathfinder & pathfinder, AI::Normal & ai,
                            const double armyStrengthThreshold )
    {
        const Maps::Tiles & tile = world.GetTiles( index );
        const MP2::MapObjectType objectType = tile.GetObject();

        if ( !MP2::isActionObject( objectType ) ) {
            return false;
        }

        // WINS_ARTIFACT victory condition does not apply to AI-controlled players, we should leave this artifact untouched for the human player
        if ( MP2::isArtifactObject( objectType ) ) {
            const Artifact art = tile.QuantityArtifact();

            if ( art.isValid() && isFindArtifactVictoryConditionForHuman( art ) ) {
                return false;
            }
        }

        const Army & army = hero.GetArmy();
        const Kingdom & kingdom = hero.GetKingdom();

        switch ( objectType ) {
        case MP2::OBJ_SHIPWRECKSURVIVOR:
        case MP2::OBJ_WATERCHEST:
        case MP2::OBJ_FLOTSAM:
        case MP2::OBJ_BOTTLE:
        case MP2::OBJ_RESOURCE:
        case MP2::OBJ_CAMPFIRE:
        case MP2::OBJ_TREASURECHEST:
            return true;

        case MP2::OBJ_BUOY:
            return !hero.isObjectTypeVisited( objectType ) && hero.GetMorale() < Morale::BLOOD && !army.AllTroopsAreUndead();

        case MP2::OBJ_MERMAID:
            return !hero.isObjectTypeVisited( objectType ) && hero.GetLuck() < Luck::IRISH;

        case MP2::OBJ_SIRENS:
            return false;

        case MP2::OBJ_MAGELLANMAPS:
            // TODO: avoid hardcoded resource values for objects.
            return !hero.isObjectTypeVisited( MP2::OBJ_MAGELLANMAPS, Visit::GLOBAL ) && kingdom.AllowPayment( { Resource::GOLD, 1000 } );

        case MP2::OBJ_WHIRLPOOL:
            // AI should never consider a whirlpool as a destination point. It uses them only to make a path.
            return false;

        case MP2::OBJ_COAST:
            // Coast is not an action object. If this assertion blows up then something is wrong with the logic above.
            assert( 0 );
            return false;

        case MP2::OBJ_SAWMILL:
        case MP2::OBJ_MINES:
        case MP2::OBJ_ALCHEMYLAB:
        case MP2::OBJ_LIGHTHOUSE:
            if ( !hero.isFriends( tile.QuantityColor() ) ) {
                if ( tile.isCaptureObjectProtected() ) {
                    return isHeroStrongerThan( tile, objectType, ai, heroArmyStrength, AI::ARMY_ADVANTAGE_SMALL );
                }

                return true;
            }
            break;

        case MP2::OBJ_ABANDONEDMINE:
            if ( !hero.isFriends( tile.QuantityColor() ) ) {
                if ( tile.isCaptureObjectProtected() ) {
                    return isHeroStrongerThan( tile, objectType, ai, heroArmyStrength, AI::ARMY_ADVANTAGE_LARGE );
                }

                return true;
            }
            break;

        case MP2::OBJ_WAGON:
        case MP2::OBJ_LEANTO:
        case MP2::OBJ_SKELETON:
            return tile.QuantityIsValid();

        case MP2::OBJ_MAGICGARDEN:
        case MP2::OBJ_WATERWHEEL:
        case MP2::OBJ_WINDMILL:
            if ( Settings::Get().ExtWorldExtObjectsCaptured() && !hero.isFriends( tile.QuantityColor() ) ) {
                if ( tile.isCaptureObjectProtected() ) {
                    return isHeroStrongerThan( tile, objectType, ai, heroArmyStrength, AI::ARMY_ADVANTAGE_MEDIUM );
                }

                return true;
            }
            else if ( tile.QuantityIsValid() )
                return true;
            break;

        case MP2::OBJ_ARTIFACT: {
            const uint32_t variants = tile.QuantityVariant();

            if ( hero.IsFullBagArtifacts() )
                return false;

            // 1,2,3 - 2000g, 2500g+3res, 3000g+5res
            if ( 1 <= variants && 3 >= variants )
                return kingdom.AllowPayment( tile.QuantityFunds() );

            // 4,5 - need have skill wisard or leadership,
            if ( 3 < variants && 6 > variants )
                return hero.HasSecondarySkill( tile.QuantitySkill().Skill() );

            // 6 - 50 rogues, 7 - 1 gin, 8,9,10,11,12,13 - 1 monster level4
            if ( 5 < variants && 14 > variants ) {
                return isHeroStrongerThan( tile, objectType, ai, heroArmyStrength, AI::ARMY_ADVANTAGE_LARGE );
            }

            // other
            return true;
        }

        case MP2::OBJ_OBSERVATIONTOWER:
            return Maps::getFogTileCountToBeRevealed( index, GameStatic::getFogDiscoveryDistance( GameStatic::FogDiscoveryType::OBSERVATION_TOWER ), hero.GetColor() )
                   > 0;

        case MP2::OBJ_OBELISK:
            return !hero.isVisited( tile, Visit::GLOBAL );

        case MP2::OBJ_BARRIER:
            return kingdom.IsVisitTravelersTent( tile.QuantityColor() );

        case MP2::OBJ_TRAVELLERTENT:
            return !kingdom.IsVisitTravelersTent( tile.QuantityColor() );

        case MP2::OBJ_SHRINE1:
        case MP2::OBJ_SHRINE2:
        case MP2::OBJ_SHRINE3: {
            const Spell & spell = tile.QuantitySpell();
            assert( spell.isValid() );
            if ( !spell.isValid() || !hero.HaveSpellBook() || hero.HaveSpell( spell )
                 || ( 3 == spell.Level() && Skill::Level::NONE == hero.GetLevelSkill( Skill::Secondary::WISDOM ) ) ) {
                return false;
            }

            if ( hero.isObjectTypeVisited( objectType, Visit::GLOBAL )
                 && ( spell == Spell::VIEWARTIFACTS || spell == Spell::VIEWHEROES || spell == Spell::VIEWMINES || spell == Spell::VIEWRESOURCES
                      || spell == Spell::VIEWTOWNS || spell == Spell::IDENTIFYHERO || spell == Spell::VISIONS ) ) {
                // AI never uses View spells except "View All".
                return false;
            }
            return true;
        }

        // primary skill
        case MP2::OBJ_FORT:
        case MP2::OBJ_MERCENARYCAMP:
        case MP2::OBJ_DOCTORHUT:
        case MP2::OBJ_STANDINGSTONES:
        // exp
        case MP2::OBJ_GAZEBO:
            return !hero.isVisited( tile );

        // sec skill
        case MP2::OBJ_WITCHSHUT: {
            const Skill::Secondary & skill = tile.QuantitySkill();
            const int skillType = skill.Skill();

            if ( !skill.isValid() || hero.HasMaxSecondarySkill() || hero.HasSecondarySkill( skillType ) ) {
                return false;
            }

            if ( army.AllTroopsAreUndead() && skillType == Skill::Secondary::LEADERSHIP ) {
                // For undead army it's pointless to have Leadership skill.
                return false;
            }

            if ( !hero.HaveSpellBook() && skillType == Skill::Secondary::MYSTICISM ) {
                // It's useless to have Mysticism with no magic book in hands.
                return false;
            }

            return true;
        }

        case MP2::OBJ_TREEKNOWLEDGE:
            if ( !hero.isVisited( tile ) ) {
                const ResourceCount & rc = tile.QuantityResourceCount();
                if ( !rc.isValid() || kingdom.AllowPayment( Funds( rc ) ) )
                    return true;
            }
            break;

        // good luck
        case MP2::OBJ_FOUNTAIN:
        case MP2::OBJ_FAERIERING:
        case MP2::OBJ_IDOL:
            return !hero.isObjectTypeVisited( objectType ) && hero.GetLuck() < Luck::IRISH;

        // good morale
        case MP2::OBJ_OASIS:
        case MP2::OBJ_WATERINGHOLE:
            return !hero.isObjectTypeVisited( objectType ) && hero.GetMorale() < Morale::BLOOD;

        case MP2::OBJ_TEMPLE:
            return !hero.isObjectTypeVisited( objectType ) && hero.GetMorale() < Morale::BLOOD && !army.AllTroopsAreUndead();

        case MP2::OBJ_MAGICWELL:
            return !hero.isObjectTypeVisited( MP2::OBJ_MAGICWELL ) && hero.HaveSpellBook() && hero.GetSpellPoints() < hero.GetMaxSpellPoints();

        case MP2::OBJ_ARTESIANSPRING:
            return !hero.isVisited( tile, Visit::GLOBAL ) && hero.HaveSpellBook() && hero.GetSpellPoints() < 2 * hero.GetMaxSpellPoints();

        case MP2::OBJ_XANADU: {
            const uint32_t level1 = hero.GetLevelSkill( Skill::Secondary::DIPLOMACY );
            const uint32_t level2 = hero.GetLevel();

            if ( !hero.isVisited( tile )
                 && ( ( level1 == Skill::Level::BASIC && 7 < level2 ) || ( level1 == Skill::Level::ADVANCED && 5 < level2 )
                      || ( level1 == Skill::Level::EXPERT && 3 < level2 ) || ( 9 < level2 ) ) )
                return true;
            break;
        }

        // accept army
        case MP2::OBJ_WATCHTOWER:
        case MP2::OBJ_EXCAVATION:
        case MP2::OBJ_CAVE:
        case MP2::OBJ_TREEHOUSE:
        case MP2::OBJ_ARCHERHOUSE:
        case MP2::OBJ_GOBLINHUT:
        case MP2::OBJ_DWARFCOTT:
        case MP2::OBJ_HALFLINGHOLE:
        case MP2::OBJ_THATCHEDHUT:
        case MP2::OBJ_PEASANTHUT: {
            const Troop & troop = tile.QuantityTroop();
            if ( !troop.isValid() ) {
                return false;
            }

            const bool armyHasMonster = army.HasMonster( troop.GetMonster() );
            if ( !armyHasMonster && army.isFullHouse() && army.areAllTroopsUnique() ) {
                return false;
            }

            return isArmyValuableToObtain( troop, armyStrengthThreshold, armyHasMonster );
        }

        // recruit army
        case MP2::OBJ_RUINS:
        case MP2::OBJ_TREECITY:
        case MP2::OBJ_WAGONCAMP:
        case MP2::OBJ_DESERTTENT:
        case MP2::OBJ_WATERALTAR:
        case MP2::OBJ_AIRALTAR:
        case MP2::OBJ_FIREALTAR:
        case MP2::OBJ_EARTHALTAR:
        case MP2::OBJ_BARROWMOUNDS:
        case MP2::OBJ_ANCIENTLAMP: {
            const Troop & troop = tile.QuantityTroop();
            if ( !troop.isValid() ) {
                return false;
            }

            const bool armyHasMonster = army.HasMonster( troop.GetMonster() );
            if ( !armyHasMonster && army.isFullHouse() && army.areAllTroopsUnique() ) {
                return false;
            }

            const payment_t singleMonsterCost = troop.GetCost();

            uint32_t recruitTroopCount = kingdom.GetFunds().getLowestQuotient( singleMonsterCost );
            if ( recruitTroopCount <= 0 ) {
                // We do not have resources to hire even a single creature.
                return false;
            }

            const uint32_t availableTroopCount = troop.GetCount();
            if ( recruitTroopCount > availableTroopCount ) {
                recruitTroopCount = availableTroopCount;
            }

            const Troop troopToHire{ troop.GetID(), recruitTroopCount };

            return isArmyValuableToObtain( troopToHire, armyStrengthThreshold, armyHasMonster );
        }

        // recruit army (battle)
        case MP2::OBJ_DRAGONCITY:
        case MP2::OBJ_CITYDEAD:
        case MP2::OBJ_TROLLBRIDGE: {
            if ( Color::NONE == tile.QuantityColor() ) {
                return isHeroStrongerThan( tile, objectType, ai, heroArmyStrength, AI::ARMY_ADVANTAGE_MEDIUM );
            }

            const Troop & troop = tile.QuantityTroop();
            if ( !troop.isValid() ) {
                return false;
            }

            const bool armyHasMonster = army.HasMonster( troop.GetMonster() );
            if ( !armyHasMonster && army.isFullHouse() && army.areAllTroopsUnique() ) {
                return false;
            }

            const payment_t & paymentCosts = troop.GetTotalCost();
            // TODO: even if AI does not have enough money it might still buy few monsters.
            if ( !kingdom.AllowPayment( paymentCosts ) ) {
                return false;
            }

            return isArmyValuableToObtain( troop, armyStrengthThreshold, armyHasMonster );
        }

        // upgrade army
        case MP2::OBJ_HILLFORT:
            return army.HasMonster( Monster::DWARF ) || army.HasMonster( Monster::ORC ) || army.HasMonster( Monster::OGRE );

        // upgrade army
        case MP2::OBJ_FREEMANFOUNDRY:
            return army.HasMonster( Monster::PIKEMAN ) || army.HasMonster( Monster::SWORDSMAN ) || army.HasMonster( Monster::IRON_GOLEM );

        // loyalty obj
        case MP2::OBJ_STABLES: {
            if ( army.HasMonster( Monster::CAVALRY ) ) {
                return true;
            }

            const int daysActive = DAYOFWEEK - world.GetDay() + 1;
            const double movementBonus = daysActive * 400.0 - 2.0 * pathfinder.getDistance( index );

            return !hero.isObjectTypeVisited( MP2::OBJ_STABLES ) && movementBonus > 0;
        }

        case MP2::OBJ_ARENA:
            return !hero.isVisited( tile );

        // poor morale obj
        case MP2::OBJ_SHIPWRECK:
        case MP2::OBJ_GRAVEYARD:
        case MP2::OBJ_DERELICTSHIP:
            if ( !hero.isVisited( tile, Visit::GLOBAL ) && tile.QuantityIsValid() ) {
                Army enemy( tile );
                return enemy.isValid() && isHeroStrongerThan( tile, objectType, ai, heroArmyStrength, 2 );
            }
            break;

        case MP2::OBJ_PYRAMID:
            if ( !hero.isVisited( tile, Visit::GLOBAL ) && tile.QuantityIsValid() ) {
                Army enemy( tile );
                return enemy.isValid() && Skill::Level::EXPERT == hero.GetLevelSkill( Skill::Secondary::WISDOM )
                       && isHeroStrongerThan( tile, objectType, ai, heroArmyStrength, AI::ARMY_ADVANTAGE_LARGE );
            }
            break;

        case MP2::OBJ_DAEMONCAVE:
            if ( tile.QuantityIsValid() && 4 != tile.QuantityVariant() )
                return isHeroStrongerThan( tile, objectType, ai, heroArmyStrength, AI::ARMY_ADVANTAGE_MEDIUM );
            break;

        case MP2::OBJ_MONSTER:
            return isHeroStrongerThan( tile, objectType, ai, heroArmyStrength, ( hero.isLosingGame() ? 1.0 : AI::ARMY_ADVANTAGE_MEDIUM ) );

        case MP2::OBJ_SIGN:
            // AI has no brains to process anything from sign messages.
            return false;

        case MP2::OBJ_HEROES: {
            const Heroes * otherHero = tile.GetHeroes();
            assert( otherHero != nullptr );

            const bool otherHeroInCastle = ( otherHero->inCastle() != nullptr );

            if ( hero.GetColor() == otherHero->GetColor() && !hero.hasMetWithHero( otherHero->GetID() ) ) {
                return !otherHeroInCastle;
            }
            if ( hero.isFriends( otherHero->GetColor() ) ) {
                return false;
            }
            // WINS_HERO victory condition does not apply to AI-controlled players, we have to keep this hero alive for the human player
            if ( otherHero == world.GetHeroesCondWins() ) {
                return false;
            }
            if ( otherHeroInCastle ) {
                return AIShouldVisitCastle( hero, index, heroArmyStrength );
            }
            if ( army.isStrongerThan( otherHero->GetArmy(), hero.isLosingGame() ? AI::ARMY_ADVANTAGE_DESPERATE : AI::ARMY_ADVANTAGE_SMALL ) ) {
                return true;
            }

            break;
        }

        case MP2::OBJ_CASTLE:
            return AIShouldVisitCastle( hero, index, heroArmyStrength );

        case MP2::OBJ_BOAT:
            // AI should never consider a boat as a destination point. It uses them only to make a path.
            return false;

        case MP2::OBJ_STONELITHS:
            // AI should never consider a stone lith as a destination point. It uses them only to make a path.
            return false;

        case MP2::OBJ_JAIL:
            return kingdom.GetHeroes().size() < Kingdom::GetMaxHeroes();
        case MP2::OBJ_HUTMAGI:
            return !hero.isObjectTypeVisited( MP2::OBJ_HUTMAGI, Visit::GLOBAL ) && !Maps::GetObjectPositions( MP2::OBJ_EYEMAGI, true ).empty();
        case MP2::OBJ_TRADINGPOST:
        case MP2::OBJ_SPHINX:
            // TODO: AI doesn't know how it use it properly.
            return false;
        case MP2::OBJ_ORACLE:
        case MP2::OBJ_EYEMAGI:
            // No use of this object for AI.
            return false;
        case MP2::OBJ_ALCHEMYTOWER: {
            const BagArtifacts & bag = hero.GetBagArtifacts();
            const uint32_t cursed = static_cast<uint32_t>( std::count_if( bag.begin(), bag.end(), []( const Artifact & art ) { return art.containsCurses(); } ) );
            if ( cursed == 0 ) {
                return false;
            }

            const payment_t payment = PaymentConditions::ForAlchemist();
            return kingdom.AllowPayment( payment );
        }
        default:
            // Did you add a new action object but forget to add AI interaction for it?
            assert( 0 );
            break;
        }

        return false;
    }

    void addHeroToMove( Heroes * hero, std::vector<AI::HeroToMove> & availableHeroes )
    {
        if ( hero->Modes( Heroes::PATROL ) ) {
            if ( hero->GetSquarePatrol() == 0 ) {
                DEBUG_LOG( DBG_AI, DBG_TRACE, hero->GetName() << " standing still. Skip turn." )
                return;
            }
        }

        if ( hero->MayStillMove( false, false ) ) {
            availableHeroes.emplace_back();
            AI::HeroToMove & heroInfo = availableHeroes.back();
            heroInfo.hero = hero;

            if ( hero->Modes( Heroes::PATROL ) ) {
                heroInfo.patrolCenter = Maps::GetIndexFromAbsPoint( hero->GetCenterPatrol() );
                heroInfo.patrolDistance = hero->GetSquarePatrol();
            }
        }
    }

    // Used for caching object validations per hero.
    class ObjectValidator
    {
    public:
        explicit ObjectValidator( const Heroes & hero, const AIWorldPathfinder & pathfinder, AI::Normal & ai )
            : _hero( hero )
            , _pathfinder( pathfinder )
            , _ai( ai )
            , _heroArmyStrength( hero.GetArmy().GetStrength() )
            , _armyStrengthThreshold( hero.getAIMininumJoiningArmyStrength() )
        {
            // Do nothing.
        }

        bool isValid( const int index )
        {
            auto iter = _validObjects.find( index );
            if ( iter != _validObjects.end() ) {
                return iter->second;
            }

            const bool valid = HeroesValidObject( _hero, _heroArmyStrength, index, _pathfinder, _ai, _armyStrengthThreshold );
            _validObjects[index] = valid;
            return valid;
        }

    private:
        const Heroes & _hero;
        const AIWorldPathfinder & _pathfinder;
        AI::Normal & _ai;

        // Hero's strength value is valid till any action is done.
        // Since an instance of this class is used only for evaluation of the future movement it is appropriate to cache the strength.
        const double _heroArmyStrength;

        // Army strength threshold is used to decide whether getting extra monsters is useful.
        const double _armyStrengthThreshold;

        std::map<int, bool> _validObjects;
    };

    // Used for caching of object value estimation per hero.
    class ObjectValueStorage
    {
    public:
        ObjectValueStorage( const Heroes & hero, const AI::Normal & ai, const double ignoreValue )
            : _hero( hero )
            , _ai( ai )
            , _ignoreValue( ignoreValue )
        {}

        double value( const std::pair<int, int> & objectInfo, const uint32_t distance )
        {
            auto iter = _objectValue.find( objectInfo );
            if ( iter != _objectValue.end() ) {
                return iter->second;
            }

            const double value = _ai.getObjectValue( _hero, objectInfo.first, _ignoreValue, distance );

            _objectValue[objectInfo] = value;
            return value;
        }

    private:
        const Heroes & _hero;
        const AI::Normal & _ai;
        const double _ignoreValue;
        std::map<std::pair<int, int>, double> _objectValue;
    };

    double getMonsterUpgradeValue( const Army & army, const int monsterId )
    {
        const uint32_t monsterCount = army.GetCountMonsters( monsterId );
        if ( monsterCount == 0 ) {
            // Nothing to upgrade.
            return 0;
        }

        const Monster currentMonster( monsterId );

        const Monster upgradedMonster = currentMonster.GetUpgrade();
        if ( upgradedMonster == currentMonster ) {
            // Monster has no upgrade.
            return 0;
        }

        return ( upgradedMonster.GetMonsterStrength() - currentMonster.GetMonsterStrength() ) * monsterCount;
    }

    // Multiply by this value if you are getting a FREE upgrade.
    const double freeMonsterUpgradeModifier = 3;

    const double dangerousTaskPenalty = 20000.0;
    const double fogDiscoveryBaseValue = -20000.0;

    double ScaleWithDistance( double value, uint32_t distance )
    {
        if ( distance == 0 )
            return value;
        // scale non-linearly (more value lost as distance increases)
        return value - ( distance * std::log10( distance ) );
    }

    double getFogDiscoveryValue( const Heroes & hero )
    {
        switch ( hero.getAIRole() ) {
        case Heroes::Role::HUNTER:
            return fogDiscoveryBaseValue;
        case Heroes::Role::COURIER:
        case Heroes::Role::FIGHTER:
        case Heroes::Role::CHAMPION:
            return fogDiscoveryBaseValue * 2;
        default:
            // If you set a new type of a hero you must add the logic here.
            assert( 0 );
            break;
        }

        return fogDiscoveryBaseValue;
    }
}

namespace AI
{
    // TODO: we might need to remove duplication code present in the methods below. For now we can keep them as it is for a simpler modification of parameters.
    // In the future we need to come up with dynamic object value estimation based not only on a hero's role but on an outcome from movement at certain position.

    double Normal::getHunterObjectValue( const Heroes & hero, const int index, const double valueToIgnore, const uint32_t distanceToObject ) const
    {
        // Hunter has almost equal priorities to all kind of objects.
        assert( hero.getAIRole() == Heroes::Role::HUNTER );

        // In the future these hardcoded values could be configured by the mod
        // 1 tile distance is 100.0 value approximately
        const Maps::Tiles & tile = world.GetTiles( index );
        const MP2::MapObjectType objectType = tile.GetObject();

        if ( objectType == MP2::OBJ_CASTLE ) {
            const Castle * castle = world.getCastleEntrance( Maps::GetPoint( index ) );
            if ( !castle )
                return valueToIgnore;

            const bool critical = isCriticalTask( index );
            if ( hero.GetColor() == castle->GetColor() ) {
                double value = castle->getVisitValue( hero );
                if ( critical )
                    return 10000 + value;

                if ( value < 500 )
                    return valueToIgnore;

                return value;
            }

            // Hero should never visit castles belonging to friendly kingdoms
            if ( hero.isFriends( castle->GetColor() ) ) {
                assert( 0 );
                return -dangerousTaskPenalty;
            }

            // WINS_HERO victory condition does not apply to AI-controlled players, we have to ignore the castle with this hero
            // to keep him alive for the human player
            if ( isHeroWhoseDefeatIsVictoryConditionForHumanInCastle( castle ) ) {
                assert( 0 );
                return -dangerousTaskPenalty;
            }

            double value = castle->getBuildingValue() * 150.0 + 3000;
            if ( critical || hero.isLosingGame() )
                value += 15000;
            // If the castle is defenseless
            if ( !castle->GetActualArmy().isValid() )
                value *= 1.25;

            if ( isCastleLossConditionForHuman( castle ) )
                value += 20000;

            return value;
        }
        else if ( objectType == MP2::OBJ_HEROES ) {
            const Heroes * otherHero = tile.GetHeroes();
            assert( otherHero );
            if ( !otherHero ) {
                return valueToIgnore;
            }

            if ( hero.GetColor() == otherHero->GetColor() ) {
                if ( hero.getAIRole() > otherHero->getAIRole() ) {
                    // The other hero has a lower role. Do not waste time for meeting. Let him to come.
                    return valueToIgnore;
                }
                if ( hero.getAIRole() == otherHero->getAIRole() && hero.getStatsValue() + 2 > otherHero->getStatsValue() ) {
                    // Two heroes are almost identical. No reason to meet.
                    return valueToIgnore;
                }

                const double value = hero.getMeetingValue( *otherHero );
                // limit the max value of friendly hero meeting to 30 tiles
                return ( value < 250 ) ? valueToIgnore : std::min( value, 10000.0 );
            }

            // Hero should never meet heroes from friendly kingdoms
            if ( hero.isFriends( otherHero->GetColor() ) ) {
                assert( 0 );
                return -dangerousTaskPenalty;
            }

            // WINS_HERO victory condition does not apply to AI-controlled players, we have to keep this hero alive for the human player
            if ( otherHero == world.GetHeroesCondWins() ) {
                assert( 0 );
                return -dangerousTaskPenalty;
            }

            // focus on enemy hero if there's priority set (i.e. hero is threatning our castle)
            return isCriticalTask( index ) ? 12000.0 : 5000.0;
        }
        else if ( objectType == MP2::OBJ_MONSTER ) {
            return 1000.0;
        }
        else if ( objectType == MP2::OBJ_MINES || objectType == MP2::OBJ_SAWMILL || objectType == MP2::OBJ_ALCHEMYLAB ) {
            if ( tile.QuantityColor() == hero.GetColor() ) {
                return -dangerousTaskPenalty; // don't even attempt to go here
            }
            return ( tile.QuantityResourceCount().first == Resource::GOLD ) ? 4000.0 : 2000.0;
        }
        else if ( objectType == MP2::OBJ_ABANDONEDMINE ) {
            if ( tile.QuantityColor() == hero.GetColor() ) {
                return -dangerousTaskPenalty; // don't even attempt to go here
            }
            return 3000.0;
        }
        else if ( MP2::isArtifactObject( objectType ) && tile.QuantityArtifact().isValid() ) {
            const Artifact art = tile.QuantityArtifact();

            // WINS_ARTIFACT victory condition does not apply to AI-controlled players, we should leave this artifact untouched for the human player
            if ( isFindArtifactVictoryConditionForHuman( art ) ) {
                assert( 0 );
                return -dangerousTaskPenalty;
            }

            return 1000.0 * art.getArtifactValue();
        }
        else if ( MP2::isPickupObject( objectType ) ) {
            return 850.0;
        }
        else if ( MP2::isCaptureObject( objectType ) && MP2::isQuantityObject( objectType ) ) {
            // Objects like WATERWHEEL, WINDMILL and MAGICGARDEN if capture setting is enabled
            return 500.0;
        }
        else if ( objectType == MP2::OBJ_XANADU ) {
            return 3000.0;
        }
        else if ( objectType == MP2::OBJ_SHRINE1 || objectType == MP2::OBJ_SHRINE2 || objectType == MP2::OBJ_SHRINE3 ) {
            const Spell & spell = tile.QuantitySpell();
            return spell.getStrategicValue( hero.GetArmy().GetStrength(), hero.GetMaxSpellPoints(), hero.GetPower() );
        }
        else if ( MP2::isHeroUpgradeObject( objectType ) ) {
            return 500.0;
        }
        else if ( MP2::isMonsterDwelling( objectType ) ) {
            return tile.QuantityTroop().GetStrength();
        }
        else if ( objectType == MP2::OBJ_STONELITHS ) {
            // Stone lith is not considered by AI as an action object. If this assertion blows up something is wrong with the logic.
            assert( 0 );
            return -dangerousTaskPenalty;
        }
        else if ( objectType == MP2::OBJ_OBSERVATIONTOWER ) {
            const int fogCountToUncover
                = Maps::getFogTileCountToBeRevealed( index, GameStatic::getFogDiscoveryDistance( GameStatic::FogDiscoveryType::OBSERVATION_TOWER ), hero.GetColor() );
            if ( fogCountToUncover <= 0 ) {
                // Nothing to uncover.
                return -dangerousTaskPenalty;
            }
            return fogCountToUncover;
        }
        else if ( objectType == MP2::OBJ_MAGELLANMAPS ) {
            // Very valuable object.
            return 5000;
        }
        else if ( objectType == MP2::OBJ_COAST ) {
            // Coast is not an object. If this assertion blows up something is wrong with the logic.
            assert( 0 );
            return -dangerousTaskPenalty;
        }
        else if ( objectType == MP2::OBJ_WHIRLPOOL ) {
            // Whirlpool is not considered by AI as an action object. If this assertion blows up something is wrong with the logic.
            assert( 0 );
            return -dangerousTaskPenalty;
        }
        else if ( objectType == MP2::OBJ_BOAT ) {
            // Boat is not considered by AI as an action object. If this assertion blows up something is wrong with the logic.
            assert( 0 );
            return -dangerousTaskPenalty;
        }
        else if ( objectType == MP2::OBJ_MAGICWELL || objectType == MP2::OBJ_ARTESIANSPRING ) {
            if ( !hero.HaveSpellBook() ) {
                return -dangerousTaskPenalty;
            }
            if ( hero.GetSpellPoints() * 2 >= hero.GetMaxSpellPoints() ) {
                return -2000; // no reason to visit the well with no magic book or with half of points
            }
            return hero.isPotentSpellcaster() ? 1500 : 0;
        }
        else if ( objectType == MP2::OBJ_TEMPLE ) {
            if ( hero.GetArmy().AllTroopsAreUndead() ) {
                // All troops are undead, no use of Morale.
                return 0;
            }

            const int moral = hero.GetMorale();
            if ( moral >= 3 ) {
                return -dangerousTaskPenalty; // no reason to visit with a maximum moral
            }
            if ( moral == 2 ) {
                return -4000; // moral is good enough to avoid visting this object
            }
            if ( moral == 1 ) {
                return -2000; // is it worth to visit this object with little better than neutral moral?
            }
            if ( moral == 0 ) {
                return 0;
            }

            return 100;
        }
        else if ( objectType == MP2::OBJ_STABLES ) {
            const int daysActive = DAYOFWEEK - world.GetDay() + 1;
            double movementBonus = daysActive * 400.0 - 2.0 * distanceToObject;
            if ( movementBonus < 0 ) {
                // Looks like this is too far away.
                movementBonus = 0;
            }

            const double upgradeValue = getMonsterUpgradeValue( hero.GetArmy(), Monster::CHAMPION );
            return movementBonus + freeMonsterUpgradeModifier * upgradeValue;
        }
        else if ( objectType == MP2::OBJ_FREEMANFOUNDRY ) {
            const double upgradePikemanValue = getMonsterUpgradeValue( hero.GetArmy(), Monster::PIKEMAN );
            const double upgradeSwordsmanValue = getMonsterUpgradeValue( hero.GetArmy(), Monster::SWORDSMAN );
            const double upgradeGolemValue = getMonsterUpgradeValue( hero.GetArmy(), Monster::IRON_GOLEM );

            return freeMonsterUpgradeModifier * ( upgradePikemanValue + upgradeSwordsmanValue + upgradeGolemValue );
        }
        else if ( objectType == MP2::OBJ_HILLFORT ) {
            const double upgradeDwarfValue = getMonsterUpgradeValue( hero.GetArmy(), Monster::DWARF );
            const double upgradeOrcValue = getMonsterUpgradeValue( hero.GetArmy(), Monster::ORC );
            const double upgradeOgreValue = getMonsterUpgradeValue( hero.GetArmy(), Monster::OGRE );

            return freeMonsterUpgradeModifier * ( upgradeDwarfValue + upgradeOrcValue + upgradeOgreValue );
        }
        else if ( objectType == MP2::OBJ_TRAVELLERTENT ) {
            // Most likely it'll lead to opening more land.
            return 1000;
        }
        else if ( objectType == MP2::OBJ_OASIS ) {
            return std::max( 800.0 - 2.0 * distanceToObject, 0.0 );
        }
        else if ( objectType == MP2::OBJ_WATERINGHOLE ) {
            return std::max( 400.0 - 2.0 * distanceToObject, 0.0 );
        }
        else if ( objectType == MP2::OBJ_JAIL ) {
            // A free hero is always good and it could be very powerful.
            return 3000;
        }
        else if ( objectType == MP2::OBJ_HUTMAGI ) {
            const MapsIndexes eyeMagiIndexes = Maps::GetObjectPositions( MP2::OBJ_EYEMAGI, true );
            int fogCountToUncover = 0;
            const int heroColor = hero.GetColor();
            const int eyeViewDistance = GameStatic::getFogDiscoveryDistance( GameStatic::FogDiscoveryType::MAGI_EYES );

            for ( const int32_t eyeIndex : eyeMagiIndexes ) {
                fogCountToUncover += Maps::getFogTileCountToBeRevealed( eyeIndex, eyeViewDistance, heroColor );
            }

            return fogCountToUncover;
        }
        else if ( objectType == MP2::OBJ_GAZEBO ) {
            // Free 1000 experience. We need to calculate value of this object based on hero's experience. The higher hero's level the less valueable this object is.
            const uint32_t heroExperience = hero.GetExperience();
            const uint32_t nextLevelExperience = Heroes::GetExperienceFromLevel( Heroes::GetLevelFromExperience( heroExperience ) );
            const uint32_t neededExperience = nextLevelExperience - heroExperience;
            if ( neededExperience < 1000 ) {
                // A new level. Have to visit.
                return 1000;
            }

            return 1000.0 * 1000.0 / neededExperience;
        }
        else if ( objectType == MP2::OBJ_LIGHTHOUSE ) {
            if ( tile.QuantityColor() == hero.GetColor() ) {
                return -dangerousTaskPenalty; // don't even attempt to go here
            }
            return 500;
        }
        else if ( objectType == MP2::OBJ_PYRAMID ) {
            return 1500;
        }

        // TODO: add support for all possible objects.

        return 0;
    }

    double Normal::getFighterObjectValue( const Heroes & hero, const int index, const double valueToIgnore, const uint32_t distanceToObject ) const
    {
        // Fighters have higher priority for battles and smaller values for other objects.
        assert( hero.getAIRole() == Heroes::Role::FIGHTER || hero.getAIRole() == Heroes::Role::CHAMPION );

        // In the future these hardcoded values could be configured by the mod
        // 1 tile distance is 100.0 value approximately
        const Maps::Tiles & tile = world.GetTiles( index );
        const MP2::MapObjectType objectType = tile.GetObject();
        const bool anotherFriendlyHeroPresent = _regions[tile.GetRegion()].friendlyHeroes > 1;

        if ( objectType == MP2::OBJ_CASTLE ) {
            const Castle * castle = world.getCastleEntrance( Maps::GetPoint( index ) );
            if ( !castle )
                return valueToIgnore;

            const bool critical = isCriticalTask( index );
            if ( hero.GetColor() == castle->GetColor() ) {
                double value = castle->getVisitValue( hero );
                if ( critical )
                    return 15000 + value;

                if ( value < 500 )
                    return valueToIgnore;

                return value / 2;
            }

            // Hero should never visit castles belonging to friendly kingdoms
            if ( hero.isFriends( castle->GetColor() ) ) {
                assert( 0 );
                return -dangerousTaskPenalty;
            }

            // WINS_HERO victory condition does not apply to AI-controlled players, we have to ignore the castle with this hero
            // to keep him alive for the human player
            if ( isHeroWhoseDefeatIsVictoryConditionForHumanInCastle( castle ) ) {
                assert( 0 );
                return -dangerousTaskPenalty;
            }

            double value = castle->getBuildingValue() * 500.0 + 15000;
            if ( critical || hero.isLosingGame() )
                value += 15000;
            // If the castle is defenseless
            // This modifier shouldn't be too high to avoid players baiting AI in
            if ( !castle->GetActualArmy().isValid() )
                value *= 1.5;

            if ( isCastleLossConditionForHuman( castle ) )
                value += 20000;

            return value;
        }
        else if ( objectType == MP2::OBJ_HEROES ) {
            const Heroes * otherHero = tile.GetHeroes();
            assert( otherHero );
            if ( !otherHero ) {
                return valueToIgnore;
            }

            if ( hero.GetColor() == otherHero->GetColor() ) {
                if ( hero.getAIRole() > otherHero->getAIRole() ) {
                    // The other hero has a lower role. Do not waste time for meeting. Let him to come.
                    return valueToIgnore;
                }
                if ( hero.getAIRole() == otherHero->getAIRole() && hero.getStatsValue() + 3 > otherHero->getStatsValue() ) {
                    // Two heroes are almost identical. No reason to meet.
                    return valueToIgnore;
                }

                const double value = hero.getMeetingValue( *otherHero );
                // limit the max value of friendly hero meeting to 30 tiles
                return ( value < 250 ) ? valueToIgnore : std::min( value, 5000.0 );
            }

            // Hero should never meet heroes from friendly kingdoms
            if ( hero.isFriends( otherHero->GetColor() ) ) {
                assert( 0 );
                return -dangerousTaskPenalty;
            }

            // WINS_HERO victory condition does not apply to AI-controlled players, we have to keep this hero alive for the human player
            if ( otherHero == world.GetHeroesCondWins() ) {
                assert( 0 );
                return -dangerousTaskPenalty;
            }

            return isCriticalTask( index ) ? 20000.0 : 12000.0;
        }
        else if ( objectType == MP2::OBJ_MONSTER ) {
            return anotherFriendlyHeroPresent ? 4000.0 : 1000.0;
        }
        else if ( objectType == MP2::OBJ_MINES || objectType == MP2::OBJ_SAWMILL || objectType == MP2::OBJ_ALCHEMYLAB ) {
            if ( tile.QuantityColor() == hero.GetColor() ) {
                return -dangerousTaskPenalty; // don't even attempt to go here
            }
            return ( tile.QuantityResourceCount().first == Resource::GOLD ) ? 3000.0 : 1500.0;
        }
        else if ( objectType == MP2::OBJ_ABANDONEDMINE ) {
            if ( tile.QuantityColor() == hero.GetColor() ) {
                return -dangerousTaskPenalty; // don't even attempt to go here
            }
            return 5000.0;
        }
        else if ( MP2::isArtifactObject( objectType ) && tile.QuantityArtifact().isValid() ) {
            const Artifact art = tile.QuantityArtifact();

            // WINS_ARTIFACT victory condition does not apply to AI-controlled players, we should leave this artifact untouched for the human player
            if ( isFindArtifactVictoryConditionForHuman( art ) ) {
                assert( 0 );
                return -dangerousTaskPenalty;
            }

            return 1500.0 * art.getArtifactValue();
        }
        else if ( objectType == MP2::OBJ_TREASURECHEST ) {
            // Treasure chest without the artifact
            return std::max( tile.QuantityGold(), 1000U );
        }
        else if ( MP2::isPickupObject( objectType ) ) {
            return anotherFriendlyHeroPresent ? 100.0 : 500.0;
        }
        else if ( MP2::isCaptureObject( objectType ) && MP2::isQuantityObject( objectType ) ) {
            // Objects like WATERWHEEL, WINDMILL and MAGICGARDEN if capture setting is enabled
            return 100.0;
        }
        else if ( objectType == MP2::OBJ_XANADU ) {
            return 3000.0;
        }
        else if ( objectType == MP2::OBJ_SHRINE1 || objectType == MP2::OBJ_SHRINE2 || objectType == MP2::OBJ_SHRINE3 ) {
            const Spell & spell = tile.QuantitySpell();
            return spell.getStrategicValue( hero.GetArmy().GetStrength(), hero.GetMaxSpellPoints(), hero.GetPower() );
        }
        else if ( MP2::isHeroUpgradeObject( objectType ) ) {
            return 1250.0;
        }
        else if ( MP2::isMonsterDwelling( objectType ) ) {
            return tile.QuantityTroop().GetStrength();
        }
        else if ( objectType == MP2::OBJ_STONELITHS ) {
            // Stone lith is not considered by AI as an action object. If this assertion blows up something is wrong with the logic.
            assert( 0 );
            return -dangerousTaskPenalty;
        }
        else if ( objectType == MP2::OBJ_OBSERVATIONTOWER ) {
            const int fogCountToUncover
                = Maps::getFogTileCountToBeRevealed( index, GameStatic::getFogDiscoveryDistance( GameStatic::FogDiscoveryType::OBSERVATION_TOWER ), hero.GetColor() );
            if ( fogCountToUncover <= 0 ) {
                // Nothing to uncover.
                return -dangerousTaskPenalty;
            }
            return fogCountToUncover / 2;
        }
        else if ( objectType == MP2::OBJ_MAGELLANMAPS ) {
            // Very valuable object.
            return 5000;
        }
        else if ( objectType == MP2::OBJ_COAST ) {
            // Coast is not an object. If this assertion blows up something is wrong the the logic.
            assert( 0 );
            return -dangerousTaskPenalty;
        }
        else if ( objectType == MP2::OBJ_WHIRLPOOL ) {
            // Whirlpool is not considered by AI as an action object. If this assertion blows up something is wrong with the logic.
            assert( 0 );
            return -dangerousTaskPenalty;
        }
        else if ( objectType == MP2::OBJ_BOAT ) {
            // Boat is not considered by AI as an action object. If this assertion blows up something is wrong the the logic.
            assert( 0 );
            return -dangerousTaskPenalty;
        }
        else if ( objectType == MP2::OBJ_MAGICWELL || objectType == MP2::OBJ_ARTESIANSPRING ) {
            if ( !hero.HaveSpellBook() ) {
                return -dangerousTaskPenalty;
            }
            if ( hero.GetSpellPoints() * 2 >= hero.GetMaxSpellPoints() ) {
                return -2000; // no reason to visit the well with no magic book or with half of points
            }
            return hero.isPotentSpellcaster() ? 2500 : 0;
        }
        else if ( objectType == MP2::OBJ_TEMPLE ) {
            if ( hero.GetArmy().AllTroopsAreUndead() ) {
                // All troops are undead, no use of Morale.
                return 0;
            }

            const int moral = hero.GetMorale();
            if ( moral >= 3 ) {
                return -dangerousTaskPenalty; // no reason to visit with a maximum moral
            }
            if ( moral == 2 ) {
                return -4000; // moral is good enough to avoid visting this object
            }
            if ( moral == 1 ) {
                return -2000; // is it worth to visit this object with little better than neutral moral?
            }
            if ( moral == 0 ) {
                return 0;
            }

            return 200;
        }
        else if ( objectType == MP2::OBJ_STABLES ) {
            const int daysActive = DAYOFWEEK - world.GetDay() + 1;
            double movementBonus = daysActive * 400.0 - 2.0 * distanceToObject;
            if ( movementBonus < 0 ) {
                // Looks like this is too far away.
                movementBonus = 0;
            }

            const double upgradeValue = getMonsterUpgradeValue( hero.GetArmy(), Monster::CHAMPION );
            return movementBonus + freeMonsterUpgradeModifier * upgradeValue;
        }
        else if ( objectType == MP2::OBJ_FREEMANFOUNDRY ) {
            const double upgradePikemanValue = getMonsterUpgradeValue( hero.GetArmy(), Monster::PIKEMAN );
            const double upgradeSwordsmanValue = getMonsterUpgradeValue( hero.GetArmy(), Monster::SWORDSMAN );
            const double upgradeGolemValue = getMonsterUpgradeValue( hero.GetArmy(), Monster::IRON_GOLEM );

            return freeMonsterUpgradeModifier * ( upgradePikemanValue + upgradeSwordsmanValue + upgradeGolemValue );
        }
        else if ( objectType == MP2::OBJ_HILLFORT ) {
            const double upgradeDwarfValue = getMonsterUpgradeValue( hero.GetArmy(), Monster::DWARF );
            const double upgradeOrcValue = getMonsterUpgradeValue( hero.GetArmy(), Monster::ORC );
            const double upgradeOgreValue = getMonsterUpgradeValue( hero.GetArmy(), Monster::OGRE );

            return freeMonsterUpgradeModifier * ( upgradeDwarfValue + upgradeOrcValue + upgradeOgreValue );
        }
        else if ( objectType == MP2::OBJ_TRAVELLERTENT ) {
            // Most likely it'll lead to opening more land.
            return 1000;
        }
        else if ( objectType == MP2::OBJ_OASIS ) {
            return std::max( 800.0 - 2.0 * distanceToObject, 0.0 );
        }
        else if ( objectType == MP2::OBJ_WATERINGHOLE ) {
            return std::max( 400.0 - 2.0 * distanceToObject, 0.0 );
        }
        else if ( objectType == MP2::OBJ_JAIL ) {
            // A free hero is always good and it could be very powerful.
            return 3000;
        }
        else if ( objectType == MP2::OBJ_HUTMAGI ) {
            const MapsIndexes eyeMagiIndexes = Maps::GetObjectPositions( MP2::OBJ_EYEMAGI, true );
            int fogCountToUncover = 0;
            const int heroColor = hero.GetColor();
            const int eyeViewDistance = GameStatic::getFogDiscoveryDistance( GameStatic::FogDiscoveryType::MAGI_EYES );

            for ( const int32_t eyeIndex : eyeMagiIndexes ) {
                fogCountToUncover += Maps::getFogTileCountToBeRevealed( eyeIndex, eyeViewDistance, heroColor );
            }

            return fogCountToUncover / 2;
        }
        else if ( objectType == MP2::OBJ_GAZEBO ) {
            // Free 1000 experience. We need to calculate value of this object based on hero's experience. The higher hero's level the less valueable this object is.
            const uint32_t heroExperience = hero.GetExperience();
            const uint32_t nextLevelExperience = Heroes::GetExperienceFromLevel( Heroes::GetLevelFromExperience( heroExperience ) );
            const uint32_t neededExperience = nextLevelExperience - heroExperience;
            if ( neededExperience < 1000 ) {
                // A new level. Have to visit.
                return 1000;
            }

            return 1000.0 * 1000.0 / neededExperience;
        }
        else if ( objectType == MP2::OBJ_LIGHTHOUSE ) {
            if ( tile.QuantityColor() == hero.GetColor() ) {
                return -dangerousTaskPenalty; // don't even attempt to go here
            }
            return 250;
        }
        else if ( objectType == MP2::OBJ_PYRAMID ) {
            return 10000;
        }

        // TODO: add support for all possible objects.

        return 0;
    }

    double Normal::getCourierObjectValue( const Heroes & hero, const int index, const double valueToIgnore, const uint32_t distanceToObject ) const
    {
        // Courier should focus on its main task and visit other objects only if it's close to the destination
        assert( hero.getAIRole() == Heroes::Role::COURIER );

        // Values (n logn) for pre-defined distances where AI courier would consider taking a detour to visit those
        const double twoTiles = 500;
        const double fiveTiles = 1400;
        const double tenTiles = 3000;

        const Maps::Tiles & tile = world.GetTiles( index );
        const MP2::MapObjectType objectType = tile.GetObject();

        if ( objectType == MP2::OBJ_CASTLE ) {
            const Castle * castle = world.getCastleEntrance( Maps::GetPoint( index ) );
            if ( !castle )
                return valueToIgnore;

            const bool critical = isCriticalTask( index );
            if ( hero.GetColor() == castle->GetColor() ) {
                double value = castle->getVisitValue( hero );
                if ( critical )
                    return 10000 + value;

                if ( value < 500 )
                    return valueToIgnore;

                return value;
            }

            // Hero should never visit castles belonging to friendly kingdoms
            if ( hero.isFriends( castle->GetColor() ) ) {
                assert( 0 );
                return -dangerousTaskPenalty;
            }

            // WINS_HERO victory condition does not apply to AI-controlled players, we have to ignore the castle with this hero
            // to keep him alive for the human player
            if ( isHeroWhoseDefeatIsVictoryConditionForHumanInCastle( castle ) ) {
                assert( 0 );
                return -dangerousTaskPenalty;
            }

            double value = castle->getBuildingValue() * 150.0 + 3000;
            if ( critical || hero.isLosingGame() )
                value += 15000;
            // If the castle is defenseless
            if ( !castle->GetActualArmy().isValid() )
                value *= 1.25;

            if ( isCastleLossConditionForHuman( castle ) )
                value += 20000;

            return value;
        }
        else if ( objectType == MP2::OBJ_HEROES ) {
            const Heroes * otherHero = tile.GetHeroes();
            assert( otherHero );
            if ( !otherHero ) {
                return valueToIgnore;
            }

            if ( hero.GetColor() == otherHero->GetColor() ) {
                // Will be handled by the main task
                return valueToIgnore;
            }

            // Hero should never meet heroes from friendly kingdoms
            if ( hero.isFriends( otherHero->GetColor() ) ) {
                assert( 0 );
                return -dangerousTaskPenalty;
            }

            // WINS_HERO victory condition does not apply to AI-controlled players, we have to keep this hero alive for the human player
            if ( otherHero == world.GetHeroesCondWins() ) {
                assert( 0 );
                return -dangerousTaskPenalty;
            }

            // focus on enemy hero if there's priority set (i.e. hero is threatning our castle)
            return isCriticalTask( index ) ? 10000.0 : tenTiles;
        }
        else if ( objectType == MP2::OBJ_MONSTER ) {
            return twoTiles;
        }
        else if ( objectType == MP2::OBJ_MINES || objectType == MP2::OBJ_SAWMILL || objectType == MP2::OBJ_ALCHEMYLAB ) {
            if ( tile.QuantityColor() == hero.GetColor() ) {
                return -dangerousTaskPenalty; // don't even attempt to go here
            }
            return ( tile.QuantityResourceCount().first == Resource::GOLD ) ? tenTiles : fiveTiles;
        }
        else if ( MP2::isArtifactObject( objectType ) && tile.QuantityArtifact().isValid() ) {
            const Artifact art = tile.QuantityArtifact();

            // WINS_ARTIFACT victory condition does not apply to AI-controlled players, we should leave this artifact untouched for the human player
            if ( isFindArtifactVictoryConditionForHuman( art ) ) {
                assert( 0 );
                return -dangerousTaskPenalty;
            }

            return 1000.0 * art.getArtifactValue();
        }
        else if ( MP2::isPickupObject( objectType ) ) {
            return twoTiles;
        }
        else if ( MP2::isCaptureObject( objectType ) && MP2::isQuantityObject( objectType ) ) {
            // Objects like WATERWHEEL, WINDMILL and MAGICGARDEN if capture setting is enabled
            return twoTiles;
        }
        else if ( objectType == MP2::OBJ_XANADU ) {
            return fiveTiles;
        }
        else if ( objectType == MP2::OBJ_SHRINE1 || objectType == MP2::OBJ_SHRINE2 || objectType == MP2::OBJ_SHRINE3 ) {
            const Spell & spell = tile.QuantitySpell();
            return spell.getStrategicValue( hero.GetArmy().GetStrength(), hero.GetMaxSpellPoints(), hero.GetPower() );
        }
        else if ( MP2::isHeroUpgradeObject( objectType ) ) {
            return fiveTiles;
        }
        else if ( MP2::isMonsterDwelling( objectType ) ) {
            return tile.QuantityTroop().GetStrength();
        }
        else if ( objectType == MP2::OBJ_OBSERVATIONTOWER ) {
            const int fogCountToUncover
                = Maps::getFogTileCountToBeRevealed( index, GameStatic::getFogDiscoveryDistance( GameStatic::FogDiscoveryType::OBSERVATION_TOWER ), hero.GetColor() );
            if ( fogCountToUncover <= 0 ) {
                // Nothing to uncover.
                return -dangerousTaskPenalty;
            }
            return fogCountToUncover;
        }
        else if ( objectType == MP2::OBJ_MAGICWELL || objectType == MP2::OBJ_ARTESIANSPRING ) {
            if ( !hero.HaveSpellBook() || hero.GetSpellPoints() * 2 >= hero.GetMaxSpellPoints() ) {
                return -dangerousTaskPenalty; // no reason to visit the well with no magic book or with half of points
            }
            return hero.isPotentSpellcaster() ? fiveTiles : 0;
        }
        else if ( objectType == MP2::OBJ_STABLES ) {
            const int daysActive = DAYOFWEEK - world.GetDay() + 1;
            double movementBonus = daysActive * 400.0 - 2.0 * distanceToObject;
            if ( movementBonus < 0 ) {
                // Looks like this is too far away.
                movementBonus = 0;
            }

            const double upgradeValue = getMonsterUpgradeValue( hero.GetArmy(), Monster::CHAMPION );
            return movementBonus + freeMonsterUpgradeModifier * upgradeValue;
        }
        else if ( objectType == MP2::OBJ_FREEMANFOUNDRY ) {
            const double upgradePikemanValue = getMonsterUpgradeValue( hero.GetArmy(), Monster::PIKEMAN );
            const double upgradeSwordsmanValue = getMonsterUpgradeValue( hero.GetArmy(), Monster::SWORDSMAN );
            const double upgradeGolemValue = getMonsterUpgradeValue( hero.GetArmy(), Monster::IRON_GOLEM );

            return freeMonsterUpgradeModifier * ( upgradePikemanValue + upgradeSwordsmanValue + upgradeGolemValue );
        }
        else if ( objectType == MP2::OBJ_HILLFORT ) {
            const double upgradeDwarfValue = getMonsterUpgradeValue( hero.GetArmy(), Monster::DWARF );
            const double upgradeOrcValue = getMonsterUpgradeValue( hero.GetArmy(), Monster::ORC );
            const double upgradeOgreValue = getMonsterUpgradeValue( hero.GetArmy(), Monster::OGRE );

            return freeMonsterUpgradeModifier * ( upgradeDwarfValue + upgradeOrcValue + upgradeOgreValue );
        }
        else if ( objectType == MP2::OBJ_TRAVELLERTENT ) {
            // Most likely it'll lead to opening more land.
            return 1000;
        }
        else if ( objectType == MP2::OBJ_OASIS ) {
            return std::max( 800.0 - 2.0 * distanceToObject, 0.0 );
        }
        else if ( objectType == MP2::OBJ_WATERINGHOLE ) {
            return std::max( 400.0 - 2.0 * distanceToObject, 0.0 );
        }
        else if ( objectType == MP2::OBJ_JAIL ) {
            // A free hero is always good and it could be very powerful.
            return fiveTiles;
        }
        else if ( objectType == MP2::OBJ_GAZEBO ) {
            // Free 1000 experience. We need to calculate value of this object based on hero's experience. The higher hero's level the less valueable this object is.
            const uint32_t heroExperience = hero.GetExperience();
            const uint32_t nextLevelExperience = Heroes::GetExperienceFromLevel( Heroes::GetLevelFromExperience( heroExperience ) );
            const uint32_t neededExperience = nextLevelExperience - heroExperience;
            if ( neededExperience < 1000 ) {
                // A new level. Have to visit.
                return 1000;
            }

            return 1000.0 * 1000.0 / neededExperience;
        }

        return valueToIgnore;
    }

    double Normal::getObjectValue( const Heroes & hero, const int index, const double valueToIgnore, const uint32_t distanceToObject ) const
    {
        switch ( hero.getAIRole() ) {
        case Heroes::Role::HUNTER:
            return getHunterObjectValue( hero, index, valueToIgnore, distanceToObject );
        case Heroes::Role::CHAMPION:
        case Heroes::Role::FIGHTER:
            return getFighterObjectValue( hero, index, valueToIgnore, distanceToObject );
        case Heroes::Role::COURIER:
            return getCourierObjectValue( hero, index, valueToIgnore, distanceToObject );
        default:
            // If you set a new type of a hero you must add the logic here.
            assert( 0 );
            break;
        }

        return 0;
    }

    int Normal::getCourierMainTarget( const Heroes & hero, double lowestPossibleValue ) const
    {
        assert( hero.getAIRole() == Heroes::Role::COURIER );
        int targetIndex = -1;

        const Kingdom & kingdom = hero.GetKingdom();
        const KingdomHeroes & allHeroes = kingdom.GetHeroes();

        // Check if we have army and should bring it to friendly hero first
        double bestTargetValue = lowestPossibleValue;

        for ( const Heroes * otherHero : allHeroes ) {
            if ( !otherHero || hero.GetID() == otherHero->GetID() )
                continue;

            Heroes::Role role = otherHero->getAIRole();
            if ( role == Heroes::Role::COURIER || role == Heroes::Role::SCOUT )
                continue;

            const int currentHeroIndex = otherHero->GetIndex();
            const uint32_t dist = _pathfinder.getDistance( currentHeroIndex );
            if ( dist == 0 || hero.hasMetWithHero( otherHero->GetID() ) )
                continue;

            double value = hero.getMeetingValue( *otherHero );
            if ( value < 500 )
                continue;

            if ( role == Heroes::Role::CHAMPION ) {
                value *= 2.5;
            }
            value -= dist;

            if ( value > bestTargetValue ) {
                bestTargetValue = value;
                targetIndex = currentHeroIndex;
            }
        }

        if ( targetIndex != -1 )
            return targetIndex;

        // Reset the max value
        bestTargetValue = lowestPossibleValue;

        for ( const Castle * castle : kingdom.GetCastles() ) {
            if ( !castle || castle->GetHero() != nullptr )
                continue;

            const int currentCastleIndex = castle->GetIndex();
            const uint32_t dist = _pathfinder.getDistance( currentCastleIndex );

            if ( dist == 0 )
                continue;

            double value = castle->getVisitValue( hero );
            if ( value < 250 )
                continue;

            const int safetyFactor = _regions[world.GetTiles( currentCastleIndex ).GetRegion()].safetyFactor;
            if ( safetyFactor > 100 ) {
                value *= 2;
            }
            else if ( safetyFactor < 0 ) {
                value /= 2;
            }

            // additional distance scaling is not required since Couriers are meant to travel far
            value -= dist;

            if ( value > bestTargetValue ) {
                bestTargetValue = value;
                targetIndex = currentCastleIndex;
            }
        }
        return targetIndex;
    }

    int AI::Normal::getPriorityTarget( const HeroToMove & heroInfo, double & maxPriority )
    {
        Heroes & hero = *heroInfo.hero;
        const double lowestPossibleValue = -1.0 * Maps::Ground::slowestMovePenalty * world.getSize();
        const bool heroInPatrolMode = heroInfo.patrolCenter != -1;
        const double heroStrength = hero.GetArmy().GetStrength();

        int priorityTarget = -1;
        maxPriority = lowestPossibleValue;
#ifdef WITH_DEBUG
        MP2::MapObjectType objectType = MP2::OBJ_ZERO;
#endif

        // pre-cache the pathfinder
        _pathfinder.reEvaluateIfNeeded( hero );

        const uint32_t leftMovePoints = hero.GetMovePoints();

        ObjectValidator objectValidator( hero, _pathfinder, *this );
        ObjectValueStorage valueStorage( hero, *this, lowestPossibleValue );

        auto getObjectValue = [&objectValidator, &valueStorage, this, heroStrength, &hero, leftMovePoints]( const int destination, uint32_t & distance, double & value,
                                                                                                            const bool isDimensionDoor ) {
            if ( !isDimensionDoor ) {
                // Dimension door path does not include any objects on the way.
                const std::vector<IndexObject> & list = _pathfinder.getObjectsOnTheWay( destination );
                for ( const IndexObject & pair : list ) {
                    if ( objectValidator.isValid( pair.first ) && std::binary_search( _mapObjects.begin(), _mapObjects.end(), pair ) ) {
                        const double extraValue = valueStorage.value( pair, 0 ); // object is on the way, we don't loose any movement points.
                        if ( extraValue > 0 ) {
                            // There is no need to reduce the quality of the object even if the path has others.
                            value += extraValue;
                        }
                    }
                }
            }

            const RegionStats & regionStats = _regions[world.GetTiles( destination ).GetRegion()];

            if ( heroStrength < regionStats.highestThreat ) {
                const Castle * castle = world.getCastleEntrance( Maps::GetPoint( destination ) );

                if ( castle && ( castle->GetGarrisonStrength( &hero ) <= 0 || castle->GetColor() == hero.GetColor() ) )
                    value -= dangerousTaskPenalty / 2;
                else
                    value -= dangerousTaskPenalty;
            }

            if ( distance > leftMovePoints ) {
                // Distant object which is out of reach for the current turn must have lower priority.
                distance = leftMovePoints + ( distance - leftMovePoints ) * 2;
            }

            value = ScaleWithDistance( value, distance );
        };

        // Set baseline target if it's a special role
        if ( hero.getAIRole() == Heroes::Role::COURIER ) {
            const int courierTarget = getCourierMainTarget( hero, lowestPossibleValue );
            if ( courierTarget != -1 ) {
                // Anything with positive value can override the courier's main task (i.e. castle or mine capture on the way)
                maxPriority = 0;
                priorityTarget = courierTarget;
#ifdef WITH_DEBUG
                objectType = world.GetTiles( courierTarget ).GetObject();
#endif

                DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() << " courier main task is " << courierTarget )
            }
            else {
                // If there's nothing to do as a Courier reset the role
                hero.setAIRole( Heroes::Role::HUNTER );
            }
        }

        for ( const IndexObject & node : _mapObjects ) {
            // Skip if hero in patrol mode and object outside of reach
            if ( heroInPatrolMode && Maps::GetApproximateDistance( node.first, heroInfo.patrolCenter ) > heroInfo.patrolDistance )
                continue;

            if ( objectValidator.isValid( node.first ) ) {
                uint32_t dist = _pathfinder.getDistance( node.first );

                bool useDimensionDoor = false;
                const uint32_t dimensionDoorDist = AIWorldPathfinder::calculatePathPenalty( _pathfinder.getDimensionDoorPath( hero, node.first ) );
                if ( dimensionDoorDist > 0 && ( dist == 0 || dimensionDoorDist < dist / 2 ) ) {
                    dist = dimensionDoorDist;
                    useDimensionDoor = true;
                }

                if ( dist == 0 ) {
                    continue;
                }

                double value = valueStorage.value( node, dist );
                getObjectValue( node.first, dist, value, useDimensionDoor );

                if ( dist && value > maxPriority ) {
                    maxPriority = value;
                    priorityTarget = node.first;
#ifdef WITH_DEBUG
                    objectType = static_cast<MP2::MapObjectType>( node.second );
#endif

                    DEBUG_LOG( DBG_AI, DBG_TRACE,
                               hero.GetName() << ": valid object at " << node.first << " value is " << value << " ("
                                              << MP2::StringObject( static_cast<MP2::MapObjectType>( node.second ) ) << ")" )
                }
            }
        }

        double fogDiscoveryValue = getFogDiscoveryValue( hero );
        const int fogDiscoveryTarget = _pathfinder.getFogDiscoveryTile( hero );
        if ( fogDiscoveryTarget >= 0 ) {
            uint32_t distanceToFogDiscovery = _pathfinder.getDistance( fogDiscoveryTarget );

            bool useDimensionDoor = false;
            const uint32_t dimensionDoorDist = AIWorldPathfinder::calculatePathPenalty( _pathfinder.getDimensionDoorPath( hero, fogDiscoveryTarget ) );
            if ( dimensionDoorDist > 0 && ( distanceToFogDiscovery == 0 || dimensionDoorDist < distanceToFogDiscovery / 2 ) ) {
                distanceToFogDiscovery = dimensionDoorDist;
                useDimensionDoor = true;
            }

            getObjectValue( fogDiscoveryTarget, distanceToFogDiscovery, fogDiscoveryValue, useDimensionDoor );
        }

        if ( priorityTarget != -1 ) {
            if ( fogDiscoveryTarget >= 0 && fogDiscoveryValue > maxPriority ) {
                priorityTarget = fogDiscoveryTarget;
                maxPriority = fogDiscoveryValue;
            }
            DEBUG_LOG( DBG_AI, DBG_INFO,
                       hero.GetName() << ": priority selected: " << priorityTarget << " value is " << maxPriority << " (" << MP2::StringObject( objectType ) << ")" )
        }
        else if ( !heroInPatrolMode ) {
            priorityTarget = fogDiscoveryTarget;
            DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() << " can't find an object. Scouting the fog of war at " << priorityTarget )
        }

        return priorityTarget;
    }

    void Normal::HeroesActionComplete( Heroes & hero, int32_t tileIndex, const MP2::MapObjectType objectType )
    {
        Castle * castle = hero.inCastleMutable();
        if ( castle ) {
            reinforceHeroInCastle( hero, *castle, castle->GetKingdom().GetFunds() );
        }

        if ( isMonsterStrengthCacheable( objectType ) ) {
            _neutralMonsterStrengthCache.erase( tileIndex );
        }
        if ( objectType == MP2::OBJ_CASTLE || objectType == MP2::OBJ_HEROES ) {
            const auto it = _priorityTargets.find( tileIndex );
            if ( it != _priorityTargets.end() ) {
                if ( it->second == PriorityTask::DEFEND ) {
                    hero.SetModes( Heroes::SLEEPER );
                }

                _priorityTargets.erase( it );
            }
        }
    }

    bool Normal::HeroesTurn( VecHeroes & heroes )
    {
        if ( heroes.empty() ) {
            // No heroes so we indicate that all heroes moved.
            return true;
        }

        std::vector<HeroToMove> availableHeroes;

        for ( Heroes * hero : heroes ) {
            addHeroToMove( hero, availableHeroes );
        }

        const double originalMonsterStrengthMultiplier = _pathfinder.getCurrentArmyStrengthMultiplier();

        const int monsterStrengthMultiplierCount = 2;
        const double monsterStrengthMultipliers[monsterStrengthMultiplierCount] = { ARMY_ADVANTAGE_MEDIUM, ARMY_ADVANTAGE_SMALL };

        while ( !availableHeroes.empty() ) {
            Heroes * bestHero = availableHeroes.front().hero;
            double maxPriority = 0;
            int bestTargetIndex = -1;

            while ( true ) {
                for ( const HeroToMove & heroInfo : availableHeroes ) {
                    double priority = -1;
                    const int targetIndex = getPriorityTarget( heroInfo, priority );
                    if ( targetIndex != -1 && ( priority > maxPriority || bestTargetIndex == -1 ) ) {
                        maxPriority = priority;
                        bestTargetIndex = targetIndex;
                        bestHero = heroInfo.hero;
                    }
                }

                if ( bestTargetIndex != -1 ) {
                    break;
                }

                // If nowhere to move perhaps it's because of high monster estimation. Let's reduce it.
                const double currentMonsterStrengthMultiplier = _pathfinder.getCurrentArmyStrengthMultiplier();
                bool setNewMultiplier = false;
                for ( int i = 0; i < monsterStrengthMultiplierCount; ++i ) {
                    if ( currentMonsterStrengthMultiplier > monsterStrengthMultipliers[i] ) {
                        _pathfinder.setArmyStrengthMultiplier( bestHero->isLosingGame() ? ARMY_ADVANTAGE_DESPERATE : monsterStrengthMultipliers[i] );
                        _pathfinder.setSpellPointReserve( 0 );
                        setNewMultiplier = true;
                        break;
                    }
                }

                if ( !setNewMultiplier ) {
                    break;
                }
            }

            if ( bestTargetIndex == -1 ) {
                // Possibly heroes have nothing to do because one of them is blocking the way. Move a random hero randomly and see what happens.
                Rand::Shuffle( availableHeroes );

                for ( HeroToMove & heroInfo : availableHeroes ) {
                    // Skip heroes who are in castles or on patrol.
                    if ( heroInfo.patrolCenter >= 0 && heroInfo.hero->inCastle() != nullptr ) {
                        continue;
                    }

                    if ( !AIWorldPathfinder::isHeroPossiblyBlockingWay( *heroInfo.hero ) ) {
                        continue;
                    }

                    const int targetIndex = _pathfinder.getNearestTileToMove( *heroInfo.hero );
                    if ( targetIndex != -1 ) {
                        bestTargetIndex = targetIndex;
                        bestHero = heroInfo.hero;

                        DEBUG_LOG( DBG_AI, DBG_INFO, bestHero->GetName() << " may be blocking the way. Moving to " << bestTargetIndex )

                        break;
                    }
                }

                if ( bestTargetIndex == -1 ) {
                    // Nothing to do. Stop everything
                    _pathfinder.setArmyStrengthMultiplier( originalMonsterStrengthMultiplier );
                    break;
                }
            }

            const size_t heroesBefore = heroes.size();
            _pathfinder.reEvaluateIfNeeded( *bestHero );

            // check if we want to use Dimension Door spell or move regularly
            const std::list<Route::Step> & dimensionPath = _pathfinder.getDimensionDoorPath( *bestHero, bestTargetIndex );
            const uint32_t dimensionDoorDistance = AIWorldPathfinder::calculatePathPenalty( dimensionPath );
            const uint32_t moveDistance = _pathfinder.getDistance( bestTargetIndex );
            if ( dimensionDoorDistance && ( !moveDistance || dimensionDoorDistance < moveDistance / 2 ) ) {
                HeroesCastDimensionDoor( *bestHero, dimensionPath.front().GetIndex() );
            }
            else {
                bestHero->GetPath().setPath( _pathfinder.buildPath( bestTargetIndex ), bestTargetIndex );

                HeroesMove( *bestHero );
            }

            if ( heroes.size() > heroesBefore ) {
                addHeroToMove( heroes.back(), availableHeroes );
            }

            for ( size_t i = 0; i < availableHeroes.size(); ) {
                if ( !availableHeroes[i].hero->MayStillMove( false, false ) ) {
                    availableHeroes.erase( availableHeroes.begin() + i );
                    continue;
                }

                ++i;
            }

            _pathfinder.setArmyStrengthMultiplier( originalMonsterStrengthMultiplier );
            _pathfinder.setSpellPointReserve( 0.5 );
        }

        const bool allHeroesMoved = availableHeroes.empty();

        _pathfinder.setArmyStrengthMultiplier( originalMonsterStrengthMultiplier );
        _pathfinder.setSpellPointReserve( 0.5 );

        return allHeroesMoved;
    }
}
