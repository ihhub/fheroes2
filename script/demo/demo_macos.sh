#!/bin/bash

mkdir demo
cd demo
curl -O -L https://archive.org/download/HeroesofMightandMagicIITheSuccessionWars_1020/h2demo.zip > h2demo.zip
unzip h2demo.zip
rm h2demo.zip

cp -r ./DATA ./../../../
cp -r ./MAPS ./../../../
