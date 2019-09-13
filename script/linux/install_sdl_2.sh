#!/bin/bash

# Uninstall SDL 1.2
sudo apt-get remove libsdl-image1.2-dev
sudo apt-get remove libsdl-mixer1.2-dev
sudo apt-get remove libsdl-ttf2.0-dev
sudo apt-get remove libsdl1.2-dev

# Install SDL 2
sudo apt-get install -y libsdl2-dev
sudo apt-get install -y libsdl2-ttf-dev
sudo apt-get install -y libsdl2-mixer-dev
sudo apt-get install -y libsdl2-image-dev
