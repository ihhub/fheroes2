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

#include "castle.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <iterator>
#include <ostream>

#include "agg_image.h"
#include "ai_planner.h"
#include "army_troop.h"
#include "artifact.h"
#include "audio_manager.h"
#include "battle_board.h"
#include "battle_tower.h"
#include "castle_building_info.h"
#include "dialog.h"
#include "difficulty.h"
#include "direction.h"
#include "game.h"
#include "game_io.h"
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
#include "map_format_helper.h"
#include "map_format_info.h"
#include "maps.h"
#include "maps_fileinfo.h"
#include "maps_tiles.h"
#include "morale.h"
#include "mp2.h"
#include "payment.h"
#include "profit.h"
#include "race.h"
#include "rand.h"
#include "resource.h"
#include "save_format_version.h"
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
    , _constructedBuildings( 0 )
    , _disabledBuildings( 0 )
    , captain( *this )
    , army( nullptr )
{
    dwelling = { 0 };
    army.SetCommander( &captain );
}

Castle::Castle( int32_t cx, int32_t cy, int rc )
    : MapPosition( fheroes2::Point( cx, cy ) )
    , race( rc )
    , _constructedBuildings( 0 )
    , _disabledBuildings( 0 )
    , captain( *this )
    , army( nullptr )
{
    dwelling = { 0 };
    army.SetCommander( &captain );
}

void Castle::LoadFromMP2( const std::vector<uint8_t> & data )
{
    assert( data.size() == MP2::MP2_CASTLE_STRUCTURE_SIZE );

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
            _constructedBuildings |= BUILD_THIEVESGUILD;
        if ( 0x0004 & commonBuildings )
            _constructedBuildings |= BUILD_TAVERN;
        if ( 0x0008 & commonBuildings )
            _constructedBuildings |= BUILD_SHIPYARD;
        if ( 0x0010 & commonBuildings )
            _constructedBuildings |= BUILD_WELL;
        if ( 0x0080 & commonBuildings )
            _constructedBuildings |= BUILD_STATUE;
        if ( 0x0100 & commonBuildings )
            _constructedBuildings |= BUILD_LEFTTURRET;
        if ( 0x0200 & commonBuildings )
            _constructedBuildings |= BUILD_RIGHTTURRET;
        if ( 0x0400 & commonBuildings )
            _constructedBuildings |= BUILD_MARKETPLACE;
        if ( 0x1000 & commonBuildings )
            _constructedBuildings |= BUILD_MOAT;
        if ( 0x0800 & commonBuildings )
            _constructedBuildings |= BUILD_WEL2;
        if ( 0x2000 & commonBuildings )
            _constructedBuildings |= BUILD_SPEC;

        // Existing dwellings.
        const uint16_t existingDwellings = dataStream.getLE16();
        if ( 0x0008 & existingDwellings )
            _constructedBuildings |= DWELLING_MONSTER1;
        if ( 0x0010 & existingDwellings )
            _constructedBuildings |= DWELLING_MONSTER2;
        if ( 0x0020 & existingDwellings )
            _constructedBuildings |= DWELLING_MONSTER3;
        if ( 0x0040 & existingDwellings )
            _constructedBuildings |= DWELLING_MONSTER4;
        if ( 0x0080 & existingDwellings )
            _constructedBuildings |= DWELLING_MONSTER5;
        if ( 0x0100 & existingDwellings )
            _constructedBuildings |= DWELLING_MONSTER6;
        if ( 0x0200 & existingDwellings )
            _constructedBuildings |= DWELLING_UPGRADE2 | DWELLING_MONSTER2;
        if ( 0x0400 & existingDwellings )
            _constructedBuildings |= DWELLING_UPGRADE3 | DWELLING_MONSTER3;
        if ( 0x0800 & existingDwellings )
            _constructedBuildings |= DWELLING_UPGRADE4 | DWELLING_MONSTER4;
        if ( 0x1000 & existingDwellings )
            _constructedBuildings |= DWELLING_UPGRADE5 | DWELLING_MONSTER5;
        if ( 0x2000 & existingDwellings )
            _constructedBuildings |= DWELLING_UPGRADE6 | DWELLING_MONSTER6;

        // magic tower
        const uint8_t magicGuildLevel = dataStream.get();
        if ( 0 < magicGuildLevel )
            _constructedBuildings |= BUILD_MAGEGUILD1;
        if ( 1 < magicGuildLevel )
            _constructedBuildings |= BUILD_MAGEGUILD2;
        if ( 2 < magicGuildLevel )
            _constructedBuildings |= BUILD_MAGEGUILD3;
        if ( 3 < magicGuildLevel )
            _constructedBuildings |= BUILD_MAGEGUILD4;
        if ( 4 < magicGuildLevel )
            _constructedBuildings |= BUILD_MAGEGUILD5;
    }
    else {
        // Skip reading 5 bytes corresponding to custom buildings for the town / castle.
        dataStream.skip( 5 );

        // Set default buildings.
        _setDefaultBuildings();
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
        SetModes( CUSTOM_ARMY );
    }
    else {
        // Skip 15 bytes as custom defenders are not set.
        dataStream.skip( 15 );
    }

    const bool isCaptainAvailable = ( dataStream.get() != 0 );
    if ( isCaptainAvailable ) {
        _constructedBuildings |= BUILD_CAPTAIN;
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
        _constructedBuildings |= BUILD_CASTLE;
    }
    else {
        _constructedBuildings |= BUILD_TENT;
    }

    _disabledBuildings = 0;

    const bool isCastleNotAllowed = ( dataStream.get() != 0 );
    if ( isCastleNotAllowed ) {
        _disabledBuildings |= BUILD_CASTLE;
    }

    // Skip the rest of 29 bytes.

    PostLoad();
}

void Castle::loadFromResurrectionMap( const Maps::Map_Format::CastleMetadata & metadata )
{
    modes = 0;

    _constructedBuildings = Maps::getBuildingsFromVector( metadata.builtBuildings );

    if ( !metadata.customBuildings ) {
        _setDefaultBuildings();
    }

    _disabledBuildings = 0;

    for ( const uint32_t building : metadata.bannedBuildings ) {
        _disabledBuildings |= building;
    }

    // Check the default Army state for the Neutral player.
    if ( Maps::loadCastleArmy( army, metadata ) ) {
        SetModes( CUSTOM_ARMY );
    }

    if ( !metadata.customName.empty() ) {
        name = metadata.customName;
    }

    PostLoad();
}

void Castle::PostLoad()
{
    // Fix dwelling upgrades dependent from race. (For random race towns.)
    switch ( race ) {
    case Race::KNGT:
        _constructedBuildings &= ~DWELLING_UPGRADE7;
        break;
    case Race::BARB:
        _constructedBuildings &= ~( DWELLING_UPGRADE3 | DWELLING_UPGRADE6 | DWELLING_UPGRADE7 );
        break;
    case Race::SORC:
        _constructedBuildings &= ~( DWELLING_UPGRADE5 | DWELLING_UPGRADE6 | DWELLING_UPGRADE7 );
        break;
    case Race::WRLK:
        _constructedBuildings &= ~( DWELLING_UPGRADE2 | DWELLING_UPGRADE3 | DWELLING_UPGRADE5 );
        break;
    case Race::WZRD:
        _constructedBuildings &= ~( DWELLING_UPGRADE2 | DWELLING_UPGRADE4 | DWELLING_UPGRADE7 );
        break;
    case Race::NECR:
        _constructedBuildings &= ~( DWELLING_UPGRADE6 | DWELLING_UPGRADE7 );
        break;
    default:
        break;
    }

    // Fill built dwellings with weekly growth monsters.
    if ( _constructedBuildings & DWELLING_MONSTER1 ) {
        dwelling[0] = Monster( race, DWELLING_MONSTER1 ).GetGrown();
    }

    if ( _constructedBuildings & DWELLING_UPGRADE2 ) {
        dwelling[1] = Monster( race, DWELLING_UPGRADE2 ).GetGrown();
    }
    else if ( _constructedBuildings & DWELLING_MONSTER2 ) {
        dwelling[1] = Monster( race, DWELLING_MONSTER2 ).GetGrown();
    }

    if ( _constructedBuildings & DWELLING_UPGRADE3 ) {
        dwelling[2] = Monster( race, DWELLING_UPGRADE3 ).GetGrown();
    }
    else if ( _constructedBuildings & DWELLING_MONSTER3 ) {
        dwelling[2] = Monster( race, DWELLING_MONSTER3 ).GetGrown();
    }

    if ( _constructedBuildings & DWELLING_UPGRADE4 ) {
        dwelling[3] = Monster( race, DWELLING_UPGRADE4 ).GetGrown();
    }
    else if ( _constructedBuildings & DWELLING_MONSTER4 ) {
        dwelling[3] = Monster( race, DWELLING_MONSTER4 ).GetGrown();
    }

    if ( _constructedBuildings & DWELLING_UPGRADE5 ) {
        dwelling[4] = Monster( race, DWELLING_UPGRADE5 ).GetGrown();
    }
    else if ( _constructedBuildings & DWELLING_MONSTER5 ) {
        dwelling[4] = Monster( race, DWELLING_MONSTER5 ).GetGrown();
    }

    if ( _constructedBuildings & DWELLING_UPGRADE7 ) {
        dwelling[5] = Monster( race, DWELLING_UPGRADE7 ).GetGrown();
    }
    else if ( _constructedBuildings & DWELLING_UPGRADE6 ) {
        dwelling[5] = Monster( race, DWELLING_UPGRADE6 ).GetGrown();
    }
    else if ( _constructedBuildings & DWELLING_MONSTER6 ) {
        dwelling[5] = Monster( race, DWELLING_MONSTER6 ).GetGrown();
    }

    army.SetColor( GetColor() );

    // fix captain
    if ( _constructedBuildings & BUILD_CAPTAIN ) {
        captain.LoadDefaults( HeroBase::CAPTAIN, race );
        captain.SetSpellPoints( captain.GetMaxSpellPoints() );
    }

    // MageGuild
    mageguild.initialize( race, HaveLibraryCapability() );
    // educate heroes and captain
    EducateHeroes();

    // AI troops auto pack for gray towns
    if ( Color::NONE == GetColor() && !Modes( CUSTOM_ARMY ) ) {
        // towns get 4 reinforcements at the start of the game
        for ( int i = 0; i < 4; ++i )
            JoinRNDArmy();
    }

    if ( !HasSeaAccess() ) {
        // Remove shipyard if no sea access.
        _constructedBuildings &= ~BUILD_SHIPYARD;
    }

    // remove tavern from necromancer castle
    if ( Race::NECR == race && ( _constructedBuildings & BUILD_TAVERN ) ) {
        _constructedBuildings &= ~BUILD_TAVERN;
        const GameVersion version = Settings::Get().getCurrentMapInfo().version;

        if ( version == GameVersion::PRICE_OF_LOYALTY || version == GameVersion::RESURRECTION ) {
            _constructedBuildings |= BUILD_SHRINE;
        }
    }

    SetModes( ALLOW_TO_BUILD_TODAY );

    // end
    DEBUG_LOG( DBG_GAME, DBG_INFO,
               ( _constructedBuildings & BUILD_CASTLE ? "castle" : "town" )
                   << ": " << name << ", color: " << Color::String( GetColor() ) << ", race: " << Race::String( race ) )
}

void Castle::_setDefaultBuildings()
{
    _constructedBuildings |= DWELLING_MONSTER1;
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

    if ( dwelling2 >= Rand::Get( 1, 100 ) ) {
        _constructedBuildings |= DWELLING_MONSTER2;
    }
}

uint32_t Castle::CountBuildings() const
{
    uint32_t tavern = BUILD_TAVERN;
    if ( race == Race::NECR ) {
        const GameVersion version = Settings::Get().getCurrentMapInfo().version;
        if ( version == GameVersion::PRICE_OF_LOYALTY || version == GameVersion::RESURRECTION ) {
            tavern = BUILD_SHRINE;
        }
        else {
            tavern = BUILD_NOTHING;
        }
    }

    return CountBits( _constructedBuildings
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
            const Funds payment = PaymentConditions::BuySpellBook();
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
        if ( troop != nullptr && troop->isValid() && troop->isAllowUpgrade() ) {
            if ( GetRace() != troop->GetRace() ) {
                continue;
            }

            if ( !isBuild( troop->GetUpgrade().GetDwelling() ) ) {
                continue;
            }

            const Funds payment = troop->GetTotalUpgradeCost();
            if ( potentialFunds >= payment ) {
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
    if ( ( _constructedBuildings & buildingToCheck ) == 0 ) {
        return false;
    }

    const auto checkBuilding = [this]( const uint32_t expectedLevels, const uint32_t allPossibleLevels ) {
        // All expected levels should be built
        assert( ( _constructedBuildings & expectedLevels ) == expectedLevels );

        // Only the expected levels of all possible levels should be built
        return ( _constructedBuildings & allPossibleLevels ) == expectedLevels;
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

    SetModes( ALLOW_TO_BUILD_TODAY );
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

    const bool isNeutral = ( GetColor() == Color::NONE );
    const bool isPlagueWeek = ( world.GetWeekType().GetType() == WeekName::PLAGUE );
    const bool isMonsterWeek = ( world.GetWeekType().GetType() == WeekName::MONSTERS );

    if ( !isPlagueWeek ) {
        static const std::array<uint32_t, 6> basicDwellings
            = { DWELLING_MONSTER1, DWELLING_MONSTER2, DWELLING_MONSTER3, DWELLING_MONSTER4, DWELLING_MONSTER5, DWELLING_MONSTER6 };

        // Normal population growth
        for ( const uint32_t dwellingId : basicDwellings ) {
            uint32_t * dwellingMonsters = GetDwelling( dwellingId );
            if ( dwellingMonsters == nullptr ) {
                // Such dwelling (or its upgrade) has not been built
                continue;
            }

            uint32_t growth = Monster( race, GetActualDwelling( dwellingId ) ).GetGrown();

            if ( _constructedBuildings & BUILD_WELL ) {
                // The well is built.
                growth += GetGrownWell();
            }

            if ( ( dwellingId == DWELLING_MONSTER1 ) && ( _constructedBuildings & BUILD_WEL2 ) ) {
                growth += GetGrownWel2();
            }

            if ( isNeutral ) {
                // Neutral towns always have 50% population growth.
                growth /= 2;
            }

            *dwellingMonsters += growth;
        }

        if ( isMonsterWeek && !world.BeginMonth() ) {
            for ( const uint32_t dwellingId : allDwellings ) {
                // A building of exactly this level should be built (its upgraded versions should not be considered)
                if ( !isExactBuildingBuilt( dwellingId ) ) {
                    continue;
                }

                const Monster mons( race, dwellingId );

                if ( !mons.isValid() || mons.GetID() != world.GetWeekType().GetMonster() ) {
                    continue;
                }

                uint32_t * dwellingMonsters = GetDwelling( dwellingId );
                assert( dwellingMonsters != nullptr );

                *dwellingMonsters += GetGrownWeekOf();
                break;
            }
        }

        if ( isNeutral ) {
            // Neutral towns have additional increase in garrison army.
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

        if ( isPlagueWeek ) {
            for ( uint32_t & dwellingRef : dwelling ) {
                dwellingRef /= 2;
            }
        }
        else if ( isMonsterWeek ) {
            for ( const uint32_t dwellingId : allDwellings ) {
                // A building of exactly this level should be built (its upgraded versions should not be considered)
                if ( !isExactBuildingBuilt( dwellingId ) ) {
                    continue;
                }

                const Monster mons( race, dwellingId );

                if ( !mons.isValid() || mons.GetID() != world.GetWeekType().GetMonster() ) {
                    continue;
                }

                uint32_t * dwellingMonsters = GetDwelling( dwellingId );
                assert( dwellingMonsters != nullptr );

                *dwellingMonsters += *dwellingMonsters * GetGrownMonthOf() / 100;
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
    if ( _constructedBuildings & BUILD_MAGEGUILD5 )
        return 5;
    if ( _constructedBuildings & BUILD_MAGEGUILD4 )
        return 4;
    if ( _constructedBuildings & BUILD_MAGEGUILD3 )
        return 3;
    if ( _constructedBuildings & BUILD_MAGEGUILD2 )
        return 2;
    if ( _constructedBuildings & BUILD_MAGEGUILD1 )
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

bool Castle::isFortificationBuilt() const
{
    return race == Race::KNGT && isBuild( BUILD_SPEC );
}

const char * Castle::GetStringBuilding( uint32_t build, int race )
{
    return fheroes2::getBuildingName( race, static_cast<building_t>( build ) );
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
            *msg = _( "Cannot afford a Hero." );
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

    const Funds paymentCosts = troop.GetTotalCost();
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
    const Monster monster( race, GetActualDwelling( dw ) );
    assert( count <= getRecruitLimit( monster, GetKingdom().GetFunds() ) );

    const Troop troop( monster, std::min( count, getRecruitLimit( monster, GetKingdom().GetFunds() ) ) );

    if ( RecruitMonster( troop, false ) ) {
        return true;
    }

    // TODO: before removing an existing stack of monsters try to upgrade them and also merge some stacks.

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

void Castle::recruitBestAvailable( Funds budget )
{
    for ( uint32_t dw = DWELLING_MONSTER6; dw >= DWELLING_MONSTER1; dw >>= 1 ) {
        if ( !isBuild( dw ) ) {
            continue;
        }

        const Monster monster( race, GetActualDwelling( dw ) );
        const uint32_t willRecruit = getRecruitLimit( monster, budget );
        if ( willRecruit == 0 ) {
            continue;
        }

        if ( RecruitMonsterFromDwelling( dw, willRecruit, true ) ) {
            budget -= ( monster.GetCost() * willRecruit );
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

BuildingStatus Castle::CheckBuyBuilding( const uint32_t build ) const
{
    if ( build & _constructedBuildings ) {
        return BuildingStatus::ALREADY_BUILT;
    }

    if ( _disabledBuildings & build ) {
        return BuildingStatus::BUILD_DISABLE;
    }

    // TODO: remove these conditions and do calculation once per game.
    switch ( build ) {
    case BUILD_SHIPYARD:
        if ( !HasSeaAccess() ) {
            return BuildingStatus::SHIPYARD_NOT_ALLOWED;
        }
        break;
    case BUILD_SHRINE:
        if ( Race::NECR != GetRace() || ( Settings::Get().getCurrentMapInfo().version == GameVersion::SUCCESSION_WARS ) ) {
            return BuildingStatus::BUILD_DISABLE;
        }
        break;
    case BUILD_TAVERN:
        if ( Race::NECR == GetRace() ) {
            return BuildingStatus::BUILD_DISABLE;
        }
        break;
    default:
        break;
    }

    if ( build >= BUILD_MAGEGUILD2 && build <= BUILD_MAGEGUILD5 ) {
        const uint32_t prevMageGuild = build >> 1;

        if ( !( _constructedBuildings & prevMageGuild ) ) {
            return BuildingStatus::BUILD_DISABLE;
        }
    }

    if ( !Modes( ALLOW_TO_BUILD_TODAY ) ) {
        return BuildingStatus::NOT_TODAY;
    }

    if ( isCastle() ) {
        if ( build == BUILD_TENT ) {
            return BuildingStatus::BUILD_DISABLE;
        }
    }
    else {
        if ( build != BUILD_CASTLE ) {
            return BuildingStatus::NEED_CASTLE;
        }
    }

    switch ( build ) {
    case DWELLING_UPGRADE2:
        if ( ( Race::WRLK | Race::WZRD ) & race )
            return BuildingStatus::UNKNOWN_UPGRADE;
        break;
    case DWELLING_UPGRADE3:
        if ( ( Race::BARB | Race::WRLK ) & race )
            return BuildingStatus::UNKNOWN_UPGRADE;
        break;
    case DWELLING_UPGRADE4:
        if ( Race::WZRD & race )
            return BuildingStatus::UNKNOWN_UPGRADE;
        break;
    case DWELLING_UPGRADE5:
        if ( ( Race::SORC | Race::WRLK ) & race )
            return BuildingStatus::UNKNOWN_UPGRADE;
        break;
    case DWELLING_UPGRADE6:
        if ( ( Race::BARB | Race::SORC | Race::NECR ) & race )
            return BuildingStatus::UNKNOWN_UPGRADE;
        break;
    case DWELLING_UPGRADE7:
        if ( Race::WRLK != race )
            return BuildingStatus::UNKNOWN_UPGRADE;
        break;

    default:
        break;
    }

    const uint32_t requirement = fheroes2::getBuildingRequirement( race, static_cast<building_t>( build ) );

    for ( uint32_t itr = 0x00000001; itr; itr <<= 1 ) {
        if ( ( requirement & itr ) && !( _constructedBuildings & itr ) ) {
            return BuildingStatus::REQUIRES_BUILD;
        }
    }

    if ( !GetKingdom().AllowPayment( PaymentConditions::BuyBuilding( race, build ) ) ) {
        return BuildingStatus::LACK_RESOURCES;
    }

    return BuildingStatus::ALLOW_BUILD;
}

BuildingStatus Castle::GetAllBuildingStatus( const Castle & castle )
{
    if ( !castle.Modes( ALLOW_TO_BUILD_TODAY ) )
        return BuildingStatus::NOT_TODAY;
    if ( !castle.isCastle() )
        return BuildingStatus::NEED_CASTLE;

    const uint32_t rest = ~castle._constructedBuildings;

    for ( uint32_t itr = 0x00000001; itr; itr <<= 1 )
        if ( ( rest & itr ) && ( BuildingStatus::ALLOW_BUILD == castle.CheckBuyBuilding( itr ) ) )
            return BuildingStatus::ALLOW_BUILD;

    for ( uint32_t itr = 0x00000001; itr; itr <<= 1 )
        if ( ( rest & itr ) && ( BuildingStatus::LACK_RESOURCES == castle.CheckBuyBuilding( itr ) ) )
            return BuildingStatus::LACK_RESOURCES;

    for ( uint32_t itr = 0x00000001; itr; itr <<= 1 )
        if ( ( rest & itr ) && ( BuildingStatus::REQUIRES_BUILD == castle.CheckBuyBuilding( itr ) ) )
            return BuildingStatus::REQUIRES_BUILD;

    return BuildingStatus::UNKNOWN_COND;
}

bool Castle::AllowBuyBuilding( uint32_t build ) const
{
    return BuildingStatus::ALLOW_BUILD == CheckBuyBuilding( build );
}

bool Castle::BuyBuilding( uint32_t build )
{
    if ( !AllowBuyBuilding( build ) )
        return false;

    GetKingdom().OddFundsResource( PaymentConditions::BuyBuilding( race, build ) );

    // add build
    _constructedBuildings |= build;

    switch ( build ) {
    case BUILD_CASTLE:
        _constructedBuildings &= ~BUILD_TENT;
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
    ResetModes( ALLOW_TO_BUILD_TODAY );

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
    if ( !( BUILD_CASTLE & _constructedBuildings ) )
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

bool Castle::HasSeaAccess() const
{
    const fheroes2::Point possibleSeaTile{ center.x, center.y + 2 };
    if ( !Maps::isValidAbsPoint( possibleSeaTile.x, possibleSeaTile.y ) ) {
        // If a tile below doesn't exist then no reason to check other tiles.
        return false;
    }

    auto doesTileAllowsToPutBoat = []( const Maps::Tiles & tile ) {
        if ( !tile.isWater() ) {
            // No water, no boat.
            return false;
        }

        if ( tile.getObjectIcnType() == MP2::OBJ_ICN_TYPE_UNKNOWN ) {
            // The main addon does not exist on this tile.
            // This means that all objects on this tile are not primary objects (like shadows or some parts of objects).
            return true;
        }

        // If this is an object's shadow or this is an action object that can be removed then it is possible to put a boat here.
        const MP2::MapObjectType objectType = tile.GetObject();
        return MP2::isPickupObject( objectType ) || objectType == MP2::OBJ_BOAT || tile.isPassabilityTransparent();
    };

    const int32_t index = Maps::GetIndexFromAbsPoint( possibleSeaTile.x, possibleSeaTile.y );
    if ( doesTileAllowsToPutBoat( world.GetTiles( index ) ) ) {
        return true;
    }

    if ( Maps::isValidAbsPoint( possibleSeaTile.x - 1, possibleSeaTile.y ) && doesTileAllowsToPutBoat( world.GetTiles( index - 1 ) ) ) {
        return true;
    }

    if ( Maps::isValidAbsPoint( possibleSeaTile.x + 1, possibleSeaTile.y ) && doesTileAllowsToPutBoat( world.GetTiles( index + 1 ) ) ) {
        return true;
    }

    return false;
}

bool Castle::HasBoatNearby() const
{
    const fheroes2::Point possibleSeaTile{ center.x, center.y + 2 };
    if ( !Maps::isValidAbsPoint( possibleSeaTile.x, possibleSeaTile.y ) ) {
        // If a tile below doesn't exist then no reason to check other tiles.
        return false;
    }

    auto doesTileHaveBoat = []( const Maps::Tiles & tile ) {
        if ( !tile.isWater() ) {
            // No water, no boat.
            return false;
        }

        const MP2::MapObjectType objectType = tile.GetObject();
        return ( objectType == MP2::OBJ_BOAT || objectType == MP2::OBJ_HERO );
    };

    const int32_t index = Maps::GetIndexFromAbsPoint( possibleSeaTile.x, possibleSeaTile.y );
    if ( doesTileHaveBoat( world.GetTiles( index ) ) ) {
        return true;
    }

    if ( Maps::isValidAbsPoint( possibleSeaTile.x - 1, possibleSeaTile.y ) && doesTileHaveBoat( world.GetTiles( index - 1 ) ) ) {
        return true;
    }

    if ( Maps::isValidAbsPoint( possibleSeaTile.x + 1, possibleSeaTile.y ) && doesTileHaveBoat( world.GetTiles( index + 1 ) ) ) {
        return true;
    }

    return false;
}

int32_t Castle::getTileIndexToPlaceBoat() const
{
    const fheroes2::Point possibleSeaTile{ center.x, center.y + 2 };
    if ( !Maps::isValidAbsPoint( possibleSeaTile.x, possibleSeaTile.y ) ) {
        // If a tile below doesn't exist then no reason to check other tiles.
        return -1;
    }

    auto doesTileAllowsToPutBoat = []( const Maps::Tiles & tile ) {
        if ( !tile.isWater() ) {
            // No water, no boat.
            return false;
        }

        // Mark the tile as worthy to a place a boat if the main addon does not exist on this tile.
        // This means that all objects on this tile are not primary objects (like shadows or some parts of objects).
        return ( tile.getObjectIcnType() == MP2::OBJ_ICN_TYPE_UNKNOWN || tile.isPassabilityTransparent() );
    };

    const int32_t index = Maps::GetIndexFromAbsPoint( possibleSeaTile.x, possibleSeaTile.y );
    if ( doesTileAllowsToPutBoat( world.GetTiles( index ) ) ) {
        return index;
    }

    if ( Maps::isValidAbsPoint( possibleSeaTile.x - 1, possibleSeaTile.y ) && doesTileAllowsToPutBoat( world.GetTiles( index - 1 ) ) ) {
        return index - 1;
    }

    if ( Maps::isValidAbsPoint( possibleSeaTile.x + 1, possibleSeaTile.y ) && doesTileAllowsToPutBoat( world.GetTiles( index + 1 ) ) ) {
        return index + 1;
    }

    return -1;
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
        return _constructedBuildings & DWELLING_UPGRADE2 ? DWELLING_UPGRADE2 : buildId;
    case DWELLING_MONSTER3:
        return _constructedBuildings & DWELLING_UPGRADE3 ? DWELLING_UPGRADE3 : buildId;
    case DWELLING_MONSTER4:
        return _constructedBuildings & DWELLING_UPGRADE4 ? DWELLING_UPGRADE4 : buildId;
    case DWELLING_MONSTER5:
        return _constructedBuildings & DWELLING_UPGRADE5 ? DWELLING_UPGRADE5 : buildId;
    case DWELLING_MONSTER6:
        return _constructedBuildings & DWELLING_UPGRADE7 ? DWELLING_UPGRADE7 : ( _constructedBuildings & DWELLING_UPGRADE6 ? DWELLING_UPGRADE6 : buildId );
    case DWELLING_UPGRADE6:
        return _constructedBuildings & DWELLING_UPGRADE7 ? DWELLING_UPGRADE7 : buildId;
    default:
        break;
    }

    return BUILD_NOTHING;
}

uint32_t Castle::GetUpgradeBuilding( const uint32_t buildingId ) const
{
    if ( race == Race::WRLK && buildingId == DWELLING_MONSTER6 && isBuild( DWELLING_UPGRADE6 ) ) {
        // Warlock's dwelling 6 is a special case.
        return fheroes2::getUpgradeForBuilding( race, DWELLING_UPGRADE6 );
    }

    return fheroes2::getUpgradeForBuilding( race, static_cast<building_t>( buildingId ) );
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
       << "coast/has boat  : " << ( HasSeaAccess() ? "yes" : "no" ) << " / " << ( HasBoatNearby() ? "yes" : "no" ) << std::endl
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
    if ( isCastle() && totalStrength > 0.1 ) {
        const Battle::Tower tower( *this, Battle::TowerType::TWR_CENTER, 0 );
        const double towerStr = tower.GetStrengthWithBonus( tower.GetAttackBonus(), 0 );

        totalStrength += towerStr;
        if ( isBuild( BUILD_LEFTTURRET ) ) {
            totalStrength += towerStr / 2;
        }
        if ( isBuild( BUILD_RIGHTTURRET ) ) {
            totalStrength += towerStr / 2;
        }

        if ( attackingHero && !attackingHero->HasSecondarySkill( Skill::Secondary::BALLISTICS ) && attackingHero->GetArmy().isMeleeDominantArmy() ) {
            // Heavy penalty if the attacking hero does not have a ballistic skill, and his army is based on melee infantry
            totalStrength *= isBuild( BUILD_MOAT ) ? 1.45 : 1.25;
        }
        else {
            totalStrength *= isBuild( BUILD_MOAT ) ? 1.2 : 1.15;
        }
    }

    return totalStrength;
}

bool Castle::AllowBuyBoat( const bool checkPayment ) const
{
    if ( !isBuild( BUILD_SHIPYARD ) ) {
        return false;
    }

    if ( checkPayment && !GetKingdom().AllowPayment( PaymentConditions::BuyBoat() ) ) {
        return false;
    }

    if ( HasBoatNearby() ) {
        return false;
    }

    return getTileIndexToPlaceBoat() >= 0;
}

bool Castle::BuyBoat() const
{
    if ( !AllowBuyBoat( true ) ) {
        return false;
    }

    // If this assertion blows up then you didn't even check conditions to build a shipyard!
    assert( HasSeaAccess() );

    const int32_t index = getTileIndexToPlaceBoat();
    if ( index < 0 ) {
        return false;
    }

    if ( isControlHuman() ) {
        AudioManager::PlaySound( M82::BUILDTWN );
    }

    Kingdom & kingdom = GetKingdom();
    kingdom.OddFundsResource( PaymentConditions::BuyBoat() );
    world.GetTiles( index ).setBoat( Direction::RIGHT, kingdom.GetColor() );

    return true;
}

void Castle::setName( const std::set<std::string, std::less<>> & usedNames )
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
    return race == Race::NECR && ( BUILD_SHRINE & _constructedBuildings );
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
    if ( isControlAI() ) {
        AI::Planner::CastlePreBattle( *this );

        return;
    }

    Heroes * hero = world.GetHero( *this );
    if ( hero == nullptr ) {
        return;
    }

    hero->GetArmy().ArrangeForCastleDefense( army );
}

void Castle::ActionAfterBattle( bool attacker_wins )
{
    if ( attacker_wins ) {
        army.Clean();
        ResetModes( CUSTOM_ARMY );
    }
}

Castle * VecCastles::GetFirstCastle() const
{
    const_iterator iter = std::find_if( begin(), end(), []( const Castle * castle ) { return castle->isCastle(); } );
    if ( iter == end() ) {
        return nullptr;
    }

    return *iter;
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

    // Castles are added from top to bottom, from left to right.
    // Tiles containing castle ID cannot be overwritten.

    for ( int32_t y = -2; y <= 1; ++y ) {
        for ( int32_t x = -2; x <= 2; ++x ) {
            if ( y == 1 && x == 0 ) {
                // Do not mark a tile below castle's entrance as castle.
                continue;
            }

            const auto [dummy, inserted] = _castleTiles.try_emplace( center + fheroes2::Point( x, y ), id );
            if ( !inserted ) {
                DEBUG_LOG( DBG_GAME, DBG_INFO, "Tile [" << center.x + x << ", " << center.y + y << "] is occupied by another castle" )
            }
        }
    }

    const auto [dummy, inserted] = _castleTiles.try_emplace( center + fheroes2::Point( 0, -3 ), id );
    if ( !inserted ) {
        DEBUG_LOG( DBG_GAME, DBG_INFO, "Tile [" << center.x << ", " << center.y - 3 << "] is occupied by another castle" )
    }
}

void AllCastles::Scout( int colors ) const
{
    for ( auto it = begin(); it != end(); ++it )
        if ( colors & ( *it )->GetColor() )
            ( *it )->Scout();
}

StreamBase & operator<<( StreamBase & msg, const Castle & castle )
{
    const ColorBase & color = castle;

    msg << static_cast<const MapPosition &>( castle ) << castle.modes << castle.race << castle._constructedBuildings << castle._disabledBuildings << castle.captain
        << color << castle.name << castle.mageguild << static_cast<uint32_t>( castle.dwelling.size() );

    for ( const uint32_t dwelling : castle.dwelling ) {
        msg << dwelling;
    }

    return msg << castle.army;
}

StreamBase & operator>>( StreamBase & msg, Castle & castle )
{
    msg >> static_cast<MapPosition &>( castle ) >> castle.modes >> castle.race >> castle._constructedBuildings;

    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_1101_RELEASE, "Remove the logic below." );
    if ( Game::GetVersionOfCurrentSaveFile() < FORMAT_VERSION_1101_RELEASE ) {
        if ( !castle.Modes( Castle::UNUSED_ALLOW_CASTLE_CONSTRUCTION ) ) {
            castle._disabledBuildings = BUILD_CASTLE;
        }
    }
    else {
        msg >> castle._disabledBuildings;
    }

    ColorBase & color = castle;
    msg >> castle.captain >> color >> castle.name >> castle.mageguild;

    uint32_t dwellingcount;
    msg >> dwellingcount;

    if ( dwellingcount != castle.dwelling.size() ) {
        // Is it a corrupted save?
        assert( 0 );

        castle.dwelling = { 0 };
    }
    else {
        for ( uint32_t & dwelling : castle.dwelling ) {
            msg >> dwelling;
        }
    }

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
    std::string res = fheroes2::getBuildingDescription( GetRace(), static_cast<building_t>( build ) );

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
            res.append( "\n\n" );
            res.append( Battle::Tower::GetInfo( *this ) );
        }

        if ( isBuild( BUILD_MOAT ) ) {
            res.append( "\n\n" );
            res.append( Battle::Board::GetMoatInfo() );
        }
        break;
    }

    case BUILD_SPEC:
    case BUILD_STATUE: {
        const Funds profit = ProfitConditions::FromBuilding( build, GetRace() );
        StringReplace( res, "%{count}", profit.gold );
        break;
    }

    default:
        break;
    }

    return res;
}
