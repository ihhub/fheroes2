/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
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

#include "agg.h"
#include "ai.h"
#include "battle_board.h"
#include "battle_tower.h"
#include "castle.h"
#include "dialog.h"
#include "difficulty.h"
#include "game.h"
#include "game_static.h"
#include "ground.h"
#include "kingdom.h"
#include "luck.h"
#include "maps_tiles.h"
#include "morale.h"
#include "payment.h"
#include "profit.h"
#include "race.h"
#include "settings.h"
#include "text.h"
#include "world.h"

Castle::Castle()
    : race( Race::NONE )
    , building( 0 )
    , captain( *this )
    , army( NULL )
{
    std::fill( dwelling, dwelling + CASTLEMAXMONSTER, 0 );
    army.SetCommander( &captain );
}

Castle::Castle( s32 cx, s32 cy, int rc )
    : MapPosition( Point( cx, cy ) )
    , race( rc )
    , building( 0 )
    , captain( *this )
    , army( NULL )
{
    std::fill( dwelling, dwelling + CASTLEMAXMONSTER, 0 );
    army.SetCommander( &captain );
}

void Castle::LoadFromMP2( StreamBuf st )
{
    switch ( st.get() ) {
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

    // custom building
    if ( st.get() ) {
        // building
        int build = st.getLE16();
        if ( 0x0002 & build )
            building |= BUILD_THIEVESGUILD;
        if ( 0x0004 & build )
            building |= BUILD_TAVERN;
        if ( 0x0008 & build )
            building |= BUILD_SHIPYARD;
        if ( 0x0010 & build )
            building |= BUILD_WELL;
        if ( 0x0080 & build )
            building |= BUILD_STATUE;
        if ( 0x0100 & build )
            building |= BUILD_LEFTTURRET;
        if ( 0x0200 & build )
            building |= BUILD_RIGHTTURRET;
        if ( 0x0400 & build )
            building |= BUILD_MARKETPLACE;
        if ( 0x1000 & build )
            building |= BUILD_MOAT;
        if ( 0x0800 & build )
            building |= BUILD_WEL2;
        if ( 0x2000 & build )
            building |= BUILD_SPEC;

        // dwelling
        int dwell = st.getLE16();
        if ( 0x0008 & dwell )
            building |= DWELLING_MONSTER1;
        if ( 0x0010 & dwell )
            building |= DWELLING_MONSTER2;
        if ( 0x0020 & dwell )
            building |= DWELLING_MONSTER3;
        if ( 0x0040 & dwell )
            building |= DWELLING_MONSTER4;
        if ( 0x0080 & dwell )
            building |= DWELLING_MONSTER5;
        if ( 0x0100 & dwell )
            building |= DWELLING_MONSTER6;
        if ( 0x0200 & dwell )
            building |= DWELLING_UPGRADE2 | DWELLING_MONSTER2;
        if ( 0x0400 & dwell )
            building |= DWELLING_UPGRADE3 | DWELLING_MONSTER3;
        if ( 0x0800 & dwell )
            building |= DWELLING_UPGRADE4 | DWELLING_MONSTER4;
        if ( 0x1000 & dwell )
            building |= DWELLING_UPGRADE5 | DWELLING_MONSTER5;
        if ( 0x2000 & dwell )
            building |= DWELLING_UPGRADE6 | DWELLING_MONSTER6;

        // magic tower
        int level = st.get();
        if ( 0 < level )
            building |= BUILD_MAGEGUILD1;
        if ( 1 < level )
            building |= BUILD_MAGEGUILD2;
        if ( 2 < level )
            building |= BUILD_MAGEGUILD3;
        if ( 3 < level )
            building |= BUILD_MAGEGUILD4;
        if ( 4 < level )
            building |= BUILD_MAGEGUILD5;
    }
    else {
        st.skip( 5 );

        // default building
        building |= DWELLING_MONSTER1;
        u32 dwelling2 = 0;
        switch ( Settings::Get().GameDifficulty() ) {
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

    // custom troops
    bool custom_troops = st.get();
    if ( custom_troops ) {
        Troop troops[5];

        // set monster id
        for ( u32 ii = 0; ii < ARRAY_COUNT( troops ); ++ii )
            troops[ii].SetMonster( st.get() + 1 );

        // set count
        for ( u32 ii = 0; ii < ARRAY_COUNT( troops ); ++ii )
            troops[ii].SetCount( st.getLE16() );

        army.Assign( troops, ARRAY_COUNT_END( troops ) );
        SetModes( CUSTOMARMY );
    }
    else
        st.skip( 15 );

    // captain
    if ( st.get() )
        building |= BUILD_CAPTAIN;

    // custom name
    st.skip( 1 );
    name = Game::GetEncodeString( st.toString( 13 ) );

    // race
    u32 kingdom_race = Players::GetPlayerRace( GetColor() );
    switch ( st.get() ) {
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
    default:
        race = ( Color::NONE != GetColor() && ( Race::ALL & kingdom_race ) ? kingdom_race : Race::Rand() );
        break;
    }

    // castle
    building |= st.get() ? BUILD_CASTLE : BUILD_TENT;

    // allow upgrade to castle (0 - true, 1 - false)
    if ( st.get() )
        ResetModes( ALLOWCASTLE );
    else
        SetModes( ALLOWCASTLE );

    // unknown 29 byte
    //

    PostLoad();
}

void Castle::PostLoad( void )
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

    // fix upgrade dwelling dependend from race
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
        building &= ~( DWELLING_UPGRADE6 );
        break;
    default:
        break;
    }

    army.SetColor( GetColor() );

    // fix captain
    if ( building & BUILD_CAPTAIN )
        captain.LoadDefaults( HeroBase::CAPTAIN, race );

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
        building &= ~( BUILD_SHIPYARD );

    // remove tavern from necromancer castle
    if ( Race::NECR == race && ( building & BUILD_TAVERN ) ) {
        building &= ~BUILD_TAVERN;
        if ( Settings::Get().PriceLoyaltyVersion() )
            building |= BUILD_SHRINE;
    }

    SetModes( ALLOWBUILD );

    // end
    DEBUG( DBG_GAME, DBG_INFO,
           ( building & BUILD_CASTLE ? "castle" : "town" ) << ": " << name << ", color: " << Color::String( GetColor() ) << ", race: " << Race::String( race ) );
}

Captain & Castle::GetCaptain( void )
{
    return captain;
}

const Captain & Castle::GetCaptain( void ) const
{
    return captain;
}

bool Castle::isCastle( void ) const
{
    return ( building & BUILD_CASTLE ) != 0;
}

bool Castle::isCapital( void ) const
{
    return Modes( CAPITAL );
}

u32 Castle::CountBuildings( void ) const
{
    const u32 tavern = ( race == Race::NECR ? ( Settings::Get().PriceLoyaltyVersion() ? BUILD_SHRINE : BUILD_NOTHING ) : BUILD_TAVERN );

    return CountBits( building
                      & ( BUILD_THIEVESGUILD | tavern | BUILD_SHIPYARD | BUILD_WELL | BUILD_STATUE | BUILD_LEFTTURRET | BUILD_RIGHTTURRET | BUILD_MARKETPLACE | BUILD_WEL2
                          | BUILD_MOAT | BUILD_SPEC | BUILD_CAPTAIN | BUILD_CASTLE | BUILD_MAGEGUILD1 | DWELLING_MONSTER1 | DWELLING_MONSTER2 | DWELLING_MONSTER3
                          | DWELLING_MONSTER4 | DWELLING_MONSTER5 | DWELLING_MONSTER6 ) );
}

bool Castle::isPosition( const Point & pt ) const
{
    const Point & mp = GetCenter();

    /*
              -
             ---
            -+++-
            ++X++
    */

    return ( ( pt.x >= mp.x - 1 && pt.x <= mp.x + 1 && ( pt.y == mp.y - 1 || pt.y == mp.y ) ) || ( ( pt.x == mp.x - 2 || pt.x == mp.x + 2 ) && pt.y == mp.y ) );
}

void Castle::EducateHeroes( void )
{
    // for learns new spells need 1 day
    if ( GetLevelMageGuild() ) {
        CastleHeroes heroes = world.GetHeroes( *this );

        if ( heroes.FullHouse() ) {
            MageGuildEducateHero( *heroes.Guest() );
            MageGuildEducateHero( *heroes.Guard() );
        }
        else if ( heroes.IsValid() )
            MageGuildEducateHero( *heroes.GuestFirst() );

        // captain
        if ( captain.isValid() )
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

double Castle::getVisitValue( const Heroes & hero ) const
{
    double spellValue = 0;
    const SpellStorage & guildSpells = mageguild.GetSpells( GetLevelMageGuild(), isLibraryBuild() );
    for ( const Spell & spell : guildSpells ) {
        if ( hero.CanLearnSpell( spell ) && !hero.HaveSpell( spell, true ) ) {
            spellValue += spell.Level() * 250.0;
        }
    }

    // we don't spend actual funds, so make a copy here
    Funds potentialFunds = GetKingdom().GetFunds();
    Troops reinforcement;
    for ( uint32_t dw = DWELLING_MONSTER6; dw >= DWELLING_MONSTER1; dw >>= 1 ) {
        if ( isBuild( dw ) ) {
            const Monster monster( race, GetActualDwelling( dw ) );
            const uint32_t available = getMonstersInDwelling( dw );

            uint32_t couldRecruit = potentialFunds.getLowestQuotient( monster.GetCost() );
            if ( available < couldRecruit )
                couldRecruit = available;

            potentialFunds -= ( monster.GetCost() * couldRecruit );

            reinforcement.PushBack( monster, couldRecruit );
        }
    }

    return spellValue + hero.GetArmy().getReinforcementValue( reinforcement );
}

void Castle::ActionNewDay( void )
{
    EducateHeroes();

    SetModes( ALLOWBUILD );
}

u32 * Castle::GetDwelling( u32 dw )
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
    return NULL;
}

void Castle::ActionNewWeek( void )
{
    ResetModes( DISABLEHIRES );
    const bool isNeutral = GetColor() == Color::NONE;

    // increase population
    if ( world.GetWeekType().GetType() != Week::PLAGUE ) {
        const u32 dwellings1[] = {DWELLING_MONSTER1, DWELLING_MONSTER2, DWELLING_MONSTER3, DWELLING_MONSTER4, DWELLING_MONSTER5, DWELLING_MONSTER6, 0};
        u32 * dw = NULL;

        // simple growth
        for ( u32 ii = 0; dwellings1[ii]; ++ii )
            if ( NULL != ( dw = GetDwelling( dwellings1[ii] ) ) ) {
                u32 growth = Monster( race, GetActualDwelling( dwellings1[ii] ) ).GetGrown();

                // well build
                if ( building & BUILD_WELL )
                    growth += GetGrownWell();

                // wel2 extras
                if ( ( dwellings1[ii] == DWELLING_MONSTER1 ) && ( building & BUILD_WEL2 ) )
                    growth += GetGrownWel2();

                if ( isControlAI() )
                    growth *= Difficulty::GetUnitGrowthBonus( Settings::Get().GameDifficulty() );

                // neutral town: half population (normal for begin month)
                if ( isNeutral && !world.BeginMonth() )
                    growth /= 2;

                *dw += growth;
            }

        // Week Of
        if ( world.GetWeekType().GetType() == Week::MONSTERS && !world.BeginMonth() ) {
            const u32 dwellings2[] = {DWELLING_MONSTER1, DWELLING_UPGRADE2, DWELLING_UPGRADE3, DWELLING_UPGRADE4, DWELLING_UPGRADE5,
                                      DWELLING_MONSTER2, DWELLING_MONSTER3, DWELLING_MONSTER4, DWELLING_MONSTER5, 0};

            for ( u32 ii = 0; dwellings2[ii]; ++ii )
                if ( NULL != ( dw = GetDwelling( dwellings2[ii] ) ) ) {
                    const Monster mons( race, dwellings2[ii] );
                    if ( mons.isValid() && mons.GetID() == world.GetWeekType().GetMonster() ) {
                        *dw += GetGrownWeekOf( mons );
                        break;
                    }
                }
        }

        // neutral town: increase garrisons
        if ( isNeutral && !Modes( CUSTOMARMY ) ) {
            JoinRNDArmy();
            // if it's a town there's 40% chance (or it's a castle) to get extra troops
            if ( isCastle() || Rand::Get( 1, 100 ) <= 40 )
                JoinRNDArmy();
        }
    }
}

void Castle::ActionNewMonth( void )
{
    // population halved
    if ( world.GetWeekType().GetType() == Week::PLAGUE ) {
        for ( u32 ii = 0; ii < CASTLEMAXMONSTER; ++ii )
            if ( dwelling[ii] )
                dwelling[ii] /= 2;
    }
    else
        // Month Of
        if ( world.GetWeekType().GetType() == Week::MONSTERS ) {
        const u32 dwellings[] = {DWELLING_MONSTER1, DWELLING_UPGRADE2, DWELLING_UPGRADE3, DWELLING_UPGRADE4, DWELLING_UPGRADE5,
                                 DWELLING_MONSTER2, DWELLING_MONSTER3, DWELLING_MONSTER4, DWELLING_MONSTER5, 0};
        u32 * dw = NULL;

        for ( u32 ii = 0; dwellings[ii]; ++ii )
            if ( NULL != ( dw = GetDwelling( dwellings[ii] ) ) ) {
                const Monster mons( race, dwellings[ii] );
                if ( mons.isValid() && mons.GetID() == world.GetWeekType().GetMonster() ) {
                    *dw += *dw * GetGrownMonthOf() / 100;
                    break;
                }
            }
    }
}

// change castle color
void Castle::ChangeColor( int cl )
{
    SetColor( cl );
    army.SetColor( cl );
}

// return mage guild level
int Castle::GetLevelMageGuild( void ) const
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

const MageGuild & Castle::GetMageGuild( void ) const
{
    return mageguild;
}

bool Castle::HaveLibraryCapability( void ) const
{
    return race == Race::WZRD;
}

bool Castle::isLibraryBuild( void ) const
{
    return race == Race::WZRD && isBuild( BUILD_SPEC );
}

void Castle::MageGuildEducateHero( HeroBase & hero ) const
{
    mageguild.educateHero( hero, GetLevelMageGuild(), isLibraryBuild() );
}

const char * Castle::GetStringBuilding( u32 build, int race )
{
    const char * str_build[] = {_( "Thieves' Guild" ),
                                _( "Tavern" ),
                                _( "Shipyard" ),
                                _( "Well" ),
                                _( "Statue" ),
                                _( "Left Turret" ),
                                _( "Right Turret" ),
                                _( "Marketplace" ),
                                _( "Moat" ),
                                _( "Castle" ),
                                _( "Tent" ),
                                _( "Captain's Quarters" ),
                                _( "Mage Guild, Level 1" ),
                                _( "Mage Guild, Level 2" ),
                                _( "Mage Guild, Level 3" ),
                                _( "Mage Guild, Level 4" ),
                                _( "Mage Guild, Level 5" ),
                                "Unknown"};

    const char * str_wel2[] = {_( "Farm" ), _( "Garbage Heap" ), _( "Crystal Garden" ), _( "Waterfall" ), _( "Orchard" ), _( "Skull Pile" )};

    const char * str_spec[] = {_( "Fortifications" ), _( "Coliseum" ), _( "Rainbow" ), _( "Dungeon" ), _( "Library" ), _( "Storm" )};

    const char * str_dwelling[] = {_( "Thatched Hut" ),   _( "Hut" ),       _( "Treehouse" ),     _( "Cave" ),        _( "Habitat" ),      _( "Excavation" ),
                                   _( "Archery Range" ),  _( "Stick Hut" ), _( "Cottage" ),       _( "Crypt" ),       _( "Pen" ),          _( "Graveyard" ),
                                   _( "Blacksmith" ),     _( "Den" ),       _( "Archery Range" ), _( "Nest" ),        _( "Foundry" ),      _( "Pyramid" ),
                                   _( "Armory" ),         _( "Adobe" ),     _( "Stonehenge" ),    _( "Maze" ),        _( "Cliff Nest" ),   _( "Mansion" ),
                                   _( "Jousting Arena" ), _( "Bridge" ),    _( "Fenced Meadow" ), _( "Swamp" ),       _( "Ivory Tower" ),  _( "Mausoleum" ),
                                   _( "Cathedral" ),      _( "Pyramid" ),   _( "Red Tower" ),     _( "Green Tower" ), _( "Cloud Castle" ), _( "Laboratory" )};

    const char * str_upgrade[] = {_( "Upg. Archery Range" ),
                                  _( "Upg. Stick Hut" ),
                                  _( "Upg. Cottage" ),
                                  _( "Crypt" ),
                                  _( "Pen" ),
                                  _( "Upg. Graveyard" ),
                                  _( "Upg. Blacksmith" ),
                                  _( "Den" ),
                                  _( "Upg. Archery Range" ),
                                  _( "Nest" ),
                                  _( "Upg. Foundry" ),
                                  _( "Upg. Pyramid" ),
                                  _( "Upg. Armory" ),
                                  _( "Upg. Adobe" ),
                                  _( "Upg. Stonehenge" ),
                                  _( "Upg. Maze" ),
                                  _( "Cliff Nest" ),
                                  _( "Upg. Mansion" ),
                                  _( "Upg. Jousting Arena" ),
                                  _( "Upg. Bridge" ),
                                  _( "Fenced Meadow" ),
                                  _( "Swamp" ),
                                  _( "Upg. Ivory Tower" ),
                                  _( "Upg. Mausoleum" ),
                                  _( "Upg. Cathedral" ),
                                  _( "Pyramid" ),
                                  _( "Red Tower" ),
                                  _( "Red Tower" ),
                                  _( "Upg. Cloud Castle" ),
                                  _( "Laboratory" ),
                                  "",
                                  "",
                                  "",
                                  _( "Black Tower" ),
                                  "",
                                  ""};

    const char * shrine = _( "Shrine" );

    u32 offset = 0;

    switch ( race ) {
    case Race::KNGT:
        offset = 0;
        break;
    case Race::BARB:
        offset = 1;
        break;
    case Race::SORC:
        offset = 2;
        break;
    case Race::WRLK:
        offset = 3;
        break;
    case Race::WZRD:
        offset = 4;
        break;
    case Race::NECR:
        offset = 5;
        break;
    default:
        break;
    }

    switch ( build ) {
    case BUILD_SHRINE:
        return shrine;
    case BUILD_THIEVESGUILD:
        return str_build[0];
    case BUILD_TAVERN:
        return str_build[1];
    case BUILD_SHIPYARD:
        return str_build[2];
    case BUILD_WELL:
        return str_build[3];
    case BUILD_STATUE:
        return str_build[4];
    case BUILD_LEFTTURRET:
        return str_build[5];
    case BUILD_RIGHTTURRET:
        return str_build[6];
    case BUILD_MARKETPLACE:
        return str_build[7];
    case BUILD_MOAT:
        return str_build[8];
    case BUILD_CASTLE:
        return str_build[9];
    case BUILD_TENT:
        return str_build[10];
    case BUILD_CAPTAIN:
        return str_build[11];
    case BUILD_MAGEGUILD1:
        return str_build[12];
    case BUILD_MAGEGUILD2:
        return str_build[13];
    case BUILD_MAGEGUILD3:
        return str_build[14];
    case BUILD_MAGEGUILD4:
        return str_build[15];
    case BUILD_MAGEGUILD5:
        return str_build[16];

    case BUILD_SPEC:
        return str_spec[offset];
    case BUILD_WEL2:
        return str_wel2[offset];

    case DWELLING_MONSTER1:
        return str_dwelling[offset];
    case DWELLING_MONSTER2:
        return str_dwelling[6 + offset];
    case DWELLING_MONSTER3:
        return str_dwelling[12 + offset];
    case DWELLING_MONSTER4:
        return str_dwelling[18 + offset];
    case DWELLING_MONSTER5:
        return str_dwelling[24 + offset];
    case DWELLING_MONSTER6:
        return str_dwelling[30 + offset];

    case DWELLING_UPGRADE2:
        return str_upgrade[offset];
    case DWELLING_UPGRADE3:
        return str_upgrade[6 + offset];
    case DWELLING_UPGRADE4:
        return str_upgrade[12 + offset];
    case DWELLING_UPGRADE5:
        return str_upgrade[18 + offset];
    case DWELLING_UPGRADE6:
        return str_upgrade[24 + offset];
    case DWELLING_UPGRADE7:
        return str_upgrade[30 + offset];

    default:
        break;
    }

    return str_build[17];
}

const char * Castle::GetDescriptionBuilding( u32 build, int race )
{
    const char * desc_build[] = {
        _( "The Thieves' Guild provides information on enemy players. Thieves' Guilds can also provide scouting information on enemy towns. Additional Guilds provide more information." ),
        _( "The Tavern increases morale for troops defending the castle." ),
        _( "The Shipyard allows ships to be built." ),
        _( "The Well increases the growth rate of all dwellings by %{count} creatures per week." ),
        _( "The Statue increases your town's income by %{count} per day." ),
        _( "The Left Turret provides extra firepower during castle combat." ),
        _( "The Right Turret provides extra firepower during castle combat." ),
        _( "The Marketplace can be used to convert one type of resource into another. The more marketplaces you control, the better the exchange rate." ),
        _( "The Moat slows attacking units. Any unit entering the moat must end its turn there and becomes more vulnerable to attack." ),
        _( "The Castle improves town defense and increases income to %{count} gold per day." ),
        _( "The Tent provides workers to build a castle, provided the materials and the gold are available." ),
        _( "The Captain's Quarters provides a captain to assist in the castle's defense when no hero is present." ),
        _( "The Mage Guild allows heroes to learn spells and replenish their spell points." ),
        "Unknown"};

    const char * desc_wel2[] = {_( "The Farm increases production of Peasants by %{count} per week." ),
                                _( "The Garbage Heap increases production of Goblins by %{count} per week." ),
                                _( "The Crystal Garden increases production of Sprites by %{count} per week." ),
                                _( "The Waterfall increases production of Centaurs by %{count} per week." ),
                                _( "The Orchard increases production of Halflings by %{count} per week." ),
                                _( "The Skull Pile increases production of Skeletons by %{count} per week." )};

    const char * desc_spec[] = {_( "The Fortifications increase the toughness of the walls, increasing the number of turns it takes to knock them down." ),
                                _( "The Coliseum provides inspiring spectacles to defending troops, raising their morale by two during combat." ),
                                _( "The Rainbow increases the luck of the defending units by two." ),
                                _( "The Dungeon increases the income of the town by %{count} / day." ),
                                _( "The Library increases the number of spells in the Guild by one for each level of the guild." ),
                                _( "The Storm adds +2 to the power of spells of a defending spell caster." )};

    const char * shrine_descr = _( "The Shrine increases the necromancy skill of all your necromancers by 10 percent." );

    u32 offset = 0;

    switch ( race ) {
    case Race::KNGT:
        offset = 0;
        break;
    case Race::BARB:
        offset = 1;
        break;
    case Race::SORC:
        offset = 2;
        break;
    case Race::WRLK:
        offset = 3;
        break;
    case Race::WZRD:
        offset = 4;
        break;
    case Race::NECR:
        offset = 5;
        break;
    default:
        break;
    }

    switch ( build ) {
    case BUILD_SHRINE:
        return shrine_descr;
    case BUILD_THIEVESGUILD:
        return desc_build[0];
    case BUILD_TAVERN:
        return desc_build[1];
    case BUILD_SHIPYARD:
        return desc_build[2];
    case BUILD_WELL:
        return desc_build[3];
    case BUILD_STATUE:
        return desc_build[4];
    case BUILD_LEFTTURRET:
        return desc_build[5];
    case BUILD_RIGHTTURRET:
        return desc_build[6];
    case BUILD_MARKETPLACE:
        return desc_build[7];
    case BUILD_MOAT:
        return desc_build[8];
    case BUILD_CASTLE:
        return desc_build[9];
    case BUILD_TENT:
        return desc_build[10];
    case BUILD_CAPTAIN:
        return desc_build[11];
    case BUILD_MAGEGUILD1:
    case BUILD_MAGEGUILD2:
    case BUILD_MAGEGUILD3:
    case BUILD_MAGEGUILD4:
    case BUILD_MAGEGUILD5:
        return desc_build[12];

    case BUILD_SPEC:
        return desc_spec[offset];
    case BUILD_WEL2:
        return desc_wel2[offset];

    default:
        break;
    }

    return desc_build[13];
}

bool Castle::AllowBuyHero( const Heroes & hero, std::string * msg )
{
    const Kingdom & myKingdom = GetKingdom();
    if ( Modes( DISABLEHIRES ) || myKingdom.Modes( Kingdom::DISABLEHIRES ) ) {
        if ( msg )
            *msg = _( "Cannot recruit - you already recruit hero in current week." );
        return false;
    }

    CastleHeroes heroes = world.GetHeroes( *this );

    if ( heroes.Guest() ) {
        // allow recruit with auto move guest to guard
        if ( Settings::Get().ExtCastleAllowGuardians() && !heroes.Guard() ) {
            if ( !heroes.Guest()->GetArmy().CanJoinTroops( army ) ) {
                if ( msg )
                    *msg = _( "Cannot recruit - guest to guard automove error." );
                return false;
            }
        }
        else {
            if ( msg )
                *msg = _( "Cannot recruit - you already have a Hero in this town." );
            return false;
        }
    }

    if ( !myKingdom.AllowRecruitHero( false, hero.GetLevel() ) ) {
        if ( msg )
            *msg = _( "Cannot recruit - you have too many Heroes." );
        return false;
    }

    if ( !myKingdom.AllowRecruitHero( true, hero.GetLevel() ) ) {
        if ( msg )
            *msg = _( "Cannot afford a Hero" );
        return false;
    }

    return true;
}

Heroes * Castle::RecruitHero( Heroes * hero )
{
    if ( !hero || !AllowBuyHero( *hero ) )
        return NULL;

    CastleHeroes heroes = world.GetHeroes( *this );
    if ( heroes.Guest() ) {
        if ( Settings::Get().ExtCastleAllowGuardians() && !heroes.Guard() ) {
            // move guest to guard
            SwapCastleHeroes( heroes );
        }
        else
            return NULL;
    }

    // recruit
    if ( !hero->Recruit( *this ) )
        return NULL;

    Kingdom & kingdom = GetKingdom();

    if ( kingdom.GetLastLostHero() == hero )
        kingdom.ResetLastLostHero();

    // actually update available heroes to recruit
    kingdom.GetRecruits();

    kingdom.OddFundsResource( PaymentConditions::RecruitHero( hero->GetLevel() ) );

    // update spell book
    if ( GetLevelMageGuild() )
        MageGuildEducateHero( *hero );

    if ( Settings::Get().ExtWorldOneHeroHiredEveryWeek() )
        kingdom.SetModes( Kingdom::DISABLEHIRES );

    if ( Settings::Get().ExtCastleOneHeroHiredEveryWeek() )
        SetModes( DISABLEHIRES );

    DEBUG( DBG_GAME, DBG_INFO, name << ", recruit: " << hero->GetName() );

    return hero;
}

/* recruit monster from building to castle army */
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

    Monster monster = troop;
    uint32_t count = troop.GetCount();

    // fix count
    if ( dwelling[dwellingIndex] < count )
        count = dwelling[dwellingIndex];

    // buy
    const payment_t paymentCosts = monster.GetCost() * count;
    Kingdom & kingdom = GetKingdom();

    if ( !kingdom.AllowPayment( paymentCosts ) )
        return false;

    // first: guard army join
    if ( !GetArmy().JoinTroop( monster, count ) ) {
        CastleHeroes heroes = world.GetHeroes( *this );

        if ( !heroes.Guest() || !heroes.Guest()->GetArmy().JoinTroop( monster, count ) ) {
            if ( showDialog ) {
                Dialog::Message( "", _( "There is no room in the garrison for this army." ), Font::BIG, Dialog::OK );
            }
            return false;
        }
    }

    kingdom.OddFundsResource( paymentCosts );
    dwelling[dwellingIndex] -= count;

    DEBUG( DBG_GAME, DBG_TRACE, name << " recruit: " << monster.GetMultiName() << "(" << count << ")" );

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
                DEBUG( DBG_GAME, DBG_INFO,
                       name << ": " << troop.GetCount() << " " << troop.GetMultiName() << " replace " << weak->GetCount() << " " << weak->GetMultiName() );
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
u32 Castle::getMonstersInDwelling( u32 dw ) const
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
u32 Castle::GetBuildingRequirement( u32 build ) const
{
    u32 requirement = 0;

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
            requirement |= DWELLING_MONSTER2;
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
            requirement |= DWELLING_MONSTER2;
            requirement |= DWELLING_MONSTER3;
            requirement |= DWELLING_MONSTER4;
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
            requirement |= DWELLING_MONSTER2;
            requirement |= DWELLING_MONSTER3;
            requirement |= DWELLING_MONSTER4;
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

/* check allow buy building */
int Castle::CheckBuyBuilding( u32 build ) const
{
    if ( build & building )
        return ALREADY_BUILT;

    switch ( build ) {
    // allow build castle
    case BUILD_CASTLE:
        if ( !Modes( ALLOWCASTLE ) )
            return BUILD_DISABLE;
        break;
    // buid shipyard only nearly sea
    case BUILD_SHIPYARD:
        if ( !HaveNearlySea() )
            return BUILD_DISABLE;
        break;
    case BUILD_SHRINE:
        if ( Race::NECR != GetRace() || !Settings::Get().PriceLoyaltyVersion() )
            return BUILD_DISABLE;
        break;
    case BUILD_TAVERN:
        if ( Race::NECR == GetRace() )
            return BUILD_DISABLE;
        break;

    default:
        break;
    }

    if ( !Modes( ALLOWBUILD ) )
        return NOT_TODAY;

    if ( isCastle() ) {
        if ( build == BUILD_TENT )
            return BUILD_DISABLE;
    }
    else {
        if ( build != BUILD_CASTLE )
            return NEED_CASTLE;
    }

    switch ( build ) {
        // check upgrade dwelling
    case DWELLING_UPGRADE2:
        if ( ( Race::WRLK | Race::WZRD ) & race )
            return UNKNOWN_UPGRADE;
        break;
    case DWELLING_UPGRADE3:
        if ( ( Race::BARB | Race::WRLK ) & race )
            return UNKNOWN_UPGRADE;
        break;
    case DWELLING_UPGRADE4:
        if ( (Race::WZRD)&race )
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

    // check build requirements
    const u32 requirement = Castle::GetBuildingRequirement( build );

    for ( u32 itr = 0x00000001; itr; itr <<= 1 )
        if ( ( requirement & itr ) && !( building & itr ) )
            return REQUIRES_BUILD;

    // check valid payment
    if ( !GetKingdom().AllowPayment( PaymentConditions::BuyBuilding( race, build ) ) )
        return LACK_RESOURCES;

    return ALLOW_BUILD;
}

int Castle::GetAllBuildingStatus( const Castle & castle )
{
    if ( !castle.Modes( ALLOWBUILD ) )
        return NOT_TODAY;
    if ( !castle.isCastle() )
        return NEED_CASTLE;

    const u32 rest = ~castle.building;

    for ( u32 itr = 0x00000001; itr; itr <<= 1 )
        if ( ( rest & itr ) && ( ALLOW_BUILD == castle.CheckBuyBuilding( itr ) ) )
            return ALLOW_BUILD;

    for ( u32 itr = 0x00000001; itr; itr <<= 1 )
        if ( ( rest & itr ) && ( LACK_RESOURCES == castle.CheckBuyBuilding( itr ) ) )
            return LACK_RESOURCES;

    for ( u32 itr = 0x00000001; itr; itr <<= 1 )
        if ( ( rest & itr ) && ( REQUIRES_BUILD == castle.CheckBuyBuilding( itr ) ) )
            return REQUIRES_BUILD;

    return UNKNOWN_COND;
}

bool Castle::AllowBuyBuilding( u32 build ) const
{
    return ALLOW_BUILD == CheckBuyBuilding( build );
}

/* buy building */
bool Castle::BuyBuilding( u32 build )
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
        Maps::ClearFog( GetIndex(), Game::GetViewDistance( Game::VIEW_CASTLE ), GetColor() );
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

    DEBUG( DBG_GAME, DBG_INFO, name << " build " << GetStringBuilding( build, race ) );
    return true;
}

/* draw image castle to position */
void Castle::DrawImageCastle( const Point & pt ) const
{
    fheroes2::Display & display = fheroes2::Display::instance();
    const Maps::Tiles & tile = world.GetTiles( GetIndex() );

    u32 index = 0;
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

    for ( u32 ii = 0; ii < 5; ++ii ) {
        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::OBJNTWBA, index + ii );
        dst_pt.x = pt.x + ii * 32 + sprite.x();
        dst_pt.y = pt.y + 3 * 32 + sprite.y();
        fheroes2::Blit( sprite, display, dst_pt.x, dst_pt.y );
    }

    for ( u32 ii = 0; ii < 5; ++ii ) {
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
    for ( u32 ii = 0; ii < 5; ++ii ) {
        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::OBJNTOWN, index + 1 + ii );
        dst_pt.x = pt.x + ii * 32 + sprite.x();
        dst_pt.y = pt.y + 32 + sprite.y();
        fheroes2::Blit( sprite, display, dst_pt.x, dst_pt.y );
    }
    for ( u32 ii = 0; ii < 5; ++ii ) {
        const fheroes2::Sprite & sprite = fheroes2::AGG::GetICN( ICN::OBJNTOWN, index + 6 + ii );
        dst_pt.x = pt.x + ii * 32 + sprite.x();
        dst_pt.y = pt.y + 2 * 32 + sprite.y();
        fheroes2::Blit( sprite, display, dst_pt.x, dst_pt.y );
    }
    for ( u32 ii = 0; ii < 5; ++ii ) {
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

    DEBUG( DBG_GAME, DBG_WARN, "return unknown" );
    return ICN::UNKNOWN;
}

/* get building name ICN */
int Castle::GetICNBuilding( u32 build, int race )
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
            return ICN::TWNKWEL2;
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
            return ICN::TWNNCAPT;
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

    DEBUG( DBG_GAME, DBG_WARN,
           "return unknown"
               << ", race: " << Race::String( race ) << ", build: " << Castle::GetStringBuilding( build, race ) << ", " << build );

    return ICN::UNKNOWN;
}

CastleHeroes Castle::GetHeroes( void ) const
{
    return world.GetHeroes( *this );
}

bool Castle::HaveNearlySea( void ) const
{
    // check nearest ocean
    if ( Maps::isValidAbsPoint( center.x, center.y + 2 ) ) {
        const s32 index = Maps::GetIndexFromAbsPoint( center.x, center.y + 2 );
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

bool Castle::PresentBoat( void ) const
{
    // 2 cell down
    if ( Maps::isValidAbsPoint( center.x, center.y + 2 ) ) {
        const s32 index = Maps::GetIndexFromAbsPoint( center.x, center.y + 2 );
        const s32 max = world.w() * world.h();

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

u32 Castle::GetActualDwelling( u32 build ) const
{
    switch ( build ) {
    case DWELLING_MONSTER1:
    case DWELLING_UPGRADE2:
    case DWELLING_UPGRADE3:
    case DWELLING_UPGRADE4:
    case DWELLING_UPGRADE5:
    case DWELLING_UPGRADE7:
        return build;
    case DWELLING_MONSTER2:
        return building & DWELLING_UPGRADE2 ? DWELLING_UPGRADE2 : build;
    case DWELLING_MONSTER3:
        return building & DWELLING_UPGRADE3 ? DWELLING_UPGRADE3 : build;
    case DWELLING_MONSTER4:
        return building & DWELLING_UPGRADE4 ? DWELLING_UPGRADE4 : build;
    case DWELLING_MONSTER5:
        return building & DWELLING_UPGRADE5 ? DWELLING_UPGRADE5 : build;
    case DWELLING_MONSTER6:
        return building & DWELLING_UPGRADE7 ? DWELLING_UPGRADE7 : ( building & DWELLING_UPGRADE6 ? DWELLING_UPGRADE6 : build );
    case DWELLING_UPGRADE6:
        return building & DWELLING_UPGRADE7 ? DWELLING_UPGRADE7 : build;
    default:
        break;
    }

    return BUILD_NOTHING;
}

u32 Castle::GetUpgradeBuilding( u32 build ) const
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

bool Castle::PredicateIsCapital( const Castle * castle )
{
    return castle && castle->Modes( CAPITAL );
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

std::string Castle::String( void ) const
{
    std::ostringstream os;
    const CastleHeroes heroes = GetHeroes();
    const Heroes * hero = NULL;

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

    if ( NULL != ( hero = heroes.Guard() ) ) {
        os << "army guard      : " << hero->GetArmy().String() << std::endl;
    }

    if ( NULL != ( hero = heroes.Guest() ) ) {
        os << "army guest      : " << hero->GetArmy().String() << std::endl;
    }

    return os.str();
}

int Castle::GetAttackModificator( std::string * ) const
{
    return 0;
}

int Castle::GetDefenseModificator( std::string * ) const
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

int Castle::GetKnowledgeModificator( std::string * ) const
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

const Army & Castle::GetArmy( void ) const
{
    const CastleHeroes heroes = world.GetHeroes( *this );
    return heroes.Guard() ? heroes.Guard()->GetArmy() : army;
}

Army & Castle::GetArmy( void )
{
    CastleHeroes heroes = world.GetHeroes( *this );
    return heroes.Guard() ? heroes.Guard()->GetArmy() : army;
}

const Army & Castle::GetActualArmy( void ) const
{
    CastleHeroes heroes = world.GetHeroes( *this );
    const Heroes * hero = heroes.GuardFirst();
    return hero ? hero->GetArmy() : army;
}

Army & Castle::GetActualArmy( void )
{
    CastleHeroes heroes = world.GetHeroes( *this );
    Heroes * hero = heroes.GuardFirst();
    return hero ? hero->GetArmy() : army;
}

bool Castle::AllowBuyBoat( void ) const
{
    // check payment and present other boat
    return ( HaveNearlySea() && isBuild( BUILD_SHIPYARD ) && GetKingdom().AllowPayment( PaymentConditions::BuyBoat() ) && !PresentBoat() );
}

bool Castle::BuyBoat( void )
{
    if ( !AllowBuyBoat() )
        return false;
    if ( isControlHuman() )
        AGG::PlaySound( M82::BUILDTWN );

    if ( !Maps::isValidAbsPoint( center.x, center.y + 2 ) )
        return false;

    const s32 index = Maps::GetIndexFromAbsPoint( center.x, center.y + 2 );
    Maps::Tiles & left = world.GetTiles( index - 1 );
    Maps::Tiles & right = world.GetTiles( index + 1 );
    Maps::Tiles & middle = world.GetTiles( index );
    Kingdom & kingdom = GetKingdom();

    if ( MP2::OBJ_ZERO == left.GetObject() && left.isWater() ) {
        kingdom.OddFundsResource( PaymentConditions::BuyBoat() );

        left.setBoat( Direction::RIGHT );
    }
    else if ( MP2::OBJ_ZERO == right.GetObject() && right.isWater() ) {
        kingdom.OddFundsResource( PaymentConditions::BuyBoat() );

        right.setBoat( Direction::RIGHT );
    }
    else if ( MP2::OBJ_ZERO == middle.GetObject() && middle.isWater() ) {
        kingdom.OddFundsResource( PaymentConditions::BuyBoat() );

        middle.setBoat( Direction::RIGHT );
    }

    return true;
}

int Castle::GetRace( void ) const
{
    return race;
}

const std::string & Castle::GetName( void ) const
{
    return name;
}

int Castle::GetControl( void ) const
{
    /* gray towns: ai control */
    return GetColor() & Color::ALL ? GetKingdom().GetControl() : CONTROL_AI;
}

bool Castle::AllowBuild( void ) const
{
    return Modes( ALLOWBUILD );
}

bool Castle::isBuild( u32 bd ) const
{
    return ( building & bd ) != 0;
}

bool Castle::isNecromancyShrineBuild( void ) const
{
    return race == Race::NECR && ( BUILD_SHRINE & building );
}

u32 Castle::GetGrownWell( void )
{
    return GameStatic::GetCastleGrownWell();
}

u32 Castle::GetGrownWel2( void )
{
    return GameStatic::GetCastleGrownWel2();
}

u32 Castle::GetGrownWeekOf( const Monster & mons )
{
    return Settings::Get().ExtWorldNewVersionWeekOf() ? mons.GetGrown() : GameStatic::GetCastleGrownWeekOf();
}

u32 Castle::GetGrownMonthOf( void )
{
    return GameStatic::GetCastleGrownMonthOf();
}

void Castle::Scoute( void ) const
{
    Maps::ClearFog( GetIndex(), Game::GetViewDistance( isCastle() ? Game::VIEW_CASTLE : Game::VIEW_TOWN ), GetColor() );
}

void Castle::JoinRNDArmy( void )
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

    army.JoinTroop( Monster( race, dwellingType ), count );
}

void Castle::ActionPreBattle( void )
{
    CastleHeroes heroes = world.GetHeroes( *this );
    Heroes * hero = heroes.GuardFirst();
    if ( hero && army.isValid() )
        hero->GetArmy().JoinStrongestFromArmy( army );

    if ( isControlAI() )
        AI::Get().CastlePreBattle( *this );
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

Castle * VecCastles::Get( const Point & position ) const
{
    const_iterator it = std::find_if( begin(), end(), [&position]( const Castle * castle ) { return castle->isPosition( position ); } );
    return end() != it ? *it : NULL;
}

Castle * VecCastles::GetFirstCastle( void ) const
{
    const_iterator it = std::find_if( begin(), end(), []( const Castle * castle ) { return castle->isCastle(); } );
    return end() != it ? *it : NULL;
}

void VecCastles::SortByBuildingValue()
{
    std::sort( begin(), end(), []( const Castle * left, const Castle * right ) {
        if ( left && right )
            return left->getBuildingValue() > right->getBuildingValue();
        return right == NULL;
    } );
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
    reserve( MAXCASTLES );
}

AllCastles::~AllCastles()
{
    AllCastles::clear();
}

void AllCastles::Init( void )
{
    if ( size() )
        AllCastles::clear();
}

void AllCastles::clear( void )
{
    for ( iterator it = begin(); it != end(); ++it )
        delete *it;
    std::vector<Castle *>::clear();
}

void AllCastles::Scoute( int colors ) const
{
    for ( const_iterator it = begin(); it != end(); ++it )
        if ( colors & ( *it )->GetColor() )
            ( *it )->Scoute();
}

/* pack castle */
StreamBase & operator<<( StreamBase & msg, const Castle & castle )
{
    const ColorBase & color = castle;

    msg << static_cast<const MapPosition &>( castle ) << castle.modes << castle.race << castle.building << castle.captain << color << castle.name << castle.mageguild
        << static_cast<u32>( CASTLEMAXMONSTER );

    for ( u32 ii = 0; ii < CASTLEMAXMONSTER; ++ii )
        msg << castle.dwelling[ii];

    return msg << castle.army;
}

/* unpack castle */
StreamBase & operator>>( StreamBase & msg, Castle & castle )
{
    ColorBase & color = castle;
    u32 dwellingcount;

    msg >> static_cast<MapPosition &>( castle ) >> castle.modes >> castle.race >> castle.building >> castle.captain >> color >> castle.name >> castle.mageguild;

    msg >> dwellingcount;
    for ( u32 ii = 0; ii < dwellingcount; ++ii )
        msg >> castle.dwelling[ii];

    msg >> castle.army;
    castle.army.SetCommander( &castle.captain );

    return msg;
}

StreamBase & operator<<( StreamBase & msg, const VecCastles & castles )
{
    msg << static_cast<u32>( castles.size() );

    for ( AllCastles::const_iterator it = castles.begin(); it != castles.end(); ++it )
        msg << ( *it ? ( *it )->GetIndex() : static_cast<s32>( -1 ) );

    return msg;
}

StreamBase & operator>>( StreamBase & msg, VecCastles & castles )
{
    s32 index;
    u32 size;
    msg >> size;

    castles.resize( size, NULL );

    for ( AllCastles::iterator it = castles.begin(); it != castles.end(); ++it ) {
        msg >> index;
        *it = ( index < 0 ? NULL : world.GetCastle( Maps::GetPoint( index ) ) );
    }

    return msg;
}

StreamBase & operator<<( StreamBase & msg, const AllCastles & castles )
{
    msg << static_cast<u32>( castles.size() );

    for ( AllCastles::const_iterator it = castles.begin(); it != castles.end(); ++it )
        msg << **it;

    return msg;
}

StreamBase & operator>>( StreamBase & msg, AllCastles & castles )
{
    u32 size;
    msg >> size;

    castles.clear();
    castles.resize( size, NULL );

    for ( AllCastles::iterator it = castles.begin(); it != castles.end(); ++it ) {
        *it = new Castle();
        msg >> **it;
    }

    return msg;
}

void Castle::SwapCastleHeroes( CastleHeroes & heroes )
{
    if ( heroes.Guest() && heroes.Guard() ) {
        heroes.Guest()->SetModes( Heroes::GUARDIAN );
        heroes.Guest()->ResetModes( Heroes::SLEEPER );
        heroes.Guard()->ResetModes( Heroes::GUARDIAN );
        heroes.Swap();

        world.GetTiles( center.x, center.y ).SetHeroes( NULL );

        Point position( heroes.Guard()->GetCenter() );
        position.y -= 1;
        heroes.Guard()->SetCenter( position );
        heroes.Guard()->GetPath().Reset();

        position = heroes.Guest()->GetCenter();
        position.y += 1;
        heroes.Guest()->SetCenter( position );
        heroes.Guest()->GetPath().Reset();

        world.GetTiles( center.x, center.y ).SetHeroes( heroes.Guest() );
    }
    else if ( heroes.Guest() && !heroes.Guard() ) {
        heroes.Guest()->SetModes( Heroes::GUARDIAN );
        heroes.Guest()->ResetModes( Heroes::SLEEPER );
        heroes.Swap();
        heroes.Guard()->GetArmy().JoinTroops( army );

        world.GetTiles( center.x, center.y ).SetHeroes( NULL );

        Point position( heroes.Guard()->GetCenter() );
        position.y -= 1;
        heroes.Guard()->SetCenter( position );
        heroes.Guard()->GetPath().Reset();
    }
    else if ( !heroes.Guest() && heroes.Guard() ) {
        heroes.Guard()->ResetModes( Heroes::GUARDIAN );
        heroes.Swap();

        Point position( heroes.Guest()->GetCenter() );
        position.y += 1;
        heroes.Guest()->SetCenter( position );
        heroes.Guest()->GetPath().Reset();

        world.GetTiles( center.x, center.y ).SetHeroes( heroes.Guest() );
    }
}

std::string Castle::GetStringBuilding( u32 build ) const
{
    return GetStringBuilding( build, GetRace() );
}

std::string Castle::GetDescriptionBuilding( u32 build ) const
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
    } break;

    case BUILD_SPEC:
    case BUILD_STATUE: {
        payment_t profit = ProfitConditions::FromBuilding( build, GetRace() );
        StringReplace( res, "%{count}", profit.gold );
    } break;

    default:
        break;
    }

    return res;
}
