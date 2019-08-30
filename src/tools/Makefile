# makefile
# project: Free Heroes2 Tools
#

TARGETS := extractor 82m2wav til2img icn2img xmi2mid
LIBENGINE := ../engine/libengine.a
LIBS := $(LIBENGINE) $(LIBS)
CFLAGS := $(CFLAGS) -I../engine

all: $(TARGETS)

$(TARGETS): $(addsuffix .cpp, $(TARGETS)) $(LIBENGINE)
	$(CXX) -c $@.cpp $(CFLAGS)
	$(CXX) -o $@ $@.o $(LIBS)

.PHONY: clean

clean:
	rm -f *.o *.exe $(TARGETS)
