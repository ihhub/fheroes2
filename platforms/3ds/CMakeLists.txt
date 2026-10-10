###########################################################################
#   fheroes2: https://github.com/ihhub/fheroes2                           #
#   Copyright (C) 2022 - 2026                                             #
#                                                                         #
#   This program is free software; you can redistribute it and/or modify  #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation; either version 2 of the License, or     #
#   (at your option) any later version.                                   #
#                                                                         #
#   This program is distributed in the hope that it will be useful,       #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#   GNU General Public License for more details.                          #
#                                                                         #
#   You should have received a copy of the GNU General Public License     #
#   along with this program; if not, write to the                         #
#   Free Software Foundation, Inc.,                                       #
#   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             #
###########################################################################

cmake_minimum_required(VERSION 3.24)
project(fheroes2_3ds LANGUAGES C CXX)

if(NOT NINTENDO_3DS)
    message(FATAL_ERROR "Use the devkitPro 3DS toolchain")
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# The statically linked 3DS executable uses newlib TLS without a dynamic loader.
set(CMAKE_POSITION_INDEPENDENT_CODE OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(FHEROES2_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(USE_SDL_VERSION SDL2)
set(FHEROES2_DATA "sdmc:/3ds/fheroes2")
set(ENABLE_IMAGE OFF)
set(MACOS_APP_BUNDLE OFF)
set(FHEROES2_SDL2_SOURCE_DIR "" CACHE PATH "Optional SDL2 source directory")
set(FHEROES2_SDL2_MIXER_SOURCE_DIR "" CACHE PATH "Optional SDL2_mixer source directory")

if(FHEROES2_SDL2_SOURCE_DIR)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_TEST OFF CACHE BOOL "" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)
    set(SDL_SENSOR OFF CACHE BOOL "" FORCE)
    add_subdirectory("${FHEROES2_SDL2_SOURCE_DIR}" sdl2)
else()
    find_package(SDL2 REQUIRED)
endif()
if(FHEROES2_SDL2_MIXER_SOURCE_DIR)
    foreach(feature INSTALL SAMPLES DEPS_SHARED CMD MOD MIDI OPUS WAVPACK)
        set(SDL2MIXER_${feature} OFF CACHE BOOL "" FORCE)
    endforeach()
    set(SDL2MIXER_VORBIS STB CACHE STRING "" FORCE)
    add_subdirectory("${FHEROES2_SDL2_MIXER_SOURCE_DIR}" mixer)
    if(NOT TARGET SDL2_mixer::SDL2_mixer)
        add_library(SDL2_mixer::SDL2_mixer ALIAS SDL2_mixer)
    endif()
else()
    find_package(SDL2_mixer REQUIRED)
endif()
find_package(ZLIB REQUIRED)
# newlib threading uses libctru rather than a host pthread library.
add_library(threads_3ds INTERFACE)
target_link_libraries(threads_3ds INTERFACE ctru)
add_library(Threads::Threads ALIAS threads_3ds)
add_compile_options(-fsigned-char)
add_subdirectory("${FHEROES2_ROOT}/src" src)

find_program(THREEDSXTOOL 3dsxtool HINTS "${DEVKITPRO}/tools/bin" "$ENV{DEVKITPRO}/tools/bin" REQUIRED)
find_program(SMDHTOOL smdhtool HINTS "${DEVKITPRO}/tools/bin" "$ENV{DEVKITPRO}/tools/bin" REQUIRED)
find_file(THREEDS_ICON default_icon.png HINTS "${DEVKITPRO}/libctru" "$ENV{DEVKITPRO}/libctru" NO_CMAKE_FIND_ROOT_PATH REQUIRED)
add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/fheroes2.smdh"
    COMMAND "${SMDHTOOL}" --create "fheroes2" "Heroes of Might and Magic II engine" "fheroes2 contributors"
        "${THREEDS_ICON}" "${CMAKE_BINARY_DIR}/fheroes2.smdh"
    DEPENDS "${THREEDS_ICON}" VERBATIM)
add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/fheroes2.3dsx"
    COMMAND "${THREEDSXTOOL}" "$<TARGET_FILE:fheroes2>" "${CMAKE_BINARY_DIR}/fheroes2.3dsx" "--smdh=${CMAKE_BINARY_DIR}/fheroes2.smdh"
    DEPENDS fheroes2 "${CMAKE_BINARY_DIR}/fheroes2.smdh" VERBATIM)
add_custom_target(fheroes2_3dsx ALL DEPENDS "${CMAKE_BINARY_DIR}/fheroes2.3dsx")
