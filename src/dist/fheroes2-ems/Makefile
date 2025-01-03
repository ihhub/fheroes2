###########################################################################
#   fheroes2: https://github.com/ihhub/fheroes2                           #
#   Copyright (C) 2025                                                    #
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

DEPLIBS := ../engine/libengine.a
CCFLAGS := $(CCFLAGS) -I../../engine

ifndef FHEROES2_WITH_SYSTEM_SMACKER
DEPLIBS := $(DEPLIBS) ../thirdparty/libsmacker/libsmacker.a
CCFLAGS := $(CCFLAGS) -I../../thirdparty/libsmacker
endif

SOURCEROOT := ../../fheroes2
SOURCEDIRS := $(filter %/,$(wildcard $(SOURCEROOT)/*/))
SOURCES := $(wildcard $(SOURCEROOT)/*/*.cpp)

VPATH := $(SOURCEDIRS)

.PHONY: all clean

all: fheroes2.js

fheroes2.js: $(notdir $(patsubst %.cpp, %.o, $(SOURCES))) $(DEPLIBS)
	$(CXX) -o $@ $^ $(LIBS) $(LDFLAGS)

%.o: %.cpp
	$(CXX) -c -MD $< $(addprefix -I, $(SOURCEDIRS)) $(CCFLAGS) $(CXXFLAGS) $(CPPFLAGS)

include $(wildcard *.d)

clean:
	rm -f *.d *.o fheroes2.data fheroes2.js fheroes2.wasm
