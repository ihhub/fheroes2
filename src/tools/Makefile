# makefile
# project: Free Heroes2 Tools

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
