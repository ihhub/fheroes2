# makefile
# project: Free Heroes2
#

ifeq ($(PREFIX),)
PREFIX := /usr/local
endif

TARGET := fheroes2
LIBENGINE := ../engine/libengine.a
CFLAGS := $(CFLAGS) -I../engine

ifndef WITHOUT_XML
LIBENGINE := $(LIBENGINE) ../xmlccwrap/libxmlccwrap.a
CFLAGS := $(CFLAGS) -I../xmlccwrap
endif

ifdef PREFIX
CFLAGS := $(CFLAGS) -I$(PREFIX)/include
endif

RES	:=
ICOFILE	:= ../../image/fheroes2_32x32.ico

ifdef WITH_SVNREV
ifndef SVNVER
SVNVER	:= $(shell svnversion ..)
endif
CFLAGS	:= $(CFLAGS) -DSVN_REVISION="\"$(SVNVER)\""
endif

ifdef WITH_ICONS
RES	:= $(TARGET).res
endif

SOURCEROOT := ../fheroes2
SOURCEDIR  := $(shell ls $(SOURCEROOT))
POT        := $(TARGET).pot
SRCDIRLIST := $(addprefix $(SOURCEROOT)/, $(SOURCEDIR))

ifdef WITH_AI
SRCDIRLIST := $(SRCDIRLIST) $(SOURCEROOT)/ai/$(WITH_AI)
else
SRCDIRLIST := $(SRCDIRLIST) $(SOURCEROOT)/ai/empty
endif

SEARCH     := $(addsuffix /*.cpp, $(SRCDIRLIST))


all: $(TARGET)

$(TARGET): $(notdir $(patsubst %.cpp, %.o, $(wildcard $(SEARCH)))) $(LIBENGINE) $(RES)
	@echo "lnk: $@"
	$(CXX) $(LDFLAGS) $^ $(LIBS) -o $@

pot: $(wildcard $(SEARCH))
	@echo "gen: $(POT)"
	@xgettext -d $(TARGET) -C -k_ -o $(POT) $(wildcard $(SEARCH))

$(RES): $(ICOFILE)
	@echo "gen: $(RES)"
	@echo "$(IDICON) ICON \"$(ICOFILE)\"" > $(TARGET).rc
	$(WINDRES) $(TARGET).rc -O coff -o $(TARGET).res

VPATH := $(SRCDIRLIST)

%.o: %.cpp
	@echo "cxx: $@"
	@$(CXX) -c -MD $(addprefix -I, $(SRCDIRLIST)) $< $(CFLAGS)

include $(wildcard *.d)

.PHONY: clean

clean:
	rm -f *.pot *.o *.d *.rc *.res *.exe $(TARGET)
