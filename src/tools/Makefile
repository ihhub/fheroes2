# makefile
# project: Free Heroes2 Tools
#

ifdef WITH_SDL2
SDL_LIBS := $(shell sdl2-config --libs)
SDL_FLAGS := $(shell sdl2-config --cflags)
else
SDL_LIBS := $(shell sdl-config --libs)
SDL_FLAGS := $(shell sdl-config --cflags)
endif

TARGETS := extractor 82m2wav til2img icn2img xmi2mid bin2txt
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
