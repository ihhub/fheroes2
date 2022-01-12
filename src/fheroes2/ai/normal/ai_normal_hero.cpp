/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) 2020                                                    *
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

#include "ai_normal.h"
#include "game.h"
#include "ground.h"
#include "heroes.h"
#include "logging.h"
#include "luck.h"
#include "maps.h"
#include "morale.h"
#include "mp2.h"
#include "settings.h"
#include "world.h"

namespace
{
    bool isHeroAllowedToUseWhirlpool( const Heroes & hero )
    {
        if ( hero.GetArmy().getTotalCount() == 1 ) {
            // No way a hero can loose any army.
            return true;
        }

        switch ( hero.getAIRole() ) {
        case Heroes::Role::HUNTER:
            break;
        case Heroes::Role::FIGHTER:
            // Fighters shouldn't use whirlpools as they can loose an important army.
            return false;
        default:
            // If you set a new type of a hero you must add the logic here.
            assert( 0 );
            break;
        }

        return true;
    }

    bool AIShouldVisitCastle( const Heroes & hero, int castleIndex )
    {
        const Castle * castle = world.getCastleEntrance( Maps::GetPoint( castleIndex ) );
        if ( castle ) {
            if ( hero.GetColor() == castle->GetColor() ) {
                return castle->GetHeroes().Guest() == nullptr;
            }
            else if ( !hero.isFriends( castle->GetColor() ) ) {
                return hero.GetArmy().GetStrength() > castle->GetGarrisonStrength( &hero ) * AI::ARMY_STRENGTH_ADVANTAGE_MEDUIM;
            }
        }
        return false;
    }

    bool HeroesValidObject( const Heroes & hero, const int32_t index, const AIWorldPathfinder & pathfinder )
    {
        const Maps::Tiles & tile = world.GetTiles( index );
        const MP2::MapObjectType objectType = tile.GetObject();
        const Army & army = hero.GetArmy();
        const Kingdom & kingdom = hero.GetKingdom();

        if ( !MP2::isActionObject( objectType ) ) {
            return false;
        }

        switch ( objectType ) {
        case MP2::OBJ_SHIPWRECKSURVIVOR:
        case MP2::OBJ_WATERCHEST:
        case MP2::OBJ_FLOTSAM:
        case MP2::OBJ_BOTTLE:
            return hero.isShipMaster();

        case MP2::OBJ_BUOY:
            return !hero.isObjectTypeVisited( objectType ) && hero.GetMorale() < Morale::BLOOD && !hero.GetArmy().AllTroopsAreUndead();

        case MP2::OBJ_MERMAID:
            return !hero.isObjectTypeVisited( objectType ) && hero.GetLuck() < Luck::IRISH;

        case MP2::OBJ_SIRENS:
            return false;

        case MP2::OBJ_MAGELLANMAPS:
            return hero.isShipMaster() && !hero.isObjectTypeVisited( MP2::OBJ_MAGELLANMAPS, Visit::GLOBAL ) && kingdom.AllowPayment( { Resource::GOLD, 1000 } );

        case MP2::OBJ_WHIRLPOOL:
            // Additional checks will follow
            return true;

        case MP2::OBJ_COAST:
            // Coast is not an action object. If this assertion blows up then something wrong with the logic above.
            assert( 0 );
            return false;

        case MP2::OBJ_SAWMILL:
        case MP2::OBJ_MINES:
        case MP2::OBJ_ALCHEMYLAB:
        case MP2::OBJ_LIGHTHOUSE:
            if ( !hero.isFriends( tile.QuantityColor() ) ) {
                if ( tile.CaptureObjectIsProtection() ) {
                    const Army enemy( tile );
                    return army.isStrongerThan( enemy, AI::ARMY_STRENGTH_ADVANTAGE_SMALL );
                }

                return true;
            }
            break;

        case MP2::OBJ_ABANDONEDMINE:
            if ( !hero.isFriends( tile.QuantityColor() ) ) {
                if ( tile.CaptureObjectIsProtection() ) {
                    const Army enemy( tile );
                    return army.isStrongerThan( enemy, AI::ARMY_STRENGTH_ADVANTAGE_LARGE );
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
                if ( tile.CaptureObjectIsProtection() ) {
                    const Army enemy( tile );
                    return army.isStrongerThan( enemy, AI::ARMY_STRENGTH_ADVANTAGE_MEDUIM );
                }

                return true;
            }
            else if ( tile.QuantityIsValid() )
                return true;
            break;

        case MP2::OBJ_RESOURCE:
        case MP2::OBJ_CAMPFIRE:
        case MP2::OBJ_TREASURECHEST:
            return !hero.isShipMaster();

        case MP2::OBJ_ARTIFACT: {
            const uint32_t variants = tile.QuantityVariant();

            if ( hero.IsFullBagArtifacts() )
                return false;

            if ( hero.isShipMaster() )
                return false;

            // 1,2,3 - 2000g, 2500g+3res, 3000g+5res
            if ( 1 <= variants && 3 >= variants )
                return kingdom.AllowPayment( tile.QuantityFunds() );

            // 4,5 - need have skill wisard or leadership,
            if ( 3 < variants && 6 > variants )
                return hero.HasSecondarySkill( tile.QuantitySkill().Skill() );

            // 6 - 50 rogues, 7 - 1 gin, 8,9,10,11,12,13 - 1 monster level4
            if ( 5 < variants && 14 > variants ) {
                Army enemy( tile );
                return army.isStrongerThan( enemy, AI::ARMY_STRENGTH_ADVANTAGE_LARGE );
            }

            // other
            return true;
        }

        case MP2::OBJ_OBSERVATIONTOWER:
            return Maps::getFogTileCountToBeRevealed( index, Game::GetViewDistance( Game::VIEW_OBSERVATION_TOWER ), hero.GetColor() ) > 0;

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
                 && ( spell == Spell::VIEWALL || spell == Spell::VIEWARTIFACTS || spell == Spell::VIEWHEROES || spell == Spell::VIEWMINES || spell == Spell::VIEWRESOURCES
                      || spell == Spell::VIEWTOWNS || spell == Spell::IDENTIFYHERO || spell == Spell::VISIONS ) ) {
                // AI never uses View spells.
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

            if ( hero.GetArmy().AllTroopsAreUndead() && skillType == Skill::Secondary::LEADERSHIP ) {
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
            return !hero.isObjectTypeVisited( objectType ) && hero.GetMorale() < Morale::BLOOD && !hero.GetArmy().AllTroopsAreUndead();

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
        case MP2::OBJ_THATCHEDHUT: {
            const Troop & troop = tile.QuantityTroop();
            return troop.isValid() && ( army.HasMonster( troop.GetMonster() ) || ( !army.isFullHouse() ) );
        }

        case MP2::OBJ_PEASANTHUT: {
            // Peasants are special monsters. They're the weakest! Think twice before getting them.
            const Troop & troop = tile.QuantityTroop();
            return troop.isValid() && ( army.HasMonster( troop.GetMonster() ) || ( !army.isFullHouse() && army.GetStrength() < troop.GetStrength() * 10 ) );
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
        case MP2::OBJ_BARROWMOUNDS: {
            const Troop & troop = tile.QuantityTroop();
            const payment_t & paymentCosts = troop.GetCost();

            return troop.isValid() && kingdom.AllowPayment( paymentCosts ) && ( army.HasMonster( troop.GetMonster() ) || !army.isFullHouse() );
        }

        // recruit army (battle)
        case MP2::OBJ_DRAGONCITY:
        case MP2::OBJ_CITYDEAD:
        case MP2::OBJ_TROLLBRIDGE: {
            if ( Color::NONE == tile.QuantityColor() ) {
                return army.isStrongerThan( Army( tile ), AI::ARMY_STRENGTH_ADVANTAGE_MEDUIM );
            }
            else {
                const Troop & troop = tile.QuantityTroop();
                const payment_t & paymentCosts = troop.GetCost();

                return troop.isValid() && kingdom.AllowPayment( paymentCosts ) && ( army.HasMonster( troop.GetMonster() ) || ( !army.isFullHouse() ) );
            }
        }

        // recruit genie
        case MP2::OBJ_ANCIENTLAMP: {
            const Troop & troop = tile.QuantityTroop();
            const payment_t & paymentCosts = troop.GetCost();

            return troop.isValid() && kingdom.AllowPayment( paymentCosts ) && ( army.HasMonster( troop.GetMonster() ) || ( !army.isFullHouse() ) );
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
                return enemy.isValid() && army.isStrongerThan( enemy, 2 );
            }
            break;

        case MP2::OBJ_PYRAMID:
            if ( !hero.isVisited( tile, Visit::GLOBAL ) && tile.QuantityIsValid() ) {
                Army enemy( tile );
                return enemy.isValid() && Skill::Level::EXPERT == hero.GetLevelSkill( Skill::Secondary::WISDOM )
                       && army.isStrongerThan( enemy, AI::ARMY_STRENGTH_ADVANTAGE_LARGE );
            }
            break;

        case MP2::OBJ_DAEMONCAVE:
            if ( tile.QuantityIsValid() && 4 != tile.QuantityVariant() )
                return army.isStrongerThan( Army( tile ), AI::ARMY_STRENGTH_ADVANTAGE_MEDUIM );
            break;

        case MP2::OBJ_MONSTER:
            return army.isStrongerThan( Army( tile ), AI::ARMY_STRENGTH_ADVANTAGE_MEDUIM );

        case MP2::OBJ_SIGN:
            // AI has no brains to process anything from sign messages.
            return false;

        case MP2::OBJ_HEROES: {
            const Heroes * hero2 = tile.GetHeroes();
            if ( hero2 ) {
                const bool otherHeroInCastle = ( hero2->inCastle() != nullptr );

                if ( hero.GetColor() == hero2->GetColor() && !hero.hasMetWithHero( hero2->GetID() ) )
                    return !otherHeroInCastle;
                else if ( hero.isFriends( hero2->GetColor() ) )
                    return false;
                else if ( otherHeroInCastle )
                    return AIShouldVisitCastle( hero, index );
                else if ( army.isStrongerThan( hero2->GetArmy(), AI::ARMY_STRENGTH_ADVANTAGE_SMALL ) )
                    return true;
            }
            break;
        }

        case MP2::OBJ_CASTLE:
            return AIShouldVisitCastle( hero, index );

        case MP2::OBJ_BOAT:
            // AI should never consider a boat as a destination point. It uses them only to make a path.
            return false;

        case MP2::OBJ_STONELITHS:
            // Additional checks will follow
            return true;

        case MP2::OBJ_JAIL:
            return hero.GetKingdom().GetHeroes().size() < Kingdom::GetMaxHeroes();
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
            const uint32_t cursed = static_cast<uint32_t>( std::count_if( bag.begin(), bag.end(), []( const Artifact & art ) { return art.isAlchemistRemove(); } ) );

            const payment_t payment = PaymentConditions::ForAlchemist();

            return cursed > 0 && hero.GetKingdom().AllowPayment( payment );
        }
        default:
            // Did you add a new action object but forget to add AI interaction for it?
            assert( 0 );
            break;
        }

        return false;
    }

    struct HeroToMove
    {
        Heroes * hero = nullptr;
        int patrolCenter = -1;
        uint32_t patrolDistance = 0;
    };

    void addHeroToMove( Heroes * hero, std::vector<HeroToMove> & availableHeroes )
    {
        if ( hero->Modes( Heroes::PATROL ) ) {
            if ( hero->GetSquarePatrol() == 0 ) {
                DEBUG_LOG( DBG_AI, DBG_TRACE, hero->GetName() << " standing still. Skip turn." );
                hero->SetModes( Heroes::MOVED );
                return;
            }
        }

        hero->ResetModes( Heroes::WAITING | Heroes::MOVED | Heroes::SKIPPED_TURN );
        if ( !hero->MayStillMove( false, false ) ) {
            hero->SetModes( Heroes::MOVED );
        }
        else {
            availableHeroes.emplace_back();
            HeroToMove & heroInfo = availableHeroes.back();
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
        explicit ObjectValidator( const Heroes & hero, const AIWorldPathfinder & pathfinder )
            : _hero( hero )
            , _pathfinder( pathfinder )
        {}

        bool isValid( const int index )
        {
            auto iter = _validObjects.find( index );
            if ( iter != _validObjects.end() ) {
                return iter->second;
            }

            const bool valid = HeroesValidObject( _hero, index, _pathfinder );
            _validObjects[index] = valid;
            return valid;
        }

    private:
        const Heroes & _hero;
        const AIWorldPathfinder & _pathfinder;

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

    double ScaleWithDistance( double value, uint32_t distance )
    {
        if ( distance == 0 )
            return value;
        // scale non-linearly (more value lost as distance increases)
        return value - ( distance * std::log10( distance ) );
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

            if ( hero.GetColor() == castle->GetColor() ) {
                double value = castle->getVisitValue( hero );
                if ( value < 500 )
                    return valueToIgnore;

                return value;
            }
            else {
                double value = castle->getBuildingValue() * 150.0 + 3000;
                // If the castle is defenseless
                if ( !castle->GetActualArmy().isValid() )
                    value *= 1.25;
                return value;
            }
        }
        else if ( objectType == MP2::OBJ_HEROES ) {
            const Heroes * otherHero = tile.GetHeroes();
            assert( otherHero );
            if ( !otherHero ) {
                return valueToIgnore;
            }

            if ( hero.GetColor() == otherHero->GetColor() ) {
                if ( hero.getStatsValue() + 2 > otherHero->getStatsValue() )
                    return valueToIgnore;

                const double value = hero.getMeetingValue( *otherHero );
                // limit the max value of friendly hero meeting to 30 tiles
                return ( value < 250 ) ? valueToIgnore : std::min( value, 10000.0 );
            }
            return 5000.0;
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
            return 1000.0 * tile.QuantityArtifact().getArtifactValue();
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
        else if ( objectType == MP2::OBJ_SHRINE1 ) {
            return 100;
        }
        else if ( objectType == MP2::OBJ_SHRINE2 ) {
            return 250;
        }
        else if ( objectType == MP2::OBJ_SHRINE3 ) {
            return 500;
        }
        else if ( MP2::isHeroUpgradeObject( objectType ) ) {
            return 500.0;
        }
        else if ( MP2::isMonsterDwelling( objectType ) ) {
            return tile.QuantityTroop().GetStrength();
        }
        else if ( objectType == MP2::OBJ_STONELITHS ) {
            const MapsIndexes & list = world.GetTeleportEndPoints( index );
            for ( const int teleportIndex : list ) {
                if ( world.GetTiles( teleportIndex ).isFog( hero.GetColor() ) )
                    return 0;
            }
            return valueToIgnore;
        }
        else if ( objectType == MP2::OBJ_OBSERVATIONTOWER ) {
            const int fogCountToUncover = Maps::getFogTileCountToBeRevealed( index, Game::GetViewDistance( Game::VIEW_OBSERVATION_TOWER ), hero.GetColor() );
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
            const MapsIndexes & list = world.GetWhirlpoolEndPoints( index );
            for ( const int whirlpoolIndex : list ) {
                if ( world.GetTiles( whirlpoolIndex ).isFog( hero.GetColor() ) )
                    return -3000.0;
            }
            return valueToIgnore;
        }
        else if ( objectType == MP2::OBJ_BOAT ) {
            // Boat is not considered by AI as an action object. If this assertion blows up something is wrong with the logic.
            assert( 0 );
            return -dangerousTaskPenalty;
        }
        else if ( objectType == MP2::OBJ_MAGICWELL ) {
            if ( !hero.HaveSpellBook() ) {
                return -dangerousTaskPenalty;
            }
            if ( hero.GetSpellPoints() * 2 >= hero.GetMaxSpellPoints() ) {
                return -2000; // no reason to visit the well with no magic book or with half of points
            }
            return 0;
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
            const int eyeViewDistance = Game::GetViewDistance( Game::VIEW_MAGI_EYES );

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
        assert( hero.getAIRole() == Heroes::Role::FIGHTER );

        // In the future these hardcoded values could be configured by the mod
        // 1 tile distance is 100.0 value approximately
        const Maps::Tiles & tile = world.GetTiles( index );
        const MP2::MapObjectType objectType = tile.GetObject();

        if ( objectType == MP2::OBJ_CASTLE ) {
            const Castle * castle = world.getCastleEntrance( Maps::GetPoint( index ) );
            if ( !castle )
                return valueToIgnore;

            if ( hero.GetColor() == castle->GetColor() ) {
                double value = castle->getVisitValue( hero );
                if ( value < 500 )
                    return valueToIgnore;

                return value / 2;
            }
            else {
                double value = castle->getBuildingValue() * 500.0 + 15000;
                // If the castle is defenseless
                if ( !castle->GetActualArmy().isValid() )
                    value *= 2.5;
                return value;
            }
        }
        else if ( objectType == MP2::OBJ_HEROES ) {
            const Heroes * otherHero = tile.GetHeroes();
            assert( otherHero );
            if ( !otherHero ) {
                return valueToIgnore;
            }

            if ( hero.GetColor() == otherHero->GetColor() ) {
                if ( hero.getStatsValue() + 2 > otherHero->getStatsValue() )
                    return valueToIgnore;

                const double value = hero.getMeetingValue( *otherHero );
                // limit the max value of friendly hero meeting to 30 tiles
                return ( value < 250 ) ? valueToIgnore : std::min( value, 5000.0 );
            }
            return 12000.0;
        }
        else if ( objectType == MP2::OBJ_MONSTER ) {
            return 8000.0;
        }
        else if ( objectType == MP2::OBJ_MINES || objectType == MP2::OBJ_SAWMILL || objectType == MP2::OBJ_ALCHEMYLAB ) {
            if ( tile.QuantityColor() == hero.GetColor() ) {
                return -dangerousTaskPenalty; // don't even attempt to go here
            }
            return ( tile.QuantityResourceCount().first == Resource::GOLD ) ? 2000.0 : 1000.0;
        }
        else if ( objectType == MP2::OBJ_ABANDONEDMINE ) {
            if ( tile.QuantityColor() == hero.GetColor() ) {
                return -dangerousTaskPenalty; // don't even attempt to go here
            }
            return 5000.0;
        }
        else if ( MP2::isArtifactObject( objectType ) && tile.QuantityArtifact().isValid() ) {
            return 1500.0 * tile.QuantityArtifact().getArtifactValue();
        }
        else if ( MP2::isPickupObject( objectType ) ) {
            return 100.0;
        }
        else if ( MP2::isCaptureObject( objectType ) && MP2::isQuantityObject( objectType ) ) {
            // Objects like WATERWHEEL, WINDMILL and MAGICGARDEN if capture setting is enabled
            return 100.0;
        }
        else if ( objectType == MP2::OBJ_XANADU ) {
            return 2000.0;
        }
        else if ( objectType == MP2::OBJ_SHRINE1 ) {
            return 100;
        }
        else if ( objectType == MP2::OBJ_SHRINE2 ) {
            return 250;
        }
        else if ( objectType == MP2::OBJ_SHRINE3 ) {
            return 500;
        }
        else if ( MP2::isHeroUpgradeObject( objectType ) ) {
            return 750.0;
        }
        else if ( MP2::isMonsterDwelling( objectType ) ) {
            return tile.QuantityTroop().GetStrength();
        }
        else if ( objectType == MP2::OBJ_STONELITHS ) {
            const MapsIndexes & list = world.GetTeleportEndPoints( index );
            for ( const int teleportIndex : list ) {
                if ( world.GetTiles( teleportIndex ).isFog( hero.GetColor() ) )
                    return 0;
            }
            return valueToIgnore;
        }
        else if ( objectType == MP2::OBJ_OBSERVATIONTOWER ) {
            const int fogCountToUncover = Maps::getFogTileCountToBeRevealed( index, Game::GetViewDistance( Game::VIEW_OBSERVATION_TOWER ), hero.GetColor() );
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
            const MapsIndexes & list = world.GetWhirlpoolEndPoints( index );
            for ( const int whirlpoolIndex : list ) {
                if ( world.GetTiles( whirlpoolIndex ).isFog( hero.GetColor() ) )
                    return -3000.0;
            }
            return valueToIgnore;
        }
        else if ( objectType == MP2::OBJ_BOAT ) {
            // Boat is not considered by AI as an action object. If this assertion blows up something is wrong the the logic.
            assert( 0 );
            return -dangerousTaskPenalty;
        }
        else if ( objectType == MP2::OBJ_MAGICWELL ) {
            if ( !hero.HaveSpellBook() ) {
                return -dangerousTaskPenalty;
            }
            if ( hero.GetSpellPoints() * 2 >= hero.GetMaxSpellPoints() ) {
                return -2000; // no reason to visit the well with no magic book or with half of points
            }
            return 0;
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
            const int eyeViewDistance = Game::GetViewDistance( Game::VIEW_MAGI_EYES );

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

    double Normal::getObjectValue( const Heroes & hero, const int index, const double valueToIgnore, const uint32_t distanceToObject ) const
    {
        switch ( hero.getAIRole() ) {
        case Heroes::Role::HUNTER:
            return getHunterObjectValue( hero, index, valueToIgnore, distanceToObject );
        case Heroes::Role::FIGHTER:
            return getFighterObjectValue( hero, index, valueToIgnore, distanceToObject );
        default:
            // If you set a new type of a hero you must add the logic here.
            assert( 0 );
            break;
        }

        return 0;
    }

    int AI::Normal::getPriorityTarget( const Heroes & hero, double & maxPriority, int patrolIndex, uint32_t distanceLimit )
    {
        const double lowestPossibleValue = -1.0 * Maps::Ground::slowestMovePenalty * world.getSize();
        const bool heroInPatrolMode = patrolIndex != -1;
        const double heroStrength = hero.GetArmy().GetStrength();

        int priorityTarget = -1;
        maxPriority = lowestPossibleValue;
#ifdef WITH_DEBUG
        MP2::MapObjectType objectType = MP2::OBJ_ZERO;
#endif

        // pre-cache the pathfinder
        _pathfinder.reEvaluateIfNeeded( hero, isHeroAllowedToUseWhirlpool( hero ) );

        const uint32_t leftMovePoints = hero.GetMovePoints();

        ObjectValidator objectValidator( hero, _pathfinder );
        ObjectValueStorage valueStorage( hero, *this, lowestPossibleValue );

        for ( size_t idx = 0; idx < _mapObjects.size(); ++idx ) {
            const IndexObject & node = _mapObjects[idx];

            // Skip if hero in patrol mode and object outside of reach
            if ( heroInPatrolMode && Maps::GetApproximateDistance( node.first, patrolIndex ) > distanceLimit )
                continue;

            if ( objectValidator.isValid( node.first ) ) {
                uint32_t dist = _pathfinder.getDistance( node.first );
                if ( dist == 0 )
                    continue;

                double value = valueStorage.value( node, dist );

                const std::vector<IndexObject> & list = _pathfinder.getObjectsOnTheWay( node.first );
                for ( const IndexObject & pair : list ) {
                    if ( objectValidator.isValid( pair.first ) && std::binary_search( _mapObjects.begin(), _mapObjects.end(), pair ) ) {
                        const double extraValue = valueStorage.value( pair, 0 ); // object is on the way, we don't loose any movement points.
                        if ( extraValue > 0 ) {
                            // There is no need to reduce the quality of the object even if the path has others.
                            value += extraValue;
                        }
                    }
                }
                const RegionStats & regionStats = _regions[world.GetTiles( node.first ).GetRegion()];

                if ( heroStrength < regionStats.highestThreat ) {
                    const Castle * castle = world.getCastleEntrance( Maps::GetPoint( node.first ) );

                    if ( castle && ( castle->GetGarrisonStrength( &hero ) <= 0 || castle->GetColor() == hero.GetColor() ) )
                        value -= dangerousTaskPenalty / 2;
                    else
                        value -= dangerousTaskPenalty;
                }

                if ( dist > leftMovePoints ) {
                    // Distant object which is out of reach for the current turn must have lower priority.
                    dist = leftMovePoints + ( dist - leftMovePoints ) * 2;
                }

                value = ScaleWithDistance( value, dist );

                if ( dist && value > maxPriority ) {
                    maxPriority = value;
                    priorityTarget = node.first;
#ifdef WITH_DEBUG
                    objectType = static_cast<MP2::MapObjectType>( node.second );
#endif

                    DEBUG_LOG( DBG_AI, DBG_TRACE,
                               hero.GetName() << ": valid object at " << node.first << " value is " << value << " ("
                                              << MP2::StringObject( static_cast<MP2::MapObjectType>( node.second ) ) << ")" );
                }
            }
        }

        if ( priorityTarget != -1 ) {
            DEBUG_LOG( DBG_AI, DBG_INFO,
                       hero.GetName() << ": priority selected: " << priorityTarget << " value is " << maxPriority << " (" << MP2::StringObject( objectType ) << ")" );
        }
        else if ( !heroInPatrolMode ) {
            priorityTarget = _pathfinder.getFogDiscoveryTile( hero, isHeroAllowedToUseWhirlpool( hero ) );
            DEBUG_LOG( DBG_AI, DBG_INFO, hero.GetName() << " can't find an object. Scouting the fog of war at " << priorityTarget );
        }

        return priorityTarget;
    }

    void Normal::HeroesActionComplete( Heroes & hero )
    {
        Castle * castle = hero.inCastleMutable();
        if ( castle ) {
            ReinforceHeroInCastle( hero, *castle, castle->GetKingdom().GetFunds() );
        }
    }

    bool Normal::HeroesTurn( VecHeroes & heroes )
    {
        if ( heroes.empty() ) {
            // No heroes so we idicate that all heroes moved.
            return true;
        }

        std::vector<HeroToMove> availableHeroes;

        for ( Heroes * hero : heroes ) {
            addHeroToMove( hero, availableHeroes );
        }

        const double originalMonsterStrengthMultipler = _pathfinder.getCurrentArmyStrengthMultiplier();

        const int monsterStrengthMultiplierCount = 2;
        const double monsterStrengthMultipliers[monsterStrengthMultiplierCount] = { ARMY_STRENGTH_ADVANTAGE_MEDUIM, ARMY_STRENGTH_ADVANTAGE_SMALL };

        while ( !availableHeroes.empty() ) {
            Heroes * bestHero = availableHeroes.front().hero;
            double maxPriority = 0;
            int bestTargetIndex = -1;

            while ( true ) {
                for ( const HeroToMove & heroInfo : availableHeroes ) {
                    double priority = -1;
                    const int targetIndex = getPriorityTarget( *heroInfo.hero, priority, heroInfo.patrolCenter, heroInfo.patrolDistance );
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
                const double currentMonsterStrengthMultipler = _pathfinder.getCurrentArmyStrengthMultiplier();
                bool setNewMultipler = false;
                for ( int i = 0; i < monsterStrengthMultiplierCount; ++i ) {
                    if ( currentMonsterStrengthMultipler > monsterStrengthMultipliers[i] ) {
                        _pathfinder.setArmyStrengthMultplier( monsterStrengthMultipliers[i] );
                        setNewMultipler = true;
                        break;
                    }
                }

                if ( !setNewMultipler ) {
                    break;
                }
            }

            if ( bestTargetIndex == -1 ) {
                if ( availableHeroes.size() > 1 ) {
                    // Possibly heroes have nothing to do because one of them is blocking the way. Move a hero randomly and see what happens.
                    for ( HeroToMove & heroInfo : availableHeroes ) {
                        // Skip heroes who are in castles or on patrol.
                        if ( heroInfo.patrolCenter >= 0 && heroInfo.hero->inCastle() != nullptr ) {
                            continue;
                        }

                        if ( !_pathfinder.isHeroPossiblyBlockingWay( *heroInfo.hero, isHeroAllowedToUseWhirlpool( *heroInfo.hero ) ) ) {
                            continue;
                        }

                        const int targetIndex = _pathfinder.getNearestTileToMove( *heroInfo.hero, isHeroAllowedToUseWhirlpool( *heroInfo.hero ) );
                        if ( targetIndex != -1 ) {
                            bestTargetIndex = targetIndex;
                            bestHero = heroInfo.hero;
                            break;
                        }
                    }
                }

                if ( bestTargetIndex == -1 ) {
                    // Nothing to do. Stop everything
                    _pathfinder.setArmyStrengthMultplier( originalMonsterStrengthMultipler );
                    break;
                }
            }

            _pathfinder.reEvaluateIfNeeded( *bestHero, isHeroAllowedToUseWhirlpool( *bestHero ) );
            bestHero->GetPath().setPath( _pathfinder.buildPath( bestTargetIndex ), bestTargetIndex );

            const size_t heroesBefore = heroes.size();

            HeroesMove( *bestHero );

            if ( heroes.size() > heroesBefore ) {
                addHeroToMove( heroes.back(), availableHeroes );
            }

            for ( size_t i = 0; i < availableHeroes.size(); ) {
                if ( !availableHeroes[i].hero->MayStillMove( false, false ) ) {
                    availableHeroes[i].hero->SetModes( Heroes::MOVED );
                    availableHeroes.erase( availableHeroes.begin() + i );
                    continue;
                }

                ++i;
            }

            _pathfinder.setArmyStrengthMultplier( originalMonsterStrengthMultipler );
        }

        const bool allHeroesMoved = availableHeroes.empty();

        for ( HeroToMove & heroInfo : availableHeroes ) {
            if ( !heroInfo.hero->MayStillMove( false, false ) ) {
                heroInfo.hero->SetModes( Heroes::MOVED );
            }
        }

        _pathfinder.setArmyStrengthMultplier( originalMonsterStrengthMultipler );

        return allHeroesMoved;
    }
}
