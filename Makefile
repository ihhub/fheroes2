###########################################################################
#   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  #
#   Copyright (C) 2021                                                    #
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

# Options:
# DEBUG: build in debug mode
# RELEASE: build with addons extensions
#
# WITHOUT_ZLIB: build without zlib (disable compressed save files)
# WITHOUT_MIXER: build without SDL_mixer library
# WITHOUT_AUDIOCD: disable audio CD support
# WITHOUT_UNICODE: build without unicode (disable translation and ttf font)
# FHEROES2_IMAGE_SUPPORT: build with SDL image support
# WITHOUT_XML: skip build tinyxml, used for load alt. resources
# WITH_TOOLS: build tools
# WITHOUT_BUNDLED_LIBS: do not build XML third party library
# FHEROES2_STRICT_COMPILATION: build with strict compilation option (makes warnings into errors)
#
# -DCONFIGURE_FHEROES2_LOCALEDIR: system locale dir
# -DCONFIGURE_FHEROES2_DATA: system fheroes2 game dir

TARGET	:= fheroes2

all:
	$(MAKE) -C src
	@cp src/dist/$(TARGET) .

clean:
	$(MAKE) -C src clean
	@rm -f ./$(TARGET)
