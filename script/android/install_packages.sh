#!/usr/bin/env bash

###########################################################################
#   fheroes2: https://github.com/ihhub/fheroes2                           #
#   Copyright (C) 2022 - 2026                                             #
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

PKG_FILE="android.zip"
PKG_FILE_SHA256="4a34d7cb11eeb31a16107b0d8e56ec2d487803ec618080bd5944834a4fe11c40"
PKG_URL="https://github.com/fheroes2/fheroes2-prebuilt-deps/releases/download/android-deps/$PKG_FILE"

# Network games use the standalone header-only version of asio
ASIO_VERSION="1-38-2"
ASIO_FILE="asio-$ASIO_VERSION.zip"
ASIO_FILE_SHA256="1907e462b736f3b682e1539c6d7a8ececde66dcb25fe98c8386701c101a93388"
ASIO_URL="https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-$ASIO_VERSION.zip"

TMP_DIR="$(mktemp -d)"

function download_file
{
    local URL="$1"
    local FILE="$2"

    if [[ -n "$(command -v wget)" ]]; then
        wget -O "$TMP_DIR/$FILE" "$URL"
    elif [[ -n "$(command -v curl)" ]]; then
        curl -o "$TMP_DIR/$FILE" -L "$URL"
    else
        echo "Neither wget nor curl were found in your system. Unable to download the package archive. Installation aborted."
        exit 1
    fi
}

download_file "$PKG_URL" "$PKG_FILE"
download_file "$ASIO_URL" "$ASIO_FILE"

echo "$PKG_FILE_SHA256 *$PKG_FILE" > "$TMP_DIR/checksums"
echo "$ASIO_FILE_SHA256 *$ASIO_FILE" >> "$TMP_DIR/checksums"

if [[ -n "$(command -v shasum)" ]]; then
    (cd "$TMP_DIR" && shasum --check --algorithm 256 checksums)
elif [[ -n "$(command -v sha256sum)" ]]; then
    (cd "$TMP_DIR" && sha256sum --check --strict checksums)
else
    echo "Neither shasum nor sha256sum were found in your system. Unable to verify the downloaded file. Installation aborted."
    exit 1
fi

unzip -d "$(dirname "$0")/../../android" "$TMP_DIR/$PKG_FILE"

ASIO_DST_DIR="$(dirname "$0")/../../android/app/jni/asio"

unzip -d "$TMP_DIR" "$TMP_DIR/$ASIO_FILE"
rm -rf "$ASIO_DST_DIR"
mkdir -p "$ASIO_DST_DIR"
cp -R "$TMP_DIR/asio-asio-$ASIO_VERSION/include" "$TMP_DIR/asio-asio-$ASIO_VERSION/LICENSE_1_0.txt" "$ASIO_DST_DIR"
