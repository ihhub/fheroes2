#
# makefile
#

.PHONY: all clean merge

all: $(patsubst %.po, %.mo, $(wildcard *.po))

merge:
	$(MAKE) $(wildcard *.po)

%.po: ../../src/dist/fheroes2.pot
	msgmerge -U --no-location $@ $<

# Spanish: drop accents transliterated with `"` (which breaks translation file format)
# and transliterate the rest using the es_ES.UTF-8 locale
es.mo: es.po
	sed -e 'y/äëïöőüűÄËÏŐÖÜŰ/aeioouuAEIOOUU/' $< > $<.tmp
	LANG=es_ES.UTF-8 LC_ALL=es_ES.UTF-8 LC_CTYPE=es_ES.UTF-8 iconv -f utf-8 -t ascii//TRANSLIT $<.tmp > $<.2.tmp
	msgfmt $<.2.tmp -o $@

# In French, accented characters are mapped to unused ASCII characters
# Non-mapped characters are replaced by their non-accented equivalents
fr.mo: fr.po
	sed -e 'y/àâçéèêîïôùûüÉÊÀ/@*^~`|><#&$$uEEA/' $< > $<.tmp
	sed -i~ -e 's/œ/oe/g' $<.tmp
	msgfmt $<.tmp -o $@

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

# German version uses CP1252
de.mo: de.po
	iconv -f utf-8 -t CP1252 $< > $<.tmp
	LANG=de.CP1252 LC_ALL=de.CP1252 LC_CTYPE=de.CP1252 sed -e 's/UTF-8/CP1252/' $<.tmp > $<.2.tmp
	msgfmt $<.2.tmp -o $@

# Norwegian has locale-specific transliteration rules (æ -> ae, etc)
nb.mo: nb.po
	sed -e 's/æ/ae/g' $< > $<.tmp
	sed -i~ -e 's/ø/oe/g' $<.tmp
	sed -i~ -e 's/å/aa/g' $<.tmp
	sed -i~ -e 's/Æ/Ae/g' $<.tmp
	sed -i~ -e 's/Ø/Oe/g' $<.tmp
	sed -i~ -e 's/Å/Aa/g' $<.tmp
	sed -i~ -e 's/é/e/g' $<.tmp
	iconv -f utf-8 -t ascii//TRANSLIT $<.tmp > $<.2.tmp
	msgfmt $<.2.tmp -o $@

# Italian
it.mo: it.po
	sed -e 'y/àèéìòùÀÈÉÌÒÙ/aeeiouAEEIOU/' $< > $<.tmp
	iconv -f utf-8 -t ascii//TRANSLIT $<.tmp > $<.2.tmp
	msgfmt $<.2.tmp -o $@

# All other languages: drop accents transliterated with `"` (which breaks translation file format)
# and transliterate the rest with default iconv rules
%.mo: %.po
	sed -e 'y/äëïöőüűÄËÏŐÖÜŰ/aeioouuAEIOOUU/' $< > $<.tmp
	iconv -f utf-8 -t ascii//TRANSLIT $<.tmp > $<.2.tmp
	msgfmt $<.2.tmp -o $@

clean:
	rm -f *.mo *.po~ *.tmp *.tmp~
