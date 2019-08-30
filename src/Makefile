# makefile
# project: Free Heroes2
#

TARGET := fheroes2

ifdef WITH_SDL2
SDL_LIBS := $(shell sdl2-config --libs)
SDL_FLAGS := $(shell sdl2-config --cflags)
else
SDL_LIBS := $(shell sdl-config --libs)
SDL_FLAGS := $(shell sdl-config --cflags)
endif

CFLAGS := $(CFLAGS) -Wall -fsigned-char
LDFLAGS := $(LDFLAGS)
LIBS :=

ifdef DEBUG
CFLAGS := $(CFLAGS) -O0 -g -pedantic -DWITH_DEBUG
else
CFLAGS := -O2 $(CFLAGS)
endif

ifndef WITHOUT_MIXER
CFLAGS := $(CFLAGS) -DWITH_MIXER
ifdef WITH_SDL2
SDL_LIBS := $(SDL_LIBS) -lSDL2_mixer
else
SDL_LIBS := $(SDL_LIBS) -lSDL_mixer
endif
endif

ifndef WITHOUT_IMAGE
CFLAGS := $(CFLAGS) -DWITH_IMAGE $(shell libpng-config --cflags) -DWITH_ZLIB
ifdef WITH_SDL2
SDL_LIBS := $(SDL_LIBS) -lSDL2_image $(shell libpng-config --libs) -lz
else
SDL_LIBS := $(SDL_LIBS) -lSDL_image $(shell libpng-config --libs) -lz
endif
endif

ifndef WITHOUT_UNICODE
CFLAGS := $(CFLAGS) -DWITH_TTF
ifdef WITH_SDL2
SDL_LIBS := $(SDL_LIBS) -lSDL2_ttf
else
SDL_LIBS := $(SDL_LIBS) -lSDL_ttf
endif
endif

#ifndef WITHOUT_NETWORK
#CFLAGS := $(CFLAGS) -DWITH_NET_DISABLE
#ifdef WITH_SDL2
#SDL_LIBS := $(SDL_LIBS) -lSDL2_net
#else
#SDL_LIBS := $(SDL_LIBS) -lSDL_net
#endif
#endif

ifndef WITHOUT_XML
CFLAGS := $(CFLAGS) -DWITH_XML
endif

ifndef WITHOUT_ZLIB
CFLAGS := $(CFLAGS) -DWITH_ZLIB
LIBS := $(LIBS) -lz
endif

ifdef RELEASE
CFLAGS := $(CFLAGS) -DBUILD_RELEASE
endif

CFLAGS := $(SDL_FLAGS) $(CFLAGS)
LIBS := $(SDL_LIBS) $(LIBS)

# platform specific flags
ifndef PLATFORM
ifndef OS
OS := $(shell uname)
endif

ifeq ($(OS),Windows_NT)
PLATFORM := mingw
endif
ifeq ($(OS),FreeBSD)
PLATFORM := bsd
endif
ifeq ($(OS),Darwin)
PLATFORM := osx
endif
ifeq ($(OS),Linux)
PLATFORM := all
endif
ifeq ($(OS),Haiku)
PLATFORM := all
LIBS := -liconv
endif
endif

include Makefile.$(PLATFORM)

export CXX AR LINK WINDRES LDFLAGS CFLAGS LIBS PLATFORM

.PHONY: clean

all:
ifndef WITHOUT_XML
	$(MAKE) -C xmlccwrap
endif
	$(MAKE) -C engine
	$(MAKE) -C dist
ifdef WITH_TOOLS
	$(MAKE) -C tools
endif
ifndef WITHOUT_UNICODE
	$(MAKE) -C dist pot
endif

clean:
	$(MAKE) -C xmlccwrap clean
	$(MAKE) -C tools clean
	$(MAKE) -C dist clean
	$(MAKE) -C engine clean
