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
set -e

PLATFORM_NAME="$(uname 2> /dev/null)"
if [[ "${PLATFORM_NAME}" != "Darwin" ]]; then
    echo "This script must be run on a macOS host"
    exit 1
fi

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

echo "The following directories will be removed:"
echo "1. ${HOME}/Library/Preferences/fheroes2"
[[ -d "${HOME}/Library/Preferences/fheroes2" ]] && echo "   - Contains $(ls -A "${HOME}/Library/Preferences/fheroes2" ! -type d | wc -l) files"

echo "2. ${HOME}/Library/Application Support/fheroes2"
[[ -d "${HOME}/Library/Application Support/fheroes2" ]] && echo "   - Contains $(ls -A "${HOME}/Library/Application Support/fheroes2" ! -type d | wc -l) files"

echo "3. ${REPO_ROOT}/fheroes2.app"
[[ -d "${REPO_ROOT}/fheroes2.app" ]] && echo "   - Contains $(ls -A "${REPO_ROOT}/fheroes2.app" ! -type d | wc -l) files"
echo

echo "Removing preferences directory..."
rm -rf "${HOME}/Library/Preferences/fheroes2"

echo "Removing application support directory..."
rm -rf "${HOME}/Library/Application Support/fheroes2"

echo "Cleanup complete"
