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

add_executable(82m2wav 82m2wav.cpp)
add_executable(bin2txt bin2txt.cpp)
add_executable(extractor extractor.cpp)
add_executable(icn2img icn2img.cpp)
add_executable(til2img til2img.cpp)
add_executable(xmi2mid xmi2mid_cli.cpp)

target_link_libraries(82m2wav
	engine
	)
target_link_libraries(bin2txt
	engine
	)
target_link_libraries(extractor
	engine
	)
target_link_libraries(icn2img
	engine
	)
target_link_libraries(til2img
	engine
	)
target_link_libraries(xmi2mid
	engine
	)
