#!/usr/bin/env bash

set -e

function echo_red {
    echo -e "\033[0;31m$*\033[0m"
}

function echo_green {
    echo -e "\033[0;32m$*\033[0m"
}

function echo_yellow {
    echo -e "\033[0;33m$*\033[0m"
}

function echo_stage {
    echo
    echo_green "$*"
    echo
}

function verify_homm2_path {
    [[ -f "$1/DATA/HEROES2.AGG" && -d "$1/MAPS" ]]
}

echo_green "This script will extract and copy game resources from the original distribution of Heroes of Might and Magic II"

echo_stage "[1/3] determining the destination directory"

DEST_PATH=""

if [[ -n "$2" ]]; then
    DEST_PATH="$2"
elif [[ -f fheroes2 && -x fheroes2 ]]; then
    DEST_PATH="."
elif [[ -d ../../src ]]; then
    # Special hack for developers running this script from the source tree
    DEST_PATH="../.."
fi

if [[ -z "$DEST_PATH" || ! -d "$DEST_PATH" || ! -w "$DEST_PATH" ]]; then
    if [[ "$(uname 2> /dev/null)" == "Linux" ]]; then
        DEST_PATH="${XDG_CONFIG_HOME:-$HOME/.local/share}/fheroes2"
    elif [[ -z "$2" ]]; then
        DEST_PATH="$HOME/.fheroes2"
    fi
fi

echo_green "Destination directory: $DEST_PATH"

echo_stage "[2/3] determining the HoMM2 directory or installer package"

if [[ "$#" -gt "0" ]]; then
    HOMM2_PATH="$1"
else
    read -e -p "Please enter the full path to the HoMM2 directory or installer package (e.g. /home/user/homm2 or /tmp/installer.exe): " HOMM2_PATH
fi

if [[ -f "$HOMM2_PATH" ]]; then
    if [[ -z "$(command -v innoextract)" ]]; then
        echo_red "innoextract was not found in your system. Unable to extract the installer package. Installation aborted."
        exit 1
    fi

    EXTRACT_DIR="$(mktemp -d)"

    echo_green "Verifying the installer package, please wait..."

    innoextract -e -s -d "$EXTRACT_DIR" -- "$HOMM2_PATH"

    for SUBDIR in . app; do
        if verify_homm2_path "$EXTRACT_DIR/$SUBDIR"; then
            echo_green "HoMM2 installer package: $HOMM2_PATH"

            HOMM2_PATH="$EXTRACT_DIR/$SUBDIR"

            break
        fi
    done

    if [[ -f "$HOMM2_PATH" ]]; then
        echo_red "Unable to find HoMM2 files in this installer package. Installation aborted."
        exit 1
    fi
elif verify_homm2_path "$HOMM2_PATH"; then
    echo_green "HoMM2 directory: $HOMM2_PATH"
else
    echo_red "Unable to find the HoMM2 directory or installer package. Installation aborted."
    exit 1
fi

echo_stage "[3/3] copying game resources"

[[ ! -d "$DEST_PATH" ]] && mkdir -p "$DEST_PATH"

cd "$DEST_PATH"

[[ ! -d anim ]]  && mkdir anim
[[ ! -d data ]]  && mkdir data
[[ ! -d maps ]]  && mkdir maps
[[ ! -d music ]] && mkdir music

[[ -d "$HOMM2_PATH/HEROES2/ANIM" ]] && cp -r "$HOMM2_PATH/HEROES2/ANIM"/* anim
[[ -d "$HOMM2_PATH/ANIM" ]]         && cp -r "$HOMM2_PATH/ANIM"/*         anim
[[ -d "$HOMM2_PATH/DATA" ]]         && cp -r "$HOMM2_PATH/DATA"/*         data
[[ -d "$HOMM2_PATH/MAPS" ]]         && cp -r "$HOMM2_PATH/MAPS"/*         maps
[[ -d "$HOMM2_PATH/MUSIC" ]]        && cp -r "$HOMM2_PATH/MUSIC"/*        music

if [[ ! -f "$HOMM2_PATH/homm2.gog" ]]; then
    exit 0
fi

# Special case - CD image from GOG

if [[ -n "$(command -v python3)" ]]; then
    PYTHON="python3"
elif [[ -n "$(command -v python2)" ]]; then
    PYTHON="python2"
elif [[ -n "$(command -v python)" ]]; then
    PYTHON="python"
else
    echo_yellow "python was not found in your system. Please install it and re-run this script to extract animation resources."
    exit 0
fi

if [[ -z "$(command -v bsdtar)" ]]; then
    echo_yellow "bsdtar was not found in your system. Please install it and re-run this script to extract animation resources."
    exit 0
fi

echo_green "Extracting animation resources, please wait..."

"$PYTHON" - << EOF
with open("$HOMM2_PATH/homm2.gog", "rb") as raw_file:
    with open("homm2.iso", "wb") as iso_file:
        while True:
            buf = raw_file.read(2352)
            if not buf:
                break
            if buf[15] == 2:
                iso_file.write(buf[24:2072])
            else:
                iso_file.write(buf[16:2064])
EOF

bsdtar -x -f homm2.iso -C anim --include "HEROES2/ANIM/*" --strip-components=2

rm -f homm2.iso
