# [**fheroes2**](README.md) graphical artist guide

The original game has its graphical assets stored in the `HEROES2.agg` file and potentially the `HEROES2X.agg` file if the expansion
pack The Price of Loyalty is present. These two AGG files are located in the `DATA` folder of the original Heroes 2 installation.

For convenience's sake the fheroes2 project provides a set of tools and a script that artists can use to easily extract and convert the original
graphical assets into viewable PNG files.

New assets that the fheroes2 project adds to the game are included in the `files/data/resurrection.h2d` data file.

This guide is designed for Linux, MacOS and Windows users. The source files to these tools are available in the project's repository.

## Where to get the tools

You can download the precompiled tools for your operating system (OS) of choice here:<br>
[Windows 32-bit](https://github.com/ihhub/fheroes2/releases/download/fheroes2-windows-x86-SDL2/fheroes2_tools_windows_x86_SDL2.zip)<br>
[Windows 64-bit](https://github.com/ihhub/fheroes2/releases/download/fheroes2-windows-x64-SDL2/fheroes2_tools_windows_x64_SDL2.zip)

[MacOS](https://github.com/ihhub/fheroes2/releases/download/fheroes2-osx-sdl2_dev/fheroes2_tools_macos_x86-64_SDL2.zip)

[Linux Ubuntu 64-bit](https://github.com/ihhub/fheroes2/releases/download/fheroes2-linux-sdl2_dev/fheroes2_tools_ubuntu_x86-64_SDL2.zip)<br>
[Linux Ubuntu ARM 64-bit](https://github.com/ihhub/fheroes2/releases/download/fheroes2-linux-arm-sdl2_dev/fheroes2_tools_ubuntu_arm64_SDL2.zip)


Here is a list and short description of the various graphical artist tools:

- extractor

This tool extracts all the contents of the AGG files mentioned above. Among the extracted files are the ICNs which hold the graphical assets.

- icn2img

This tool converts the various graphical assets contained within the ICNs into PNG or BMP files.

- pal2img

This tool generates a PNG or BMP file from the palette file `KB.PAL`. It displays it as 16x16 tiles of 16 pixels each.

## How to use the tools to extract the images

Place the `HEROES2.agg` and optionally the `HEROES2X.agg` files in the same folder as you extracted the tools to.
Then run the script called `extract_agg`. This will create two folders: `agg` and `icn`. Both of them have a subdirectory named `HEROES2`,
and potentially a `HEROES2X` one, reflecting what AGG file the contents come from. In addition running the script will generate a PNG file
of the palette of the game with the name `palette.png` in the same directory as the folders `agg` and `icn`.

The folder named `agg` contains the untouched contents of the AGG files.

Meanwhile, the folder named `icn` contains the PNG-converted graphical assets. These assets will be stored within folders named according to what
ICN they were stored in, e.g. the folder `apanel` came from the ICN `apanel.icn`. The `icn` folder is in other words the most relevant one for
graphical artists.

## Where is help needed

We have labelled and tracked some issues that need help from graphical artists.
These can be found [here](https://github.com/ihhub/fheroes2/issues?q=is%3Aopen+is%3Aissue+label%3Aassets).

NOTE! Before starting to work on an issue make sure to check if someone isn't already assigned to it by looking at the relevant place
within the issue. Also notify the fheroes2 team that you wish to work on the issue. This is to avoid duplicate work.

## The work process

1. Choose the item you are going to work on and notify the fheroes2 team to avoid duplicate work.
2. Present a prototype / complete enough work in your opinion of your work.
3. 1-2 members of the team will give their feedback.
4. Address the given feedback if any.
5. Repeat steps 3-4 until approval.
6. Expand the review team to 3-4 members and steps 3-4 are repeated.
7. Once everyone approves we will add this file into the `resurrection.h2d` data file and change the code in the fheroes2 source code to use it.

