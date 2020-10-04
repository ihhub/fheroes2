# PlayStation Vita port of [fheroes2](https://github.com/ihhub/fheroes2) project

## Install
Download fheroes2.vpk file from https://github.com/Northfear/fheroes2-vita/releases.

Install fheroes2.vpk to your Vita.

FHeroes2 requires data files from the original Heroes of Might and Magic 2.

Copy HEROES2.AGG and HEROES2X.AGG (if you own Price of Loyalty expansion) from the original games "DATA" folder into "ux0:data/fheroes2/data" and everything from "MAPS" folder into "ux0:data/fheroes2/maps".

Data from GoG version of the game is working nicely. Files from the demo version are working too.

Music files in OGG format (from GoG release of the game) should be placed into the "ux0:data/fheroes2/files/music/" folder. To enable OGG music set "music = external" option in "ux0:data/fheroes2/fheroes2.cfg".

rePatch reDux0 plugin is required for proper suspend/resume support

https://github.com/dots-tb/rePatch-reDux0

## Building
### Prerequisites
- libSDL2
- libSDL2-mixer (optional)
- libSDL2-image (optional)
- libSDL2-ttf (optional)

To build the game just run
```
make -f Makefile.vita
```

## Controls
- Left analog stick - Pointer movement
- X button - Left mouse button
- O button - Right mouse button
- D-Pad - Map scrolling
- START - Enter

Text input is done with D-Pad.

- Left - Remove character
- Right - Add new character
- Down - Next character (alphabetically)
- Up - Previous character
- R1, L1 - Switch current character between uppercase/lowercase

## Vita specific options

Pointer movement speed can be changed with 'vita_pointer_speed' parameter in "ux0:data/fheroes2/fheroes2.cfg".

Use "fullscreen = on" option to scale game area to native Vita resolution or "fullscreen = off" to keep game area at the center of the screen.

"vita_keep_aspect_ratio = 1" keeps aspect ratio of original image when scaling. "vita_keep_aspect_ratio = 0" just scales it to 960x544.

Native resolution is supported (set "videomode = 960x544" option in config file or just click on the left door in main menu and select resolution manually. Game restart is required after resolution change).

If "ux0:data/fheroes2/fheroes2.cfg" file is missing, just copy the default one from "ux0:app/FHOMM0002/" to "ux0:data/fheroes2"

fheroes2
======
[![Build status](https://travis-ci.org/ihhub/fheroes2.svg?branch=master)](https://travis-ci.org/ihhub/fheroes2) [![Build status](https://ci.appveyor.com/api/projects/status/ih6cw0yr1yuxf4ll?svg=true)](https://ci.appveyor.com/project/ihhub/fheroes2) [![Github Downloads (monthly)](https://img.shields.io/github/downloads/ihhub/fheroes2/total.svg)](https://github.com/ihhub/fheroes2/releases) [![Discord](https://img.shields.io/discord/733093692860137523.svg?label=&logo=discord&logoColor=ffffff&color=7389D8&labelColor=6A7EC2)](https://discord.gg/xF85vbZ) [![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/paypalme/fheroes2)

Free implementation of Heroes of the Might and Magic II engine.

<img src="files/images/screenshots/screenshot_world_map.png?raw=true" width="410"> <img src="files/images/screenshots/screenshot_castle.png?raw=true" width="410">

Download
---------------------------
You can download a compiled version of the game for your operating system via [**Github releases**](https://github.com/ihhub/fheroes2/releases) of this project.

Please note that for **32-bit** version of the game on **Windows OS** you have to install [**Visual Studio 2015 Redistributable (vc_redist.x86.exe)**](https://www.microsoft.com/en-sg/download/details.aspx?id=48145) if you face an error during application startup.

Copyright
---------------------------
All rights for the original game and its resources belong to former The 3DO Company. These rights were transferred to Ubisoft. We do not encourage and do not support any form of illegal usage of the original game. We strongly advise to purchase the original game on [**GOG**](https://www.gog.com) or [**Ubisoft Store**](https://store.ubi.com) platforms. Alternatively, you can download a free demo version of the game.

Requirements
---------------------------
You are required to have at least a demo version of Heroes of Might and Magic II game to be able to play it. Please use one of our scripts to download the demo version of the original game. A script comes with the compiled game.

MacOS and Linux
--------------------------
Unix OSes need an explicit installation of SDL. Please go to `script/macos` or `script/linux` directory depending on your OS package and run **install_sdl_1.sh** or **install_sdl_2.sh** file. For MacOS we highly recommend to use SDL 2 as latest versions of MacOS does not support SDL 1 fully.


Compilation
---------------------------
[![Maintainability Rating](https://sonarcloud.io/api/project_badges/measure?project=ihhub_fheroes2&metric=sqale_rating)](https://sonarcloud.io/dashboard?id=ihhub_fheroes2) [![Reliability Rating](https://sonarcloud.io/api/project_badges/measure?project=ihhub_fheroes2&metric=reliability_rating)](https://sonarcloud.io/dashboard?id=ihhub_fheroes2) [![Bugs](https://sonarcloud.io/api/project_badges/measure?project=ihhub_fheroes2&metric=bugs)](https://sonarcloud.io/dashboard?id=ihhub_fheroes2) [![Code Smells](https://sonarcloud.io/api/project_badges/measure?project=ihhub_fheroes2&metric=code_smells)](https://sonarcloud.io/dashboard?id=ihhub_fheroes2) [![Duplicated Lines (%)](https://sonarcloud.io/api/project_badges/measure?project=ihhub_fheroes2&metric=duplicated_lines_density)](https://sonarcloud.io/dashboard?id=ihhub_fheroes2) [![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/ihhub/fheroes2.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/ihhub/fheroes2/context:cpp) [![Total alerts](https://img.shields.io/lgtm/alerts/g/ihhub/fheroes2.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/ihhub/fheroes2/alerts/) [![Coverity](https://img.shields.io/coverity/scan/19630.svg)](https://scan.coverity.com/projects/ihhub-fheroes2)

Please follow below instructions to be able to compile the project:

Windows
------------------------
- **Optional step**: Install **7-zip** archiver at its default location as `C:\Program Files\7-Zip\7z.exe` or otherwise, you will need to manually extract each downloaded package (follow instructions shown by batch scripts mentioned below).
- open `script/windows` directory and run **install_packages.bat** file. It installs necessary packages for Visual Studio build.
- open `script/demo` directory and run **demo_windows.bat** file. It downloads a demo version of the game which is needed for minimum development.
- open **fheroes.vcxproj** by Visual Studio and compile the project.

MacOS and Linux
-------------------
- open `script/macos` or `script/linux` directory depending on your OS and run **install_sdl_1.sh** or **install_sdl_2.sh** file. For MacOS we recommend to run SDL 2 as latest versions of MacOS does not support SDL 1 fully.
- open `script/demo` directory and run **demo_macos.sh** or **demo_linux.sh** file depending on your OS. It downloads a demo version of the game which is needed for minimum development.
- run `make` command in root directory of the project. For SDL 2 compilation please run `export WITH_SDL2="ON"` command before compiling the project.

Contribution
---------------------------
We welcome and appreciate any help, even if it is a tiny text or code change. Please read [contribution](https://github.com/ihhub/fheroes2/blob/master/CONTRIBUTING.md) page before starting work on a pull request. All contributors are listed in the project's wiki [page](https://github.com/ihhub/fheroes2/wiki/Contributors). 
Not sure what to start with? Feel free to refer to [good first issue](https://github.com/ihhub/fheroes2/issues?q=is%3Aissue+is%3Aopen+label%3A%22good+first+issue%22) or [help wanted](https://github.com/ihhub/fheroes2/issues?q=is%3Aissue+is%3Aopen+label%3A%22help+wanted%22) tags.

License
---------------------------
This project is under GNU General Public License v2.0. Please refer to file [**LICENSE**](https://github.com/ihhub/fheroes2/blob/master/LICENSE) for more details.

The original project had been developed at [sourceforge](https://sourceforge.net/projects/fheroes2/)

Donation
---------------------------
Currently we accept donations via PayPal. All donations will be used only for the future project development as we do not consider this project as a source of income by any means.

[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/paypalme/fheroes2)
