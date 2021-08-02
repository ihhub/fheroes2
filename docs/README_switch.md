This is a port of Free Heroes 2 (free implementation of Heroes of Might and Magic 2 engine) to Nintendo Switch.
It's a homebrew app, so you need to be running custom firmware for it to work.


[1] BUILDING
------------

If you're targeting Nintendo Switch, you will need to install the [devkitPro](https://devkitpro.org/) toolchain. Make sure to include the following packages:

    libnx switch-sdl2_image switch-sdl2_mixer

Then follow the usual steps to build Free Heroes 2.


[2] SETUP
---------

You will need a copy of the official game to run this port.

Free Heroes 2 root directory is hardcoded as `/switch/fheroes2`. Put the game files there (specifically `ANIM`, `DATA`, `MAPS` and `MUSIC` folders),
then copy over the `files` directory, as well as `fheroes2.nro`. `ANIM` and `MUSIC` folders are optional: you can play without any music or movies.
If you don't have the `ANIM` folder, create it and copy whatever `*.smk` files you have from `DATA` to `ANIM`.


In the end you should have the following directory tree on your SD card:

    switch
     |
     +-- ...
     +-- fheroes2  <--- this is the game directory
         |
         +--- anim
         +--- data
         +--- files
         +--- maps
         +--- music
         +--- fheroes2.nro


[3] RUNNING
-----------

This build of Free Heroes 2 was tested on 12.0.3|AMS M.19.4|S (FAT32). exFAT is not recommended.
It's possible to use USB mice / keyboards with this build.

Working controls are:
- Touchscreen - emulates mouse, including dragging
- L-stick - move mouse cursor
- R-stick/D-pad - scroll
- A - left mouse click
- B - Right mouse click
- X - Escape
- Y - Enter
- (+) - Cast spell
- (-) - End turn
- R - Cycle through towns
- L - Cycle through heroes

Translations are being worked on, FR / RU locales should already be usable. Changing the language is done
by replacing `lang = en` parameter in `fheroes2.cfg` with the desired country code. Generally, you will need
game resources from the localized version of HoMM2 in order to use translations in Free Heroes 2.
