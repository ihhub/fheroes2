# [**fheroes2**](README.md) graphical artist guide

The original game uses graphical assets stored in the `HEROES2.agg` container, and the Expansion Pack "Price of Loyalty" adds the additional
files in `HEROES2X.agg`. These are located in the `DATA` folder.

For convenience the fheroes2 project provides a set of tools and a script that artists can easily use to extract and convert the original
assets into viewable PNG files. New assets that the fheroes2 project adds to the game are included in the `files/data/resurrection.h2d`
container. This guide is designed for Linux, MacOS and Windows users. The source files to these tools are available in the project's repository.

which tools to download, how to extract images and which issues to look in GitHub as a start point

## Where to get the tools

You can download the precompiled tools for your OS of choice here:
[Windows 64-bit](https://github.com/ihhub/fheroes2/releases/download/fheroes2-windows-x64-SDL2/fheroes2_tools_windows_x64_SDL2.zip)
[Windows 32-bit](https://github.com/ihhub/fheroes2/releases/download/fheroes2-windows-x86-SDL2/fheroes2_tools_windows_x86_SDL2.zip)

[MacOS](https://github.com/ihhub/fheroes2/releases/download/fheroes2-osx-sdl2_dev/fheroes2_tools_macos_x86-64_SDL2.zip)

[Linux Ubuntu 64-bit](https://github.com/ihhub/fheroes2/releases/download/fheroes2-linux-sdl2_dev/fheroes2_tools_ubuntu_x86-64_SDL2.zip)
[Linux Ubuntu ARM 64-bit](https://github.com/ihhub/fheroes2/releases/download/fheroes2-linux-arm-sdl2_dev/fheroes2_tools_ubuntu_arm64_SDL2.zip)


Here is a list and description of the tools:

- extractor

This tool is extracts the files in the form of ICNs from the AGG containers mentioned above.

- icn2img

This tool will convert the various sprite assets into PNG files.

- pal2img

This tool will generate a PNG image from the palette file `KB.PAL`. It shows 16x16 tiles of 16 pixels each.

## How to use the tools to extract images

For Windows users you can simply place the `HEROES2.agg` and optionally the `HEROES2X.agg` containers in the same folder as the tools, and then
run the script called `extract_agg.bat`. This will place the extracted files in a folder named `agg` and the converted PNG images in a folder
named `icn`, which will contain separate folders named according to the ICNs they are converted from.

## Where is help needed for the project


