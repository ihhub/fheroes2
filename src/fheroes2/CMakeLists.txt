file(GLOB_RECURSE FHEROES2_SOURCES CONFIGURE_DEPENDS *.cpp)

if(MINGW)
	set(MINGW_LIBRARIES mingw32 winmm)
endif(MINGW)

if(MACOS_APP_BUNDLE)
	set(fheroes2Icon "${CMAKE_CURRENT_SOURCE_DIR}/../resources/fheroes2.icns")
	set_source_files_properties(${fheroes2Icon} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
	add_executable(fheroes2 MACOSX_BUNDLE ${fheroes2Icon} ${FHEROES2_SOURCES} ${TRANSLATION_DATA})
	target_compile_definitions(fheroes2 PRIVATE
		$<$<BOOL:${MACOS_APP_BUNDLE}>:MACOS_APP_BUNDLE>
		)
	target_link_libraries(fheroes2 "-framework CoreFoundation")

	set_target_properties(fheroes2 PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "${CMAKE_PROJECT_NAME}")
	set_target_properties(fheroes2 PROPERTIES MACOSX_BUNDLE_EXECUTABLE_NAME "${MACOSX_BUNDLE_BUNDLE_NAME}")
	set_target_properties(fheroes2 PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/../resources/Info.plist.in")
	set_target_properties(fheroes2 PROPERTIES MACOSX_BUNDLE_ICON_FILE "fheroes2.icns")
	set_target_properties(fheroes2 PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "com.fheroes2.${CMAKE_PROJECT_NAME}")
	set_target_properties(fheroes2 PROPERTIES MACOSX_BUNDLE_SHORT_VERSION_STRING "${CMAKE_PROJECT_VERSION}")
	set_target_properties(fheroes2 PROPERTIES MACOSX_BUNDLE_BUNDLE_VERSION "${CMAKE_PROJECT_VERSION}")
	set_target_properties(fheroes2 PROPERTIES OUTPUT_NAME "${CMAKE_PROJECT_NAME}")
else(MACOS_APP_BUNDLE)
	get_filename_component(CONFIGURE_FHEROES2_DATA_ABSOLUTE
		${CONFIGURE_FHEROES2_DATA} ABSOLUTE
		BASE_DIR ${CMAKE_INSTALL_PREFIX}
		CACHE
		)

	add_executable(fheroes2 ${FHEROES2_SOURCES})
	target_compile_definitions(fheroes2 PRIVATE
		CONFIGURE_FHEROES2_DATA=${CONFIGURE_FHEROES2_DATA_ABSOLUTE}
		)
endif(MACOS_APP_BUNDLE)

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
