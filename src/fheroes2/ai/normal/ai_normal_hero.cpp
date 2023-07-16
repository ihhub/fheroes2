/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2020 - 2023                                             *
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
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "ai.h"
#include "ai_normal.h"
#include "army.h"
#include "army_troop.h"
#include "artifact.h"
#include "castle.h"
#include "color.h"
#include "game_interface.h"
#include "game_over.h"
#include "game_static.h"
#include "gamedefs.h"
#include "ground.h"
#include "heroes.h"
#include "interface_status.h"
#include "kingdom.h"
#include "logging.h"
#include "luck.h"
#include "maps.h"
#include "maps_tiles.h"
#include "maps_tiles_helper.h"
#include "math_base.h"
#include "monster.h"
#include "morale.h"
#include "mp2.h"
#include "pairs.h"
#include "payment.h"
#include "players.h"
#include "profit.h"
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

    uint32_t getDistanceToObject( const Heroes & hero, const AIWorldPathfinder & pathfinder, const int32_t index )
    {
        const uint32_t dist = pathfinder.getDistance( index );

        const std::list<Route::Step> dimensionDoorSteps = pathfinder.getDimensionDoorPath( hero, index );
        if ( dimensionDoorSteps.empty() ) {
            return dist;
        }

        const uint32_t dimensionDoorDist = AIWorldPathfinder::calculatePathPenalty( dimensionDoorSteps );
        assert( dimensionDoorDist > 0 );

        if ( dist == 0 || dimensionDoorDist < dist / 2 ) {
            return dimensionDoorDist;
        }

        return dist;
    }

    bool AIShouldVisitCastle( const Heroes & hero, int castleIndex, const double heroArmyStrength )
    {
        const Castle * castle = world.getCastleEntrance( Maps::GetPoint( castleIndex ) );
        if ( castle == nullptr ) {
            // How is it possible that a castle does not exist?
            assert( 0 );
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
        const double castleStrength = castle->GetGarrisonStrength( &hero ) * advantage;

        return heroArmyStrength > castleStrength;
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

    bool isSpellUsedByAI( const int spellId )
    {
        // TODO: All these spells are not used by AI at the moment.
        switch ( spellId ) {
        case Spell::EARTHQUAKE:
        case Spell::HAUNT:
        case Spell::IDENTIFYHERO:
        case Spell::SETAGUARDIAN:
        case Spell::SETEGUARDIAN:
        case Spell::SETFGUARDIAN:
        case Spell::SETWGUARDIAN:
        case Spell::TELEPORT:
        case Spell::VIEWARTIFACTS:
        case Spell::VIEWHEROES:
        case Spell::VIEWMINES:
        case Spell::VIEWRESOURCES:
        case Spell::VIEWTOWNS:
        case Spell::VISIONS:
            return false;
        default:
            break;
        }

        return true;
    }

    bool HeroesValidObject( const Heroes & hero, const double heroArmyStrength, const int32_t index, const AIWorldPathfinder & pathfinder, AI::Normal & ai,
                            const double armyStrengthThreshold )
    {
        const Maps::Tiles & tile = world.GetTiles( index );
        const MP2::MapObjectType objectType = tile.GetObject();

        // WINS_ARTIFACT victory condition does not apply to AI-controlled players, we should leave this artifact untouched for the human player
        if ( MP2::isArtifactObject( objectType ) ) {
            const Artifact art = getArtifactFromTile( tile );

            if ( art.isValid() && isFindArtifactVictoryConditionForHuman( art ) ) {
                return false;
            }
        }

        const Army & army = hero.GetArmy();
        const Kingdom & kingdom = hero.GetKingdom();

        // If you add a new object to a group of objects sort them alphabetically.
        switch ( objectType ) {
        case MP2::OBJ_BOTTLE:
        case MP2::OBJ_CAMPFIRE:
        case MP2::OBJ_FLOTSAM:
        case MP2::OBJ_RESOURCE:
        case MP2::OBJ_SEA_CHEST:
        case MP2::OBJ_SHIPWRECK_SURVIVOR:
        case MP2::OBJ_TREASURE_CHEST:
            return true;

        case MP2::OBJ_BUOY:
        case MP2::OBJ_TEMPLE:
            return !hero.isObjectTypeVisited( objectType ) && hero.GetMorale() < Morale::BLOOD && !army.AllTroopsAreUndead();

        case MP2::OBJ_MAGELLANS_MAPS:
            // TODO: avoid hardcoded resource values for objects.
            return !hero.isObjectTypeVisited( objectType, Visit::GLOBAL ) && kingdom.AllowPayment( { Resource::GOLD, 1000 } );

        case MP2::OBJ_ALCHEMIST_LAB:
        case MP2::OBJ_LIGHTHOUSE:
        case MP2::OBJ_MINES:
        case MP2::OBJ_SAWMILL:
            if ( !hero.isFriends( getColorFromTile( tile ) ) ) {
                if ( tile.isCaptureObjectProtected() ) {
                    return isHeroStrongerThan( tile, objectType, ai, heroArmyStrength, AI::ARMY_ADVANTAGE_SMALL );
                }

                return true;
            }
            break;

        case MP2::OBJ_ABANDONED_MINE:
            return isHeroStrongerThan( tile, objectType, ai, heroArmyStrength, AI::ARMY_ADVANTAGE_LARGE );

        case MP2::OBJ_LEAN_TO:
        case MP2::OBJ_MAGIC_GARDEN:
        case MP2::OBJ_SKELETON:
        case MP2::OBJ_WAGON:
        case MP2::OBJ_WATER_WHEEL:
        case MP2::OBJ_WINDMILL:
            return doesTileContainValuableItems( tile );

        case MP2::OBJ_ARTIFACT: {
            if ( hero.IsFullBagArtifacts() )
                return false;

            const Maps::ArtifactCaptureCondition condition = getArtifactCaptureCondition( tile );

            if ( condition == Maps::ArtifactCaptureCondition::PAY_2000_GOLD || condition == Maps::ArtifactCaptureCondition::PAY_2500_GOLD_AND_3_RESOURCES
                 || condition == Maps::ArtifactCaptureCondition::PAY_3000_GOLD_AND_5_RESOURCES ) {
                return kingdom.AllowPayment( getArtifactResourceRequirement( tile ) );
            }

            if ( condition == Maps::ArtifactCaptureCondition::HAVE_WISDOM_SKILL || condition == Maps::ArtifactCaptureCondition::HAVE_LEADERSHIP_SKILL ) {
                return hero.HasSecondarySkill( getArtifactSecondarySkillRequirement( tile ).Skill() );
            }

            // 6 - 50 rogues, 7 - 1 gin, 8,9,10,11,12,13 - 1 monster level4
            if ( condition >= Maps::ArtifactCaptureCondition::FIGHT_50_ROGUES && condition <= Maps::ArtifactCaptureCondition::FIGHT_1_BONE_DRAGON ) {
                return isHeroStrongerThan( tile, objectType, ai, heroArmyStrength, AI::ARMY_ADVANTAGE_LARGE );
            }

            // No conditions to capture an artifact exist.
            return true;
        }

        case MP2::OBJ_OBSERVATION_TOWER:
            return Maps::getFogTileCountToBeRevealed( index, GameStatic::getFogDiscoveryDistance( GameStatic::FogDiscoveryType::OBSERVATION_TOWER ), hero.GetColor() )
                   > 0;

        case MP2::OBJ_OBELISK:
            // TODO: add the logic to dig an Ultimate artifact when a digging tile is visible.
            // But for now AI should not waste time visiting Obelisks.
            return false;

        case MP2::OBJ_BARRIER:
            return kingdom.IsVisitTravelersTent( getColorFromTile( tile ) );

        case MP2::OBJ_TRAVELLER_TENT:
            return !kingdom.IsVisitTravelersTent( getColorFromTile( tile ) );

        case MP2::OBJ_SHRINE_FIRST_CIRCLE:
        case MP2::OBJ_SHRINE_SECOND_CIRCLE:
        case MP2::OBJ_SHRINE_THIRD_CIRCLE: {
            const Spell & spell = getSpellFromTile( tile );
            if ( !spell.isValid() ) {
                // The spell cannot be invalid!
                assert( 0 );
                return false;
            }

            if ( !hero.HaveSpellBook() || hero.HaveSpell( spell, true )
                 || ( 3 == spell.Level() && Skill::Level::NONE == hero.GetLevelSkill( Skill::Secondary::WISDOM ) ) ) {
                return false;
            }

            if ( hero.isVisited( tile, Visit::GLOBAL ) && !isSpellUsedByAI( spell.GetID() ) ) {
                return false;
            }
            return true;
        }

        // Arena allows to visit only one time for the whole map.
        case MP2::OBJ_ARENA:
            return !hero.isObjectTypeVisited( objectType );

        // On-time visit free Primary Skill or Experience object.
        case MP2::OBJ_FORT:
        case MP2::OBJ_GAZEBO:
        case MP2::OBJ_MERCENARY_CAMP:
        case MP2::OBJ_STANDING_STONES:
        case MP2::OBJ_WITCH_DOCTORS_HUT:
            return !hero.isVisited( tile );

        // One time visit Secondary Skill object.
        case MP2::OBJ_WITCHS_HUT: {
            const Skill::Secondary & skill = getSecondarySkillFromWitchsHut( tile );
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

        case MP2::OBJ_TREE_OF_KNOWLEDGE:
            if ( !hero.isVisited( tile ) ) {
                const Funds & rc = getTreeOfKnowledgeRequirement( tile );
                // If the payment is required do not waste all resources from the kingdom. Use them wisely.
                if ( rc.GetValidItemsCount() == 0 || kingdom.AllowPayment( rc * 5 ) ) {
                    return true;
                }
            }
            break;

        // Objects increasing Luck.
        case MP2::OBJ_FAERIE_RING:
        case MP2::OBJ_FOUNTAIN:
        case MP2::OBJ_IDOL:
        case MP2::OBJ_MERMAID:
            return !hero.isObjectTypeVisited( objectType ) && hero.GetLuck() < Luck::IRISH;

        // Objects increasing Movement points and Morale.
        case MP2::OBJ_OASIS:
        case MP2::OBJ_WATERING_HOLE: {
            if ( hero.isObjectTypeVisited( objectType ) ) {
                return false;
            }

            const uint32_t distance = getDistanceToObject( hero, pathfinder, index );
            if ( distance == 0 ) {
                return false;
            }

            const double movementPenalty = 2.0 * distance;
            return movementPenalty < GameStatic::getMovementPointBonus( objectType ) || hero.GetMorale() < Morale::BLOOD;
        }

        case MP2::OBJ_MAGIC_WELL: {
            if ( hero.isObjectTypeVisited( objectType ) ) {
                return false;
            }

            if ( !hero.HaveSpellBook() ) {
                return false;
            }

            if ( hero.GetSpellPoints() >= hero.GetMaxSpellPoints() ) {
                return false;
            }

            const uint32_t distance = getDistanceToObject( hero, pathfinder, index );
            if ( distance == 0 ) {
                return false;
            }

            if ( distance > hero.GetMovePoints() && hero.getDailyRestoredSpellPoints() + hero.GetSpellPoints() >= hero.GetMaxSpellPoints() ) {
                // The Well is located at a distance which cannot be reached by the hero at the current turn.
                // But if the hero will restore all spell points by the next day there is no reason to even to visit the Well.
                return false;
            }

            return true;
        }

        case MP2::OBJ_ARTESIAN_SPRING:
            return !hero.isVisited( tile, Visit::GLOBAL ) && hero.HaveSpellBook() && hero.GetSpellPoints() < 2 * hero.GetMaxSpellPoints();

        case MP2::OBJ_XANADU:
            return !hero.isVisited( tile ) && GameStatic::isHeroWorthyToVisitXanadu( hero );

        // Dwellings with free army.
        case MP2::OBJ_ARCHER_HOUSE:
        case MP2::OBJ_CAVE:
        case MP2::OBJ_DWARF_COTTAGE:
        case MP2::OBJ_EXCAVATION:
        case MP2::OBJ_GOBLIN_HUT:
        case MP2::OBJ_HALFLING_HOLE:
        case MP2::OBJ_PEASANT_HUT:
        case MP2::OBJ_TREE_HOUSE:
        case MP2::OBJ_WATCH_TOWER: {
            const Troop & troop = getTroopFromTile( tile );
            if ( !troop.isValid() ) {
                return false;
            }

            const bool armyHasMonster = army.HasMonster( troop.GetMonster() );
            if ( !armyHasMonster && army.isFullHouse() && army.areAllTroopsUnique() ) {
                return false;
            }

            return isArmyValuableToObtain( troop, armyStrengthThreshold, armyHasMonster );
        }

        // Dwellings where AI can recruit an army.
        case MP2::OBJ_AIR_ALTAR:
        case MP2::OBJ_BARROW_MOUNDS:
        case MP2::OBJ_DESERT_TENT:
        case MP2::OBJ_EARTH_ALTAR:
        case MP2::OBJ_FIRE_ALTAR:
        case MP2::OBJ_GENIE_LAMP:
        case MP2::OBJ_RUINS:
        case MP2::OBJ_TREE_CITY:
        case MP2::OBJ_WAGON_CAMP:
        case MP2::OBJ_WATER_ALTAR: {
            const Troop & troop = getTroopFromTile( tile );
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

        // Dwellings where AI might fight monsters first before recruiting them.
        case MP2::OBJ_CITY_OF_DEAD:
        case MP2::OBJ_DRAGON_CITY:
        case MP2::OBJ_TROLL_BRIDGE: {
            if ( Color::NONE == getColorFromTile( tile ) ) {
                return isHeroStrongerThan( tile, objectType, ai, heroArmyStrength, AI::ARMY_ADVANTAGE_MEDIUM );
            }

            const Troop & troop = getTroopFromTile( tile );
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

        // Free army upgrade objects.
        case MP2::OBJ_FREEMANS_FOUNDRY:
            return army.HasMonster( Monster::PIKEMAN ) || army.HasMonster( Monster::SWORDSMAN ) || army.HasMonster( Monster::IRON_GOLEM );
        case MP2::OBJ_HILL_FORT:
            return army.HasMonster( Monster::DWARF ) || army.HasMonster( Monster::ORC ) || army.HasMonster( Monster::OGRE );

        // Free army upgrade and extra movement points for the rest of the week.
        case MP2::OBJ_STABLES: {
            if ( army.HasMonster( Monster::CAVALRY ) ) {
                return true;
            }

            const uint32_t distance = getDistanceToObject( hero, pathfinder, index );
            if ( distance == 0 ) {
                return false;
            }

            const int daysActive = DAYOFWEEK - world.GetDay() + 1;
            const double movementBonus = daysActive * GameStatic::getMovementPointBonus( objectType ) - 2.0 * distance;

            return !hero.isObjectTypeVisited( objectType ) && movementBonus > 0;
        }

        // Objects that give goods but curse with bad morale when visiting them for subsequent times.
        case MP2::OBJ_DERELICT_SHIP:
        case MP2::OBJ_GRAVEYARD:
        case MP2::OBJ_SHIPWRECK:
            if ( !hero.isVisited( tile, Visit::GLOBAL ) && doesTileContainValuableItems( tile ) ) {
                Army enemy( tile );
                return enemy.isValid() && isHeroStrongerThan( tile, objectType, ai, heroArmyStrength, 2 );
            }
            break;

        case MP2::OBJ_PYRAMID:
            if ( !hero.isVisited( tile, Visit::GLOBAL ) && doesTileContainValuableItems( tile ) ) {
                Army enemy( tile );
                return enemy.isValid() && Skill::Level::EXPERT == hero.GetLevelSkill( Skill::Secondary::WISDOM )
                       && isHeroStrongerThan( tile, objectType, ai, heroArmyStrength, AI::ARMY_ADVANTAGE_LARGE );
            }
            break;

        case MP2::OBJ_DAEMON_CAVE:
            if ( doesTileContainValuableItems( tile ) ) {
                // AI always chooses to fight the demon's servants and doesn't roll the dice
                return isHeroStrongerThan( tile, objectType, ai, heroArmyStrength, AI::ARMY_ADVANTAGE_MEDIUM );
            }
            break;

        case MP2::OBJ_MONSTER:
            return isHeroStrongerThan( tile, objectType, ai, heroArmyStrength, ( hero.isLosingGame() ? 1.0 : AI::ARMY_ADVANTAGE_MEDIUM ) );

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
            return AIShouldVisitCastle( hero, index, heroArmyStrength ) || ai.isPriorityTask( index );

        case MP2::OBJ_JAIL:
            return kingdom.GetHeroes().size() < Kingdom::GetMaxHeroes();
        case MP2::OBJ_HUT_OF_MAGI:
            return !hero.isObjectTypeVisited( objectType, Visit::GLOBAL ) && !Maps::GetObjectPositions( MP2::OBJ_EYE_OF_MAGI, true ).empty();

        case MP2::OBJ_ALCHEMIST_TOWER: {
            const BagArtifacts & bag = hero.GetBagArtifacts();
            const uint32_t cursed = static_cast<uint32_t>( std::count_if( bag.begin(), bag.end(), []( const Artifact & art ) { return art.containsCurses(); } ) );
            if ( cursed == 0 ) {
                return false;
            }

            const payment_t payment = PaymentConditions::ForAlchemist();
            return kingdom.AllowPayment( payment );
        }

        // AI should never consider a boat as a destination point. It uses them only to make a path.
        case MP2::OBJ_BOAT:
        // Eye of Magi is not an action object at all.
        case MP2::OBJ_EYE_OF_MAGI:
        // No use of these object for AI.
        case MP2::OBJ_ORACLE:
        // AI has no brains to do anything from sign messages.
        case MP2::OBJ_SIGN:
        // AI has no brains to handle Sirens object.
        case MP2::OBJ_SIRENS:
        // TODO: AI doesn't know how it use Sphinx object properly.
        case MP2::OBJ_SPHINX:
        // AI should never consider a stone lith as a destination point. It uses them only to make a path.
        case MP2::OBJ_STONE_LITHS:
        // TODO: AI doesn't know how it use Trading Post object properly.
        case MP2::OBJ_TRADING_POST:
        // AI should never consider a whirlpool as a destination point. It uses them only to make a path.
        case MP2::OBJ_WHIRLPOOL:
            return false;

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
            if ( hero->GetPatrolDistance() == 0 ) {
                DEBUG_LOG( DBG_AI, DBG_TRACE, hero->GetName() << " standing still. Skip turn." )
                return;
            }
        }

        if ( hero->MayStillMove( false, false ) ) {
            availableHeroes.emplace_back();
            AI::HeroToMove & heroInfo = availableHeroes.back();
            heroInfo.hero = hero;

            if ( hero->Modes( Heroes::PATROL ) ) {
                heroInfo.patrolCenter = Maps::GetIndexFromAbsPoint( hero->GetPatrolCenter() );
                heroInfo.patrolDistance = hero->GetPatrolDistance();
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
            , _armyStrengthThreshold( hero.getAIMinimumJoiningArmyStrength() )
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

            const double value = _ai.getObjectValue( _hero, objectInfo.first, objectInfo.second, _ignoreValue, distance );

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
    const double fogDiscoveryBaseValue = -10000.0;

    double getDistanceModifier( const MP2::MapObjectType objectType )
    {
        // The value above 1.0 means that the object is useful only if it is nearby.
        // The value below 1.0 means that the object should remain in focus even on greater distances.
        if ( !MP2::isActionObject( objectType ) ) {
            return 1.0;
        }

        switch ( objectType ) {
        case MP2::OBJ_CASTLE:
            return 0.8;
        case MP2::OBJ_ALCHEMIST_LAB:
        case MP2::OBJ_ARTIFACT:
        case MP2::OBJ_HEROES:
        case MP2::OBJ_MINES:
        case MP2::OBJ_SAWMILL:
            return 0.9;
        case MP2::OBJ_CAMPFIRE:
        case MP2::OBJ_FLOTSAM:
        case MP2::OBJ_GENIE_LAMP:
        case MP2::OBJ_RESOURCE:
        case MP2::OBJ_SEA_CHEST:
            return 0.95;
        case MP2::OBJ_BUOY:
        case MP2::OBJ_TEMPLE:
        case MP2::OBJ_FAERIE_RING:
        case MP2::OBJ_FOUNTAIN:
        case MP2::OBJ_IDOL:
        case MP2::OBJ_MERMAID:
            // In most situations Luck and Morale modifier objects are useful to be visited when they are very close.
            return 1.1;
        default:
            break;
        }

        return 1.0;
    }

    double ScaleWithDistance( const double value, const uint32_t distance, const MP2::MapObjectType objectType )
    {
        if ( distance == 0 ) {
            return value;
        }

        // Some objects do not loose their value drastically over distances. This allows AI heroes to keep focus on important targets.
        const double correctedDistance = distance * getDistanceModifier( objectType );

        // Scale non-linearly (more value lost as distance increases)
        return value - ( correctedDistance * std::log10( correctedDistance ) );
    }

    double getFogDiscoveryValue( const Heroes & hero )
    {
        switch ( hero.getAIRole() ) {
        case Heroes::Role::SCOUT:
            return 0;
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
    // TODO: In the future we need to come up with dynamic object value estimation based not only on a hero's role but on an outcome from movement at certain position.

    double Normal::getGeneralObjectValue( const Heroes & hero, const int index, const double valueToIgnore, const uint32_t distanceToObject ) const
    {
        // In the future these hardcoded values could be configured by the mod
        // 1 tile distance is 100.0 value approximately
        const Maps::Tiles & tile = world.GetTiles( index );
        const MP2::MapObjectType objectType = tile.GetObject();

        switch ( objectType ) {
        case MP2::OBJ_CASTLE: {
            const Castle * castle = world.getCastleEntrance( Maps::GetPoint( index ) );
            if ( !castle ) {
                // How is it even possible?
                assert( 0 );
                return valueToIgnore;
            }

            const bool priority = isPriorityTask( index );
            const bool critical = isCriticalTask( index );
            if ( hero.GetColor() == castle->GetColor() ) {
                double value = castle->getVisitValue( hero );
                if ( critical )
                    return 10000 + value;

                if ( !priority && value < 500 )
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
        case MP2::OBJ_HEROES: {
            const Heroes * otherHero = tile.GetHeroes();
            assert( otherHero );
            if ( !otherHero ) {
                // How is it even possible?
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

            // focus on enemy hero if there's priority set (i.e. hero is threatening our castle)
            return isCriticalTask( index ) ? 12000.0 : 5000.0;
        }
        case MP2::OBJ_MONSTER: {
            const Army monsters( tile );
            if ( !monsters.isValid() ) {
                // How is it even possible?
                assert( 0 );
                return -dangerousTaskPenalty;
            }

            // TODO: we should add logic to compare monsters and hero army strengths.
            return 1000.0 + monsters.getTotalHP() / 100.0;
        }
        case MP2::OBJ_ALCHEMIST_LAB:
        case MP2::OBJ_MINES:
        case MP2::OBJ_SAWMILL:
        case MP2::OBJ_ABANDONED_MINE: {
            int resourceType = Resource::UNKNOWN;
            int32_t resourceAmount = 0;

            // Abandoned mines are gold mines under the hood
            if ( objectType == MP2::OBJ_ABANDONED_MINE ) {
                resourceType = Resource::GOLD;
                resourceAmount = ProfitConditions::FromMine( Resource::GOLD ).Get( Resource::GOLD );
            }
            else {
                if ( getColorFromTile( tile ) == hero.GetColor() ) {
                    return -valueToIgnore; // don't even attempt to go here
                }

                std::tie( resourceType, resourceAmount ) = getDailyIncomeObjectResources( tile ).getFirstValidResource();
            }

            assert( resourceType != Resource::UNKNOWN && resourceAmount != 0 );

            // Since mines constantly bring resources, they are valuable objects
            return resourceAmount * getResourcePriorityModifier( resourceType, true );
        }
        case MP2::OBJ_ARTIFACT: {
            const Artifact art = getArtifactFromTile( tile );
            assert( art.isValid() );

            // WINS_ARTIFACT victory condition does not apply to AI-controlled players, we should leave this artifact untouched for the human player
            if ( isFindArtifactVictoryConditionForHuman( art ) ) {
                assert( 0 );
                return -dangerousTaskPenalty;
            }

            return 1000.0 * art.getArtifactValue();
        }
        case MP2::OBJ_SEA_CHEST:
        case MP2::OBJ_SHIPWRECK_SURVIVOR:
        case MP2::OBJ_TREASURE_CHEST: {
            if ( getArtifactFromTile( tile ).isValid() ) {
                const Artifact art = getArtifactFromTile( tile );

                // WINS_ARTIFACT victory condition does not apply to AI-controlled players, we should leave this artifact untouched for the human player
                if ( isFindArtifactVictoryConditionForHuman( art ) ) {
                    assert( 0 );
                    return -dangerousTaskPenalty;
                }

                return 1000.0 * art.getArtifactValue();
            }

            const Funds funds = getFundsFromTile( tile );
            assert( funds.gold > 0 || funds.GetValidItemsCount() == 0 );

            return funds.gold * getResourcePriorityModifier( Resource::GOLD, false );
        }
        case MP2::OBJ_DAEMON_CAVE: {
            // If this cave is already empty, then we should never come here
            if ( !doesTileContainValuableItems( tile ) ) {
                assert( 0 );
                return -dangerousTaskPenalty;
            }

            // Daemon Cave always gives 2500 Gold after a battle and AI always chooses to fight the demon's servants and doesn't roll the dice
            return 2500 * getResourcePriorityModifier( Resource::GOLD, false );
        }
        case MP2::OBJ_GRAVEYARD:
        case MP2::OBJ_SHIPWRECK:
        case MP2::OBJ_SKELETON:
        case MP2::OBJ_WAGON: {
            if ( !getArtifactFromTile( tile ).isValid() ) {
                // Don't waste time to go here.
                return -dangerousTaskPenalty;
            }

            const Artifact art = getArtifactFromTile( tile );

            // WINS_ARTIFACT victory condition does not apply to AI-controlled players, we should leave this artifact untouched for the human player
            if ( isFindArtifactVictoryConditionForHuman( art ) ) {
                assert( 0 );
                return -dangerousTaskPenalty;
            }

            return 1000.0 * art.getArtifactValue();
        }
        case MP2::OBJ_BOTTLE: {
            // A bottle is useless to AI as it contains only a message but it might block path.
            return 0;
        }
        case MP2::OBJ_CAMPFIRE:
        case MP2::OBJ_DERELICT_SHIP:
        case MP2::OBJ_FLOTSAM:
        case MP2::OBJ_LEAN_TO:
        case MP2::OBJ_MAGIC_GARDEN:
        case MP2::OBJ_RESOURCE:
        case MP2::OBJ_WATER_WHEEL:
        case MP2::OBJ_WINDMILL: {
            const Funds loot = getFundsFromTile( tile );

            double value = 0;

            Resource::forEach( loot.GetValidItems(), [this, &loot, &value]( const int res ) {
                const int amount = loot.Get( res );
                if ( amount <= 0 ) {
                    return;
                }

                value += amount * getResourcePriorityModifier( res, false );
            } );

            // This object could have already been visited
            if ( value < 1 ) {
                return -valueToIgnore;
            }

            return value;
        }
        case MP2::OBJ_LIGHTHOUSE: {
            // TODO: add more complex logic for cases when AI has boats.
            if ( getColorFromTile( tile ) == hero.GetColor() ) {
                return -dangerousTaskPenalty; // don't even attempt to go here
            }
            return 500;
        }
        case MP2::OBJ_XANADU: {
            return 3000;
        }
        case MP2::OBJ_SHRINE_FIRST_CIRCLE:
        case MP2::OBJ_SHRINE_SECOND_CIRCLE:
        case MP2::OBJ_SHRINE_THIRD_CIRCLE: {
            const Spell & spell = getSpellFromTile( tile );
            return spell.getStrategicValue( hero.GetArmy().GetStrength(), hero.GetMaxSpellPoints(), hero.GetPower() );
        }
        case MP2::OBJ_ARENA:
        case MP2::OBJ_FORT:
        case MP2::OBJ_MERCENARY_CAMP:
        case MP2::OBJ_STANDING_STONES:
        case MP2::OBJ_TREE_OF_KNOWLEDGE:
        case MP2::OBJ_WITCH_DOCTORS_HUT:
        case MP2::OBJ_WITCHS_HUT: {
            return 500.0;
        }
        case MP2::OBJ_AIR_ALTAR:
        case MP2::OBJ_ARCHER_HOUSE:
        case MP2::OBJ_BARROW_MOUNDS:
        case MP2::OBJ_CAVE:
        case MP2::OBJ_CITY_OF_DEAD:
        case MP2::OBJ_DESERT_TENT:
        case MP2::OBJ_DRAGON_CITY:
        case MP2::OBJ_DWARF_COTTAGE:
        case MP2::OBJ_EARTH_ALTAR:
        case MP2::OBJ_EXCAVATION:
        case MP2::OBJ_FIRE_ALTAR:
        case MP2::OBJ_GENIE_LAMP:
        case MP2::OBJ_GOBLIN_HUT:
        case MP2::OBJ_HALFLING_HOLE:
        case MP2::OBJ_PEASANT_HUT:
        case MP2::OBJ_RUINS:
        case MP2::OBJ_TREE_CITY:
        case MP2::OBJ_TREE_HOUSE:
        case MP2::OBJ_TROLL_BRIDGE:
        case MP2::OBJ_WAGON_CAMP:
        case MP2::OBJ_WATCH_TOWER:
        case MP2::OBJ_WATER_ALTAR: {
            return getTroopFromTile( tile ).GetStrength();
        }
        case MP2::OBJ_STONE_LITHS: {
            // Stone lith is not considered by AI as an action object. If this assertion blows up something is wrong with the logic.
            assert( 0 );
            return -dangerousTaskPenalty;
        }
        case MP2::OBJ_OBSERVATION_TOWER: {
            const int fogCountToUncover
                = Maps::getFogTileCountToBeRevealed( index, GameStatic::getFogDiscoveryDistance( GameStatic::FogDiscoveryType::OBSERVATION_TOWER ), hero.GetColor() );
            if ( fogCountToUncover <= 0 ) {
                // Nothing to uncover.
                return -dangerousTaskPenalty;
            }
            return fogCountToUncover;
        }
        case MP2::OBJ_MAGELLANS_MAPS: {
            // Very valuable object.
            return 5000;
        }
        case MP2::OBJ_BOAT:
        case MP2::OBJ_COAST:
        case MP2::OBJ_WHIRLPOOL: {
            // Coast is not an object while Whirlpool and Boat are not considered by AI as an action object.
            // If this assertion blows up something is wrong with the logic.
            assert( 0 );
            return -dangerousTaskPenalty;
        }
        case MP2::OBJ_ARTESIAN_SPRING: {
            if ( !hero.HaveSpellBook() || hero.GetSpellPoints() * 2 >= hero.GetMaxSpellPoints() ) {
                // No reason to visit Artesian Spring with no magic book or if no points will be gained.
                return -dangerousTaskPenalty;
            }

            return hero.isPotentSpellcaster() ? 1500 : 0;
        }
        case MP2::OBJ_MAGIC_WELL: {
            if ( !hero.HaveSpellBook() || hero.GetSpellPoints() >= hero.GetMaxSpellPoints() ) {
                // No reason to visit Magic Well with no magic book or if no points will be gained.
                return -dangerousTaskPenalty;
            }

            return hero.isPotentSpellcaster() ? 1500 : 0;
        }
        case MP2::OBJ_BUOY:
        case MP2::OBJ_TEMPLE: {
            if ( hero.GetArmy().AllTroopsAreUndead() ) {
                // All troops are undead, no use of Morale.
                return 0;
            }

            const int morale = hero.GetMorale();
            if ( morale >= Morale::BLOOD ) {
                return -dangerousTaskPenalty; // No reason to visit with maximum morale
            }
            if ( morale == Morale::GREAT ) {
                return -4000; // Morale is good enough to avoid visiting this object
            }
            if ( morale == Morale::GOOD ) {
                return -2000; // Is it worth visiting this facility with a morale slightly better than neutral?
            }
            if ( morale == Morale::NORMAL ) {
                return 50;
            }

            return 100;
        }
        case MP2::OBJ_STABLES: {
            const int daysActive = DAYOFWEEK - world.GetDay() + 1;
            double movementBonus = daysActive * GameStatic::getMovementPointBonus( objectType ) - 2.0 * distanceToObject;
            if ( movementBonus < 0 ) {
                // Looks like this is too far away.
                movementBonus = 0;
            }

            const double upgradeValue = getMonsterUpgradeValue( hero.GetArmy(), Monster::CHAMPION );
            return movementBonus + freeMonsterUpgradeModifier * upgradeValue;
        }
        case MP2::OBJ_FREEMANS_FOUNDRY: {
            const double upgradePikemanValue = getMonsterUpgradeValue( hero.GetArmy(), Monster::PIKEMAN );
            const double upgradeSwordsmanValue = getMonsterUpgradeValue( hero.GetArmy(), Monster::SWORDSMAN );
            const double upgradeGolemValue = getMonsterUpgradeValue( hero.GetArmy(), Monster::IRON_GOLEM );

            return freeMonsterUpgradeModifier * ( upgradePikemanValue + upgradeSwordsmanValue + upgradeGolemValue );
        }
        case MP2::OBJ_HILL_FORT: {
            const double upgradeDwarfValue = getMonsterUpgradeValue( hero.GetArmy(), Monster::DWARF );
            const double upgradeOrcValue = getMonsterUpgradeValue( hero.GetArmy(), Monster::ORC );
            const double upgradeOgreValue = getMonsterUpgradeValue( hero.GetArmy(), Monster::OGRE );

            return freeMonsterUpgradeModifier * ( upgradeDwarfValue + upgradeOrcValue + upgradeOgreValue );
        }
        case MP2::OBJ_BARRIER:
        case MP2::OBJ_TRAVELLER_TENT: {
            // Most likely it'll lead to opening more land.
            return 1000;
        }
        case MP2::OBJ_OASIS:
        case MP2::OBJ_WATERING_HOLE: {
            return std::max( GameStatic::getMovementPointBonus( objectType ) - 2.0 * distanceToObject, 0.0 );
        }
        case MP2::OBJ_JAIL: {
            // A free hero is always good and it could be very powerful.
            return 3000;
        }
        case MP2::OBJ_HUT_OF_MAGI: {
            const MapsIndexes eyeMagiIndexes = Maps::GetObjectPositions( MP2::OBJ_EYE_OF_MAGI, true );
            int fogCountToUncover = 0;
            const int heroColor = hero.GetColor();
            const int eyeViewDistance = GameStatic::getFogDiscoveryDistance( GameStatic::FogDiscoveryType::MAGI_EYES );

            for ( const int32_t eyeIndex : eyeMagiIndexes ) {
                fogCountToUncover += Maps::getFogTileCountToBeRevealed( eyeIndex, eyeViewDistance, heroColor );
            }

            return fogCountToUncover;
        }
        case MP2::OBJ_GAZEBO: {
            // Free 1000 experience. We need to calculate value of this object based on hero's experience. The higher hero's level the less valuable this object is.
            const uint32_t heroExperience = hero.GetExperience();
            const uint32_t nextLevelExperience = Heroes::GetExperienceFromLevel( Heroes::GetLevelFromExperience( heroExperience ) );
            const uint32_t neededExperience = nextLevelExperience - heroExperience;
            if ( neededExperience < 1000 ) {
                // A new level. Have to visit.
                return 1000;
            }

            return 1000.0 * 1000.0 / neededExperience;
        }
        case MP2::OBJ_PYRAMID: {
            return 1500;
        }
        case MP2::OBJ_FAERIE_RING:
        case MP2::OBJ_FOUNTAIN:
        case MP2::OBJ_IDOL:
        case MP2::OBJ_MERMAID: {
            const int luck = hero.GetLuck();
            if ( luck >= Luck::IRISH ) {
                return -dangerousTaskPenalty; // No reason to visit with maximum morale
            }
            if ( luck == Luck::GREAT ) {
                return -4000; // Morale is good enough to avoid visiting this object
            }
            if ( luck == Luck::GOOD ) {
                return -2000; // Is it worth visiting this facility with a morale slightly better than neutral?
            }
            if ( luck == Luck::NORMAL ) {
                return 50;
            }

            return 100;
        }
        case MP2::OBJ_ALCHEMIST_TOWER: {
            const BagArtifacts & bag = hero.GetBagArtifacts();
            const uint32_t cursed = static_cast<uint32_t>( std::count_if( bag.begin(), bag.end(), []( const Artifact & art ) { return art.containsCurses(); } ) );
            if ( cursed == 0 ) {
                return -dangerousTaskPenalty;
            }

            // TODO: evaluate this object properly.
            return 0;
        }
        case MP2::OBJ_EYE_OF_MAGI:
        case MP2::OBJ_ORACLE:
        case MP2::OBJ_SIGN: {
            // These objects are useless for AI.
            return -dangerousTaskPenalty;
        }
        case MP2::OBJ_OBELISK:
        case MP2::OBJ_SIRENS:
        case MP2::OBJ_SPHINX:
        case MP2::OBJ_TRADING_POST: {
            // TODO: add logic to evaluate these objects.
            // As of now these objects should be avoided by AI as they are useless.
            return -dangerousTaskPenalty;
        }
        default:
            // Did you forget to add logic for an action object?
            assert( !MP2::isActionObject( objectType ) );
            break;
        }

        return 0;
    }

    double Normal::getFighterObjectValue( const Heroes & hero, const int index, const double valueToIgnore, const uint32_t distanceToObject ) const
    {
        // Fighters have higher priority for battles and smaller values for other objects.
        assert( hero.getAIRole() == Heroes::Role::FIGHTER || hero.getAIRole() == Heroes::Role::CHAMPION );

        const Maps::Tiles & tile = world.GetTiles( index );
        const MP2::MapObjectType objectType = tile.GetObject();
        const bool anotherFriendlyHeroPresent = _regions[tile.GetRegion()].friendlyHeroes > 1;

        switch ( objectType ) {
        case MP2::OBJ_CASTLE: {
            const Castle * castle = world.getCastleEntrance( Maps::GetPoint( index ) );
            if ( !castle ) {
                // How is it even possible?
                return valueToIgnore;
            }

            const bool priority = isPriorityTask( index );
            const bool critical = isCriticalTask( index );
            if ( hero.GetColor() == castle->GetColor() ) {
                double value = castle->getVisitValue( hero );
                if ( critical )
                    return 15000 + value;

                if ( !priority && value < 500 )
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
        case MP2::OBJ_HEROES: {
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
        case MP2::OBJ_MONSTER: {
            const Army monsters( tile );
            if ( !monsters.isValid() ) {
                // How is it even possible?
                assert( 0 );
                return -dangerousTaskPenalty;
            }

            // TODO: we should add logic to compare monsters and hero army strengths.
            return ( anotherFriendlyHeroPresent ? 4000.0 : 1000.0 ) + monsters.getTotalHP() / 100.0;
        }
        case MP2::OBJ_ABANDONED_MINE: {
            return 5000.0;
        }
        case MP2::OBJ_ARTIFACT: {
            const Artifact art = getArtifactFromTile( tile );
            assert( art.isValid() );

            // WINS_ARTIFACT victory condition does not apply to AI-controlled players, we should leave this artifact untouched for the human player
            if ( isFindArtifactVictoryConditionForHuman( art ) ) {
                assert( 0 );
                return -dangerousTaskPenalty;
            }

            return 1500.0 * art.getArtifactValue();
        }
        case MP2::OBJ_CAMPFIRE:
        case MP2::OBJ_FLOTSAM:
        case MP2::OBJ_LEAN_TO:
        case MP2::OBJ_MAGIC_GARDEN:
        case MP2::OBJ_RESOURCE:
        case MP2::OBJ_WATER_WHEEL:
        case MP2::OBJ_WINDMILL: {
            if ( anotherFriendlyHeroPresent ) {
                return 100.0;
            }
            // fall through to the general value calculation
            break;
        }
        case MP2::OBJ_LIGHTHOUSE: {
            // TODO: add more complex logic for cases when AI has boats.
            if ( getColorFromTile( tile ) == hero.GetColor() ) {
                return -dangerousTaskPenalty; // don't even attempt to go here
            }
            return 250;
        }
        case MP2::OBJ_XANADU: {
            return 3500;
        }
        case MP2::OBJ_SHRINE_FIRST_CIRCLE:
        case MP2::OBJ_SHRINE_SECOND_CIRCLE:
        case MP2::OBJ_SHRINE_THIRD_CIRCLE: {
            const Spell & spell = getSpellFromTile( tile );
            return spell.getStrategicValue( hero.GetArmy().GetStrength(), hero.GetMaxSpellPoints(), hero.GetPower() ) * 1.1;
        }
        case MP2::OBJ_ARENA:
        case MP2::OBJ_FORT:
        case MP2::OBJ_MERCENARY_CAMP:
        case MP2::OBJ_STANDING_STONES:
        case MP2::OBJ_TREE_OF_KNOWLEDGE:
        case MP2::OBJ_WITCH_DOCTORS_HUT:
        case MP2::OBJ_WITCHS_HUT: {
            return 1250.0;
        }
        case MP2::OBJ_OBSERVATION_TOWER: {
            return getGeneralObjectValue( hero, index, valueToIgnore, distanceToObject ) / 2;
        }
        case MP2::OBJ_ARTESIAN_SPRING: {
            if ( !hero.HaveSpellBook() || hero.GetSpellPoints() * 2 >= hero.GetMaxSpellPoints() ) {
                // No reason to visit Artesian Spring with no magic book or if no points will be gained.
                return -dangerousTaskPenalty;
            }

            return hero.isPotentSpellcaster() ? 2000 : 0;
        }
        case MP2::OBJ_MAGIC_WELL: {
            if ( !hero.HaveSpellBook() || hero.GetSpellPoints() >= hero.GetMaxSpellPoints() ) {
                // No reason to visit Magic Well with no magic book or if no points will be gained.
                return -dangerousTaskPenalty;
            }

            return hero.isPotentSpellcaster() ? 2000 : 0;
        }
        case MP2::OBJ_BUOY:
        case MP2::OBJ_TEMPLE: {
            if ( hero.GetArmy().AllTroopsAreUndead() ) {
                // All troops are undead, no use of Morale.
                return 0;
            }

            const int morale = hero.GetMorale();
            if ( morale >= Morale::BLOOD ) {
                return -dangerousTaskPenalty; // No reason to visit with maximum morale
            }
            if ( morale == Morale::GREAT ) {
                return -4000; // Morale is good enough to avoid visiting this object
            }
            if ( morale == Morale::GOOD ) {
                return -2000; // Is it worth visiting this facility with a morale slightly better than neutral?
            }
            if ( morale == Morale::NORMAL ) {
                return 50;
            }

            return 200;
        }
        case MP2::OBJ_HUT_OF_MAGI: {
            return getGeneralObjectValue( hero, index, valueToIgnore, distanceToObject ) / 2;
        }
        case MP2::OBJ_PYRAMID: {
            return 10000;
        }
        default:
            break;
        }

        return getGeneralObjectValue( hero, index, valueToIgnore, distanceToObject );
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

        switch ( objectType ) {
        case MP2::OBJ_HEROES: {
            const Heroes * otherHero = tile.GetHeroes();
            assert( otherHero );
            if ( !otherHero ) {
                return valueToIgnore;
            }

            if ( hero.GetColor() == otherHero->GetColor() ) {
                // Will be handled by the main task.
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

            // focus on enemy hero if there's priority set (i.e. hero is threatening our castle)
            return isCriticalTask( index ) ? 10000.0 : tenTiles;
        }
        case MP2::OBJ_MONSTER: {
            const Army monsters( tile );
            if ( !monsters.isValid() ) {
                // How is it even possible?
                assert( 0 );
                return -dangerousTaskPenalty;
            }

            // TODO: we should add logic to compare monsters and hero army strengths.
            return twoTiles + monsters.getTotalHP() / 100.0;
        }
        case MP2::OBJ_ALCHEMIST_LAB:
        case MP2::OBJ_MINES:
        case MP2::OBJ_SAWMILL: {
            if ( getColorFromTile( tile ) == hero.GetColor() ) {
                return -dangerousTaskPenalty; // don't even attempt to go here
            }
            return ( getDailyIncomeObjectResources( tile ).gold > 0 ) ? tenTiles : fiveTiles;
        }
        case MP2::OBJ_CAMPFIRE:
        case MP2::OBJ_FLOTSAM:
        case MP2::OBJ_GENIE_LAMP:
        case MP2::OBJ_RESOURCE:
        case MP2::OBJ_SEA_CHEST: {
            return twoTiles;
        }
        case MP2::OBJ_ARENA:
        case MP2::OBJ_FORT:
        case MP2::OBJ_MERCENARY_CAMP:
        case MP2::OBJ_STANDING_STONES:
        case MP2::OBJ_TREE_OF_KNOWLEDGE:
        case MP2::OBJ_WITCH_DOCTORS_HUT:
        case MP2::OBJ_WITCHS_HUT:
        case MP2::OBJ_XANADU: {
            return fiveTiles;
        }
        case MP2::OBJ_ARTESIAN_SPRING: {
            if ( !hero.HaveSpellBook() || hero.GetSpellPoints() * 2 >= hero.GetMaxSpellPoints() ) {
                // No reason to visit Artesian Spring with no magic book or if no points will be gained.
                return -dangerousTaskPenalty;
            }

            return hero.isPotentSpellcaster() ? fiveTiles : 0;
        }
        case MP2::OBJ_MAGIC_WELL: {
            if ( !hero.HaveSpellBook() || hero.GetSpellPoints() >= hero.GetMaxSpellPoints() ) {
                // No reason to visit Magic Well with no magic book or if no points will be gained.
                return -dangerousTaskPenalty;
            }

            return hero.isPotentSpellcaster() ? fiveTiles : 0;
        }
        default:
            break;
        }

        return getGeneralObjectValue( hero, index, valueToIgnore, distanceToObject );
    }

    double Normal::getObjectValue( const Heroes & hero, const int index, const int objectType, const double valueToIgnore, const uint32_t distanceToObject ) const
    {
        assert( objectType == world.GetTiles( index ).GetObject() );

        switch ( hero.getAIRole() ) {
        case Heroes::Role::HUNTER:
        case Heroes::Role::SCOUT:
            return getGeneralObjectValue( hero, index, valueToIgnore, distanceToObject );
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

    int Normal::getCourierMainTarget( const Heroes & hero, const AIWorldPathfinder & pathfinder, double lowestPossibleValue ) const
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
            const uint32_t dist = getDistanceToObject( hero, pathfinder, currentHeroIndex );
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
            const uint32_t dist = getDistanceToObject( hero, pathfinder, currentCastleIndex );

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

    int Normal::getPriorityTarget( const HeroToMove & heroInfo, double & maxPriority )
    {
        Heroes & hero = *heroInfo.hero;

        DEBUG_LOG( DBG_AI, DBG_INFO, "Find Adventure Map target for hero " << hero.GetName() << " at current position " << hero.GetIndex() )

        const double lowestPossibleValue = -1.0 * Maps::Ground::slowestMovePenalty * world.getSize();
        const bool heroInPatrolMode = heroInfo.patrolCenter != -1;
        const double heroStrength = hero.GetArmy().GetStrength();

        int priorityTarget = -1;
        maxPriority = lowestPossibleValue;
#ifdef WITH_DEBUG
        MP2::MapObjectType objectType = MP2::OBJ_NONE;

        // If this assertion blows up then the array is not sorted and the logic below will not work as intended.
        assert( std::is_sorted( _mapActionObjects.begin(), _mapActionObjects.end() ) );

        std::set<int> objectIndexes;

        for ( const auto & actionObject : _mapActionObjects ) {
            if ( actionObject.second == MP2::OBJ_HEROES ) {
                assert( world.GetTiles( actionObject.first ).GetHeroes() != nullptr );
            }

            const auto [dummy, inserted] = objectIndexes.emplace( actionObject.first );
            if ( !inserted ) {
                assert( 0 );
            }
        }
#endif

        // pre-cache the pathfinder
        _pathfinder.reEvaluateIfNeeded( hero );

        const uint32_t leftMovePoints = hero.GetMovePoints();

        ObjectValidator objectValidator( hero, _pathfinder, *this );
        ObjectValueStorage valueStorage( hero, *this, lowestPossibleValue );

        auto getObjectValue = [&objectValidator, &valueStorage, this, heroStrength, &hero, leftMovePoints]( const int destination, uint32_t & distance, double & value,
                                                                                                            const MP2::MapObjectType type, const bool isDimensionDoor ) {
            if ( !isDimensionDoor ) {
                // Dimension door path does not include any objects on the way.
                std::vector<IndexObject> list = _pathfinder.getObjectsOnTheWay( destination );
                for ( const IndexObject & pair : list ) {
                    if ( objectValidator.isValid( pair.first ) && std::binary_search( _mapActionObjects.begin(), _mapActionObjects.end(), pair ) ) {
                        const double extraValue = valueStorage.value( pair, 0 ); // object is on the way, we don't loose any movement points.
                        if ( extraValue > 0 ) {
                            // There is no need to reduce the quality of the object even if the path has others.
                            value += extraValue;
                        }
                    }
                }
            }

            const Maps::Tiles & destinationTile = world.GetTiles( destination );

            // TODO: check nearby enemy heroes and distance to them instead of relying on region stats.
            const RegionStats & regionStats = _regions[destinationTile.GetRegion()];

            const bool isObjectReachableAtThisTurn = ( distance <= leftMovePoints );

            // Go into "coward" mode only if the threat is real. Equal by strength heroes rarely attack each other.
            if ( heroStrength * AI::ARMY_ADVANTAGE_SMALL < regionStats.highestThreat ) {
                switch ( type ) {
                case MP2::OBJ_CASTLE: {
                    const Castle * castle = world.getCastleEntrance( Maps::GetPoint( destination ) );
                    assert( castle != nullptr );
                    if ( castle == nullptr ) {
                        break;
                    }

                    if ( castle->GetColor() == hero.GetColor() ) {
                        // Friendly castles are always the priority so no penalty for them.
                        break;
                    }

                    if ( isObjectReachableAtThisTurn ) {
                        if ( castle->GetGarrisonStrength( &hero ) > heroStrength / 2 ) {
                            value -= dangerousTaskPenalty / 4;
                        }
                        else {
                            value -= dangerousTaskPenalty / 10;
                        }
                    }
                    else if ( castle->GetGarrisonStrength( &hero ) > heroStrength / 2 ) {
                        value -= dangerousTaskPenalty / 2;
                    }
                    else {
                        value -= dangerousTaskPenalty / 3;
                    }

                    break;
                }
                case MP2::OBJ_HEROES: {
                    const Heroes * anotherHero = destinationTile.GetHeroes();
                    assert( anotherHero != nullptr );
                    if ( anotherHero == nullptr ) {
                        break;
                    }

                    if ( anotherHero->GetColor() == hero.GetColor() ) {
                        if ( isObjectReachableAtThisTurn ) {
                            value -= dangerousTaskPenalty / 8;
                        }
                        else {
                            value -= dangerousTaskPenalty / 4;
                        }
                    }
                    else {
                        if ( isObjectReachableAtThisTurn ) {
                            value -= dangerousTaskPenalty / 8;
                        }
                        else {
                            value -= dangerousTaskPenalty / 2;
                        }
                    }

                    break;
                }
                default:
                    // It is better to avoid all other objects if the current hero is under a big threat.
                    value -= dangerousTaskPenalty;
                    break;
                }
            }

            if ( !isObjectReachableAtThisTurn ) {
                // Distant object which is out of reach for the current turn must have lower priority.
                distance = leftMovePoints + ( distance - leftMovePoints ) * 2;
            }

            value = ScaleWithDistance( value, distance, type );
        };

        // Set baseline target if it's a special role
        if ( hero.getAIRole() == Heroes::Role::COURIER ) {
            const int courierTarget = getCourierMainTarget( hero, _pathfinder, lowestPossibleValue );
            if ( courierTarget != -1 ) {
                // Anything with positive value can override the courier's main task (i.e. castle or mine capture on the way)
                maxPriority = 0;
                priorityTarget = courierTarget;
#ifdef WITH_DEBUG
                objectType = world.GetTiles( courierTarget ).GetObject();
#endif

                DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() << " is a courier with a main target tile at " << courierTarget )
            }
            else {
                // If there's nothing to do as a Courier reset the role
                hero.setAIRole( Heroes::Role::HUNTER );
            }
        }

        for ( const IndexObject & node : _mapActionObjects ) {
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
                getObjectValue( node.first, dist, value, static_cast<MP2::MapObjectType>( node.second ), useDimensionDoor );

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
        bool isTerritoryExpansion = false;
        const int fogDiscoveryTarget = _pathfinder.getFogDiscoveryTile( hero, isTerritoryExpansion );
        if ( fogDiscoveryTarget >= 0 ) {
            uint32_t distanceToFogDiscovery = _pathfinder.getDistance( fogDiscoveryTarget );

            // TODO: add logic to check fog discovery based on Dimension Door distance, not the nearest tile.
            bool useDimensionDoor = false;
            const uint32_t dimensionDoorDist = AIWorldPathfinder::calculatePathPenalty( _pathfinder.getDimensionDoorPath( hero, fogDiscoveryTarget ) );
            if ( dimensionDoorDist > 0 && ( distanceToFogDiscovery == 0 || dimensionDoorDist < distanceToFogDiscovery / 2 ) ) {
                distanceToFogDiscovery = dimensionDoorDist;
                useDimensionDoor = true;
            }

            if ( isTerritoryExpansion && fogDiscoveryValue < 0 ) {
                // This is actually a very useful fog discovery action which might lead to finding of new objects.
                // Increase the value of this action.
                fogDiscoveryValue /= 2;
            }

            getObjectValue( fogDiscoveryTarget, distanceToFogDiscovery, fogDiscoveryValue, MP2::OBJ_NONE, useDimensionDoor );
        }

        if ( priorityTarget != -1 ) {
            if ( fogDiscoveryTarget >= 0 && fogDiscoveryValue > maxPriority ) {
                priorityTarget = fogDiscoveryTarget;
                maxPriority = fogDiscoveryValue;
                DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() << ": selected target: " << priorityTarget << " value is " << maxPriority << " (fog discovery)" )
            }
            else {
                DEBUG_LOG( DBG_AI, DBG_INFO,
                           hero.GetName() << ": selected target: " << priorityTarget << " value is " << maxPriority << " (" << MP2::StringObject( objectType ) << ")" )
            }
        }
        else if ( !heroInPatrolMode ) {
            priorityTarget = fogDiscoveryTarget;
            DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() << " can't find an object. Scouting the fog of war at " << priorityTarget )
        }
        else {
            DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() << " is in Patrol mode. No movement is required." )
        }

        return priorityTarget;
    }

    void Normal::updatePriorityTargets( Heroes & hero, int32_t tileIndex, const MP2::MapObjectType objectType )
    {
        if ( objectType != MP2::OBJ_CASTLE && objectType != MP2::OBJ_HEROES ) {
            // Priorities are only for castles and heroes at the moment.
            return;
        }

        auto updateAttackPriorityTarget = [this, tileIndex, &hero, objectType]() {
            if ( objectType == MP2::OBJ_CASTLE ) {
                const Castle * castle = world.getCastleEntrance( Maps::GetPoint( tileIndex ) );
                if ( castle == nullptr ) {
                    // How is it possible?
                    assert( 0 );

                    removeEnemyArmies( tileIndex );
                    return;
                }

                if ( hero.isFriends( castle->GetColor() ) ) {
                    removeEnemyArmies( tileIndex );

                    updatePriorityForCastle( *castle );
                }
                else {
                    updatePriorityAttackTarget( hero.GetKingdom(), world.GetTiles( tileIndex ) );
                }
            }
            else if ( objectType == MP2::OBJ_HEROES ) {
                const Maps::Tiles & tile = world.GetTiles( tileIndex );

                const Heroes * anotherHero = tile.GetHeroes();
                if ( anotherHero == nullptr ) {
                    // The hero died.
                    removeEnemyArmies( tileIndex );
                    return;
                }

                if ( !hero.isFriends( anotherHero->GetColor() ) ) {
                    updatePriorityAttackTarget( hero.GetKingdom(), tile );
                }
            }
            else {
                // Unsupported object type!
                assert( 0 );
            }
        };

        auto it = _priorityTargets.find( tileIndex );
        if ( it == _priorityTargets.end() ) {
            // If the object is not a priority we have to update it after the battle as it can become the one.
            // Especially, when the opposite army has grown Skeletons or Ghosts.
            updateAttackPriorityTarget();

            // If the update did not add any priorities then nothing more to do.
            it = _priorityTargets.find( tileIndex );
            if ( it == _priorityTargets.end() ) {
                return;
            }
        }

        const PriorityTask & task = it->second;

        switch ( task.type ) {
        case PriorityTaskType::DEFEND:
        case PriorityTaskType::REINFORCE: {
            if ( objectType == MP2::OBJ_HEROES ) {
                // The castle has just been captured. No task should be updated.

                // If this assertion blows up then it is not the case described above.
                assert( ( world.GetTiles( tileIndex ).GetObject() == MP2::OBJ_CASTLE ) && ( hero.GetIndex() != tileIndex ) );
                return;
            }

            // These tasks are only for castles at the moment!
            assert( objectType == MP2::OBJ_CASTLE );

            // How is it even possible that a hero died while simply moving into a castle?
            assert( !hero.isFreeman() );

            // TODO: sort the army between the castle and hero to have maximum movement points for the next day
            // TODO: but also have enough army to defend the castle.

            hero.SetModes( Heroes::SLEEPER );
            _priorityTargets.erase( tileIndex );

            break;
        }
        case PriorityTaskType::ATTACK: {
            removePriorityAttackTarget( tileIndex );

            updateAttackPriorityTarget();
            break;
        }
        default:
            // Did you add a new type of priority task? Add the logic above!
            assert( 0 );
            break;
        }
    }

    void Normal::HeroesActionComplete( Heroes & hero, const int32_t tileIndex, const MP2::MapObjectType objectType )
    {
        // This method is called upon action completion and the hero could no longer be available.
        // So it is to check if the hero is still present.
        if ( !hero.isFreeman() ) {
            Castle * castle = hero.inCastleMutable();
            if ( castle ) {
                // Reinforcement in a castle can lead to removing defense priority task for a castle.
                reinforceHeroInCastle( hero, *castle, castle->GetKingdom().GetFunds() );
            }
        }

        if ( isMonsterStrengthCacheable( objectType ) ) {
            _neutralMonsterStrengthCache.erase( tileIndex );
        }

        updatePriorityTargets( hero, tileIndex, objectType );

        updateMapActionObjectCache( tileIndex );
    }

    bool Normal::HeroesTurn( VecHeroes & heroes, const uint32_t startProgressValue, const uint32_t endProgressValue )
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

        Interface::StatusWindow & status = Interface::AdventureMap::Get().getStatusWindow();

        uint32_t currentProgressValue = startProgressValue;

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

            int prevHeroPosition = bestHero->GetIndex();

            // check if we want to use Dimension Door spell or move regularly
            std::list<Route::Step> dimensionPath = _pathfinder.getDimensionDoorPath( *bestHero, bestTargetIndex );
            uint32_t dimensionDoorDistance = AIWorldPathfinder::calculatePathPenalty( dimensionPath );
            uint32_t moveDistance = _pathfinder.getDistance( bestTargetIndex );
            if ( dimensionDoorDistance && ( !moveDistance || dimensionDoorDistance < moveDistance / 2 ) ) {
                while ( ( !moveDistance || dimensionDoorDistance < moveDistance / 2 ) && !dimensionPath.empty() && bestHero->MayStillMove( false, false )
                        && bestHero->CanCastSpell( Spell::DIMENSIONDOOR ) ) {
                    HeroesCastDimensionDoor( *bestHero, dimensionPath.front().GetIndex() );
                    dimensionDoorDistance -= dimensionPath.front().GetPenalty();

                    _pathfinder.reEvaluateIfNeeded( *bestHero );
                    moveDistance = _pathfinder.getDistance( bestTargetIndex );

                    dimensionPath.pop_front();

                    // Hero can jump straight into the fog using the Dimension Door spell, which triggers the mechanics of fog revealing for his new tile
                    // and this results in inserting a new hero position into the action object cache. Perform the necessary updates.
                    assert( !bestHero->isFreeman() && bestHero->GetIndex() != prevHeroPosition );

                    updateMapActionObjectCache( prevHeroPosition );
                    updateMapActionObjectCache( bestHero->GetIndex() );

                    prevHeroPosition = bestHero->GetIndex();
                }

                if ( dimensionDoorDistance > 0 ) {
                    // The rest of the path the hero should do by foot.
                    bestHero->GetPath().setPath( _pathfinder.buildPath( bestTargetIndex ), bestTargetIndex );

                    HeroesMove( *bestHero );
                }
            }
            else {
                bestHero->GetPath().setPath( _pathfinder.buildPath( bestTargetIndex ), bestTargetIndex );

                HeroesMove( *bestHero );
            }

            if ( bestHero->isFreeman() || bestHero->GetIndex() != prevHeroPosition ) {
                // The hero died or moved to another position. We have to update the action object cache.
                updateMapActionObjectCache( prevHeroPosition );

                if ( !bestHero->isFreeman() ) {
                    // Hero moved to another position and is still alive.
                    updateMapActionObjectCache( bestHero->GetIndex() );
                }
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

            // The size of heroes can be increased if a new hero is released from Jail.
            const size_t maxHeroCount = std::max( heroes.size(), availableHeroes.size() );

            if ( maxHeroCount > 0 ) {
                // At least one hero still exist in the kingdom.
                const size_t progressValue = ( endProgressValue - startProgressValue ) * ( maxHeroCount - availableHeroes.size() ) / maxHeroCount + startProgressValue;
                if ( currentProgressValue < progressValue ) {
                    currentProgressValue = static_cast<uint32_t>( progressValue );
                    status.DrawAITurnProgress( currentProgressValue );
                }
            }
        }

        const bool allHeroesMoved = availableHeroes.empty();

        _pathfinder.setArmyStrengthMultiplier( originalMonsterStrengthMultiplier );
        _pathfinder.setSpellPointReserve( 0.5 );

        status.DrawAITurnProgress( endProgressValue );

        return allHeroesMoved;
    }
}
