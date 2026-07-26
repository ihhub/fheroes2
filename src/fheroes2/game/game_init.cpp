/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2026                                                    *
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

#include "game_init.h"

#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

#include "agg.h"
#include "audio_manager.h"
#include "core.h"
#include "cursor.h"
#include "dir.h"
#include "embedded_image.h"
#include "exception.h"
#include "game_assets.h"
#include "game_delays.h"
#include "game_exit.h"
#include "game_hotkeys.h"
#include "game_invalid_assets.h"
#include "game_mode.h"
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
#include "version.h"
#include "zzlib.h"

namespace fheroes2
{
    class Image;
}

namespace
{
    const std::string_view rootDir{ "fheroes2" };

    const char * const appCaption{ "fheroes2 engine, version: " ENGINE_VERSION };

    class DisplayInitializer final : public ComponentBase
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

            auto & engine = fheroes2::engine();
            if ( engine.isFullScreen() != conf.FullScreen() ) {
                engine.toggleFullScreen();
            }

            display.setWindowPos( conf.getSavedWindowPos() );
            display.setResolution( bestResolution );

            engine.setTitle( appCaption );

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
            engine.setIcon( appIcon );
#endif
        }

        DisplayInitializer( const DisplayInitializer & ) = delete;
        DisplayInitializer & operator=( const DisplayInitializer & ) = delete;

        ~DisplayInitializer() override
        {
            fheroes2::RenderProcessor::instance().unregisterRenderers();
            fheroes2::Display::instance().release();
        }

    private:
        // This member must not be initialized before Display.
        std::unique_ptr<fheroes2::SystemInfoRenderer> _systemInfoRenderer;
    };

    class DataInitializer final : public ComponentBase
    {
    public:
        DataInitializer()
        {
            const fheroes2::ScreenPaletteRestorer screenRestorer;

            try {
                _aggInitializer = std::make_unique<AGG::AGGInitializer>();

                _h2dInitializer = std::make_unique<fheroes2::h2d::H2DInitializer>();

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
        ~DataInitializer() override = default;

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

namespace Game
{
    void initLogging()
    {
        Logging::InitLog();

        COUT( appCaption )
    }

    void initDataDir()
    {
        const std::string dataDir = System::GetDataDirectory( rootDir );

        if ( dataDir.empty() ) {
            return;
        }

        const std::string dataFiles = System::concatPath( dataDir, "files" );
        const std::string dataFilesSave = System::concatPath( dataFiles, "save" );

        // This call will also create dataDir and dataFiles
        System::MakeDirectory( dataFilesSave );
    }

    void initConfigDir( const char * appPath )
    {
        Settings & conf = Settings::Get();
        conf.SetProgramPath( appPath );

        const std::string configDir = System::GetConfigDirectory( rootDir );

        System::MakeDirectory( configDir );

        const std::string configurationFileName{ Settings::configFileName };
        const std::string confFile = Settings::GetLastFile( "", configurationFileName );

        if ( System::IsFile( confFile ) && conf.Read( confFile ) ) {
            LocalEvent::Get().SetControllerPointerSpeed( conf.controllerPointerSpeed() );
        }
        else {
            conf.Save( configurationFileName );
        }

        DEBUG_LOG( DBG_GAME, DBG_INFO, conf.getOptionsString() )
    }

    void initPalette()
    {
        fheroes2::setGamePalette( AGG::getDataFromAggFile( "KB.PAL", false ) );
        fheroes2::Display::instance().changePalette( nullptr, true );
    }

    void initTranslations()
    {
        // Update the fonts according to the game language set in the configuration.
        // NOTICE: it must be done before initializing the engine to properly load all
        // language-specific font characters for the selected language because during
        // initialization the English language is forced to properly read the configuration files.
        Settings & conf = Settings::Get();
        conf.setGameLanguage( conf.getGameLanguage() );
    }

    void initEventHandler()
    {
        // set global events
        LocalEvent & eventHandler = LocalEvent::Get();
        eventHandler.setGlobalMouseMotionEventHook( Cursor::updateCursorPosition );
        eventHandler.setGlobalKeyDownEventHook( globalKeyDownEvent );
        eventHandler.setQuitEventProcessingHook( []() { return ( processExitEvent() == fheroes2::GameMode::QUIT_GAME ); } );
    }

    void initAnimation()
    {
        initAnimateDelays();
    }

    void initHotKeys()
    {
        HotKeysLoad( Settings::GetLastFile( "", hotkeyFileName ) );
    }

    std::unique_ptr<ComponentBase> createHardwareComponent()
    {
        return std::make_unique<System::HardwareInitializer>();
    }

    std::unique_ptr<ComponentBase> createCoreComponent()
    {
        return std::make_unique<System::CoreInitializer>();
    }

    std::unique_ptr<ComponentBase> createDisplayComponent()
    {
        return std::make_unique<DisplayInitializer>();
    }

    std::unique_ptr<ComponentBase> createDataComponent()
    {
        return std::make_unique<DataInitializer>();
    }

    std::unique_ptr<ComponentBase> createAudioComponent( const ComponentBase * dataComponent )
    {
        const DataInitializer * dataInitializer = dynamic_cast<const DataInitializer *>( dataComponent );
        if ( dataInitializer == nullptr ) {
            throw fheroes2::InvalidDataResources{ "Data resources have not been provided." };
        }

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

        return std::make_unique<AudioManager::AudioInitializer>( dataInitializer->getOriginalAGGFilePath(), dataInitializer->getExpansionAGGFilePath(), midiSoundFonts,
                                                                 timidityCfgPath );
    }
}
