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

#include "game_io.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <ctime>
#include <ostream>
#include <utility>

#include "campaign_savedata.h"
#include "campaign_scenariodata.h"
#include "dialog.h"
#include "game.h"
#include "game_over.h"
#include "logging.h"
#include "maps_fileinfo.h"
#include "save_format_version.h"
#include "serialize.h"
#include "settings.h"
#include "system.h"
#include "translations.h"
#include "ui_dialog.h"
#include "ui_language.h"
#include "world.h"
#include "zzlib.h"

namespace
{
    const std::string autoSaveName{ "AUTOSAVE" };

    const uint16_t SAV2ID3 = 0xFF03;

    uint16_t versionOfCurrentSaveFile = CURRENT_FORMAT_VERSION;

    std::string lastSaveName;

    struct HeaderSAV
    {
        enum
        {
            REQUIRES_POL_RESOURCES = 0x4000
        };

        HeaderSAV()
            : status( 0 )
            , gameType( 0 )
        {}

        HeaderSAV( const Maps::FileInfo & fi, const int type, const uint32_t worldDay, const uint32_t worldWeek, const uint32_t worldMonth )
            : status( 0 )
            , info( fi )
            , gameType( type )
        {
            time_t rawtime;
            std::time( &rawtime );

            info.timestamp = static_cast<uint32_t>( rawtime );
            info.worldDay = worldDay;
            info.worldWeek = worldWeek;
            info.worldMonth = worldMonth;

            if ( fi.version != GameVersion::SUCCESSION_WARS ) {
                status |= REQUIRES_POL_RESOURCES;
            }
        }

        uint16_t status;
        Maps::FileInfo info;
        int gameType;
    };

    OStreamBase & operator<<( OStreamBase & stream, const HeaderSAV & hdr )
    {
        return stream << hdr.status << hdr.info << hdr.gameType;
    }

    IStreamBase & operator>>( IStreamBase & stream, HeaderSAV & hdr )
    {
        return stream >> hdr.status >> hdr.info >> hdr.gameType;
    }
}

bool Game::AutoSave()
{
    return Game::Save( System::concatPath( GetSaveDir(), autoSaveName + GetSaveFileExtension() ), true );
}

bool Game::Save( const std::string & filePath, const bool autoSave /* = false */ )
{
    DEBUG_LOG( DBG_GAME, DBG_INFO, filePath )

    const Settings & conf = Settings::Get();

    StreamFile fileStream;
    fileStream.setBigendian( true );

    if ( !fileStream.open( filePath, "wb" ) ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, "Error opening the file " << filePath )
        return false;
    }

    // Always use the latest version of the file save format
    SetVersionOfCurrentSaveFile( CURRENT_FORMAT_VERSION );
    uint16_t saveFileVersion = CURRENT_FORMAT_VERSION;

    // Header
    fileStream << SAV2ID3 << std::to_string( saveFileVersion ) << saveFileVersion
               << HeaderSAV( conf.getCurrentMapInfo(), conf.GameType(), world.GetDay(), world.GetWeek(), world.GetMonth() );
    if ( fileStream.fail() ) {
        return false;
    }

    RWStreamBuf dataStream;
    dataStream.setBigendian( true );

    dataStream << World::Get() << Settings::Get() << GameOver::Result::Get();
    if ( dataStream.fail() ) {
        return false;
    }

    if ( conf.isCampaignGameType() ) {
        dataStream << Campaign::CampaignSaveData::Get();
    }

    // End-of-data marker
    dataStream << SAV2ID3;
    if ( dataStream.fail() || !Compression::zipStreamBuf( dataStream, fileStream ) ) {
        return false;
    }

    if ( !autoSave ) {
        Game::SetLastSaveName( filePath );
    }

    return true;
}

fheroes2::GameMode Game::Load( const std::string & filePath )
{
    DEBUG_LOG( DBG_GAME, DBG_INFO, filePath )

    const auto showGenericErrorMessage = []() { fheroes2::showStandardTextMessage( _( "Error" ), _( "The save file is corrupted." ), Dialog::OK ); };

    StreamFile fileStream;
    fileStream.setBigendian( true );

    if ( !fileStream.open( filePath, "rb" ) ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, "Error opening the file " << filePath )

        showGenericErrorMessage();

        return fheroes2::GameMode::CANCEL;
    }

    uint16_t savId = 0;
    fileStream >> savId;

    if ( savId != SAV2ID3 ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, "Invalid file identifier in the file " << filePath )

        showGenericErrorMessage();

        return fheroes2::GameMode::CANCEL;
    }

    std::string saveFileVersionStr;
    uint16_t saveFileVersion = 0;

    fileStream >> saveFileVersionStr >> saveFileVersion;
    if ( fileStream.fail() ) {
        showGenericErrorMessage();
        return fheroes2::GameMode::CANCEL;
    }

    DEBUG_LOG( DBG_GAME, DBG_TRACE, "Version of the file " << filePath << ": " << saveFileVersion )

    if ( saveFileVersion > CURRENT_FORMAT_VERSION || saveFileVersion < LAST_SUPPORTED_FORMAT_VERSION ) {
        std::string errorMessage( _( "Unsupported save format: " ) );
        errorMessage += std::to_string( saveFileVersion );
        errorMessage += ".\n";
        errorMessage += _( "Current game version: " );
        errorMessage += std::to_string( CURRENT_FORMAT_VERSION );
        errorMessage += ".\n";
        errorMessage += _( "Last supported version: " );
        errorMessage += std::to_string( LAST_SUPPORTED_FORMAT_VERSION );
        errorMessage += ".\n";

        fheroes2::showStandardTextMessage( _( "Error" ), errorMessage, Dialog::OK );

        return fheroes2::GameMode::CANCEL;
    }

    SetVersionOfCurrentSaveFile( saveFileVersion );

    HeaderSAV header;
    fileStream >> header;

    Settings & conf = Settings::Get();
    if ( ( conf.GameType() & header.gameType ) == 0 ) {
        fheroes2::showStandardTextMessage( _( "Error" ), _( "This file contains a save with an invalid game type." ), Dialog::OK );

        return fheroes2::GameMode::CANCEL;
    }

    RWStreamBuf dataStream;
    dataStream.setBigendian( true );

    if ( !Compression::unzipStream( fileStream, dataStream ) ) {
        showGenericErrorMessage();
        return fheroes2::GameMode::CANCEL;
    }

    if ( ( header.status & HeaderSAV::REQUIRES_POL_RESOURCES ) && !conf.isPriceOfLoyaltySupported() ) {
        fheroes2::showStandardTextMessage( _( "Error" ),
                                           _( "This save file requires \"The Price of Loyalty\" game assets, but they have not been provided to the engine." ),
                                           Dialog::OK );

        return fheroes2::GameMode::CANCEL;
    }

    dataStream >> World::Get() >> conf >> GameOver::Result::Get();
    if ( dataStream.fail() ) {
        showGenericErrorMessage();
        return fheroes2::GameMode::CANCEL;
    }

    fheroes2::GameMode returnValue = fheroes2::GameMode::START_GAME;

    if ( conf.isCampaignGameType() ) {
        Campaign::CampaignSaveData & saveData = Campaign::CampaignSaveData::Get();
        dataStream >> saveData;

        if ( !saveData.isStarting() && saveData.getCurrentScenarioInfoId() == saveData.getLastCompletedScenarioInfoID() ) {
            // This is the end of the current scenario. We should show next scenario selection.
            returnValue = fheroes2::GameMode::COMPLETE_CAMPAIGN_SCENARIO_FROM_LOAD_FILE;
        }
    }

    uint16_t endOfDataMarker = 0;
    dataStream >> endOfDataMarker;
    if ( dataStream.fail() || endOfDataMarker != SAV2ID3 ) {
        showGenericErrorMessage();
        return fheroes2::GameMode::CANCEL;
    }

    // Settings should contain the full path to the current map file, if this map is available
    conf.getCurrentMapInfo().filename = Settings::GetLastFile( "maps", System::GetFileName( conf.getCurrentMapInfo().filename ) );

    if ( !conf.loadedFileLanguage().empty() && conf.loadedFileLanguage() != "en" && conf.loadedFileLanguage() != conf.getGameLanguage() ) {
        std::string warningMessage( _( "This saved game is localized to '" ) );
        warningMessage.append( fheroes2::getLanguageName( fheroes2::getLanguageFromAbbreviation( conf.loadedFileLanguage() ) ) );
        warningMessage.append( _( "' language, but the current language of the game is '" ) );
        warningMessage.append( fheroes2::getLanguageName( fheroes2::getLanguageFromAbbreviation( conf.getGameLanguage() ) ) );
        warningMessage += "'.";

        fheroes2::showStandardTextMessage( _( "Warning" ), warningMessage, Dialog::OK );
    }

    Game::SetLastSaveName( filePath );
    conf.SetGameType( conf.GameType() | Game::TYPE_LOADFILE );

    return returnValue;
}

bool Game::LoadSAV2FileInfo( std::string filePath, Maps::FileInfo & fileInfo )
{
    DEBUG_LOG( DBG_GAME, DBG_INFO, filePath )

    StreamFile fs;
    fs.setBigendian( true );

    if ( !fs.open( filePath, "rb" ) ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, "Error opening the file " << filePath )
        return false;
    }

    uint16_t savId = 0;
    fs >> savId;

    if ( savId != SAV2ID3 ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, "Invalid file identifier in the file " << filePath )
        return false;
    }

    std::string saveFileVersionStr;
    uint16_t saveFileVersion = 0;

    fs >> saveFileVersionStr >> saveFileVersion;

    DEBUG_LOG( DBG_GAME, DBG_TRACE, "Version of the file " << filePath << ": " << saveFileVersion )

    if ( saveFileVersion > CURRENT_FORMAT_VERSION || saveFileVersion < LAST_SUPPORTED_FORMAT_VERSION ) {
        return false;
    }

    SetVersionOfCurrentSaveFile( saveFileVersion );

    HeaderSAV header;
    fs >> header;

    if ( fs.fail() ) {
        return false;
    }

    if ( ( Settings::Get().GameType() & header.gameType ) == 0 ) {
        return false;
    }

    fileInfo = std::move( header.info );
    fileInfo.filename = std::move( filePath );

    return true;
}

void Game::SetVersionOfCurrentSaveFile( const uint16_t version )
{
    versionOfCurrentSaveFile = version;
}

uint16_t Game::GetVersionOfCurrentSaveFile()
{
    return versionOfCurrentSaveFile;
}

const std::string & Game::GetLastSaveName()
{
    return lastSaveName;
}

void Game::SetLastSaveName( const std::string & name )
{
    lastSaveName = name;
}

std::string Game::GetSaveDir()
{
    return System::concatPath( System::concatPath( System::GetDataDirectory( "fheroes2" ), "files" ), "save" );
}

std::string Game::GetSaveFileBaseName()
{
    std::string baseName = Settings::Get().getCurrentMapInfo().name;

    // Replace all non-ASCII characters by exclamation marks
    std::replace_if(
        baseName.begin(), baseName.end(), []( const unsigned char c ) { return ( c > 127 ); }, '!' );
    // Remove all non-printable characters
    baseName.erase( std::remove_if( baseName.begin(), baseName.end(), []( const unsigned char c ) { return !std::isprint( c ); } ), baseName.end() );
    // Replace all remaining non-alphanumeric characters (excluding exclamation marks) by underscores
    std::replace_if(
        baseName.begin(), baseName.end(), []( const unsigned char c ) { return ( c != '!' && !std::isalnum( c ) ); }, '_' );
    // If in the end there are no characters left, set the base name to "newgame"
    if ( baseName.empty() ) {
        baseName = "newgame";
    }

    return baseName;
}

std::string Game::GetSaveFileExtension()
{
    return GetSaveFileExtension( Settings::Get().GameType() );
}

std::string Game::GetSaveFileExtension( const int gameType )
{
    if ( gameType & Game::TYPE_STANDARD )
        return ".sav";
    else if ( gameType & Game::TYPE_CAMPAIGN )
        return ".savc";
    else if ( gameType & Game::TYPE_HOTSEAT )
        return ".savh";

    return ".savm";
}

bool Game::SaveCompletedCampaignScenario()
{
    return Save( System::concatPath( GetSaveDir(), GetSaveFileBaseName() ) + "_Complete" + GetSaveFileExtension() );
}
