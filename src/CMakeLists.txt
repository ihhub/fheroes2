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

if (MSVC)
	add_compile_options(/W4)
else()
	add_compile_options(
		-pedantic -Wall -Wextra -Wcast-align -Wctor-dtor-privacy -Wdouble-promotion
		-Wfloat-conversion -Wfloat-equal -Winit-self -Woverloaded-virtual -Wredundant-decls
		-Wshadow -Wundef -Wuninitialized -Wunused
		)
endif()

if(ENABLE_STRICT_COMPILATION)
	if (MSVC)
		add_compile_options(/WX)
	else()
		add_compile_options(-Werror)
	endif()
endif()

add_compile_definitions($<$<CONFIG:Debug>:WITH_DEBUG>)

add_subdirectory(thirdparty)
add_subdirectory(engine)
add_subdirectory(fheroes2)
if(ENABLE_TOOLS)
	add_subdirectory(tools)
endif()
