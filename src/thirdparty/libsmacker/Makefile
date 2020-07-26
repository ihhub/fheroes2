# makefile
# project: libsmacker

TARGET	:= libsmacker

all: $(TARGET).a

$(TARGET).a: $(patsubst %.c, %.o, $(wildcard *.c)) 
	$(AR) crvs $@ $^

%.o: %.c
	$(CC) -c -MD $< $(CFLAGS)

include $(wildcard *.d)

.PHONY: clean

clean:
	rm -f *.a *.so *.d *.o
