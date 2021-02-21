#!/bin/bash

# Uninstall SDL 1.2
sudo apt-get remove libsdl-image1.2-dev
sudo apt-get remove libsdl-mixer1.2-dev
sudo apt-get remove libsdl-ttf2.0-dev
sudo apt-get remove libsdl1.2-dev

# Install SDL 2
sudo apt-get install -y libsdl2-dev libsdl2-ttf-dev libsdl2-mixer-dev libsdl2-image-dev
