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
SDL := sdl
else
SDL := sdl2
endif
# ifdef FHEROES2_IMAGE_SUPPORT
# CFLAGS := -DWITH_IMAGE $(shell libpng-config --cflags)
# LIBS   := -l$(shell X="${SDL}"; echo $${X^^})_image $(shell libpng-config --libs)
# endif

SDL_LIBS   := $(shell $(SDL)-config --libs)
SDL_CFLAGS := $(shell $(SDL)-config --cflags)

ifeq ($(OS),Windows_NT)
LIBS       := -static-libgcc -static-libstdc++ -Wl,-Bstatic -lstdc++ -lwinpthread
SDL_LIBS   := $(subst -mwindows,-mconsole,$(SDL_LIBS))
SDL_CFLAGS := $(filter-out -Dmain=SDL_main,$(SDL_CFLAGS))
ifdef FHEROES2_SDL1
SDL_LIBS   += -liconv -lwinmm -lgdi32 -ldxguid
else
SDL_LIBS   += -lole32 -loleaut32 -limm32 -lwinmm -lgdi32 -ldxguid -lsetupapi -lversion
endif
endif

TARGETS := 82m2wav bin2txt extractor icn2img til2img xmi2mid
AGG     := agg_file.o
IMAGE   := image.o
ITOOL   := image_tool.o
LOG     := logging.o
RECT    := rect.o
SER     := serialize.o
SYS     := system.o
TOOLS   := tools.o
XMI     := xmi2mid.o
PREREQS := $(AGG) $(IMAGE) $(ITOOL) $(LOG) $(RECT) $(SER) $(SYS) $(TOOLS) $(XMI)
CFLAGS  := -I../engine

all: $(TARGETS)

82m2wav:         $(SER) $(LOG)

extractor:       $(LOG) $(RECT) $(SER) $(SYS) $(TOOLS)

icn2img til2img: $(AGG) $(IMAGE) $(ITOOL) $(LOG) $(RECT) $(SER) $(SYS) $(TOOLS)

xmi2mid:         $(LOG) $(RECT) $(SER) $(TOOLS) $(XMI)

$(TARGETS):
	@echo "gen: $@"
	@g++ $@.cpp $(CFLAGS) $^ $(LIBS) $(SDL_LIBS) -o $@

$(PREREQS):
	@g++ -c $(@:%.o=../engine/%.cpp) $(SDL_CFLAGS) -o $@

.PHONY: clean

clean:
	@rm -f *.o $(TARGETS)
