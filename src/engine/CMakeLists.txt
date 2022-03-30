###########################################################################
#   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  #
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

file(GLOB_RECURSE LIBENGINE_SOURCES CONFIGURE_DEPENDS *.cpp)

add_library(engine STATIC ${LIBENGINE_SOURCES})

target_compile_definitions(engine PRIVATE
	$<$<BOOL:${ENABLE_IMAGE}>:WITH_IMAGE>
	$<$<BOOL:${MACOS_APP_BUNDLE}>:MACOS_APP_BUNDLE>
	)

target_include_directories(engine PUBLIC
	$<$<BOOL:${ENABLE_IMAGE}>:${${USE_SDL_VERSION}_IMAGE_INCLUDE_DIR}>
	${${USE_SDL_VERSION}_MIXER_INCLUDE_DIR}
	${${USE_SDL_VERSION}_INCLUDE_DIR}
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<INSTALL_INTERFACE:include>
	)

target_link_libraries(engine
	smacker
	${${USE_SDL_VERSION}MAIN_LIBRARY}
	${${USE_SDL_VERSION}_LIBRARY}
	${${USE_SDL_VERSION}_MIXER_LIBRARIES}
	$<$<BOOL:${ENABLE_IMAGE}>:${${USE_SDL_VERSION}_IMAGE_LIBRARIES}>
	$<$<BOOL:${ENABLE_IMAGE}>:PNG::PNG>
	Threads::Threads # To match the build settings of the main app
	ZLIB::ZLIB
	)

export(TARGETS engine FILE EngineConfig.cmake)
