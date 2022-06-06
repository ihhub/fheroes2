LIN_DATA := $$HOME/.local/share/fheroes2
LIN_CONF := $$HOME/.config/fheroes2
UNIX_DATA := $$HOME/.fheroes2

SEDEX := /^\#_SG/,/^\#_EG/ d

ifneq ($(FHEROES2_DATA),)
SEDEX := /^\#_SG/,/^\#_EG/ { /^\#_[SE]G/d; s:@FHEROES2_DATA@:$(FHEROES2_DATA):}
endif
ifeq ($(shell uname), Linux)
SEDEX := $(SEDEX); s:@data@:$(LIN_DATA):; s:@config@:$(LIN_CONF):
else
SEDEX := $(SEDEX); /@data@/,/^This path/ {s:@data@:$(UNIX_DATA):; /^This path/d}
SEDEX := $(SEDEX); /@config@/,/^This path/ {s:@config@:$(UNIX_DATA):; /^This path/d}
endif
.PHONY: all clean

all:
	@sed -e '$(SEDEX)' fheroes2.6.in > fheroes2.6

clean:
	@rm -f fheroes2.6