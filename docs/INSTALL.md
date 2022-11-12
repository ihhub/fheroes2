# [**fheroes2**](README.md) installation guide

## Requirements

You will need to have a demo version or the full version of **Heroes of Might and Magic II** game to be able to play. We strongly
advise to purchase the original game on [**GOG**](https://www.gog.com) or [**Ubisoft Store**](https://store.ubi.com) platforms.

Alternatively, you can **download a free demo version of the game (in English only) using the bundled script**. See detailed
instructions below.

## Installation

Precompiled binaries of the release version are currently available for the following platforms and operating systems:

* [**Windows**](#windows)
  * [**Windows installer**](#windows-installer)
  * [**Windows ZIP archive**](#windows-zip-archive)
* [**macOS**](#macos)
  * [**MacPorts**](#macports)
  * [**Homebrew**](#homebrew-mac)
  * [**macOS native app**](#macos-native-app)
  * [**macOS ZIP archive**](#macos-zip-archive)
* [**Linux**](#linux)
  * [**AUR package**](#aur-package)
  * [**Homebrew**](#homebrew-linux)
  * [**Flatpak**](#flatpak-linux)
  * [**Linux ZIP archive**](#linux-zip-archive)
* [**PlayStation Vita**](#playstation-vita)
* [**Nintendo Switch**](#nintendo-switch)

Alternatively, you can download the precompiled binaries of the latest commit (snapshot) [**here**](#snapshots-latest-builds).

<a name="windows"></a>
## Windows

<a name="windows-installer"></a>
### Windows installer

* Download one of the following Windows installer packages:

  * **Windows x64 (64-bit)**:<br>
  [**SDL2 (recommended)**](https://github.com/ihhub/fheroes2/releases/latest/download/fheroes2_windows_x64_SDL2_installer.exe) or<br>
  [**SDL1**](https://github.com/ihhub/fheroes2/releases/latest/download/fheroes2_windows_x64_SDL1_installer.exe)

  * **Windows x86 (32-bit)**:<br>
  [**SDL2 (recommended)**](https://github.com/ihhub/fheroes2/releases/latest/download/fheroes2_windows_x86_SDL2_installer.exe) or<br>
  [**SDL1**](https://github.com/ihhub/fheroes2/releases/latest/download/fheroes2_windows_x86_SDL1_installer.exe)

* After downloading the installer, launch it and follow the instructions.

* During the installation process, you will be prompted to extract game resources from the original game (select this option if you
  already have a legally purchased copy of the original game installed), or download the demo version of the original game and extract
  game resources from it (you can also do this later by clicking the appropriate shortcut in the program's group in the Windows Start
  menu).

* If you purchased a copy of the original game only after installing fheroes2, you can run the `Extract game resources from the original
  distribution of HoMM2` shortcut in the program's group in the Windows Start menu. This script will try to perform an automatic search
  for an existing installation of the original game and extract all the necessary resource files. If it can't find an existing installation,
  you will be prompted to enter the location of the original game manually.

* As an alternative to the previous step, you can manually copy the subdirectories `ANIM`, `DATA`, `MAPS` and `MUSIC` (some of them may
  be missing depending on the version of the original game) from the original game directory to the fheroes2 installation directory.

<a name="windows-zip-archive"></a>
### Windows ZIP archive

* Download one of the following Windows ZIP archives:

  * **Windows x64 (64-bit)**:<br>
  [**SDL2 (recommended)**](https://github.com/ihhub/fheroes2/releases/latest/download/fheroes2_windows_x64_SDL2.zip) or<br>
  [**SDL1**](https://github.com/ihhub/fheroes2/releases/latest/download/fheroes2_windows_x64_SDL1.zip)

  * **Windows x86 (32-bit)**:<br>
  [**SDL2 (recommended)**](https://github.com/ihhub/fheroes2/releases/latest/download/fheroes2_windows_x86_SDL2.zip) or<br>
  [**SDL1**](https://github.com/ihhub/fheroes2/releases/latest/download/fheroes2_windows_x86_SDL1.zip)

* After downloading the ZIP archive, extract it to a suitable directory of your choice.

* If you have a legally purchased copy of the original game, run the `extract_homm2_resources.bat` script supplied in the ZIP archive.
  This script will try to perform an automatic search for an existing installation of the original game and extract all the necessary
  resource files. If it can't find an existing installation, you will be prompted to enter the location of the original game manually.

* As an alternative to the previous step, you can manually copy the subdirectories `ANIM`, `DATA`, `MAPS` and `MUSIC` (some of them may
  be missing depending on the version of the original game) from the original game directory to the fheroes2 installation directory.

* If you don't have a legally purchased copy of the original game, you can download and install the demo version of the original game
  by running the `download_demo_version.bat` script supplied in the ZIP archive.

<a name="macos"></a>
## macOS

<a name="macports"></a>
### MacPorts

If you are using [**MacPorts**](https://www.macports.org/), you can install the game by running the following command:

```sh
port install fheroes2
```

Then follow the instructions on the screen.

<a name="homebrew-mac"></a>
### Homebrew

If you are using [**Homebrew**](https://brew.sh/), you can install the game by running the following command:

```sh
brew install fheroes2
```

<a name="macos-native-app"></a>
### macOS native app

* Download the source and compile with the `-DMACOS_APP_BUNDLE=ON` option (if using CMake) or using the following commands (with make):

```sh
make FHEROES2_MACOS_APP_BUNDLE=ON
make FHEROES2_MACOS_APP_BUNDLE=ON bundle
```

* If you have a legally purchased copy of the original game, run the `extract_homm2_resources_for_app_bundle.sh` script supplied in the
  `script/homm2` directory. You will be prompted to enter the location of the original game, and the script will extract all the
  necessary resource files.

* As an alternative to the previous step, you can manually copy the subdirectories `ANIM`, `DATA`, `MAPS` and `MUSIC` (some of them may
  be missing depending on the version of the original game) from the original game directory to `~/Library/Application Support/fheroes2`.

* If you don't have a legally purchased copy of the original game, you can download and install the demo version of the original game
  by running the `download_demo_version_for_app_bundle.sh` script supplied in `script/demo` directory.

<a name="macos-zip-archive"></a>
### macOS ZIP archive

* Download the [**macOS ZIP archive**](https://github.com/ihhub/fheroes2/releases/latest/download/fheroes2_macos_x86-64_SDL2.zip).
  Currently only x86-64 binaries are provided. If you use a machine with an Apple Silicon chip, you should choose another installation
  method (using [**MacPorts**](#macports) or [**Homebrew**](#homebrew-mac)), or
  [**build the project from source**](DEVELOPMENT.md#macos-and-linux).

* After downloading the ZIP archive, extract it to a suitable directory of your choice and then run the script `install_sdl_1.sh` or
  `install_sdl_2.sh` (depending on the downloaded build) from the `script/macos` subdirectory. This will install the SDL libraries
  required to run the game.

* If you have a legally purchased copy of the original game, run the `extract_homm2_resources.sh` script supplied in the ZIP archive.
  You will be prompted to enter the location of the original game, and the script will extract all the necessary resource files.

* As an alternative to the previous step, you can manually copy the subdirectories `ANIM`, `DATA`, `MAPS` and `MUSIC` (some of them may
  be missing depending on the version of the original game) from the original game directory to the fheroes2 installation directory.

* If you don't have a legally purchased copy of the original game, you can download and install the demo version of the original game
  by running the `download_demo_version.sh` script supplied in the ZIP archive.

<a name="linux"></a>
## Linux

<a name="aur-package"></a>
### AUR package

If you are using Arch Linux or compatible distribution, you can install [fheroes2 package](https://aur.archlinux.org/packages/fheroes2)
from AUR (Arch User Repository).

#### Install using AUR helper

If you use one of AUR helpers, e.g. `yay`, you can install the game by running the following command:

```sh
yay -S aur/fheroes2
```

#### Install using official guide

Follow [official guide](https://wiki.archlinux.org/title/Arch_User_Repository#Installing_and_upgrading_packages).
One of possible command sets:

```sh
git clone https://aur.archlinux.org/fheroes2.git
cd fheroes2
makepkg -si
```

<a name="homebrew-linux"></a>
### Homebrew

If you are using [**Homebrew**](https://brew.sh/), you can install the game by running the following command:

```sh
brew install fheroes2
```

If you have a legally purchased copy of the original game, copy the subdirectories `ANIM`, `DATA`, `MAPS` and `MUSIC` (some of them may
be missing depending on the version of the original game) from the original game directory to the `$XDG_DATA_HOME/fheroes2` (usually
`~/.local/share/fheroes2`) directory. Otherwise, you can download and install the demo version of the original game by running the
`/usr/share/fheroes2/download_demo_version.sh` script.

<a name="flatpak-linux"></a>
### Flatpak

If you are using [**Flatpak**](https://flatpak.org) and [**Flathub**](https://flathub.org), you can install the game by running the
following command:

```sh
flatpak install flathub io.github.ihhub.Fheroes2
```

And launch from the start menu or the console:

```sh
flatpak run io.github.ihhub.Fheroes2
```

Alternatively, you can use an application manager like Discover, which is also available on Steam Deck.

You have to copy the subdirectories `ANIM`, `DATA`, `MAPS` and `MUSIC` from the original game or demo directory to the
`~/.var/app/io.github.ihhub.Fheroes2/data/fheroes2` directory. You will be asked when you start the application.

<a name="linux-zip-archive"></a>
### Linux ZIP archive

* Download one of the following Linux ZIP archives:<br>
  [**SDL2 (recommended)**](https://github.com/ihhub/fheroes2/releases/latest/download/fheroes2_ubuntu_SDL2.zip) or<br>
  [**SDL1**](https://github.com/ihhub/fheroes2/releases/latest/download/fheroes2_ubuntu_SDL1.zip).

* After downloading the ZIP archive, extract it to a suitable directory of your choice. Then you will need to install the SDL libraries
  required to run the game. The installation procedure depends on the Linux distribution you are using:

  * **Debian-based**: run the script `install_sdl_1.sh` or `install_sdl_2.sh` (depending on the downloaded build) from the `script/linux`
    subdirectory.

  * **Pacman-based (e.g. Arch Linux)**: run one of the following commands: `sudo pacman -S sdl sdl_mixer` or `sudo pacman -S sdl2 sdl2_mixer`
    (depending on the downloaded build).

  * **RedHat-based**: for RPM-based distributions (such as Fedora or RedHat) use the commands `sudo yum install SDL*` or `sudo dnf install SDL*`.

  * **openSUSE**: openSUSE supports the One-Click-Install using the `SDL_mixer.ymp` file from the `script/linux` subdirectory.

  * **Gentoo**: run the following command: `emerge --ask media-libs/sdl2-mixer`.

* After all dependencies are installed, run the `extract_homm2_resources.sh` script supplied in the ZIP archive if you have a legally purchased
  copy of the original game. You will be prompted to enter the location of the original game, and the script will extract all the necessary
  resource files.

* As an alternative to the previous step, you can manually copy the subdirectories `ANIM`, `DATA`, `MAPS` and `MUSIC` (some of them may
  be missing depending on the version of the original game) from the original game directory to the fheroes2 installation directory.

* If you don't have a legally purchased copy of the original game, you can download and install the demo version of the original game
  by running the `download_demo_version.sh` script supplied in the ZIP archive.

<a name="playstation-vita"></a>
## PlayStation Vita

**Please note**: you need to be running custom firmware for it to work.

* Download the [**PlayStation Vita ZIP archive**](https://github.com/ihhub/fheroes2/releases/latest/download/fheroes2_psv.zip).

* Follow the [**instructions**](README_PSV.md).

<a name="nintendo-switch"></a>
## Nintendo Switch

**Please note**: you need to be running custom firmware for it to work.

* Download the [**Nintendo Switch ZIP archive**](https://github.com/ihhub/fheroes2/releases/latest/download/fheroes2_switch.zip).

* Follow the [**instructions**](README_switch.md).

<a name="snapshots-latest-builds"></a>
## Snapshots (latest builds)

You can download the precompiled binaries of the latest commit (snapshot) for
**Windows** (
[**x64 SDL2**](https://github.com/ihhub/fheroes2/releases/tag/fheroes2-windows-x64-SDL2),
[**x64 SDL1**](https://github.com/ihhub/fheroes2/releases/tag/fheroes2-windows-x64-SDL1),
[**x86 SDL2**](https://github.com/ihhub/fheroes2/releases/tag/fheroes2-windows-x86-SDL2) and
[**x86 SDL1**](https://github.com/ihhub/fheroes2/releases/tag/fheroes2-windows-x86-SDL1)
),
**macOS x86-64** (
[**SDL2**](https://github.com/ihhub/fheroes2/releases/tag/fheroes2-osx-sdl2_dev) and
[**SDL1**](https://github.com/ihhub/fheroes2/releases/tag/fheroes2-osx-sdl1_dev)
),
**Ubuntu** (
[**SDL2**](https://github.com/ihhub/fheroes2/releases/tag/fheroes2-linux-sdl2_dev) and
[**SDL1**](https://github.com/ihhub/fheroes2/releases/tag/fheroes2-linux-sdl1_dev)
),
[**Android**](https://github.com/ihhub/fheroes2/releases/tag/fheroes2-android),
[**PlayStation Vita**](https://github.com/ihhub/fheroes2/releases/tag/fheroes2-psv-sdl2_dev) and
[**Nintendo Switch**](https://github.com/ihhub/fheroes2/releases/tag/fheroes2-switch-sdl2_dev).
**These binaries incorporate all the latest changes, but also all the latest bugs, and are mainly intended for developers.
DON'T EXPECT THEM TO WORK PROPERLY.**
