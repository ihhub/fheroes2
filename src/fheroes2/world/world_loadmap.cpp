/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2022                                             *
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
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <ostream>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "artifact.h"
#include "artifact_ultimate.h"
#include "campaign_savedata.h"
#include "campaign_scenariodata.h"
#include "castle.h"
#include "color.h"
#include "game_over.h"
#include "heroes.h"
#include "icn.h"
#include "kingdom.h"
#include "logging.h"
#include "maps.h"
#include "maps_objects.h"
#include "maps_tiles.h"
#include "math_base.h"
#include "mp2.h"
#include "mp2_helper.h"
#include "players.h"
#include "race.h"
#include "rand.h"
#include "serialize.h"
#include "settings.h"
#include "world.h"

namespace
{
    const int32_t ultimateArtifactOffset = 9;

    Artifact getUltimateArtifact()
    {
        if ( Settings::Get().isCampaignGameType() ) {
            const Campaign::ScenarioVictoryCondition victoryCondition = Campaign::getCurrentScenarioVictoryCondition();
            if ( victoryCondition == Campaign::ScenarioVictoryCondition::OBTAIN_ULTIMATE_CROWN ) {
                return Artifact::ULTIMATE_CROWN;
            }
            else if ( victoryCondition == Campaign::ScenarioVictoryCondition::OBTAIN_SPHERE_NEGATION ) {
                return Artifact::SPHERE_NEGATION;
            }
        }

        return Artifact::Rand( Artifact::ART_ULTIMATE );
    }

    void fixCastleNames( const AllCastles & castles )
    {
        // Find castles with no names.
        std::vector<Castle *> castleWithNoName;
        std::set<std::string> castleNames;

        for ( Castle * castle : castles ) {
            if ( castle == nullptr ) {
                // How do we have an empty pointer in this container?
                assert( 0 );
                continue;
            }

            const std::string & name = castle->GetName();

            if ( name.empty() ) {
                castleWithNoName.emplace_back( castle );
            }
            else {
                castleNames.emplace( name );
            }
        }

        if ( castleWithNoName.empty() ) {
            return;
        }

        for ( Castle * castle : castleWithNoName ) {
            castle->setName( castleNames );
            castleNames.emplace( castle->GetName() );
        }
    }
}

namespace GameStatic
{
    extern uint32_t uniq;
}

bool World::LoadMapMP2( const std::string & filename )
{
    Reset();
    Defaults();

    StreamFile fs;
    if ( !fs.open( filename, "rb" ) ) {
        DEBUG_LOG( DBG_GAME | DBG_ENGINE, DBG_WARN, "file not found " << filename.c_str() )
        return false;
    }

    // check (mp2, mx2) ID
    if ( fs.getBE32() != 0x5C000000 )
        return false;

    // endof
    const size_t endof_mp2 = fs.size();
    fs.seek( endof_mp2 - 4 );

    // read uniq
    GameStatic::uniq = fs.getLE32();

    // offset data
    fs.seek( MP2::MP2OFFSETDATA - 2 * 4 );

    // width
    switch ( fs.getLE32() ) {
    case Maps::SMALL:
        width = Maps::SMALL;
        break;
    case Maps::MEDIUM:
        width = Maps::MEDIUM;
        break;
    case Maps::LARGE:
        width = Maps::LARGE;
        break;
    case Maps::XLARGE:
        width = Maps::XLARGE;
        break;
    default:
        width = 0;
        break;
    }

    // height
    switch ( fs.getLE32() ) {
    case Maps::SMALL:
        height = Maps::SMALL;
        break;
    case Maps::MEDIUM:
        height = Maps::MEDIUM;
        break;
    case Maps::LARGE:
        height = Maps::LARGE;
        break;
    case Maps::XLARGE:
        height = Maps::XLARGE;
        break;
    default:
        height = 0;
        break;
    }

    if ( width == 0 || height == 0 || width != height ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, "incorrect map size" )
        return false;
    }

    const int32_t worldSize = width * height;

    // seek to ADDONS block
    fs.skip( worldSize * MP2::SIZEOFMP2TILE );

    // read all addons
    std::vector<MP2::mp2addon_t> vec_mp2addons( fs.getLE32() /* count mp2addon_t */ );

    for ( MP2::mp2addon_t & mp2addon : vec_mp2addons ) {
        MP2::loadAddon( fs, mp2addon );
    }

    const size_t endof_addons = fs.tell();
    DEBUG_LOG( DBG_GAME, DBG_INFO, "read all tiles addons, tellg: " << endof_addons )

    // offset data
    fs.seek( MP2::MP2OFFSETDATA );

    vec_tiles.resize( worldSize );

    // In the future we need to check 3 things which could point that this map is The Price of Loyalty version:
    // - new object types
    // - new artifact types on map
    // - new artifact types in hero's bag

    MapsIndexes vec_object; // index maps for OBJ_CASTLE, OBJ_HEROES, OBJ_SIGN, OBJ_BOTTLE, OBJ_EVENT
    vec_object.reserve( 100 );

    // read all tiles
    for ( int32_t i = 0; i < worldSize; ++i ) {
        Maps::Tiles & tile = vec_tiles[i];

        MP2::mp2tile_t mp2tile;
        MP2::loadTile( fs, mp2tile );

        tile.Init( i, mp2tile );

        // Read extra information if it's present.
        size_t addonIndex = mp2tile.nextAddonIndex;
        while ( addonIndex > 0 ) {
            if ( vec_mp2addons.size() <= addonIndex ) {
                DEBUG_LOG( DBG_GAME, DBG_WARN, "index out of range" )
                break;
            }
            tile.AddonsPushLevel1( vec_mp2addons[addonIndex] );
            tile.AddonsPushLevel2( vec_mp2addons[addonIndex] );
            addonIndex = vec_mp2addons[addonIndex].nextAddonIndex;
        }

        tile.AddonsSort();

        switch ( mp2tile.mapObjectType ) {
        case MP2::OBJ_RNDTOWN:
        case MP2::OBJ_RNDCASTLE:
        case MP2::OBJ_CASTLE:
        case MP2::OBJ_HEROES:
        case MP2::OBJ_SIGN:
        case MP2::OBJ_BOTTLE:
        case MP2::OBJ_EVENT:
        case MP2::OBJ_SPHINX:
        case MP2::OBJ_JAIL:
            vec_object.push_back( i );
            break;
        default:
            break;
        }
    }

    DEBUG_LOG( DBG_GAME, DBG_INFO, "read all tiles, tellg: " << fs.tell() )

    // after addons
    fs.seek( endof_addons );

    // cood castles
    // 72 x 3 byte (cx, cy, id)
    for ( int32_t i = 0; i < 72; ++i ) {
        const uint32_t cx = fs.get();
        const uint32_t cy = fs.get();
        const uint32_t id = fs.get();

        // empty block
        if ( 0xFF == cx && 0xFF == cy )
            continue;

        switch ( id ) {
        case 0x00: // tower: knight
        case 0x80: // castle: knight
            vec_castles.AddCastle( new Castle( cx, cy, Race::KNGT ) );
            break;

        case 0x01: // tower: barbarian
        case 0x81: // castle: barbarian
            vec_castles.AddCastle( new Castle( cx, cy, Race::BARB ) );
            break;

        case 0x02: // tower: sorceress
        case 0x82: // castle: sorceress
            vec_castles.AddCastle( new Castle( cx, cy, Race::SORC ) );
            break;

        case 0x03: // tower: warlock
        case 0x83: // castle: warlock
            vec_castles.AddCastle( new Castle( cx, cy, Race::WRLK ) );
            break;

        case 0x04: // tower: wizard
        case 0x84: // castle: wizard
            vec_castles.AddCastle( new Castle( cx, cy, Race::WZRD ) );
            break;

        case 0x05: // tower: necromancer
        case 0x85: // castle: necromancer
            vec_castles.AddCastle( new Castle( cx, cy, Race::NECR ) );
            break;

        case 0x06: // tower: random
        case 0x86: // castle: random
            vec_castles.AddCastle( new Castle( cx, cy, Race::NONE ) );
            break;

        default:
            DEBUG_LOG( DBG_GAME, DBG_WARN,
                       "castle block: "
                           << "unknown id: " << id << ", maps index: " << cx + cy * width )
            break;
        }
        // preload in to capture objects cache
        map_captureobj.Set( Maps::GetIndexFromAbsPoint( cx, cy ), MP2::OBJ_CASTLE, Color::NONE );
    }

    DEBUG_LOG( DBG_GAME, DBG_INFO, "read coord castles, tellg: " << fs.tell() )
    fs.seek( endof_addons + ( 72 * 3 ) );

    // cood resource kingdoms
    // 144 x 3 byte (cx, cy, id)
    for ( int32_t i = 0; i < 144; ++i ) {
        const uint32_t cx = fs.get();
        const uint32_t cy = fs.get();
        const uint32_t id = fs.get();

        // empty block
        if ( 0xFF == cx && 0xFF == cy )
            continue;

        switch ( id ) {
        // mines: wood
        case 0x00:
            map_captureobj.Set( Maps::GetIndexFromAbsPoint( cx, cy ), MP2::OBJ_SAWMILL, Color::NONE );
            break;
        // mines: mercury
        case 0x01:
            map_captureobj.Set( Maps::GetIndexFromAbsPoint( cx, cy ), MP2::OBJ_ALCHEMYLAB, Color::NONE );
            break;
        // mines: ore
        case 0x02:
        // mines: sulfur
        case 0x03:
        // mines: crystal
        case 0x04:
        // mines: gems
        case 0x05:
        // mines: gold
        case 0x06:
            map_captureobj.Set( Maps::GetIndexFromAbsPoint( cx, cy ), MP2::OBJ_MINES, Color::NONE );
            break;
        // lighthouse
        case 0x64:
            map_captureobj.Set( Maps::GetIndexFromAbsPoint( cx, cy ), MP2::OBJ_LIGHTHOUSE, Color::NONE );
            break;
        // dragon city
        case 0x65:
            map_captureobj.Set( Maps::GetIndexFromAbsPoint( cx, cy ), MP2::OBJ_DRAGONCITY, Color::NONE );
            break;
        // abandoned mines
        case 0x67:
            map_captureobj.Set( Maps::GetIndexFromAbsPoint( cx, cy ), MP2::OBJ_ABANDONEDMINE, Color::NONE );
            break;
        default:
            DEBUG_LOG( DBG_GAME, DBG_WARN,
                       "kingdom block: "
                           << "unknown id: " << id << ", maps index: " << cx + cy * width )
            break;
        }
    }

    DEBUG_LOG( DBG_GAME, DBG_INFO, "read coord other resource, tellg: " << fs.tell() )
    fs.seek( endof_addons + ( 72 * 3 ) + ( 144 * 3 ) );

    // byte: num obelisks (01 default)
    fs.skip( 1 );

    // count final mp2 blocks
    uint32_t countblock = 0;
    while ( true ) {
        const uint32_t l = fs.get();
        const uint32_t h = fs.get();

        if ( 0 == h && 0 == l )
            break;
        else {
            countblock = 256 * h + l - 1;
        }
    }

    // castle or heroes or (events, rumors, etc)
    for ( uint32_t ii = 0; ii < countblock; ++ii ) {
        int32_t findobject = -1;

        // read block
        size_t sizeblock = fs.getLE16();
        std::vector<uint8_t> pblock = fs.getRaw( sizeblock );

        for ( MapsIndexes::const_iterator it_index = vec_object.begin(); it_index != vec_object.end() && findobject < 0; ++it_index ) {
            const Maps::Tiles & tile = vec_tiles[*it_index];

            // orders(quantity2, quantity1)
            uint32_t orders = ( tile.GetQuantity2() ? tile.GetQuantity2() : 0 );
            orders <<= 8;
            orders |= tile.GetQuantity1();

            if ( orders && !( orders % 0x08 ) && ( ii + 1 == orders / 0x08 ) )
                findobject = *it_index;
        }

        if ( 0 <= findobject ) {
            const Maps::Tiles & tile = vec_tiles[findobject];

            switch ( tile.GetObject() ) {
            case MP2::OBJ_CASTLE:
                // add castle
                if ( MP2::SIZEOFMP2CASTLE != pblock.size() ) {
                    DEBUG_LOG( DBG_GAME, DBG_WARN,
                               "read castle: "
                                   << "incorrect size block: " << pblock.size() )
                }
                else {
                    Castle * castle = getCastleEntrance( Maps::GetPoint( findobject ) );
                    if ( castle ) {
                        castle->LoadFromMP2( pblock );
                        map_captureobj.SetColor( tile.GetIndex(), castle->GetColor() );
                    }
                    else {
                        DEBUG_LOG( DBG_GAME, DBG_WARN,
                                   "load castle: "
                                       << "not found, index: " << findobject )
                    }
                }
                break;
            case MP2::OBJ_RNDTOWN:
            case MP2::OBJ_RNDCASTLE:
                // add rnd castle
                if ( MP2::SIZEOFMP2CASTLE != pblock.size() ) {
                    DEBUG_LOG( DBG_GAME, DBG_WARN,
                               "read castle: "
                                   << "incorrect size block: " << pblock.size() )
                }
                else {
                    // Random castle's entrance tile is marked as OBJ_RNDCASTLE or OBJ_RNDTOWN instead of OBJ_CASTLE.
                    Castle * castle = getCastle( Maps::GetPoint( findobject ) );
                    if ( castle ) {
                        castle->LoadFromMP2( pblock );
                        Maps::UpdateCastleSprite( castle->GetCenter(), castle->GetRace(), castle->isCastle(), true );
                        Maps::ReplaceRandomCastleObjectId( castle->GetCenter() );
                        map_captureobj.SetColor( tile.GetIndex(), castle->GetColor() );
                    }
                    else {
                        DEBUG_LOG( DBG_GAME, DBG_WARN,
                                   "load castle: "
                                       << "not found, index: " << findobject )
                    }
                }
                break;
            case MP2::OBJ_JAIL:
                // add jail
                if ( MP2::SIZEOFMP2HEROES != pblock.size() ) {
                    DEBUG_LOG( DBG_GAME, DBG_WARN,
                               "read heroes: "
                                   << "incorrect size block: " << pblock.size() )
                }
                else {
                    int race = Race::KNGT;
                    switch ( pblock[0x3c] ) {
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
                        break;
                    }

                    Heroes * hero = GetFreemanHeroes( race );

                    if ( hero ) {
                        hero->LoadFromMP2( findobject, Color::NONE, hero->GetRace(), StreamBuf( pblock ) );
                        hero->SetModes( Heroes::JAIL );
                    }
                }
                break;
            case MP2::OBJ_HEROES:
                // add heroes
                if ( MP2::SIZEOFMP2HEROES != pblock.size() ) {
                    DEBUG_LOG( DBG_GAME, DBG_WARN,
                               "read heroes: "
                                   << "incorrect size block: " << pblock.size() )
                }
                else {
                    std::pair<int, int> colorRace = Maps::Tiles::ColorRaceFromHeroSprite( tile.GetObjectSpriteIndex() );
                    const Kingdom & kingdom = GetKingdom( colorRace.first );

                    if ( colorRace.second == Race::RAND && colorRace.first != Color::NONE )
                        colorRace.second = kingdom.GetRace();

                    // check heroes max count
                    if ( kingdom.AllowRecruitHero( false ) ) {
                        Heroes * hero = nullptr;

                        if ( pblock[17] && pblock[18] < Heroes::BAX )
                            hero = vec_heroes.Get( pblock[18] );

                        if ( !hero || !hero->isFreeman() )
                            hero = GetFreemanHeroes( colorRace.second );

                        if ( hero )
                            hero->LoadFromMP2( findobject, colorRace.first, colorRace.second, StreamBuf( pblock ) );
                    }
                    else {
                        DEBUG_LOG( DBG_GAME, DBG_WARN, "load heroes maximum" )
                    }
                }
                break;
            case MP2::OBJ_SIGN:
            case MP2::OBJ_BOTTLE:
                // add sign or buttle
                if ( MP2::SIZEOFMP2SIGN - 1 < pblock.size() && 0x01 == pblock[0] ) {
                    MapSign * obj = new MapSign();
                    obj->LoadFromMP2( findobject, StreamBuf( pblock ) );
                    map_objects.add( obj );
                }
                break;
            case MP2::OBJ_EVENT:
                // add event maps
                if ( MP2::SIZEOFMP2EVENT - 1 < pblock.size() && 0x01 == pblock[0] ) {
                    MapEvent * obj = new MapEvent();
                    obj->LoadFromMP2( findobject, StreamBuf( pblock ) );
                    map_objects.add( obj );
                }
                break;
            case MP2::OBJ_SPHINX:
                // add riddle sphinx
                if ( MP2::SIZEOFMP2RIDDLE - 1 < pblock.size() && 0x00 == pblock[0] ) {
                    MapSphinx * obj = new MapSphinx();
                    obj->LoadFromMP2( findobject, StreamBuf( pblock ) );
                    map_objects.add( obj );
                }
                break;
            default:
                break;
            }
        }
        // other events
        else if ( 0x00 == pblock[0] ) {
            // add event day
            if ( MP2::SIZEOFMP2EVENT - 1 < pblock.size() && 1 == pblock[42] ) {
                vec_eventsday.emplace_back();
                vec_eventsday.back().LoadFromMP2( StreamBuf( pblock ) );
            }
            // add rumors
            else if ( MP2::SIZEOFMP2RUMOR - 1 < pblock.size() ) {
                if ( pblock[8] ) {
                    _rumors.emplace_back( StreamBuf( &pblock[8], pblock.size() - 8 ).toString() );
                    DEBUG_LOG( DBG_GAME, DBG_INFO, "add rumors: " << _rumors.back() )
                }
            }
        }
        // debug
        else {
            DEBUG_LOG( DBG_GAME, DBG_WARN, "read maps: unknown block addons, size: " << pblock.size() )
        }
    }

    fixCastleNames( vec_castles );

    // clear artifact flags to correctly generate random artifacts
    fheroes2::ResetArtifactStats();

    const Settings & conf = Settings::Get();

    // do not let the player get a random artifact that allows him to win the game
    if ( ( conf.ConditionWins() & GameOver::WINS_ARTIFACT ) == GameOver::WINS_ARTIFACT && !conf.WinsFindUltimateArtifact() ) {
        const Artifact art = conf.WinsFindArtifactID();

        fheroes2::ExcludeArtifactFromRandom( art.GetID() );
    }

    ProcessNewMap();

    DEBUG_LOG( DBG_GAME, DBG_INFO, "end load" )
    return true;
}

void World::ProcessNewMap()
{
    // modify other objects
    for ( size_t i = 0; i < vec_tiles.size(); ++i ) {
        Maps::Tiles & tile = vec_tiles[i];
        Maps::Tiles::fixTileObjectType( tile );

        switch ( tile.GetObject() ) {
        case MP2::OBJ_WITCHSHUT:
        case MP2::OBJ_SHRINE1:
        case MP2::OBJ_SHRINE2:
        case MP2::OBJ_SHRINE3:
        case MP2::OBJ_STONELITHS:
        case MP2::OBJ_FOUNTAIN:
        case MP2::OBJ_EVENT:
        case MP2::OBJ_BOAT:
        case MP2::OBJ_RNDARTIFACT:
        case MP2::OBJ_RNDARTIFACT1:
        case MP2::OBJ_RNDARTIFACT2:
        case MP2::OBJ_RNDARTIFACT3:
        case MP2::OBJ_RNDRESOURCE:
        case MP2::OBJ_WATERCHEST:
        case MP2::OBJ_TREASURECHEST:
        case MP2::OBJ_ARTIFACT:
        case MP2::OBJ_RESOURCE:
        case MP2::OBJ_MAGICGARDEN:
        case MP2::OBJ_WATERWHEEL:
        case MP2::OBJ_WINDMILL:
        case MP2::OBJ_WAGON:
        case MP2::OBJ_SKELETON:
        case MP2::OBJ_LEANTO:
        case MP2::OBJ_CAMPFIRE:
        case MP2::OBJ_FLOTSAM:
        case MP2::OBJ_SHIPWRECKSURVIVOR:
        case MP2::OBJ_DERELICTSHIP:
        case MP2::OBJ_SHIPWRECK:
        case MP2::OBJ_GRAVEYARD:
        case MP2::OBJ_PYRAMID:
        case MP2::OBJ_DAEMONCAVE:
        case MP2::OBJ_ABANDONEDMINE:
        case MP2::OBJ_ALCHEMYLAB:
        case MP2::OBJ_SAWMILL:
        case MP2::OBJ_MINES:
        case MP2::OBJ_TREEKNOWLEDGE:
        case MP2::OBJ_BARRIER:
        case MP2::OBJ_TRAVELLERTENT:
        case MP2::OBJ_MONSTER:
        case MP2::OBJ_RNDMONSTER:
        case MP2::OBJ_RNDMONSTER1:
        case MP2::OBJ_RNDMONSTER2:
        case MP2::OBJ_RNDMONSTER3:
        case MP2::OBJ_RNDMONSTER4:
        case MP2::OBJ_ANCIENTLAMP:
        case MP2::OBJ_WATCHTOWER:
        case MP2::OBJ_EXCAVATION:
        case MP2::OBJ_CAVE:
        case MP2::OBJ_TREEHOUSE:
        case MP2::OBJ_ARCHERHOUSE:
        case MP2::OBJ_GOBLINHUT:
        case MP2::OBJ_DWARFCOTT:
        case MP2::OBJ_HALFLINGHOLE:
        case MP2::OBJ_PEASANTHUT:
        case MP2::OBJ_THATCHEDHUT:
        case MP2::OBJ_RUINS:
        case MP2::OBJ_TREECITY:
        case MP2::OBJ_WAGONCAMP:
        case MP2::OBJ_DESERTTENT:
        case MP2::OBJ_TROLLBRIDGE:
        case MP2::OBJ_DRAGONCITY:
        case MP2::OBJ_CITYDEAD:
            tile.QuantityUpdate();
            break;

        case MP2::OBJ_WATERALTAR:
        case MP2::OBJ_AIRALTAR:
        case MP2::OBJ_FIREALTAR:
        case MP2::OBJ_EARTHALTAR:
        case MP2::OBJ_BARROWMOUNDS:
            tile.QuantityReset();
            tile.QuantityUpdate();
            break;

        case MP2::OBJ_HEROES: {
            // remove map editor sprite
            if ( MP2::GetICNObject( tile.GetObjectTileset() ) == ICN::MINIHERO )
                tile.Remove( tile.GetObjectUID() );

            tile.SetHeroes( GetHeroes( Maps::GetPoint( static_cast<int32_t>( i ) ) ) );
            break;
        }

        default:
            break;
        }
    }

    // add heroes to kingdoms
    vec_kingdoms.AddHeroes( vec_heroes );

    // add castles to kingdoms
    vec_kingdoms.AddCastles( vec_castles );

    const Settings & conf = Settings::Get();

    // update wins, loss conditions
    if ( GameOver::WINS_HERO & conf.ConditionWins() ) {
        const Heroes * hero = GetHeroes( conf.WinsMapsPositionObject() );
        heroes_cond_wins = hero ? hero->GetID() : Heroes::UNKNOWN;
    }
    if ( GameOver::LOSS_HERO & conf.ConditionLoss() ) {
        Heroes * hero = GetHeroes( conf.LossMapsPositionObject() );
        heroes_cond_loss = hero ? hero->GetID() : Heroes::UNKNOWN;

        if ( hero ) {
            hero->SetModes( Heroes::NOTDISMISS | Heroes::NOTDEFAULTS );
        }
    }

    // Search for a tile with a predefined Ultimate Artifact
    const MapsTiles::iterator ultArtTileIter
        = std::find_if( vec_tiles.begin(), vec_tiles.end(), []( const Maps::Tiles & tile ) { return tile.isObject( MP2::OBJ_RNDULTIMATEARTIFACT ); } );

    auto checkTileForSuitabilityForUltArt = [this]( const int32_t idx ) {
        const int32_t x = idx % width;
        if ( x < ultimateArtifactOffset || x >= width - ultimateArtifactOffset ) {
            return false;
        }

        const int32_t y = idx / width;
        if ( y < ultimateArtifactOffset || y >= height - ultimateArtifactOffset ) {
            return false;
        }

        return GetTiles( idx ).GoodForUltimateArtifact();
    };

    // There is no tile with a predefined Ultimate Artifact, pick a suitable tile randomly
    if ( ultArtTileIter == vec_tiles.end() ) {
        MapsIndexes pool;
        pool.reserve( vec_tiles.size() / 2 );

        for ( const Maps::Tiles & tile : vec_tiles ) {
            const int32_t idx = tile.GetIndex();

            if ( checkTileForSuitabilityForUltArt( idx ) ) {
                pool.push_back( idx );
            }
        }

        if ( !pool.empty() ) {
            const int32_t pos = Rand::Get( pool );

            ultimate_artifact.Set( pos, getUltimateArtifact() );

            DEBUG_LOG( DBG_GAME, DBG_INFO, "Ultimate Artifact index: " << pos )
        }
        else {
            DEBUG_LOG( DBG_GAME, DBG_WARN, "no suitable tile to place the Ultimate Artifact was found" )
        }
    }
    // There is a tile with a predefined Ultimate Artifact, pick a tile nearby in the radius specified in the artifact's properties
    else {
        static_assert( std::is_same_v<decltype( ultArtTileIter->GetQuantity1() ), uint8_t> && std::is_same_v<decltype( ultArtTileIter->GetQuantity2() ), uint8_t>,
                       "Types of tile's quantities have been changed, check the bitwise arithmetic below" );

        // The radius can be in the range 0 - 127, it is represented by 2 low-order bits of quantity2 and 5 high-order bits of quantity1
        const int32_t radius = ( ( ultArtTileIter->GetQuantity2() & 0x03 ) << 5 ) + ( ultArtTileIter->GetQuantity1() >> 3 );

        // Remove the predefined Ultimate Artifact object
        ultArtTileIter->Remove( ultArtTileIter->GetObjectUID() );
        ultArtTileIter->setAsEmpty();

        // Use the predefined Ultimate Artifact tile index as a fallback
        int32_t pos = ultArtTileIter->GetIndex();

        if ( radius > 0 ) {
            MapsIndexes pool = Maps::getAroundIndexes( pos, radius );

            // Maps::getAroundIndexes() results does not include the central index, so we have to append it manually
            assert( std::find( pool.begin(), pool.end(), pos ) == pool.end() );
            pool.push_back( pos );

            pool.erase( std::remove_if( pool.begin(), pool.end(),
                                        [&checkTileForSuitabilityForUltArt]( const int32_t idx ) { return !checkTileForSuitabilityForUltArt( idx ); } ),
                        pool.end() );

            if ( !pool.empty() ) {
                pos = Rand::Get( pool );
            }
        }

        ultimate_artifact.Set( pos, getUltimateArtifact() );

        DEBUG_LOG( DBG_GAME, DBG_INFO,
                   "predefined Ultimate Artifact index: " << ultArtTileIter->GetIndex() << ", radius: " << radius << ", Ultimate Artifact index: " << pos )
    }

    PostLoad( true );

    vec_kingdoms.ApplyPlayWithStartingHero();

    // If we are in developer mode, then add the DEBUG_HERO
    if ( IS_DEVEL() ) {
        Kingdom & kingdom = GetKingdom( Color::GetFirst( Players::HumanColors() ) );

        if ( !kingdom.GetCastles().empty() ) {
            const Castle * castle = kingdom.GetCastles().front();
            const fheroes2::Point & cp = castle->GetCenter();
            Heroes * hero = vec_heroes.Get( Heroes::DEBUG_HERO );

            if ( hero && !GetTiles( cp.x, cp.y + 1 ).GetHeroes() ) {
                hero->Recruit( castle->GetColor(), { cp.x, cp.y + 1 } );
            }
        }
    }
}
