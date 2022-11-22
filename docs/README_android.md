# Android port of [**fheroes2**](README.md) project

## Installation
Install the fheroes2.apk to your device running Android. You need to have set the necessary permissions on your device to
do this.

fheroes2 requires the data files from the original Heroes of Might and Magic II.

Copy HEROES2.AGG and HEROES2X.AGG (if you own the Price of Loyalty expansion) from the original game's "DATA" folder into
"internalStorage:Android/data/org.fheroes2/files/data/" and everything from "MAPS" folder into
"internalStorage:Android/data/org.fheroes2/files/maps/".

The music files should be placed in the "internalStorage:Android/data/org.fheroes2/files/maps/" folder.

fheroes2 supports the ingame cinematics. This is necessary for full campaign support. To play the intro and the other
videos, make sure that all of the "*.SMK" files are placed in the "internalStorage:Android/data/org.fheroes2/files/anim/"
folder.

## Controls
To simulate a right click to get info on various items, you need to first touch and keep touching anywhere on the screen
and then touch on the item of interest. You can slide away while maintaining the touch to keep viewing the info on the item.

By default normal adventure map scrolling on the borders of the screen is disabled. To pane the viewing area around you
need to press anywhere on the adventure map and slide around to change where you are viewing.

Note that text input is not yet implemented.

## Android specific notes

Pointer movement speed can be changed with 'gamepad_pointer_speed' parameter in "ux0:data/fheroes2/fheroes2.cfg".

Fullscreen is enabled by default. To have the info bar with the clock visible you can disable fullscreen mode within the
main menu settings under "Graphics".
