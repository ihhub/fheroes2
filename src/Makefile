# makefile
# project: Free Heroes of Might and Magic II (https://github.com/ihhub/fheroes2)

TARGET := fheroes2

ifdef DEBUG
CFLAGS := $(CFLAGS) -O0 -g -pedantic -DWITH_DEBUG
else
CFLAGS := -O3 $(CFLAGS)
endif

CFLAGS := $(CFLAGS) -fsigned-char
CXXFLAGS := -std=c++11 $(CFLAGS)
LDFLAGS := $(LDFLAGS)
LIBS :=

CFLAGS_TP := $(CFLAGS)
CXXFLAGS_TP := $(CXXFLAGS)

# Add -Wconversion -Wsign-conversion flags back once we fix all other warnings!
CFLAGS := $(CXXFLAGS) -Wall -Wextra -Wpedantic

ifdef FHEROES2_STRICT_COMPILATION
CFLAGS := $(CFLAGS) -Werror
endif

ifdef WITH_SDL2
SDL_LIBS := $(shell sdl2-config --libs)
SDL_FLAGS := $(shell sdl2-config --cflags)
else
SDL_LIBS := $(shell sdl-config --libs)
SDL_FLAGS := $(shell sdl-config --cflags)
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

ifndef WITHOUT_XML
CFLAGS := $(CFLAGS) -DWITH_XML
ifdef WITHOUT_BUNDLED_LIBS
CFLAGS := $(CFLAGS) $(shell pkg-config tinyxml --cflags)
LIBS := $(LIBS) $(shell pkg-config tinyxml --libs)
endif
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

export CC CXX AR LINK WINDRES LDFLAGS CFLAGS LIBS PLATFORM

.PHONY: clean

all:
ifndef WITHOUT_XML
ifndef WITHOUT_BUNDLED_LIBS
	$(MAKE) -C thirdparty/tinyxml CFLAGS="$(CXXFLAGS_TP)"
endif
endif
	$(MAKE) -C thirdparty/libsmacker CFLAGS="$(CFLAGS_TP)"
	$(MAKE) -C engine
	$(MAKE) -C dist
ifdef WITH_TOOLS
	$(MAKE) -C tools
endif
ifndef WITHOUT_UNICODE
	$(MAKE) -C dist pot
endif

clean:
	$(MAKE) -C thirdparty/tinyxml clean
	$(MAKE) -C thirdparty/libsmacker clean
	$(MAKE) -C tools clean
	$(MAKE) -C dist clean
	$(MAKE) -C engine clean
