# makefile
# project: Free Heroes2 Tools

ifndef WITH_TOOLS
ifdef FHEROES2_SDL1
LIBS   := $(shell sdl-config --libs)
CFLAGS := $(shell sdl-config --cflags)
else
LIBS   := $(shell sdl2-config --libs)
CFLAGS := $(shell sdl2-config --cflags)
endif
ifndef WITHOUT_UNICODE
CFLAGS += -DWITH_ICONV
ifeq ($(OS),Windows_NT)
LIBS   += -liconv
endif
endif
ifeq ($(OS),Windows_NT)
LIBS   += -static-libgcc -static-libstdc++ -Wl,-Bstatic -lstdc++ -lwinpthread
endif
endif

ifeq ($(OS),Windows_NT)
LIBS   := -static $(subst -mwindows,-mconsole,$(LIBS))
CFLAGS := $(filter-out -Dmain=SDL_main,$(CFLAGS))
ifdef FHEROES2_SDL1
LIBS   += -lwinmm -lgdi32 -ldxguid
else
LIBS   += -lole32 -loleaut32 -limm32 -lwinmm -lgdi32 -dxguid -lsetupapi -lversion
endif
endif

TARGETS   := 82m2wav bin2txt extractor icn2img til2img xmi2mid
LIBENGINE := ../engine/libengine.a
LIBS      := $(LIBENGINE) $(LIBS)
CFLAGS    += -I../engine

all: $(TARGETS)

$(TARGETS):
	$(CXX) -c $@.cpp $(CFLAGS)
	$(CXX) -o $@ $@.o $(LIBS)

.PHONY: clean

clean:
	@rm -f *.o $(TARGETS)
