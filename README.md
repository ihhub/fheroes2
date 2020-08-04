fheroes2
======
[![Build status](https://travis-ci.org/ihhub/fheroes2.svg?branch=master)](https://travis-ci.org/ihhub/fheroes2) [![Build status](https://ci.appveyor.com/api/projects/status/ih6cw0yr1yuxf4ll?svg=true)](https://ci.appveyor.com/project/ihhub/fheroes2) [![Github Downloads (monthly)](https://img.shields.io/github/downloads/ihhub/fheroes2/total.svg)](https://github.com/ihhub/fheroes2/releases) [![Discord](https://img.shields.io/discord/733093692860137523.svg?label=&logo=discord&logoColor=ffffff&color=7389D8&labelColor=6A7EC2)](https://discord.gg/xF85vbZ) [![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/paypalme/fheroes2)

Direct continuation of free implementation of Heroes of the Might and Magic II engine. The original project had been developed at [sourceforge](https://sourceforge.net/projects/fheroes2/).

<img src="files/images/screenshots/screenshot_world_map.png?raw=true" width="410"> <img src="files/images/screenshots/screenshot_castle.png?raw=true" width="410">

Copyright
---------------------------
All rights for the original game and its resources belong to former The 3DO Company. These rights were transferred to Ubisoft. We do not encourage and do not support any form of illegal usage of the original game. We strongly advise to purchase the original game on [**GOG**](https://www.gog.com) or [**Ubisoft Store**](https://store.ubi.com) platforms. Alternatively, you can download a free demo version of the game.

Download
---------------------------
You can download a compiled version of the game for your operating system via [**Github releases**](https://github.com/ihhub/fheroes2/releases) of this project.

Please note that for **32-bit** version of the game on **Windows OS** you have to install [**Visual Studio 2015 Redistributable (vc_redist.x86.exe)**](https://www.microsoft.com/en-sg/download/details.aspx?id=48145) if you face an error during application startup.

Requirements
---------------------------
You are required to have at least a demo version of Heroes of Might and Magic II game to be able to play it. Please use one of our scripts to download the demo version of the original game. A script comes with the compiled game.

**MacOS and Linux**   
Unix OSes need an explicit installation of SDL. Please go to `script/macos` or `script/linux` directory depending on your OS package and run **install_sdl_1.sh** or **install_sdl_2.sh** file. For MacOS we highly recommend to use SDL 2 as latest versions of MacOS does not support SDL 1 fully.


Compilation
---------------------------
[![Maintainability Rating](https://sonarcloud.io/api/project_badges/measure?project=ihhub_fheroes2&metric=sqale_rating)](https://sonarcloud.io/dashboard?id=ihhub_fheroes2) [![Reliability Rating](https://sonarcloud.io/api/project_badges/measure?project=ihhub_fheroes2&metric=reliability_rating)](https://sonarcloud.io/dashboard?id=ihhub_fheroes2) [![Bugs](https://sonarcloud.io/api/project_badges/measure?project=ihhub_fheroes2&metric=bugs)](https://sonarcloud.io/dashboard?id=ihhub_fheroes2) [![Code Smells](https://sonarcloud.io/api/project_badges/measure?project=ihhub_fheroes2&metric=code_smells)](https://sonarcloud.io/dashboard?id=ihhub_fheroes2) [![Duplicated Lines (%)](https://sonarcloud.io/api/project_badges/measure?project=ihhub_fheroes2&metric=duplicated_lines_density)](https://sonarcloud.io/dashboard?id=ihhub_fheroes2) [![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/ihhub/fheroes2.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/ihhub/fheroes2/context:cpp) [![Total alerts](https://img.shields.io/lgtm/alerts/g/ihhub/fheroes2.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/ihhub/fheroes2/alerts/) [![Coverity](https://img.shields.io/coverity/scan/19630.svg)](https://scan.coverity.com/projects/ihhub-fheroes2)

Please follow below instructions to be able to compile the project:

**Windows**
- **Optional step**: Install **7-zip** archiver at its default location as `C:\Program Files\7-Zip\7z.exe` or otherwise, you will need to manually extract each downloaded package (follow instructions shown by batch scripts mentioned below).
- open `script/windows` directory and run **install_packages.bat** file. It installs necessary packages for Visual Studio build.
- open `script/demo` directory and run **demo_windows.bat** file. It downloads a demo version of the game which is needed for minimum development.
- open **fheroes.vcxproj** by Visual Studio and compile the project.

**MacOS and Linux**
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


Donation
---------------------------
Currently we accept donations via PayPal. All donations will be used only for the future project development as we do not consider this project as a source of income by any means.

[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/paypalme/fheroes2)
