# makefile
# project: Free Heroes2
#

ifeq ($(PREFIX),)
PREFIX := /usr/local
endif

TARGET := fheroes2
LIBENGINE := ../engine/libengine.a
CFLAGS := $(CFLAGS) -I../engine

LIBENGINE := $(LIBENGINE) ../thirdparty/libsmacker/libsmacker.a
CFLAGS := $(CFLAGS) -I../thirdparty/libsmacker

ifndef WITHOUT_XML
ifndef WITHOUT_BUNDLED_LIBS
LIBENGINE := $(LIBENGINE) ../thirdparty/tinyxml/libtinyxml.a
CFLAGS := $(CFLAGS) -I../thirdparty/tinyxml
endif
endif

ifdef PREFIX
CFLAGS := $(CFLAGS) -I$(PREFIX)/include
endif

ifeq ($(PLATFORM), mingw)
RES := ../../files/$(TARGET).res
RC  := ../../files/fheroes2.rc
ifdef DEBUG
DEBUGFLAG := -DDEBUG
endif
endif

SOURCEROOT := ../fheroes2
SOURCEDIR  := $(shell ls $(SOURCEROOT))
POT        := $(TARGET).pot
SRCDIRLIST := $(addprefix $(SOURCEROOT)/, $(SOURCEDIR))

SRCDIRLIST := $(SRCDIRLIST) $(SOURCEROOT)/ai/normal

SEARCH     := $(addsuffix /*.cpp, $(SRCDIRLIST))


all: $(TARGET)

$(TARGET): $(notdir $(patsubst %.cpp, %.o, $(wildcard $(SEARCH)))) $(LIBENGINE) $(RES)
	@echo "lnk: $@"
	$(CXX) $(LDFLAGS) $^ $(LIBS) -o $@
ifeq ($(PLATFORM), mingw)
ifdef RELEASE
	@strip -s $@.exe -o $@.stripped.exe
	@upx -9 -o$@.stripped.UPX.exe $@.stripped.exe > /dev/nul
	@echo "Size    : $$(ls -sh $@ | cut -d' ' -f1)"
	@echo "Stripped: $$(ls -sh $@.stripped.exe | cut -d' ' -f1)"
	@echo "UPX'd   : $$(ls -sh $@.stripped.UPX.exe | cut -d' ' -f1)"
endif
endif

pot: $(wildcard $(SEARCH))
	@echo "gen: $(POT)"
	@xgettext -d $(TARGET) -C -k_ -o $(POT) $(wildcard $(SEARCH))

$(RES): $(RC)
	@echo "gen: $@"
	$(WINDRES) -I../fheroes2/system $(DEBUGFLAG) -DTARGET=$(TARGET) $< -O coff -o $@

VPATH := $(SRCDIRLIST)

%.o: %.cpp
	@echo "$(CXX): $@"
	@$(CXX) -c -MD $(addprefix -I, $(SRCDIRLIST)) $< $(CFLAGS)

include $(wildcard *.d)

.PHONY: clean

clean:
	@rm -f *.pot *.o *.d $(RES) $(TARGET)*
