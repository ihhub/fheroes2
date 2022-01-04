#
# makefile
#

.PHONY: all clean merge copy

all: $(patsubst %.po, %.mo, $(wildcard *.po))

merge:
	$(MAKE) $(wildcard *.po)

copy:
	cp *.mo ../../build/x64/Debug-SDL1/files/lang 2>/dev/null || :
	cp *.mo ../../build/x86/Debug-SDL1/files/lang 2>/dev/null || :
	cp *.mo ../../build/x64/Debug-SDL2/files/lang 2>/dev/null || :
	cp *.mo ../../build/x86/Debug-SDL2/files/lang 2>/dev/null || :
	cp *.mo ../../build/x64/Release-SDL1/files/lang 2>/dev/null || :
	cp *.mo ../../build/x86/Release-SDL1/files/lang 2>/dev/null || :
	cp *.mo ../../build/x64/Release-SDL2/files/lang 2>/dev/null || :
	cp *.mo ../../build/x86/Release-SDL2/files/lang 2>/dev/null || :

%.po: ../../src/dist/fheroes2.pot
	msgmerge -U --no-location $@ $<

# Spanish: drop accents transliterated with `"` (which breaks translation file format)
# and transliterate the rest using the es_ES.UTF-8 locale
es.mo: es.po
	sed -e 'y/äëïöőüűÄËÏŐÖÜŰ/aeioouuAEIOOUU/' $< > $<.tmp
	LANG=es_ES.UTF-8 LC_ALL=es_ES.UTF-8 LC_CTYPE=es_ES.UTF-8 iconv -f utf-8 -t ascii//TRANSLIT $<.tmp > $<.2.tmp
	msgfmt $<.2.tmp -o $@
	rm -f $<.tmp $<.2.tmp

# In French, accented characters are mapped to unused ASCII characters
# Non-mapped characters are replaced by their non-accented equivalents
fr.mo: fr.po
	sed -e 'y/àâçéèêîïôùûüÉÊÀ/@*^~`|><#&$$uEEA/' $< > $<.tmp
	msgfmt $<.tmp -o $@
	rm -f $<.tmp

# Polish version uses CP1250
pl.mo: pl.po
	iconv -f utf-8 -t CP1250 $< > $<.tmp
	LANG=pl.CP1250 LC_ALL=pl.CP1250 LC_CTYPE=pl.CP1250 sed -e 's/UTF-8/CP1250/' $<.tmp > $<.2.tmp
	msgfmt $<.2.tmp -o $@
	rm -f $<.tmp $<.2.tmp

# Russian versions from "Buka" and "XXI vek" use CP1251 encoding (supported)
# Russian version from "Fargus" uses Russian keyboard layout as encoding (not supported)
# Russian alphabet can be transliterated on Debian with `konwert UTF8-ascii/1 -o $<.tmp $<`
ru.mo: ru.po
	iconv -f utf-8 -t CP1251 $< > $<.tmp
	LANG=ru_RU.CP1251 LC_ALL=ru_RU.CP1251 LC_CTYPE=ru_RU.CP1251 sed -e 's/UTF-8/CP1251/' $<.tmp > $<.2.tmp
	msgfmt $<.2.tmp -o $@
	rm -f $<.tmp $<.2.tmp

# German has locale-specific transliteration rules (ä -> ae, etc)
de.mo: de.po
	sed -e 's/ä/ae/g' $< > $<.tmp
	sed -i -e 's/ö/oe/g' $<.tmp
	sed -i -e 's/ü/ue/g' $<.tmp
	sed -i -e 's/Ö/Oe/g' $<.tmp
	sed -i -e 's/Ü/Ue/g' $<.tmp
	iconv -f utf-8 -t ascii//TRANSLIT $<.tmp > $<.2.tmp
	msgfmt $<.2.tmp -o $@
	rm -f $<.tmp $<.2.tmp
	
# Norwegian has locale-specific transliteration rules (æ -> ae, etc)
nb.mo: nb.po
	sed -e 's/æ/ae/g' $< > $<.tmp
	sed -i -e 's/ø/oe/g' $<.tmp
	sed -i -e 's/å/aa/g' $<.tmp
	sed -i -e 's/Æ/Ae/g' $<.tmp
	sed -i -e 's/Ø/Oe/g' $<.tmp
	sed -i -e 's/Å/Aa/g' $<.tmp
	sed -i -e 's/é/e/g' $<.tmp
	iconv -f utf-8 -t ascii//TRANSLIT $<.tmp > $<.2.tmp
	msgfmt $<.2.tmp -o $@
	rm -f $<.tmp $<.2.tmp
	
# Italian
it.mo: it.po
	sed -e 'y/àèéìòùÀÈÉÌÒÙ/aeeiouAEEIOU/' $< > $<.tmp
	iconv -f utf-8 -t ascii//TRANSLIT $<.tmp > $<.2.tmp
	msgfmt $<.2.tmp -o $@
	rm -f $<.tmp $<.2.tmp

# All other languages: drop accents transliterated with `"` (which breaks translation file format)
# and transliterate the rest with default iconv rules
%.mo: %.po
	sed -e 'y/äëïöőüűÄËÏŐÖÜŰ/aeioouuAEIOOUU/' $< > $<.tmp
	iconv -f utf-8 -t ascii//TRANSLIT $<.tmp > $<.2.tmp
	msgfmt $<.2.tmp -o $@
	rm -f $<.tmp $<.2.tmp

clean:
	rm -f *.mo *.po~ *.tmp

