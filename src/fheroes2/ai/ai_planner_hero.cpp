/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2024                                                    *
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
#include <functional>
#include <list>
#include <map>
#include <ostream>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "ai_common.h"
#include "ai_hero_action.h"
#include "ai_planner.h" // IWYU pragma: associated
#include "ai_planner_internals.h"
#include "army.h"
#include "army_troop.h"
#include "artifact.h"
#include "castle.h"
#include "color.h"
#include "difficulty.h"
#include "direction.h"
#include "game.h"
#include "game_interface.h"
#include "game_over.h"
#include "game_static.h"
#include "ground.h"
#include "heroes.h"
#include "heroes_base.h"
#include "interface_status.h"
#include "kingdom.h"
#include "logging.h"
#include "luck.h"
#include "maps.h"
#include "maps_fileinfo.h"
#include "maps_tiles.h"
#include "maps_tiles_helper.h"
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

        const Maps::FileInfo & mapInfo = Settings::Get().getCurrentMapInfo();

        if ( ( mapInfo.ConditionWins() & GameOver::WINS_ARTIFACT ) == 0 ) {
            return false;
        }

        if ( mapInfo.WinsFindUltimateArtifact() ) {
            return art.isUltimate();
        }

        return ( art.GetID() == mapInfo.WinsFindArtifactID() );
    }

    bool isCastleLossConditionForHuman( const Castle * castle )
    {
        assert( castle != nullptr );

        const Maps::FileInfo & mapInfo = Settings::Get().getCurrentMapInfo();
        const bool isSinglePlayer = ( Colors( Players::HumanColors() ).size() == 1 );

        if ( isSinglePlayer && ( mapInfo.ConditionLoss() & GameOver::LOSS_TOWN ) != 0 && castle->GetCenter() == mapInfo.LossMapsPositionObject() ) {
            // It is a loss town condition for human.
            return true;
        }

        if ( mapInfo.WinsCompAlsoWins() && ( mapInfo.ConditionWins() & GameOver::WINS_TOWN ) != 0 && castle->GetCenter() == mapInfo.WinsMapsPositionObject() ) {
            // It is a town capture winning condition for AI.
            return true;
        }

        return false;
    }

    bool shouldUseDimensionDoor( const uint32_t regularMovementDist, const uint32_t dimensionDoorDist )
    {
        if ( dimensionDoorDist == 0 ) {
            return false;
        }

        return ( regularMovementDist == 0 || dimensionDoorDist < regularMovementDist / 2 );
    }

    // Returns a pair consisting of a distance and a boolean value set to true if this distance was calculated
    // for the case of movement using the Dimension Door spell, otherwise this value is set to false
    std::pair<uint32_t, bool> getDistanceToTile( AIWorldPathfinder & pathfinder, const int32_t index )
    {
        const uint32_t regularMovementDist = pathfinder.getDistance( index );

        const std::list<Route::Step> dimensionDoorPath = pathfinder.buildDimensionDoorPath( index );
        if ( dimensionDoorPath.empty() ) {
            return { regularMovementDist, false };
        }

        const uint32_t dimensionDoorDist = Route::calculatePathPenalty( dimensionDoorPath );
        assert( dimensionDoorDist > 0 );

        if ( shouldUseDimensionDoor( regularMovementDist, dimensionDoorDist ) ) {
            return { dimensionDoorDist, true };
        }

        return { regularMovementDist, false };
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
        const double castleStrength = castle->GetGarrisonStrength( hero ) * advantage;

        return heroArmyStrength > castleStrength;
    }

    bool isHeroStrongerThan( const Maps::Tile & tile, AI::Planner & ai, const double heroArmyStrength, const double targetStrengthMultiplier )
    {
        return heroArmyStrength > ai.getTileArmyStrength( tile ) * targetStrengthMultiplier;
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

    bool isArmyValuableToHire( const Army & army, const Kingdom & kingdom, const Maps::Tile & tile, const double armyStrengthThreshold )
    {
        const Troop & troop = getTroopFromTile( tile );
        if ( !troop.isValid() ) {
            return false;
        }

        const bool armyHasMonster = army.HasMonster( troop.GetMonster() );
        if ( !armyHasMonster && army.isFullHouse() && army.areAllTroopsUnique() ) {
            return false;
        }

        const Funds singleMonsterCost = troop.GetCost();

        uint32_t recruitTroopCount = kingdom.GetFunds().getLowestQuotient( singleMonsterCost );
        if ( recruitTroopCount == 0 ) {
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

    bool HeroesValidObject( const Heroes & hero, const double heroArmyStrength, const int32_t index, AIWorldPathfinder & pathfinder, AI::Planner & ai,
                            const double armyStrengthThreshold, const bool underHero )
    {
        const Maps::Tile & tile = world.getTile( index );
        const MP2::MapObjectType objectType = tile.getMainObjectType( !underHero );

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
            return !hero.isObjectTypeVisited( objectType, Visit::GLOBAL ) && kingdom.AllowPayment( PaymentConditions::getMagellansMapsPurchasePrice() );

        case MP2::OBJ_ALCHEMIST_LAB:
        case MP2::OBJ_LIGHTHOUSE:
        case MP2::OBJ_MINE:
        case MP2::OBJ_SAWMILL:
            if ( !hero.isFriends( getColorFromTile( tile ) ) ) {
                if ( isCaptureObjectProtected( tile ) ) {
                    return isHeroStrongerThan( tile, ai, heroArmyStrength, AI::ARMY_ADVANTAGE_SMALL );
                }

                return true;
            }
            break;

        case MP2::OBJ_ABANDONED_MINE:
            return isHeroStrongerThan( tile, ai, heroArmyStrength, AI::ARMY_ADVANTAGE_LARGE );

        case MP2::OBJ_LEAN_TO:
        case MP2::OBJ_MAGIC_GARDEN:
        case MP2::OBJ_SKELETON:
        case MP2::OBJ_WAGON:
        case MP2::OBJ_WATER_WHEEL:
        case MP2::OBJ_WINDMILL:
            return doesTileContainValuableItems( tile );

        case MP2::OBJ_ARTIFACT: {
            if ( hero.IsFullBagArtifacts() ) {
                return false;
            }

            const Artifact art = getArtifactFromTile( tile );
            assert( art.isValid() );

            if ( art.GetID() == Artifact::MAGIC_BOOK && hero.HaveSpellBook() ) {
                return false;
            }

            const Maps::ArtifactCaptureCondition condition = getArtifactCaptureCondition( tile );

            if ( condition == Maps::ArtifactCaptureCondition::PAY_2000_GOLD || condition == Maps::ArtifactCaptureCondition::PAY_2500_GOLD_AND_3_RESOURCES
                 || condition == Maps::ArtifactCaptureCondition::PAY_3000_GOLD_AND_5_RESOURCES ) {
                return kingdom.AllowPayment( getArtifactResourceRequirement( tile ) );
            }

            if ( condition == Maps::ArtifactCaptureCondition::HAVE_WISDOM_SKILL || condition == Maps::ArtifactCaptureCondition::HAVE_LEADERSHIP_SKILL ) {
                return hero.HasSecondarySkill( getArtifactSecondarySkillRequirement( tile ).Skill() );
            }

            if ( condition >= Maps::ArtifactCaptureCondition::FIGHT_50_ROGUES && condition <= Maps::ArtifactCaptureCondition::FIGHT_1_BONE_DRAGON ) {
                return isHeroStrongerThan( tile, ai, heroArmyStrength, AI::ARMY_ADVANTAGE_LARGE );
            }

            // No conditions to capture an artifact exist.
            return true;
        }

        case MP2::OBJ_OBSERVATION_TOWER: {
            const int32_t fogCountToUncoverByTower
                = Maps::getFogTileCountToBeRevealed( index, GameStatic::getFogDiscoveryDistance( GameStatic::FogDiscoveryType::OBSERVATION_TOWER ), hero.GetColor() );

            return fogCountToUncoverByTower > 0;
        }

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

            if ( !hero.HaveSpellBook() ) {
                return false;
            }

            if ( spell.Level() == 3 && hero.GetLevelSkill( Skill::Secondary::WISDOM ) == Skill::Level::NONE ) {
                // No reason to visit this shrine as the hero cannot learn this spell.
                return false;
            }

            if ( !hero.isVisited( tile, Visit::GLOBAL ) ) {
                // This shrine has not been visited by any hero. It's worth to do it.
                return true;
            }

            if ( hero.HaveSpell( spell, true ) || !isSpellUsedByAI( spell.GetID() ) ) {
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
            if ( !skill.isValid() ) {
                // How is it possible that no skill exist?
                assert( 0 );
                return false;
            }

            if ( hero.HasMaxSecondarySkill() ) {
                return false;
            }

            if ( !hero.isVisited( tile, Visit::GLOBAL ) ) {
                // The AI heroes should not have prior knowledge what skill this object has.
                return true;
            }

            if ( hero.HasSecondarySkill( skillType ) ) {
                return false;
            }

            if ( army.AllTroopsAreUndead() && skillType == Skill::Secondary::LEADERSHIP ) {
                // For undead army it's pointless to have Leadership skill.
                return false;
            }

            if ( !hero.HaveSpellBook() && ( skillType == Skill::Secondary::MYSTICISM || skillType == Skill::Secondary::EAGLE_EYE ) ) {
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

            const auto [distance, dummy] = getDistanceToTile( pathfinder, index );
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

            const auto [distance, dummy] = getDistanceToTile( pathfinder, index );
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
            return !world.isAnyKingdomVisited( objectType, index ) && hero.HaveSpellBook() && hero.GetSpellPoints() < 2 * hero.GetMaxSpellPoints();

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
        case MP2::OBJ_WATER_ALTAR:
            return isArmyValuableToHire( army, kingdom, tile, armyStrengthThreshold );

        // Dwellings where AI might fight monsters first before recruiting them.
        case MP2::OBJ_CITY_OF_DEAD:
        case MP2::OBJ_DRAGON_CITY:
        case MP2::OBJ_TROLL_BRIDGE: {
            if ( Color::NONE == getColorFromTile( tile ) ) {
                return isHeroStrongerThan( tile, ai, heroArmyStrength, AI::ARMY_ADVANTAGE_MEDIUM );
            }

            return isArmyValuableToHire( army, kingdom, tile, armyStrengthThreshold );
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

            if ( hero.isObjectTypeVisited( objectType ) ) {
                return false;
            }

            const auto [distance, dummy] = getDistanceToTile( pathfinder, index );
            if ( distance == 0 ) {
                return false;
            }

            const int daysActive = numOfDaysPerWeek - world.GetDay() + 1;
            const double movementBonus = daysActive * GameStatic::getMovementPointBonus( objectType ) - 2.0 * distance;

            return movementBonus > 0;
        }

        // Objects that give goods but curse with bad morale when visiting them for subsequent times.
        case MP2::OBJ_DERELICT_SHIP:
        case MP2::OBJ_GRAVEYARD:
        case MP2::OBJ_SHIPWRECK:
            if ( !hero.isVisited( tile, Visit::GLOBAL ) && doesTileContainValuableItems( tile ) ) {
                Army enemy( tile );
                return enemy.isValid() && isHeroStrongerThan( tile, ai, heroArmyStrength, 2 );
            }
            break;

        case MP2::OBJ_PYRAMID:
            if ( !hero.isVisited( tile, Visit::GLOBAL ) && doesTileContainValuableItems( tile ) ) {
                Army enemy( tile );
                return enemy.isValid() && Skill::Level::EXPERT == hero.GetLevelSkill( Skill::Secondary::WISDOM )
                       && isHeroStrongerThan( tile, ai, heroArmyStrength, AI::ARMY_ADVANTAGE_LARGE );
            }
            break;

        case MP2::OBJ_DAEMON_CAVE:
            if ( doesTileContainValuableItems( tile ) ) {
                // AI always chooses to fight the demon's servants and doesn't roll the dice
                return isHeroStrongerThan( tile, ai, heroArmyStrength, AI::ARMY_ADVANTAGE_MEDIUM );
            }
            break;

        case MP2::OBJ_MONSTER:
            return isHeroStrongerThan( tile, ai, heroArmyStrength, ( hero.isLosingGame() ? 1.0 : AI::ARMY_ADVANTAGE_MEDIUM ) );

        case MP2::OBJ_HERO: {
            const Heroes * otherHero = tile.getHero();
            assert( otherHero != nullptr );

            const bool otherHeroInCastle = ( otherHero->inCastle() != nullptr );

            if ( hero.GetColor() == otherHero->GetColor() ) {
                if ( hero.hasMetWithHero( otherHero->GetID() ) ) {
                    return false;
                }

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

            return army.isStrongerThan( otherHero->GetArmy(), hero.isLosingGame() ? AI::ARMY_ADVANTAGE_DESPERATE : AI::ARMY_ADVANTAGE_SMALL );
        }

        case MP2::OBJ_CASTLE:
            return AIShouldVisitCastle( hero, index, heroArmyStrength );

        case MP2::OBJ_JAIL:
            return kingdom.GetHeroes().size() < Kingdom::GetMaxHeroes();
        case MP2::OBJ_HUT_OF_MAGI:
            return !hero.isObjectTypeVisited( objectType, Visit::GLOBAL ) && Maps::doesObjectExistOnMap( MP2::OBJ_EYE_OF_MAGI );

        case MP2::OBJ_ALCHEMIST_TOWER: {
            const BagArtifacts & bag = hero.GetBagArtifacts();
            const uint32_t cursed = static_cast<uint32_t>( std::count_if( bag.begin(), bag.end(), []( const Artifact & art ) { return art.containsCurses(); } ) );
            if ( cursed == 0 ) {
                return false;
            }

            const Funds payment = PaymentConditions::ForAlchemist();
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

    void addHeroToMove( Heroes * hero, std::vector<Heroes *> & availableHeroes )
    {
        if ( hero->Modes( Heroes::PATROL ) && ( hero->GetPatrolDistance() == 0 ) ) {
            DEBUG_LOG( DBG_AI, DBG_TRACE, hero->GetName() << " stands still due to the patrol settings" )
            return;
        }

        if ( hero->MayStillMove( false, false ) ) {
            availableHeroes.emplace_back( hero );
        }
    }

    // Used for caching object validations per hero.
    class ObjectValidator
    {
    public:
        explicit ObjectValidator( const Heroes & hero, AIWorldPathfinder & pathfinder, AI::Planner & ai )
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

            const bool valid = HeroesValidObject( _hero, _heroArmyStrength, index, _pathfinder, _ai, _armyStrengthThreshold, false );
            _validObjects[index] = valid;
            return valid;
        }

    private:
        const Heroes & _hero;
        AIWorldPathfinder & _pathfinder;
        AI::Planner & _ai;

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
        ObjectValueStorage( const Heroes & hero, const AI::Planner & ai, const double ignoreValue )
            : _hero( hero )
            , _ai( ai )
            , _ignoreValue( ignoreValue )
        {}

        double value( const IndexObject & objectInfo, const uint32_t distance )
        {
            const auto [iter, inserted] = _objectValue.try_emplace( objectInfo, 0.0 );
            if ( inserted ) {
                iter->second = _ai.getObjectValue( _hero, objectInfo.first, objectInfo.second, _ignoreValue, distance );
            }

            return iter->second;
        }

    private:
        const Heroes & _hero;
        const AI::Planner & _ai;
        const double _ignoreValue;
        std::map<IndexObject, double> _objectValue;
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

    // TODO: consider making this penalty dynamic
    const double dangerousTaskPenalty = 50000.0;
    const double fogDiscoveryBaseValue = -10000.0;

    double getDistanceModifier( const MP2::MapObjectType objectType )
    {
        // The value above 1.0 means that the object is useful only if it is nearby.
        // The value below 1.0 means that the object should remain in focus even on greater distances.
        if ( !MP2::isInGameActionObject( objectType ) ) {
            return 1.0;
        }

        switch ( objectType ) {
        case MP2::OBJ_CASTLE:
            return 0.8;
        case MP2::OBJ_ALCHEMIST_LAB:
        case MP2::OBJ_ARTIFACT:
        case MP2::OBJ_HERO:
        case MP2::OBJ_MINE:
        case MP2::OBJ_SAWMILL:
            return 0.9;
        case MP2::OBJ_CAMPFIRE:
        case MP2::OBJ_FLOTSAM:
        case MP2::OBJ_GENIE_LAMP:
        case MP2::OBJ_RESOURCE:
        case MP2::OBJ_SEA_CHEST:
        case MP2::OBJ_TREASURE_CHEST:
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

    double scaleWithDistanceAndTime( const double value, const uint32_t distance, const MP2::MapObjectType objectType )
    {
        if ( distance == 0 ) {
            return value;
        }

        // Some objects do not loose their value drastically over distances. This allows AI heroes to keep focus on important targets.
        double correctedDistance = distance * getDistanceModifier( objectType );

        // Distances should be corrected over time as AI heroes should focus on keeping important objects in focus.
        // This value shouldn't be big but very slowly increases over time.
        const double timeCorrectionCoeff = 1 - std::min( 0.5, world.CountDay() * 0.0001 );
        correctedDistance *= timeCorrectionCoeff;

        // Scale non-linearly (more value lost as distance increases)
        return value - ( correctedDistance * std::log10( correctedDistance ) );
    }

    double getFogDiscoveryValue( const Heroes & hero )
    {
        switch ( hero.getAIRole() ) {
        case Heroes::Role::SCOUT:
            // Scouts do not have any penalty on map exploration as it is their main priority.
            return 0;
        case Heroes::Role::HUNTER:
        case Heroes::Role::FIGHTER:
        case Heroes::Role::CHAMPION:
            return fogDiscoveryBaseValue;
        case Heroes::Role::COURIER:
            return fogDiscoveryBaseValue * 2;
        default:
            // If you set a new type of a hero you must add the logic here.
            assert( 0 );
            break;
        }

        return fogDiscoveryBaseValue;
    }

    double getSecondarySkillValue( const Heroes & hero, const Skill::Secondary & skill )
    {
        const int type = skill.Skill();
        const int level = skill.Level();
        if ( hero.GetLevelSkill( type ) >= level ) {
            return 0;
        }

        switch ( type ) {
        case Skill::Secondary::WISDOM:
            // wouldn't check castles/spell availability since high wisdom drives building priority
            return level == Skill::Level::BASIC ? 2500.0 : 1000.0;
        case Skill::Secondary::LOGISTICS:
            return 1500.0;
        case Skill::Secondary::LEADERSHIP:
            return hero.GetArmy().AllTroopsAreUndead() ? 100.0 : 1000.0;
        case Skill::Secondary::NECROMANCY:
            return hero.GetArmy().AllTroopsAreUndead() ? 1000.0 : 100.0;
        case Skill::Secondary::LUCK: {
            const Heroes::Role role = hero.getAIRole();
            if ( role == Heroes::Role::COURIER || role == Heroes::Role::SCOUT ) {
                return 100.0;
            }
            return 500.0;
        }
        case Skill::Secondary::BALLISTICS: {
            const Heroes::Role role = hero.getAIRole();
            if ( role == Heroes::Role::COURIER || role == Heroes::Role::SCOUT ) {
                return 100.0;
            }
            return hero.GetArmy().isMeleeDominantArmy() ? 1250.0 : 250.0;
        }
        case Skill::Secondary::ARCHERY: {
            const Heroes::Role role = hero.getAIRole();
            if ( role == Heroes::Role::COURIER || role == Heroes::Role::SCOUT ) {
                return 100.0;
            }
            return hero.GetArmy().isMeleeDominantArmy() ? 100.0 : 500.0;
        }
        case Skill::Secondary::ESTATES: {
            const Heroes::Role role = hero.getAIRole();
            if ( role == Heroes::Role::CHAMPION || role == Heroes::Role::FIGHTER ) {
                return 0.0;
            }
            return 1000.0;
        }
        case Skill::Secondary::PATHFINDING: {
            const double roughness = world.getLandRoughness();
            return ( roughness > 1.25 ) ? 1000.0 : ( roughness > 1.1 ) ? 250.0 : 100.0;
        }
        case Skill::Secondary::NAVIGATION: {
            const uint8_t waterPercentage = world.getWaterPercentage();
            return ( waterPercentage > 60 ) ? 1000.0 : ( waterPercentage > 25 ) ? 100.0 : 0.0;
        }
        case Skill::Secondary::SCOUTING: {
            const Heroes::Role role = hero.getAIRole();
            if ( role == Heroes::Role::CHAMPION || role == Heroes::Role::FIGHTER ) {
                return 0.0;
            }
            return hero.getAIRole() == Heroes::Role::SCOUT ? 1250.0 : 100.0;
        }
        case Skill::Secondary::MYSTICISM:
            return hero.HaveSpellBook() ? 500.0 : 100.0;
        case Skill::Secondary::EAGLE_EYE:
            return hero.HaveSpellBook() ? 250.0 : 0.0;
        case Skill::Secondary::DIPLOMACY:
            // discourage AI picking it up, but if it's already there prioritise leveling to save gold
            return level == Skill::Level::BASIC ? 100.0 : 1250.0;
        case Skill::Secondary::UNKNOWN:
            return 0;
        default:
            // If you make a new secondary skill don't forget to update this.
            assert( 0 );
            break;
        }
        return 0;
    }

    uint32_t getTimeoutBeforeFogDiscoveryIntensification( const Heroes & hero )
    {
        switch ( hero.getAIRole() ) {
        case Heroes::Role::SCOUT:
            return 30;
        case Heroes::Role::HUNTER:
            return 90;
        case Heroes::Role::FIGHTER:
        case Heroes::Role::CHAMPION:
            return 60;
        case Heroes::Role::COURIER:
            return 120;
        default:
            // If you set a new type of a hero you must add the logic here.
            assert( 0 );
            break;
        }

        return 30;
    }
}

// TODO: In the future we need to come up with dynamic object value estimation based not only on a hero's role but on an outcome from movement at certain position.
double AI::Planner::getGeneralObjectValue( const Heroes & hero, const int32_t index, const double valueToIgnore, const uint32_t distanceToObject ) const
{
    // In the future these hardcoded values could be configured by the mod
    // 1 tile distance is 100.0 value approximately
    const Maps::Tile & tile = world.getTile( index );
    const MP2::MapObjectType objectType = tile.getMainObjectType();

    const std::function<double( const Castle * )> calculateCastleValue = [this, &hero, &calculateCastleValue]( const Castle * castle ) {
        assert( castle != nullptr );

        double value = castle->getBuildingValue() * 150.0 + 3000;

        if ( hero.isLosingGame() ) {
            value += 15000;
        }

        if ( isCastleLossConditionForHuman( castle ) ) {
            value += 20000;
        }

        // This modifier shouldn't be too high to avoid players baiting AI in
        const double defenselessCastleModifier = 1.25;

        // This is our own castle in need of protection, evaluate it no worse than a similar defenseless enemy castle
        if ( hero.GetColor() == castle->GetColor() ) {
            value *= defenselessCastleModifier;
        }
        else {
            const int32_t castleIndex = castle->GetIndex();

            // If this castle threatens our castles, then we should evaluate it no worse than the best of our castles that it threatens
            // (as if our best castle had to be taken back from the enemy). If a threatening castle can be attacked right now, it is
            // better to do it than wait until the hero hired in it captures one of our castles.
            if ( isCriticalTask( castleIndex ) ) {
                const auto iter = _priorityTargets.find( castleIndex );
                assert( iter != _priorityTargets.end() );

                const PriorityTask & attackTask = iter->second;
                assert( attackTask.type == PriorityTaskType::ATTACK );

                for ( const int32_t secondaryTaskTileIdx : attackTask.secondaryTaskTileId ) {
                    const Castle * castleUnderThreat = world.getCastleEntrance( Maps::GetPoint( secondaryTaskTileIdx ) );
                    if ( castleUnderThreat == nullptr ) {
                        continue;
                    }

                    assert( hero.GetColor() == castleUnderThreat->GetColor() );

                    // Apply a bonus so that the AI prefers to eliminate the threat if possible instead of guarding its castle
                    value = std::max( value, calculateCastleValue( castleUnderThreat ) * 2 );
                }
            }

            // This castle is defenseless
            if ( !castle->GetActualArmy().isValid() ) {
                value *= defenselessCastleModifier;
            }
        }

        return value;
    };

    switch ( objectType ) {
    case MP2::OBJ_CASTLE: {
        const Castle * castle = world.getCastleEntrance( Maps::GetPoint( index ) );
        if ( !castle ) {
            // How is it even possible?
            assert( 0 );
            return valueToIgnore;
        }

        if ( hero.GetColor() == castle->GetColor() ) {
            // If our castle is in danger, then we should evaluate it not from the point of view of momentary benefits
            // for the hero (for example, the presence of reinforcements), but from the point of view of the castle,
            // which then will have to be taken back from the enemy, that is, according to approximately the same logic
            // as enemy castles
            if ( isCriticalTask( index ) ) {
                return calculateCastleValue( castle );
            }

            double value = castle->getVisitValue( hero );

            if ( !isPriorityTask( index ) && value < 500 ) {
                return valueToIgnore;
            }

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

        return calculateCastleValue( castle );
    }
    case MP2::OBJ_HERO: {
        const Heroes * otherHero = tile.getHero();
        if ( !otherHero ) {
            // How is it even possible?
            assert( 0 );
            return valueToIgnore;
        }

        if ( hero.GetColor() == otherHero->GetColor() ) {
            if ( hero.getAIRole() > otherHero->getAIRole() ) {
                // The other hero has a lower role. Do not waste time for meeting. Let him to come.
                return valueToIgnore;
            }

            if ( hero.getAIRole() == otherHero->getAIRole()
                 && hero.getStatsValue() + Difficulty::getMinStatDiffForAIHeroesMeeting( Game::getDifficulty() ) > otherHero->getStatsValue() ) {
                // Either this hero is superior or approximately equal to another hero in terms of stats. Do not waste time for meeting. Let him come instead.
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

        double value = 5000;

        // If this hero threatens our castles, then we should evaluate him no worse than the best of our castles that he threatens
        // (as if it had to be taken back from the enemy). If a threatening hero can be attacked in an open field, it is better to
        // do it now than to attack him later in our castle occupied by him.
        if ( isCriticalTask( index ) ) {
            const auto iter = _priorityTargets.find( index );
            assert( iter != _priorityTargets.end() );

            const PriorityTask & attackTask = iter->second;
            assert( attackTask.type == PriorityTaskType::ATTACK );

            for ( const int32_t secondaryTaskTileIdx : attackTask.secondaryTaskTileId ) {
                const Castle * castle = world.getCastleEntrance( Maps::GetPoint( secondaryTaskTileIdx ) );
                if ( castle == nullptr ) {
                    continue;
                }

                assert( hero.GetColor() == castle->GetColor() );

                // Apply a bonus so that the AI prefers to eliminate the threat if possible instead of guarding its castle
                value = std::max( value, calculateCastleValue( castle ) * 2 );
            }
        }
        else {
            const Castle * castle = otherHero->inCastle();

            if ( castle != nullptr ) {
                // If the enemy hero is in the castle, then after defeating this hero, AI will automatically capture his
                // castle, so it's worth adding the value of the castle to the value of the enemy hero
                value += calculateCastleValue( castle );
            }
            else if ( otherHero->GetControl() == CONTROL_AI ) {
                // AI heroes should not attack other AI heroes so aggressively as human heroes.
                // This is done to avoid situations when human players just wait when AI heroes kill each other.
                value *= 0.8;
            }
        }

        return value;
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
    case MP2::OBJ_MINE:
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
                return valueToIgnore; // don't even attempt to go here
            }

            std::tie( resourceType, resourceAmount ) = getDailyIncomeObjectResources( tile ).getFirstValidResource();
        }

        assert( resourceType != Resource::UNKNOWN && resourceAmount != 0 );

        // Since mines constantly bring resources, they are valuable objects
        return resourceAmount * getResourcePriorityModifier( resourceType, true );
    }
    case MP2::OBJ_ARTIFACT:
    case MP2::OBJ_SHIPWRECK_SURVIVOR: {
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
    case MP2::OBJ_TREASURE_CHEST: {
        const Artifact art = getArtifactFromTile( tile );
        if ( art.isValid() && isFindArtifactVictoryConditionForHuman( art ) ) {
            // WINS_ARTIFACT victory condition does not apply to AI-controlled players, we should leave this object untouched for the human player.
            assert( 0 );
            return -dangerousTaskPenalty;
        }

        // This is an average gold amount you can get from a treasure chest or sea chest.
        return getFundsValueBasedOnPriority( { Resource::GOLD, 1500 } );
    }
    case MP2::OBJ_DAEMON_CAVE: {
        // If this cave is already empty, then we should never come here
        if ( !doesTileContainValuableItems( tile ) ) {
            assert( 0 );
            return -dangerousTaskPenalty;
        }

        // Daemon Cave always gives 2500 Gold after a battle and AI always chooses to fight the demon's servants and doesn't roll the dice
        return getFundsValueBasedOnPriority( { Resource::GOLD, 2500 } );
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
    case MP2::OBJ_CAMPFIRE: {
        // A campfire has 4-6 random resources plus 400-600 gold so we use an average to get evaluation.
        // Since we should not expose the resource type let's assume that it gives 6 resources, 1 each and 400 gold.
        const Funds loot{ 1, 1, 1, 1, 1, 1, 400 };

        const double value = getFundsValueBasedOnPriority( loot );
        assert( value > 0 );

        return value;
    }

    case MP2::OBJ_MAGIC_GARDEN: {
        // Magic Garden has visual representation whether it was visited so verify that is has resources.
        assert( doesTileContainValuableItems( tile ) );

        // Magic Garden can have either 5 gems or 500 gold. Since, 5 is not divisible by 2, let's put 3 instead.
        const Funds loot{ 0, 0, 0, 0, 0, 3, 250 };

        const double value = getFundsValueBasedOnPriority( loot );
        assert( value > 0 );

        return value;
    }

    case MP2::OBJ_RESOURCE: {
        const Funds loot = getFundsFromTile( tile );

        // Resource object has visual representation of the type of resources.
        const auto & [resourceType, resourceCount] = loot.getFirstValidResource();

        Funds estimatedResource;

        switch ( resourceType ) {
        case Resource::GOLD:
            estimatedResource = Funds( resourceType, 750 );
            break;
        case Resource::WOOD:
        case Resource::ORE:
            estimatedResource = Funds( resourceType, 7 );
            break;
        case Resource::MERCURY:
        case Resource::SULFUR:
        case Resource::CRYSTAL:
        case Resource::GEMS:
            estimatedResource = Funds( resourceType, 4 );
            break;
        default:
            assert( 0 );
            break;
        }

        const double value = getFundsValueBasedOnPriority( estimatedResource );
        assert( value > 0 );

        return value;
    }

    case MP2::OBJ_DERELICT_SHIP:
    case MP2::OBJ_LEAN_TO:
    case MP2::OBJ_WATER_WHEEL:
    case MP2::OBJ_WINDMILL: {
        const Funds loot = getFundsFromTile( tile );

        const double value = getFundsValueBasedOnPriority( loot );

        // This object could have already been visited
        if ( value < 1 ) {
            return valueToIgnore;
        }

        return value;
    }

    case MP2::OBJ_FLOTSAM: {
        // Flotsam has the following chances and resources:
        // - 25%: 500 gold + 10 wood
        // - 25%: 200 gold + 5 wood
        // - 25%: 5 wood
        // - 25%: empty
        // Therefore, use an average value as an estimation of a possible reward of picking this object.
        // The AI must not know precise reward from this object as it would be called cheating.
        // For all cases we have: 700 gold and 20 wood. So we divide by 4 to get an average number of resources.
        // In total, we should use 175 gold and 5 wood.
        const Funds loot{ 0, 5, 0, 0, 0, 0, 175 };

        const double value = getFundsValueBasedOnPriority( loot );
        assert( value > 0 );

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
    // These are dwelling where monsters can be purchased.
    case MP2::OBJ_AIR_ALTAR:
    case MP2::OBJ_BARROW_MOUNDS:
    case MP2::OBJ_CITY_OF_DEAD:
    case MP2::OBJ_DESERT_TENT:
    case MP2::OBJ_DRAGON_CITY:
    case MP2::OBJ_EARTH_ALTAR:
    case MP2::OBJ_FIRE_ALTAR:
    case MP2::OBJ_GENIE_LAMP:
    case MP2::OBJ_RUINS:
    case MP2::OBJ_TREE_CITY:
    case MP2::OBJ_TROLL_BRIDGE:
    case MP2::OBJ_WAGON_CAMP:
    case MP2::OBJ_WATER_ALTAR: {
        const Troop & troop = getTroopFromTile( tile );
        if ( !troop.isValid() ) {
            // This should not happen! The dwelling must have monsters to reach this code.
            assert( 0 );
            return -dangerousTaskPenalty;
        }

        const Funds singleMonsterCost = troop.GetCost();

        uint32_t recruitTroopCount = hero.GetKingdom().GetFunds().getLowestQuotient( singleMonsterCost );
        if ( recruitTroopCount == 0 ) {
            // This can actually happen when AI spends some money in between visiting objects.
            return -dangerousTaskPenalty;
        }

        const uint32_t availableTroopCount = troop.GetCount();
        if ( recruitTroopCount > availableTroopCount ) {
            recruitTroopCount = availableTroopCount;
        }

        return Troop{ troop.GetID(), recruitTroopCount }.GetStrength();
    }
    // These are dwellings where monsters can be hired for free.
    case MP2::OBJ_ARCHER_HOUSE:
    case MP2::OBJ_CAVE:
    case MP2::OBJ_DWARF_COTTAGE:
    case MP2::OBJ_EXCAVATION:
    case MP2::OBJ_GOBLIN_HUT:
    case MP2::OBJ_HALFLING_HOLE:
    case MP2::OBJ_PEASANT_HUT:
    case MP2::OBJ_TREE_HOUSE:
    case MP2::OBJ_WATCH_TOWER: {
        return getTroopFromTile( tile ).GetStrength();
    }
    case MP2::OBJ_STONE_LITHS: {
        // Stone lith is not considered by AI as an action object. If this assertion blows up something is wrong with the logic.
        assert( 0 );
        return -dangerousTaskPenalty;
    }
    case MP2::OBJ_OBSERVATION_TOWER: {
        const int32_t fogCountToUncoverByTower
            = Maps::getFogTileCountToBeRevealed( index, GameStatic::getFogDiscoveryDistance( GameStatic::FogDiscoveryType::OBSERVATION_TOWER ), hero.GetColor() );

        if ( fogCountToUncoverByTower == 0 ) {
            // Nothing to uncover.
            return -dangerousTaskPenalty;
        }
        return fogCountToUncoverByTower;
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
        const int daysActive = numOfDaysPerWeek - world.GetDay() + 1;
        double movementBonus = daysActive * GameStatic::getMovementPointBonus( objectType ) - 2.0 * distanceToObject;

        const double upgradeValue = getMonsterUpgradeValue( hero.GetArmy(), Monster::CHAMPION );
        if ( upgradeValue > 0.0001 ) {
            // Even if no movement bonus exist it is worth to visit just to upgrade monsters.
            return std::max( movementBonus, 0.0 ) + freeMonsterUpgradeModifier * upgradeValue;
        }

        if ( movementBonus < 0 ) {
            // Don't waste movement to even visit this object.
            return -dangerousTaskPenalty;
        }

        return movementBonus;
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
        const double movementBonus = GameStatic::getMovementPointBonus( objectType ) - 2.0 * distanceToObject;
        if ( movementBonus < 0 ) {
            // Don't waste movement to even visit this object.
            return -dangerousTaskPenalty;
        }

        return movementBonus;
    }
    case MP2::OBJ_JAIL: {
        // A free hero is always good and he could be very powerful.
        return 3000;
    }
    case MP2::OBJ_HUT_OF_MAGI: {
        const auto & eyeMagiIndexes = world.getAllEyeOfMagiPositions();
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
        assert( !MP2::isInGameActionObject( objectType ) );
        break;
    }

    return 0;
}

double AI::Planner::getFighterObjectValue( const Heroes & hero, const int32_t index, const double valueToIgnore, const uint32_t distanceToObject ) const
{
    // Fighters have higher priority for battles and smaller values for other objects.
    assert( hero.getAIRole() == Heroes::Role::FIGHTER || hero.getAIRole() == Heroes::Role::CHAMPION );

    const Maps::Tile & tile = world.getTile( index );
    const MP2::MapObjectType objectType = tile.getMainObjectType();

    const std::function<double( const Castle * )> calculateCastleValue = [this, &hero, &calculateCastleValue]( const Castle * castle ) {
        assert( castle != nullptr );

        double value = castle->getBuildingValue() * 500.0 + 15000;

        if ( hero.isLosingGame() ) {
            value += 15000;
        }

        if ( isCastleLossConditionForHuman( castle ) ) {
            value += 20000;
        }

        // This modifier shouldn't be too high to avoid players baiting AI in
        const double defenselessCastleModifier = 1.5;

        // This is our own castle in need of protection, evaluate it no worse than a similar defenseless enemy castle
        if ( hero.GetColor() == castle->GetColor() ) {
            value *= defenselessCastleModifier;
        }
        else {
            const int32_t castleIndex = castle->GetIndex();

            // If this castle threatens our castles, then we should evaluate it no worse than the best of our castles that it threatens
            // (as if our best castle had to be taken back from the enemy). If a threatening castle can be attacked right now, it is
            // better to do it than wait until the hero hired in it captures one of our castles.
            if ( isCriticalTask( castleIndex ) ) {
                const auto iter = _priorityTargets.find( castleIndex );
                assert( iter != _priorityTargets.end() );

                const PriorityTask & attackTask = iter->second;
                assert( attackTask.type == PriorityTaskType::ATTACK );

                for ( const int32_t secondaryTaskTileIdx : attackTask.secondaryTaskTileId ) {
                    const Castle * castleUnderThreat = world.getCastleEntrance( Maps::GetPoint( secondaryTaskTileIdx ) );
                    if ( castleUnderThreat == nullptr ) {
                        continue;
                    }

                    assert( hero.GetColor() == castleUnderThreat->GetColor() );

                    // Apply a bonus so that the AI prefers to eliminate the threat if possible instead of guarding its castle
                    value = std::max( value, calculateCastleValue( castleUnderThreat ) * 2 );
                }
            }

            // This castle is defenseless
            if ( !castle->GetActualArmy().isValid() ) {
                value *= defenselessCastleModifier;
            }
        }

        return value;
    };

    switch ( objectType ) {
    case MP2::OBJ_CASTLE: {
        const Castle * castle = world.getCastleEntrance( Maps::GetPoint( index ) );
        if ( !castle ) {
            // How is it even possible?
            assert( 0 );
            return valueToIgnore;
        }

        if ( hero.GetColor() == castle->GetColor() ) {
            // If our castle is in danger, then we should evaluate it not from the point of view of momentary benefits
            // for the hero (for example, the presence of reinforcements), but from the point of view of the castle,
            // which then will have to be taken back from the enemy, that is, according to approximately the same logic
            // as enemy castles
            if ( isCriticalTask( index ) ) {
                return calculateCastleValue( castle );
            }

            double value = castle->getVisitValue( hero );

            if ( !isPriorityTask( index ) && value < 500 ) {
                return valueToIgnore;
            }

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

        return calculateCastleValue( castle );
    }
    case MP2::OBJ_HERO: {
        const Heroes * otherHero = tile.getHero();
        if ( !otherHero ) {
            // How is it even possible?
            assert( 0 );
            return valueToIgnore;
        }

        if ( hero.GetColor() == otherHero->GetColor() ) {
            if ( hero.getAIRole() > otherHero->getAIRole() ) {
                // The other hero has a lower role. Do not waste time for meeting. Let him to come.
                return valueToIgnore;
            }

            if ( hero.getAIRole() == otherHero->getAIRole()
                 && hero.getStatsValue() + Difficulty::getMinStatDiffForAIHeroesMeeting( Game::getDifficulty() ) + 1 > otherHero->getStatsValue() ) {
                // Either this hero is superior or approximately equal to another hero in terms of stats. Do not waste time for meeting. Let him come instead.
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

        double value = 12000;

        // If this hero threatens our castles, then we should evaluate him no worse than the best of our castles that he threatens
        // (as if it had to be taken back from the enemy). If a threatening hero can be attacked in an open field, it is better to
        // do it now than to attack him later in our castle occupied by him.
        if ( isCriticalTask( index ) ) {
            const auto iter = _priorityTargets.find( index );
            assert( iter != _priorityTargets.end() );

            const PriorityTask & attackTask = iter->second;
            assert( attackTask.type == PriorityTaskType::ATTACK );

            for ( const int32_t secondaryTaskTileIdx : attackTask.secondaryTaskTileId ) {
                const Castle * castle = world.getCastleEntrance( Maps::GetPoint( secondaryTaskTileIdx ) );
                if ( castle == nullptr ) {
                    continue;
                }

                assert( hero.GetColor() == castle->GetColor() );

                // Apply a bonus so that the AI prefers to eliminate the threat if possible instead of guarding its castle
                value = std::max( value, calculateCastleValue( castle ) * 2 );
            }
        }
        else {
            const Castle * castle = otherHero->inCastle();

            if ( castle != nullptr ) {
                // If the enemy hero is in the castle, then after defeating this hero, AI will automatically capture his
                // castle, so it's worth adding the value of the castle to the value of the enemy hero
                value += calculateCastleValue( castle );
            }
            else if ( otherHero->GetControl() == CONTROL_AI ) {
                // AI heroes should not attack other AI heroes so aggressively as human heroes.
                // This is done to avoid situations when human players just wait when AI heroes kill each other.
                value *= 0.8;
            }
        }

        return value;
    }
    case MP2::OBJ_MONSTER: {
        const Army monsters( tile );
        if ( !monsters.isValid() ) {
            // How is it even possible?
            assert( 0 );
            return -dangerousTaskPenalty;
        }

        // TODO: we should add logic to compare monsters and hero army strengths.
        const bool anotherFriendlyHeroPresent = _regions[tile.GetRegion()].friendlyHeroes > 1;
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
        const bool anotherFriendlyHeroPresent = _regions[tile.GetRegion()].friendlyHeroes > 1;
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

double AI::Planner::getCourierObjectValue( const Heroes & hero, const int32_t index, const double valueToIgnore, const uint32_t distanceToObject ) const
{
    // Courier should focus on its main task and visit other objects only if it's close to the destination
    assert( hero.getAIRole() == Heroes::Role::COURIER );

    // Values (n logn) for pre-defined distances where AI courier would consider taking a detour to visit those
    const double twoTiles = 500;
    const double fiveTiles = 1400;
    const double tenTiles = 3000;

    const Maps::Tile & tile = world.getTile( index );
    const MP2::MapObjectType objectType = tile.getMainObjectType();

    switch ( objectType ) {
    case MP2::OBJ_HERO: {
        const Heroes * otherHero = tile.getHero();
        if ( !otherHero ) {
            // How is it even possible?
            assert( 0 );
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

        // This hero threatens our castles, use the general evaluation in this case
        if ( isCriticalTask( index ) ) {
            break;
        }

        return tenTiles;
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
    case MP2::OBJ_MINE:
    case MP2::OBJ_SAWMILL: {
        if ( getColorFromTile( tile ) == hero.GetColor() ) {
            return -dangerousTaskPenalty; // don't even attempt to go here
        }
        return ( getDailyIncomeObjectResources( tile ).gold > 0 ) ? tenTiles : fiveTiles;
    }
    case MP2::OBJ_CAMPFIRE:
    case MP2::OBJ_FLOTSAM:
    case MP2::OBJ_GENIE_LAMP:
    case MP2::OBJ_RESOURCE: {
        return twoTiles;
    }
    case MP2::OBJ_SEA_CHEST:
    case MP2::OBJ_TREASURE_CHEST: {
        const Artifact art = getArtifactFromTile( tile );
        if ( art.isValid() && isFindArtifactVictoryConditionForHuman( art ) ) {
            // WINS_ARTIFACT victory condition does not apply to AI-controlled players, we should leave this object untouched for the human player.
            assert( 0 );
            return -dangerousTaskPenalty;
        }

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

double AI::Planner::getScoutObjectValue( const Heroes & hero, const int32_t index, const double valueToIgnore, const uint32_t distanceToObject ) const
{
    // Courier should focus on its main task and visit other objects only if it's close to the destination
    assert( hero.getAIRole() == Heroes::Role::SCOUT );

    const Maps::Tile & tile = world.getTile( index );
    const MP2::MapObjectType objectType = tile.getMainObjectType();

    switch ( objectType ) {
    case MP2::OBJ_WITCHS_HUT: {
        if ( !hero.isVisited( tile, Visit::GLOBAL ) ) {
            // Since this object has not been visited use general value estimation.
            return getGeneralObjectValue( hero, index, valueToIgnore, distanceToObject );
        }

        const Skill::Secondary & skill = getSecondarySkillFromWitchsHut( tile );
        if ( !skill.isValid() || hero.HasMaxSecondarySkill() ) {
            // This mustn't happen as these checks are done prior this code.
            assert( 0 );
            return -dangerousTaskPenalty;
        }

        const int skillType = skill.Skill();
        if ( hero.HasSecondarySkill( skillType ) ) {
            // This mustn't happen as these checks are done prior this code.
            assert( 0 );
            return -dangerousTaskPenalty;
        }

        double value = getGeneralObjectValue( hero, index, valueToIgnore, distanceToObject );
        if ( skillType == Skill::Secondary::SCOUTING || skillType == Skill::Secondary::LOGISTICS ) {
            // Scouts should focus on scouting so these skills must have much higher priority.
            value = value * 3;
        }
        else if ( skillType == Skill::Secondary::PATHFINDING ) {
            value = value * 2;
        }

        return value;
    }
    default:
        break;
    }

    return getGeneralObjectValue( hero, index, valueToIgnore, distanceToObject );
}

double AI::Planner::getObjectValue( const Heroes & hero, const int32_t index, const MP2::MapObjectType objectType, const double valueToIgnore,
                                    const uint32_t distanceToObject ) const
{
    assert( objectType == world.getTile( index ).getMainObjectType() );

#ifdef NDEBUG
    (void)objectType;
#endif

    switch ( hero.getAIRole() ) {
    case Heroes::Role::HUNTER:
        return getGeneralObjectValue( hero, index, valueToIgnore, distanceToObject );
    case Heroes::Role::SCOUT:
        return getScoutObjectValue( hero, index, valueToIgnore, distanceToObject );
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

int AI::Planner::getCourierMainTarget( const Heroes & hero, const double lowestPossibleValue )
{
    assert( hero.getAIRole() == Heroes::Role::COURIER );

    int targetIndex = -1;

    const Kingdom & kingdom = hero.GetKingdom();
    const VecHeroes & allHeroes = kingdom.GetHeroes();

    // Check if we have army and should bring it to friendly hero first
    double bestTargetValue = lowestPossibleValue;

    for ( const Heroes * otherHero : allHeroes ) {
        if ( !otherHero || hero.GetID() == otherHero->GetID() ) {
            continue;
        }

        const Heroes::Role role = otherHero->getAIRole();
        if ( role == Heroes::Role::COURIER || role == Heroes::Role::SCOUT ) {
            continue;
        }

        const int currentHeroIndex = otherHero->GetIndex();

        const auto [dist, dummy] = getDistanceToTile( _pathfinder, currentHeroIndex );
        if ( dist == 0 || hero.hasMetWithHero( otherHero->GetID() ) ) {
            continue;
        }

        double value = hero.getMeetingValue( *otherHero );
        if ( value < 500 ) {
            continue;
        }

        if ( role == Heroes::Role::CHAMPION ) {
            value *= 2.5;
        }
        value -= dist;

        if ( value > bestTargetValue ) {
            bestTargetValue = value;
            targetIndex = currentHeroIndex;
        }
    }

    if ( targetIndex != -1 ) {
        return targetIndex;
    }

    // Reset the max value
    bestTargetValue = lowestPossibleValue;

    for ( const Castle * castle : kingdom.GetCastles() ) {
        if ( castle == nullptr || castle->GetHero() != nullptr ) {
            continue;
        }

        const int currentCastleIndex = castle->GetIndex();

        const auto [dist, dummy] = getDistanceToTile( _pathfinder, currentCastleIndex );
        if ( dist == 0 ) {
            continue;
        }

        double value = castle->getVisitValue( hero );
        if ( value < 250 ) {
            continue;
        }

        const int safetyFactor = _regions[world.getTile( currentCastleIndex ).GetRegion()].safetyFactor;
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

int AI::Planner::getPriorityTarget( Heroes & hero, double & maxPriority )
{
    DEBUG_LOG( DBG_AI, DBG_INFO, "Find Adventure Map target for hero " << hero.GetName() << " at current position " << hero.GetIndex() )

    const double lowestPossibleValue = -1.0 * Maps::Ground::slowestMovePenalty * world.getSize();

    int priorityTarget = -1;
    maxPriority = lowestPossibleValue;
#ifdef WITH_DEBUG
    {
        std::set<int> objectIndexes;

        for ( const auto & [idx, objType] : _mapActionObjects ) {
            if ( objType == MP2::OBJ_HERO ) {
                assert( world.getTile( idx ).getHero() != nullptr );
            }

            if ( const auto [dummy, inserted] = objectIndexes.emplace( idx ); !inserted ) {
                assert( 0 );
            }
        }
    }

    MP2::MapObjectType objectType = MP2::OBJ_NONE;
#endif

    // Pre-calculate penalties for tiles where there is a threat of enemy attack
    const std::vector<double> enemyThreatPenalties = [this, &hero = std::as_const( hero )]() {
        std::vector<double> result( world.getSize(), 0.0 );

        const AIWorldPathfinderStateRestorer pathfinderStateRestorer( _pathfinder );

        // Use the "optimistic" pathfinder settings for enemy heroes - minimal army advantage, minimal reserve of spell points
        _pathfinder.setMinimalArmyStrengthAdvantage( ARMY_ADVANTAGE_DESPERATE );
        _pathfinder.setSpellPointsReserveRatio( 0.0 );

        const double heroStrength = hero.GetArmy().GetStrength();

        for ( const auto & [dummy, enemyArmy] : _enemyArmies ) {
            // Only enemy heroes are taken into account
            if ( enemyArmy.hero == nullptr ) {
                continue;
            }

            // An enemy hero does not pose a threat if he is approximately equal in strength or weaker than our hero
            if ( heroStrength * ARMY_ADVANTAGE_SMALL >= enemyArmy.strength ) {
                continue;
            }

            // In theory, this should never be the case
            if ( enemyArmy.movePoints == 0 ) {
                assert( 0 );
                continue;
            }

            // Safe tiles should not be located close to a tile accessible to an enemy hero, some margin is needed
            const uint32_t enemyArmyMovePointsThreshold = enemyArmy.movePoints + Maps::Ground::slowestMovePenalty * 2;
            // If the enemy hero can't cross paths with our hero anywhere, then it makes sense to use a rough but quick estimate. Otherwise, an accurate but
            // relatively slow estimate will be used.
            const bool useRoughEstimate = ( Maps::GetApproximateDistance( hero.GetIndex(), enemyArmy.index ) * Maps::Ground::fastestMovePenalty
                                            > hero.GetMovePoints() + enemyArmyMovePointsThreshold );

            if ( !useRoughEstimate ) {
                // Pre-cache the pathfinder database for the enemy hero
                _pathfinder.reEvaluateIfNeeded( *enemyArmy.hero );
            }

            for ( size_t i = 0; i < result.size(); ++i ) {
                const int32_t tileIdx = static_cast<int32_t>( i );
                assert( Maps::isValidAbsIndex( tileIdx ) );

                const auto [distToTile, isTileConsideredSafe] = [this, enemyArmyIdx = enemyArmy.index, enemyArmyMovePointsThreshold, useRoughEstimate, tileIdx]() {
                    // The tile on which the enemy hero is located is always considered unsafe
                    if ( tileIdx == enemyArmyIdx ) {
                        return std::make_pair( static_cast<uint32_t>( 0 ), false );
                    }

                    if ( useRoughEstimate ) {
                        const uint32_t dist = Maps::GetApproximateDistance( tileIdx, enemyArmyIdx ) * Maps::Ground::fastestMovePenalty;

                        // When using a rough estimate, a tile is considered safe if the enemy hero cannot reach it within one turn, even if the path from the enemy
                        // hero to this tile is straight and with a minimum movement penalty. The potential ability of the enemy hero to use spells to move to this
                        // tile (for example, the Dimension Door or Town Portal) is not considered in this assessment.
                        return std::make_pair( dist, dist > enemyArmyMovePointsThreshold );
                    }

                    const uint32_t dist = _pathfinder.getDistance( tileIdx );

                    // When using an accurate estimate, a tile is considered safe if the enemy hero does not have access to it (in particular, if it is hidden from
                    // him in the fog) or he cannot reach it within one turn. The potential ability of the enemy hero to use spells to move to this tile (for example,
                    // the Dimension Door or Town Portal) is not considered in this assessment.
                    return std::make_pair( dist, dist == 0 || dist > enemyArmyMovePointsThreshold );
                }();

                if ( isTileConsideredSafe ) {
                    continue;
                }

                // The penalty is cumulative (i.e. this is the sum of the penalties from all threatening heroes), the penalty from each threatening hero increases
                // linearly as the distance to that hero decreases
                result[i] += dangerousTaskPenalty * ( 2.0 - static_cast<double>( distToTile ) / static_cast<double>( enemyArmyMovePointsThreshold ) );
            }
        }

        return result;
    }();

    // Pre-cache the pathfinder database for our hero
    _pathfinder.reEvaluateIfNeeded( hero );

    ObjectValidator objectValidator( hero, _pathfinder, *this );
    ObjectValueStorage valueStorage( hero, *this, lowestPossibleValue );

    const auto getObjectValue = [this, &hero = std::as_const( hero ), &enemyThreatPenalties, &objectValidator,
                                 &valueStorage]( const int destination, uint32_t & distance, double & value, const MP2::MapObjectType type, const bool isDimensionDoor ) {
        // Dimension door path does not include any objects on the way.
        if ( !isDimensionDoor ) {
            for ( const IndexObject & pair : _pathfinder.getObjectsOnTheWay( destination ) ) {
                if ( !objectValidator.isValid( pair.first ) ) {
                    continue;
                }

                if ( const auto iter = _mapActionObjects.find( pair.first ); iter == _mapActionObjects.end() || iter->second != pair.second ) {
                    continue;
                }

                const double extraValue = valueStorage.value( pair, 0 );
                if ( extraValue > 0 ) {
                    // There is no need to reduce the quality of the object even if the path has others.
                    value += extraValue;
                }
            }
        }

        const uint32_t heroMovePoints = hero.GetMovePoints();

        const double enemyThreatPenalty = [&hero, &enemyThreatPenalties, destination, distance, type, heroMovePoints]() {
            if ( type == MP2::OBJ_CASTLE ) {
                const Castle * castle = world.getCastleEntrance( Maps::GetPoint( destination ) );
                assert( castle != nullptr );

                // We should strive to defend our castles even if our hero is relatively weak (he can get reinforcements in the castle), so enemy threat
                // penalties should not apply if our hero is able to reach the castle within one turn.
                if ( castle->GetColor() == hero.GetColor() && distance <= heroMovePoints ) {
                    return 0.0;
                }
            }

            assert( destination >= 0 && static_cast<size_t>( destination ) < enemyThreatPenalties.size() );

            return enemyThreatPenalties[destination];
        }();

        value -= enemyThreatPenalty;

        // Distant object which is out of reach for the current turn must have lower priority.
        if ( distance > heroMovePoints ) {
            distance = heroMovePoints + ( distance - heroMovePoints ) * 2;
        }

        value = scaleWithDistanceAndTime( value, distance, type );
    };

    // Set baseline target if it's a special role
    if ( hero.getAIRole() == Heroes::Role::COURIER ) {
        const int courierTarget = getCourierMainTarget( hero, lowestPossibleValue );
        if ( courierTarget != -1 ) {
            // Anything with positive value can override the courier's main task (i.e. castle or mine capture on the way)
            maxPriority = 0;
            priorityTarget = courierTarget;
#ifdef WITH_DEBUG
            objectType = world.getTile( courierTarget ).getMainObjectType();
#endif

            DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() << " is a courier with a main target tile at " << courierTarget )
        }
        else {
            // If there's nothing to do as a Courier reset the role
            hero.setAIRole( Heroes::Role::HUNTER );
        }
    }

    for ( const auto & [idx, objType] : _mapActionObjects ) {
        if ( !objectValidator.isValid( idx ) ) {
            continue;
        }

        auto [dist, useDimensionDoor] = getDistanceToTile( _pathfinder, idx );
        if ( dist == 0 ) {
            continue;
        }

        double value = valueStorage.value( { idx, objType }, dist );
        getObjectValue( idx, dist, value, objType, useDimensionDoor );

        if ( dist > 0 && value > maxPriority ) {
            maxPriority = value;
            priorityTarget = idx;
#ifdef WITH_DEBUG
            objectType = objType;
#endif

            DEBUG_LOG( DBG_AI, DBG_TRACE, hero.GetName() << ": valid object at " << idx << " value is " << value << " (" << MP2::StringObject( objType ) << ")" )
        }
    }

    double fogDiscoveryValue = getFogDiscoveryValue( hero );
    // TODO: add logic to check fog discovery based on Dimension Door distance, not the nearest tile.
    const auto [fogDiscoveryTarget, isTerritoryExpansion] = _pathfinder.getFogDiscoveryTile( hero );

    if ( fogDiscoveryTarget >= 0 ) {
        auto [distanceToFogDiscovery, useDimensionDoor] = getDistanceToTile( _pathfinder, fogDiscoveryTarget );

        if ( isTerritoryExpansion ) {
            // Over time the AI should focus more on territory expansion.
            const uint32_t period = getTimeoutBeforeFogDiscoveryIntensification( hero );
            assert( period > 0 );

            if ( fogDiscoveryValue < 0 ) {
                // This is actually a very useful fog discovery action which might lead to finding of new objects.
                // Increase the value of this action.

                if ( world.CountDay() > period ) {
                    fogDiscoveryValue = 0;
                }
                else {
                    fogDiscoveryValue = fogDiscoveryValue / 2 * ( period - world.CountDay() ) / period;
                }
            }
            else {
                // Over time the AI should focus more on territory expansion.
                // Scouts must focus on expansion so they should reach maximum "attention" in a month.
                fogDiscoveryValue += std::min( 1000.0 * world.CountDay() / period, 1000.0 );
            }
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
    else {
        priorityTarget = fogDiscoveryTarget;
        DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() << " can't find an object. Scouting the fog of war at " << priorityTarget )
    }

    return priorityTarget;
}

void AI::Planner::updatePriorityTargets( Heroes & hero, int32_t tileIndex, const MP2::MapObjectType objectType )
{
    if ( objectType != MP2::OBJ_CASTLE && objectType != MP2::OBJ_HERO ) {
        // Priorities are only for castles and heroes at the moment.
        return;
    }

    const auto updateTile = [this, &hero, tileIndex, objectType]() {
        const auto updateCastle = [this, &hero, tileIndex]() {
            const Castle * castle = world.getCastleEntrance( Maps::GetPoint( tileIndex ) );
            if ( castle == nullptr ) {
                // How is it possible?
                assert( 0 );

                _enemyArmies.erase( tileIndex );
                return;
            }

            if ( hero.isFriends( castle->GetColor() ) ) {
                _enemyArmies.erase( tileIndex );

                updatePriorityForCastle( *castle );
            }
            else {
                updatePriorityAttackTarget( hero.GetKingdom(), world.getTile( tileIndex ) );
            }
        };

        if ( objectType == MP2::OBJ_CASTLE ) {
            updateCastle();
        }
        else if ( objectType == MP2::OBJ_HERO ) {
            const Maps::Tile & tile = world.getTile( tileIndex );

            const Heroes * anotherHero = tile.getHero();
            if ( anotherHero == nullptr ) {
                // Another hero lost the battle, but he could defend a castle
                if ( tile.getMainObjectType() == MP2::OBJ_CASTLE ) {
                    updateCastle();
                }
                else {
                    _enemyArmies.erase( tileIndex );
                }

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

    auto iter = _priorityTargets.find( tileIndex );
    if ( iter == _priorityTargets.end() ) {
        // If the object is not a priority we have to update it after the battle as it can become the one.
        // Especially, when the opposite army has grown Skeletons or Ghosts.
        updateTile();

        // If the update did not add any priorities then nothing more to do.
        iter = _priorityTargets.find( tileIndex );
        if ( iter == _priorityTargets.end() ) {
            return;
        }
    }

    const PriorityTaskType taskType = iter->second.type;

    switch ( taskType ) {
    case PriorityTaskType::DEFEND:
    case PriorityTaskType::REINFORCE:
        if ( hero.GetIndex() != tileIndex ) {
            // Either the castle has just been captured, or the hero meets the guest hero of a friendly castle. If any of these assertions blow up, then this is not
            // one of these cases.
#ifndef NDEBUG
            const Maps::Tile & tile = world.getTile( tileIndex );
#endif
            assert( tile.getMainObjectType( false ) == MP2::OBJ_CASTLE && hero.GetColor() == Maps::getColorFromTile( tile ) );
            assert( Maps::isValidDirection( tileIndex, Direction::BOTTOM ) && hero.GetIndex() == Maps::GetDirectionIndex( tileIndex, Direction::BOTTOM ) );

            // In case the castle has just been captured, we need to update the information related to the object on the corresponding tile.
            updateTile();

            // Since the hero has not yet reached the tile associated with the corresponding task, this task is not considered completed and should not be removed
            // yet.
            return;
        }

        // These tasks are only for castles at the moment!
        assert( objectType == MP2::OBJ_CASTLE );

        // How is it even possible that a hero died while simply moving into a castle?
        assert( hero.isActive() );

        hero.SetModes( Heroes::SLEEPER );

#ifdef WITH_DEBUG
        {
            const Castle * castle = world.getCastleEntrance( Maps::GetPoint( tileIndex ) );
            assert( castle != nullptr );

            DEBUG_LOG( DBG_AI, DBG_INFO,
                       hero.GetName() << " stays in " << castle->GetName() << " to " << ( taskType == PriorityTaskType::DEFEND ? "defend the castle" : "reinforce" ) )
        }
#endif

        _priorityTargets.erase( tileIndex );

        break;
    case PriorityTaskType::ATTACK:
        removePriorityAttackTarget( tileIndex );

        updateTile();

        break;
    default:
        // Did you add a new type of priority task? Add the logic above!
        assert( 0 );
        break;
    }
}

Skill::Secondary AI::Planner::pickSecondarySkill( const Heroes & hero, const Skill::Secondary & left, const Skill::Secondary & right )
{
    // heroes can get 1 or 0 skill choices depending on a level
    if ( !right.isValid() ) {
        // if both left and right are invalid returning either is fine
        return left;
    }

    const double leftValue = getSecondarySkillValue( hero, left );
    const double rightValue = getSecondarySkillValue( hero, right );

    DEBUG_LOG( DBG_AI, DBG_TRACE,
               hero.GetName() << " picking a skill: " << leftValue << " for " << left.String( left.first ) << " and " << rightValue << " for "
                              << right.String( right.first ) )

    if ( std::fabs( leftValue - rightValue ) < 0.001 ) {
        // If skill value is lower than the threshold then it's undesireable. Avoid learning it.
        if ( leftValue < 300.0 ) {
            return left.Level() == Skill::Level::BASIC ? right : left;
        }

        return left.Level() == Skill::Level::BASIC ? left : right;
    }

    return leftValue > rightValue ? left : right;
}

void AI::Planner::HeroesBeginMovement( Heroes & hero )
{
    assert( hero.isActive() );

    const Route::Path & path = hero.GetPath();
    if ( !path.isValidForMovement() ) {
        return;
    }

    const int32_t heroIdx = hero.GetIndex();

    const int frontDirection = path.GetFrontDirection();
    assert( Maps::isValidDirection( heroIdx, frontDirection ) );

    const int32_t nextTileIdx = Maps::GetDirectionIndex( heroIdx, frontDirection );

    const Maps::Tile & currTile = world.getTile( heroIdx );
    const Maps::Tile & nextTile = world.getTile( nextTileIdx );

    if ( currTile.isWater() || !nextTile.isWater() || nextTile.getMainObjectType() != MP2::OBJ_NONE ) {
        return;
    }

    // If the hero goes to the water tile, then this should be his last movement
    assert( path.size() == 1 );

    const int32_t formerBoatIdx = HeroesCastSummonBoat( hero, nextTileIdx );

    updateMapActionObjectCache( formerBoatIdx );
    updateMapActionObjectCache( nextTileIdx );
}

void AI::Planner::HeroesActionComplete( Heroes & hero, const int32_t tileIndex, const MP2::MapObjectType objectType )
{
    // This method is called upon action completion and the hero could no longer be available.
    // So it is to check if the hero is still present.
    if ( hero.isActive() ) {
        Castle * castle = hero.inCastleMutable();

        if ( castle ) {
            reinforceHeroInCastle( hero, *castle, castle->GetKingdom().GetFunds() );
        }
        else {
            OptimizeTroopsOrder( hero.GetArmy() );
        }
    }

    _tileArmyStrengthValues.erase( tileIndex );

    updatePriorityTargets( hero, tileIndex, objectType );

    updateMapActionObjectCache( tileIndex );
}

void AI::Planner::HeroesActionNewPosition( Heroes & hero )
{
    if ( !hero.isActive() ) {
        return;
    }

    Route::Path & path = hero.GetPath();
    if ( !path.isValidForMovement() ) {
        return;
    }

    const int32_t heroIdx = hero.GetIndex();
    assert( heroIdx == path.GetFrontIndex() );

    const int nextStepDirection = path.GetNextStepDirection();
    if ( !Maps::isValidDirection( heroIdx, nextStepDirection ) ) {
        return;
    }

    const int32_t nextTileIdx = Maps::GetDirectionIndex( heroIdx, nextStepDirection );

    const Maps::Tile & currTile = world.getTile( heroIdx );
    const Maps::Tile & nextTile = world.getTile( nextTileIdx );

    if ( currTile.isWater() || !nextTile.isWater() || nextTile.getMainObjectType() != MP2::OBJ_NONE ) {
        return;
    }

    // If the hero goes to the water tile, then this should be his last movement
    // (not counting the current step, which is not yet completed at the moment)
    assert( path.size() == 2 );

    // It may happen that although the hero at the beginning of his path had enough spell points to
    // summon a boat, but while moving through the guarded tiles, these spell points were spent. In
    // this case, just stop.
    if ( !hero.CanCastSpell( Spell::SUMMONBOAT ) ) {
        path.Reset();
        hero.SetMove( false );

        return;
    }

    const int32_t formerBoatIdx = HeroesCastSummonBoat( hero, nextTileIdx );

    updateMapActionObjectCache( formerBoatIdx );
    updateMapActionObjectCache( nextTileIdx );
}

bool AI::Planner::isValidHeroObject( const Heroes & hero, const int32_t index, const bool underHero )
{
    return HeroesValidObject( hero, hero.GetArmy().GetStrength(), index, _pathfinder, *this, hero.getAIMinimumJoiningArmyStrength(), underHero );
}

void AI::Planner::HeroesPreBattle( HeroBase & hero, bool isAttacking )
{
    if ( isAttacking ) {
        OptimizeTroopsOrder( hero.GetArmy() );
    }
}

bool AI::Planner::HeroesTurn( VecHeroes & heroes, uint32_t & currentProgressValue, uint32_t endProgressValue )
{
    if ( heroes.empty() ) {
        // No heroes so we indicate that all heroes moved.
        return true;
    }

    std::vector<Heroes *> availableHeroes;
    availableHeroes.reserve( heroes.size() );

    for ( Heroes * hero : heroes ) {
        assert( hero != nullptr );

        addHeroToMove( hero, availableHeroes );
    }

    Interface::StatusPanel & status = Interface::AdventureMap::Get().getStatusPanel();

    uint32_t heroesToMoveTotalCount = static_cast<uint32_t>( availableHeroes.size() );
    uint32_t startProgressValue = currentProgressValue;

    if ( endProgressValue - currentProgressValue >= heroesToMoveTotalCount * 2 ) {
        // An extra case when there is only one hero and we have more than 2 points to display the turn progress.
        // We increase the start value to display this progress after the pathfinder ends his work
        // and the next progress increase will be aster the hero makes his move.
        ++startProgressValue;
    }

    uint32_t turnProgressScale = 4 * ( endProgressValue - startProgressValue );

    while ( !availableHeroes.empty() ) {
        const AIWorldPathfinderStateRestorer pathfinderStateRestorer( _pathfinder );

        Heroes * bestHero = availableHeroes.front();
        int bestTargetIndex = -1;

        {
            const bool isLosingGame = bestHero->isLosingGame();

            static const std::vector<std::pair<double, double>> commonPathfinderConfigurations{ { ARMY_ADVANTAGE_LARGE, 0.5 },
                                                                                                { ARMY_ADVANTAGE_MEDIUM, 0.25 },
                                                                                                { ARMY_ADVANTAGE_SMALL, 0.0 } };
            static const std::vector<std::pair<double, double>> emergencyPathfinderConfigurations{ { ARMY_ADVANTAGE_DESPERATE, 0.0 } };

            for ( const auto & [minStrengthAdvantage, spReserveRatio] : isLosingGame ? emergencyPathfinderConfigurations : commonPathfinderConfigurations ) {
                _pathfinder.setMinimalArmyStrengthAdvantage( minStrengthAdvantage );
                _pathfinder.setSpellPointsReserveRatio( spReserveRatio );

                double maxPriority = 0;

                for ( Heroes * hero : availableHeroes ) {
                    double priority = -1;
                    const int targetIndex = getPriorityTarget( *hero, priority );

                    if ( targetIndex != -1 && ( priority > maxPriority || bestTargetIndex == -1 ) ) {
                        maxPriority = priority;
                        bestTargetIndex = targetIndex;
                        bestHero = hero;
                    }

                    // This loop may take many time for computations so update hourglass grains animation.
                    status.drawAITurnProgress( currentProgressValue );
                }

                if ( bestTargetIndex != -1 ) {
                    break;
                }
            }
        }

        // Calculate turn progress taking into account that the current 'bestHero' most likely will end his turn
        // and will be removed from 'availableHeroes' vector: we add 3/4 (not 1/2) to help rounding the result.
        uint32_t progressValue = ( turnProgressScale * ( heroesToMoveTotalCount - static_cast<uint32_t>( availableHeroes.size() ) ) + 3 * heroesToMoveTotalCount )
                                     / ( 4 * heroesToMoveTotalCount )
                                 + startProgressValue;
        if ( currentProgressValue < progressValue ) {
            currentProgressValue = progressValue;
            status.drawAITurnProgress( currentProgressValue );
        }

        if ( bestTargetIndex == -1 ) {
            // Possibly heroes have nothing to do because one of them is blocking the way. Move a random hero randomly and see what happens.
            Rand::Shuffle( availableHeroes );

            for ( Heroes * hero : availableHeroes ) {
                // Skip heroes located in castles.
                if ( hero->inCastle() != nullptr ) {
                    continue;
                }

                if ( !AIWorldPathfinder::isHeroPossiblyBlockingWay( *hero ) ) {
                    continue;
                }

                const int targetIndex = _pathfinder.getNearestTileToMove( *hero );
                if ( targetIndex != -1 ) {
                    bestTargetIndex = targetIndex;
                    bestHero = hero;

                    DEBUG_LOG( DBG_AI, DBG_INFO, bestHero->GetName() << " may be blocking the way. Moving to " << bestTargetIndex )

                    break;
                }
            }
        }

        if ( bestTargetIndex == -1 ) {
            // Nothing to do. Stop everything.
            break;
        }

        const size_t heroesBefore = heroes.size();
        _pathfinder.reEvaluateIfNeeded( *bestHero );

        int prevHeroPosition = bestHero->GetIndex();

        {
            std::list<Route::Step> dimensionDoorPath = _pathfinder.buildDimensionDoorPath( bestTargetIndex );
            uint32_t regularMovementDist = _pathfinder.getDistance( bestTargetIndex );
            uint32_t dimensionDoorDist = Route::calculatePathPenalty( dimensionDoorPath );

            if ( shouldUseDimensionDoor( regularMovementDist, dimensionDoorDist ) ) {
                while ( shouldUseDimensionDoor( regularMovementDist, dimensionDoorDist ) ) {
                    assert( !dimensionDoorPath.empty() && bestHero->MayStillMove( false, false ) );

                    HeroesCastDimensionDoor( *bestHero, dimensionDoorPath.front().GetIndex() );
                    dimensionDoorDist -= dimensionDoorPath.front().GetPenalty();

                    _pathfinder.reEvaluateIfNeeded( *bestHero );
                    regularMovementDist = _pathfinder.getDistance( bestTargetIndex );

                    dimensionDoorPath.pop_front();

                    // Hero can jump straight into the fog using the Dimension Door spell, which triggers the mechanics of fog revealing for his new tile
                    // and this results in inserting a new hero position into the action object cache. Perform the necessary updates.
                    assert( bestHero->isActive() && bestHero->GetIndex() != prevHeroPosition );

                    updateMapActionObjectCache( prevHeroPosition );
                    updateMapActionObjectCache( bestHero->GetIndex() );

                    prevHeroPosition = bestHero->GetIndex();
                }

                if ( dimensionDoorDist > 0 ) {
                    // The rest of the path the hero should do by foot.
                    bestHero->GetPath().setPath( _pathfinder.buildPath( bestTargetIndex ) );

                    HeroesMove( *bestHero );
                }
            }
            else {
                bestHero->GetPath().setPath( _pathfinder.buildPath( bestTargetIndex ) );

                HeroesMove( *bestHero );
            }
        }

        if ( !bestHero->isActive() || bestHero->GetIndex() != prevHeroPosition ) {
            // The hero died or moved to another position. We have to update the action object cache.
            updateMapActionObjectCache( prevHeroPosition );

            if ( bestHero->isActive() ) {
                // Hero moved to another position and is still alive.
                updateMapActionObjectCache( bestHero->GetIndex() );
            }
        }

        // The size of heroes can be increased if a new hero is released from Jail.
        if ( heroes.size() > heroesBefore ) {
            const size_t availableHeroesBefore = availableHeroes.size();

            addHeroToMove( heroes.back(), availableHeroes );

            if ( availableHeroesBefore < availableHeroes.size() ) {
                ++heroesToMoveTotalCount;

                if ( endProgressValue < 8 ) {
                    ++endProgressValue;
                    turnProgressScale += 4;
                }
            }
        }

        availableHeroes.erase( std::remove_if( availableHeroes.begin(), availableHeroes.end(),
                                               []( const Heroes * hero ) { return !hero->MayStillMove( false, false ); } ),
                               availableHeroes.end() );

        progressValue = ( turnProgressScale * ( heroesToMoveTotalCount - static_cast<uint32_t>( availableHeroes.size() ) ) + heroesToMoveTotalCount )
                            / ( 4 * heroesToMoveTotalCount )
                        + startProgressValue;
        if ( currentProgressValue < progressValue ) {
            currentProgressValue = progressValue;
            status.drawAITurnProgress( currentProgressValue );
        }
    }

    status.drawAITurnProgress( endProgressValue );

    return availableHeroes.empty();
}
