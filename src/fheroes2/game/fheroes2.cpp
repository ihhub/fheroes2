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

#include <cstdlib>
#include <iostream>
#include <string>

#include "agg.h"
#include "audio_music.h"
#include "bin_info.h"
#include "cursor.h"
#include "dir.h"
#include "embedded_image.h"
#include "engine.h"
#include "error.h"
#include "game.h"
#include "game_interface.h"
#include "game_video.h"
#include "gamedefs.h"
#include "screen.h"
#include "settings.h"
#include "system.h"
#include "zzlib.h"

void SetVideoDriver( const std::string & );
void SetTimidityEnvPath();
void SetLangEnvPath( const Settings & );
void InitHomeDir( void );
bool ReadConfigs( void );

int PrintHelp( const char * basename )
{
    COUT( "Usage: " << basename << " [OPTIONS]" );
#ifndef BUILD_RELEASE
    COUT( "  -d\tdebug mode" );
#endif
    COUT( "  -h\tprint this help and exit" );

    return EXIT_SUCCESS;
}

std::string GetCaption( void )
{
    return std::string( "Free Heroes of Might and Magic II, version: " + Settings::GetVersion() );
}

#if defined( _MSC_VER )
#undef main
#endif

int main( int argc, char ** argv )
{
    Settings & conf = Settings::Get();

    DEBUG( DBG_ALL, DBG_INFO, "Free Heroes of Might and Magic II, " + conf.GetVersion() );

    conf.SetProgramPath( argv[0] );

    InitHomeDir();
    bool isFirstGameRun = ReadConfigs();

    // getopt
    {
        int opt;
        while ( ( opt = System::GetCommandOptions( argc, argv, "hd:" ) ) != -1 )
            switch ( opt ) {
#ifndef BUILD_RELEASE
            case 'd':
                conf.SetDebug( System::GetOptionsArgument() ? GetInt( System::GetOptionsArgument() ) : 0 );
                break;
#endif
            case '?':
            case 'h':
                return PrintHelp( argv[0] );

            default:
                break;
            }
    }

    if ( conf.SelectVideoDriver().size() )
        SetVideoDriver( conf.SelectVideoDriver() );

    // random init
    if ( conf.Music() )
        SetTimidityEnvPath();

    u32 subsystem = INIT_VIDEO | INIT_TIMER;

    if ( conf.Sound() || conf.Music() )
        subsystem |= INIT_AUDIO;
#ifdef WITH_AUDIOCD
    if ( conf.MusicCD() )
        subsystem |= INIT_CDROM | INIT_AUDIO;
#endif
    if ( SDL::Init( subsystem ) )
#ifndef ANDROID
        try
#endif
        {
            std::atexit( SDL::Quit );

            SetLangEnvPath( conf );

            if ( Mixer::isValid() ) {
                Mixer::SetChannels( 16 );
                Mixer::Volume( -1, Mixer::MaxVolume() * conf.SoundVolume() / 10 );
                Music::Volume( Mixer::MaxVolume() * conf.MusicVolume() / 10 );
                if ( conf.Music() ) {
                    Music::SetFadeIn( 900 );
                }
            }
            else if ( conf.Sound() || conf.Music() ) {
                conf.ResetSound();
                conf.ResetMusic();
            }

            fheroes2::Display & display = fheroes2::Display::instance();
            if ( conf.FullScreen() != fheroes2::engine().isFullScreen() )
                fheroes2::engine().toggleFullScreen();

            display.resize( conf.VideoMode().w, conf.VideoMode().h );
            fheroes2::engine().setTitle( GetCaption() );

            SDL_ShowCursor( SDL_DISABLE ); // hide system cursor

            // Ensure the mouse position is updated to prevent bad initial values.
            LocalEvent::Get().RegisterCycling( fheroes2::PreRenderSystemInfo, fheroes2::PostRenderSystemInfo );
            LocalEvent::Get().GetMouseCursor();

#ifdef WITH_ZLIB
            const fheroes2::Image & appIcon = CreateImageFromZlib( 32, 32, iconImageLayer, sizeof( iconImageLayer ), iconTransformLayer, sizeof( iconTransformLayer ) );
            fheroes2::engine().setIcon( appIcon );
#endif

            DEBUG( DBG_GAME, DBG_INFO, conf.String() );
            // DEBUG( DBG_GAME | DBG_ENGINE, DBG_INFO, display.GetInfo() );

            // read data dir
            if ( !AGG::Init() ) {
                fheroes2::Display::instance().release();
                return EXIT_FAILURE;
            }

            atexit( &AGG::Quit );

            // load BIN data
            Bin_Info::InitBinInfo();

            // init cursor
            Cursor::Get().SetThemes( Cursor::POINTER );

            // init game data
            Game::Init();

            Video::ShowVideo( Settings::GetLastFile( System::ConcatePath( "heroes2", "anim" ), "H2XINTRO.SMK" ), false );

            for ( int rs = Game::MAINMENU; rs != Game::QUITGAME; ) {
                switch ( rs ) {
                case Game::MAINMENU:
                    rs = Game::MainMenu( isFirstGameRun );
                    isFirstGameRun = false;
                    break;
                case Game::NEWGAME:
                    rs = Game::NewGame();
                    break;
                case Game::LOADGAME:
                    rs = Game::LoadGame();
                    break;
                case Game::HIGHSCORES:
                    rs = Game::HighScores();
                    break;
                case Game::CREDITS:
                    rs = Game::Credits();
                    break;
                case Game::NEWSTANDARD:
                    rs = Game::NewStandard();
                    break;
                case Game::NEWCAMPAIN:
                    rs = Game::NewCampain();
                    break;
                case Game::NEWMULTI:
                    rs = Game::NewMulti();
                    break;
                case Game::NEWHOTSEAT:
                    rs = Game::NewHotSeat();
                    break;
#ifdef NETWORK_ENABLE
                case Game::NEWNETWORK:
                    rs = Game::NewNetwork();
                    break;
#endif
                case Game::NEWBATTLEONLY:
                    rs = Game::NewBattleOnly();
                    break;
                case Game::LOADSTANDARD:
                    rs = Game::LoadStandard();
                    break;
                case Game::LOADCAMPAIN:
                    rs = Game::LoadCampain();
                    break;
                case Game::LOADMULTI:
                    rs = Game::LoadMulti();
                    break;
                case Game::LOADHOTSEAT:
                    rs = Game::LoadHotseat();
                    break;
                case Game::LOADNETWORK:
                    rs = Game::LoadNetwork();
                    break;
                case Game::SCENARIOINFO:
                    rs = Game::ScenarioInfo();
                    break;
                case Game::SELECTSCENARIO:
                    rs = Game::SelectScenario();
                    break;
                case Game::STARTGAME:
                    rs = Game::StartGame();
                    break;

                default:
                    break;
                }
            }
        }
#ifndef ANDROID
        catch ( Error::Exception & ) {
            VERBOSE( std::endl << conf.String() );
        }
#endif
    fheroes2::Display::instance().release();

    return EXIT_SUCCESS;
}

bool ReadConfigs( void )
{
    Settings & conf = Settings::Get();
    const ListFiles & files = conf.GetListFiles( "", "fheroes2.cfg" );

    bool isValidConfigurationFile = false;
    for ( ListFiles::const_iterator it = files.begin(); it != files.end(); ++it ) {
        if ( System::IsFile( *it ) ) {
            if ( conf.Read( *it ) ) {
                isValidConfigurationFile = true;
                break;
            }
        }
    }

    if ( !isValidConfigurationFile )
        conf.Save( "fheroes2.cfg" );

    return !isValidConfigurationFile;
}

void InitHomeDir( void )
{
    const std::string home = System::GetHomeDirectory( "fheroes2" );

    if ( !home.empty() ) {
        const std::string home_maps = System::ConcatePath( home, "maps" );
        const std::string home_files = System::ConcatePath( home, "files" );
        const std::string home_files_save = System::ConcatePath( home_files, "save" );

        if ( !System::IsDirectory( home ) )
            System::MakeDirectory( home );

        if ( System::IsDirectory( home, true ) && !System::IsDirectory( home_maps ) )
            System::MakeDirectory( home_maps );

        if ( System::IsDirectory( home, true ) && !System::IsDirectory( home_files ) )
            System::MakeDirectory( home_files );

        if ( System::IsDirectory( home_files, true ) && !System::IsDirectory( home_files_save ) )
            System::MakeDirectory( home_files_save );
    }
}

void SetVideoDriver( const std::string & driver )
{
    System::SetEnvironment( "SDL_VIDEODRIVER", driver.c_str() );
}

void SetTimidityEnvPath()
{
    const std::string prefix_timidity = System::ConcatePath( "files", "timidity" );
    const std::string result = Settings::GetLastFile( prefix_timidity, "timidity.cfg" );

    if ( System::IsFile( result ) )
        System::SetEnvironment( "TIMIDITY_PATH", System::GetDirname( result ).c_str() );
}

void SetLangEnvPath( const Settings & conf )
{
#ifdef WITH_TTF
    if ( conf.Unicode() ) {
        System::SetLocale( LC_ALL, "" );
        System::SetLocale( LC_NUMERIC, "C" );

        std::string mofile = conf.ForceLang().empty() ? System::GetMessageLocale( 1 ).append( ".mo" ) : std::string( conf.ForceLang() ).append( ".mo" );

        ListFiles translations = Settings::GetListFiles( System::ConcatePath( "files", "lang" ), mofile );

        if ( translations.size() ) {
            if ( Translation::bindDomain( "fheroes2", translations.back().c_str() ) )
                Translation::setDomain( "fheroes2" );
        }
        else
            ERROR( "translation not found: " << mofile );
    }
#else
    (void)conf;
#endif
    Translation::setStripContext( '|' );
}
