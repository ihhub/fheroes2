file(GLOB_RECURSE FHEROES2_SOURCES CONFIGURE_DEPENDS *.cpp)

if(MINGW)
	set(MINGW_LIBRARIES mingw32 winmm)
endif(MINGW)

get_filename_component(CONFIGURE_FHEROES2_DATA_ABSOLUTE
	${CONFIGURE_FHEROES2_DATA} ABSOLUTE
	BASE_DIR ${CMAKE_INSTALL_PREFIX}
	CACHE
	)

if(MACOS_APP_BUNDLE)
	set(fheroes2Icon "${CMAKE_CURRENT_SOURCE_DIR}/../resources/fheroes2.icns")
	set_source_files_properties(${fheroes2Icon} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
	add_executable(fheroes2 MACOSX_BUNDLE ${fheroes2Icon} ${FHEROES2_SOURCES})
	set_target_properties(fheroes2 PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "${CMAKE_PROJECT_NAME}")
	set_target_properties(fheroes2 PROPERTIES MACOSX_BUNDLE_EXECUTABLE_NAME "${MACOSX_BUNDLE_BUNDLE_NAME}")
	set_target_properties(fheroes2 PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/../resources/Info.plist.in")
	set_target_properties(fheroes2 PROPERTIES MACOSX_BUNDLE_ICON_FILE "fheroes2.icns")
	set_target_properties(fheroes2 PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "com.horns.and.hoovers.${CMAKE_PROJECT_NAME}")
	set_target_properties(fheroes2 PROPERTIES MACOSX_BUNDLE_SHORT_VERSION_STRING "${CMAKE_PROJECT_VERSION}")
	set_target_properties(fheroes2 PROPERTIES MACOSX_BUNDLE_BUNDLE_VERSION "${CMAKE_PROJECT_VERSION}")
	set_target_properties(fheroes2 PROPERTIES OUTPUT_NAME "fheroes2")
else(MACOS_APP_BUNDLE)
	add_executable(fheroes2 ${FHEROES2_SOURCES})
endif(MACOS_APP_BUNDLE)

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

if(NOT MACOS_APP_BUNDLE)
	install(TARGETS fheroes2 DESTINATION ${CMAKE_INSTALL_BINDIR})
endif(NOT MACOS_APP_BUNDLE)
