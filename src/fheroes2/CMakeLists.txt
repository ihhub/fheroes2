file(GLOB_RECURSE FHEROES2_SOURCES CONFIGURE_DEPENDS *.cpp)

if(MINGW)
	set(MINGW_LIBRARIES mingw32 winmm)
endif(MINGW)

get_filename_component(CONFIGURE_FHEROES2_DATA_ABSOLUTE
	${CONFIGURE_FHEROES2_DATA} ABSOLUTE
	BASE_DIR ${CMAKE_INSTALL_PREFIX}
	CACHE
	)

add_executable(fheroes2 ${FHEROES2_SOURCES})
target_compile_definitions(fheroes2 PRIVATE
	CONFIGURE_FHEROES2_DATA=${CONFIGURE_FHEROES2_DATA_ABSOLUTE}
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
	h2d
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
