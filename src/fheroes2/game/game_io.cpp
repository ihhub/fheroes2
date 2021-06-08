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

#include <cstring>
#include <ctime>
#include <sstream>

#include "army.h"
#include "campaign_savedata.h"
#include "castle.h"
#include "dialog.h"
#include "game.h"
#include "game_io.h"
#include "game_over.h"
#include "game_static.h"
#include "heroes.h"
#include "interface_gamearea.h"
#include "kingdom.h"
#include "logging.h"
#include "monster.h"
#include "system.h"
#include "text.h"
#include "tools.h"
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
            IS_COMPRESS = 0x8000,
            IS_LOYALTY = 0x4000
        };

        HeaderSAV() = delete;

        explicit HeaderSAV( const int saveFileVersion )
            : status( 0 )
            , gameType( 0 )
            , _saveFileVersion( saveFileVersion )
        {}

        HeaderSAV( const Maps::FileInfo & fi, const int gameType_, const int saveFileVersion )
            : status( 0 )
            , info( fi )
            , gameType( gameType_ )
            , _saveFileVersion( saveFileVersion )
        {
            time_t rawtime;
            std::time( &rawtime );
            info.localtime = rawtime;

            if ( fi._version == GameVersion::PRICE_OF_LOYALTY )
                status |= IS_LOYALTY;

#ifdef WITH_ZLIB
            status |= IS_COMPRESS;
#endif
        }

        uint16_t status;
        Maps::FileInfo info;
        int gameType;
        const int _saveFileVersion;
    };

    StreamBase & operator<<( StreamBase & msg, const HeaderSAV & hdr )
    {
        return msg << hdr.status << hdr.info << hdr.gameType;
    }

    StreamBase & operator>>( StreamBase & msg, HeaderSAV & hdr )
    {
        if ( hdr._saveFileVersion < FORMAT_VERSION_094_RELEASE ) {
            msg >> hdr.status >> hdr.info >> hdr.gameType;
        }
        else {
            msg >> hdr.status >> hdr.info;
            msg >> hdr.info._version;
            msg >> hdr.gameType;
        }

        return msg;
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
        DEBUG_LOG( DBG_GAME, DBG_INFO, fn << ", error open" );
        return false;
    }

    u16 loadver = GetLoadVersion();
    if ( !autosave )
        Game::SetLastSavename( fn );

    // raw info content
    fs << static_cast<uint8_t>( SAV2ID3 >> 8 ) << static_cast<uint8_t>( SAV2ID3 & 0xFF ) << std::to_string( loadver ) << loadver
       << HeaderSAV( conf.CurrentFileInfo(), conf.GameType(), CURRENT_FORMAT_VERSION );
    fs.close();

    ZStreamFile fz;
    fz.setbigendian( true );

    // zip game data content
    fz << loadver << World::Get() << Settings::Get() << GameOver::Result::Get() << GameStatic::Data::Get() << MonsterStaticData::Get();

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
        DEBUG_LOG( DBG_GAME, DBG_INFO, fn << ", error open" );
        return fheroes2::GameMode::CANCEL;
    }

    Game::ShowMapLoadingText();

    char major;
    char minor;
    fs >> major >> minor;
    const u16 savid = ( static_cast<u16>( major ) << 8 ) | static_cast<u16>( minor );

    // check version sav file
    if ( savid != SAV2ID2 && savid != SAV2ID3 ) {
        DEBUG_LOG( DBG_GAME, DBG_INFO, fn << ", incorrect SAV2ID" );
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
    HeaderSAV header( binver );
    fs >> header;
    fileGameType = header.gameType;

    size_t offset = fs.tell();
    fs.close();

    Settings & conf = Settings::Get();
    if ( ( conf.GameType() & fileGameType ) == 0 ) {
        Dialog::Message( "Warning", _( "Invalid file game type. Please ensure that you are running the latest type of save files." ), Font::BIG, Dialog::OK );
        return fheroes2::GameMode::CANCEL;
    }

#ifndef WITH_ZLIB
    if ( header.status & HeaderSAV::IS_COMPRESS ) {
        DEBUG_LOG( DBG_GAME, DBG_INFO, fn << ", zlib: unsupported" );
        return fheroes2::GameMode::CANCEL;
    }
#endif

    ZStreamFile fz;
    fz.setbigendian( true );

    if ( !fz.read( fn, offset ) ) {
        DEBUG_LOG( DBG_GAME, DBG_INFO, ", uncompress: error" );
        return fheroes2::GameMode::CANCEL;
    }

    if ( ( header.status & HeaderSAV::IS_LOYALTY ) && !conf.isPriceOfLoyaltySupported() ) {
        Dialog::Message( "Warning", _( "This file is saved in the \"The Price of Loyalty\" version.\nSome items may be unavailable." ), Font::BIG, Dialog::OK );
    }

    fz >> binver;

    // check version: false
    if ( binver > CURRENT_FORMAT_VERSION || binver < LAST_SUPPORTED_FORMAT_VERSION ) {
        std::ostringstream os;
        os << "usupported save format: " << binver << std::endl
           << "game version: " << CURRENT_FORMAT_VERSION << std::endl
           << "last supported version: " << LAST_SUPPORTED_FORMAT_VERSION;
        Dialog::Message( "Error", os.str(), Font::BIG, Dialog::OK );
        return fheroes2::GameMode::CANCEL;
    }

    DEBUG_LOG( DBG_GAME, DBG_TRACE, "load version: " << binver );
    SetLoadVersion( binver );

    fz >> World::Get() >> conf >> GameOver::Result::Get() >> GameStatic::Data::Get() >> MonsterStaticData::Get();
    if ( conf.loadedFileLanguage() != "en" && conf.loadedFileLanguage() != conf.ForceLang() && !conf.Unicode() ) {
        std::string warningMessage( "This is an saved game is localized for lang = " );
        warningMessage.append( conf.loadedFileLanguage() );
        warningMessage.append( ", and most of the messages will be displayed incorrectly.\n \n" );
        warningMessage.append( "(tip: set unicode = on)" );
        Dialog::Message( "Warning!", warningMessage, Font::BIG, Dialog::OK );
    }

    fheroes2::GameMode returnValue = fheroes2::GameMode::START_GAME;

    if ( conf.isCampaignGameType() ) {
        Campaign::CampaignSaveData & saveData = Campaign::CampaignSaveData::Get();
        fz >> saveData;

        if ( !saveData.isStarting() && saveData.getCurrentScenarioID() == saveData.getLastCompletedScenarioID() ) {
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

    // rescan path passability for all heroes, for this we need actual info about players from Settings
    World::Get().RescanAllHeroesPathPassable();

    return returnValue;
}

bool Game::LoadSAV2FileInfo( const std::string & fn, Maps::FileInfo & finfo )
{
    StreamFile fs;
    fs.setbigendian( true );

    if ( !fs.open( fn, "rb" ) ) {
        DEBUG_LOG( DBG_GAME, DBG_INFO, fn << ", error open" );
        return false;
    }

    char major, minor;
    fs >> major >> minor;
    const u16 savid = ( static_cast<u16>( major ) << 8 ) | static_cast<u16>( minor );

    // check version sav file
    if ( savid != SAV2ID2 && savid != SAV2ID3 ) {
        DEBUG_LOG( DBG_GAME, DBG_INFO, fn << ", incorrect SAV2ID" );
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
    HeaderSAV header( binver );
    fs >> header;
    fileGameType = header.gameType;

    if ( ( Settings::Get().GameType() & fileGameType ) == 0 )
        return false;

#ifndef WITH_ZLIB
    // check: compress game data
    if ( header.status & HeaderSAV::IS_COMPRESS ) {
        DEBUG_LOG( DBG_GAME, DBG_INFO, fn << ", zlib: unsupported" );
        return false;
    }
#endif

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
