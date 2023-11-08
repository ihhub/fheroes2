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

#include "game.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "agg_image.h"
#include "ai.h"
#include "army.h"
#include "audio.h"
#include "audio_manager.h"
#include "battle_only.h"
#include "castle.h"
#include "color.h"
#include "cursor.h"
#include "dialog.h"
#include "direction.h"
#include "game_delays.h"
#include "game_hotkeys.h"
#include "game_interface.h"
#include "game_io.h"
#include "game_mode.h"
#include "game_over.h"
#include "heroes.h"
#include "icn.h"
#include "image.h"
#include "interface_buttons.h"
#include "interface_cpanel.h"
#include "interface_gamearea.h"
#include "interface_icons.h"
#include "interface_radar.h"
#include "interface_status.h"
#include "kingdom.h"
#include "localevent.h"
#include "logging.h"
#include "m82.h"
#include "maps.h"
#include "maps_tiles.h"
#include "maps_tiles_helper.h"
#include "math_base.h"
#include "monster.h"
#include "mp2.h"
#include "mus.h"
#include "players.h"
#include "rand.h"
#include "resource.h"
#include "route.h"
#include "screen.h"
#include "settings.h"
#include "system.h"
#include "tools.h"
#include "translations.h"
#include "ui_dialog.h"
#include "ui_text.h"
#include "ui_tool.h"
#include "week.h"
#include "world.h"
#include "SDL_timer.h"
#include "SDL_system.h"
#include "SDL_thread.h"
#include "jni.h"

uint32_t lwpLastMapUpdate = 0;
bool forceMapUpdate = true;
bool forceConfigUpdate = true;

void initWallpaper() {
    Settings &conf = Settings::Get();

    const MapsFileInfoList list = Maps::PrepareMapsFileInfoList(conf.IsGameType(Game::TYPE_MULTI));
    const uint32_t randomMapIndex = Rand::Get(0, list.size() - 1);
    const Maps::FileInfo &map = list.at(randomMapIndex);

    conf.SetCurrentFileInfo(map);
    VERBOSE_LOG("initWallpaper name: " << map.name.c_str() << " file: "
                                       << map.file.c_str())
    conf.GetPlayers().SetStartGame();

    world.LoadMapMP2(conf.MapsFile(), false);
}

bool shouldUpdateMapRegion() {
    uint32_t updateInterval = Settings::Get().GetLWPMapUpdateInterval();
    uint32_t currentTime = std::time(nullptr);
    bool isExpired = lwpLastMapUpdate < currentTime - updateInterval;

    VERBOSE_LOG(
            "ShouldUpdateMapRegion"
                    << " interval:" << updateInterval
                    << " current: " << currentTime
                    << " last update: " << lwpLastMapUpdate
    )

    return isExpired;
}

void randomizeGameAreaPoint() {
    if (!shouldUpdateMapRegion()) {
        return;
    }

    if (lwpLastMapUpdate != 0) {
        initWallpaper();
    }

    lwpLastMapUpdate = std::time(nullptr);

    fheroes2::Display &display = fheroes2::Display::instance();
    int32_t displayWidth = display.width();
    int32_t displayHeight = display.height();

    int32_t mapWidth = World::Get().w();
    int32_t mapHeight = World::Get().h();

    int32_t screenHeight = static_cast<int32_t>(floor(displayHeight / TILEWIDTH));
    int32_t screenWidth = static_cast<int32_t>(floor(displayWidth / TILEWIDTH));

    int32_t halfHeight = static_cast<int32_t>(floor(screenHeight / 2));
    int32_t halfWidth = static_cast<int32_t>(floor(screenWidth / 2));

    int32_t widthFrom = halfWidth + 1;
    int32_t widthTo = mapWidth - halfWidth - 1;
    int32_t x = Rand::Get(widthFrom, widthTo);

    int32_t heightFrom = halfHeight + 1;
    int32_t heightTo = mapHeight - halfHeight - 1;
    int32_t y = Rand::Get(heightFrom, heightTo);

    VERBOSE_LOG("Map w: " << mapWidth << " h: " << mapHeight)
    VERBOSE_LOG("Screen w: " << screenWidth << " h: " << screenHeight)
    VERBOSE_LOG("Half screen w: " << halfWidth << " h: " << halfHeight)
    VERBOSE_LOG("Width from: " << widthFrom << " to: " << widthTo)
    VERBOSE_LOG("Height from: " << heightFrom << " to: " << heightTo)
    VERBOSE_LOG("Next point x: " << x << " y: " << y)

    Interface::AdventureMap::Get().getGameArea().SetCenter({x, y});
}

void updateBrightness() {
    int brightness = Settings::Get().GetLWPBrightness();
    int brightnessAlpha = static_cast<uint8_t>(floor((100 - brightness * 255) / 100));
    VERBOSE_LOG("updateBrightness value: " << brightness << " alpha: " << brightnessAlpha)
    fheroes2::engine().setBrightness(brightnessAlpha);
}

void readConfigFile() {
    VERBOSE_LOG("readConfigFile")
    const std::string configurationFileName(Settings::configFileName);
    const std::string confFile = Settings::GetLastFile("", configurationFileName);

    if (System::IsFile(confFile)) {
        Settings::Get().Read(confFile);
    }
}

void resizeDisplay() {
    fheroes2::Display &display = fheroes2::Display::instance();
    int scale = Settings::Get().GetLWPScale();
    VERBOSE_LOG("resizeDisplay scale: " << scale)

    fheroes2::ResolutionInfo nextResolution = display.getScaledScreenSize(scale);

    if (nextResolution.screenWidth != display.width() ||
        nextResolution.screenHeight != display.height()) {
        display.setResolution(nextResolution);
        Interface::AdventureMap::Get().getGameArea().generate({display.width(), display.height()},
                                                              true);
    }
}

void renderMap() {
    Interface::GameArea const &gameArea = Interface::AdventureMap::Get().getGameArea();
    fheroes2::Display &display = fheroes2::Display::instance();

    Game::updateAdventureMapAnimationIndex();
    gameArea.Redraw(
            display,
            Interface::RedrawLevelType::LEVEL_OBJECTS |
            Interface::RedrawLevelType::LEVEL_HEROES);
    display.render();
}

void rereadAndApplyConfigs() {
    readConfigFile();
    resizeDisplay();
    updateBrightness();
}

void forceUpdates() {
    if (forceConfigUpdate) {
        rereadAndApplyConfigs();
        forceConfigUpdate = false;
    }

    if (forceMapUpdate) {
        randomizeGameAreaPoint();
        renderMap();
        forceMapUpdate = false;
    }
}

extern "C" JNIEXPORT void JNICALL
Java_org_libsdl_app_SDLActivity_nativeUpdateVisibleMapRegion([[maybe_unused]] JNIEnv *env,
                                                             [[maybe_unused]] jclass cls) {
    VERBOSE_LOG("nativeUpdateVisibleMapRegion")
    forceMapUpdate = true;
}

extern "C" JNIEXPORT void JNICALL
Java_org_libsdl_app_SDLActivity_nativeUpdateConfigs([[maybe_unused]] JNIEnv *env,
                                                    [[maybe_unused]] jclass cls) {
    VERBOSE_LOG("nativeUpdateConfigs")
    forceConfigUpdate = true;
}

void handleSDLEvents(SDL_Event &event, LocalEvent &le, fheroes2::Display &display) {
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_RENDER_TARGETS_RESET: {
                VERBOSE_LOG("SDL_RENDER_TARGETS_RESET")
                display.render();
                break;
            }
            case SDL_RENDER_DEVICE_RESET: {
                VERBOSE_LOG("SDL_RENDER_DEVICE_RESET")
                le.HandleRenderDeviceResetEvent();
                display.render();
                break;
            }
        }
    }
}

void renderWallpaper() {
    Interface::GameArea &gameArea = Interface::AdventureMap::Get().getGameArea();

    fheroes2::Display &display = fheroes2::Display::instance();
    LocalEvent &le = LocalEvent::Get();
    SDL_Event event;

    gameArea.generate({display.width(), display.height()}, true);

    while (true) {
        forceUpdates();

        handleSDLEvents(event, le, display);

        if (Game::validateAnimationDelay(Game::MAPS_DELAY)) {
            renderMap();
        } else {
            SDL_Delay(Game::getAnimationDelayValue(Game::MAPS_DELAY));
        }
    }
}

void overrideConfiguration() {
    Settings &conf = Settings::Get();
    conf.SetGameType(Game::TYPE_STANDARD);
    conf.SetCurrentColor(Color::NONE);
    conf.setVSync(true);
    conf.setSystemInfo(false);
    conf.setHideInterface(true);
    conf.SetShowControlPanel(false);
}

fheroes2::GameMode Game::Wallpaper() {
    rereadAndApplyConfigs();
    overrideConfiguration();
    initWallpaper();
    renderWallpaper();

    return fheroes2::GameMode::END_TURN;
}
