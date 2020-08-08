# makefile
# project: libsmacker

CONLYFLAGS:=$(filter-out -std=c++11,$(CFLAGS))

TARGET	:= libsmacker

all: $(TARGET).a

$(TARGET).a: $(patsubst %.c, %.o, $(wildcard *.c)) 
	$(AR) crvs $@ $^

%.o: %.c
	$(CC) -c -MD $< $(CONLYFLAGS)

include $(wildcard *.d)

.PHONY: clean

clean:
	rm -f *.a *.so *.d *.o
