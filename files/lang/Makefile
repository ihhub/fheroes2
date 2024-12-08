###########################################################################
#   fheroes2: https://github.com/ihhub/fheroes2                           #
#   Copyright (C) 2024                                                    #
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

ICONV  = iconv
# TODO: consider converting game fonts and texts to UTF-8 in the engine instead
MSGFMT = sed -e '1,20 s/UTF-8/$(1)/' $< | $(ICONV) -f utf-8 -t $(1) | if msgfmt --help | grep -q no-convert >/dev/null 2>/dev/null; then msgfmt - -o $@ --no-convert; else msgfmt - -o $@; fi

.PHONY: all clean merge

all: $(patsubst %.po, %.mo, $(wildcard *.po))

merge: ../../src/dist/fheroes2/fheroes2.pot
	for i in $(wildcard *.po); do msgmerge -U --no-location $$i $<; done

# Czech, Hungarian, Polish and Slovak versions use CP1250
cs.mo hu.mo pl.mo sk.mo: %.mo: %.po
	$(call MSGFMT,CP1250)

# Belarusian, Bulgarian, Ukrainian versions use CP1251
# Russian versions from "Buka" and "XXI vek" use CP1251 encoding (supported)
# Russian version from "Fargus" uses Russian keyboard layout as encoding (not supported)
be.mo bg.mo ru.mo uk.mo: %.mo: %.po
	$(call MSGFMT,CP1251)

# German, Danish, French, Italian, Dutch, Norwegian, Portuguese, Spanish and Swedish translations use CP1252
de.mo dk.mo es.mo fr.mo it.mo nb.mo pt.mo sv.mo nl.mo: %.mo: %.po
	$(call MSGFMT,CP1252)

# Turkish uses CP1254
tr.mo: tr.po
	$(call MSGFMT,CP1254)
	
# Lithaunian uses CP1257
lt.mo: lt.po
	$(call MSGFMT,CP1257)
	
# Vietnamese uses CP1258
vi.mo: vi.po
	$(call MSGFMT,CP1258)

# Romanian uses ISO-8859-16
ro.mo: ro.po
	$(call MSGFMT,ISO-8859-16)

# All other languages: drop accents transliterated with `"` (which breaks translation file format)
# and transliterate the rest with default iconv rules
%.mo: %.po
	sed -e 'y/äëïöőüűÄËÏŐÖÜŰ/aeioouuAEIOOUU/' $< | $(ICONV) -f utf-8 -t ascii//TRANSLIT | msgfmt - -o $@

clean:
	rm -f *.mo *.po~
