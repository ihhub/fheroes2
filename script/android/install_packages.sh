#!/usr/bin/env bash

set -e

PKG_FILE="android.zip"
PKG_FILE_SHA256="f33350acc50d23d4c7a7d8c34ed57fb0198f27861e1f7fcc70542868a92eee6f"
PKG_URL="https://github.com/fheroes2/fheroes2-prebuilt-deps/releases/download/android-deps/$PKG_FILE"

TMP_DIR="$(mktemp -d)"

if [[ "$(command -v wget)" != "" ]]; then
    wget -O "$TMP_DIR/$PKG_FILE" "$PKG_URL"
elif [[ "$(command -v curl)" != "" ]]; then
    curl -o "$TMP_DIR/$PKG_FILE" -L "$PKG_URL"
else
    echo "Neither wget nor curl were found in your system. Unable to download the package archive. Installation aborted."
    exit 1
fi

echo "$PKG_FILE_SHA256 *$PKG_FILE" > "$TMP_DIR/checksums"

if [[ "$(command -v shasum)" != "" ]]; then
    (cd "$TMP_DIR" && shasum --check --algorithm 256 checksums)
elif [[ "$(command -v sha256sum)" != "" ]]; then
    (cd "$TMP_DIR" && sha256sum --check --strict checksums)
else
    echo "Neither shasum nor sha256sum were found in your system. Unable to verify the downloaded file. Installation aborted."
    exit 1
fi

unzip -d "$(dirname "$0")/../../android" "$TMP_DIR/$PKG_FILE"
