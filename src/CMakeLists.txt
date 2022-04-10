###########################################################################
#   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  #
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

set(
	GNU_CC_WARN_OPTS
	-pedantic -Wall -Wextra -Wdouble-promotion -Wfloat-conversion -Wfloat-equal -Winit-self
	-Wredundant-decls -Wshadow -Wundef -Wuninitialized -Wunused
	)
set(
	GNU_CXX_WARN_OPTS
	${GNU_CC_WARN_OPTS} -Wctor-dtor-privacy -Woverloaded-virtual
	)

add_compile_options($<$<OR:$<COMPILE_LANG_AND_ID:C,MSVC>,$<COMPILE_LANG_AND_ID:CXX,MSVC>>:/W4>)
add_compile_options("$<$<COMPILE_LANG_AND_ID:C,AppleClang,Clang,GNU>:${GNU_CC_WARN_OPTS}>")
add_compile_options("$<$<COMPILE_LANG_AND_ID:CXX,AppleClang,Clang,GNU>:${GNU_CXX_WARN_OPTS}>")

if(ENABLE_STRICT_COMPILATION)
	add_compile_options($<$<OR:$<COMPILE_LANG_AND_ID:C,MSVC>,$<COMPILE_LANG_AND_ID:CXX,MSVC>>:/WX>)
	add_compile_options($<$<OR:$<COMPILE_LANG_AND_ID:C,AppleClang,Clang,GNU>,$<COMPILE_LANG_AND_ID:CXX,AppleClang,Clang,GNU>>:-Werror>)
endif(ENABLE_STRICT_COMPILATION)

add_compile_definitions($<$<CONFIG:Debug>:WITH_DEBUG>)

add_subdirectory(thirdparty)
add_subdirectory(engine)
add_subdirectory(fheroes2)
if(ENABLE_TOOLS)
	add_subdirectory(tools)
endif(ENABLE_TOOLS)
