#!/usr/bin/env bash

###########################################################################
#   fheroes2: https://github.com/ihhub/fheroes2                           #
#   Copyright (C) 2021 - 2023                                             #
#                                                                         #
#   This program is free software; you can redistribute it and/or modify  #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation; either version 2 of the License, or     #
#   (at your option) any later version.                                   #
#                                                                         #
#   This program is distributed in the hope that it will be useful,       #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#   GNU General Public License for more details.                          #
#                                                                         #
#   You should have received a copy of the GNU General Public License     #
#   along with this program; if not, write to the                         #
#   Free Software Foundation, Inc.,                                       #
#   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             #
###########################################################################

set -e -o pipefail

H2DEMO_URL="https://archive.org/download/HeroesofMightandMagicIITheSuccessionWars_1020/h2demo.zip"
H2DEMO_SHA256="12048c8b03875c81e69534a3813aaf6340975e77b762dc1b79a4ff5514240e3c"

function echo_red {
    echo -e "\033[0;31m$*\033[0m"
}

function echo_green {
    echo -e "\033[0;32m$*\033[0m"
}

function echo_stage {
    echo
    echo_green "$*"
    echo
}

echo_green "This script will download the demo version of the original Heroes of Might and Magic II"
echo_green "It may take a few minutes, please wait..."

echo_stage "[1/4] determining the destination directory"

DEST_PATH=""

if [[ -n "$1" ]]; then
    DEST_PATH="$1"
elif [[ -f fheroes2 && -x fheroes2 ]]; then
    DEST_PATH="."
elif [[ -d ../../src ]]; then
    # Special hack for developers running this script from the source tree
    DEST_PATH="../.."
fi

if [[ -z "$DEST_PATH" || ! -d "$DEST_PATH" || ! -w "$DEST_PATH" ]]; then
    if [[ "$(uname 2> /dev/null)" == "Linux" ]]; then
        DEST_PATH="${XDG_CONFIG_HOME:-$HOME/.local/share}/fheroes2"
    elif [[ -z "$1" ]]; then
        DEST_PATH="$HOME/.fheroes2"
    fi
fi

echo_green "Destination directory: $DEST_PATH"

echo_stage "[2/4] downloading the demo version"

[[ ! -d "$DEST_PATH/demo" ]] && mkdir -p "$DEST_PATH/demo"

cd "$DEST_PATH/demo"

if [[ -n "$(command -v wget)" ]]; then
    wget -O h2demo.zip "$H2DEMO_URL"
elif [[ -n "$(command -v curl)" ]]; then
    curl -o h2demo.zip -L "$H2DEMO_URL"
else
    echo_red "Neither wget nor curl were found in your system. Unable to download the demo version. Installation aborted."
    exit 1
fi

echo "$H2DEMO_SHA256 *h2demo.zip" > checksums

if [[ -n "$(command -v shasum)" ]]; then
    shasum --check --algorithm 256 checksums
elif [[ -n "$(command -v sha256sum)" ]]; then
    sha256sum --check --strict checksums
else
    echo_red "Neither shasum nor sha256sum were found in your system. Unable to verify the downloaded file. Installation aborted."
    exit 1
fi

echo_stage "[3/4] unpacking archives"

unzip -o h2demo.zip

echo_stage "[4/4] copying files"

[[ ! -d ../data ]] && mkdir ../data
[[ ! -d ../maps ]] && mkdir ../maps

cp -r DATA/* ../data
cp -r MAPS/* ../maps
