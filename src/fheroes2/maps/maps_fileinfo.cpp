/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2025                                             *
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

#include "maps_fileinfo.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <functional>
#include <list>
#include <map>
#include <sstream>
#include <type_traits>
#include <utility>

#include "color.h"
#include "difficulty.h"
#include "dir.h"
#include "game_io.h"
#include "game_over.h"
#include "logging.h"
#include "map_format_info.h"
#include "maps_tiles.h"
#include "maps_tiles_helper.h"
#include "mp2.h"
#include "mp2_helper.h"
#include "race.h"
#include "save_format_version.h"
#include "serialize.h"
#include "settings.h"
#include "system.h"
#include "tools.h"
#include "ui_font.h"
#include "ui_language.h"

namespace
{
    const size_t mapNameLength = 16;
    const size_t mapDescriptionLength = 200;

    // This function returns an unsorted array. It is a caller responsibility to take care of sorting if needed.
    MapsFileInfoList getValidMaps( const ListFiles & mapFiles, const uint8_t humanPlayerCount, const bool isForEditor, const bool isOriginalMapFormat )
    {
        // create a list of unique maps (based on the map file name) and filter it by the preferred number of players
        std::map<std::string, Maps::FileInfo, std::less<>> uniqueMaps;

        // Maps made by the original French version Editor or hacked maps could contain
        // special ASCII characters that are not supposed to be there.
        // While reading the original maps we attempt to fix these characters
        // with an assumption that it might alter the original map maker text but to be true to the original game allowed characters.
        const bool fixSpecialFrenchCharacters
            = isOriginalMapFormat
              && ( fheroes2::getCurrentLanguage() == fheroes2::SupportedLanguage::French && fheroes2::getResourceLanguage() == fheroes2::SupportedLanguage::French );

        for ( const std::string & mapFile : mapFiles ) {
            Maps::FileInfo fi;

            if ( isOriginalMapFormat ) {
                if ( !fi.readMP2Map( mapFile, isForEditor ) ) {
                    continue;
                }
            }
            else {
                if ( !fi.readResurrectionMap( mapFile, isForEditor ) ) {
                    continue;
                }
            }

            if ( !isForEditor ) {
                assert( humanPlayerCount >= 1 );

                const int humanOnlyColorsCount = Color::Count( fi.HumanOnlyColors() );
                if ( humanOnlyColorsCount > humanPlayerCount ) {
                    // This map requires more human-only players than needed.
                    continue;
                }

                const int computerHumanColorsCount = Color::Count( fi.AllowCompHumanColors() );
                if ( humanPlayerCount > ( humanOnlyColorsCount + computerHumanColorsCount ) ) {
                    // This map does not allow to be played by this number of human players.
                    continue;
                }

                if ( humanOnlyColorsCount == humanPlayerCount ) {
                    // The map has the exact number of human-only players. Make sure that the user cannot select any other players.
                    fi.removeHumanColors( fi.AllowCompHumanColors() );
                }

                // Update French language-specific characters to match CP1252.
                if ( fixSpecialFrenchCharacters ) {
                    fheroes2::fixFrenchCharactersForMP2Map( fi.name );
                    fheroes2::fixFrenchCharactersForMP2Map( fi.description );
                }
            }

            uniqueMaps.try_emplace( System::GetFileName( mapFile ), std::move( fi ) );
        }

        MapsFileInfoList result;
        result.reserve( uniqueMaps.size() );

        for ( auto & [name, info] : uniqueMaps ) {
            result.emplace_back( std::move( info ) );
        }

        return result;
    }
}

namespace Editor
{
    enum
    {
        Wins = 0x1000,
        CaptureTown = 0x1001,
        DefeatHero = 0x1002,
        FindArtifact = 0x1003,
        SideWins = 0x1004,
        AccumulateGold = 0x1005,
        CompAlsoWins = 0x0100,
        AllowNormalVictory = 0x0200,
        Loss = 0x2000,
        LoseTown = 0x2001,
        LoseHero = 0x2002,
        OutTime = 0x2003
    };
}

void Maps::FileInfo::Reset()
{
    filename.clear();
    name.clear();
    description.clear();

    width = 0;
    height = 0;
    difficulty = Difficulty::NORMAL;

    static_assert( std::is_same_v<decltype( races ), std::array<uint8_t, maxNumOfPlayers>> );
    static_assert( std::is_same_v<decltype( unions ), std::array<PlayerColorsSet, maxNumOfPlayers>> );

    for ( int i = 0; i < maxNumOfPlayers; ++i ) {
        races[i] = Race::NONE;
        unions[i] = static_cast<PlayerColorsSet>( Color::IndexToColor( i ) );
    }

    kingdomColors = 0;
    colorsAvailableForHumans = 0;
    colorsAvailableForComp = 0;
    colorsOfRandomRaces = 0;

    victoryConditionType = VICTORY_DEFEAT_EVERYONE;
    compAlsoWins = false;
    allowNormalVictory = false;
    victoryConditionParams.fill( 0 );

    lossConditionType = LOSS_EVERYTHING;
    lossConditionParams.fill( 0 );

    timestamp = 0;

    startWithHeroInFirstCastle = false;

    version = GameVersion::SUCCESSION_WARS;

    worldDay = 0;
    worldWeek = 0;
    worldMonth = 0;

    mainLanguage = fheroes2::SupportedLanguage::English;
}

bool Maps::FileInfo::readMP2Map( std::string filePath, const bool isForEditor )
{
    Reset();

    StreamFile fs;
    if ( !fs.open( filePath, "rb" ) ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, "Error opening the file " << filePath )
        return false;
    }

    // magic byte
    if ( fs.getBE32() != 0x5C000000 ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, "File " << filePath << " is not a valid map file" )
        return false;
    }

    filename = std::move( filePath );

    // Difficulty level
    switch ( fs.getLE16() ) {
    case 0x00:
        difficulty = Difficulty::EASY;
        break;
    case 0x01:
        difficulty = Difficulty::NORMAL;
        break;
    case 0x02:
        difficulty = Difficulty::HARD;
        break;
    case 0x03:
        difficulty = Difficulty::EXPERT;
        break;
    default:
        difficulty = Difficulty::NORMAL;
        break;
    }

    // Width & height of the map in tiles
    width = fs.get();
    height = fs.get();

    const PlayerColorsVector colors( Color::allPlayerColors() );

    // Colors used by kingdoms: blue, green, red, yellow, orange, purple
    for ( const PlayerColor color : colors ) {
        if ( fs.get() != 0 ) {
            kingdomColors |= color;
        }
    }

    // Colors available for human players: blue, green, red, yellow, orange, purple
    for ( const PlayerColor color : colors ) {
        if ( fs.get() != 0 ) {
            colorsAvailableForHumans |= color;
        }
    }

    // Colors available for computer players: blue, green, red, yellow, orange, purple
    for ( const PlayerColor color : colors ) {
        if ( fs.get() != 0 ) {
            colorsAvailableForComp |= color;
        }
    }

    if ( !isForEditor && colorsAvailableForHumans == 0 ) {
        // This is not a valid map since no human players exist so it cannot be played.
        DEBUG_LOG( DBG_GAME, DBG_WARN, "Map " << filename << " does not contain any human players." )
        return false;
    }

    // TODO: Number of active kingdoms (unused)
    // fs.seekg(0x1A, std::ios_base::beg);
    // fs.get();

    fs.seek( 29 );
    // Victory condition type.
    victoryConditionType = fs.get();
    // Do the victory conditions apply to AI too?
    compAlsoWins = ( fs.get() != 0 );
    // Is "normal victory" (defeating all other players) applicable here?
    allowNormalVictory = ( fs.get() != 0 );
    // Parameter of victory condition.
    victoryConditionParams[0] = fs.getLE16();
    // Loss condition type.
    lossConditionType = fs.get();
    // Parameter of loss condition.
    lossConditionParams[0] = fs.getLE16();
    // Does the game start with a hero in the first castle?
    startWithHeroInFirstCastle = ( 0 == fs.get() );

    static_assert( std::is_same_v<decltype( races ), std::array<uint8_t, maxNumOfPlayers>> );

    // Initial races.
    for ( const PlayerColor color : colors ) {
        const uint8_t race = Race::IndexToRace( fs.get() );
        const int idx = Color::GetIndex( color );
        assert( idx < maxNumOfPlayers );

        races[idx] = race;

        if ( Race::RAND == race ) {
            colorsOfRandomRaces |= color;
        }
    }

    // Additional parameter of victory condition.
    victoryConditionParams[1] = fs.getLE16();
    // Additional parameter of loss condition.
    lossConditionParams[1] = fs.getLE16();

    bool skipUnionSetup = false;
    // If loss conditions are LOSS_HERO and victory conditions are VICTORY_DEFEAT_EVERYONE then we have to verify the color to which this object belongs to.
    // If the color is under computer control only we have to make it as an ally for human player.
    if ( lossConditionType == LOSS_HERO && victoryConditionType == VICTORY_DEFEAT_EVERYONE && Color::Count( colorsAvailableForHumans ) == 1 ) {
        // Each tile needs 16 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 8 + 16 + 32 + 32 = 160 bits or 20 bytes.
        fs.seek( MP2::MP2_MAP_INFO_SIZE + ( lossConditionParams[0] + lossConditionParams[1] * width ) * 20 );

        MP2::MP2TileInfo mp2tile;
        MP2::loadTile( fs, mp2tile );

        Maps::Tile tile;
        tile.Init( mp2tile );
        tile.sortObjectParts();

        if ( const auto [color, dummy] = getColorRaceFromHeroSprite( tile.getMainObjectPart().icnIndex ); ( colorsAvailableForHumans & color ) == 0 ) {
            const PlayerColorsSet side1 = colorsAvailableForHumans | color;
            const PlayerColorsSet side2 = colorsAvailableForComp ^ color;

            FillUnions( side1, side2 );

            victoryConditionType = VICTORY_DEFEAT_OTHER_SIDE;

            skipUnionSetup = true;
        }
    }

    // Map name
    fs.seek( 58 );
    name = fs.getString( mapNameLength );

    // Map description
    fs.seek( 118 );
    description = fs.getString( mapDescriptionLength );

    // Alliances of kingdoms
    if ( victoryConditionType == VICTORY_DEFEAT_OTHER_SIDE && !skipUnionSetup ) {
        PlayerColorsSet side1 = 0;
        PlayerColorsSet side2 = 0;

        const PlayerColorsVector availableColors( kingdomColors );
        if ( availableColors.empty() ) {
            DEBUG_LOG( DBG_GAME, DBG_WARN, "File " << filename << ": invalid list of kingdom colors during map load" )
            return false;
        }

        const int numPlayersSide1 = victoryConditionParams[0];
        if ( ( numPlayersSide1 <= 0 ) || ( numPlayersSide1 >= static_cast<int>( availableColors.size() ) ) ) {
            DEBUG_LOG( DBG_GAME, DBG_WARN, "File " << filename << ": invalid victory condition param during map load" )
            return false;
        }

        int playerIdx = 0;
        for ( const PlayerColor color : availableColors ) {
            if ( playerIdx < numPlayersSide1 ) {
                side1 |= color;
            }
            else {
                side2 |= color;
            }

            ++playerIdx;
        }

        FillUnions( side1, side2 );
    }

    // Determine the type of the map
    const size_t pos = filename.rfind( '.' );
    if ( pos != std::string::npos ) {
        const std::string fileExtension = StringLower( filename.substr( pos + 1 ) );

        version = ( fileExtension == "mx2" || fileExtension == "hxc" ) ? GameVersion::PRICE_OF_LOYALTY : GameVersion::SUCCESSION_WARS;
    }

    return true;
}

bool Maps::FileInfo::readResurrectionMap( std::string filePath, const bool isForEditor )
{
    Reset();

    Maps::Map_Format::BaseMapFormat map;
    if ( !Maps::Map_Format::loadBaseMap( filePath, map ) ) {
        return false;
    }

    if ( !loadResurrectionMap( map, std::move( filePath ) ) ) {
        return false;
    }

    if ( !isForEditor && colorsAvailableForHumans == 0 ) {
        // This is not a valid map since no human players exist so it cannot be played.
        DEBUG_LOG( DBG_GAME, DBG_WARN, "Map " << filename << " does not contain any human players." )
        return false;
    }

    return true;
}

bool Maps::FileInfo::loadResurrectionMap( const Map_Format::BaseMapFormat & map, std::string filePath )
{
    Reset();

    filename = std::move( filePath );

    difficulty = map.difficulty;

    width = static_cast<uint16_t>( map.width );
    height = static_cast<uint16_t>( map.width );

    name = map.name;
    description = map.description;

    assert( ( map.availablePlayerColors & map.humanPlayerColors ) == map.humanPlayerColors );
    assert( ( map.availablePlayerColors & map.computerPlayerColors ) == map.computerPlayerColors );
    assert( ( map.availablePlayerColors & ( map.humanPlayerColors | map.computerPlayerColors ) ) == map.availablePlayerColors );

    kingdomColors = map.availablePlayerColors;
    colorsAvailableForHumans = map.humanPlayerColors;
    colorsAvailableForComp = map.computerPlayerColors;

    races = map.playerRace;

    victoryConditionType = map.victoryConditionType;
    compAlsoWins = map.isVictoryConditionApplicableForAI;
    allowNormalVictory = map.allowNormalVictory;

    lossConditionType = map.lossConditionType;

    switch ( lossConditionType ) {
    case LOSS_EVERYTHING:
        // No special conditions of loss.
        // So, no metadata should exist.
        assert( map.lossConditionMetadata.empty() );
        break;
    case LOSS_TOWN:
    case LOSS_HERO:
        // Town or hero loss metadata must include the following:
        // - tile index of the object
        // - color of the object (the color is needed to modify the game for multi-player)
        assert( map.lossConditionMetadata.size() == 2 );
        lossConditionParams[0] = static_cast<uint16_t>( map.lossConditionMetadata[0] % map.width );
        lossConditionParams[1] = static_cast<uint16_t>( map.lossConditionMetadata[0] / map.width );
        assert( ( map.lossConditionMetadata[1] & static_cast<uint32_t>( map.humanPlayerColors ) ) == map.lossConditionMetadata[1] );
        break;
    case LOSS_OUT_OF_TIME:
        assert( map.lossConditionMetadata.size() == 1 );
        assert( map.lossConditionMetadata[0] > 0 );

        lossConditionParams[0] = static_cast<uint16_t>( map.lossConditionMetadata[0] );
        break;
    default:
        // Did you add a new loss condition? Add the logic above!
        assert( 0 );
        break;
    }

    switch ( victoryConditionType ) {
    case VICTORY_DEFEAT_EVERYONE:
        // Since this is a normal victory condition.
        // So, no metadata should exist.
        assert( map.victoryConditionMetadata.empty() );
        compAlsoWins = true;
        allowNormalVictory = true;
        break;
    case VICTORY_CAPTURE_TOWN:
    case VICTORY_KILL_HERO:
        // Town or hero capture metadata must include the following:
        // - tile index of the object
        // - color of the object (the color is needed to modify the game for multi-player mode)
        assert( map.victoryConditionMetadata.size() == 2 );
        victoryConditionParams[0] = static_cast<uint16_t>( map.victoryConditionMetadata[0] % map.width );
        victoryConditionParams[1] = static_cast<uint16_t>( map.victoryConditionMetadata[0] / map.width );
        assert( ( map.victoryConditionMetadata[1] & static_cast<uint32_t>( map.availablePlayerColors ) ) == map.victoryConditionMetadata[1] );
        break;
    case VICTORY_OBTAIN_ARTIFACT:
        assert( map.victoryConditionMetadata.size() == 1 );

        victoryConditionParams[0] = static_cast<uint16_t>( map.victoryConditionMetadata[0] );
        break;
    case VICTORY_COLLECT_ENOUGH_GOLD:
        assert( map.victoryConditionMetadata.size() == 1 );
        // 10,000 gold is the minimum value can be set since Easy difficulty gives a player this amount of gold.
        assert( map.victoryConditionMetadata[0] >= 10000 );

        // Divide by 1000 since the old format supports only this.
        victoryConditionParams[0] = static_cast<uint16_t>( map.victoryConditionMetadata[0] / 1000 );
        break;
    case VICTORY_DEFEAT_OTHER_SIDE:
        // As of now only 2 alliances are supported.
        assert( map.alliances.size() == 2 );

        assert( std::all_of( map.alliances.begin(), map.alliances.end(),
                             [&map = std::as_const( map )]( const PlayerColorsSet colors ) { return ( map.availablePlayerColors & colors ) == colors; } ) );

        FillUnions( map.alliances[0], map.alliances[1] );
        break;
    default:
        // Did you add a new victory condition? Add the logic for it!
        assert( 0 );
        break;
    }

    for ( size_t i = 0; i < races.size(); ++i ) {
        if ( races[i] == Race::RAND ) {
            colorsOfRandomRaces |= static_cast<PlayerColor>( 1 << i );
        }
    }

    version = GameVersion::RESURRECTION;

    mainLanguage = map.mainLanguage;

    return true;
}

void Maps::FileInfo::FillUnions( const PlayerColorsSet side1Colors, const PlayerColorsSet side2Colors )
{
    static_assert( std::is_same_v<decltype( unions ), std::array<PlayerColorsSet, maxNumOfPlayers>> );

    for ( int i = 0; i < maxNumOfPlayers; ++i ) {
        const PlayerColor color = Color::IndexToColor( i );

        if ( side1Colors & color ) {
            unions[i] = side1Colors;
        }
        else if ( side2Colors & color ) {
            unions[i] = side2Colors;
        }
        else {
            unions[i] = static_cast<PlayerColorsSet>( color );
        }
    }
}

bool Maps::FileInfo::CompareByFileName::operator()( const FileInfo & lhs, const FileInfo & rhs ) const
{
    return fheroes2::compareStringsCaseInsensitively( lhs.filename, rhs.filename );
}

bool Maps::FileInfo::CompareByFileName::operator()( const FileInfo & lhs, const std::string & rhs ) const
{
    return fheroes2::compareStringsCaseInsensitively( lhs.filename, rhs );
}

bool Maps::FileInfo::CompareByFileName::operator()( const std::string & lhs, const FileInfo & rhs ) const
{
    return fheroes2::compareStringsCaseInsensitively( lhs, rhs.filename );
}

bool Maps::FileInfo::CompareByMapName::operator()( const FileInfo & lhs, const FileInfo & rhs ) const
{
    return fheroes2::compareStringsCaseInsensitively( lhs.name, rhs.name );
}

int Maps::FileInfo::KingdomRace( const PlayerColor color ) const
{
    switch ( color ) {
    case PlayerColor::BLUE:
        return races[0];
    case PlayerColor::GREEN:
        return races[1];
    case PlayerColor::RED:
        return races[2];
    case PlayerColor::YELLOW:
        return races[3];
    case PlayerColor::ORANGE:
        return races[4];
    case PlayerColor::PURPLE:
        return races[5];
    default:
        break;
    }
    return 0;
}

uint32_t Maps::FileInfo::ConditionWins() const
{
    switch ( victoryConditionType ) {
    case VICTORY_DEFEAT_EVERYONE:
        return GameOver::WINS_ALL;
    case VICTORY_CAPTURE_TOWN:
        return allowNormalVictory ? ( GameOver::WINS_TOWN | GameOver::WINS_ALL ) : GameOver::WINS_TOWN;
    case VICTORY_KILL_HERO:
        return allowNormalVictory ? ( GameOver::WINS_HERO | GameOver::WINS_ALL ) : GameOver::WINS_HERO;
    case VICTORY_OBTAIN_ARTIFACT:
        return allowNormalVictory ? ( GameOver::WINS_ARTIFACT | GameOver::WINS_ALL ) : GameOver::WINS_ARTIFACT;
    case VICTORY_DEFEAT_OTHER_SIDE:
        return GameOver::WINS_SIDE;
    case VICTORY_COLLECT_ENOUGH_GOLD:
        return allowNormalVictory ? ( GameOver::WINS_GOLD | GameOver::WINS_ALL ) : GameOver::WINS_GOLD;
    default:
        // This is an unsupported winning condition! Please add the logic to handle it.
        assert( 0 );
        break;
    }

    return GameOver::COND_NONE;
}

uint32_t Maps::FileInfo::ConditionLoss() const
{
    switch ( lossConditionType ) {
    case LOSS_EVERYTHING:
        return GameOver::LOSS_ALL;
    case LOSS_TOWN:
        return GameOver::LOSS_TOWN;
    case LOSS_HERO:
        return GameOver::LOSS_HERO;
    case LOSS_OUT_OF_TIME:
        return GameOver::LOSS_TIME;
    default:
        // This is an unsupported loss condition! Please add the logic to handle it.
        assert( 0 );
        break;
    }

    return GameOver::COND_NONE;
}

bool Maps::FileInfo::WinsCompAlsoWins() const
{
    return compAlsoWins && ( ( GameOver::WINS_TOWN | GameOver::WINS_GOLD ) & ConditionWins() );
}

std::string Maps::FileInfo::getSummary() const
{
    std::ostringstream os;

    os << "Map name: " << name << std::endl
       << "File name: " << filename << std::endl
       << "Description: " << description << std::endl
       << "Size: " << width << "x" << height << std::endl
       << "Difficulty: " << Difficulty::String( difficulty ) << std::endl;

    return os.str();
}

OStreamBase & Maps::operator<<( OStreamBase & stream, const FileInfo & fi )
{
    // Only the filename part of the path to the map file is saved
    stream << System::GetFileName( fi.filename ) << fi.name << fi.description << fi.width << fi.height << fi.difficulty << static_cast<uint8_t>( maxNumOfPlayers );

    static_assert( std::is_same_v<decltype( fi.races ), std::array<uint8_t, maxNumOfPlayers>> );
    static_assert( std::is_same_v<decltype( fi.unions ), std::array<PlayerColorsSet, maxNumOfPlayers>> );

    for ( size_t i = 0; i < maxNumOfPlayers; ++i ) {
        stream << fi.races[i] << fi.unions[i];
    }

    return stream << fi.kingdomColors << fi.colorsAvailableForHumans << fi.colorsAvailableForComp << fi.colorsOfRandomRaces << fi.victoryConditionType << fi.compAlsoWins
                  << fi.allowNormalVictory << fi.victoryConditionParams[0] << fi.victoryConditionParams[1] << fi.lossConditionType << fi.lossConditionParams[0]
                  << fi.lossConditionParams[1] << fi.timestamp << fi.startWithHeroInFirstCastle << fi.version << fi.worldDay << fi.worldWeek << fi.worldMonth
                  << fi.mainLanguage;
}

IStreamBase & Maps::operator>>( IStreamBase & stream, FileInfo & fi )
{
    uint8_t kingdommax = 0;

    // Only the filename part of the path to the map file is loaded
    stream >> fi.filename >> fi.name >> fi.description >> fi.width >> fi.height >> fi.difficulty >> kingdommax;

    static_assert( std::is_same_v<decltype( fi.races ), std::array<uint8_t, maxNumOfPlayers>> );
    static_assert( std::is_same_v<decltype( fi.unions ), std::array<PlayerColorsSet, maxNumOfPlayers>> );

    using RacesItemType = decltype( fi.races )::value_type;
    using UnionsItemType = decltype( fi.unions )::value_type;

    for ( size_t i = 0; i < kingdommax; ++i ) {
        RacesItemType racesItem = 0;
        UnionsItemType unionsItem = static_cast<PlayerColorsSet>( PlayerColor::NONE );

        stream >> racesItem >> unionsItem;

        if ( i < maxNumOfPlayers ) {
            fi.races[i] = racesItem;
            fi.unions[i] = unionsItem;
        }
    }

    stream >> fi.kingdomColors >> fi.colorsAvailableForHumans >> fi.colorsAvailableForComp >> fi.colorsOfRandomRaces >> fi.victoryConditionType >> fi.compAlsoWins
        >> fi.allowNormalVictory >> fi.victoryConditionParams[0] >> fi.victoryConditionParams[1] >> fi.lossConditionType >> fi.lossConditionParams[0]
        >> fi.lossConditionParams[1] >> fi.timestamp >> fi.startWithHeroInFirstCastle >> fi.version >> fi.worldDay >> fi.worldWeek >> fi.worldMonth;

    static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_1103_RELEASE, "Remove the logic below." );
    if ( Game::GetVersionOfCurrentSaveFile() < FORMAT_VERSION_1103_RELEASE ) {
        fi.mainLanguage = fheroes2::SupportedLanguage::English;
    }
    else {
        stream >> fi.mainLanguage;
    }

    return stream;
}

MapsFileInfoList Maps::getAllMapFileInfos( const bool isForEditor, const uint8_t humanPlayerCount )
{
    ListFiles maps = Settings::FindFiles( "maps", ".mp2", false );

    const bool isPOLSupported = Settings::Get().isPriceOfLoyaltySupported();

    if ( isPOLSupported ) {
        maps.Append( Settings::FindFiles( "maps", ".mx2", false ) );
    }

    MapsFileInfoList validMaps = getValidMaps( maps, humanPlayerCount, isForEditor, true );

    if ( isPOLSupported ) {
        const ListFiles resurrectionMaps = Settings::FindFiles( "maps", ".fh2m", false );
        MapsFileInfoList validResurrectionMaps = getValidMaps( resurrectionMaps, humanPlayerCount, isForEditor, false );

        validMaps.reserve( maps.size() + resurrectionMaps.size() );

        for ( auto & map : validResurrectionMaps ) {
            validMaps.emplace_back( std::move( map ) );
        }
    }

    if ( isForEditor ) {
        std::sort( validMaps.begin(), validMaps.end(), Maps::FileInfo::CompareByFileName{} );
    }
    else {
        std::sort( validMaps.begin(), validMaps.end(), Maps::FileInfo::CompareByMapName{} );
    }

    return validMaps;
}

MapsFileInfoList Maps::getResurrectionMapFileInfos( const bool isForEditor, const uint8_t humanPlayerCount )
{
    if ( !Settings::Get().isPriceOfLoyaltySupported() ) {
        // Resurrection maps require POL resources presence.
        return {};
    }

    const ListFiles maps = Settings::FindFiles( "maps", ".fh2m", false );
    MapsFileInfoList validMaps = getValidMaps( maps, humanPlayerCount, isForEditor, false );

    if ( isForEditor ) {
        std::sort( validMaps.begin(), validMaps.end(), Maps::FileInfo::CompareByFileName{} );
    }
    else {
        std::sort( validMaps.begin(), validMaps.end(), Maps::FileInfo::CompareByMapName{} );
    }

    return validMaps;
}
