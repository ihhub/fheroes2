#!/bin/bash

# Uninstall SDL 2
sudo apt-get remove libsdl2-image-dev
sudo apt-get remove libsdl2-mixer-dev
sudo apt-get remove libsdl2-ttf-dev
sudo apt-get remove libsdl2-dev

# Install SDL 1.2
sudo apt-get install -y libsdl1.2-dev libsdl-ttf2.0-dev libsdl-mixer1.2-dev libsdl-image1.2-dev
