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

# We might need to move packages into https://github.com/fheroes2/fheroes2-prebuilt-deps
# repository as we did for other packages.
PKG_NAME="release-2.32.10"
PKG_FILE="$PKG_NAME.zip"
PKG_FILE_SHA256="7a3c207b8509edc487d658df357ad764cd852d68fe248d307b25c0741d52fdf0"
PKG_URL="https://github.com/libsdl-org/SDL/archive/refs/tags/$PKG_FILE"

TMP_DIR="$(mktemp -d)"

if [[ -n "$(command -v wget)" ]]; then
    wget -O "$TMP_DIR/$PKG_FILE" "$PKG_URL"
elif [[ -n "$(command -v curl)" ]]; then
    curl -o "$TMP_DIR/$PKG_FILE" -L "$PKG_URL"
else
    echo "Neither wget nor curl were found in your system. Unable to download the package archive. Installation aborted."
    exit 1
fi

echo "$PKG_FILE_SHA256 *$PKG_FILE" > "$TMP_DIR/checksums"

if [[ -n "$(command -v shasum)" ]]; then
    (cd "$TMP_DIR" && shasum --check --algorithm 256 checksums)
elif [[ -n "$(command -v sha256sum)" ]]; then
    (cd "$TMP_DIR" && sha256sum --check --strict checksums)
else
    echo "Neither shasum nor sha256sum were found in your system. Unable to verify the downloaded file. Installation aborted."
    exit 1
fi

unzip -d "$(dirname "$0")/../../ios" "$TMP_DIR/$PKG_FILE"

mv "$(dirname "$0")/../../ios/SDL-$PKG_NAME" "$(dirname "$0")/../../ios/SDL2"

PKG_NAME="release-2.8.1"
PKG_FILE="$PKG_NAME.zip"
PKG_FILE_SHA256="3738827df73c86268dfa52898780769d1a796316d73b535e2ab5ff2d8d0ff44f"
PKG_URL="https://github.com/libsdl-org/SDL_mixer/archive/refs/tags/$PKG_FILE"

if [[ -n "$(command -v wget)" ]]; then
    wget -O "$TMP_DIR/$PKG_FILE" "$PKG_URL"
elif [[ -n "$(command -v curl)" ]]; then
    curl -o "$TMP_DIR/$PKG_FILE" -L "$PKG_URL"
else
    echo "Neither wget nor curl were found in your system. Unable to download the package archive. Installation aborted."
    exit 1
fi

echo "$PKG_FILE_SHA256 *$PKG_FILE" > "$TMP_DIR/checksums"

unzip -d "$(dirname "$0")/../../ios" "$TMP_DIR/$PKG_FILE"

mv "$(dirname "$0")/../../ios/SDL_Mixer-$PKG_NAME" "$(dirname "$0")/../../ios/SDL2_Mixer"

# Patch SDL_Mixer project.
# It is needed since we are trying to build SDL Mixer using the latest SDL2 version.
# Also, SDL Mixer doesn't support iPhone Simulator so we have to change this code or another one.
sed -i '' 's#$(SRCROOT)/$(PLATFORM)/SDL2.framework/Headers#$(SRCROOT)/../../SDL2/include#' \
    ios/SDL2_Mixer/Xcode/SDL_mixer.xcodeproj/project.pbxproj
