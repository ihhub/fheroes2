###########################################################################
#   fheroes2: https://github.com/ihhub/fheroes2                           #
#   Copyright (C) 2021 - 2024                                             #
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

#
# Common build options for fheroes2 and third-party libraries
#

# Common flags for both C and C++ compilers
CCFLAGS := -fsigned-char -pthread
# Flags for the C compiler
CFLAGS := $(CFLAGS)
# Flags for the C++ compiler
CXXFLAGS := $(CXXFLAGS)
# Flags for the preprocessor
CPPFLAGS := $(CPPFLAGS)
# Flags for the linker
LDFLAGS := $(LDFLAGS) -pthread
# Flags for additional libraries
LDLIBS := $(LDLIBS)

ifdef FHEROES2_WITH_DEBUG
CCFLAGS := $(CCFLAGS) -O0 -g
else
CCFLAGS := $(CCFLAGS) -O3
endif

ifneq ($(or $(FHEROES2_WITH_ASAN),$(FHEROES2_WITH_TSAN)),)
SANITIZERS := undefined

ifdef FHEROES2_WITH_ASAN
SANITIZERS := $(SANITIZERS),address
endif
ifdef FHEROES2_WITH_TSAN
SANITIZERS := $(SANITIZERS),thread
endif

CCFLAGS := $(CCFLAGS) -fsanitize=$(SANITIZERS)
LDFLAGS := $(LDFLAGS) -fsanitize=$(SANITIZERS)
endif

#
# Platform-specific build options
#

ifndef PLATFORM
ifndef OS
OS := $(shell uname)
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

include Makefile.$(PLATFORM)

#
# Build options for third-party libraries
#

CCFLAGS_TP := $(CCFLAGS)
CFLAGS_TP := $(CFLAGS)
CXXFLAGS_TP := $(CXXFLAGS)
CPPFLAGS_TP := $(CPPFLAGS)

#
# Build options for fheroes2
#

# *FLAGS_FH2 can be passed from platform-specific Makefiles
CCFLAGS := $(CCFLAGS) $(CCFLAGS_FH2)
CFLAGS := $(CFLAGS) $(CFLAGS_FH2)
CXXFLAGS := $(CXXFLAGS) $(CXXFLAGS_FH2) -std=c++17
CPPFLAGS := $(CPPFLAGS) $(CPPFLAGS_FH2)

ifdef FHEROES2_WITH_SYSTEM_SMACKER
LIBS := $(LIBS) -lsmacker
endif
LIBS := $(LIBS) -lz $(LDLIBS)

ifdef FHEROES2_WITH_DEBUG
CCFLAGS := $(CCFLAGS) -DWITH_DEBUG
endif
ifdef FHEROES2_WITH_IMAGE
CCFLAGS := $(CCFLAGS) -DWITH_IMAGE
endif
ifdef FHEROES2_DATA
CCFLAGS := $(CCFLAGS) -DFHEROES2_DATA="$(FHEROES2_DATA)"
endif

# TODO: Add -Wconversion -Wsign-conversion flags once we fix all the corresponding code smells
CCWARNOPTS := -pedantic -Wall -Wextra -Wcast-align -Wcast-qual -Wdouble-promotion -Wfloat-conversion -Wfloat-equal \
              -Wredundant-decls -Wshadow -Wswitch-default -Wundef -Wunused
CFLAGS := $(CFLAGS) $(CCWARNOPTS)
CXXFLAGS := $(CXXFLAGS) $(CCWARNOPTS) -Wctor-dtor-privacy -Wextra-semi -Wmissing-declarations -Wold-style-cast \
                                      -Woverloaded-virtual -Wsuggest-override

ifdef FHEROES2_STRICT_COMPILATION
CCFLAGS := $(CCFLAGS) -Werror
endif

#
# SDL-related build options
#

# SDL_FLAGS can already be defined in a platform-specific Makefile
ifeq ($(origin SDL_FLAGS),undefined)
SDL_FLAGS := $(shell sdl2-config --cflags)

ifdef FHEROES2_WITH_IMAGE
SDL_FLAGS := $(SDL_FLAGS) $(shell libpng-config --cflags)
endif
endif

# SDL_LIBS can already be defined in a platform-specific Makefile
ifeq ($(origin SDL_LIBS),undefined)
SDL_LIBS := -lSDL2_mixer $(shell sdl2-config --libs)

ifdef FHEROES2_WITH_IMAGE
SDL_LIBS := $(SDL_LIBS) -lSDL2_image $(shell libpng-config --libs)
endif
endif

CCFLAGS := $(CCFLAGS) $(SDL_FLAGS)
LIBS := $(SDL_LIBS) $(LIBS)

export CC CXX AR CCFLAGS CFLAGS CXXFLAGS CPPFLAGS LDFLAGS LIBS PLATFORM

.PHONY: all clean

all:
ifndef FHEROES2_WITH_SYSTEM_SMACKER
	$(MAKE) -C thirdparty/libsmacker CCFLAGS="$(CCFLAGS_TP)" CFLAGS="$(CFLAGS_TP)" CXXFLAGS="$(CXXFLAGS_TP)" CPPFLAGS="$(CPPFLAGS_TP)"
endif
	$(MAKE) -C engine
	$(MAKE) -C fheroes2
ifdef FHEROES2_WITH_TOOLS
	$(MAKE) -C tools
endif

clean:
ifndef FHEROES2_WITH_SYSTEM_SMACKER
	$(MAKE) -C thirdparty/libsmacker clean
endif
	$(MAKE) -C engine clean
	$(MAKE) -C fheroes2 clean
	$(MAKE) -C tools clean
