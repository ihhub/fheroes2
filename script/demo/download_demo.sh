#!/bin/bash

echo "Downloading demo..."
curl -s -O -L https://archive.org/download/HeroesofMightandMagicIITheSuccessionWars_1020/h2demo.zip
echo "Extracting necessary files"
unzip -o -qq h2demo.zip "*DATA*" "*MAPS*"
rm -f h2demo.zip
echo "All done!"
