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
list(APPEND CP1250 cs.po pl.po)
list(APPEND CP1251 be.po bg.po ru.po uk.po)
list(APPEND CP1252 de.po es.po it.po nb.po pt.po sv.po)

if(MSVC OR CMAKE_GENERATOR MATCHES "Ninja")
	foreach(name SED_EXE ICONV_EXE XGETTEXT_EXE MSGMERGE_EXE MSGFMT_EXE)
		cmake_path(NATIVE_PATH ${name} ${name})
	endforeach()
endif()

file(GENERATE OUTPUT fr.sed
     CONTENT "y/àâçéèêîïôùûüÉÊÀ/@*^~`|><#&$uEEA/\ns/œ/oe/g"
     NEWLINE_STYLE UNIX)

file(GENERATE OUTPUT translit.sed
     CONTENT "y/äëïöőüűÄËÏŐÖÜŰ/aeioouuAEIOOUU/"
     NEWLINE_STYLE UNIX)

foreach(charset CP1250 CP1251 CP1252 ISO-8859-16)
	file(CONFIGURE OUTPUT ${charset}.sed
		CONTENT "1,20 s/UTF-8/@charset@/"
		@ONLY
		NEWLINE_STYLE UNIX)
endforeach()

add_custom_target(
	translations
	ALL
	DEPENDS ${MO_FILES}
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	)

execute_process(
	COMMAND ${XGETTEXT_EXE} -d ${PROJECT_NAME} -C -F -k_ -k_n:1,2 -o - ${ALL_SOURCES}
	COMMAND ${SED_EXE} -e \'/\#, c-format/d\'
	OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pot
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/fheroes2
	)

foreach(PO MO IN ZIP_LISTS PO_FILES MO_FILES)
	if(${PO} IN_LIST CP1250)
		set(CHARSET "CP1250")
		set(SCRIPT ${CHARSET}.sed)
	elseif(${PO} IN_LIST CP1251)
		set(CHARSET "CP1251")
		set(SCRIPT ${CHARSET}.sed)
	elseif(${PO} IN_LIST CP1252)
		set(CHARSET "CP1252")
		set(SCRIPT ${CHARSET}.sed)
	elseif(${PO} STREQUAL "ro.po")
		set(CHARSET "ISO-8859-16")
		set(SCRIPT ${CHARSET}.sed)
	elseif(${PO} STREQUAL "fr.po")
		set(CHARSET "UTF-8")
		set(SCRIPT fr.sed)
	else()
		set(CHARSET "ascii//TRANSLIT")
		set(SCRIPT translit.sed)
	endif()

	file(STRINGS ${CMAKE_CURRENT_LIST_DIR}/${PO} LANGUAGE LIMIT_COUNT 1 REGEX "translation")
	string(REGEX REPLACE "# (.* translation).*" "\\1" LANGUAGE "${LANGUAGE}")

    add_custom_command(
	COMMAND ${MSGMERGE_EXE} -q -o ${PO} --no-location ${CMAKE_CURRENT_LIST_DIR}/${PO} ${PROJECT_NAME}.pot
	COMMAND ${SED_EXE} -i -f ${SCRIPT} ${PO}
	COMMAND ${ICONV_EXE} -f UTF-8 -t ${CHARSET} ${PO} > ${PO}.new
	COMMAND ${CMAKE_COMMAND} -E rename ${PO}.new ${PO}
	COMMAND ${MSGFMT_EXE} ${PO} -o ${MO}
	DEPENDS ${PROJECT_NAME}.pot ${CMAKE_CURRENT_LIST_DIR}/${PO}
	COMMENT "Generating ${LANGUAGE}"
	OUTPUT ${MO}
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	)
endforeach()

install(
	DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	DESTINATION ${FHEROES2_DATA}
	COMPONENT translations
	FILES_MATCHING
	PATTERN *.mo
	PATTERN CMakeFiles EXCLUDE
	)
