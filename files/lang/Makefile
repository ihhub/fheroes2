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

.PHONY: all clean merge

all: $(patsubst %.po, %.mo, $(wildcard *.po))

merge:
	$(MAKE) $(wildcard *.po)

%.po: ../../src/dist/fheroes2.pot
	msgmerge -U --no-location $@ $<

# Spanish uses CP1252
es.mo: es.po
	iconv -f utf-8 -t CP1252 $< > $<.tmp
	LANG=es.CP1252 LC_ALL=es.CP1252 LC_CTYPE=es.CP1252 sed -e 's/UTF-8/CP1252/' $<.tmp > $<.2.tmp
	msgfmt $<.2.tmp -o $@

# In French, accented characters are mapped to unused ASCII characters
# Non-mapped characters are replaced by their non-accented equivalents
fr.mo: fr.po
	sed -e 'y/àâçéèêîïôùûüÉÊÀ/@*^~`|><#&$$uEEA/' $< > $<.tmp
	sed -i~ -e 's/œ/oe/g' $<.tmp
	msgfmt $<.tmp -o $@
	
# Portuguese uses CP1252	
pt.mo: pt.po
	iconv -f utf-8 -t CP1252 $< > $<.tmp
	LANG=pt.CP1252 LC_ALL=pt.CP1252 LC_CTYPE=pt.CP1252 sed -e 's/UTF-8/CP1252/' $<.tmp > $<.2.tmp
	msgfmt $<.2.tmp -o $@

# Polish version uses CP1250
pl.mo: pl.po
	iconv -f utf-8 -t CP1250 $< > $<.tmp
	LANG=pl.CP1250 LC_ALL=pl.CP1250 LC_CTYPE=pl.CP1250 sed -e 's/UTF-8/CP1250/' $<.tmp > $<.2.tmp
	msgfmt $<.2.tmp -o $@

# Czech version uses CP1250	
cs.mo: cs.po
	iconv -f utf-8 -t CP1250 $< > $<.tmp
	LANG=cs.CP1250 LC_ALL=cs.CP1250 LC_CTYPE=cs.CP1250 sed -e 's/UTF-8/CP1250/' $<.tmp > $<.2.tmp
	msgfmt $<.2.tmp -o $@

# Russian versions from "Buka" and "XXI vek" use CP1251 encoding (supported)
# Russian version from "Fargus" uses Russian keyboard layout as encoding (not supported)
# Russian alphabet can be transliterated on Debian with `konwert UTF8-ascii/1 -o $<.tmp $<`
ru.mo: ru.po
	iconv -f utf-8 -t CP1251 $< > $<.tmp
	LANG=ru_RU.CP1251 LC_ALL=ru_RU.CP1251 LC_CTYPE=ru_RU.CP1251 sed -e 's/UTF-8/CP1251/' $<.tmp > $<.2.tmp
	msgfmt $<.2.tmp -o $@

# Belarusian language uses CP1251.
be.mo: be.po
	iconv -f utf-8 -t CP1251 $< > $<.tmp
	LANG=be_BE.CP1251 LC_ALL=be_BE.CP1251 LC_CTYPE=be_BE.CP1251 sed -e 's/UTF-8/CP1251/' $<.tmp > $<.2.tmp
	msgfmt $<.2.tmp -o $@

# Bulgarian language uses CP1251.
bg.mo: bg.po
	iconv -f utf-8 -t CP1251 $< > $<.tmp
	LANG=bg_BG.CP1251 LC_ALL=bg_BG.CP1251 LC_CTYPE=bg_BG.CP1251 sed -e 's/UTF-8/CP1251/' $<.tmp > $<.2.tmp
	msgfmt $<.2.tmp -o $@

# Ukrainian language uses CP1251.
uk.mo: uk.po
	iconv -f utf-8 -t CP1251 $< > $<.tmp
	LANG=uk_UK.CP1251 LC_ALL=uk_UK.CP1251 LC_CTYPE=uk_UK.CP1251 sed -e 's/UTF-8/CP1251/' $<.tmp > $<.2.tmp
	msgfmt $<.2.tmp -o $@

# German version uses CP1252
de.mo: de.po
	iconv -f utf-8 -t CP1252 $< > $<.tmp
	LANG=de.CP1252 LC_ALL=de.CP1252 LC_CTYPE=de.CP1252 sed -e 's/UTF-8/CP1252/' $<.tmp > $<.2.tmp
	msgfmt $<.2.tmp -o $@

# Norwegian uses CP1252 in the code currently
nb.mo: nb.po
	iconv -f utf-8 -t CP1252 $< > $<.tmp
	LANG=nb.CP1252 LC_ALL=nb.CP1252 LC_CTYPE=nb.CP1252 sed -e 's/UTF-8/CP1252/' $<.tmp > $<.2.tmp
	msgfmt $<.2.tmp -o $@

# Italian
it.mo: it.po
	iconv -f utf-8 -t CP1252 $< > $<.tmp
	LANG=it.CP1252 LC_ALL=it.CP1252 LC_CTYPE=it.CP1252 sed -e 's/UTF-8/CP1252/' $<.tmp > $<.2.tmp
	msgfmt $<.2.tmp -o $@
	
# Romanian uses CP1250
ro.mo: ro.po
	iconv -f utf-8 -t CP1250 $< > $<.tmp
	LANG=ro.CP1250 LC_ALL=ro.CP1250 LC_CTYPE=ro.CP1250 sed -e 's/UTF-8/CP1250/' $<.tmp > $<.2.tmp
	msgfmt $<.2.tmp -o $@

# All other languages: drop accents transliterated with `"` (which breaks translation file format)
# and transliterate the rest with default iconv rules
%.mo: %.po
	sed -e 'y/äëïöőüűÄËÏŐÖÜŰ/aeioouuAEIOOUU/' $< > $<.tmp
	iconv -f utf-8 -t ascii//TRANSLIT $<.tmp > $<.2.tmp
	msgfmt $<.2.tmp -o $@

clean:
	rm -f *.mo *.po~ *.tmp *.tmp~
