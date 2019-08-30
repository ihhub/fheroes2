#
# makefile
#

all: $(patsubst %.po, %.mo, $(wildcard *.po))

%.mo: %.po
	msgmerge -U $< fheroes2.pot
	msgfmt $< -o $@

clean:
	rm -f *.mo
