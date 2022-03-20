#!/bin/zsh

set -e

PLATFORM_NAME="$(uname 2> /dev/null)"
if [[ "${PLATFORM_NAME}" != "Darwin" ]]; then
    echo_red "This script must be run on a macOS host"
    exit 1
fi

zsh "$(dirname "${(%):-%x}")/download_demo_version.sh" "${HOME}/Library/Application Support/fheroes2"
