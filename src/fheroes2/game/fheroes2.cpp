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

#include <cstdlib>
#include <exception>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <string>

#include <SDL_events.h>
#include <SDL_main.h> // IWYU pragma: keep
#include <SDL_mouse.h>
#include <SDL_version.h>

#if defined( _WIN32 )

#if SDL_VERSION_ATLEAST( 2, 0, 0 )
#include <cassert>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#endif

#include "agg.h"
#include "audio_manager.h"
#include "bin_info.h"
#include "core.h"
#include "cursor.h"
#include "dir.h"
#include "embedded_image.h"
#include "game.h"
#include "game_logo.h"
#include "game_video.h"
#include "game_video_type.h"
#include "h2d.h"
#include "image.h"
#include "image_palette.h"
#include "localevent.h"
#include "logging.h"
#include "math_base.h"
#include "screen.h"
#include "settings.h"
#include "system.h"
#include "ui_tool.h"
#include "zzlib.h"

namespace
{
    std::string GetCaption()
    {
        return std::string( "fheroes2 engine, version: " + Settings::GetVersion() );
    }

    void ReadConfigs()
    {
        const std::string configurationFileName( Settings::configFileName );
        const std::string confFile = Settings::GetLastFile( "", configurationFileName );

        Settings & conf = Settings::Get();
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

        if ( !System::IsDirectory( configDir ) ) {
            System::MakeDirectory( configDir );
        }
    }

    void InitDataDir()
    {
        const std::string dataDir = System::GetDataDirectory( "fheroes2" );

        if ( dataDir.empty() )
            return;

        const std::string dataFiles = System::concatPath( dataDir, "files" );
        const std::string dataFilesSave = System::concatPath( dataFiles, "save" );

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
            fheroes2::engine().setPosition(conf.WindowPosition());

            display.resize( conf.VideoMode().width, conf.VideoMode().height );
            display.fill( 0 ); // start from a black screen

            fheroes2::engine().setTitle( GetCaption() );

            SDL_ShowCursor( SDL_DISABLE ); // hide system cursor

            // Initialize local event processing.
            LocalEvent::RegisterCycling( fheroes2::PreRenderSystemInfo, fheroes2::PostRenderSystemInfo );

            // Update mouse cursor when switching between software emulation and OS mouse modes.
            fheroes2::cursor().registerUpdater( Cursor::Refresh );

#if !defined( MACOS_APP_BUNDLE )
            const fheroes2::Image & appIcon = CreateImageFromZlib( 32, 32, iconImage, sizeof( iconImage ), true );
            fheroes2::engine().setIcon( appIcon );
#endif
        }

        DisplayInitializer( const DisplayInitializer & ) = delete;
        DisplayInitializer & operator=( const DisplayInitializer & ) = delete;

        ~DisplayInitializer()
        {
            fheroes2::Display::instance().release();
        }
    };

    class DataInitializer
    {
    public:
        DataInitializer()
        {
            const fheroes2::ScreenPaletteRestorer screenRestorer;

            try {
                _aggInitializer.reset( new AGG::AGGInitializer );

                _h2dInitializer.reset( new fheroes2::h2d::H2DInitializer );
            }
            catch ( ... ) {
                fheroes2::Display & display = fheroes2::Display::instance();
                const fheroes2::Image & image = CreateImageFromZlib( 290, 190, errorMessage, sizeof( errorMessage ), false );

                display.fill( 0 );
                fheroes2::Resize( image, display );

                display.render();

                LocalEvent & le = LocalEvent::Get();
                while ( le.HandleEvents() && !le.KeyPress() && !le.MouseClickLeft() ) {
                    // Do nothing.
                }

                throw;
            }
        }

        DataInitializer( const DataInitializer & ) = delete;
        DataInitializer & operator=( const DataInitializer & ) = delete;
        ~DataInitializer() = default;

        const std::string & getOriginalAGGFilePath() const
        {
            return _aggInitializer->getOriginalAGGFilePath();
        }

        const std::string & getExpansionAGGFilePath() const
        {
            return _aggInitializer->getExpansionAGGFilePath();
        }

    private:
        std::unique_ptr<AGG::AGGInitializer> _aggInitializer;
        std::unique_ptr<fheroes2::h2d::H2DInitializer> _h2dInitializer;
    };
}

// SDL1: this app is not linked against the SDLmain.lib, implement our own WinMain
#if defined( _WIN32 ) && !SDL_VERSION_ATLEAST( 2, 0, 0 )
#undef main

int main( int argc, char ** argv );

int WINAPI WinMain( HINSTANCE /* hInstance */, HINSTANCE /* hPrevInstance */, LPSTR /* pCmdLine */, int /* nCmdShow */ )
{
    return main( __argc, __argv );
}
#endif

int main( int argc, char ** argv )
{
// SDL2main.lib converts argv to UTF-8, but this application expects ANSI, use the original argv
#if defined( _WIN32 ) && SDL_VERSION_ATLEAST( 2, 0, 0 )
    assert( argc == __argc );

    argv = __argv;
#else
    (void)argc;
#endif

    try {
        const fheroes2::HardwareInitializer hardwareInitializer;
        Logging::InitLog();

        COUT( GetCaption() )

        Settings & conf = Settings::Get();
        conf.SetProgramPath( argv[0] );

        InitConfigDir();
        InitDataDir();
        ReadConfigs();

        std::set<fheroes2::SystemInitializationComponent> coreComponents{ fheroes2::SystemInitializationComponent::Audio,
                                                                          fheroes2::SystemInitializationComponent::Video };

#if defined( TARGET_PS_VITA ) || defined( TARGET_NINTENDO_SWITCH )
        coreComponents.emplace( fheroes2::SystemInitializationComponent::GameController );
#endif

        const fheroes2::CoreInitializer coreInitializer( coreComponents );

        DEBUG_LOG( DBG_GAME, DBG_INFO, conf.String() )

        const DisplayInitializer displayInitializer;
        const DataInitializer dataInitializer;

        ListFiles midiSoundFonts;

        midiSoundFonts.Append( Settings::FindFiles( System::concatPath( "files", "soundfonts" ), ".sf2", false ) );
        midiSoundFonts.Append( Settings::FindFiles( System::concatPath( "files", "soundfonts" ), ".sf3", false ) );

#ifdef WITH_DEBUG
        for ( const std::string & file : midiSoundFonts ) {
            DEBUG_LOG( DBG_GAME, DBG_INFO, "MIDI SoundFont to load: " << file )
        }
#endif

        const AudioManager::AudioInitializer audioInitializer( dataInitializer.getOriginalAGGFilePath(), dataInitializer.getExpansionAGGFilePath(), midiSoundFonts );

        // Load palette.
        fheroes2::setGamePalette( AGG::getDataFromAggFile( "KB.PAL" ) );
        fheroes2::Display::instance().changePalette( nullptr, true );

        // load BIN data
        Bin_Info::InitBinInfo();

        // init game data
        Game::Init();

        conf.setGameLanguage( conf.getGameLanguage() );

        if ( conf.isShowIntro() ) {
            fheroes2::showTeamInfo();

            Video::ShowVideo( "NWCLOGO.SMK", Video::VideoAction::PLAY_TILL_VIDEO_END );
            Video::ShowVideo( "CYLOGO.SMK", Video::VideoAction::PLAY_TILL_VIDEO_END );
            Video::ShowVideo( "H2XINTRO.SMK", Video::VideoAction::PLAY_TILL_VIDEO_END );
        }

        // init cursor
        const CursorRestorer cursorRestorer( true, Cursor::POINTER );

        Game::mainGameLoop( conf.isFirstGameRun() );
    }
    catch ( const std::exception & ex ) {
        ERROR_LOG( "Exception '" << ex.what() << "' occurred during application runtime." )
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
