cmake_minimum_required(VERSION 3.14)
cmake_policy(SET CMP0074 NEW)

project(fheroes2 VERSION 0.9.13 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)

include(GNUInstallDirs)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

#
# Compile-time options
#
set(SDL_VERSIONS SDL SDL2)
set(USE_SDL_VERSION SDL2 CACHE STRING "SDL version")
set(CONFIGURE_FHEROES2_DATA "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}" CACHE STRING "System fheroes2 game directory")

option(ENABLE_IMAGE "Enable SDL/SDL2 Image support (requires libpng)" ON)
option(ENABLE_TOOLS "Enable additional tools" OFF)
option(GET_HOMM2_DEMO "Fetch and install HoMM II demo data" OFF)
option(USE_SYSTEM_LIBSMACKER "Use system libsmacker instead of bundled version" OFF)
option(FHEROES2_STRICT_COMPILATION "Enable -Werror strict compilation" OFF)

set_property(CACHE USE_SDL_VERSION PROPERTY STRINGS ${SDL_VERSIONS})

#
# Library & feature detection
#
find_package(${USE_SDL_VERSION} REQUIRED)
find_package(${USE_SDL_VERSION}_mixer REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Threads)

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
if(NOT WIN32)
	find_program(MAKE_EXECUTABLE make)
	find_program(SED_EXECUTABLE sed)
	find_program(XGETTEXT_EXECUTABLE xgettext)

	file(GLOB_RECURSE ALL_SOURCES CONFIGURE_DEPENDS src/*.cpp)

	add_custom_target(
		${PROJECT_NAME}.pot
		COMMAND ${XGETTEXT_EXECUTABLE} -d ${PROJECT_NAME} -C -F -k_ -k_n:1,2 -o ${PROJECT_NAME}.pot ${ALL_SOURCES}
		COMMAND ${SED_EXECUTABLE} -i~ -e "s/, c-format//" ${PROJECT_NAME}.pot
		DEPENDS fheroes2
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/dist
		)
	add_custom_target(
		translations
		ALL
		COMMAND ${MAKE_EXECUTABLE}
		DEPENDS ${PROJECT_NAME}.pot
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/files/lang
		)
endif(NOT WIN32)

#
# macOS app bundle
#
if(MACOS_APP_BUNDLE)
	if(NOT APPLE)
		message(SEND_ERROR "macOS bundles are only valid on macOS")
	endif(NOT APPLE)

	add_custom_target(
		app_bundle_h2d_files
		ALL
		COMMAND mkdir -p "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.app/Contents/Resources/h2d"
		COMMAND cp *.h2d "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.app/Contents/Resources/h2d"
		DEPENDS fheroes2
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/files/data"
		)
	add_custom_target(
		app_bundle_key_file
		ALL
		COMMAND cp fheroes2.key "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.app/Contents/Resources"
		DEPENDS fheroes2
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
		)
	add_custom_target(
		app_bundle_translations
		ALL
		COMMAND mkdir -p "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.app/Contents/Resources/translations"
		COMMAND cp *.mo "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.app/Contents/Resources/translations"
		DEPENDS translations
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/files/lang"
		)
	add_custom_target(
		run_dylibbundler
		ALL
		COMMAND dylibbundler -od -b -x "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.app/Contents/MacOS/${PROJECT_NAME}" -d "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.app/Contents/libs"
		DEPENDS fheroes2 app_bundle_h2d_files app_bundle_key_file app_bundle_translations
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
		)
endif(MACOS_APP_BUNDLE)

#
# Installation of auxiliary files
#
if(NOT MACOS_APP_BUNDLE)
	install(
		FILES fheroes2.key
		DESTINATION ${CONFIGURE_FHEROES2_DATA}
		)
	install(
		DIRECTORY files/data/
		DESTINATION ${CONFIGURE_FHEROES2_DATA}/files/data
		FILES_MATCHING
		PATTERN *.h2d
		)
	install(
		FILES docs/README.txt LICENSE changelog.txt
		DESTINATION ${CMAKE_INSTALL_DOCDIR}
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

		install(DIRECTORY ${demodata_SOURCE_DIR}/DATA/
			DESTINATION ${CONFIGURE_FHEROES2_DATA}/data
			FILES_MATCHING
			PATTERN *.*
			)
		install(DIRECTORY ${demodata_SOURCE_DIR}/MAPS/
			DESTINATION ${CONFIGURE_FHEROES2_DATA}/maps
			FILES_MATCHING
			PATTERN *.*
			)
	endif(GET_HOMM2_DEMO)

	if(WIN32)
		install(
			FILES script/demo/download_demo_version.bat script/demo/download_demo_version.ps1
			DESTINATION ${CMAKE_INSTALL_DOCDIR}/demo
			)
		install(
			FILES script/homm2/extract_homm2_resources.bat script/homm2/extract_homm2_resources.ps1
			DESTINATION ${CMAKE_INSTALL_DOCDIR}/homm2
			)
	else(WIN32)
		install(
			DIRECTORY files/lang/
			DESTINATION ${CONFIGURE_FHEROES2_DATA}/files/lang
			FILES_MATCHING
			PATTERN *.mo
			)
		install(
			FILES script/demo/download_demo_version.sh
			PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
			DESTINATION ${CMAKE_INSTALL_DOCDIR}/demo
			)
		install(
			FILES script/homm2/extract_homm2_resources.sh
			PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
			DESTINATION ${CMAKE_INSTALL_DOCDIR}/homm2
			)
	endif(WIN32)
endif(NOT MACOS_APP_BUNDLE)
