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
	export(TARGETS smacker FILE SmackerConfig.cmake)
endif(USE_SYSTEM_LIBSMACKER)

if(ENABLE_XML)
	if(USE_SYSTEM_TINYXML)
		include(FindPkgConfig)
		pkg_check_modules(tinyxml REQUIRED tinyxml)
	else()
		set(LIBTINYXML_SOURCES
			tinyxml/tinystr.cpp
			tinyxml/tinyxml.cpp
			tinyxml/tinyxmlerror.cpp
			tinyxml/tinyxmlparser.cpp
			)
		add_library(tinyxml STATIC ${LIBTINYXML_SOURCES})
		target_include_directories(tinyxml PUBLIC
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/tinyxml>
			$<INSTALL_INTERFACE:include>
			)
		export(TARGETS tinyxml FILE TinyXMLConfig.cmake)
	endif(USE_SYSTEM_TINYXML)
endif(ENABLE_XML)
