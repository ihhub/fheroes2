# [fheroes2](README.md) build and contribution guide

## Building the fheroes2 Project

Please follow the instructions below to be able to build the project from source:

**Note:** You will need to clone the `fheroes2` project before following the instructions below.

### Windows

* Go to the directory `script/windows` and run the file `install_packages.bat`. This script will install all the
  required packages to build the project using Microsoft Visual Studio.
* If you have the original game, then copy the subdirectories `ANIM`, `DATA`, `MAPS` and `MUSIC` (some of them may
  be missing depending on the version of the original game) to the project root directory.
* Alternatively, go to the directory `script/demo` and run the file `download_demo_version.bat`.
  This script will download a demo version of the original game, which is the minimum required for development.
* Open the file `fheroes2-vs2019.vcxproj` (targeted for Visual Studio 2019) and build the project.
* Visual Studio will automatically copy game files in the root directory to the build directory.

### macOS and Linux

* Depending on your OS, run the following scripts to install the dependencies required for the build:
  * macOS: go to the directory `script/macos` and run `brew bundle` to install the required dependencies.
  * Linux: go to the directory `script/linux` and run the file `install_sdl2_dev.sh`.
* If you have the original game, then copy the subdirectories `ANIM`, `DATA`, `MAPS` and `MUSIC` (some of them may
  be missing depending on the version of the original game) to the project root directory.
* Alternatively, go to the directory `script/demo` and run the file `download_demo_version.sh`.
  This script will download a demo version of the original game, which is the minimum required for development.
* Run the `make` command in the root directory of the project to build it.

### Android

* Go to the directory `script/android` and run the file `install_packages.bat` if your development platform is Windows or
  `install_packages.sh` if your development platform is macOS or Linux. This script will install all the required packages
  to build the project using Android Studio.
* Launch Android Studio, open the project in the `android` directory and run the build.

### iOS

* To build this build you need to have the necessary Data, Anim, Maps and Music folders for the engine present in the fheroes2 project's folder.
* Download and install Xcode on your macos device. Download the iOS SDK during installation.
* Go to the directory `script/ios` and run the file `install_packages.sh`. This script will install all the required packages to build the project using Xcode.
* Open `fheroes2.xcodeproj` in the "iOS" folder".
* In the Editor area select "fheroes2" under the "Targets" menu instead of "Projects". Open the "Signing & Capabilities" tab. Under "Signing", add your AppleID in
the "Team" field.
* Connect your iOS device to your mac device with the necessary cable. Select the device in the devices in Xcode's upper Toolbar.
* Click the play button or command + B to build. If asked if you want to download the Metal toolchain, agree to this as it is necessary for the app. After this
you will be prompted to enable developer mode on your iOS device. Follow the steps provided on your macOS and iOS devices to enable this.
* Once you have setup developer mode on your device and added your AppleID to Xcode, you can build and run the project. The first build will likely take a while.
Once fheroes2 starts on your iOS device a white screen will show for a minute or more.

### PlayStation Vita

If you would like to build and run this project on PlayStation Vita please follow the instructions on [**this page**](README_PSV.md).

### Nintendo Switch

If you would like to build and run this project on Nintendo Switch please follow the instructions on [**this page**](README_switch.md).

### Emscripten (Wasm)

If you would like to run this project in a web browser please follow the instructions on [**this page**](README_emscripten.md).

### Build with CMake

If you would like to build the project using CMake please follow the instructions on [**this page**](README_cmake.md).

## Building the front end website

We host the website on Github pages, which is a highly customized version of the
Jekyll static site engine. The instructions for developing the website can be
found in the [**website local dev**](WEBSITE_LOCAL_DEV.md) guide.

## Contribution

We welcome and appreciate any help, even if it is a tiny text or code change. Please read our
[**contribution guide**](https://github.com/ihhub/fheroes2/blob/master/CONTRIBUTING.md) before starting work on a pull request.
Not sure what to start with? Feel free to refer to
[**good first issue**](https://github.com/ihhub/fheroes2/issues?q=is%3Aissue%20is%3Aopen%20label%3A%22good%20first%20issue%22) or
[**help wanted**](https://github.com/ihhub/fheroes2/issues?q=is%3Aissue%20is%3Aopen%20label%3A%22help%20wanted%22) tags.
