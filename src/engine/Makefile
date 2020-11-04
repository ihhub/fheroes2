# makefile
# project: Free Heroes2
# libSDL C++ wrapper engine

TARGET	:= libengine
CFLAGS := $(CFLAGS) -I../thirdparty/libsmacker

all: $(TARGET).a

$(TARGET).a: $(patsubst %.cpp, %.o, $(wildcard *.cpp)) $(patsubst %.S, %.o, $(wildcard *.S)) 
	$(AR) crvs $@ $^

%.o: %.cpp
	$(CXX) -c -MD $< $(CFLAGS)

include $(wildcard *.d)

.PHONY: clean

clean:
	rm -f *.a *.so *.d *.o
