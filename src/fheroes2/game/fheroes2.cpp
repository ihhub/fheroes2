/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2026                                             *
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

#include <cstdint>
#include <cstdlib>
#include <exception>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <vector>

// Managing compiler warnings for SDL headers
#if defined( __GNUC__ )
#pragma GCC diagnostic push

#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wswitch-default"
#endif

#include <SDL_events.h>
#include <SDL_main.h> // IWYU pragma: keep

// Managing compiler warnings for SDL headers
#if defined( __GNUC__ )
#pragma GCC diagnostic pop
#endif

#if defined( _WIN32 )
#include <cassert>
#endif

#include "agg.h"
#include "audio_manager.h"
#include "core.h"
#include "cursor.h"
#include "dir.h"
#include "embedded_image.h"
#include "exception.h"
#include "game.h"
#include "game_assets.h"
#include "game_invalid_assets.h"
#include "h2d.h"
#include "icn.h"
#include "image_palette.h"
#include "localevent.h"
#include "logging.h"
#include "render_processor.h"
#include "screen.h"
#include "settings.h"
#include "system.h"
#include "ui_tool.h"
#include "zzlib.h"

namespace fheroes2
{
    class Image;
}

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

            // Fullscreen mode can be enabled by default for some devices, we need to forcibly
            // synchronize reality with the default config if config file was not read
            conf.setFullScreen( conf.FullScreen() );
        }
    }

    void InitConfigDir()
    {
        const std::string configDir = System::GetConfigDirectory( "fheroes2" );

        System::MakeDirectory( configDir );
    }

    void InitDataDir()
    {
        const std::string dataDir = System::GetDataDirectory( "fheroes2" );

        if ( dataDir.empty() ) {
            return;
        }

        const std::string dataFiles = System::concatPath( dataDir, "files" );
        const std::string dataFilesSave = System::concatPath( dataFiles, "save" );

        // This call will also create dataDir and dataFiles
        System::MakeDirectory( dataFilesSave );
    }

    class DisplayInitializer final
    {
    public:
        DisplayInitializer()
        {
            const Settings & conf = Settings::Get();

            fheroes2::Display & display = fheroes2::Display::instance();
            fheroes2::ResolutionInfo bestResolution{ conf.currentResolutionInfo() };

            if ( conf.isFirstGameRun() && System::isHandheldDevice() ) {
                // We do not show resolution dialog for first run on handheld devices. In this case it is wise to set 'widest' resolution by default.
                const std::vector<fheroes2::ResolutionInfo> resolutions = fheroes2::engine().getAvailableResolutions();

                for ( const fheroes2::ResolutionInfo & info : resolutions ) {
                    if ( info.gameWidth > bestResolution.gameWidth && info.gameHeight == bestResolution.gameHeight ) {
                        bestResolution = info;
                    }
                }
            }

            display.setWindowPos( conf.getSavedWindowPos() );
            display.setResolution( bestResolution );

            fheroes2::engine().setTitle( GetCaption() );
            
            auto & cursor = fheroes2::cursor();
            cursor.enableSoftwareEmulation( conf.isSoftwareEmulationEnabled() );
            cursor.show( false );

            fheroes2::RenderProcessor & renderProcessor = fheroes2::RenderProcessor::instance();

            display.subscribe( [&renderProcessor]( std::vector<uint8_t> & palette ) { return renderProcessor.preRenderAction( palette ); },
                               [&renderProcessor]() { renderProcessor.postRenderAction(); } );

            // Initialize system info renderer.
            _systemInfoRenderer = std::make_unique<fheroes2::SystemInfoRenderer>();

            renderProcessor.registerRenderers( [sysInfoRenderer = _systemInfoRenderer.get()]() { sysInfoRenderer->preRender(); },
                                               [sysInfoRenderer = _systemInfoRenderer.get()]() { sysInfoRenderer->postRender(); } );
            renderProcessor.startColorCycling();

            // Update mouse cursor when switching between software emulation and OS mouse modes.
            cursor.registerUpdater( Cursor::Refresh );

#if !defined( MACOS_APP_BUNDLE )
            const fheroes2::Image & appIcon = Compression::CreateImageFromZlib( 32, 32, iconImage, sizeof( iconImage ), true );
            fheroes2::engine().setIcon( appIcon );
#endif
        }

        DisplayInitializer( const DisplayInitializer & ) = delete;
        DisplayInitializer & operator=( const DisplayInitializer & ) = delete;

        ~DisplayInitializer()
        {
            fheroes2::RenderProcessor::instance().unregisterRenderers();
            fheroes2::Display::instance().release();
        }

    private:
        // This member must not be initialized before Display.
        std::unique_ptr<fheroes2::SystemInfoRenderer> _systemInfoRenderer;
    };

    class DataInitializer final
    {
    public:
        DataInitializer()
        {
            const fheroes2::ScreenPaletteRestorer screenRestorer;

            try {
                _aggInitializer.reset( new AGG::AGGInitializer );

                _h2dInitializer.reset( new fheroes2::h2d::H2DInitializer );

                // Verify that the font is present and it is not corrupted.
                Assets::getImage( ICN::FONT, 0 );
            }
            catch ( ... ) {
                showMissingAssetsImage();

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

int main( int argc, char ** argv )
{
// SDL2main.lib converts argv to UTF-8, but this application expects ANSI, use the original argv
#if defined( _WIN32 )
    assert( argc == __argc );

    argv = __argv;
#else
    (void)argc;
#endif

    try {
        const System::HardwareInitializer hardwareInitializer;
        Logging::InitLog();

        COUT( GetCaption() )

        Settings & conf = Settings::Get();
        conf.SetProgramPath( argv[0] );

        InitConfigDir();
        InitDataDir();
        ReadConfigs();

        const System::CoreInitializer coreInitializer;

        DEBUG_LOG( DBG_GAME, DBG_INFO, conf.getOptionsString() )

        const DisplayInitializer displayInitializer;
        const DataInitializer dataInitializer;

        ListFiles midiSoundFonts;
        {
            const std::string path = System::concatPath( "files", "soundfonts" );
            midiSoundFonts.Append( Settings::FindFiles( path, ".sf2", false ) );
            midiSoundFonts.Append( Settings::FindFiles( path, ".sf3", false ) );
        }

#ifdef WITH_DEBUG
        for ( const std::string & file : midiSoundFonts ) {
            DEBUG_LOG( DBG_GAME, DBG_INFO, "MIDI SoundFont to load: " << file )
        }
#endif

        const std::string timidityCfgPath = []() -> std::string {
            if ( std::string path; Settings::findFile( System::concatPath( "files", "timidity" ), "timidity.cfg", path ) ) {
                return path;
            }

            return {};
        }();

#ifdef WITH_DEBUG
        if ( !timidityCfgPath.empty() ) {
            DEBUG_LOG( DBG_GAME, DBG_INFO, "Path to the timidity.cfg file: " << timidityCfgPath )
        }
#endif

        const AudioManager::AudioInitializer audioInitializer( dataInitializer.getOriginalAGGFilePath(), dataInitializer.getExpansionAGGFilePath(), midiSoundFonts,
                                                               timidityCfgPath );

        // Load palette.
        fheroes2::setGamePalette( AGG::getDataFromAggFile( "KB.PAL", false ) );
        const fheroes2::Display & display = fheroes2::Display::instance();
        display.changePalette( nullptr, true );

        // Update the fonts according to the game language set in the configuration.
        // NOTICE: it must be done before initializing the engine to properly load all
        // language-specific font characters for the selected language because during
        // initialization the English language is forced to properly read the configuration files.
        conf.setGameLanguage( conf.getGameLanguage() );

        // Initialize game data.
        Game::Init();

        try {
            Game::runMainGameLoop();
        }
        catch ( const fheroes2::InvalidDataResources & ex ) {
            ERROR_LOG( ex.what() )
            showMissingAssetsImage();
            return EXIT_FAILURE;
        }
        catch ( const fheroes2::CorruptedExecutable & ex ) {
            ERROR_LOG( ex.what() )
            return EXIT_FAILURE;
        }
        catch ( const fheroes2::UserRequestedApplicationClosure & ) {
            // Yes, this an evil way of doing things but our application design doesn't allow to simply propagate application closure event.
            return EXIT_SUCCESS;
        }
    }
    catch ( const std::exception & ex ) {
        ERROR_LOG( "Exception '" << ex.what() << "' occurred during application runtime." )
        return EXIT_FAILURE;
    }
    catch ( ... ) {
        ERROR_LOG( "An unknown exception occurred during application runtime." )
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
