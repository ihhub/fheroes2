###########################################################################
#   fheroes2: https://github.com/ihhub/fheroes2                           #
#   Copyright (C) 2022                                                    #
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

cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0074 NEW)

file(STRINGS ${CMAKE_SOURCE_DIR}/version.txt FHEROES2_VERSION LIMIT_COUNT 1 REGEX "^[0-9]+\.[0-9]+\.[0-9]+$")

project(fheroes2 VERSION ${FHEROES2_VERSION} LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)

include(CMakeDependentOption)
include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

#
# Compile-time options
#
option(USE_SYSTEM_LIBSMACKER "Use system libsmacker instead of the bundled version" OFF)
option(ENABLE_STRICT_COMPILATION "Enable strict compilation mode (turns warnings into errors)" OFF)
option(ENABLE_IMAGE "Enable SDL2 Image support (requires libpng)" ON)
option(ENABLE_TOOLS "Enable additional tools" OFF)

# Available only on macOS
cmake_dependent_option(MACOS_APP_BUNDLE "Create a Mac app bundle" OFF "APPLE" OFF)

# Should not be available if we are creating a Mac app bundle
cmake_dependent_option(GET_HOMM2_DEMO "Fetch and install the HoMM II demo data" OFF "NOT MACOS_APP_BUNDLE" OFF)

set(USE_SDL_VERSION SDL2 CACHE STRING "Version of the SDL library used")
if(WIN32)
	set(FHEROES2_DATA ${CMAKE_INSTALL_PREFIX})
	set(CMAKE_INSTALL_BINDIR ${CMAKE_INSTALL_PREFIX})
	set(CMAKE_INSTALL_DOCDIR ${CMAKE_INSTALL_PREFIX})
else(WIN32)
	set(FHEROES2_DATA ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME} CACHE STRING "Path to the fheroes2 data directory (relative to CMAKE_INSTALL_PREFIX)")
endif(WIN32)

# For ComboBox in CMake GUI
set_property(CACHE USE_SDL_VERSION PROPERTY STRINGS SDL SDL2)

#
# Library & feature detection
#
find_package(${USE_SDL_VERSION} REQUIRED)
find_package(${USE_SDL_VERSION}_mixer REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Threads)
find_program(ICONV_EXECUTABLE iconv)
find_program(SED_EXECUTABLE sed)
find_program(XGETTEXT_EXECUTABLE xgettext)

cmake_dependent_option(BUILD_TRANSLATIONS "Build translations" ON "ICONV_EXECUTABLE;SED_EXECUTABLE;XGETTEXT_EXECUTABLE" OFF)

if(ENABLE_IMAGE)
	find_package(${USE_SDL_VERSION}_image REQUIRED)
	find_package(PNG REQUIRED)
endif(ENABLE_IMAGE)

#
# Source files
#
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_subdirectory(src)

#
# Translation files
#
if(BUILD_TRANSLATIONS)
	add_subdirectory(files/lang)
endif(BUILD_TRANSLATIONS)

#
# macOS app bundle
#
if(MACOS_APP_BUNDLE)
	add_custom_target(
		app_bundle_h2d_files
		ALL
		COMMAND mkdir -p ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.app/Contents/Resources/h2d
		COMMAND cp *.h2d ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.app/Contents/Resources/h2d
		DEPENDS fheroes2
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/files/data
		)
	add_custom_target(
		app_bundle_translations
		ALL
		COMMAND mkdir -p ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.app/Contents/Resources/translations
		COMMAND cp *.mo ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.app/Contents/Resources/translations
		DEPENDS translations
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/files/lang
		)
	add_custom_target(
		run_dylibbundler
		ALL
		COMMAND dylibbundler -od -b -x ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.app/Contents/MacOS/${PROJECT_NAME} -d ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.app/Contents/libs
		DEPENDS fheroes2 app_bundle_h2d_files app_bundle_translations
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		)
else(MACOS_APP_BUNDLE)

#
# Installation of auxiliary files
#
	install(
		DIRECTORY files/data/
		DESTINATION ${FHEROES2_DATA}/data
		FILES_MATCHING
		PATTERN *.h2d
		)
	install(
		FILES docs/README.txt LICENSE changelog.txt
		DESTINATION ${CMAKE_INSTALL_DOCDIR}
		)
	install(
		PROGRAMS $<$<PLATFORM_ID:Windows>:${CMAKE_SOURCE_DIR}/script/homm2/extract_homm2_resources.bat>
		script/homm2/extract_homm2_resources$<IF:$<PLATFORM_ID:Windows>,.ps1,.sh>
		DESTINATION ${CMAKE_INSTALL_DOCDIR}$<$<NOT:$<PLATFORM_ID:Windows>>:/homm2>
		)

	if(GET_HOMM2_DEMO)
		include(FetchContent)
		set(FETCHCONTENT_QUIET OFF)

		FetchContent_Declare(
			demodata
			URL https://archive.org/download/HeroesofMightandMagicIITheSuccessionWars_1020/h2demo.zip
			URL_HASH SHA256=12048c8b03875c81e69534a3813aaf6340975e77b762dc1b79a4ff5514240e3c
			)
		FetchContent_MakeAvailable(demodata)

		file(COPY ${demodata_SOURCE_DIR}/DATA/ DESTINATION data PATTERN *.*)
		file(COPY ${demodata_SOURCE_DIR}/MAPS/ DESTINATION maps PATTERN *.*)

		install(
			DIRECTORY ${demodata_SOURCE_DIR}/DATA/
			DESTINATION ${FHEROES2_DATA}/data
			FILES_MATCHING
			PATTERN *.*
			)
		install(
			DIRECTORY ${demodata_SOURCE_DIR}/MAPS/
			DESTINATION ${FHEROES2_DATA}/maps
			FILES_MATCHING
			PATTERN *.*
			)
	else(GET_HOMM2_DEMO)
		install(
			PROGRAMS $<$<PLATFORM_ID:Windows>:${CMAKE_SOURCE_DIR}/script/demo/download_demo_version.bat>
			script/demo/download_demo_version$<IF:$<PLATFORM_ID:Windows>,.ps1,.sh>
			DESTINATION ${CMAKE_INSTALL_DOCDIR}$<$<NOT:$<PLATFORM_ID:Windows>>:/demo>
			)
	endif(GET_HOMM2_DEMO)

	if(WIN32)
		install(
			FILES script/homm2/resource_extraction_toolset.bat script/homm2/resource_extraction_toolset.ps1
			DESTINATION ${CMAKE_INSTALL_PREFIX}
			)
	else(WIN32)
		install(
			FILES script/packaging/common/fheroes2.appdata.xml
			DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/metainfo
			)
		install(
			FILES src/resources/fheroes2.png
			DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/128x128/apps
			)
		install(
			FILES script/packaging/common/fheroes2.desktop
			DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
			)
	endif(WIN32)
endif(MACOS_APP_BUNDLE)
