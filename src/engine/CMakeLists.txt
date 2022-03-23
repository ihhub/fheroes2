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
