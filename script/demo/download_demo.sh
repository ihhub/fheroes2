#!/bin/bash

if ! [ '$(command -v wget)' ]; then
	get='curl > demo.zip -L'
else
	get='wget -O demo.zip'
fi


echo 'Downloading demo...'
eval $get https://archive.org/download/HeroesofMightandMagicIITheSuccessionWars_1020/h2demo.zip
echo 'Extracting necessary files...'
unzip -o -qq demo.zip DATA/* MAPS/*
rm -f demo.zip
echo 'All done!'
