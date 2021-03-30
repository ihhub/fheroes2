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

ifdef FHEROES2_SDL1
SDL_LIBS := $(shell sdl-config --libs)
SDL_FLAGS := $(shell sdl-config --cflags)
else
SDL_LIBS := $(shell sdl2-config --libs)
SDL_FLAGS := $(shell sdl2-config --cflags)
endif

TARGETS := extractor 82m2wav til2img icn2img xmi2mid_cli bin2txt
LIBENGINE := ../engine/libengine.a
LIBS := $(LIBENGINE) $(SDL_LIBS) $(LIBS)
CFLAGS := $(SDL_FLAGS) $(CFLAGS) -I../engine

all: $(TARGETS)

$(TARGETS): $(addsuffix .cpp, $(TARGETS)) $(LIBENGINE)
	$(CXX) -c $@.cpp $(CFLAGS)
	$(CXX) -o $@ $@.o $(LIBS)

.PHONY: clean

clean:
	rm -f *.o *.exe $(TARGETS)
