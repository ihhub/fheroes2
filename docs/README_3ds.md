# Nintendo 3DS platform foundation

This is an initial Homebrew Launcher port for review, not the complete touchscreen interface.
The game uses an 800×480 indexed frame. The top screen shows an 800×240 wide-mode overview on supported consoles,
with standard 400×240 output on Old 2DS or when model detection fails. The lower screen shows a 320×240 crop centered on the controller cursor.
Wide-mode pixels are half as wide physically, so the overview retains the game frame's proportions. Stereo 3D is disabled.

## Building

Install the devkitPro Nintendo 3DS toolchain, libctru, zlib and the 3DS tools.
Use SDL2 and SDL2_mixer built for that toolchain. Their CMake packages can be used directly;
alternatively supply source checkouts with the two optional paths below. Original game data is not needed to compile.

The source-based build has been checked with SDL2 commit `4c2d9014afda49553c76f7045529207bb593f9b5`
and SDL2_mixer commit `8f0c805d54dbd1ff6b4b784d351ca884b8fe5ee9`.

```sh
cmake -S platforms/3ds -B build-3ds \
    -DCMAKE_TOOLCHAIN_FILE="$DEVKITPRO/cmake/3DS.cmake" \
    -DFHEROES2_SDL2_SOURCE_DIR=/path/to/SDL2 \
    -DFHEROES2_SDL2_MIXER_SOURCE_DIR=/path/to/SDL2_mixer \
    -DCMAKE_BUILD_TYPE=Release
cmake --build build-3ds --parallel 4
```

With installed SDL2 packages, omit the two source-directory arguments.
The output is `build-3ds/fheroes2.3dsx` with Homebrew Launcher metadata in `fheroes2.smdh`.
The maintained local validation uses native Windows CMake and an external devkitARM toolchain adapter;
the stock devkitPro command above still needs independent validation on its supported host environment.

## Installation and controls

Put the application in `/3ds/fheroes2/` on the SD card. Add your own original Heroes II data in its `data`, `maps`,
`anim` and optional `music` directories. Copy `files/data` and any compiled translations from this repository into `files` below the same directory.
Original game files are not distributed with this port. Configuration, saves and the log use the SD application directory.

The Circle Pad moves the cursor; A is the left mouse button, B is the right mouse button, L is Escape and Start is Enter.
R uses the existing controller pointer-speed modifier. The original game UI is retained in this first change.
Stylus input is deliberately disabled until lower-viewport coordinate mapping is introduced.

## Review scope and follow-ups

This change adds a standalone CMake entry point, newlib/SDL type compatibility, SD paths/logging,
controller initialization and native framebuffer presentation. Startup logging, intro playback and SDL audio initialization retain the existing game behavior.
SDL initialization errors follow the existing startup failure path; no platform-specific retry is introduced.

The separate development port has been run on a real New 3DS XL for matches, AI turns, battles and saving/loading.
Those reports cover the broader port, not an independent hardware run of this reduced branch.
Physical Old 3DS compatibility, long sessions and audio still need testing.

Follow-ups will bring stylus mapping/hold gestures, viewport smoothing, contextual D-pad scrolling,
the adventure command strip and dialogs adapted to the two screens. The initial renderer and build integration are open to architectural feedback.
