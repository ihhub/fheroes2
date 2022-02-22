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

ifeq ($(PREFIX),)
PREFIX := /usr/local
endif

TARGET := fheroes2
LIBENGINE := ../engine/libengine.a
CFLAGS := $(CFLAGS) -I../engine

LIBENGINE := $(LIBENGINE) ../thirdparty/libsmacker/libsmacker.a
CFLAGS := $(CFLAGS) -I../thirdparty/libsmacker

ifdef PREFIX
CFLAGS := $(CFLAGS) -I$(PREFIX)/include
endif

ifeq ($(PLATFORM), mingw)
RES := ../resources/$(TARGET).res
RC  := ../resources/fheroes2.rc
ifdef DEBUG
DEBUGFLAG := -DDEBUG
endif
endif

SOURCEROOT := ../fheroes2
SOURCEDIR  := $(filter %/, $(wildcard $(SOURCEROOT)/*/)) $(filter %/, $(wildcard $(SOURCEROOT)/*/*/))
POT        := $(TARGET).pot

SEARCH     := $(wildcard $(SOURCEROOT)/*/*.cpp) $(wildcard $(SOURCEROOT)/*/*/*.cpp)

.PHONY: all clean pot

all: $(TARGET)

$(TARGET): $(notdir $(patsubst %.cpp, %.o, $(SEARCH))) $(LIBENGINE) $(RES)
	@echo "lnk: $@"
	$(CXX) $(LDFLAGS) $^ $(LIBS) -o $@

pot: $(wildcard $(SEARCH))
	@echo "gen: $(POT)"
	@xgettext -d $(TARGET) -C -F -k_ -k_n:1,2 -o $(POT) $(sort $(wildcard $(SEARCH)))
	@sed -i~ -e 's/, c-format//' $(POT)

$(RES): $(RC)
	@echo "gen: $@"
	$(WINDRES) -I../fheroes2/system $(DEBUGFLAG) $< -O coff -o $@

VPATH := $(SOURCEDIR)

%.o: %.cpp
	$(CXX) -c -MD $(addprefix -I, $(SOURCEDIR)) $< $(CFLAGS)

include $(wildcard *.d)

clean:
	rm -f *.pot *.pot~ *.o *.d *.exe $(TARGET) $(RES)
