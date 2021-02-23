# For file(GLOB_RECOURSE CONFIGURE_DEPENDS ...)
cmake_minimum_required(VERSION 3.12)

project(fheroes2 VERSION 0.8.4 LANGUAGES C CXX)
set (CMAKE_CXX_STANDARD 11)

include(GNUInstallDirs)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Compile-time options
set(SDL_VERSIONS SDL SDL2)
set(USE_SDL_VERSION SDL CACHE STRING "SDL version")
set_property(CACHE USE_SDL_VERSION PROPERTY STRINGS ${SDL_VERSIONS})

option(ENABLE_IMAGE    "Enable SDL/SDL2 Image support (requires libpng)" ON)
option(ENABLE_MIXER    "Enable SDL/SDL2 Mixer support" ON)
option(ENABLE_UNICODE  "Enable Unicode support (requires SDL/SDL2 TTF and iconv libraries)" ON)
option(ENABLE_XML      "Enable XML support (requires libtinyxml)" ON)
option(ENABLE_ZLIB     "Enable Zlib support (for compressed savefiles)" ON)
option(ENABLE_AUDIOCD  "Enable AudioCD support" OFF)
option(ENABLE_EDITOR   "Enable build of editor (requires Qt5)" OFF)
option(ENABLE_TOOLS    "Enable additional tools" OFF)
option(ENABLE_RELEASE  "Release mode (disables debug options)" ON)

option(USE_SYSTEM_TINYXML        "Use system libtinyxml instead bundled version" OFF)
option(USE_SYSTEM_LIBSMACKER     "Use system libsmacker instead bundled version" OFF)

option(FHEROES2_STRICT_COMPILATION "Enable -Werror strict compilation" OFF)

set(CONFIGURE_FHEROES2_LOCALEDIR "${CMAKE_INSTALL_LOCALEDIR}/${PROJECT_NAME}" CACHE STRING "System Locale directory")
set(CONFIGURE_FHEROES2_DATA      "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}" CACHE STRING "System fheroes2 game directory")

# Find libraries and enable/disable features
find_package(${USE_SDL_VERSION} REQUIRED)
find_package(Threads)

if(ENABLE_IMAGE)
	find_package(${USE_SDL_VERSION}_image REQUIRED)
	find_package(PNG REQUIRED)
	find_package(ZLIB REQUIRED)
endif(ENABLE_IMAGE)
if(ENABLE_MIXER)
	find_package(${USE_SDL_VERSION}_mixer REQUIRED)
endif(ENABLE_MIXER)
if(ENABLE_UNICODE)
	find_package(${USE_SDL_VERSION}_ttf REQUIRED)
	find_package(Iconv REQUIRED)
endif(ENABLE_UNICODE)
if(ENABLE_ZLIB)
	find_package(ZLIB REQUIRED)
endif(ENABLE_ZLIB)

add_subdirectory(src)

if(ENABLE_EDITOR)
	add_subdirectory(editor)
endif(ENABLE_EDITOR)

install(FILES doc/README.txt changelog.txt DESTINATION ${CMAKE_INSTALL_DOCDIR})

