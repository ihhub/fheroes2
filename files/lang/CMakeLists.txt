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

file(GLOB_RECURSE ALL_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp)
file(GLOB PO_FILES CONFIGURE_DEPENDS RELATIVE ${CMAKE_CURRENT_LIST_DIR} *.po)
list(APPEND MO_FILES ${PO_FILES})
list(TRANSFORM MO_FILES REPLACE ".po" ".mo")
list(APPEND CP1250 cs.po pl.po)
list(APPEND CP1251 be.po bg.po ru.po uk.po)
list(APPEND CP1252 de.po es.po it.po nb.po pt.po sv.po)
list(APPEND ASCII hu.po lt.po nl.po tr.po)

# Try to detect whether we're running in bash
if(MSVC OR (WIN32 AND CMAKE_GENERATOR MATCHES "Ninja"))
	set(FR \"y/\\xC3\\xA0\\xC3\\xA2\\xC3\\xA7\\xC3\\xA9\\xC3\\xA8\\xC3\\xAA\\xC3\\xAE\\xC3\\xAF\\xC3\\xB4\\xC3\\xB9\\xC3\\xBB\\xC3\\xBC\\xC3\\x89\\xC3\\x8A\\xC3\\x80/@*^~`|><\#\&\$\$uEEA/\\\\\; s/\\xC5\\x93/oe/g\")
	set(TRANSLIT \"y/\\xC3\\xA4\\xC3\\xAB\\xC3\\xAF\\xC3\\xB6\\xC5\\x91\\xC3\\xBC\\xC5\\xB1\\xC3\\x84\\xC3\\x8B\\xC3\\x8F\\xC5\\x90\\xC3\\x96\\xC3\\x9C\\xC5\\xB0/aeioouuAEIOOUU/\")
else()
	set(FR \'y/àâçéèêîïôùûüÉÊÀ/@*^~`|><\#&$$uEEA/\\\\\; s/œ/oe/g\')
	set(TRANSLIT \'y/äëïöőüűÄËÏŐÖÜŰ/aeioouuAEIOOUU/\')
endif()

add_custom_target(
	translations
	ALL
	DEPENDS ${MO_FILES}
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	)

add_custom_command(
	COMMAND ${XGETTEXT_EXECUTABLE} -d ${PROJECT_NAME} -C -F -k_ -k_n:1,2 -o ${PROJECT_NAME}.pot ${ALL_SOURCES}
	COMMAND ${SED_EXECUTABLE} -i -e \'s/, c-format//\' ${PROJECT_NAME}.pot
	OUTPUT ${PROJECT_NAME}.pot
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	)

foreach(PO MO IN ZIP_LISTS PO_FILES MO_FILES)
	if(${PO} IN_LIST CP1250)
		set(CHARSET "CP1250")
		set(SED_SCRIPT '1,20 s/UTF-8/${CHARSET}/')
	elseif(${PO} IN_LIST CP1251)
		set(CHARSET "CP1251")
		set(SED_SCRIPT '1,20 s/UTF-8/${CHARSET}/')
	elseif(${PO} IN_LIST CP1252)
		set(CHARSET "CP1252")
		set(SED_SCRIPT '1,20 s/UTF-8/${CHARSET}/')
	elseif(${PO} STREQUAL "ro.po")
		set(CHARSET "ISO-8859-16")
		set(SED_SCRIPT '1,20 s/UTF-8/${CHARSET}/')
	elseif(${PO} STREQUAL "fr.po")
		set(CHARSET "UTF-8")
		set(SED_SCRIPT ${FR})
	else()
		set(CHARSET "ascii//TRANSLIT")
		set(SED_SCRIPT ${TRANSLIT})
	endif()

	file(STRINGS ${CMAKE_CURRENT_LIST_DIR}/${PO} LANGUAGE LIMIT_COUNT 1 REGEX "translation")
	string(REGEX REPLACE "# (.*) translation.*" "\\1 Translation" LANGUAGE "${LANGUAGE}")

    add_custom_command(
	COMMAND msgmerge -q -o ${PO} --no-location ${CMAKE_CURRENT_LIST_DIR}/${PO} ${PROJECT_NAME}.pot
	DEPENDS ${PROJECT_NAME}.pot
	COMMENT ""
	OUTPUT "${LANGUAGE} Source"
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	)

    add_custom_command(
	COMMAND ${SED_EXECUTABLE} -i -e ${SED_SCRIPT} ${PO}
	COMMAND ${ICONV_EXECUTABLE} -f UTF-8 -t ${CHARSET} ${PO} > ${PO}.new
	COMMAND ${CMAKE_COMMAND} -E rename ${PO}.new ${PO}
	COMMAND msgfmt ${PO} -o ${MO}
	DEPENDS "${LANGUAGE} Source"
	COMMENT "Generating ${LANGUAGE}"
	OUTPUT ${MO}
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	)
endforeach()

install(
	DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/
	DESTINATION ${FHEROES2_DATA}/lang
	COMPONENT translations
	FILES_MATCHING
	PATTERN *.mo
	PATTERN CMakeFiles EXCLUDE
	)
