#
# makefile
#

# TODO: remove the iconv/konwert line once unicode support is there
# `-iconv -c -f utf-8 -t ascii $< > $<.tmp` simply removes all non-ASCII characters
# Most non-ASCII characters can be converted to ASCII with `iconv -f utf-8 -t ascii//TRANSLIT -o $<.tmp $<`
# Russian alphabet can be transliterated on Debian with `konwert UTF8-ascii/1 -o $<.tmp $<`

all: $(patsubst %.po, %.mo, $(wildcard *.po))

%.mo: %.po
	msgmerge -U --no-location $< fheroes2.pot
	-iconv -c -f utf-8 -t ascii $< > $<.tmp
	msgfmt $<.tmp -o $@

clean:
	rm -f *.mo *.tmp
