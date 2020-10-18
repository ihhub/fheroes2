# makefile
# xml wrapper
# http://www.sourceforge.net/projects/tinyxml

TARGET	:= libtinyxml

all: $(TARGET).a

$(TARGET).a: $(patsubst %.cpp, %.o, $(wildcard *.cpp))
	$(AR) crvs $@ $^

%.o: %.cpp
	$(CXX) -c -MD $< $(CFLAGS)

include $(wildcard *.d)

.PHONY: clean

clean:
	rm -f *.a *.so *.d *.o
