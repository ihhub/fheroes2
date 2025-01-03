###########################################################################
#   fheroes2: https://github.com/ihhub/fheroes2                           #
#   Copyright (C) 2022 - 2024                                             #
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
	sed -e '$(SEDEX)' fheroes2.6.in > fheroes2.6

clean:
	rm -f fheroes2.6
