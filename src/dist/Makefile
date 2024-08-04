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

TARGET := fheroes2

LIBENGINE := ../engine/libengine.a
CCFLAGS := $(CCFLAGS) -I../engine

ifndef FHEROES2_WITH_SYSTEM_SMACKER
LIBENGINE := $(LIBENGINE) ../thirdparty/libsmacker/libsmacker.a
CCFLAGS := $(CCFLAGS) -I../thirdparty/libsmacker
endif

SOURCEROOT := ../fheroes2
SOURCEDIR  := $(filter %/,$(wildcard $(SOURCEROOT)/*/))
POT        := $(TARGET).pot

SEARCH     := $(wildcard $(SOURCEROOT)/*/*.cpp)

.PHONY: all clean pot

all: $(TARGET)

$(TARGET): $(notdir $(patsubst %.cpp, %.o, $(SEARCH))) $(LIBENGINE)
	@echo "lnk: $@"
	$(CXX) -o $@ $^ $(LIBS) $(LDFLAGS)

pot: $(wildcard $(SEARCH))
	@echo "gen: $(POT)"
	@xgettext -d $(TARGET) -C -F -k_ -k_n:1,2 -o $(POT) $(sort $(wildcard $(SEARCH)))
	@sed -i~ -e 's/, c-format//' $(POT)

VPATH := $(SOURCEDIR)

%.o: %.cpp
	$(CXX) -c -MD $(addprefix -I, $(SOURCEDIR)) $< $(CCFLAGS) $(CXXFLAGS) $(CPPFLAGS)

include $(wildcard *.d)

clean:
	rm -f *.pot *.pot~ *.o *.d *.exe $(TARGET)
	rm -rf *.app
