# [**fheroes2**](README.md) build and contribution guide

## Build

Please follow the instructions below to be able to build the project from source:

### Windows

* Go to the directory `script/windows` and run the file `install_packages.bat`. This script will install all the required packages to build the project using Microsoft Visual Studio.
* Go to the directory `script/demo` and run the file `demo_windows.bat`. This script will download a demo version of the original game, which is the minimum required for development.
* Open the file `fheroes2-vs2015.vcxproj` or `fheroes2-vs2019.vcxproj` depending on the version of Microsoft Visual Studio installed and build the project.

### macOS and Linux

* Depending on your OS, run the following scripts to install the dependencies required for the build:
  * macOS: go to the directory `script/macos` and run the file `install_sdl_2.sh` (default setup) or `install_sdl_1.sh`.
  * Linux: go to the directory `script/linux` and run the file `install_sdl_2_dev.sh` (default setup) or `install_sdl_1_dev.sh`.
* Go to the directory `script/demo` and run the file `demo_unix.sh`. This script will download a demo version of the original game, which is the minimum required for development.
* Run the `make` command in the root directory of the project to build it with **SDL2** or `make FHEROES2_SDL1="ON"` to build it with **SDL1**.

### PS Vita

If you would like to build and run this project on PS Vita please follow the instructions on [**this page**](README_PSV.md).

### Build with CMake

If you would like to build the project using CMake please follow the instructions on [**this page**](README_cmake.md).

## Contribution

We welcome and appreciate any help, even if it is a tiny text or code change. Please read our [**contribution guide**](https://github.com/ihhub/fheroes2/blob/master/CONTRIBUTING.md) before starting work on a pull request.
Not sure what to start with? Feel free to refer to [**good first issue**](https://github.com/ihhub/fheroes2/issues?q=is%3Aissue+is%3Aopen+label%3A%22good+first+issue%22) or [**help wanted**](https://github.com/ihhub/fheroes2/issues?q=is%3Aissue+is%3Aopen+label%3A%22help+wanted%22) tags.
