#!/bin/bash

set -e

H2DEMO_URL="https://archive.org/download/HeroesofMightandMagicIITheSuccessionWars_1020/h2demo.zip"
H2DEMO_SHA256="12048c8b03875c81e69534a3813aaf6340975e77b762dc1b79a4ff5514240e3c"

FHEROES2_PATH="."

if [[ ! -f fheroes2 && -d ../../src ]]; then
    FHEROES2_PATH="../.."
fi

if ! touch "$FHEROES2_PATH/.test-writable" 2> /dev/null || ! rm "$FHEROES2_PATH/.test-writable" 2>/dev/null; then
    if [[ "$(uname 2> /dev/null)" == "Linux" ]]; then
        FHEROES2_PATH="${XDG_CONFIG_HOME:-$HOME/.local/share}/fheroes2"
    else
        FHEROES2_PATH="$HOME/.fheroes2"
    fi
fi

echo "Destination directory: $FHEROES2_PATH"

[[ ! -d "$FHEROES2_PATH/demo" ]] && mkdir -p "$FHEROES2_PATH/demo"

cd "$FHEROES2_PATH/demo"

if [[ "$(command -v wget)" != "" ]]; then
    wget -O h2demo.zip "$H2DEMO_URL"
elif [[ "$(command -v curl)" != "" ]]; then
    curl -O -L "$H2DEMO_URL" > h2demo.zip
else
    echo "wget or curl not found in your system. Unable to download demo version. Installation aborted."
    exit 1
fi

echo "$H2DEMO_SHA256 *h2demo.zip" > checksums

if [[ "$(command -v shasum)" != "" ]]; then
    shasum --check --algorithm 256 checksums
elif [[ "$(command -v sha256sum)" != "" ]]; then
    sha256sum --check --strict checksums
else
    echo "shasum or sha256sum not found in your system. Unable to verify downloaded file. Installation aborted."
    exit 1
fi

unzip -o h2demo.zip

[[ ! -d ../data ]] && mkdir ../data
[[ ! -d ../maps ]] && mkdir ../maps

cp -r DATA/* ../data
cp -r MAPS/* ../maps
