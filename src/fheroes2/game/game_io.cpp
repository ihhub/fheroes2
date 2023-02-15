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
#include <cctype>
#include <cstdint>
#include <ctime>
#include <ostream>

#include "campaign_savedata.h"
#include "campaign_scenariodata.h"
#include "dialog.h"
#include "game.h"
#include "game_io.h"
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
#include "ui_text.h"
#include "world.h"
#include "zzlib.h"

namespace
{
    const std::string autoSaveName{ "AUTOSAVE" };

    const uint16_t SAV2ID2 = 0xFF02;
    const uint16_t SAV2ID3 = 0xFF03;

    struct HeaderSAV
    {
        enum
        {
            IS_PRICE_OF_LOYALTY_MAP = 0x4000
        };

        HeaderSAV()
            : status( 0 )
            , gameType( 0 )
        {}

        HeaderSAV( const Maps::FileInfo & fi, const int gameType_ )
            : status( 0 )
            , info( fi )
            , gameType( gameType_ )
        {
            time_t rawtime;
            std::time( &rawtime );
            info.localtime = static_cast<uint32_t>( rawtime );

            if ( fi._version == GameVersion::PRICE_OF_LOYALTY )
                status |= IS_PRICE_OF_LOYALTY_MAP;
        }

        uint16_t status;
        Maps::FileInfo info;
        int gameType;
    };

    StreamBase & operator<<( StreamBase & msg, const HeaderSAV & hdr )
    {
        return msg << hdr.status << hdr.info << hdr.gameType;
    }

    StreamBase & operator>>( StreamBase & msg, HeaderSAV & hdr )
    {
        return msg >> hdr.status >> hdr.info >> hdr.gameType;
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

    StreamFile fs;
    fs.setbigendian( true );

    if ( !fs.open( filePath, "wb" ) ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, filePath << ", error open" )
        return false;
    }

    uint16_t loadver = GetLoadVersion();
    if ( !autoSave ) {
        Game::SetLastSavename( filePath );
    }

    // raw info content
    fs << static_cast<uint8_t>( SAV2ID3 >> 8 ) << static_cast<uint8_t>( SAV2ID3 & 0xFF ) << std::to_string( loadver ) << loadver
       << HeaderSAV( conf.CurrentFileInfo(), conf.GameType() );
    fs.close();

    ZStreamFile fz;
    fz.setbigendian( true );

    // zip game data content
    fz << loadver << World::Get() << Settings::Get() << GameOver::Result::Get();

    if ( conf.isCampaignGameType() )
        fz << Campaign::CampaignSaveData::Get();

    fz << SAV2ID3; // eof marker

    return !fz.fail() && fz.write( filePath, true );
}

fheroes2::GameMode Game::Load( const std::string & filePath )
{
    DEBUG_LOG( DBG_GAME, DBG_INFO, filePath )

    StreamFile fs;
    fs.setbigendian( true );

    if ( !fs.open( filePath, "rb" ) ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, filePath << ", error open" )
        return fheroes2::GameMode::CANCEL;
    }

    char major;
    char minor;
    fs >> major >> minor;
    const uint16_t savid = ( static_cast<uint16_t>( major ) << 8 ) | static_cast<uint16_t>( minor );

    // check version sav file
    if ( savid != SAV2ID2 && savid != SAV2ID3 ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, filePath << ", incorrect SAV2ID" )
        return fheroes2::GameMode::CANCEL;
    }

    std::string strver;
    uint16_t binver = 0;

    // read raw info
    fs >> strver >> binver;

    // hide: unsupported version
    if ( binver > CURRENT_FORMAT_VERSION || binver < LAST_SUPPORTED_FORMAT_VERSION )
        return fheroes2::GameMode::CANCEL;

    int fileGameType = Game::TYPE_STANDARD;
    HeaderSAV header;
    fs >> header;
    fileGameType = header.gameType;

    size_t offset = fs.tell();
    fs.close();

    Settings & conf = Settings::Get();
    if ( ( conf.GameType() & fileGameType ) == 0 ) {
        fheroes2::showMessage( fheroes2::Text( _( "Warning" ), fheroes2::FontType::normalYellow() ),
                               fheroes2::Text( _( "This file contains a save with an invalid game type." ), fheroes2::FontType::normalWhite() ), Dialog::OK );
        return fheroes2::GameMode::CANCEL;
    }

    ZStreamFile fz;
    fz.setbigendian( true );

    if ( !fz.read( filePath, offset ) ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, ", uncompress: error" )
        return fheroes2::GameMode::CANCEL;
    }

    if ( ( header.status & HeaderSAV::IS_PRICE_OF_LOYALTY_MAP ) && !conf.isPriceOfLoyaltySupported() ) {
        fheroes2::showStandardTextMessage(
            _( "Warning" ), _( "This file was saved for a \"The Price of Loyalty\" map, but the corresponding game assets have not been provided to the engine." ),
            Dialog::OK );
        return fheroes2::GameMode::CANCEL;
    }

    fz >> binver;

    // check version: false
    if ( binver > CURRENT_FORMAT_VERSION || binver < LAST_SUPPORTED_FORMAT_VERSION ) {
        std::string errorMessage( _( "Unsupported save format: " ) );
        errorMessage += std::to_string( binver );
        errorMessage += ".\n";
        errorMessage += _( "Current game version: " );
        errorMessage += std::to_string( CURRENT_FORMAT_VERSION );
        errorMessage += ".\n";
        errorMessage += _( "Last supported version: " );
        errorMessage += std::to_string( LAST_SUPPORTED_FORMAT_VERSION );
        errorMessage += ".\n";

        fheroes2::showMessage( fheroes2::Text( _( "Error" ), fheroes2::FontType::normalYellow() ), fheroes2::Text( errorMessage, fheroes2::FontType::normalWhite() ),
                               Dialog::OK );

        return fheroes2::GameMode::CANCEL;
    }

    DEBUG_LOG( DBG_GAME, DBG_TRACE, "load version: " << binver )
    SetLoadVersion( binver );

    fz >> World::Get() >> conf >> GameOver::Result::Get();

    // Settings should contain the full path to the current map file, if this map is available
    conf.SetMapsFile( Settings::GetLastFile( "maps", System::GetBasename( conf.MapsFile() ) ) );

    if ( !conf.loadedFileLanguage().empty() && conf.loadedFileLanguage() != "en" && conf.loadedFileLanguage() != conf.getGameLanguage() ) {
        std::string warningMessage( _( "This saved game is localized to '" ) );
        warningMessage.append( fheroes2::getLanguageName( fheroes2::getLanguageFromAbbreviation( conf.loadedFileLanguage() ) ) );
        warningMessage.append( _( "' language, but the current language of the game is '" ) );
        warningMessage.append( fheroes2::getLanguageName( fheroes2::getLanguageFromAbbreviation( conf.getGameLanguage() ) ) );
        warningMessage += "'.";

        fheroes2::showMessage( fheroes2::Text( _( "Warning" ), fheroes2::FontType::normalYellow() ), fheroes2::Text( warningMessage, fheroes2::FontType::normalWhite() ),
                               Dialog::OK );
    }

    fheroes2::GameMode returnValue = fheroes2::GameMode::START_GAME;

    if ( conf.isCampaignGameType() ) {
        Campaign::CampaignSaveData & saveData = Campaign::CampaignSaveData::Get();
        fz >> saveData;

        if ( !saveData.isStarting() && saveData.getCurrentScenarioInfoId() == saveData.getLastCompletedScenarioInfoID() ) {
            // This is the end of the current scenario. We should show next scenario selection.
            returnValue = fheroes2::GameMode::COMPLETE_CAMPAIGN_SCENARIO_FROM_LOAD_FILE;
        }
    }

    uint16_t end_check = 0;
    fz >> end_check;

    if ( fz.fail() || ( end_check != SAV2ID2 && end_check != SAV2ID3 ) ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, "invalid load file: " << filePath )
        return fheroes2::GameMode::CANCEL;
    }

    SetLoadVersion( CURRENT_FORMAT_VERSION );

    Game::SetLastSavename( filePath );
    conf.SetGameType( conf.GameType() | Game::TYPE_LOADFILE );

    return returnValue;
}

bool Game::LoadSAV2FileInfo( const std::string & filePath, Maps::FileInfo & fileInfo )
{
    DEBUG_LOG( DBG_GAME, DBG_INFO, filePath )

    StreamFile fs;
    fs.setbigendian( true );

    if ( !fs.open( filePath, "rb" ) ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, filePath << ", error open" )
        return false;
    }

    char major;
    char minor;
    fs >> major >> minor;
    const uint16_t savid = ( static_cast<uint16_t>( major ) << 8 ) | static_cast<uint16_t>( minor );

    // check version sav file
    if ( savid != SAV2ID2 && savid != SAV2ID3 ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, filePath << ", incorrect SAV2ID" )
        return false;
    }

    std::string strver;
    uint16_t binver = 0;

    // read raw info
    fs >> strver >> binver;

    // hide: unsupported version
    if ( binver > CURRENT_FORMAT_VERSION || binver < LAST_SUPPORTED_FORMAT_VERSION )
        return false;

    int fileGameType = Game::TYPE_STANDARD;
    HeaderSAV header;
    fs >> header;
    fileGameType = header.gameType;

    if ( ( Settings::Get().GameType() & fileGameType ) == 0 )
        return false;

    fileInfo = header.info;
    fileInfo.file = filePath;

    return true;
}

std::string Game::GetSaveDir()
{
    return System::concatPath( System::concatPath( System::GetDataDirectory( "fheroes2" ), "files" ), "save" );
}

std::string Game::GetSaveFileBaseName()
{
    std::string baseName = Settings::Get().CurrentFileInfo().name;

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
