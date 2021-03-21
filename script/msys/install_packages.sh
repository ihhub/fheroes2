#!/usr/bin/bash

echo "Installing missing packages"
pacman -Syu --noconfirm --needed base-devel git {un,}zip ${MINGW_PACKAGE_PREFIX}-{toolchain,SDL{,2}_mixer,SDL{,2}_ttf}