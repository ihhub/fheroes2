#!/usr/bin/bash

echo "Installing missing packages"
pacman -S --noconfirm --needed base-devel p7zip ${MINGW_PACKAGE_PREFIX}-{toolchain,SDL{,2}_mixer}
