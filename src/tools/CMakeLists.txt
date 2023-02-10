###########################################################################
#   fheroes2: https://github.com/ihhub/fheroes2                           #
#   Copyright (C) 2022 - 2023                                             #
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

add_compile_options("$<$<COMPILE_LANG_AND_ID:C,AppleClang,Clang,GNU>:${GNU_CC_WARN_OPTS}>")
add_compile_options("$<$<COMPILE_LANG_AND_ID:CXX,AppleClang,Clang,GNU>:${GNU_CXX_WARN_OPTS}>")
add_compile_options("$<$<OR:$<COMPILE_LANG_AND_ID:C,MSVC>,$<COMPILE_LANG_AND_ID:CXX,MSVC>>:${MSVC_CC_WARN_OPTS}>")

if(ENABLE_STRICT_COMPILATION)
	add_compile_options($<$<OR:$<COMPILE_LANG_AND_ID:C,AppleClang,Clang,GNU>,$<COMPILE_LANG_AND_ID:CXX,AppleClang,Clang,GNU>>:-Werror>)
	add_compile_options($<$<OR:$<COMPILE_LANG_AND_ID:C,MSVC>,$<COMPILE_LANG_AND_ID:CXX,MSVC>>:/WX>)
endif(ENABLE_STRICT_COMPILATION)

# MSVC: suppress deprecation warnings
add_compile_definitions($<$<OR:$<COMPILE_LANG_AND_ID:C,MSVC>,$<COMPILE_LANG_AND_ID:CXX,MSVC>>:_CRT_SECURE_NO_WARNINGS>)
add_compile_definitions($<$<CONFIG:Debug>:WITH_DEBUG>)

add_executable(82m2wav 82m2wav.cpp)
add_executable(bin2txt bin2txt.cpp)
add_executable(extractor extractor.cpp)
add_executable(icn2img icn2img.cpp)
add_executable(til2img til2img.cpp)
add_executable(xmi2midi xmi2midi.cpp)

target_link_libraries(82m2wav engine)
target_link_libraries(bin2txt engine)
target_link_libraries(extractor engine)
target_link_libraries(icn2img engine)
target_link_libraries(til2img engine)
target_link_libraries(xmi2midi engine)
