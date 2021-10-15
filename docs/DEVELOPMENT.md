# [**fheroes2**](README.md) build and contribution guide

## Build

Please follow the instructions below to be able to build the project from source:

### Windows

* Go to the directory `script/windows` and run the file `install_packages.bat`. This script will install all the
  required packages to build the project using Microsoft Visual Studio.
* Go to the directory `script/demo` and run the file `download_demo_version.bat`. This script will download a demo
  version of the original game, which is the minimum required for development.
* Open the file `fheroes2-vs2015.vcxproj` or `fheroes2-vs2019.vcxproj` depending on the version of Microsoft Visual
  Studio installed and build the project.

#### Visual Studio development on Windows

After executing the `download_demo_version.bat` script, the demo version files will be downloaded and extracted to
a folder called `.\demo` within the project root folder.  The script will also copy to project root directory the
minimum required files for development purposes (namely, the `data` and `maps` directories).  By default, the
Visual Studio project will automatically copy these folders to the build directory (i.e. `.\build\x64\Debug-SDL2`)
so they are available for testing.

If you have a copy of the original game and wish to include its files in your development (such as the CD music,
extra maps, and animations), then you need to make these original game files available to the build.

You have two options:
1. Copy the files to the `~\{USER_FOLDER}\AppData\Roaming\fheroes2` directory.
	* The files will be used at runtime by the build and will not be removed by cleaning the solution.
2. Copy the files to the build directory (i.e. `.\build\x64\Debug-SDL2`).
	* The files will need to be copied again to the build directory after cleaning the solution.

### macOS and Linux

* Depending on your OS, run the following scripts to install the dependencies required for the build:
  * macOS: go to the directory `script/macos` and run the file `install_sdl_2.sh` (default setup) or `install_sdl_1.sh`.
  * Linux: go to the directory `script/linux` and run the file `install_sdl_2_dev.sh` (default setup) or `install_sdl_1_dev.sh`.
* Go to the directory `script/demo` and run the file `download_demo_version.sh`. This script will download a demo version of the
  original game, which is the minimum required for development.
* Run the `make` command in the root directory of the project to build it with **SDL2** or `make FHEROES2_SDL1="ON"` to build it
  with **SDL1**.

### PlayStation Vita

If you would like to build and run this project on PlayStation Vita please follow the instructions on [**this page**](README_PSV.md).

### Nintendo Switch

If you would like to build and run this project on Nintendo Switch please follow the instructions on [**this page**](README_switch.md).

### Build with CMake

If you would like to build the project using CMake please follow the instructions on [**this page**](README_cmake.md).

## Contribution

We welcome and appreciate any help, even if it is a tiny text or code change. Please read our
[**contribution guide**](https://github.com/ihhub/fheroes2/blob/master/CONTRIBUTING.md) before starting work on a pull request.
Not sure what to start with? Feel free to refer to
[**good first issue**](https://github.com/ihhub/fheroes2/issues?q=is%3Aissue%20is%3Aopen%20label%3A%22good%20first%20issue%22) or
[**help wanted**](https://github.com/ihhub/fheroes2/issues?q=is%3Aissue%20is%3Aopen%20label%3A%22help%20wanted%22) tags.
