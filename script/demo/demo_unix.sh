#!/bin/bash

set -e

H2DEMO_URL="https://archive.org/download/HeroesofMightandMagicIITheSuccessionWars_1020/h2demo.zip"
H2DEMO_SHA256="12048c8b03875c81e69534a3813aaf6340975e77b762dc1b79a4ff5514240e3c"

if [[ "$(uname 2> /dev/null)" == "Linux" ]]; then
    FHEROES2_PATH="${XDG_CONFIG_HOME:-$HOME/.local/share}/fheroes2"
else
    FHEROES2_PATH="$HOME/.fheroes2"
fi

[[ ! -d "$FHEROES2_PATH/demo" ]] && mkdir -p "$FHEROES2_PATH/demo"

cd "$FHEROES2_PATH/demo"

if [[ "$(command -v wget)" != "" ]]; then
    wget "$H2DEMO_URL"
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

[[ ! -d "$FHEROES2_PATH/data" ]] && mkdir "$FHEROES2_PATH/data"
[[ ! -d "$FHEROES2_PATH/maps" ]] && mkdir "$FHEROES2_PATH/maps"

cp -r DATA/* "$FHEROES2_PATH/data"
cp -r MAPS/* "$FHEROES2_PATH/maps"
