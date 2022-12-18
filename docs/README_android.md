# Android port of [**fheroes2**](README.md) project

## Installation
Install the fheroes2.apk to your device running Android. You need to have set the necessary permissions on your device to
do this.

fheroes2 requires the data files from the original Heroes of Might and Magic II. If you do not have the original game you
can download a demo version of the game from https://archive.org/download/HeroesofMightandMagicIITheSuccessionWars_1020/h2demo.zip

From your original Heroes 2 installation copy the directories named "DATA", "MAPS", "ANIM" and "MUSIC" into the
"internalStorage:Android/data/org.fheroes2/files/" folder.

Depending on your version of Heroes 2, the music files that need to be placed in the "MUSIC" directory can either be on your
original CD or in the installation folder (GOG and Ubisoft).

Ingame cinematics are supported by fheroes2 and for these to be supported it is necessary to have them in the "ANIM" folder.
Depending on your original version of Heroes 2 they can either be found on your CD, disc image or in a folder named "anim" in
your Heroes 2 installation directory.

## Controls
To simulate a right-click to get info on various items, you need to first touch and keep touching on the item of interest
and then touch anywhere else on the screen. You can then remove your first finger from the screen and keep viewing the info
on the item.

By default normal adventure map scrolling on the borders of the screen is disabled. To pane the viewing area around you
need to press anywhere on the adventure map and slide around to change where you are viewing.

Note that text input is not yet implemented.

## Android specific notes
Fullscreen is enabled by default. To have the info bar with the clock visible you can disable fullscreen mode within the
main menu settings under "Graphics".
