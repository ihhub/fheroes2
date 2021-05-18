#!/bin/bash

mkdir demo
cd demo
wget https://archive.org/download/HeroesofMightandMagicIITheSuccessionWars_1020/h2demo.zip
unzip h2demo.zip
rm h2demo.zip

cp -r ./DATA ./../../../data
cp -r ./MAPS ./../../../maps
