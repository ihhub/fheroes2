if(USE_SYSTEM_LIBSMACKER)
	include(FindPkgConfig)

	pkg_check_modules(smacker REQUIRED smacker)
else()
	set(LIBSMACKER_SOURCES
		libsmacker/smacker.c
		libsmacker/smk_bitstream.c
		libsmacker/smk_hufftree.c
		)

	add_library(smacker STATIC ${LIBSMACKER_SOURCES})

	target_include_directories(smacker PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libsmacker>
		$<INSTALL_INTERFACE:include>
		)

	target_link_libraries(smacker
		Threads::Threads # To match the build settings of the main app
		)

	export(TARGETS smacker FILE SmackerConfig.cmake)
endif(USE_SYSTEM_LIBSMACKER)
