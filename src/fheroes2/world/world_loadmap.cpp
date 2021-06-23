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

#include "artifact.h"
#include "campaign_data.h"
#include "campaign_savedata.h"
#include "castle.h"
#include "game.h"
#include "game_over.h"
#include "heroes.h"
#include "icn.h"
#include "kingdom.h"
#include "logging.h"
#include "maps_tiles.h"
#include "mp2.h"
#include "race.h"
#include "rand.h"
#include "settings.h"
#include "tools.h"
#include "translations.h"
#include "world.h"
#include "zzlib.h"

namespace
{
    const int32_t ultimateArtifactOffset = 9;
}

namespace GameStatic
{
    extern u32 uniq;
}

std::vector<u8> DecodeBase64AndUncomress( const std::string & base64 )
{
    std::vector<u8> zdata = decodeBase64( base64 );
    StreamBuf sb( zdata );
    sb.skip( 4 ); // editor: version
    u32 realsz = sb.getLE32();
    sb.skip( 4 ); // qt uncompress size
    return zlibDecompress( sb.data(), sb.size(), realsz + 1 );
}

bool World::LoadMapMP2( const std::string & filename )
{
    Reset();
    Defaults();

    StreamFile fs;
    if ( !fs.open( filename, "rb" ) ) {
        DEBUG_LOG( DBG_GAME | DBG_ENGINE, DBG_WARN, "file not found " << filename.c_str() );
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
    fs.seek( MP2OFFSETDATA - 2 * 4 );

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
        DEBUG_LOG( DBG_GAME, DBG_WARN, "incrrect maps size" );
        return false;
    }

    const int32_t worldSize = width * height;

    // seek to ADDONS block
    fs.skip( worldSize * SIZEOFMP2TILE );

    // read all addons
    std::vector<MP2::mp2addon_t> vec_mp2addons( fs.getLE32() /* count mp2addon_t */ );

    for ( MP2::mp2addon_t & mp2addon : vec_mp2addons ) {
        mp2addon.nextAddonIndex = fs.getLE16();
        mp2addon.objectNameN1 = fs.get() * 2;
        mp2addon.indexNameN1 = fs.get();
        mp2addon.quantityN = fs.get();
        mp2addon.objectNameN2 = fs.get();
        mp2addon.indexNameN2 = fs.get();

        mp2addon.level1ObjectUID = fs.getLE32();
        mp2addon.level2ObjectUID = fs.getLE32();
    }

    const size_t endof_addons = fs.tell();
    DEBUG_LOG( DBG_GAME, DBG_INFO, "read all tiles addons, tellg: " << endof_addons );

    // offset data
    fs.seek( MP2OFFSETDATA );

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

        mp2tile.tileIndex = fs.getLE16();
        mp2tile.objectName1 = fs.get();
        mp2tile.indexName1 = fs.get();
        mp2tile.quantity1 = fs.get();
        mp2tile.quantity2 = fs.get();
        mp2tile.objectName2 = fs.get();
        mp2tile.indexName2 = fs.get();
        mp2tile.flags = fs.get();
        mp2tile.mapObject = fs.get();
        mp2tile.nextAddonIndex = fs.getLE16();
        mp2tile.level1ObjectUID = fs.getLE32();
        mp2tile.level2ObjectUID = fs.getLE32();

        tile.Init( i, mp2tile );

        // Read extra information if it's present.
        size_t addonIndex = mp2tile.nextAddonIndex;
        while ( addonIndex > 0 ) {
            if ( vec_mp2addons.size() <= addonIndex ) {
                DEBUG_LOG( DBG_GAME, DBG_WARN, "index out of range" );
                break;
            }
            tile.AddonsPushLevel1( vec_mp2addons[addonIndex] );
            tile.AddonsPushLevel2( vec_mp2addons[addonIndex] );
            addonIndex = vec_mp2addons[addonIndex].nextAddonIndex;
        }

        tile.AddonsSort();

        switch ( mp2tile.mapObject ) {
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

    DEBUG_LOG( DBG_GAME, DBG_INFO, "read all tiles, tellg: " << fs.tell() );

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
                           << "unknown id: " << id << ", maps index: " << cx + cy * width );
            break;
        }
        // preload in to capture objects cache
        map_captureobj.Set( Maps::GetIndexFromAbsPoint( cx, cy ), MP2::OBJ_CASTLE, Color::NONE );
    }

    DEBUG_LOG( DBG_GAME, DBG_INFO, "read coord castles, tellg: " << fs.tell() );
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
                           << "unknown id: " << id << ", maps index: " << cx + cy * width );
            break;
        }
    }

    DEBUG_LOG( DBG_GAME, DBG_INFO, "read coord other resource, tellg: " << fs.tell() );
    fs.seek( endof_addons + ( 72 * 3 ) + ( 144 * 3 ) );

    // byte: num obelisks (01 default)
    fs.skip( 1 );

    // count final mp2 blocks
    u32 countblock = 0;
    while ( 1 ) {
        const u32 l = fs.get();
        const u32 h = fs.get();

        if ( 0 == h && 0 == l )
            break;
        else {
            countblock = 256 * h + l - 1;
        }
    }

    // castle or heroes or (events, rumors, etc)
    for ( u32 ii = 0; ii < countblock; ++ii ) {
        s32 findobject = -1;

        // read block
        size_t sizeblock = fs.getLE16();
        std::vector<u8> pblock = fs.getRaw( sizeblock );

        for ( MapsIndexes::const_iterator it_index = vec_object.begin(); it_index != vec_object.end() && findobject < 0; ++it_index ) {
            const Maps::Tiles & tile = vec_tiles[*it_index];

            // orders(quantity2, quantity1)
            u32 orders = ( tile.GetQuantity2() ? tile.GetQuantity2() : 0 );
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
                if ( SIZEOFMP2CASTLE != pblock.size() ) {
                    DEBUG_LOG( DBG_GAME, DBG_WARN,
                               "read castle: "
                                   << "incorrect size block: " << pblock.size() );
                }
                else {
                    Castle * castle = GetCastle( Maps::GetPoint( findobject ) );
                    if ( castle ) {
                        castle->LoadFromMP2( StreamBuf( pblock ) );
                        Maps::MinimizeAreaForCastle( castle->GetCenter() );
                        map_captureobj.SetColor( tile.GetIndex(), castle->GetColor() );
                    }
                    else {
                        DEBUG_LOG( DBG_GAME, DBG_WARN,
                                   "load castle: "
                                       << "not found, index: " << findobject );
                    }
                }
                break;
            case MP2::OBJ_RNDTOWN:
            case MP2::OBJ_RNDCASTLE:
                // add rnd castle
                if ( SIZEOFMP2CASTLE != pblock.size() ) {
                    DEBUG_LOG( DBG_GAME, DBG_WARN,
                               "read castle: "
                                   << "incorrect size block: " << pblock.size() );
                }
                else {
                    Castle * castle = GetCastle( Maps::GetPoint( findobject ) );
                    if ( castle ) {
                        castle->LoadFromMP2( StreamBuf( pblock ) );
                        Maps::UpdateCastleSprite( castle->GetCenter(), castle->GetRace(), castle->isCastle(), true );
                        Maps::MinimizeAreaForCastle( castle->GetCenter() );
                        map_captureobj.SetColor( tile.GetIndex(), castle->GetColor() );
                    }
                    else {
                        DEBUG_LOG( DBG_GAME, DBG_WARN,
                                   "load castle: "
                                       << "not found, index: " << findobject );
                    }
                }
                break;
            case MP2::OBJ_JAIL:
                // add jail
                if ( SIZEOFMP2HEROES != pblock.size() ) {
                    DEBUG_LOG( DBG_GAME, DBG_WARN,
                               "read heroes: "
                                   << "incorrect size block: " << pblock.size() );
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
                if ( SIZEOFMP2HEROES != pblock.size() ) {
                    DEBUG_LOG( DBG_GAME, DBG_WARN,
                               "read heroes: "
                                   << "incorrect size block: " << pblock.size() );
                }
                else {
                    std::pair<int, int> colorRace = Maps::Tiles::ColorRaceFromHeroSprite( tile.GetObjectSpriteIndex() );
                    const Kingdom & kingdom = GetKingdom( colorRace.first );

                    if ( colorRace.second == Race::RAND && colorRace.first != Color::NONE )
                        colorRace.second = kingdom.GetRace();

                    // check heroes max count
                    if ( kingdom.AllowRecruitHero( false, 0 ) ) {
                        Heroes * hero = nullptr;

                        if ( pblock[17] && pblock[18] < Heroes::BAX )
                            hero = vec_heroes.Get( pblock[18] );

                        if ( !hero || !hero->isFreeman() )
                            hero = vec_heroes.GetFreeman( colorRace.second );

                        if ( hero )
                            hero->LoadFromMP2( findobject, colorRace.first, colorRace.second, StreamBuf( pblock ) );
                    }
                    else {
                        DEBUG_LOG( DBG_GAME, DBG_WARN, "load heroes maximum" );
                    }
                }
                break;
            case MP2::OBJ_SIGN:
            case MP2::OBJ_BOTTLE:
                // add sign or buttle
                if ( SIZEOFMP2SIGN - 1 < pblock.size() && 0x01 == pblock[0] ) {
                    MapSign * obj = new MapSign();
                    obj->LoadFromMP2( findobject, StreamBuf( pblock ) );
                    map_objects.add( obj );
                }
                break;
            case MP2::OBJ_EVENT:
                // add event maps
                if ( SIZEOFMP2EVENT - 1 < pblock.size() && 0x01 == pblock[0] ) {
                    MapEvent * obj = new MapEvent();
                    obj->LoadFromMP2( findobject, StreamBuf( pblock ) );
                    map_objects.add( obj );
                }
                break;
            case MP2::OBJ_SPHINX:
                // add riddle sphinx
                if ( SIZEOFMP2RIDDLE - 1 < pblock.size() && 0x00 == pblock[0] ) {
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
            if ( SIZEOFMP2EVENT - 1 < pblock.size() && 1 == pblock[42] ) {
                vec_eventsday.emplace_back( EventDate() );
                vec_eventsday.back().LoadFromMP2( StreamBuf( pblock ) );
            }
            // add rumors
            else if ( SIZEOFMP2RUMOR - 1 < pblock.size() ) {
                if ( pblock[8] ) {
                    vec_rumors.push_back( Game::GetEncodeString( StreamBuf( &pblock[8], pblock.size() - 8 ).toString() ) );
                    DEBUG_LOG( DBG_GAME, DBG_INFO, "add rumors: " << vec_rumors.back() );
                }
            }
        }
        // debug
        else {
            DEBUG_LOG( DBG_GAME, DBG_WARN, "read maps: unknown block addons, size: " << pblock.size() );
        }
    }

    ProcessNewMap();

    DEBUG_LOG( DBG_GAME, DBG_INFO, "end load" );
    return true;
}

void World::ProcessNewMap()
{
    // modify other objects
    for ( size_t i = 0; i < vec_tiles.size(); ++i ) {
        Maps::Tiles & tile = vec_tiles[i];
        Maps::Tiles::FixedPreload( tile );

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
        case MP2::OBJ_SHIPWRECKSURVIROR:
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
        } break;

        default:
            break;
        }
    }

    // add heroes to kingdoms
    vec_kingdoms.AddHeroes( vec_heroes );

    // add castles to kingdoms
    vec_kingdoms.AddCastles( vec_castles );

    // update wins, loss conditions
    if ( GameOver::WINS_HERO & Settings::Get().ConditionWins() ) {
        const Heroes * hero = GetHeroes( Settings::Get().WinsMapsPositionObject() );
        heroes_cond_wins = hero ? hero->GetID() : Heroes::UNKNOWN;
    }
    if ( GameOver::LOSS_HERO & Settings::Get().ConditionLoss() ) {
        Heroes * hero = GetHeroes( Settings::Get().LossMapsPositionObject() );
        if ( hero ) {
            heroes_cond_loss = hero->GetID();
            hero->SetModes( Heroes::NOTDISMISS | Heroes::NOTDEFAULTS );
        }
    }

    PostLoad();

    // play with hero
    vec_kingdoms.ApplyPlayWithStartingHero();

    if ( Settings::Get().ExtWorldStartHeroLossCond4Humans() )
        vec_kingdoms.AddCondLossHeroes( vec_heroes );

    // play with debug hero
    if ( IS_DEVEL() ) {
        // get first castle position
        Kingdom & kingdom = GetKingdom( Color::GetFirst( Players::HumanColors() ) );

        if ( !kingdom.GetCastles().empty() ) {
            const Castle * castle = kingdom.GetCastles().front();
            const fheroes2::Point & cp = castle->GetCenter();
            Heroes * hero = vec_heroes.Get( Heroes::DEBUG_HERO );

            if ( hero && !world.GetTiles( cp.x, cp.y + 1 ).GetHeroes() ) {
                hero->Recruit( castle->GetColor(), fheroes2::Point( cp.x, cp.y + 1 ) );
            }
        }
    }

    // set ultimate
    MapsTiles::iterator it = std::find_if( vec_tiles.begin(), vec_tiles.end(),
                                           []( const Maps::Tiles & tile ) { return tile.isObject( static_cast<int>( MP2::OBJ_RNDULTIMATEARTIFACT ) ); } );
    fheroes2::Point ultimate_pos;

    // not found
    if ( vec_tiles.end() == it ) {
        // generate position for ultimate
        MapsIndexes pools;
        pools.reserve( vec_tiles.size() / 2 );

        for ( size_t i = 0; i < vec_tiles.size(); ++i ) {
            const Maps::Tiles & tile = vec_tiles[i];
            const int32_t x = tile.GetIndex() % width;
            if ( x < ultimateArtifactOffset || x >= width - ultimateArtifactOffset )
                continue;

            const int32_t y = tile.GetIndex() / width;
            if ( y < ultimateArtifactOffset || y >= height - ultimateArtifactOffset )
                continue;

            if ( tile.GoodForUltimateArtifact() )
                pools.emplace_back( tile.GetIndex() );
        }

        if ( !pools.empty() ) {
            Artifact ultimate = Artifact::Rand( Artifact::ART_ULTIMATE );
            if ( Settings::Get().isCampaignGameType() ) {
                const Campaign::CampaignSaveData & campaignData = Campaign::CampaignSaveData::Get();

                const std::vector<Campaign::ScenarioData> & scenarios = Campaign::CampaignData::getCampaignData( campaignData.getCampaignID() ).getAllScenarios();
                const int scenarioId = campaignData.getCurrentScenarioID();
                assert( scenarioId >= 0 && static_cast<size_t>( scenarioId ) < scenarios.size() );

                if ( scenarioId >= 0 && static_cast<size_t>( scenarioId ) < scenarios.size() ) {
                    const Campaign::ScenarioVictoryCondition victoryCondition = scenarios[scenarioId].getVictoryCondition();
                    if ( victoryCondition == Campaign::ScenarioVictoryCondition::OBTAIN_ULTIMATE_CROWN ) {
                        ultimate = Artifact::ULTIMATE_CROWN;
                    }
                }
            }

            const int32_t pos = Rand::Get( pools );
            ultimate_artifact.Set( pos, ultimate );
            ultimate_pos = Maps::GetPoint( pos );
        }
    }
    else {
        // remove ultimate artifact sprite
        const uint8_t objectIndex = it->GetObjectSpriteIndex();
        it->Remove( it->GetObjectUID() );
        it->setAsEmpty();
        ultimate_artifact.Set( it->GetIndex(), Artifact::FromMP2IndexSprite( objectIndex ) );
        ultimate_pos = ( *it ).GetCenter();
    }

    vec_rumors.emplace_back( _( "The ultimate artifact is really the %{name}." ) );
    StringReplace( vec_rumors.back(), "%{name}", ultimate_artifact.GetName() );

    vec_rumors.emplace_back( _( "The ultimate artifact may be found in the %{name} regions of the world." ) );

    if ( height / 3 > ultimate_pos.y ) {
        if ( width / 3 > ultimate_pos.x )
            StringReplace( vec_rumors.back(), "%{name}", _( "north-west" ) );
        else if ( 2 * width / 3 > ultimate_pos.x )
            StringReplace( vec_rumors.back(), "%{name}", _( "north" ) );
        else
            StringReplace( vec_rumors.back(), "%{name}", _( "north-east" ) );
    }
    else if ( 2 * height / 3 > ultimate_pos.y ) {
        if ( width / 3 > ultimate_pos.x )
            StringReplace( vec_rumors.back(), "%{name}", _( "west" ) );
        else if ( 2 * width / 3 > ultimate_pos.x )
            StringReplace( vec_rumors.back(), "%{name}", _( "center" ) );
        else
            StringReplace( vec_rumors.back(), "%{name}", _( "east" ) );
    }
    else {
        if ( width / 3 > ultimate_pos.x )
            StringReplace( vec_rumors.back(), "%{name}", _( "south-west" ) );
        else if ( 2 * width / 3 > ultimate_pos.x )
            StringReplace( vec_rumors.back(), "%{name}", _( "south" ) );
        else
            StringReplace( vec_rumors.back(), "%{name}", _( "south-east" ) );
    }

    vec_rumors.emplace_back( _( "The truth is out there." ) );
    vec_rumors.emplace_back( _( "The dark side is stronger." ) );
    vec_rumors.emplace_back( _( "The end of the world is near." ) );
    vec_rumors.emplace_back( _( "The bones of Lord Slayer are buried in the foundation of the arena." ) );
    vec_rumors.emplace_back( _( "A Black Dragon will take out a Titan any day of the week." ) );
    vec_rumors.emplace_back( _( "He told her: Yada yada yada...  and then she said: Blah, blah, blah..." ) );
    vec_rumors.emplace_back( _( "An unknown force is being ressurected..." ) );

    vec_rumors.emplace_back( _( "Check the newest version of game at\nhttps://github.com/ihhub/\nfheroes2/releases" ) );
}
