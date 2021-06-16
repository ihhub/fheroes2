file(GLOB_RECURSE FHEROES2_SOURCES CONFIGURE_DEPENDS *.cpp)

if(MINGW)
	set(MINGW_LIBRARIES mingw32 winmm)
endif(MINGW)

add_executable(fheroes2 ${FHEROES2_SOURCES})
target_compile_definitions(fheroes2 PRIVATE
	$<$<BOOL:${ENABLE_AUDIOCD}>:WITH_AUDIOCD>
	$<$<BOOL:${ENABLE_RELEASE}>:BUILD_RELEASE>
	$<$<BOOL:${ENABLE_UNICODE}>:WITH_TTF>
	$<$<BOOL:${ENABLE_UNICODE}>:WITH_ICONV>
	)
target_include_directories(fheroes2 PRIVATE
	agg
	ai
	army
	battle
	campaign
	castle
	dialog
	game
	gui
	heroes
	image
	kingdom
	maps
	monster
	objects
	pocketpc
	resource
	spell
	system
	world
	)
target_link_libraries(fheroes2
	${MINGW_LIBRARIES}  # Be sure define it first. Beware WinMain@16 error!!
	${SDL_MIXER_LIBRARIES}
	engine
	Threads::Threads
	ZLIB::ZLIB
	)

install(TARGETS fheroes2 DESTINATION ${CMAKE_INSTALL_BINDIR})
