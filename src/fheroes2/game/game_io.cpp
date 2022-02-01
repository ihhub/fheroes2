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

#include <ctime>

#include "campaign_savedata.h"
#include "dialog.h"
#include "game.h"
#include "game_io.h"
#include "game_over.h"
#include "game_static.h"
#include "logging.h"
#include "monster.h"
#include "save_format_version.h"
#include "settings.h"
#include "system.h"
#include "text.h"
#include "translations.h"
#include "ui_language.h"
#include "world.h"
#include "zzlib.h"

namespace
{
    const uint16_t SAV2ID2 = 0xFF02;
    const uint16_t SAV2ID3 = 0xFF03;

    struct HeaderSAV
    {
        enum
        {
            IS_LOYALTY = 0x4000
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
                status |= IS_LOYALTY;
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
    return Game::Save( System::ConcatePath( GetSaveDir(), "AUTOSAVE" + GetSaveFileExtension() ) );
}

bool Game::Save( const std::string & fn )
{
    DEBUG_LOG( DBG_GAME, DBG_INFO, fn );
    const bool autosave = ( System::GetBasename( fn ) == "AUTOSAVE" + GetSaveFileExtension() );
    const Settings & conf = Settings::Get();

    StreamFile fs;
    fs.setbigendian( true );

    if ( !fs.open( fn, "wb" ) ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, fn << ", error open" );
        return false;
    }

    u16 loadver = GetLoadVersion();
    if ( !autosave )
        Game::SetLastSavename( fn );

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

    return !fz.fail() && fz.write( fn, true );
}

fheroes2::GameMode Game::Load( const std::string & fn )
{
    DEBUG_LOG( DBG_GAME, DBG_INFO, fn );

    StreamFile fs;
    fs.setbigendian( true );

    if ( !fs.open( fn, "rb" ) ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, fn << ", error open" );
        return fheroes2::GameMode::CANCEL;
    }

    Game::ShowMapLoadingText();

    char major;
    char minor;
    fs >> major >> minor;
    const u16 savid = ( static_cast<u16>( major ) << 8 ) | static_cast<u16>( minor );

    // check version sav file
    if ( savid != SAV2ID2 && savid != SAV2ID3 ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, fn << ", incorrect SAV2ID" );
        return fheroes2::GameMode::CANCEL;
    }

    std::string strver;
    u16 binver = 0;

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
        Dialog::Message( _( "Warning" ), _( "Invalid file game type. Please ensure that you are running the latest type of save files." ), Font::BIG, Dialog::OK );
        return fheroes2::GameMode::CANCEL;
    }

    ZStreamFile fz;
    fz.setbigendian( true );

    if ( !fz.read( fn, offset ) ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, ", uncompress: error" );
        return fheroes2::GameMode::CANCEL;
    }

    if ( ( header.status & HeaderSAV::IS_LOYALTY ) && !conf.isPriceOfLoyaltySupported() ) {
        Dialog::Message( _( "Warning" ), _( "This file is saved in the \"The Price of Loyalty\" version.\nSome items may be unavailable." ), Font::BIG, Dialog::OK );
    }

    fz >> binver;

    // check version: false
    if ( binver > CURRENT_FORMAT_VERSION || binver < LAST_SUPPORTED_FORMAT_VERSION ) {
        std::string errorMessage( _( "Usupported save format: " ) );
        errorMessage += std::to_string( binver );
        errorMessage += ".\n";
        errorMessage += _( "Current game version: " );
        errorMessage += std::to_string( CURRENT_FORMAT_VERSION );
        errorMessage += ".\n";
        errorMessage += _( "Last supported version: " );
        errorMessage += std::to_string( LAST_SUPPORTED_FORMAT_VERSION );
        errorMessage += ".\n";

        Dialog::Message( _( "Error" ), errorMessage, Font::BIG, Dialog::OK );
        return fheroes2::GameMode::CANCEL;
    }

    DEBUG_LOG( DBG_GAME, DBG_TRACE, "load version: " << binver );
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
        Dialog::Message( _( "Warning" ), warningMessage, Font::BIG, Dialog::OK );
    }

    fheroes2::GameMode returnValue = fheroes2::GameMode::START_GAME;

    if ( conf.isCampaignGameType() ) {
        Campaign::CampaignSaveData & saveData = Campaign::CampaignSaveData::Get();
        static_assert( LAST_SUPPORTED_FORMAT_VERSION < FORMAT_VERSION_0912_RELEASE, "Remove the usage of loadOldSaveSata method." );
        if ( binver < FORMAT_VERSION_0912_RELEASE ) {
            Campaign::CampaignSaveData::loadOldSaveSata( fz, saveData );
        }
        else {
            fz >> saveData;
        }

        if ( !saveData.isStarting() && Campaign::ScenarioInfoId{ saveData.getCampaignID(), saveData.getCurrentScenarioID() } == saveData.getLastCompletedScenarioInfoID() ) {
            // This is the end of the current scenario. We should show next scenario selection.
            returnValue = fheroes2::GameMode::COMPLETE_CAMPAIGN_SCENARIO_FROM_LOAD_FILE;
        }
    }

    uint16_t end_check = 0;
    fz >> end_check;

    if ( fz.fail() || ( end_check != SAV2ID2 && end_check != SAV2ID3 ) ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, "invalid load file: " << fn );
        return fheroes2::GameMode::CANCEL;
    }

    SetLoadVersion( CURRENT_FORMAT_VERSION );

    Game::SetLastSavename( fn );
    conf.SetGameType( conf.GameType() | Game::TYPE_LOADFILE );

    if ( returnValue != fheroes2::GameMode::START_GAME ) {
        return returnValue;
    }

    return returnValue;
}

bool Game::LoadSAV2FileInfo( const std::string & fn, Maps::FileInfo & finfo )
{
    DEBUG_LOG( DBG_GAME, DBG_INFO, fn );

    StreamFile fs;
    fs.setbigendian( true );

    if ( !fs.open( fn, "rb" ) ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, fn << ", error open" );
        return false;
    }

    char major;
    char minor;
    fs >> major >> minor;
    const u16 savid = ( static_cast<u16>( major ) << 8 ) | static_cast<u16>( minor );

    // check version sav file
    if ( savid != SAV2ID2 && savid != SAV2ID3 ) {
        DEBUG_LOG( DBG_GAME, DBG_WARN, fn << ", incorrect SAV2ID" );
        return false;
    }

    std::string strver;
    u16 binver = 0;

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

    finfo = header.info;
    finfo.file = fn;

    return true;
}

std::string Game::GetSaveDir()
{
    return System::ConcatePath( System::ConcatePath( System::GetDataDirectory( "fheroes2" ), "files" ), "save" );
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
    const std::string & name = Settings::Get().CurrentFileInfo().name;

    std::string base = name.empty() ? "newgame" : name;
    std::replace_if( base.begin(), base.end(), ::isspace, '_' );

    return Save( System::ConcatePath( Game::GetSaveDir(), base ) + "_Complete" + Game::GetSaveFileExtension() );
}
