#!/bin/bash

if [ $# -ge 1 ] && [ "$1" = "-d" ]; then
    PACKAGES=(libsdl1.2-dev libsdl-ttf2.0-dev libsdl-mixer1.2-dev libsdl-image1.2-dev)
else
    PACKAGES=(libsdl1.2debian libsdl-ttf2.0-0 libsdl-mixer1.2 libsdl-image1.2)
fi

# Install SDL 1.2
sudo apt-get install -y "${PACKAGES[@]}"
