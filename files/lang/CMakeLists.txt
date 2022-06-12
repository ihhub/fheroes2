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

file(GLOB_RECURSE ALL_SOURCES RELATIVE ${CMAKE_SOURCE_DIR}/src/fheroes2 ${CMAKE_SOURCE_DIR}/src/fheroes2/*.cpp)
file(GLOB PO_FILES RELATIVE ${CMAKE_CURRENT_LIST_DIR} *.po)
list(APPEND MO_FILES ${PO_FILES})
list(TRANSFORM MO_FILES REPLACE ".po" ".mo")
list(APPEND TRANSLIT hu.po lt.po nl.po tr.po)
list(APPEND CHARSETS "CP1251;CP1251;CP1250;CP1252;CP1252;UTF-8;ascii//TRANSLIT;CP1252;ascii//TRANSLIT;CP1252;ascii//TRANSLIT;CP1250;CP1252;ISO-8859-16;CP1251;CP1252;ascii//TRANSLIT;CP1251")

add_custom_target(
	translations
	ALL
	DEPENDS ${MO_FILES}
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	)

execute_process(
	COMMAND ${XGETTEXT_EXE} -d ${PROJECT_NAME} -C -F -k_ -k_n:1,2 -o - ${ALL_SOURCES}
	OUTPUT_VARIABLE POT
	ENCODING UTF-8
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/fheroes2
	)
string(REPLACE "#, c-format\n" "" POT "${POT}")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pot "${POT}")
# file(GENERATE OUTPUT ${PROJECT_NAME}.pot CONTENT "${POT}" NEWLINE_STYLE UNIX)

foreach(PO MO charset IN ZIP_LISTS PO_FILES MO_FILES CHARSETS)
	if(${PO} IN_LIST TRANSLIT)
		list(APPEND unicode  "ä;ë;ï;ö;ő;ü;ű;Ä;Ë;Ï;Ő;Ö;Ü;Ű")
		list(APPEND ansi     "a;e;i;o;o;u;u;A;E;I;O;O;U;U")
	elseif(${PO} STREQUAL "fr.po")
		list(APPEND unicode  "à;â;ç;é;è;ê;î;ï;ô;ù;û;ü;Ç;É;Ê;À;œ")
		list(APPEND ansi     "@;*;^;~;`;|;>;<;#;&;$;u;C;E;E;A;oe")
	endif()

	execute_process(
		COMMAND ${MSGMERGE_EXE} -q -o - --no-location ${CMAKE_CURRENT_LIST_DIR}/${PO} ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pot
		COMMAND_ECHO NONE
		OUTPUT_VARIABLE PO_CONTENT
		ENCODING UTF-8
		)
	if(${PO} IN_LIST TRANSLIT OR ${PO} STREQUAL "fr.po")
		foreach(u a IN ZIP_LISTS unicode ansi)
			string(REGEX REPLACE "${u}" "${a}" PO_CONTENT "${PO_CONTENT}")
		endforeach()
		unset(unicode)
		unset(ansi)
	else()
		string(REPLACE "UTF-8" "${charset}" PO_CONTENT "${PO_CONTENT}")
	endif()
	file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${PO} "${PO_CONTENT}")
	execute_process(
		COMMAND ${ICONV_EXE} -f UTF-8 -t ${charset} ${CMAKE_CURRENT_BINARY_DIR}/${PO}
		OUTPUT_VARIABLE PO_CONTENT
		ENCODING NONE
		)
	file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${PO} "${PO_CONTENT}")
	# file(GENERATE OUTPUT ${PO} CONTENT "${PO_CONTENT}" NEWLINE_STYLE UNIX)

	file(STRINGS ${PO} LANGUAGE LIMIT_COUNT 1 REGEX "translation")
	string(REGEX REPLACE "# (.* translation).*" "\\1" LANGUAGE "${LANGUAGE}")

    add_custom_command(
		COMMAND ${MSGFMT_EXE} ${PO} -o ${MO}
		COMMENT "Generating ${LANGUAGE}"
		DEPENDS ${PO}
		OUTPUT ${MO}
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		)
endforeach()

if(NOT MACOS_APP_BUNDLE)
	install(
		DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		DESTINATION ${FHEROES2_DATA}
		COMPONENT translations
		FILES_MATCHING
		PATTERN *.mo
		PATTERN CMakeFiles EXCLUDE
		)
endif(NOT MACOS_APP_BUNDLE)
