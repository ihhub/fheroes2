#!/bin/sh

FHEROES2_DIR=$HOME/.fheroes2

test -d $FHEROES2_DIR || ( mkdir -p $FHEROES2_DIR && cp /usr/share/doc/fheroes2/fheroes2.key $FHEROES2_DIR )
cd $FHEROES2_DIR
exec /usr/lib/games/fheroes2/fheroes2 $@
