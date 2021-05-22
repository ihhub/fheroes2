#!/bin/bash

set -e

H2DEMO_URL="https://archive.org/download/HeroesofMightandMagicIITheSuccessionWars_1020/h2demo.zip"
H2DEMO_SHA256="12048c8b03875c81e69534a3813aaf6340975e77b762dc1b79a4ff5514240e3c"

[[ ! -d demo ]] && mkdir demo
cd demo

if [[ "$(which wget 2> /dev/null)" != "" ]]; then
    wget "$H2DEMO_URL"
elif [[ "$(which curl 2> /dev/null)" != "" ]]; then
    curl -O -L "$H2DEMO_URL" > h2demo.zip
else
    echo "wget or curl not found in your system. Unable to download demo version. Installation aborted."
    exit 1
fi

echo "$H2DEMO_SHA256 *h2demo.zip" > checksums

if [[ "$(which shasum 2> /dev/null)" != "" ]]; then
    shasum --check --algorithm 256 checksums
elif [[ "$(which sha256sum 2> /dev/null)" != "" ]]; then
    sha256sum --check --strict checksums
else
    echo "shasum or sha256sum not found in your system. Unable to verify downloaded file. Installation aborted."
    exit 1
fi

unzip -o h2demo.zip

[[ ! -d ../../../data ]] && mkdir ../../../data
[[ ! -d ../../../maps ]] && mkdir ../../../maps

cp -r DATA/* ../../../data
cp -r MAPS/* ../../../maps
