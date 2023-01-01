#!/usr/bin/env bash

set -e

PKG_FILE="android.zip"
PKG_FILE_SHA256="187e2a8e09deeed8efbbc30e738a3192bf45d1d8644da85d53c487c0d27033a0"
PKG_URL="https://github.com/fheroes2/fheroes2-prebuilt-deps/releases/download/android-deps/$PKG_FILE"

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

unzip -d "$(dirname "$0")/../../android" "$TMP_DIR/$PKG_FILE"
