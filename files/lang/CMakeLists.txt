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
file(GLOB PO_FILES RELATIVE ${CMAKE_CURRENT_LIST_DIR} *.po)
list(APPEND MO_FILES ${PO_FILES})
list(TRANSFORM MO_FILES REPLACE ".po" ".mo")
list(APPEND CP1250 cs.po pl.po)
list(APPEND CP1251 be.po bg.po ru.po uk.po)
list(APPEND CP1252 de.po nb.po sv.po it.po es.po pt.po)
list(APPEND ASCII hu.po lt.po nl.po tr.po)

add_custom_target(
	translations
	ALL
	DEPENDS ${MO_FILES}
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/files/lang
	)

add_custom_command(
	COMMAND ${XGETTEXT_EXECUTABLE} -d ${PROJECT_NAME} -C -F -k_ -k_n:1,2 -o ${PROJECT_NAME}.pot ${ALL_SOURCES}
	COMMAND ${SED_EXECUTABLE} -i -e \"s/, c-format//\" ${PROJECT_NAME}.pot
	DEPENDS fheroes2
	OUTPUT ${PROJECT_NAME}.pot
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/files/lang
	)

foreach(PO MO IN ZIP_LISTS PO_FILES MO_FILES)
	if(${PO} IN_LIST CP1250)
		set(CHARSET "CP1250")
	elseif(${PO} IN_LIST CP1251)
		set(CHARSET "CP1251")
	elseif(${PO} IN_LIST CP1252)
		set(CHARSET "CP1252")
	elseif(${PO} STREQUAL "ro.po")
		set(CHARSET "ISO-8859-16")
	else()
		set(CHARSET "UTF-8")
	endif()

	file(STRINGS ${CMAKE_CURRENT_LIST_DIR}/${PO} LANGUAGE LIMIT_COUNT 1 REGEX "translation")
	string(REGEX REPLACE "# (.*) translation.*" "\\1 Translation Update" LANGUAGE "${LANGUAGE}")

	set_source_files_properties(${LANGUAGE} PROPERTIES SYMBOLIC TRUE)

    add_custom_command(
	COMMAND msgmerge -o ${PO} --no-location ${CMAKE_CURRENT_LIST_DIR}/${PO} ${PROJECT_NAME}.pot
	DEPENDS ${PROJECT_NAME}.pot
	OUTPUT ${LANGUAGE}
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/files/lang
	)

	if(${CHARSET} STREQUAL "UTF-8")
		if(${PO} STREQUAL "fr.po")
			add_custom_command(
			COMMAND ${SED_EXECUTABLE} -i -e \'y/àâçéèêîïôùûüÉÊÀ/@*^~`|><\#uuuEEA/\; s/œ/oe/g\' ${PO}
			DEPENDS ${LANGUAGE}
			OUTPUT ${CMAKE_BINARY_DIR}/files/lang/${PO}
			WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/files/lang
			)
		else()
			add_custom_command(
			COMMAND ${SED_EXECUTABLE} -i -e \"y/äëïöőüűÄËÏŐÖÜŰ/aeioouuAEIOOUU/\" ${PO}
			COMMAND ${ICONV_EXECUTABLE} -f UTF-8 -t ascii//TRANSLIT ${PO} > ${PO}.new
			COMMAND ${CMAKE_COMMAND} -E rename ${PO}.new ${PO} 
			DEPENDS ${LANGUAGE}
			OUTPUT ${CMAKE_BINARY_DIR}/files/lang/${PO}
			WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/files/lang
			)
		endif()
	else()
		add_custom_command(
		COMMAND ${ICONV_EXECUTABLE} -f UTF-8 -t ${CHARSET} ${PO} > ${PO}.new
		COMMAND ${CMAKE_COMMAND} -E rename ${PO}.new ${PO}
		COMMAND ${SED_EXECUTABLE} -i -e \"s/UTF-8/${CHARSET}/\" ${PO}
		DEPENDS ${LANGUAGE}
		OUTPUT ${CMAKE_BINARY_DIR}/files/lang/${PO}
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/files/lang
		)
	endif()

    add_custom_command(
	COMMAND msgfmt ${PO} -o ${MO}
	DEPENDS ${CMAKE_BINARY_DIR}/files/lang/${PO}
	OUTPUT ${CMAKE_BINARY_DIR}/files/lang/${MO}
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/files/lang
	)
endforeach()

install(
	DIRECTORY ${CMAKE_BINARY_DIR}/files/lang/
	DESTINATION ${FHEROES2_DATA}/lang
	COMPONENT translations
	FILES_MATCHING
	PATTERN *.mo
	PATTERN CMakeFiles EXCLUDE
	)
