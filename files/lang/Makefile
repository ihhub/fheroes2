#
# makefile
#

# TODO: remove the iconv/konwert line once unicode support is there

all: $(patsubst %.po, %.mo, $(wildcard *.po))

# In French, accented characters are mapped to unused ASCII characters
# Non-mapped characters are replaced by their non-accented equivalents
fr.mo: fr.po
	msgmerge -U --no-location $< fheroes2.pot
	sed -e 'y/àâçéèêîïôùûüÉÊÀ/@*^~ee><#&$$uEEA/' $< > $<.tmp
	msgfmt $<.tmp -o $@

# Russian versions from "Buka" and "XXI vek" use CP1251 encoding (supported)
# Russian version from "Fargus" uses Russian keyboard layout as encoding (not supported)
ru.mo: ru.po
	msgmerge -U --no-location $< fheroes2.pot
	iconv -f utf-8 -t CP1251 $< > $<.tmp
	LANG=ru_RU.CP1251 LC_ALL=ru_RU.CP1251 LC_CTYPE=ru_RU.CP1251 sed -e 's/UTF-8/CP1251/' $<.tmp > $<.2.tmp
	msgfmt $<.2.tmp -o $@

# All other languages: drop accents transliterated with `"` (which breaks translation file format)
# and transliterate the rest with default iconv rules
%.mo: %.po
	msgmerge -U --no-location $< fheroes2.pot
	sed -e 'y/äëïöőüűÄËÏŐÖÜŰ/aeioouuAEIOOUU/' $< > $<.tmp
	iconv -f utf-8 -t ascii//TRANSLIT $<.tmp > $<.2.tmp
	msgfmt $<.2.tmp -o $@

clean:
	rm -f *.mo *.tmp
