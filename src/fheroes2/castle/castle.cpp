/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2023                                             *
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

#include <algorithm>
#include <array>
#include <cassert>
#include <iterator>
#include <ostream>

#include "agg_image.h"
#include "ai.h"
#include "army_troop.h"
#include "artifact.h"
#include "audio_manager.h"
#include "battle_board.h"
#include "battle_tower.h"
#include "castle.h"
#include "castle_building_info.h"
#include "dialog.h"
#include "difficulty.h"
#include "direction.h"
#include "game.h"
#include "game_static.h"
#include "ground.h"
#include "heroes.h"
#include "heroes_base.h"
#include "icn.h"
#include "image.h"
#include "kingdom.h"
#include "logging.h"
#include "luck.h"
#include "m82.h"
#include "maps.h"
#include "maps_tiles.h"
#include "morale.h"
#include "mp2.h"
#include "payment.h"
#include "profit.h"
#include "race.h"
#include "rand.h"
#include "resource.h"
#include "screen.h"
#include "serialize.h"
#include "settings.h"
#include "skill.h"
#include "spell.h"
#include "spell_storage.h"
#include "tools.h"
#include "translations.h"
#include "ui_dialog.h"
#include "week.h"
#include "world.h"

namespace
{
    const size_t maximumCastles = 72;

    const std::array<const char *, maximumCastles> defaultCastleNames
        = { gettext_noop( "Blackridge" ),   gettext_noop( "Pinehurst" ),   gettext_noop( "Woodhaven" ),    gettext_noop( "Hillstone" ),  gettext_noop( "Whiteshield" ),
            gettext_noop( "Bloodreign" ),   gettext_noop( "Dragontooth" ), gettext_noop( "Greywind" ),     gettext_noop( "Blackwind" ),  gettext_noop( "Portsmith" ),
            gettext_noop( "Middle Gate" ),  gettext_noop( "Tundara" ),     gettext_noop( "Vulcania" ),     gettext_noop( "Sansobar" ),   gettext_noop( "Atlantium" ),
            gettext_noop( "Baywatch" ),     gettext_noop( "Wildabar" ),    gettext_noop( "Fountainhead" ), gettext_noop( "Vertigo" ),    gettext_noop( "Winterkill" ),
            gettext_noop( "Nightshadow" ),  gettext_noop( "Sandcaster" ),  gettext_noop( "Lakeside" ),     gettext_noop( "Olympus" ),    gettext_noop( "Brindamoor" ),
            gettext_noop( "Burlock" ),      gettext_noop( "Xabran" ),      gettext_noop( "Dragadune" ),    gettext_noop( "Alamar" ),     gettext_noop( "Kalindra" ),
            gettext_noop( "Blackfang" ),    gettext_noop( "Basenji" ),     gettext_noop( "Algary" ),       gettext_noop( "Sorpigal" ),   gettext_noop( "New Dawn" ),
            gettext_noop( "Erliquin" ),     gettext_noop( "Avone" ),       gettext_noop( "Big Oak" ),      gettext_noop( "Hampshire" ),  gettext_noop( "Chandler" ),
            gettext_noop( "South Mill" ),   gettext_noop( "Weed Patch" ),  gettext_noop( "Roc Haven" ),    gettext_noop( "Avalon" ),     gettext_noop( "Antioch" ),
            gettext_noop( "Brownston" ),    gettext_noop( "Weddington" ),  gettext_noop( "Whittingham" ),  gettext_noop( "Westfork" ),   gettext_noop( "Hilltop" ),
            gettext_noop( "Yorksford" ),    gettext_noop( "Sherman" ),     gettext_noop( "Roscomon" ),     gettext_noop( "Elk's Head" ), gettext_noop( "Cathcart" ),
            gettext_noop( "Viper's Nest" ), gettext_noop( "Pig's Eye" ),   gettext_noop( "Blacksford" ),   gettext_noop( "Burton" ),     gettext_noop( "Blackburn" ),
            gettext_noop( "Lankershire" ),  gettext_noop( "Lombard" ),     gettext_noop( "Timberhill" ),   gettext_noop( "Fenton" ),     gettext_noop( "Troy" ),
            gettext_noop( "Forder Oaks" ),  gettext_noop( "Meramec" ),     gettext_noop( "Quick Silver" ), gettext_noop( "Westmoor" ),   gettext_noop( "Willow" ),
            gettext_noop( "Sheltemburg" ),  gettext_noop( "Corackston" ) };
}

Castle::Castle()
    : race( Race::NONE )
    , building( 0 )
    , captain( *this )
    , army( nullptr )
{
    std::fill( dwelling, dwelling + CASTLEMAXMONSTER, 0 );
    army.SetCommander( &captain );
}

Castle::Castle( int32_t cx, int32_t cy, int rc )
    : MapPosition( fheroes2::Point( cx, cy ) )
    , race( rc )
    , building( 0 )
    , captain( *this )
    , army( nullptr )
{
    std::fill( dwelling, dwelling + CASTLEMAXMONSTER, 0 );
    army.SetCommander( &captain );
}

void Castle::LoadFromMP2( const std::vector<uint8_t> & data )
{
    assert( data.size() == MP2::SIZEOFMP2CASTLE );

    // Structure containing information about town or castle.
    //
    // - uint8_t (1 byte)
    //     Owner color. Possible values:
    //     00 - blue
    //     01 - green
    //     02 - red
    //     03 - yellow
    //     04 - orange
    //     05 - purple
    //     255 - none
    //
    // - uint8_t (1 byte)
    //     Does the town / castle have custom buildings set by map creator?
    //
    // - uint16_t (2 bytes)
    //    Bitfield containing common buildings within the town / castle.
    //     0000 0000 0000 0010 : Thieves' Guild
    //     0000 0000 0000 0100 : Tavern
    //     0000 0000 0000 1000 : Shipyard
    //     0000 0000 0001 0000 : Well
    //     0000 0000 1000 0000 : Statue
    //     0000 0001 0000 0000 : Left Turret
    //     0000 0010 0000 0000 : Right Turret
    //     0000 0100 0000 0000 : Marketplace
    //     0000 1000 0000 0000 : First monster level growth building
    //     0001 0000 0000 0000 : Moat
    //     0010 0000 0000 0000 : Special building
    //
    // - uint16_t (2 bytes)
    //     Bitfield containing information about built dwellings in the town / castle.
    //     0000 0000 0000 1000 : level 1 dwelling
    //     0000 0000 0001 0000 : level 2 dwelling
    //     0000 0000 0010 0000 : level 3 dwelling
    //     0000 0000 0100 0000 : level 4 dwelling
    //     0000 0000 1000 0000 : level 5 dwelling
    //     0000 0001 0000 0000 : level 6 dwelling
    //     0000 0010 0000 0000 : upgraded level 2 dwelling
    //     0000 0100 0000 0000 : upgraded level 3 dwelling
    //     0000 1000 0000 0000 : upgraded level 4 dwelling
    //     0001 0000 0000 0000 : upgraded level 5 dwelling
    //     0010 0000 0000 0000 : upgraded level 6 dwelling
    //
    // - uint8_t (1 byte)
    //     Magic Guild level.
    //
    // - uint8_t (1 byte)
    //     Does the town / castle have custom set defenders?
    //
    // - uint8_t (1 byte)
    //    Custom defender monster type in army slot 1.
    //
    // - uint8_t (1 byte)
    //    Custom defender monster type in army slot 2.
    //
    // - uint8_t (1 byte)
    //    Custom defender monster type in army slot 3.
    //
    // - uint8_t (1 byte)
    //    Custom defender monster type in army slot 4.
    //
    // - uint8_t (1 byte)
    //    Custom defender monster type in army slot 5.
    //
    // - uint16_t (2 bytes)
    //    The number of custom defender monsters in army slot 1.
    //
    // - uint16_t (2 bytes)
    //    The number of custom defender monsters in army slot 2.
    //
    // - uint16_t (2 bytes)
    //    The number of custom defender monsters in army slot 3.
    //
    // - uint16_t (2 bytes)
    //    The number of custom defender monsters in army slot 4.
    //
    // - uint16_t (2 bytes)
    //    The number of custom defender monsters in army slot 5.
    //
    // - uint8_t (1 byte)
    //     Does the town / castle have captain?
    //
    // - uint8_t (1 byte)
    //     Does the town / castle have a specified name?
    //
    // - string of 13 bytes
    //    Null terminated string of custom town / castle name.
    //
    // - uint8_t (1 byte)
    //    Town / castle faction type. Possible values
    //    00 - knight
    //    01 - barbarian
    //    02 - sorceress
    //    03 - warlock
    //    04 - wizard
    //    05 - necromancer
    //    06 - random
    //
    // - uint8_t (1 byte)
    //    Is it a castle (is castle building being built)?
    //
    // - uint8_t (1 byte)
    //    Is it allowed to build a castle?
    //
    // - unused 29 bytes
    //    Always zeros.

    StreamBuf dataStream( data );

    const uint8_t ownerColor = dataStream.get();
    switch ( ownerColor ) {
    case 0:
        SetColor( Color::BLUE );
        break;
    case 1:
        SetColor( Color::GREEN );
        break;
    case 2:
        SetColor( Color::RED );
        break;
    case 3:
        SetColor( Color::YELLOW );
        break;
    case 4:
        SetColor( Color::ORANGE );
        break;
    case 5:
        SetColor( Color::PURPLE );
        break;
    default:
        SetColor( Color::NONE );
        break;
    }

    const bool hasCustomBuildings = ( dataStream.get() != 0 );
    if ( hasCustomBuildings ) {
        // Common buildings.
        const uint16_t commonBuildings = dataStream.getLE16();
        if ( 0x0002 & commonBuildings )
            building |= BUILD_THIEVESGUILD;
        if ( 0x0004 & commonBuildings )
            building |= BUILD_TAVERN;
        if ( 0x0008 & commonBuildings )
            building |= BUILD_SHIPYARD;
        if ( 0x0010 & commonBuildings )
            building |= BUILD_WELL;
        if ( 0x0080 & commonBuildings )
            building |= BUILD_STATUE;
        if ( 0x0100 & commonBuildings )
            building |= BUILD_LEFTTURRET;
        if ( 0x0200 & commonBuildings )
            building |= BUILD_RIGHTTURRET;
        if ( 0x0400 & commonBuildings )
            building |= BUILD_MARKETPLACE;
        if ( 0x1000 & commonBuildings )
            building |= BUILD_MOAT;
        if ( 0x0800 & commonBuildings )
            building |= BUILD_WEL2;
        if ( 0x2000 & commonBuildings )
            building |= BUILD_SPEC;

        // Existing dwellings.
        const uint16_t existingDwellings = dataStream.getLE16();
        if ( 0x0008 & existingDwellings )
            building |= DWELLING_MONSTER1;
        if ( 0x0010 & existingDwellings )
            building |= DWELLING_MONSTER2;
        if ( 0x0020 & existingDwellings )
            building |= DWELLING_MONSTER3;
        if ( 0x0040 & existingDwellings )
            building |= DWELLING_MONSTER4;
        if ( 0x0080 & existingDwellings )
            building |= DWELLING_MONSTER5;
        if ( 0x0100 & existingDwellings )
            building |= DWELLING_MONSTER6;
        if ( 0x0200 & existingDwellings )
            building |= DWELLING_UPGRADE2 | DWELLING_MONSTER2;
        if ( 0x0400 & existingDwellings )
            building |= DWELLING_UPGRADE3 | DWELLING_MONSTER3;
        if ( 0x0800 & existingDwellings )
            building |= DWELLING_UPGRADE4 | DWELLING_MONSTER4;
        if ( 0x1000 & existingDwellings )
            building |= DWELLING_UPGRADE5 | DWELLING_MONSTER5;
        if ( 0x2000 & existingDwellings )
            building |= DWELLING_UPGRADE6 | DWELLING_MONSTER6;

        // magic tower
        const uint8_t magicGuildLevel = dataStream.get();
        if ( 0 < magicGuildLevel )
            building |= BUILD_MAGEGUILD1;
        if ( 1 < magicGuildLevel )
            building |= BUILD_MAGEGUILD2;
        if ( 2 < magicGuildLevel )
            building |= BUILD_MAGEGUILD3;
        if ( 3 < magicGuildLevel )
            building |= BUILD_MAGEGUILD4;
        if ( 4 < magicGuildLevel )
            building |= BUILD_MAGEGUILD5;
    }
    else {
        // Skip reading 5 bytes corresponding to custom buildings for the town / castle.
        dataStream.skip( 5 );

        // Set default buildings.
        building |= DWELLING_MONSTER1;
        uint32_t dwelling2 = 0;
        switch ( Game::getDifficulty() ) {
        case Difficulty::EASY:
            dwelling2 = 75;
            break;
        case Difficulty::NORMAL:
            dwelling2 = 50;
            break;
        case Difficulty::HARD:
            dwelling2 = 25;
            break;
        case Difficulty::EXPERT:
            dwelling2 = 10;
            break;
        default:
            break;
        }
        if ( dwelling2 && dwelling2 >= Rand::Get( 1, 100 ) )
            building |= DWELLING_MONSTER2;
    }

    const bool customDefenders = ( dataStream.get() != 0 );
    if ( customDefenders ) {
        Troop troops[5];

        // set monster id
        for ( Troop & troop : troops )
            troop.SetMonster( dataStream.get() + 1 );

        // set count
        for ( Troop & troop : troops )
            troop.SetCount( dataStream.getLE16() );

        army.Assign( troops, std::end( troops ) );
        SetModes( CUSTOMARMY );
    }
    else {
        // Skip 15 bytes as custom defenders are not set.
        dataStream.skip( 15 );
    }

    const bool isCaptainAvailable = ( dataStream.get() != 0 );
    if ( isCaptainAvailable ) {
        building |= BUILD_CAPTAIN;
    }

    const bool isCustomTownNameSet = ( dataStream.get() != 0 );
    if ( isCustomTownNameSet ) {
        name = dataStream.toString( 13 );
    }
    else {
        // Skip 13 bytes since the name is not set.
        dataStream.skip( 13 );
    }

    const uint8_t castleFaction = dataStream.get();
    switch ( castleFaction ) {
    case 0:
        race = Race::KNGT;
        break;
    case 1:
        race = Race::BARB;
        break;
    case 2:
        race = Race::SORC;
        break;
    case 3:
        race = Race::WRLK;
        break;
    case 4:
        race = Race::WZRD;
        break;
    case 5:
        race = Race::NECR;
        break;
    default: {
        const uint32_t kingdomRace = Players::GetPlayerRace( GetColor() );
        race = ( Color::NONE != GetColor() && ( Race::ALL & kingdomRace ) ? kingdomRace : Race::Rand() );
        break;
    }
    }

    const bool isCastleBuilt = ( dataStream.get() != 0 );
    if ( isCastleBuilt ) {
        building |= BUILD_CASTLE;
    }
    else {
        building |= BUILD_TENT;
    }

    const bool allowToBuildCastle = ( dataStream.get() != 0 );
    if ( allowToBuildCastle ) {
        ResetModes( ALLOWCASTLE );
    }
    else {
        SetModes( ALLOWCASTLE );
    }

    // Skip the rest of 29 bytes.

    PostLoad();
}

void Castle::PostLoad()
{
    // dwelling pack
    if ( building & DWELLING_MONSTER1 )
        dwelling[0] = Monster( race, DWELLING_MONSTER1 ).GetGrown();
    if ( building & DWELLING_MONSTER2 )
        dwelling[1] = Monster( race, DWELLING_MONSTER2 ).GetGrown();
    if ( building & DWELLING_UPGRADE2 )
        dwelling[1] = Monster( race, DWELLING_UPGRADE2 ).GetGrown();
    if ( building & DWELLING_MONSTER3 )
        dwelling[2] = Monster( race, DWELLING_MONSTER3 ).GetGrown();
    if ( building & DWELLING_UPGRADE3 )
        dwelling[2] = Monster( race, DWELLING_UPGRADE3 ).GetGrown();
    if ( building & DWELLING_MONSTER4 )
        dwelling[3] = Monster( race, DWELLING_MONSTER4 ).GetGrown();
    if ( building & DWELLING_UPGRADE4 )
        dwelling[3] = Monster( race, DWELLING_UPGRADE4 ).GetGrown();
    if ( building & DWELLING_MONSTER5 )
        dwelling[4] = Monster( race, DWELLING_MONSTER5 ).GetGrown();
    if ( building & DWELLING_UPGRADE5 )
        dwelling[4] = Monster( race, DWELLING_UPGRADE5 ).GetGrown();
    if ( building & DWELLING_MONSTER6 )
        dwelling[5] = Monster( race, DWELLING_MONSTER6 ).GetGrown();
    if ( building & DWELLING_UPGRADE6 )
        dwelling[5] = Monster( race, DWELLING_UPGRADE6 ).GetGrown();
    if ( building & DWELLING_UPGRADE7 )
        dwelling[5] = Monster( race, DWELLING_UPGRADE7 ).GetGrown();

    // fix upgrade dwelling dependent from race
    switch ( race ) {
    case Race::BARB:
        building &= ~( DWELLING_UPGRADE3 | DWELLING_UPGRADE6 );
        break;
    case Race::SORC:
        building &= ~( DWELLING_UPGRADE5 | DWELLING_UPGRADE6 );
        break;
    case Race::WRLK:
        building &= ~( DWELLING_UPGRADE2 | DWELLING_UPGRADE3 | DWELLING_UPGRADE5 );
        break;
    case Race::WZRD:
        building &= ~( DWELLING_UPGRADE2 | DWELLING_UPGRADE4 );
        break;
    case Race::NECR:
        building &= ~DWELLING_UPGRADE6;
        break;
    default:
        break;
    }

    army.SetColor( GetColor() );

    // fix captain
    if ( building & BUILD_CAPTAIN ) {
        captain.LoadDefaults( HeroBase::CAPTAIN, race );
        captain.SetSpellPoints( captain.GetMaxSpellPoints() );
    }

    // MageGuild
    mageguild.initialize( race, HaveLibraryCapability() );
    // educate heroes and captain
    EducateHeroes();

    // AI troops auto pack for gray towns
    if ( Color::NONE == GetColor() && !Modes( CUSTOMARMY ) ) {
        // towns get 4 reinforcements at the start of the game
        for ( int i = 0; i < 4; ++i )
            JoinRNDArmy();
    }

    // fix shipyard
    if ( !HaveNearlySea() )
        building &= ~BUILD_SHIPYARD;

    // remove tavern from necromancer castle
    if ( Race::NECR == race && ( building & BUILD_TAVERN ) ) {
        building &= ~BUILD_TAVERN;
        if ( Settings::Get().isCurrentMapPriceOfLoyalty() )
            building |= BUILD_SHRINE;
    }

    SetModes( ALLOWBUILD );

    // end
    DEBUG_LOG( DBG_GAME, DBG_INFO,
               ( building & BUILD_CASTLE ? "castle" : "town" ) << ": " << name << ", color: " << Color::String( GetColor() ) << ", race: " << Race::String( race ) )
}

uint32_t Castle::CountBuildings() const
{
    const uint32_t tavern = ( race == Race::NECR ? ( Settings::Get().isCurrentMapPriceOfLoyalty() ? BUILD_SHRINE : BUILD_NOTHING ) : BUILD_TAVERN );

    return CountBits( building
                      & ( BUILD_THIEVESGUILD | tavern | BUILD_SHIPYARD | BUILD_WELL | BUILD_STATUE | BUILD_LEFTTURRET | BUILD_RIGHTTURRET | BUILD_MARKETPLACE | BUILD_WEL2
                          | BUILD_MOAT | BUILD_SPEC | BUILD_CAPTAIN | BUILD_CASTLE | BUILD_MAGEGUILD1 | DWELLING_MONSTER1 | DWELLING_MONSTER2 | DWELLING_MONSTER3
                          | DWELLING_MONSTER4 | DWELLING_MONSTER5 | DWELLING_MONSTER6 ) );
}

bool Castle::isPosition( const fheroes2::Point & pt ) const
{
    const fheroes2::Point & mp = GetCenter();

    /*
              -
             ---
            -+++-
            ++X++
    */

    return ( ( pt.x >= mp.x - 1 && pt.x <= mp.x + 1 && ( pt.y == mp.y - 1 || pt.y == mp.y ) ) || ( ( pt.x == mp.x - 2 || pt.x == mp.x + 2 ) && pt.y == mp.y ) );
}

void Castle::EducateHeroes()
{
    if ( GetLevelMageGuild() == 0 ) {
        return;
    }

    Heroes * hero = world.GetHero( *this );
    if ( hero != nullptr ) {
        MageGuildEducateHero( *hero );
    }

    if ( captain.isValid() ) {
        MageGuildEducateHero( captain );
    }
}

int Castle::getBuildingValue() const
{
    int value = CountBuildings();

    // Additional value for the most important buildings
    if ( isBuild( BUILD_CASTLE ) )
        value += 5;

    if ( isBuild( DWELLING_MONSTER6 ) )
        value += 6;

    if ( race == Race::WRLK && isBuild( DWELLING_UPGRADE7 ) )
        value += 2;

    // DWELLING_UPGRADE7 resolves to a negative, can't use <= operator
    for ( uint32_t upgrade = DWELLING_UPGRADE2; upgrade <= DWELLING_UPGRADE6; upgrade <<= 1 ) {
        if ( isBuild( upgrade ) )
            ++value;
    }

    int increase = 1;
    for ( uint32_t guild = BUILD_MAGEGUILD2; guild <= BUILD_MAGEGUILD5; guild <<= 1 ) {
        if ( isBuild( guild ) )
            value += increase;
        ++increase;
    }

    return value;
}

Troops Castle::getAvailableArmy( Funds potentialBudget ) const
{
    Troops reinforcement( army.getTroops() );
    for ( uint32_t dw = DWELLING_MONSTER6; dw >= DWELLING_MONSTER1; dw >>= 1 ) {
        if ( isBuild( dw ) ) {
            const Monster monster( race, GetActualDwelling( dw ) );
            const uint32_t available = getMonstersInDwelling( dw );

            uint32_t couldRecruit = potentialBudget.getLowestQuotient( monster.GetCost() );
            if ( available < couldRecruit )
                couldRecruit = available;

            if ( couldRecruit > 0 ) {
                potentialBudget -= ( monster.GetCost() * couldRecruit );
                reinforcement.PushBack( monster, couldRecruit );
            }
        }
    }
    return reinforcement;
}

double Castle::getArmyRecruitmentValue() const
{
    return getAvailableArmy( GetKingdom().GetFunds() ).GetStrength();
}

double Castle::getVisitValue( const Heroes & hero ) const
{
    const Troops & heroArmy = hero.GetArmy();
    Troops futureArmy( heroArmy );
    const double heroArmyStrength = futureArmy.GetStrength();

    Funds potentialFunds = GetKingdom().GetFunds();

    double spellValue = 0;
    const int mageGuildLevel = GetLevelMageGuild();
    if ( mageGuildLevel > 0 ) {
        const int spellPower = hero.GetPower();
        const SpellStorage & guildSpells = mageguild.GetSpells( GetLevelMageGuild(), isLibraryBuild() );
        for ( const Spell & spell : guildSpells ) {
            if ( hero.CanLearnSpell( spell ) && !hero.HaveSpell( spell, true ) ) {
                spellValue += spell.getStrategicValue( heroArmyStrength, hero.GetMaxSpellPoints(), spellPower );
            }
        }

        if ( !hero.HaveSpellBook() && spellValue > 0 ) {
            const payment_t payment = PaymentConditions::BuySpellBook();
            if ( potentialFunds < payment || hero.GetBagArtifacts().isFull() ) {
                // Since the hero does not have a magic book and cannot buy any then spells are useless.
                spellValue = 0;
            }
            else {
                // The hero does not have a magic book but it can buy one. Let's make the visit little more valuable.
                potentialFunds -= payment;
                spellValue += 50;
            }
        }
    }

    for ( size_t i = 0; i < futureArmy.Size(); ++i ) {
        Troop * troop = futureArmy.GetTroop( i );
        if ( troop != nullptr && troop->isValid() ) {
            const payment_t payment = troop->GetTotalUpgradeCost();

            if ( GetRace() == troop->GetRace() && isBuild( troop->GetUpgrade().GetDwelling() ) && potentialFunds >= payment ) {
                potentialFunds -= payment;
                troop->Upgrade();
            }
        }
    }

    const double upgradeStrength = futureArmy.GetStrength() - heroArmyStrength;

    return spellValue + upgradeStrength + futureArmy.getReinforcementValue( getAvailableArmy( potentialFunds ) );
}

bool Castle::isExactBuildingBuilt( const uint32_t buildingToCheck ) const
{
    assert( CountBits( buildingToCheck ) == 1 );

    // This building is not built at all
    if ( ( building & buildingToCheck ) == 0 ) {
        return false;
    }

    auto checkBuilding = [this]( const uint32_t expectedLevels, const uint32_t allPossibleLevels ) {
        // All expected levels should be built
        assert( ( building & expectedLevels ) == expectedLevels );

        // Only the expected levels of all possible levels should be built
        return ( building & allPossibleLevels ) == expectedLevels;
    };

    if ( buildingToCheck & BUILD_MAGEGUILD ) {
        switch ( buildingToCheck ) {
        case BUILD_MAGEGUILD1:
            return checkBuilding( BUILD_MAGEGUILD1, BUILD_MAGEGUILD );
        case BUILD_MAGEGUILD2:
            return checkBuilding( BUILD_MAGEGUILD1 | BUILD_MAGEGUILD2, BUILD_MAGEGUILD );
        case BUILD_MAGEGUILD3:
            return checkBuilding( BUILD_MAGEGUILD1 | BUILD_MAGEGUILD2 | BUILD_MAGEGUILD3, BUILD_MAGEGUILD );
        case BUILD_MAGEGUILD4:
            return checkBuilding( BUILD_MAGEGUILD1 | BUILD_MAGEGUILD2 | BUILD_MAGEGUILD3 | BUILD_MAGEGUILD4, BUILD_MAGEGUILD );
        case BUILD_MAGEGUILD5:
            return checkBuilding( BUILD_MAGEGUILD1 | BUILD_MAGEGUILD2 | BUILD_MAGEGUILD3 | BUILD_MAGEGUILD4 | BUILD_MAGEGUILD5, BUILD_MAGEGUILD );
        default:
            assert( 0 );
        }
    }

    if ( buildingToCheck & ( DWELLING_MONSTERS | DWELLING_UPGRADES ) ) {
        switch ( buildingToCheck ) {
        case DWELLING_MONSTER1:
            // Level 1 dwellings have no upgrades
            return true;
        case DWELLING_MONSTER2:
            return checkBuilding( DWELLING_MONSTER2, DWELLING_MONSTER2 | DWELLING_UPGRADE2 );
        case DWELLING_MONSTER3:
            return checkBuilding( DWELLING_MONSTER3, DWELLING_MONSTER3 | DWELLING_UPGRADE3 );
        case DWELLING_MONSTER4:
            return checkBuilding( DWELLING_MONSTER4, DWELLING_MONSTER4 | DWELLING_UPGRADE4 );
        case DWELLING_MONSTER5:
            return checkBuilding( DWELLING_MONSTER5, DWELLING_MONSTER5 | DWELLING_UPGRADE5 );
        case DWELLING_MONSTER6:
            // Take the Black Dragon upgrade (DWELLING_UPGRADE7) into account
            return checkBuilding( DWELLING_MONSTER6, DWELLING_MONSTER6 | DWELLING_UPGRADE6 | DWELLING_UPGRADE7 );

        case DWELLING_UPGRADE2:
            return checkBuilding( DWELLING_MONSTER2 | DWELLING_UPGRADE2, DWELLING_MONSTER2 | DWELLING_UPGRADE2 );
        case DWELLING_UPGRADE3:
            return checkBuilding( DWELLING_MONSTER3 | DWELLING_UPGRADE3, DWELLING_MONSTER3 | DWELLING_UPGRADE3 );
        case DWELLING_UPGRADE4:
            return checkBuilding( DWELLING_MONSTER4 | DWELLING_UPGRADE4, DWELLING_MONSTER4 | DWELLING_UPGRADE4 );
        case DWELLING_UPGRADE5:
            return checkBuilding( DWELLING_MONSTER5 | DWELLING_UPGRADE5, DWELLING_MONSTER5 | DWELLING_UPGRADE5 );
        case DWELLING_UPGRADE6:
            // Take the Black Dragon upgrade (DWELLING_UPGRADE7) into account
            return checkBuilding( DWELLING_MONSTER6 | DWELLING_UPGRADE6, DWELLING_MONSTER6 | DWELLING_UPGRADE6 | DWELLING_UPGRADE7 );
        case DWELLING_UPGRADE7:
            // Black Dragon upgrade
            return checkBuilding( DWELLING_MONSTER6 | DWELLING_UPGRADE6 | DWELLING_UPGRADE7, DWELLING_MONSTER6 | DWELLING_UPGRADE6 | DWELLING_UPGRADE7 );

        default:
            assert( 0 );
        }
    }

    return true;
}

uint32_t * Castle::GetDwelling( uint32_t dw )
{
    if ( isBuild( dw ) )
        switch ( dw ) {
        case DWELLING_MONSTER1:
            return &dwelling[0];
        case DWELLING_MONSTER2:
        case DWELLING_UPGRADE2:
            return &dwelling[1];
        case DWELLING_MONSTER3:
        case DWELLING_UPGRADE3:
            return &dwelling[2];
        case DWELLING_MONSTER4:
        case DWELLING_UPGRADE4:
            return &dwelling[3];
        case DWELLING_MONSTER5:
        case DWELLING_UPGRADE5:
            return &dwelling[4];
        case DWELLING_MONSTER6:
        case DWELLING_UPGRADE6:
        case DWELLING_UPGRADE7:
            return &dwelling[5];
        default:
            break;
        }
    return nullptr;
}

void Castle::ActionNewDay()
{
    EducateHeroes();

    SetModes( ALLOWBUILD );
}

void Castle::ActionNewWeek()
{
    // Skip the first week
    if ( world.CountWeek() < 2 ) {
        return;
    }

    static const std::array<uint32_t, 12> allDwellings
        = { DWELLING_MONSTER1, DWELLING_MONSTER2, DWELLING_MONSTER3, DWELLING_MONSTER4, DWELLING_MONSTER5, DWELLING_MONSTER6,
            DWELLING_UPGRADE2, DWELLING_UPGRADE3, DWELLING_UPGRADE4, DWELLING_UPGRADE5, DWELLING_UPGRADE6, DWELLING_UPGRADE7 };

    const bool isNeutral = GetColor() == Color::NONE;

    // Increase the population
    if ( world.GetWeekType().GetType() != WeekName::PLAGUE ) {
        static const std::array<uint32_t, 6> basicDwellings
            = { DWELLING_MONSTER1, DWELLING_MONSTER2, DWELLING_MONSTER3, DWELLING_MONSTER4, DWELLING_MONSTER5, DWELLING_MONSTER6 };

        // Normal population growth
        for ( const uint32_t dwellingId : basicDwellings ) {
            uint32_t * dwellingPtr = GetDwelling( dwellingId );

            // Such dwelling (or its upgrade) has not been built
            if ( dwellingPtr == nullptr ) {
                continue;
            }

            uint32_t growth = Monster( race, GetActualDwelling( dwellingId ) ).GetGrown();

            // The well is built
            if ( building & BUILD_WELL ) {
                growth += GetGrownWell();
            }

            // The Horde building is built
            if ( ( dwellingId == DWELLING_MONSTER1 ) && ( building & BUILD_WEL2 ) ) {
                growth += GetGrownWel2();
            }

            if ( isControlAI() && !isNeutral ) {
                growth = static_cast<uint32_t>( growth * Difficulty::GetUnitGrowthBonusForAI( Game::getDifficulty() ) );
            }

            // Neutral towns always have 50% population growth
            if ( isNeutral ) {
                growth /= 2;
            }

            *dwellingPtr += growth;
        }

        // Week Of
        if ( world.GetWeekType().GetType() == WeekName::MONSTERS && !world.BeginMonth() ) {
            for ( const uint32_t dwellingId : allDwellings ) {
                // A building of exactly this level should be built (its upgraded versions should not be considered)
                if ( !isExactBuildingBuilt( dwellingId ) ) {
                    continue;
                }

                const Monster mons( race, dwellingId );

                if ( !mons.isValid() || mons.GetID() != world.GetWeekType().GetMonster() ) {
                    continue;
                }

                uint32_t * dwellingPtr = GetDwelling( dwellingId );
                assert( dwellingPtr != nullptr );

                *dwellingPtr += GetGrownWeekOf();

                break;
            }
        }

        // Neutral town: increase the garrison
        if ( isNeutral ) {
            JoinRNDArmy();
            // The probability that a town will get additional troops is 40%, castle always gets them
            if ( isCastle() || Rand::Get( 1, 100 ) <= 40 ) {
                JoinRNDArmy();
            }
        }
    }

    // Monthly population growth bonuses should be calculated taking the weekly growth into account
    if ( world.BeginMonth() ) {
        assert( world.GetMonth() > 1 );

        // Population halved
        if ( world.GetWeekType().GetType() == WeekName::PLAGUE ) {
            for ( uint32_t & dwellingRef : dwelling ) {
                dwellingRef /= 2;
            }
        }
        // Month Of
        else if ( world.GetWeekType().GetType() == WeekName::MONSTERS ) {
            for ( const uint32_t dwellingId : allDwellings ) {
                // A building of exactly this level should be built (its upgraded versions should not be considered)
                if ( !isExactBuildingBuilt( dwellingId ) ) {
                    continue;
                }

                const Monster mons( race, dwellingId );

                if ( !mons.isValid() || mons.GetID() != world.GetWeekType().GetMonster() ) {
                    continue;
                }

                uint32_t * dwellingPtr = GetDwelling( dwellingId );
                assert( dwellingPtr != nullptr );

                *dwellingPtr += *dwellingPtr * GetGrownMonthOf() / 100;

                break;
            }
        }
    }
}

void Castle::ActionNewMonth() const
{
    // Do nothing.
}

void Castle::ChangeColor( int cl )
{
    SetColor( cl );
    army.SetColor( cl );
}

int Castle::GetLevelMageGuild() const
{
    if ( building & BUILD_MAGEGUILD5 )
        return 5;
    else if ( building & BUILD_MAGEGUILD4 )
        return 4;
    else if ( building & BUILD_MAGEGUILD3 )
        return 3;
    else if ( building & BUILD_MAGEGUILD2 )
        return 2;
    else if ( building & BUILD_MAGEGUILD1 )
        return 1;

    return 0;
}

bool Castle::HaveLibraryCapability() const
{
    return race == Race::WZRD;
}

bool Castle::isLibraryBuild() const
{
    return race == Race::WZRD && isBuild( BUILD_SPEC );
}

void Castle::MageGuildEducateHero( HeroBase & hero ) const
{
    mageguild.educateHero( hero, GetLevelMageGuild(), isLibraryBuild() );
}

bool Castle::isFortificationBuild() const
{
    return race == Race::KNGT && isBuild( BUILD_SPEC );
}

const char * Castle::GetStringBuilding( uint32_t build, int race )
{
    return fheroes2::getBuildingName( race, static_cast<building_t>( build ) );
}

const char * Castle::GetDescriptionBuilding( uint32_t build, int race )
{
    return fheroes2::getBuildingDescription( race, static_cast<building_t>( build ) );
}

bool Castle::AllowBuyHero( std::string * msg ) const
{
    const Heroes * hero = world.GetHero( *this );
    if ( hero != nullptr ) {
        if ( msg ) {
            *msg = _( "Cannot recruit - you already have a Hero in this town." );
        }
        return false;
    }

    const Kingdom & myKingdom = GetKingdom();
    if ( !myKingdom.AllowRecruitHero( false ) ) {
        if ( msg ) {
            *msg = _( "Cannot recruit - you have too many Heroes." );
        }
        return false;
    }

    if ( !myKingdom.AllowRecruitHero( true ) ) {
        if ( msg ) {
            *msg = _( "Cannot afford a Hero" );
        }
        return false;
    }

    return true;
}

Heroes * Castle::RecruitHero( Heroes * hero )
{
    if ( !hero || !AllowBuyHero() ) {
        return nullptr;
    }

    if ( world.GetHero( *this ) != nullptr ) {
        return nullptr;
    }

    if ( !hero->Recruit( *this ) ) {
        return nullptr;
    }

    Kingdom & currentKingdom = GetKingdom();
    currentKingdom.OddFundsResource( PaymentConditions::RecruitHero() );

    if ( GetLevelMageGuild() ) {
        MageGuildEducateHero( *hero );
    }

    DEBUG_LOG( DBG_GAME, DBG_INFO, name << ", recruit: " << hero->GetName() )

    return hero;
}

bool Castle::RecruitMonster( const Troop & troop, bool showDialog )
{
    if ( !troop.isValid() )
        return false;

    int dwellingIndex = 0;

    switch ( troop.GetDwelling() ) {
    case DWELLING_MONSTER1:
        dwellingIndex = 0;
        break;
    case DWELLING_UPGRADE2:
    case DWELLING_MONSTER2:
        dwellingIndex = 1;
        break;
    case DWELLING_UPGRADE3:
    case DWELLING_MONSTER3:
        dwellingIndex = 2;
        break;
    case DWELLING_UPGRADE4:
    case DWELLING_MONSTER4:
        dwellingIndex = 3;
        break;
    case DWELLING_UPGRADE5:
    case DWELLING_MONSTER5:
        dwellingIndex = 4;
        break;
    case DWELLING_UPGRADE7:
    case DWELLING_UPGRADE6:
    case DWELLING_MONSTER6:
        dwellingIndex = 5;
        break;
    default:
        return false;
    }

    uint32_t count = troop.GetCount();

    if ( dwelling[dwellingIndex] < count ) {
        count = dwelling[dwellingIndex];
    }

    const payment_t paymentCosts = troop.GetTotalCost();
    Kingdom & kingdom = GetKingdom();

    if ( !kingdom.AllowPayment( paymentCosts ) ) {
        return false;
    }

    if ( !GetArmy().JoinTroop( troop ) ) {
        Heroes * hero = world.GetHero( *this );

        if ( hero == nullptr || !hero->GetArmy().JoinTroop( troop ) ) {
            if ( showDialog ) {
                fheroes2::showStandardTextMessage( "", _( "There is no room in the garrison for this army." ), Dialog::OK );
            }
            return false;
        }
    }

    kingdom.OddFundsResource( paymentCosts );
    dwelling[dwellingIndex] -= count;

    DEBUG_LOG( DBG_GAME, DBG_TRACE, name << " recruit: " << troop.GetMultiName() << "(" << count << ")" )

    return true;
}

bool Castle::RecruitMonsterFromDwelling( uint32_t dw, uint32_t count, bool force )
{
    Monster monster( race, GetActualDwelling( dw ) );
    Troop troop( monster, std::min( count, getRecruitLimit( monster, GetKingdom().GetFunds() ) ) );

    if ( !RecruitMonster( troop, false ) ) {
        if ( force ) {
            Troop * weak = GetArmy().GetWeakestTroop();
            if ( weak && weak->GetStrength() < troop.GetStrength() ) {
                DEBUG_LOG( DBG_GAME, DBG_INFO,
                           name << ": " << troop.GetCount() << " " << troop.GetMultiName() << " replace " << weak->GetCount() << " " << weak->GetMultiName() )
                weak->Set( troop );
                return true;
            }
        }

        return false;
    }
    return true;
}

void Castle::recruitBestAvailable( Funds budget )
{
    for ( uint32_t dw = DWELLING_MONSTER6; dw >= DWELLING_MONSTER1; dw >>= 1 ) {
        if ( isBuild( dw ) ) {
            const Monster monster( race, GetActualDwelling( dw ) );
            const uint32_t willRecruit = getRecruitLimit( monster, budget );

            if ( RecruitMonsterFromDwelling( dw, willRecruit, true ) ) {
                // success, reduce the budget
                budget -= ( monster.GetCost() * willRecruit );
            }
        }
    }
}

uint32_t Castle::getRecruitLimit( const Monster & monster, const Funds & budget ) const
{
    // validate that monster is from the current castle
    if ( monster.GetRace() != race )
        return 0;

    const uint32_t available = getMonstersInDwelling( monster.GetDwelling() );

    uint32_t willRecruit = budget.getLowestQuotient( monster.GetCost() );
    if ( available < willRecruit )
        return available;

    return willRecruit;
}

/* return current count monster in dwelling */
uint32_t Castle::getMonstersInDwelling( uint32_t dw ) const
{
    switch ( dw ) {
    case DWELLING_MONSTER1:
        return dwelling[0];
    case DWELLING_MONSTER2:
    case DWELLING_UPGRADE2:
        return dwelling[1];
    case DWELLING_MONSTER3:
    case DWELLING_UPGRADE3:
        return dwelling[2];
    case DWELLING_MONSTER4:
    case DWELLING_UPGRADE4:
        return dwelling[3];
    case DWELLING_MONSTER5:
    case DWELLING_UPGRADE5:
        return dwelling[4];
    case DWELLING_MONSTER6:
    case DWELLING_UPGRADE6:
    case DWELLING_UPGRADE7:
        return dwelling[5];

    default:
        break;
    }

    return 0;
}

/* return requirement for building */
uint32_t Castle::GetBuildingRequirement( uint32_t build ) const
{
    uint32_t requirement = 0;

    switch ( build ) {
    case BUILD_SPEC:
        switch ( race ) {
        case Race::WZRD:
            requirement |= BUILD_MAGEGUILD1;
            break;

        default:
            break;
        }
        break;

    case DWELLING_MONSTER2:
        switch ( race ) {
        case Race::KNGT:
        case Race::BARB:
        case Race::WZRD:
        case Race::WRLK:
        case Race::NECR:
            requirement |= DWELLING_MONSTER1;
            break;

        case Race::SORC:
            requirement |= DWELLING_MONSTER1;
            requirement |= BUILD_TAVERN;
            break;

        default:
            break;
        }
        break;

    case DWELLING_MONSTER3:
        switch ( race ) {
        case Race::KNGT:
            requirement |= DWELLING_MONSTER1;
            requirement |= BUILD_WELL;
            break;

        case Race::BARB:
        case Race::SORC:
        case Race::WZRD:
        case Race::WRLK:
        case Race::NECR:
            requirement |= DWELLING_MONSTER1;
            break;

        default:
            break;
        }
        break;

    case DWELLING_MONSTER4:
        switch ( race ) {
        case Race::KNGT:
            requirement |= DWELLING_MONSTER1;
            requirement |= BUILD_TAVERN;
            break;

        case Race::BARB:
            requirement |= DWELLING_MONSTER1;
            break;

        case Race::SORC:
            requirement |= DWELLING_MONSTER3;
            requirement |= BUILD_MAGEGUILD1;
            break;

        case Race::WZRD:
        case Race::WRLK:
            requirement |= DWELLING_MONSTER2;
            break;

        case Race::NECR:
            requirement |= DWELLING_MONSTER3;
            requirement |= BUILD_THIEVESGUILD;
            break;

        default:
            break;
        }
        break;

    case DWELLING_MONSTER5:
        switch ( race ) {
        case Race::KNGT:
        case Race::BARB:
            requirement |= DWELLING_MONSTER2;
            requirement |= DWELLING_MONSTER3;
            requirement |= DWELLING_MONSTER4;
            break;

        case Race::SORC:
            requirement |= DWELLING_MONSTER4;
            break;

        case Race::WRLK:
            requirement |= DWELLING_MONSTER3;
            break;

        case Race::WZRD:
            requirement |= DWELLING_MONSTER3;
            requirement |= BUILD_MAGEGUILD1;
            break;

        case Race::NECR:
            requirement |= DWELLING_MONSTER2;
            requirement |= BUILD_MAGEGUILD1;
            break;

        default:
            break;
        }
        break;

    case DWELLING_MONSTER6:
        switch ( race ) {
        case Race::KNGT:
            requirement |= DWELLING_MONSTER2;
            requirement |= DWELLING_MONSTER3;
            requirement |= DWELLING_MONSTER4;
            break;

        case Race::BARB:
        case Race::SORC:
        case Race::NECR:
            requirement |= DWELLING_MONSTER5;
            break;

        case Race::WRLK:
        case Race::WZRD:
            requirement |= DWELLING_MONSTER4;
            requirement |= DWELLING_MONSTER5;
            break;

        default:
            break;
        }
        break;

    case DWELLING_UPGRADE2:
        switch ( race ) {
        case Race::KNGT:
        case Race::BARB:
            requirement |= DWELLING_MONSTER2;
            requirement |= DWELLING_MONSTER3;
            requirement |= DWELLING_MONSTER4;
            break;

        case Race::SORC:
            requirement |= DWELLING_MONSTER2;
            requirement |= BUILD_WELL;
            break;

        case Race::NECR:
            requirement |= DWELLING_MONSTER2;
            break;

        default:
            break;
        }
        break;

    case DWELLING_UPGRADE3:
        switch ( race ) {
        case Race::KNGT:
            requirement |= DWELLING_MONSTER2;
            requirement |= DWELLING_MONSTER3;
            requirement |= DWELLING_MONSTER4;
            break;

        case Race::SORC:
            requirement |= DWELLING_MONSTER3;
            requirement |= DWELLING_MONSTER4;
            break;

        case Race::WZRD:
            requirement |= DWELLING_MONSTER3;
            requirement |= BUILD_WELL;
            break;

        case Race::NECR:
            requirement |= DWELLING_MONSTER3;
            break;

        default:
            break;
        }
        break;

    case DWELLING_UPGRADE4:
        switch ( race ) {
        case Race::KNGT:
        case Race::BARB:
            requirement |= DWELLING_MONSTER2;
            requirement |= DWELLING_MONSTER3;
            requirement |= DWELLING_MONSTER4;
            break;

        case Race::SORC:
        case Race::WRLK:
        case Race::NECR:
            requirement |= DWELLING_MONSTER4;
            break;

        default:
            break;
        }
        break;

    case DWELLING_UPGRADE5:
        switch ( race ) {
        case Race::KNGT:
            requirement |= DWELLING_MONSTER5;
            break;

        case Race::BARB:
            requirement |= DWELLING_MONSTER5;
            break;

        case Race::WZRD:
            requirement |= BUILD_SPEC;
            requirement |= DWELLING_MONSTER5;
            break;

        case Race::NECR:
            requirement |= BUILD_MAGEGUILD2;
            requirement |= DWELLING_MONSTER5;
            break;

        default:
            break;
        }
        break;

    case DWELLING_UPGRADE6:
        switch ( race ) {
        case Race::KNGT:
            requirement |= DWELLING_MONSTER6;
            break;

        case Race::WRLK:
        case Race::WZRD:
            requirement |= DWELLING_MONSTER6;
            break;

        default:
            break;
        }
        break;
    case DWELLING_UPGRADE7:
        if ( race == Race::WRLK )
            requirement |= DWELLING_UPGRADE6;
        break;

    default:
        break;
    }

    return requirement;
}

int Castle::CheckBuyBuilding( const uint32_t build ) const
{
    if ( build & building ) {
        return ALREADY_BUILT;
    }

    switch ( build ) {
    case BUILD_CASTLE:
        if ( !Modes( ALLOWCASTLE ) ) {
            return BUILD_DISABLE;
        }
        break;
    case BUILD_SHIPYARD:
        if ( !HaveNearlySea() ) {
            return BUILD_DISABLE;
        }
        break;
    case BUILD_SHRINE:
        if ( Race::NECR != GetRace() || !Settings::Get().isCurrentMapPriceOfLoyalty() ) {
            return BUILD_DISABLE;
        }
        break;
    case BUILD_TAVERN:
        if ( Race::NECR == GetRace() ) {
            return BUILD_DISABLE;
        }
        break;
    default:
        break;
    }

    if ( build >= BUILD_MAGEGUILD2 && build <= BUILD_MAGEGUILD5 ) {
        const uint32_t prevMageGuild = build >> 1;

        if ( !( building & prevMageGuild ) ) {
            return BUILD_DISABLE;
        }
    }

    if ( !Modes( ALLOWBUILD ) ) {
        return NOT_TODAY;
    }

    if ( isCastle() ) {
        if ( build == BUILD_TENT ) {
            return BUILD_DISABLE;
        }
    }
    else {
        if ( build != BUILD_CASTLE ) {
            return NEED_CASTLE;
        }
    }

    switch ( build ) {
    case DWELLING_UPGRADE2:
        if ( ( Race::WRLK | Race::WZRD ) & race )
            return UNKNOWN_UPGRADE;
        break;
    case DWELLING_UPGRADE3:
        if ( ( Race::BARB | Race::WRLK ) & race )
            return UNKNOWN_UPGRADE;
        break;
    case DWELLING_UPGRADE4:
        if ( Race::WZRD & race )
            return UNKNOWN_UPGRADE;
        break;
    case DWELLING_UPGRADE5:
        if ( ( Race::SORC | Race::WRLK ) & race )
            return UNKNOWN_UPGRADE;
        break;
    case DWELLING_UPGRADE6:
        if ( ( Race::BARB | Race::SORC | Race::NECR ) & race )
            return UNKNOWN_UPGRADE;
        break;
    case DWELLING_UPGRADE7:
        if ( Race::WRLK != race )
            return UNKNOWN_UPGRADE;
        break;

    default:
        break;
    }

    const uint32_t requirement = Castle::GetBuildingRequirement( build );

    for ( uint32_t itr = 0x00000001; itr; itr <<= 1 ) {
        if ( ( requirement & itr ) && !( building & itr ) ) {
            return REQUIRES_BUILD;
        }
    }

    if ( !GetKingdom().AllowPayment( PaymentConditions::BuyBuilding( race, build ) ) ) {
        return LACK_RESOURCES;
    }

    return ALLOW_BUILD;
}

int Castle::GetAllBuildingStatus( const Castle & castle )
{
    if ( !castle.Modes( ALLOWBUILD ) )
        return NOT_TODAY;
    if ( !castle.isCastle() )
        return NEED_CASTLE;

    const uint32_t rest = ~castle.building;

    for ( uint32_t itr = 0x00000001; itr; itr <<= 1 )
        if ( ( rest & itr ) && ( ALLOW_BUILD == castle.CheckBuyBuilding( itr ) ) )
            return ALLOW_BUILD;

    for ( uint32_t itr = 0x00000001; itr; itr <<= 1 )
        if ( ( rest & itr ) && ( LACK_RESOURCES == castle.CheckBuyBuilding( itr ) ) )
            return LACK_RESOURCES;

    for ( uint32_t itr = 0x00000001; itr; itr <<= 1 )
        if ( ( rest & itr ) && ( REQUIRES_BUILD == castle.CheckBuyBuilding( itr ) ) )
            return REQUIRES_BUILD;

    return UNKNOWN_COND;
}

bool Castle::AllowBuyBuilding( uint32_t build ) const
{
    return ALLOW_BUILD == CheckBuyBuilding( build );
}

/* buy building */
bool Castle::BuyBuilding( uint32_t build )
{
    if ( !AllowBuyBuilding( build ) )
        return false;

    GetKingdom().OddFundsResource( PaymentConditions::BuyBuilding( race, build ) );

    // add build
    building |= build;

    switch ( build ) {
    case BUILD_CASTLE:
        building &= ~BUILD_TENT;
        Maps::UpdateCastleSprite( GetCenter(), race );
        Maps::ClearFog( GetIndex(), GameStatic::getFogDiscoveryDistance( GameStatic::FogDiscoveryType::CASTLE ), GetColor() );
        break;

    case BUILD_MAGEGUILD1:
    case BUILD_MAGEGUILD2:
    case BUILD_MAGEGUILD3:
    case BUILD_MAGEGUILD4:
    case BUILD_MAGEGUILD5:
        EducateHeroes();
        break;

    case BUILD_CAPTAIN:
        captain.LoadDefaults( HeroBase::CAPTAIN, race );
        captain.SetSpellPoints( captain.GetMaxSpellPoints() );
        if ( GetLevelMageGuild() )
            MageGuildEducateHero( captain );
        break;

    case BUILD_SPEC:
        // build library
        if ( HaveLibraryCapability() )
            EducateHeroes();
        break;

    case DWELLING_MONSTER1:
        dwelling[0] = Monster( race, DWELLING_MONSTER1 ).GetGrown();
        break;
    case DWELLING_MONSTER2:
        dwelling[1] = Monster( race, DWELLING_MONSTER2 ).GetGrown();
        break;
    case DWELLING_MONSTER3:
        dwelling[2] = Monster( race, DWELLING_MONSTER3 ).GetGrown();
        break;
    case DWELLING_MONSTER4:
        dwelling[3] = Monster( race, DWELLING_MONSTER4 ).GetGrown();
        break;
    case DWELLING_MONSTER5:
        dwelling[4] = Monster( race, DWELLING_MONSTER5 ).GetGrown();
        break;
    case DWELLING_MONSTER6:
        dwelling[5] = Monster( race, DWELLING_MONSTER6 ).GetGrown();
        break;
    default:
        break;
    }

    // disable day build
    ResetModes( ALLOWBUILD );

    DEBUG_LOG( DBG_GAME, DBG_INFO, name << " build " << GetStringBuilding( build, race ) )
    return true;
}

/* draw image castle to position */
void Castle::DrawImageCastle( const fheroes2::Point & pt ) const
{
    fheroes2::Display & display = fheroes2::Display::instance();
    const Maps::Tiles & tile = world.GetTiles( GetIndex() );

    uint32_t index = 0;
    fheroes2::Point dst_pt;

    // draw ground
    switch ( tile.GetGround() ) {
    case Maps::Ground::GRASS:
        index = 0;
        break;
    case Maps::Ground::SNOW:
        index = 10;
        break;
    case Maps::Ground::SWAMP:
        index = 20;
        break;
    case Maps::Ground::LAVA:
        index = 30;
        break;
    case Maps::Ground::DESERT:
        index = 40;
        break;
    case Maps::Ground::DIRT:
        index = 50;
        break;
    case Maps::Ground::WASTELAND:
        index = 60;
        break;
    case Maps::Ground::BEACH:
        index = 70;
        break;

    default:
        return;
    }

    for ( uint32_t ii = 0; ii < 5; ++ii ) {
        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::OBJNTWBA, index + ii );
        dst_pt.x = pt.x + ii * 32 + sprite.x();
        dst_pt.y = pt.y + 3 * 32 + sprite.y();
        fheroes2::Blit( sprite, display, dst_pt.x, dst_pt.y );
    }

    for ( uint32_t ii = 0; ii < 5; ++ii ) {
        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::OBJNTWBA, index + 5 + ii );
        dst_pt.x = pt.x + ii * 32 + sprite.x();
        dst_pt.y = pt.y + 4 * 32 + sprite.y();
        fheroes2::Blit( sprite, display, dst_pt.x, dst_pt.y );
    }

    // draw castle
    switch ( race ) {
    case Race::KNGT:
        index = 0;
        break;
    case Race::BARB:
        index = 32;
        break;
    case Race::SORC:
        index = 64;
        break;
    case Race::WRLK:
        index = 96;
        break;
    case Race::WZRD:
        index = 128;
        break;
    case Race::NECR:
        index = 160;
        break;
    default:
        break;
    }
    if ( !( BUILD_CASTLE & building ) )
        index += 16;
    const fheroes2::Sprite & sprite2 = fheroes2::AGG::GetICN( ICN::OBJNTOWN, index );
    dst_pt.x = pt.x + 2 * 32 + sprite2.x();
    dst_pt.y = pt.y + sprite2.y();
    fheroes2::Blit( sprite2, display, dst_pt.x, dst_pt.y );
    for ( uint32_t ii = 0; ii < 5; ++ii ) {
        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::OBJNTOWN, index + 1 + ii );
        dst_pt.x = pt.x + ii * 32 + sprite.x();
        dst_pt.y = pt.y + 32 + sprite.y();
        fheroes2::Blit( sprite, display, dst_pt.x, dst_pt.y );
    }
    for ( uint32_t ii = 0; ii < 5; ++ii ) {
        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::OBJNTOWN, index + 6 + ii );
        dst_pt.x = pt.x + ii * 32 + sprite.x();
        dst_pt.y = pt.y + 2 * 32 + sprite.y();
        fheroes2::Blit( sprite, display, dst_pt.x, dst_pt.y );
    }
    for ( uint32_t ii = 0; ii < 5; ++ii ) {
        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::OBJNTOWN, index + 11 + ii );
        dst_pt.x = pt.x + ii * 32 + sprite.x();
        dst_pt.y = pt.y + 3 * 32 + sprite.y();
        fheroes2::Blit( sprite, display, dst_pt.x, dst_pt.y );
    }
}

int Castle::GetICNBoat( int race )
{
    switch ( race ) {
    case Race::BARB:
        return ICN::TWNBBOAT;
    case Race::KNGT:
        return ICN::TWNKBOAT;
    case Race::NECR:
        return ICN::TWNNBOAT;
    case Race::SORC:
        return ICN::TWNSBOAT;
    case Race::WRLK:
        return ICN::TWNWBOAT;
    case Race::WZRD:
        return ICN::TWNZBOAT;
    default:
        break;
    }

    DEBUG_LOG( DBG_GAME, DBG_WARN, "return unknown" )
    return ICN::UNKNOWN;
}

/* get building name ICN */
int Castle::GetICNBuilding( uint32_t build, int race )
{
    if ( Race::BARB == race ) {
        switch ( build ) {
        case BUILD_CASTLE:
            return ICN::TWNBCSTL;
        case BUILD_TENT:
            return ICN::TWNBTENT;
        case BUILD_SPEC:
            return ICN::TWNBSPEC;
        case BUILD_CAPTAIN:
            return ICN::TWNBCAPT;
        case BUILD_WEL2:
            return ICN::TWNBWEL2;
        case BUILD_LEFTTURRET:
            return ICN::TWNBLTUR;
        case BUILD_RIGHTTURRET:
            return ICN::TWNBRTUR;
        case BUILD_MOAT:
            return ICN::TWNBMOAT;
        case BUILD_MARKETPLACE:
            return ICN::TWNBMARK;
        case BUILD_THIEVESGUILD:
            return ICN::TWNBTHIE;
        case BUILD_TAVERN:
            return ICN::TWNBTVRN;
        case BUILD_WELL:
            return ICN::TWNBWELL;
        case BUILD_STATUE:
            return ICN::TWNBSTAT;
        case BUILD_SHIPYARD:
            return ICN::TWNBDOCK;
        case BUILD_MAGEGUILD1:
        case BUILD_MAGEGUILD2:
        case BUILD_MAGEGUILD3:
        case BUILD_MAGEGUILD4:
        case BUILD_MAGEGUILD5:
            return ICN::TWNBMAGE;
        case DWELLING_MONSTER1:
            return ICN::TWNBDW_0;
        case DWELLING_MONSTER2:
            return ICN::TWNBDW_1;
        case DWELLING_UPGRADE2:
            return ICN::TWNBUP_1;
        case DWELLING_MONSTER3:
            return ICN::TWNBDW_2;
        case DWELLING_MONSTER4:
            return ICN::TWNBDW_3;
        case DWELLING_UPGRADE4:
            return ICN::TWNBUP_3;
        case DWELLING_MONSTER5:
            return ICN::TWNBDW_4;
        case DWELLING_UPGRADE5:
            return ICN::TWNBUP_4;
        case DWELLING_MONSTER6:
            return ICN::TWNBDW_5;
        default:
            break;
        }
    }
    else if ( Race::KNGT == race ) {
        switch ( build ) {
        case BUILD_CASTLE:
            return ICN::TWNKCSTL;
        case BUILD_TENT:
            return ICN::TWNKTENT;
        case BUILD_SPEC:
            return ICN::TWNKSPEC;
        case BUILD_CAPTAIN:
            return ICN::TWNKCAPT;
        case BUILD_WEL2:
            return ICN::KNIGHT_CASTLE_RIGHT_FARM;
        case BUILD_LEFTTURRET:
            return ICN::TWNKLTUR;
        case BUILD_RIGHTTURRET:
            return ICN::TWNKRTUR;
        case BUILD_MOAT:
            return ICN::TWNKMOAT;
        case BUILD_MARKETPLACE:
            return ICN::TWNKMARK;
        case BUILD_THIEVESGUILD:
            return ICN::TWNKTHIE;
        case BUILD_TAVERN:
            return ICN::TWNKTVRN;
        case BUILD_WELL:
            return ICN::TWNKWELL;
        case BUILD_STATUE:
            return ICN::TWNKSTAT;
        case BUILD_SHIPYARD:
            return ICN::TWNKDOCK;
        case BUILD_MAGEGUILD1:
        case BUILD_MAGEGUILD2:
        case BUILD_MAGEGUILD3:
        case BUILD_MAGEGUILD4:
        case BUILD_MAGEGUILD5:
            return ICN::TWNKMAGE;
        case DWELLING_MONSTER1:
            return ICN::TWNKDW_0;
        case DWELLING_MONSTER2:
            return ICN::TWNKDW_1;
        case DWELLING_UPGRADE2:
            return ICN::TWNKUP_1;
        case DWELLING_MONSTER3:
            return ICN::TWNKDW_2;
        case DWELLING_UPGRADE3:
            return ICN::TWNKUP_2;
        case DWELLING_MONSTER4:
            return ICN::TWNKDW_3;
        case DWELLING_UPGRADE4:
            return ICN::TWNKUP_3;
        case DWELLING_MONSTER5:
            return ICN::TWNKDW_4;
        case DWELLING_UPGRADE5:
            return ICN::TWNKUP_4;
        case DWELLING_MONSTER6:
            return ICN::TWNKDW_5;
        case DWELLING_UPGRADE6:
            return ICN::TWNKUP_5;
        default:
            break;
        }
    }
    else if ( Race::NECR == race ) {
        switch ( build ) {
        case BUILD_CASTLE:
            return ICN::TWNNCSTL;
        case BUILD_TENT:
            return ICN::TWNNTENT;
        case BUILD_SPEC:
            return ICN::TWNNSPEC;
        case BUILD_CAPTAIN:
            return ICN::NECROMANCER_CASTLE_STANDALONE_CAPTAIN_QUARTERS;
        case BUILD_WEL2:
            return ICN::TWNNWEL2;
        case BUILD_LEFTTURRET:
            return ICN::TWNNLTUR;
        case BUILD_RIGHTTURRET:
            return ICN::TWNNRTUR;
        case BUILD_MOAT:
            return ICN::TWNNMOAT;
        case BUILD_MARKETPLACE:
            return ICN::TWNNMARK;
        case BUILD_THIEVESGUILD:
            return ICN::TWNNTHIE;
        // shrine
        case BUILD_SHRINE:
            return ICN::TWNNTVRN;
        case BUILD_WELL:
            return ICN::TWNNWELL;
        case BUILD_STATUE:
            return ICN::TWNNSTAT;
        case BUILD_SHIPYARD:
            return ICN::TWNNDOCK;
        case BUILD_MAGEGUILD1:
        case BUILD_MAGEGUILD2:
        case BUILD_MAGEGUILD3:
        case BUILD_MAGEGUILD4:
        case BUILD_MAGEGUILD5:
            return ICN::TWNNMAGE;
        case DWELLING_MONSTER1:
            return ICN::TWNNDW_0;
        case DWELLING_MONSTER2:
            return ICN::TWNNDW_1;
        case DWELLING_UPGRADE2:
            return ICN::TWNNUP_1;
        case DWELLING_MONSTER3:
            return ICN::TWNNDW_2;
        case DWELLING_UPGRADE3:
            return ICN::TWNNUP_2;
        case DWELLING_MONSTER4:
            return ICN::TWNNDW_3;
        case DWELLING_UPGRADE4:
            return ICN::TWNNUP_3;
        case DWELLING_MONSTER5:
            return ICN::TWNNDW_4;
        case DWELLING_UPGRADE5:
            return ICN::TWNNUP_4;
        case DWELLING_MONSTER6:
            return ICN::TWNNDW_5;
        default:
            break;
        }
    }
    else if ( Race::SORC == race ) {
        switch ( build ) {
        case BUILD_CASTLE:
            return ICN::TWNSCSTL;
        case BUILD_TENT:
            return ICN::TWNSTENT;
        case BUILD_SPEC:
            return ICN::TWNSSPEC;
        case BUILD_CAPTAIN:
            return ICN::TWNSCAPT;
        case BUILD_WEL2:
            return ICN::TWNSWEL2;
        case BUILD_LEFTTURRET:
            return ICN::TWNSLTUR;
        case BUILD_RIGHTTURRET:
            return ICN::TWNSRTUR;
        case BUILD_MOAT:
            return ICN::TWNSMOAT;
        case BUILD_MARKETPLACE:
            return ICN::TWNSMARK;
        case BUILD_THIEVESGUILD:
            return ICN::TWNSTHIE;
        case BUILD_TAVERN:
            return ICN::TWNSTVRN;
        case BUILD_WELL:
            return ICN::TWNSWELL;
        case BUILD_STATUE:
            return ICN::TWNSSTAT;
        case BUILD_SHIPYARD:
            return ICN::TWNSDOCK;
        case BUILD_MAGEGUILD1:
        case BUILD_MAGEGUILD2:
        case BUILD_MAGEGUILD3:
        case BUILD_MAGEGUILD4:
        case BUILD_MAGEGUILD5:
            return ICN::TWNSMAGE;
        case DWELLING_MONSTER1:
            return ICN::TWNSDW_0;
        case DWELLING_MONSTER2:
            return ICN::TWNSDW_1;
        case DWELLING_UPGRADE2:
            return ICN::TWNSUP_1;
        case DWELLING_MONSTER3:
            return ICN::TWNSDW_2;
        case DWELLING_UPGRADE3:
            return ICN::TWNSUP_2;
        case DWELLING_MONSTER4:
            return ICN::TWNSDW_3;
        case DWELLING_UPGRADE4:
            return ICN::TWNSUP_3;
        case DWELLING_MONSTER5:
            return ICN::TWNSDW_4;
        case DWELLING_MONSTER6:
            return ICN::TWNSDW_5;
        default:
            break;
        }
    }
    else if ( Race::WRLK == race ) {
        switch ( build ) {
        case BUILD_CASTLE:
            return ICN::TWNWCSTL;
        case BUILD_TENT:
            return ICN::TWNWTENT;
        case BUILD_SPEC:
            return ICN::TWNWSPEC;
        case BUILD_CAPTAIN:
            return ICN::TWNWCAPT;
        case BUILD_WEL2:
            return ICN::TWNWWEL2;
        case BUILD_LEFTTURRET:
            return ICN::TWNWLTUR;
        case BUILD_RIGHTTURRET:
            return ICN::TWNWRTUR;
        case BUILD_MOAT:
            return ICN::TWNWMOAT;
        case BUILD_MARKETPLACE:
            return ICN::TWNWMARK;
        case BUILD_THIEVESGUILD:
            return ICN::TWNWTHIE;
        case BUILD_TAVERN:
            return ICN::TWNWTVRN;
        case BUILD_WELL:
            return ICN::TWNWWELL;
        case BUILD_STATUE:
            return ICN::TWNWSTAT;
        case BUILD_SHIPYARD:
            return ICN::TWNWDOCK;
        case BUILD_MAGEGUILD1:
        case BUILD_MAGEGUILD2:
        case BUILD_MAGEGUILD3:
        case BUILD_MAGEGUILD4:
        case BUILD_MAGEGUILD5:
            return ICN::TWNWMAGE;
        case DWELLING_MONSTER1:
            return ICN::TWNWDW_0;
        case DWELLING_MONSTER2:
            return ICN::TWNWDW_1;
        case DWELLING_MONSTER3:
            return ICN::TWNWDW_2;
        case DWELLING_MONSTER4:
            return ICN::TWNWDW_3;
        case DWELLING_UPGRADE4:
            return ICN::TWNWUP_3;
        case DWELLING_MONSTER5:
            return ICN::TWNWDW_4;
        case DWELLING_MONSTER6:
            return ICN::TWNWDW_5;
        case DWELLING_UPGRADE6:
            return ICN::TWNWUP_5;
        case DWELLING_UPGRADE7:
            return ICN::TWNWUP5B;
        default:
            break;
        }
    }
    else if ( Race::WZRD == race ) {
        switch ( build ) {
        case BUILD_CASTLE:
            return ICN::TWNZCSTL;
        case BUILD_TENT:
            return ICN::TWNZTENT;
        case BUILD_SPEC:
            return ICN::TWNZSPEC;
        case BUILD_CAPTAIN:
            return ICN::TWNZCAPT;
        case BUILD_WEL2:
            return ICN::TWNZWEL2;
        case BUILD_LEFTTURRET:
            return ICN::TWNZLTUR;
        case BUILD_RIGHTTURRET:
            return ICN::TWNZRTUR;
        case BUILD_MOAT:
            return ICN::TWNZMOAT;
        case BUILD_MARKETPLACE:
            return ICN::TWNZMARK;
        case BUILD_THIEVESGUILD:
            return ICN::TWNZTHIE;
        case BUILD_TAVERN:
            return ICN::TWNZTVRN;
        case BUILD_WELL:
            return ICN::TWNZWELL;
        case BUILD_STATUE:
            return ICN::TWNZSTAT;
        case BUILD_SHIPYARD:
            return ICN::TWNZDOCK;
        case BUILD_MAGEGUILD1:
        case BUILD_MAGEGUILD2:
        case BUILD_MAGEGUILD3:
        case BUILD_MAGEGUILD4:
        case BUILD_MAGEGUILD5:
            return ICN::TWNZMAGE;
        case DWELLING_MONSTER1:
            return ICN::TWNZDW_0;
        case DWELLING_MONSTER2:
            return ICN::TWNZDW_1;
        case DWELLING_MONSTER3:
            return ICN::TWNZDW_2;
        case DWELLING_UPGRADE3:
            return ICN::TWNZUP_2;
        case DWELLING_MONSTER4:
            return ICN::TWNZDW_3;
        case DWELLING_MONSTER5:
            return ICN::TWNZDW_4;
        case DWELLING_UPGRADE5:
            return ICN::TWNZUP_4;
        case DWELLING_MONSTER6:
            return ICN::TWNZDW_5;
        case DWELLING_UPGRADE6:
            return ICN::TWNZUP_5;
        default:
            break;
        }
    }

    DEBUG_LOG( DBG_GAME, DBG_WARN,
               "return unknown"
                   << ", race: " << Race::String( race ) << ", build: " << Castle::GetStringBuilding( build, race ) << ", " << build )

    return ICN::UNKNOWN;
}

Heroes * Castle::GetHero() const
{
    return world.GetHero( *this );
}

bool Castle::HaveNearlySea() const
{
    // check nearest ocean
    if ( Maps::isValidAbsPoint( center.x, center.y + 2 ) ) {
        const int32_t index = Maps::GetIndexFromAbsPoint( center.x, center.y + 2 );
        const Maps::Tiles & left = world.GetTiles( index - 1 );
        const Maps::Tiles & right = world.GetTiles( index + 1 );
        const Maps::Tiles & middle = world.GetTiles( index );

        return left.isWater() || right.isWater() || middle.isWater();
    }
    return false;
}

bool TilePresentBoat( const Maps::Tiles & tile )
{
    return tile.isWater() && ( tile.GetObject() == MP2::OBJ_BOAT || tile.GetObject() == MP2::OBJ_HEROES );
}

bool Castle::PresentBoat() const
{
    // 2 cell down
    if ( Maps::isValidAbsPoint( center.x, center.y + 2 ) ) {
        const int32_t index = Maps::GetIndexFromAbsPoint( center.x, center.y + 2 );
        const int32_t max = world.w() * world.h();

        if ( index + 1 < max ) {
            const Maps::Tiles & left = world.GetTiles( index - 1 );
            const Maps::Tiles & right = world.GetTiles( index + 1 );
            const Maps::Tiles & middle = world.GetTiles( index );

            if ( TilePresentBoat( left ) || TilePresentBoat( right ) || TilePresentBoat( middle ) )
                return true;
        }
    }
    return false;
}

uint32_t Castle::GetActualDwelling( const uint32_t buildId ) const
{
    switch ( buildId ) {
    case DWELLING_MONSTER1:
    case DWELLING_UPGRADE2:
    case DWELLING_UPGRADE3:
    case DWELLING_UPGRADE4:
    case DWELLING_UPGRADE5:
    case DWELLING_UPGRADE7:
        return buildId;
    case DWELLING_MONSTER2:
        return building & DWELLING_UPGRADE2 ? DWELLING_UPGRADE2 : buildId;
    case DWELLING_MONSTER3:
        return building & DWELLING_UPGRADE3 ? DWELLING_UPGRADE3 : buildId;
    case DWELLING_MONSTER4:
        return building & DWELLING_UPGRADE4 ? DWELLING_UPGRADE4 : buildId;
    case DWELLING_MONSTER5:
        return building & DWELLING_UPGRADE5 ? DWELLING_UPGRADE5 : buildId;
    case DWELLING_MONSTER6:
        return building & DWELLING_UPGRADE7 ? DWELLING_UPGRADE7 : ( building & DWELLING_UPGRADE6 ? DWELLING_UPGRADE6 : buildId );
    case DWELLING_UPGRADE6:
        return building & DWELLING_UPGRADE7 ? DWELLING_UPGRADE7 : buildId;
    default:
        break;
    }

    return BUILD_NOTHING;
}

uint32_t Castle::GetUpgradeBuilding( uint32_t build ) const
{
    switch ( build ) {
    case BUILD_TENT:
        return BUILD_CASTLE;
    case BUILD_MAGEGUILD1:
        return BUILD_MAGEGUILD2;
    case BUILD_MAGEGUILD2:
        return BUILD_MAGEGUILD3;
    case BUILD_MAGEGUILD3:
        return BUILD_MAGEGUILD4;
    case BUILD_MAGEGUILD4:
        return BUILD_MAGEGUILD5;
    default:
        break;
    }

    if ( Race::BARB == race ) {
        switch ( build ) {
        case DWELLING_MONSTER2:
            return DWELLING_UPGRADE2;
        case DWELLING_MONSTER4:
            return DWELLING_UPGRADE4;
        case DWELLING_MONSTER5:
            return DWELLING_UPGRADE5;
        default:
            break;
        }
    }
    else if ( Race::KNGT == race ) {
        switch ( build ) {
        case DWELLING_MONSTER2:
            return DWELLING_UPGRADE2;
        case DWELLING_MONSTER3:
            return DWELLING_UPGRADE3;
        case DWELLING_MONSTER4:
            return DWELLING_UPGRADE4;
        case DWELLING_MONSTER5:
            return DWELLING_UPGRADE5;
        case DWELLING_MONSTER6:
            return DWELLING_UPGRADE6;
        default:
            break;
        }
    }
    else if ( Race::NECR == race ) {
        switch ( build ) {
        case DWELLING_MONSTER2:
            return DWELLING_UPGRADE2;
        case DWELLING_MONSTER3:
            return DWELLING_UPGRADE3;
        case DWELLING_MONSTER4:
            return DWELLING_UPGRADE4;
        case DWELLING_MONSTER5:
            return DWELLING_UPGRADE5;
        default:
            break;
        }
    }
    else if ( Race::SORC == race ) {
        switch ( build ) {
        case DWELLING_MONSTER2:
            return DWELLING_UPGRADE2;
        case DWELLING_MONSTER3:
            return DWELLING_UPGRADE3;
        case DWELLING_MONSTER4:
            return DWELLING_UPGRADE4;
        default:
            break;
        }
    }
    else if ( Race::WRLK == race ) {
        switch ( build ) {
        case DWELLING_MONSTER4:
            return DWELLING_UPGRADE4;
        case DWELLING_MONSTER6:
            return isBuild( DWELLING_UPGRADE6 ) ? DWELLING_UPGRADE7 : DWELLING_UPGRADE6;
        default:
            break;
        }
    }
    else if ( Race::WZRD == race ) {
        switch ( build ) {
        case DWELLING_MONSTER3:
            return DWELLING_UPGRADE3;
        case DWELLING_MONSTER5:
            return DWELLING_UPGRADE5;
        case DWELLING_MONSTER6:
            return DWELLING_UPGRADE6;
        default:
            break;
        }
    }

    return build;
}

bool Castle::PredicateIsCastle( const Castle * castle )
{
    return castle && castle->isCastle();
}

bool Castle::PredicateIsTown( const Castle * castle )
{
    return castle && !castle->isCastle();
}

bool Castle::PredicateIsBuildBuilding( const Castle * castle, const uint32_t building )
{
    return castle && castle->isBuild( building );
}

std::string Castle::String() const
{
    std::ostringstream os;
    const Heroes * hero = GetHero();

    os << "name and type   : " << name << " (" << Race::String( race ) << ")" << std::endl
       << "color           : " << Color::String( GetColor() ) << std::endl
       << "dwellings       : ";

    for ( uint32_t level = 0; level < 7; ++level ) {
        // there is no dwelling 7
        if ( level != 6 && isBuild( DWELLING_MONSTER1 << level ) )
            os << level + 1;

        if ( level > 0 && isBuild( DWELLING_UPGRADE2 << ( level - 1 ) ) )
            os << "U, ";
        else
            os << ", ";
    }
    os << std::endl;

    os << "buildings       : " << CountBuildings() << " (mage guild: " << GetLevelMageGuild() << ")" << std::endl
       << "coast/has boat  : " << ( HaveNearlySea() ? "yes" : "no" ) << " / " << ( PresentBoat() ? "yes" : "no" ) << std::endl
       << "is castle       : " << ( isCastle() ? "yes" : "no" ) << " (" << getBuildingValue() << ")" << std::endl
       << "army            : " << army.String() << std::endl;

    if ( hero != nullptr ) {
        os << "hero army       : " << hero->GetArmy().String() << std::endl;
    }

    return os.str();
}

int Castle::GetAttackModificator( const std::string * ) const
{
    return 0;
}

int Castle::GetDefenseModificator( const std::string * ) const
{
    return 0;
}

int Castle::GetPowerModificator( std::string * strs ) const
{
    int result = 0;

    if ( Race::NECR == race && isBuild( BUILD_SPEC ) ) {
        const int mod = 2;
        result += mod;
        if ( strs ) {
            strs->append( GetStringBuilding( BUILD_SPEC, race ) );
            StringAppendModifiers( *strs, mod );
        }
    }

    return result;
}

int Castle::GetKnowledgeModificator( const std::string * ) const
{
    return 0;
}

int Castle::GetMoraleModificator( std::string * strs ) const
{
    int result = Morale::NORMAL;

    // and tavern
    if ( isBuild( BUILD_TAVERN ) ) {
        const int mod = 1;
        result += mod;
        if ( strs ) {
            strs->append( GetStringBuilding( BUILD_TAVERN, race ) );
            StringAppendModifiers( *strs, mod );
            strs->append( "\n" );
        }
    }

    // and barbarian coliseum
    if ( Race::BARB == race && isBuild( BUILD_SPEC ) ) {
        const int mod = 2;
        result += mod;
        if ( strs ) {
            strs->append( GetStringBuilding( BUILD_SPEC, race ) );
            StringAppendModifiers( *strs, mod );
            strs->append( "\n" );
        }
    }

    return result;
}

int Castle::GetLuckModificator( std::string * strs ) const
{
    int result = Luck::NORMAL;

    if ( Race::SORC == race && isBuild( BUILD_SPEC ) ) {
        const int mod = 2;
        result += mod;
        if ( strs ) {
            strs->append( Castle::GetStringBuilding( BUILD_SPEC, race ) );
            StringAppendModifiers( *strs, mod );
            strs->append( "\n" );
        }
    }

    return result;
}

const Army & Castle::GetArmy() const
{
    return army;
}

Army & Castle::GetArmy()
{
    return army;
}

const Army & Castle::GetActualArmy() const
{
    const Heroes * hero = world.GetHero( *this );
    return hero ? hero->GetArmy() : army;
}

Army & Castle::GetActualArmy()
{
    Heroes * hero = world.GetHero( *this );
    return hero ? hero->GetArmy() : army;
}

double Castle::GetGarrisonStrength( const Heroes * attackingHero ) const
{
    double totalStrength = 0;

    Heroes * hero = world.GetHero( *this );

    // If there is a hero in the castle, then some of the garrison troops can join his army if
    // there is a place for them. Castle bonuses are applied to the resulting combined army.
    if ( hero ) {
        Army garrisonArmy;
        garrisonArmy.Assign( army );

        Army combinedArmy( hero );
        combinedArmy.Assign( hero->GetArmy() );
        combinedArmy.ArrangeForCastleDefense( garrisonArmy );

        totalStrength += combinedArmy.GetStrength();
    }
    // Otherwise just use the garrison army strength. Castle bonuses are also applied.
    else {
        totalStrength += army.GetStrength();
    }

    // Add castle bonuses if there are any troops defending the castle
    if ( isCastle() && totalStrength > 1 ) {
        const Battle::Tower tower( *this, Battle::TowerType::TWR_CENTER, Rand::DeterministicRandomGenerator( 0 ), 0 );
        const double towerStr = tower.GetStrengthWithBonus( tower.GetAttackBonus(), 0 );

        totalStrength += towerStr;
        if ( isBuild( BUILD_LEFTTURRET ) ) {
            totalStrength += towerStr / 2;
        }
        if ( isBuild( BUILD_RIGHTTURRET ) ) {
            totalStrength += towerStr / 2;
        }

        if ( attackingHero && ( !attackingHero->GetArmy().isMeleeDominantArmy() || attackingHero->HasSecondarySkill( Skill::Secondary::BALLISTICS ) ) ) {
            totalStrength *= isBuild( BUILD_MOAT ) ? 1.2 : 1.15;
        }
        else {
            // Heavy penalty if the attacking hero does not have a ballistic skill, and his army is based on melee infantry
            totalStrength *= isBuild( BUILD_MOAT ) ? 1.45 : 1.25;
        }
    }

    return totalStrength;
}

bool Castle::AllowBuyBoat() const
{
    // check payment and present other boat
    return ( HaveNearlySea() && isBuild( BUILD_SHIPYARD ) && GetKingdom().AllowPayment( PaymentConditions::BuyBoat() ) && !PresentBoat() );
}

bool Castle::BuyBoat() const
{
    if ( !AllowBuyBoat() )
        return false;
    if ( isControlHuman() )
        AudioManager::PlaySound( M82::BUILDTWN );

    if ( !Maps::isValidAbsPoint( center.x, center.y + 2 ) )
        return false;

    const int32_t index = Maps::GetIndexFromAbsPoint( center.x, center.y + 2 );
    Maps::Tiles & left = world.GetTiles( index - 1 );
    Maps::Tiles & right = world.GetTiles( index + 1 );
    Maps::Tiles & middle = world.GetTiles( index );
    Kingdom & kingdom = GetKingdom();

    if ( MP2::OBJ_NONE == left.GetObject() && left.isWater() ) {
        kingdom.OddFundsResource( PaymentConditions::BuyBoat() );
        left.setBoat( Direction::RIGHT, kingdom.GetColor() );
    }
    else if ( MP2::OBJ_NONE == right.GetObject() && right.isWater() ) {
        kingdom.OddFundsResource( PaymentConditions::BuyBoat() );
        right.setBoat( Direction::RIGHT, kingdom.GetColor() );
    }
    else if ( MP2::OBJ_NONE == middle.GetObject() && middle.isWater() ) {
        kingdom.OddFundsResource( PaymentConditions::BuyBoat() );
        middle.setBoat( Direction::RIGHT, kingdom.GetColor() );
    }

    return true;
}

void Castle::setName( const std::set<std::string> & usedNames )
{
    assert( name.empty() );

    std::vector<const char *> shuffledCastleNames( defaultCastleNames.begin(), defaultCastleNames.end() );

    Rand::Shuffle( shuffledCastleNames );

    for ( const char * originalName : shuffledCastleNames ) {
        const char * translatedCastleName = _( originalName );
        if ( usedNames.count( translatedCastleName ) < 1 ) {
            name = translatedCastleName;
            return;
        }
    }

    // How is it possible that we're out of castle names?
    assert( 0 );
}

int Castle::GetControl() const
{
    /* gray towns: AI control */
    return GetColor() & Color::ALL ? GetKingdom().GetControl() : CONTROL_AI;
}

bool Castle::isNecromancyShrineBuild() const
{
    return race == Race::NECR && ( BUILD_SHRINE & building );
}

uint32_t Castle::GetGrownWell()
{
    return GameStatic::GetCastleGrownWell();
}

uint32_t Castle::GetGrownWel2()
{
    return GameStatic::GetCastleGrownWel2();
}

uint32_t Castle::GetGrownWeekOf()
{
    return GameStatic::GetCastleGrownWeekOf();
}

uint32_t Castle::GetGrownMonthOf()
{
    return GameStatic::GetCastleGrownMonthOf();
}

void Castle::Scout() const
{
    Maps::ClearFog( GetIndex(), GameStatic::getFogDiscoveryDistance( GameStatic::FogDiscoveryType::CASTLE ), GetColor() );
}

void Castle::JoinRNDArmy()
{
    const uint32_t timeModifier = world.CountDay() / 10;
    const uint32_t reinforcementQuality = Rand::Get( 1, 15 ) + timeModifier;

    uint32_t count = timeModifier / 2;
    uint32_t dwellingType = DWELLING_MONSTER1;

    if ( reinforcementQuality > 15 ) {
        dwellingType = DWELLING_MONSTER5;
        count += 1;
    }
    else if ( reinforcementQuality > 13 ) {
        dwellingType = DWELLING_MONSTER4;
        count += Rand::Get( 1, 3 );
    }
    else if ( reinforcementQuality > 10 ) {
        dwellingType = DWELLING_MONSTER3;
        count += Rand::Get( 3, 5 );
    }
    else if ( reinforcementQuality > 5 ) {
        dwellingType = DWELLING_MONSTER2;
        count += Rand::Get( 5, 7 );
    }
    else {
        count += Rand::Get( 8, 15 );
    }

    army.JoinTroop( Monster( race, dwellingType ), count, false );
}

void Castle::ActionPreBattle()
{
    Heroes * hero = world.GetHero( *this );
    if ( hero ) {
        hero->GetArmy().ArrangeForCastleDefense( army );
    }

    if ( isControlAI() ) {
        AI::Get().CastlePreBattle( *this );
    }
}

void Castle::ActionAfterBattle( bool attacker_wins )
{
    if ( attacker_wins ) {
        army.Clean();
        ResetModes( CUSTOMARMY );
    }

    if ( isControlAI() )
        AI::Get().CastleAfterBattle( *this, attacker_wins );
}

Castle * VecCastles::GetFirstCastle() const
{
    const_iterator it = std::find_if( begin(), end(), []( const Castle * castle ) { return castle->isCastle(); } );
    return end() != it ? *it : nullptr;
}

void VecCastles::ChangeColors( int col1, int col2 )
{
    for ( iterator it = begin(); it != end(); ++it )
        if ( ( *it )->GetColor() == col1 )
            ( *it )->ChangeColor( col2 );
}

AllCastles::AllCastles()
{
    // reserve memory
    _castles.reserve( maximumCastles );
}

AllCastles::~AllCastles()
{
    Clear();
}

void AllCastles::Init()
{
    Clear();
}

void AllCastles::Clear()
{
    for ( auto it = begin(); it != end(); ++it )
        delete *it;
    _castles.clear();
    _castleTiles.clear();
}

void AllCastles::AddCastle( Castle * castle )
{
    _castles.push_back( castle );

    /* Register position of all castle elements on the map
    Castle element positions are:
                +
              +++++
              +++++
              ++X++
              ++ ++

     where
     X is the main castle position
     + are tiles that are considered part of the castle for the Get() method
    */

    const size_t id = _castles.size() - 1;
    const fheroes2::Point & center = castle->GetCenter();

    // We need to override any existing castle's ID that is why we use [] operator to access std::map.
    // Castles are added from top to bottom, from left to right so a newer castle must override existing data for a tile if any.

    for ( int32_t y = -2; y <= 1; ++y ) {
        for ( int32_t x = -2; x <= 2; ++x ) {
            if ( y == 1 && x == 0 ) {
                // Do not mark a tile below castle's entrance as castle.
                continue;
            }

            _castleTiles[center + fheroes2::Point( x, y )] = id;
        }
    }

    _castleTiles[center + fheroes2::Point( 0, -3 )] = id;
}

void AllCastles::Scout( int colors ) const
{
    for ( auto it = begin(); it != end(); ++it )
        if ( colors & ( *it )->GetColor() )
            ( *it )->Scout();
}

/* pack castle */
StreamBase & operator<<( StreamBase & msg, const Castle & castle )
{
    const ColorBase & color = castle;

    msg << static_cast<const MapPosition &>( castle ) << castle.modes << castle.race << castle.building << castle.captain << color << castle.name << castle.mageguild
        << static_cast<uint32_t>( CASTLEMAXMONSTER );

    for ( uint32_t ii = 0; ii < CASTLEMAXMONSTER; ++ii )
        msg << castle.dwelling[ii];

    return msg << castle.army;
}

/* unpack castle */
StreamBase & operator>>( StreamBase & msg, Castle & castle )
{
    ColorBase & color = castle;
    uint32_t dwellingcount;

    msg >> static_cast<MapPosition &>( castle ) >> castle.modes >> castle.race >> castle.building >> castle.captain >> color >> castle.name >> castle.mageguild;

    msg >> dwellingcount;
    for ( uint32_t ii = 0; ii < dwellingcount; ++ii )
        msg >> castle.dwelling[ii];

    msg >> castle.army;
    castle.army.SetCommander( &castle.captain );

    return msg;
}

StreamBase & operator<<( StreamBase & msg, const VecCastles & castles )
{
    msg << static_cast<uint32_t>( castles.size() );

    for ( auto it = castles.begin(); it != castles.end(); ++it )
        msg << ( *it ? ( *it )->GetIndex() : static_cast<int32_t>( -1 ) );

    return msg;
}

StreamBase & operator>>( StreamBase & msg, VecCastles & castles )
{
    int32_t index;
    uint32_t size;
    msg >> size;

    castles.resize( size, nullptr );

    for ( auto it = castles.begin(); it != castles.end(); ++it ) {
        msg >> index;
        *it = ( index < 0 ? nullptr : world.getCastleEntrance( Maps::GetPoint( index ) ) );
        assert( *it != nullptr );
    }

    return msg;
}

StreamBase & operator<<( StreamBase & msg, const AllCastles & castles )
{
    msg << static_cast<uint32_t>( castles.Size() );

    for ( const Castle * castle : castles )
        msg << *castle;

    return msg;
}

StreamBase & operator>>( StreamBase & msg, AllCastles & castles )
{
    uint32_t size;
    msg >> size;

    castles.Clear();

    for ( uint32_t i = 0; i < size; ++i ) {
        Castle * castle = new Castle();
        msg >> *castle;
        castles.AddCastle( castle );
    }

    return msg;
}

std::string Castle::GetStringBuilding( uint32_t build ) const
{
    return GetStringBuilding( build, GetRace() );
}

std::string Castle::GetDescriptionBuilding( uint32_t build ) const
{
    std::string res = GetDescriptionBuilding( build, GetRace() );

    switch ( build ) {
    case BUILD_WELL:
        StringReplace( res, "%{count}", GetGrownWell() );
        break;

    case BUILD_WEL2:
        StringReplace( res, "%{count}", GetGrownWel2() );
        break;

    case BUILD_CASTLE: {
        StringReplace( res, "%{count}", ProfitConditions::FromBuilding( BUILD_CASTLE, race ).gold );

        if ( isBuild( BUILD_CASTLE ) ) {
            res.append( "\n \n" );
            res.append( Battle::Tower::GetInfo( *this ) );
        }

        if ( isBuild( BUILD_MOAT ) ) {
            res.append( "\n \n" );
            res.append( Battle::Board::GetMoatInfo() );
        }
        break;
    }

    case BUILD_SPEC:
    case BUILD_STATUE: {
        const payment_t profit = ProfitConditions::FromBuilding( build, GetRace() );
        StringReplace( res, "%{count}", profit.gold );
        break;
    }

    default:
        break;
    }

    return res;
}
