#!/bin/sh

FHEROES2_DIR=$HOME/.fheroes2

test -d $FHEROES2_DIR || mkdir $FHEROES2_DIR || exit
test -x $FHEROES2_DIR/fheroes2 || (ln -s /usr/lib/games/fheroes2/fheroes2 $FHEROES2_DIR && cp /usr/share/doc/fheroes2/fheroes2.key $FHEROES2_DIR) || exit
cd $FHEROES2_DIR
exec ./fheroes2 $@
