###########################################################################
#   fheroes2: https://github.com/ihhub/fheroes2                           #
#   Copyright (C) 2022                                                    #
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

recode = sed -e 's/UTF-8/$(1)/' $< | iconv -f utf-8 -t $(1) | msgfmt - -o $@
.PHONY: all clean merge

all: $(patsubst %.po, %.mo, $(wildcard *.po))

merge: ../../src/dist/fheroes2.pot
	for i in $(wildcard *.po); do msgmerge -U --no-location $$i $<; done


# In French, accented characters are mapped to unused ASCII characters
# Non-mapped characters are replaced by their non-accented equivalents
fr.mo: fr.po
	sed -e 'y/àâçéèêîïôùûüÉÊÀ/@*^~`|><#&$$uEEA/' $< | sed -e 's/œ/oe/g' | msgfmt - -o $@

# Polish, Czech versions use CP1250
cs.mo pl.mo: %.mo: %.po
	$(call recode,CP1250)

# Belarusian, Bulgarian, Ukrainian versions use CP1251
# Russian versions from "Buka" and "XXI vek" use CP1251 encoding (supported)
# Russian version from "Fargus" uses Russian keyboard layout as encoding (not supported)
be.mo bg.mo ru.mo uk.mo: %.mo: %.po
	$(call recode,CP1251)

# German, Italian, Norwegian, Portuguese, Spanish, Swedish versions use CP1252
de.mo es.mo it.mo nb.mo pt.mo sv.mo: %.mo: %.po
	$(call recode,CP1252)

# Romanian uses ISO-8859-16
ro.mo: ro.po
	$(call recode,ISO-8859-16)

# All other languages: drop accents transliterated with `"` (which breaks translation file format)
# and transliterate the rest with default iconv rules
%.mo: %.po
	sed -e 'y/äëïöőüűÄËÏŐÖÜŰ/aeioouuAEIOOUU/' $< | iconv -f utf-8 -t ascii//TRANSLIT | msgfmt - -o $@

clean:
	rm -f *.mo
