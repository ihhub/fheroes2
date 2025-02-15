#!/usr/bin/env bash

###########################################################################
#   fheroes2: https://github.com/ihhub/fheroes2                           #
#   Copyright (C) 2025                                                    #
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

echo_green "This script will extract and copy game resources from the original Heroes of Might and Magic II CD image."

echo_stage "[1/4] determining the destination directory"

DEST_PATH=""

if [[ -n "$2" ]]; then
    DEST_PATH="$2"
elif [[ -f fheroes2 && -x fheroes2 ]]; then
    DEST_PATH="."
elif [[ -d ../../src ]]; then
    # Special hack for developers running this script from the source tree
    DEST_PATH="../.."
fi

if [[ -z "$DEST_PATH" || ( -d "$DEST_PATH" && ! -w "$DEST_PATH" ) ]]; then
    if [[ "$(uname 2> /dev/null)" == "Linux" ]]; then
        DEST_PATH="${XDG_CONFIG_HOME:-$HOME/.local/share}/fheroes2"
    elif [[ -z "$2" ]]; then
        DEST_PATH="$HOME/.fheroes2"
    fi
fi

echo_green "Destination directory: $DEST_PATH"

echo_stage "[2/4] determining the path to the HoMM2 CD image"

if [[ "$#" -gt "0" ]]; then
    HOMM2_CD_IMAGE_PATH="$1"
else
    read -e -p "Please enter the full path to the HoMM2 CD image (e.g. /home/user/homm2.cue or /home/user/homm2.iso): " HOMM2_CD_IMAGE_PATH
fi

STAGE_PATH=$(mktemp -d)
trap 'rm -rf "$STAGE_PATH"' EXIT

function stage_iso_assets {
    if [[ -z "$(command -v fuseiso)" ]]; then
        echo_red "fuseiso was not found in your system. Unable to mount CD image. Installation aborted."
        exit 1
    fi
    if [[ -z "$(command -v fusermount)" ]]; then
        echo_red "fusermount was not found in your system. Unable to unmount CD image. Installation aborted."
        exit 1
    fi

    mkdir ROM
    fuseiso "$1" ROM
    trap 'fusermount -u ROM' RETURN

    HOMM2_DIR=$(find ROM -type d -iname heroes2)
    if [[ -z "$HOMM2_DIR" ]]; then
        echo_red "Unable to locate 'heroes2' directory in CD image."
        exit 1
    fi

    copy_directory "$HOMM2_DIR" "ANIM"

    DATA_CAB_PATH=$(find "ROM" -type f -iname data1.cab)
    if [[ -n "$DATA_CAB_PATH" ]]; then
        if [[ -z "$(command -v unshield)" ]]; then
            echo_red "unshield was not found in your system. Unable to extract 'data1.cab'. Installation aborted."
            exit 1
        fi
        unshield -d "CAB" x "$DATA_CAB_PATH" > /dev/null

        copy_directory "CAB" "MAPS"
        copy_directory "CAB" "DATA"

        rm -rf CAB
    else
        copy_directory "$HOMM2_DIR" "MAPS"
        copy_directory "$HOMM2_DIR" "DATA"
    fi

    fusermount -u ROM
    trap - RETURN

    rmdir ROM
}

function copy_directory() {
    BASE_DIR=$1
    DIR_NAME=$2

    DIR_PATH=$(find "$BASE_DIR" -type d -iname "$DIR_NAME")
    if [[ -n "$DIR_PATH" ]]; then
        mkdir "$DIR_NAME"

        cp -r "$DIR_PATH/." "$DIR_NAME"
    fi
}

echo_stage "[3/4] staging game resources"
cd "$STAGE_PATH"
CD_IMAGE_FILE_NAME=$(basename "$HOMM2_CD_IMAGE_PATH")
CD_IMAGE_EXTENSION=${CD_IMAGE_FILE_NAME##*.}
if [[ "${CD_IMAGE_EXTENSION^^}" == "CUE" ]]; then
    if [[ -z "$(command -v bchunk)" ]]; then
        echo_red "bchunk was not found in your system. Unable to rip CD audio. Installation aborted."
        exit 1
    fi
    # Parse the path of the bin file directly from the CUE sheet, so that this
    # will continue to work even if the name of the cue sheet doesn't match the
    # name of the bin file.
    BIN_PATH="$(dirname "$HOMM2_CD_IMAGE_PATH")/$(cat "$HOMM2_CD_IMAGE_PATH" | grep FILE | grep -oE '"([^"]+)"' | tr -d '"')"
    bchunk -w "$BIN_PATH" "$HOMM2_CD_IMAGE_PATH" tmp_ > /dev/null

    if [[ -z "$(command -v flac)" ]]; then
        echo_red "flac was not found in your system. Unable to reencode CD audio. Installation aborted."
        exit 1
    fi
    mkdir MUSIC
    for WAV_PATH in *.wav; do
      # The music ripped from the CD starts from index 2, since the first index
      # is occupied by the ISO data.  However, the engine expects the music to
      # start from index zero, so we must subtract one from the index to meet
      # this expectation.
      WAV_FILE=$(basename "$WAV_PATH")
      OLD_TRACK_NUM=$(echo "${WAV_FILE%.*}" | cut -d'_' -f2)
      NEW_TRACK_NUM=$(printf '%02d' $((${OLD_TRACK_NUM#0}-1)))
      NEW_FILE_NAME="homm2_$NEW_TRACK_NUM.flac"
      # Flac generates a lot noise on the error stream, even when it's working
      # as expected.  So we redirect the error output to quiet things down.
      flac --delete-input-file -8 "$WAV_PATH" -o "MUSIC/$NEW_FILE_NAME" 2> /dev/null
      echo_green "Completed ripping $NEW_FILE_NAME to 'MUSIC'."
    done

    stage_iso_assets "tmp_01.iso"
    rm "tmp_01.iso"
elif [[ "${CD_IMAGE_EXTENSION^^}" == "ISO" ]]; then
    stage_iso_assets "$HOMM2_CD_IMAGE_PATH"
else
    echo_red "Unsupported CD image extension '$CD_IMAGE_EXTENSION'."
    exit 1
fi

echo_stage "[4/4] copying game resources"

[[ ! -d "$DEST_PATH" ]] && mkdir -p "$DEST_PATH"

cd "$DEST_PATH"
cp -rf "$STAGE_PATH/." .
