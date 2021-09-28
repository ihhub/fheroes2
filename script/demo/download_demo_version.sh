#!/bin/bash

set -e

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

echo_stage "[1/4] determining destination directory"

DEST_PATH=""

if [[ -f fheroes2 && -x fheroes2 ]]; then
    DEST_PATH="."
elif [[ -d ../../src ]]; then
    # Special hack for developers running this script from the source tree
    DEST_PATH="../.."
fi

if [[ -z "$DEST_PATH" || ! -d "$DEST_PATH" || ! -w "$DEST_PATH" ]]; then
    if [[ "$(uname 2> /dev/null)" == "Linux" ]]; then
        DEST_PATH="${XDG_CONFIG_HOME:-$HOME/.local/share}/fheroes2"
    else
        DEST_PATH="$HOME/.fheroes2"
    fi
fi

echo_green "Destination directory: $DEST_PATH"

echo_stage "[2/4] downloading demo version"

[[ ! -d "$DEST_PATH/demo" ]] && mkdir -p "$DEST_PATH/demo"

cd "$DEST_PATH/demo"

if [[ "$(command -v wget)" != "" ]]; then
    wget -O h2demo.zip "$H2DEMO_URL"
elif [[ "$(command -v curl)" != "" ]]; then
    curl -O -L "$H2DEMO_URL" > h2demo.zip
else
    echo_red "wget or curl not found in your system. Unable to download demo version. Installation aborted."
    exit 1
fi

echo "$H2DEMO_SHA256 *h2demo.zip" > checksums

if [[ "$(command -v shasum)" != "" ]]; then
    shasum --check --algorithm 256 checksums
elif [[ "$(command -v sha256sum)" != "" ]]; then
    sha256sum --check --strict checksums
else
    echo_red "shasum or sha256sum not found in your system. Unable to verify downloaded file. Installation aborted."
    exit 1
fi

echo_stage "[3/4] unpacking archives"

unzip -o h2demo.zip

echo_stage "[4/4] copying files"

[[ ! -d ../data ]] && mkdir ../data
[[ ! -d ../maps ]] && mkdir ../maps

cp -r DATA/* ../data
cp -r MAPS/* ../maps
