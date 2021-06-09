#!/bin/bash

if [ $# -ge 1 ] && [ "$1" = "-d" ]; then
    PACKAGES=(libsdl2-dev libsdl2-ttf-dev libsdl2-mixer-dev libsdl2-image-dev)
else
    PACKAGES=(libsdl2-2.0-0 libsdl2-ttf-2.0-0 libsdl2-mixer-2.0-0 libsdl2-image-2.0-0)
fi

# Install SDL 2
sudo apt-get install -y "${PACKAGES[@]}"
