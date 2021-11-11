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
#include "audio.h"
#include "bin_info.h"
#include "core.h"
#include "cursor.h"
#include "embedded_image.h"
#include "game.h"
#include "game_logo.h"
#include "game_video.h"
#include "localevent.h"
#include "logging.h"
#include "screen.h"
#include "settings.h"
#include "system.h"
#ifndef BUILD_RELEASE
#include "tools.h"
#endif
#include "ui_tool.h"
#include "zzlib.h"

namespace
{
    const char * configurationFileName = "fheroes2.cfg";

    std::string GetCaption()
    {
        return std::string( "Free Heroes of Might and Magic II, version: " + Settings::GetVersion() );
    }

    int PrintHelp( const char * basename )
    {
        COUT( "Usage: " << basename << " [OPTIONS]" );
#ifndef BUILD_RELEASE
        COUT( "  -d <level>\tprint debug messages, see src/engine/logging.h for possible values of <level> argument" );
#endif
        COUT( "  -h\t\tprint this help message and exit" );

        return EXIT_SUCCESS;
    }

    void ReadConfigs()
    {
        Settings & conf = Settings::Get();

        const std::string confFile = Settings::GetLastFile( "", configurationFileName );

        if ( System::IsFile( confFile ) && conf.Read( confFile ) ) {
            LocalEvent::Get().SetControllerPointerSpeed( conf.controllerPointerSpeed() );
        }
        else {
            conf.Save( configurationFileName );
        }
    }

    void InitConfigDir()
    {
        const std::string configDir = System::GetConfigDirectory( "fheroes2" );

        if ( !configDir.empty() && !System::IsDirectory( configDir ) ) {
            System::MakeDirectory( configDir );
        }
    }

    void InitDataDir()
    {
        const std::string dataDir = System::GetDataDirectory( "fheroes2" );

        if ( dataDir.empty() )
            return;

        const std::string dataFiles = System::ConcatePath( dataDir, "files" );
        const std::string dataFilesSave = System::ConcatePath( dataFiles, "save" );

        if ( !System::IsDirectory( dataDir ) )
            System::MakeDirectory( dataDir );

        if ( System::IsDirectory( dataDir, true ) && !System::IsDirectory( dataFiles ) )
            System::MakeDirectory( dataFiles );

        if ( System::IsDirectory( dataFiles, true ) && !System::IsDirectory( dataFilesSave ) )
            System::MakeDirectory( dataFilesSave );
    }

    class DisplayInitializer
    {
    public:
        DisplayInitializer()
        {
            const Settings & conf = Settings::Get();

            fheroes2::Display & display = fheroes2::Display::instance();
            if ( conf.FullScreen() != fheroes2::engine().isFullScreen() )
                fheroes2::engine().toggleFullScreen();

            display.resize( conf.VideoMode().width, conf.VideoMode().height );
            display.fill( 0 ); // start from a black screen

            fheroes2::engine().setTitle( GetCaption() );

            SDL_ShowCursor( SDL_DISABLE ); // hide system cursor

            // Initialize local event processing.
            LocalEvent::Get().RegisterCycling( fheroes2::PreRenderSystemInfo, fheroes2::PostRenderSystemInfo );

            // Update mouse cursor when switching between software emulation and OS mouse modes.
            fheroes2::cursor().registerUpdater( Cursor::Refresh );

            const fheroes2::Image & appIcon = CreateImageFromZlib( 32, 32, iconImage, sizeof( iconImage ), true );
            fheroes2::engine().setIcon( appIcon );
        }

        DisplayInitializer( const DisplayInitializer & ) = delete;
        DisplayInitializer & operator=( const DisplayInitializer & ) = delete;

        ~DisplayInitializer()
        {
            fheroes2::Display::instance().release();
        }
    };
}

#if defined( _MSC_VER )
#undef main
#endif

int main( int argc, char ** argv )
{
    try {
        const fheroes2::HardwareInitializer hardwareInitializer;
        Logging::InitLog();

        DEBUG_LOG( DBG_ALL, DBG_INFO, GetCaption() );

        Settings & conf = Settings::Get();
        conf.SetProgramPath( argv[0] );

        InitConfigDir();
        InitDataDir();
        ReadConfigs();

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

        std::set<fheroes2::SystemInitializationComponent> coreComponents{ fheroes2::SystemInitializationComponent::Audio,
                                                                          fheroes2::SystemInitializationComponent::Video };

#if defined( FHEROES2_VITA ) || defined( __SWITCH__ )
        coreComponents.emplace( fheroes2::SystemInitializationComponent::GameController );
#endif

        const fheroes2::CoreInitializer coreInitializer( coreComponents );

        if ( Audio::isValid() ) {
            Mixer::SetChannels( 16 );
            Mixer::Volume( -1, Mixer::MaxVolume() * conf.SoundVolume() / 10 );

            Music::Volume( Mixer::MaxVolume() * conf.MusicVolume() / 10 );
            Music::SetFadeIn( 900 );
        }

        DEBUG_LOG( DBG_GAME, DBG_INFO, conf.String() );

        const DisplayInitializer displayInitializer;

        const AGG::AGGInitializer aggInitializer;

        // load BIN data
        Bin_Info::InitBinInfo();

        // init game data
        Game::Init();

        conf.setGameLanguage( conf.getGameLanguage() );

        if ( conf.isShowIntro() ) {
            fheroes2::showTeamInfo();

            Video::ShowVideo( "H2XINTRO.SMK", Video::VideoAction::PLAY_TILL_VIDEO_END );
        }

        // init cursor
        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        Game::mainGameLoop( conf.isFirstGameRun() );
    }
    catch ( const std::exception & ex ) {
        ERROR_LOG( "Exception '" << ex.what() << "' occured during application runtime." );
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
