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

TARGET := fheroes2

ifdef DEBUG
CFLAGS := $(CFLAGS) -O0 -g -pedantic -DWITH_DEBUG
else
CFLAGS := -O3 $(CFLAGS)
endif

CFLAGS := $(CFLAGS) -fsigned-char
CXXFLAGS := -std=c++11 $(CFLAGS)
LDFLAGS := $(LDFLAGS)
LIBS := -pthread

CFLAGS_TP := $(CFLAGS)
CXXFLAGS_TP := $(CXXFLAGS)

# Add -Wconversion -Wsign-conversion -Weffc++ -Woverloaded-virtual -Wextra-semi flags back once we fix all other warnings!
CFLAGS := $(CXXFLAGS) -Wall -Wextra -Wpedantic -Wfloat-conversion -Wshadow -Wfloat-equal -Wredundant-decls -Wdouble-promotion -Wunused -Wuninitialized

ifdef FHEROES2_STRICT_COMPILATION
CFLAGS := $(CFLAGS) -Werror
endif

ifdef FHEROES2_SDL1
SDL_LIBS := $(shell sdl-config --libs)
SDL_FLAGS := $(shell sdl-config --cflags)
else
SDL_LIBS := $(shell sdl2-config --libs)
SDL_FLAGS := $(shell sdl2-config --cflags)
endif

ifdef FHEROES2_SDL1
SDL_LIBS := $(SDL_LIBS) -lSDL_mixer
else
SDL_LIBS := $(SDL_LIBS) -lSDL2_mixer
endif

ifdef FHEROES2_IMAGE_SUPPORT
CFLAGS := $(CFLAGS) -DWITH_IMAGE $(shell libpng-config --cflags) -DWITH_ZLIB
ifdef FHEROES2_SDL1
SDL_LIBS := $(SDL_LIBS) -lSDL_image $(shell libpng-config --libs) -lz
else
SDL_LIBS := $(SDL_LIBS) -lSDL2_image $(shell libpng-config --libs) -lz
endif
endif

ifndef WITHOUT_UNICODE
CFLAGS := $(CFLAGS) -DWITH_TTF
ifdef FHEROES2_SDL1
SDL_LIBS := $(SDL_LIBS) -lSDL_ttf
else
SDL_LIBS := $(SDL_LIBS) -lSDL2_ttf
endif
endif

ifndef WITHOUT_ZLIB
CFLAGS := $(CFLAGS) -DWITH_ZLIB
LIBS := $(LIBS) -lz
endif

ifdef RELEASE
CFLAGS := $(CFLAGS) -DBUILD_RELEASE
endif

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

ifdef FHEROES2_VITA
PLATFORM := vita
endif

include Makefile.$(PLATFORM)

CFLAGS := $(SDL_FLAGS) $(CFLAGS)
LIBS := $(SDL_LIBS) $(LIBS)

export CC CXX AR LINK WINDRES LDFLAGS CFLAGS LIBS PLATFORM

.PHONY: clean

all:
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
	$(MAKE) -C thirdparty/libsmacker clean
	$(MAKE) -C tools clean
	$(MAKE) -C dist clean
	$(MAKE) -C engine clean
