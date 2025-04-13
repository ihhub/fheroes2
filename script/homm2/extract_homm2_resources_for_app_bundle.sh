#!/usr/bin/env bash

###########################################################################
#   fheroes2: https://github.com/ihhub/fheroes2                           #
#   Copyright (C) 2022 - 2025                                             #
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

# Purpose: This script extracts resources from a Heroes of Might and Magic II
# directory or installer and copies them to the appropriate location for the fheroes2
# macOS app bundle.
#
# Usage: ./extract_homm2_resources_for_app_bundle.sh [HOMM2_PATH]
#   HOMM2_PATH: (Optional) Path to the HoMM2 installation directory or installer package.
#               If not provided, the script will interactively prompt for the path.
#
# Example: ./extract_homm2_resources_for_app_bundle.sh /path/to/homm2
#
# Note: This script must be run on a macOS system as it is designed for macOS app bundle.

set -e

PLATFORM_NAME="$(uname 2> /dev/null)"
if [[ "${PLATFORM_NAME}" != "Darwin" ]]; then
    echo "This script must be run on a macOS host"
    exit 1
fi

if [[ "$#" == "1" ]]; then
    HOMM2_PATH="$1"
else
    # Use read with -e flag for built-in tab completion
    read -e -p "Please enter the full path to the HoMM2 directory or installer package (e.g. /home/user/homm2 or /tmp/installer.exe): " HOMM2_PATH
fi

# Expand the path to handle ~ and relative paths
if [[ "${HOMM2_PATH}" == ~* ]]; then
    # Use bash's built-in tilde expansion
    HOMM2_PATH="${HOMM2_PATH/#\~/$HOME}"
fi

# Verify the path exists
if [[ ! -e "${HOMM2_PATH}" ]]; then
    echo "Error: The specified path does not exist: ${HOMM2_PATH}"
    exit 1
fi

# Define user data directory
USER_DATA_DIR="${HOME}/Library/Application Support/fheroes2"

# Get the directory where this script is located
SCRIPT_DIR=$(dirname "$0")

# Call the resource extraction script for user data directory
echo "Extracting resources to user data directory..."
"${SCRIPT_DIR}/extract_homm2_resources.sh" "${HOMM2_PATH}" "${USER_DATA_DIR}"

echo "Resource extraction complete. Files have been copied to: ${USER_DATA_DIR}"
