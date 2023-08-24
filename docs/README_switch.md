# Nintendo Switch port of [**fheroes2**](README.md) project

It's a homebrew app, so you need to be running custom firmware for it to work.

## BUILDING
You will need to install the [devkitPro](https://devkitpro.org/) toolchain. Make sure to include the following package:
```
libnx switch-sdl2_mixer
```

After installation run `make -f Makefile.switch -j 2` command to build the package.

## SETUP
You will need a copy of the official game to run this port.

fheroes2 root directory is hardcoded as `/switch/fheroes2`. Put the game files there (specifically `ANIM`, `DATA`, `MAPS`
and `MUSIC` folders), then copy over the `files` directory, as well as `fheroes2.nro`. If you have a Russian version from
Buka Enternainment, you'll likely have `Anim2` folder instead. Rename it to `ANIM` if you wish use the Buka game data with
this port.

At the end you should have the following directory tree on your SD card:

    switch
     |
     +-- ...
     +-- fheroes2  <--- this is the game directory
         |
         +--- anim         <--- HoMM2 game data
         +--- data         <--- HoMM2 game data
         +--- files        <--- Part of fheroes2 release
         +--- maps         <--- HoMM2 game data
         +--- music        <--- HoMM2 game data
         +--- fheroes2.nro <--- Part of fheroes2 release

Generally, you will need game resources from the localized version of HoMM2 in order to use translations in fheroes2. During
the first run, the game should auto-detect the game data you have and offer to choose a language you'd like to use. English
is always available.

## RUNNING
This build of fheroes2 was tested on 12.0.3|AMS M.19.4|S (FAT32). exFAT is not recommended.
USB mice and keyboards connected via an OTG adapter are supported.

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
