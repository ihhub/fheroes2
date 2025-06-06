# PlayStation Vita Port of [fheroes2](README.md) Project

## Install

Install fheroes2.vpk to your Vita.

fheroes2 requires data files from the original Heroes of Might and Magic II.

Copy HEROES2.AGG and HEROES2X.AGG (if you own Price of Loyalty expansion) from the original games "DATA" folder into
"ux0:data/fheroes2/data" and everything from "MAPS" folder into "ux0:data/fheroes2/maps".

Music files in OGG format (from GoG release of the game) should be placed into the "ux0:data/fheroes2/music/" folder.

fheroes2 supports in-game cinematics. To play intro and/or other videos, make sure that all of "*.SMK" files are placed
inside "ux0:data/fheroes2/heroes2/anim/" folder.

[rePatch reDux0](https://github.com/dots-tb/rePatch-reDux0) OR [FdFix](https://github.com/TheOfficialFloW/FdFix) plugin
is required for proper suspend/resume support (only use one at a time).

## Building

### Prerequisites

* VitaSDK
* libSDL2
* libSDL2-mixer

To build the game just run

```sh
make -f Makefile.vita
```

## Controls

* Left analog stick - Pointer movement
* Right analog stick - Map scrolling
* × - Left mouse button
* ○ - Right mouse button
* □ - End turn
* △ - Open spellbook
* D-Pad left - Next hero
* D-Pad right - Next castle
* D-Pad down - Re-visit the object that hero stands on
* R1 - Cursor acceleration
* SELECT - System menu
* START - Enter

Text input is done with D-Pad.

* Left - Remove character
* Right - Add new character
* Down - Next character (alphabetically)
* Up - Previous character
* R1, L1 - Switch current character between uppercase/lowercase

## Vita Specific Options

Pointer movement speed can be changed with 'gamepad_pointer_speed' parameter in "ux0:data/fheroes2/fheroes2.cfg".

Use "fullscreen = on" option to scale game area to native Vita resolution or "fullscreen = off" to keep the game
area at the center of the screen.

Native resolution is supported (set "videomode = 960x544" option in config file or just click on the left door
in main menu and select resolution manually).
