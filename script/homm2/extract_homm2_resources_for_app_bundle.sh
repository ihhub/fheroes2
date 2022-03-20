#!/bin/zsh

set -e

PLATFORM_NAME="$(uname 2> /dev/null)"
if [[ "${PLATFORM_NAME}" != "Darwin" ]]; then
    echo "This script must be run on a macOS host"
    exit 1
fi

if [[ "$#" == "1" ]]; then
    HOMM2_PATH="$1"
else
    read "HOMM2_PATH?Please enter the full path to the HoMM2 directory (e.g. /Users/user/GOG Games/HoMM 2 Gold): "
fi

zsh "$(dirname "${(%):-%x}")/extract_homm2_resources.sh" "$HOMM2_PATH" "${HOME}/Library/Application Support/fheroes2"
