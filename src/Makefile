###########################################################################
#   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  #
#   Copyright (C) 2021 - 2022                                             #
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

#
# Common build options for fheroes2 and third-party libraries
#

# Common flags for both C and C++ compilers
CCFLAGS := -fsigned-char -pthread
# Flags for the C compiler
CFLAGS := $(CFLAGS)
# Flags for the C++ compiler
CXXFLAGS := $(CXXFLAGS) -std=c++11
# Flags for the linker
LDFLAGS := $(LDFLAGS) -pthread
LIBS := -lz

ifdef FHEROES2_WITH_DEBUG
CCFLAGS := $(CCFLAGS) -O0 -g -DWITH_DEBUG
else
CCFLAGS := $(CCFLAGS) -O3
endif

#
# Build options for third-party libraries
#

CCFLAGS_TP := $(CCFLAGS)
CFLAGS_TP := $(CFLAGS)
CXXFLAGS_TP := $(CXXFLAGS)

#
# Build options for fheroes2
#

# Add -Wconversion -Wsign-conversion -Weffc++ -Wextra-semi flags back once we fix all other warnings!
CCFLAGS := $(CCFLAGS) -pedantic -Wall -Wextra -Wdouble-promotion -Wfloat-conversion -Wfloat-equal -Woverloaded-virtual -Wredundant-decls -Wshadow -Wuninitialized -Wunused

ifdef FHEROES2_STRICT_COMPILATION
CCFLAGS := $(CCFLAGS) -Werror
endif

ifdef FHEROES2_WITH_SDL1
SDL_LIBS := $(shell sdl-config --libs)
SDL_FLAGS := $(shell sdl-config --cflags)
else
SDL_LIBS := $(shell sdl2-config --libs)
SDL_FLAGS := $(shell sdl2-config --cflags)
endif

ifdef FHEROES2_WITH_SDL1
SDL_LIBS := $(SDL_LIBS) -lSDL_mixer
else
SDL_LIBS := $(SDL_LIBS) -lSDL2_mixer
endif

ifdef FHEROES2_WITH_IMAGE
CCFLAGS := $(CCFLAGS) -DWITH_IMAGE $(shell libpng-config --cflags)
ifdef FHEROES2_WITH_SDL1
SDL_LIBS := $(SDL_LIBS) -lSDL_image $(shell libpng-config --libs)
else
SDL_LIBS := $(SDL_LIBS) -lSDL2_image $(shell libpng-config --libs)
endif
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
endif
endif

ifdef FHEROES2_VITA
PLATFORM := vita
endif

include Makefile.$(PLATFORM)

CCFLAGS := $(CCFLAGS) $(SDL_FLAGS)
LIBS := $(SDL_LIBS) $(LIBS)

export CC CXX AR LINK WINDRES CCFLAGS CFLAGS CXXFLAGS LDFLAGS LIBS PLATFORM

.PHONY: all clean

all:
	$(MAKE) -C thirdparty/libsmacker CCFLAGS="$(CCFLAGS_TP)" CFLAGS="$(CFLAGS_TP)" CXXFLAGS="$(CXXFLAGS_TP)"
	$(MAKE) -C engine
	$(MAKE) -C dist
ifdef FHEROES2_WITH_TOOLS
	$(MAKE) -C tools
endif
	$(MAKE) -C dist pot

clean:
	$(MAKE) -C thirdparty/libsmacker clean
	$(MAKE) -C tools clean
	$(MAKE) -C dist clean
	$(MAKE) -C engine clean
